/*
 *  The Grace Programming Language.
 *
 *  This file contains the out of line definitions for the Compiler class, which outputs Grace bytecode based on Tokens provided by the Scanner. 
 *  
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#include <charconv>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stack>
#include <unordered_map>
#include <variant>

#ifdef GRACE_MSC
# include <stdlib.h>
#endif

#include <fmt/color.h>
#include <fmt/core.h>

#include "compiler.hpp"
#include "scanner.hpp"

using namespace Grace;

enum class CodeContext
{
  Catch,
  Function,
  If,
  ForLoop,
  TopLevel,
  Try,
  WhileLoop,
};

struct Local
{
  std::string name;
  bool isFinal, isIterator;
  std::int64_t index;

  Local(std::string&& name, bool final, bool iterator, std::int64_t index)
    : name(std::move(name)), isFinal(final), isIterator(iterator), index(index)
  {

  }
};

struct CompilerContext
{
  CompilerContext(const std::string& fileName, std::string&& code)
    : currentFileName(fileName)
  {
    Scanner::InitScanner(fileName, std::move(code));
    codeContextStack.push_back(CodeContext::TopLevel);
  }

  ~CompilerContext()
  {
    Scanner::PopScanner();
  }

  std::vector<CodeContext> codeContextStack;
  std::string currentFileName;

  std::optional<Scanner::Token> current, previous;
  std::vector<Local> locals;

  bool panicMode = false, hadError = false, hadWarning = false;

  bool passedImports = false;

  bool namespaceQualifierUsed = true;

  bool usingExpressionResult = false;

  bool continueJumpNeedsIndexes = false;
  bool breakJumpNeedsIndexes = false;

  // const idx, op idx
  using IndexStack = std::stack<std::vector<std::pair<std::int64_t, std::int64_t>>>;
  IndexStack breakIdxPairs, continueIdxPairs;
};

GRACE_NODISCARD static bool Match(Scanner::TokenType expected, CompilerContext& compiler);
GRACE_NODISCARD static bool Check(Scanner::TokenType expected, CompilerContext& compiler);

static void Consume(Scanner::TokenType expected, const std::string& message, CompilerContext& compiler);
static void Synchronize(CompilerContext& compiler);

GRACE_INLINE static void EmitOp(VM::Ops op, std::size_t line)
{
  VM::VM::GetInstance().PushOp(op, line);
}
template<typename T>
GRACE_INLINE static void EmitConstant(const T& value)
{
  VM::VM::GetInstance().PushConstant(value);
}

static void Advance(CompilerContext& compiler);

static void Declaration(CompilerContext& compiler);
static void ImportDeclaration(CompilerContext& compiler);
static void ClassDeclaration(CompilerContext& compiler);
static void FuncDeclaration(CompilerContext& compiler);
static void VarDeclaration(CompilerContext& compiler);
static void FinalDeclaration(CompilerContext& compiler);

static void Statement(CompilerContext& compiler);
static void ExpressionStatement(CompilerContext& compiler);
static void AssertStatement(CompilerContext& compiler);
static void BreakStatement(CompilerContext& compiler);
static void ContinueStatement(CompilerContext& compiler);
static void ForStatement(CompilerContext& compiler);
static void IfStatement(CompilerContext& compiler);
static void PrintStatement(CompilerContext& compiler);
static void PrintLnStatement(CompilerContext& compiler);
static void EPrintStatement(CompilerContext& compiler);
static void EPrintLnStatement(CompilerContext& compiler);
static void ReturnStatement(CompilerContext& compiler);
static void TryStatement(CompilerContext& compiler);
static void ThrowStatement(CompilerContext& compiler);
static void WhileStatement(CompilerContext& compiler);

static void Expression(bool canAssign, CompilerContext& compiler);

static void Or(bool canAssign, bool skipFirst, CompilerContext& compiler);
static void And(bool canAssign, bool skipFirst, CompilerContext& compiler);
static void BitwiseOr(bool canAssign, bool skipFirst, CompilerContext& compiler);
static void BitwiseXOr(bool canAssign, bool skipFirst, CompilerContext& compiler);
static void BitwiseAnd(bool canAssign, bool skipFirst, CompilerContext& compiler);
static void Equality(bool canAssign, bool skipFirst, CompilerContext& compiler);
static void Comparison(bool canAssign, bool skipFirst, CompilerContext& compiler);
static void Shift(bool canAssign, bool skipFirst, CompilerContext& compiler);
static void Term(bool canAssign, bool skipFirst, CompilerContext& compiler);
static void Factor(bool canAssign, bool skipFirst, CompilerContext& compiler);
static void Unary(bool canAssign, CompilerContext& compiler);
static void Call(bool canAssign, CompilerContext& compiler);
static void Primary(bool canAssign, CompilerContext& compiler);

static void Identifier(bool canAssign, CompilerContext& compiler);
static void Char(CompilerContext& compiler);
static void String(CompilerContext& compiler);
static void InstanceOf(CompilerContext& compiler);
static void IsObject(CompilerContext& compiler);
static void Cast(CompilerContext& compiler);
static void List(CompilerContext& compiler);
static void Dictionary(CompilerContext& compiler);
static void Typename(CompilerContext& compiler);

enum class LogLevel
{
  Warning, Error,
};

static void MessageAtCurrent(const std::string& message, LogLevel level, CompilerContext& compiler);
static void MessageAtPrevious(const std::string& message, LogLevel level, CompilerContext& compiler);
static void Message(const std::optional<Scanner::Token>& token, const std::string& message, LogLevel level, CompilerContext& compiler);

GRACE_NODISCARD static VM::InterpretResult Finalise(const std::string& mainFileName, bool verbose, const std::vector<std::string>& args);

static bool s_Verbose, s_WarningsError;
static std::stack<CompilerContext> s_CompilerContextStack;

VM::InterpretResult Grace::Compiler::Compile(const std::string& fileName, std::string&& code, bool verbose, bool warningsError, const std::vector<std::string>& args)
{
  using namespace std::chrono;

  auto start = steady_clock::now();

  s_Verbose = verbose;
  s_WarningsError = warningsError;
 
  s_CompilerContextStack.emplace(fileName, std::move(code));
  
  Advance(s_CompilerContextStack.top());
  
  auto hadError = false, hadWarning = false;
  while (!s_CompilerContextStack.empty()) {
    if (Match(Scanner::TokenType::EndOfFile, s_CompilerContextStack.top())) {
      s_CompilerContextStack.pop();
      continue;
    }
    Declaration(s_CompilerContextStack.top());
    hadError = s_CompilerContextStack.top().hadError;
    hadWarning = s_CompilerContextStack.top().hadWarning;
    if (s_CompilerContextStack.top().hadError) {
      break;
    }
  }

  while (!s_CompilerContextStack.empty()) {
    s_CompilerContextStack.pop();
  }

  if (hadError) {
    fmt::print(stderr, "Terminating process due to compilation errors.\n");
  } else if (hadWarning && warningsError) {
    fmt::print(stderr, "Terminating process due to compilation warnings treated as errors.\n");
  } else {
    if (verbose) {
      auto end = steady_clock::now();
      auto duration = duration_cast<microseconds>(end - start).count();
      if (duration > 1000) {
        fmt::print("Compilation succeeded in {} ms.\n", duration_cast<milliseconds>(end - start).count());
      } else {
        fmt::print("Compilation succeeded in {} μs.\n", duration);
      }
    }
    return Finalise(fileName, verbose, args);
  }

  return VM::InterpretResult::RuntimeError;
}

static VM::InterpretResult Finalise(const std::string& mainFileName, bool verbose, const std::vector<std::string>& args)
{
 #ifdef GRACE_DEBUG
   if (verbose) {
     VM::VM::GetInstance().PrintOps();
   }
 #endif
   if (VM::VM::GetInstance().CombineFunctions(mainFileName, verbose)) {
     return VM::VM::GetInstance().Start(mainFileName, verbose, args);
   }
  return VM::InterpretResult::RuntimeError;
}

static void Advance(CompilerContext& compiler)
{
  compiler.previous = compiler.current;
  compiler.current = Scanner::ScanToken();

#ifdef GRACE_DEBUG
  if (s_Verbose) {
    fmt::print("{}\n", compiler.current.value().ToString());
  }
#endif 

  if (compiler.current.value().GetType() == Scanner::TokenType::Error) {
    MessageAtCurrent("Unexpected token", LogLevel::Error, compiler);
  }
}

static bool Match(Scanner::TokenType expected, CompilerContext& compiler)
{
  if (!Check(expected, compiler)) {
    return false;
  }

  Advance(compiler);
  return true;
}

static bool Check(Scanner::TokenType expected, CompilerContext& compiler)
{
  return compiler.current.has_value() && compiler.current.value().GetType() == expected;
}

static void Consume(Scanner::TokenType expected, const std::string& message, CompilerContext& compiler)
{
  if (compiler.current.value().GetType() == expected) {
    Advance(compiler);
    return;
  }

  MessageAtCurrent(message, LogLevel::Error, compiler);
}

static void Synchronize(CompilerContext& compiler)
{
  compiler.panicMode = false;

  while (compiler.current.value().GetType() != Scanner::TokenType::EndOfFile) {
    if (compiler.previous.has_value() && compiler.previous.value().GetType() == Scanner::TokenType::Semicolon) {
      return;
    }

    switch (compiler.current.value().GetType()) {
      case Scanner::TokenType::Class:
      case Scanner::TokenType::Func:
      case Scanner::TokenType::Final:
      case Scanner::TokenType::For:
      case Scanner::TokenType::If:
      case Scanner::TokenType::While:
      case Scanner::TokenType::Print:
      case Scanner::TokenType::PrintLn:
      case Scanner::TokenType::Eprint:
      case Scanner::TokenType::EprintLn:
      case Scanner::TokenType::Return:
      case Scanner::TokenType::Var:
        return;
      default:
        break;
    }

    Advance(compiler);
  }
}

static bool IsKeyword(Scanner::TokenType type, std::string& outKeyword)
{
  switch (type) {
    case Scanner::TokenType::And: outKeyword = "and"; return true;
    case Scanner::TokenType::By: outKeyword = "by"; return true;
    case Scanner::TokenType::Catch: outKeyword = "catch"; return true;
    case Scanner::TokenType::Class: outKeyword = "class"; return true;
    case Scanner::TokenType::End: outKeyword = "end"; return true;
    case Scanner::TokenType::Final: outKeyword = "final"; return true;
    case Scanner::TokenType::For: outKeyword = "for"; return true;
    case Scanner::TokenType::Func: outKeyword = "func"; return true;
    case Scanner::TokenType::If: outKeyword = "if"; return true;
    case Scanner::TokenType::In: outKeyword = "in"; return true;
    case Scanner::TokenType::Or: outKeyword = "or"; return true;
    case Scanner::TokenType::Print: outKeyword = "print"; return true;
    case Scanner::TokenType::PrintLn: outKeyword = "println"; return true;
    case Scanner::TokenType::Eprint: outKeyword = "eprint"; return true;
    case Scanner::TokenType::EprintLn: outKeyword = "eprintln"; return true;
    case Scanner::TokenType::Export: outKeyword = "export"; return true;
    case Scanner::TokenType::Return: outKeyword = "return"; return true;
    case Scanner::TokenType::Throw: outKeyword = "throw"; return true;
    case Scanner::TokenType::Try: outKeyword = "try"; return true;
    case Scanner::TokenType::Var: outKeyword = "var"; return true;
    case Scanner::TokenType::While: outKeyword = "while"; return true;
    default:
      return false;
  }
}

static bool IsOperator(Scanner::TokenType type)
{
  static const std::vector<Scanner::TokenType> symbols {
    Scanner::TokenType::Colon,
    Scanner::TokenType::Semicolon,
    Scanner::TokenType::RightParen,
    Scanner::TokenType::Comma,
    Scanner::TokenType::Dot,
    Scanner::TokenType::DotDot,
    Scanner::TokenType::Plus,
    Scanner::TokenType::Slash,
    Scanner::TokenType::Star,
    Scanner::TokenType::StarStar,
    Scanner::TokenType::BangEqual,
    Scanner::TokenType::Equal,
    Scanner::TokenType::EqualEqual,
    Scanner::TokenType::LessThan,
    Scanner::TokenType::GreaterThan,
    Scanner::TokenType::LessEqual,
    Scanner::TokenType::GreaterEqual,
    Scanner::TokenType::Bar,
    Scanner::TokenType::Ampersand,
    Scanner::TokenType::Caret,
    Scanner::TokenType::ShiftRight,
    Scanner::TokenType::ShiftLeft,
  };

  return std::any_of(symbols.begin(), symbols.end(), [type](Scanner::TokenType t) {
    return t == type;
  });
}

static void Declaration(CompilerContext& compiler)
{
  if (Match(Scanner::TokenType::Import, compiler)) {
    ImportDeclaration(compiler);
  } else if (Match(Scanner::TokenType::Class, compiler)) {
    compiler.passedImports = true;
    ClassDeclaration(compiler);
  } else if (Match(Scanner::TokenType::Func, compiler)) {
    compiler.passedImports = true;
    FuncDeclaration(compiler);
  } else if (Match(Scanner::TokenType::Var, compiler)) {
    compiler.passedImports = true;
    VarDeclaration(compiler);
  } else if (Match(Scanner::TokenType::Final, compiler)) {
    compiler.passedImports = true;
    FinalDeclaration(compiler);    
  } else {
    Statement(compiler);
  }
 
  if (compiler.panicMode) {
    Synchronize(compiler);
  }
}

static void Statement(CompilerContext& compiler)
{
  if (compiler.codeContextStack.back() == CodeContext::TopLevel) {
    MessageAtCurrent("Only functions and classes are allowed at top level", LogLevel::Error, compiler);
    return;
  }

  if (Match(Scanner::TokenType::For, compiler)) {
    ForStatement(compiler);
  } else if (Match(Scanner::TokenType::If, compiler)) {
    IfStatement(compiler);
  } else if (Match(Scanner::TokenType::Print, compiler)) {
    PrintStatement(compiler);
  } else if (Match(Scanner::TokenType::PrintLn, compiler)) {
    PrintLnStatement(compiler);
  } else if (Match(Scanner::TokenType::Eprint, compiler)) {
    EPrintStatement(compiler);
  } else if (Match(Scanner::TokenType::EprintLn, compiler)) {
    EPrintLnStatement(compiler);
  } else if (Match(Scanner::TokenType::Return, compiler)) {
    ReturnStatement(compiler);
  } else if (Match(Scanner::TokenType::While, compiler)) {
    WhileStatement(compiler);
  } else if (Match(Scanner::TokenType::Try, compiler)) {
    TryStatement(compiler);
  } else if (Match(Scanner::TokenType::Throw, compiler)) {
    ThrowStatement(compiler);
  } else if (Match(Scanner::TokenType::Assert, compiler)) {
    AssertStatement(compiler);
  } else if (Match(Scanner::TokenType::Break, compiler)) {
    BreakStatement(compiler);
  } else if (Match(Scanner::TokenType::Continue, compiler)) {
    ContinueStatement(compiler);
  } else if (Check(Scanner::TokenType::Catch, compiler)) {
    if (compiler.codeContextStack.back() != CodeContext::Try) {
      MessageAtCurrent("`catch` block only allowed after `try` block", LogLevel::Error, compiler);
      Advance(compiler);  // consume illegal catch
      return;
    }
  } else {
    ExpressionStatement(compiler);
  }
}

static bool IsValidTypeAnnotation(const Scanner::TokenType& token)
{
  static const std::vector<Scanner::TokenType> valid = {
    Scanner::TokenType::Identifier,
    Scanner::TokenType::IntIdent,
    Scanner::TokenType::FloatIdent,
    Scanner::TokenType::BoolIdent,
    Scanner::TokenType::CharIdent,
    Scanner::TokenType::Null,
    Scanner::TokenType::StringIdent,
    Scanner::TokenType::ListIdent,
    Scanner::TokenType::DictIdent,
  };
  return std::any_of(valid.begin(), valid.end(), [token] (Scanner::TokenType t) {
    return t == token;
  });
}

static void ImportDeclaration(CompilerContext& compiler)
{
  if (compiler.passedImports) {
    MessageAtPrevious("`import` only allowed before any other declarations", LogLevel::Error, compiler);
    return;
  }

  std::optional<Scanner::Token> lastPathToken;
  std::optional<bool> isStdImport = std::nullopt;
  std::string importPath;
  while (true) {
    if (!Match(Scanner::TokenType::Identifier, compiler)) {
      MessageAtCurrent("Expected path", LogLevel::Error, compiler);
      return;
    }
    auto txt = compiler.previous.value().GetText();
    if (!isStdImport.has_value() && txt == "std") {
      isStdImport = true;
    }
    importPath.append(txt);
    lastPathToken = compiler.previous;
    if (Match(Scanner::TokenType::Semicolon, compiler)) {
      importPath.append(".gr");
      break;
    }
    if (Match(Scanner::TokenType::EndOfFile, compiler)) {
      MessageAtPrevious("Unterminated `import` statement", LogLevel::Error, compiler);
      return;
    }
    Consume(Scanner::TokenType::ColonColon, "Expected `::` for path continuation", compiler);
    importPath.push_back('/');
  }

  namespace fs = std::filesystem;

  fs::path inPath;
  // if it was an std import, get the std path env variable and look there
  if (isStdImport.has_value()) {
    std::string stdPath;

#ifdef GRACE_MSC
    std::size_t size;
    getenv_s(&size, NULL, 0, "GRACE_STD_PATH");
    if (size == 0) {
      fmt::print(stderr, "The `GRACE_STD_PATH` environment variable has not been set, so cannot continue importing file {}\n", importPath);
      return;
    }
    char* libPath = (char*)std::malloc(size * sizeof(char));
    getenv_s(&size, libPath, size, "GRACE_STD_PATH");
    if (libPath == NULL) {
      fmt::print(stderr, "Failed to retrive the `GRACE_STD_PATH` environment variable, so cannot continue importing file {}\n", importPath);
      return;
    }
    stdPath = libPath;
    std::free(libPath);
#else
    char* pathPtr = std::getenv("GRACE_STD_PATH");
    if (pathPtr == nullptr) {
      fmt::print(stderr, "The `GRACE_STD_PATH` environment variable has not been set, so cannot continue importing file {}\n", importPath);
      return;
    }
    stdPath = pathPtr;
#endif

    inPath = fs::path(stdPath) / fs::path(importPath.substr(4));  // trim off 'std/' because that's contained within the path environment variable
  } else {
    auto basePath = fs::path(compiler.currentFileName).parent_path();
    inPath = basePath / fs::path(importPath);
  }

  if (!fs::exists(inPath)) {
    Message(lastPathToken, fmt::format("Could not find file `{}` to import", importPath), LogLevel::Error, compiler);
    return;
  }

  if (Scanner::HasFile(importPath)) {
    // don't produce a warning, but silently ignore the duplicate import
    // TODO: maybe ignore this in the std, but warn the user if they have duplicate imports in one file?
    return;
  }

  std::stringstream inFileStream;
  try {
    std::ifstream inFile(inPath);
    inFileStream << inFile.rdbuf();
  } catch (const std::exception& e) {
    fmt::print(stderr, "Error reading imported file `{}`: {}. This import will be ignored.\n", inPath.string(), e.what());
    return;
  }

  s_CompilerContextStack.emplace(std::move(importPath), inFileStream.str());
  Advance(s_CompilerContextStack.top());
}

static void ClassDeclaration(GRACE_MAYBE_UNUSED CompilerContext& compiler)
{
  GRACE_NOT_IMPLEMENTED();
}

static void FuncDeclaration(CompilerContext& compiler)
{
  for (auto it = compiler.codeContextStack.crbegin(); it != compiler.codeContextStack.crend(); ++it) {
    if (*it == CodeContext::Function) {
      MessageAtPrevious("Nested functions are not permitted, prefer lambdas", LogLevel::Error, compiler);
      return;
    }
  }

  auto exportFunction = false;
  compiler.codeContextStack.emplace_back(CodeContext::Function);

  if (Match(Scanner::TokenType::Export, compiler)) {
    exportFunction = true;
  }

  Consume(Scanner::TokenType::Identifier, "Expected function name", compiler);
  auto funcNameToken = compiler.previous;
  auto name = std::string(compiler.previous.value().GetText());
  if (name.starts_with("__")) {
    MessageAtPrevious("Function names beginning with double underscore `__` are reserved for internal use", LogLevel::Error, compiler);
    return;
  }
  auto isMainFunction = name == "main";

  Consume(Scanner::TokenType::LeftParen, "Expected '(' after function name", compiler);

  std::vector<std::string> parameters;
  while (!Match(Scanner::TokenType::RightParen, compiler)) {
    if (isMainFunction && parameters.size() > 1) {
      Message(funcNameToken, fmt::format("`main` function can only take 0 or 1 parameter(s) but got {}", parameters.size()), LogLevel::Error, compiler);
      return;
    }

    if (Match(Scanner::TokenType::Final, compiler)) {
      Consume(Scanner::TokenType::Identifier, "Expected identifier after `final`", compiler);
      auto p = std::string(compiler.previous.value().GetText());
      if (std::find(parameters.begin(), parameters.end(), p) != parameters.end()) {
        MessageAtPrevious("Function parameters with the same name already defined", LogLevel::Error, compiler);
        return;
      }
      parameters.push_back(p);
      compiler.locals.emplace_back(std::move(p), true, false, compiler.locals.size());

      if (Match(Scanner::TokenType::Colon, compiler)) {
        if (!IsValidTypeAnnotation(compiler.current.value().GetType())) {
          MessageAtCurrent("Expected type name after type annotation", LogLevel::Error, compiler);
          return;
        }
        Advance(compiler);
      }

      if (!Check(Scanner::TokenType::RightParen, compiler)) {
        Consume(Scanner::TokenType::Comma, "Expected ',' after function parameter", compiler);
      }
    } else if (Match(Scanner::TokenType::Identifier, compiler)) {
      auto p = std::string(compiler.previous.value().GetText());
      if (std::find(parameters.begin(), parameters.end(), p) != parameters.end()) {
        MessageAtPrevious("Function parameters with the same name already defined", LogLevel::Error, compiler);
        return;
      }

      parameters.push_back(p);
      compiler.locals.emplace_back(std::move(p), false, false, compiler.locals.size());

      if (Match(Scanner::TokenType::Colon, compiler)) {
        if (!IsValidTypeAnnotation(compiler.current.value().GetType())) {
          MessageAtCurrent("Expected type name after type annotation", LogLevel::Error, compiler);
          return;
        }
        Advance(compiler);
      }

      if (!Check(Scanner::TokenType::RightParen, compiler)) {
        Consume(Scanner::TokenType::Comma, "Expected ',' after function parameter", compiler);
      }
    } else {
      MessageAtCurrent("Expected identifier or `final`", LogLevel::Error, compiler);
      return;
    } 
  }

  if (Match(Scanner::TokenType::ColonColon, compiler)) {
    if (isMainFunction) {
      MessageAtPrevious("`main` does not return a value", LogLevel::Error, compiler);
      return;
    }
    if (!IsValidTypeAnnotation(compiler.current.value().GetType())) {
      MessageAtCurrent("Expected type name after type annotation", LogLevel::Error, compiler);
      return;
    }
    Advance(compiler);
  }

  // cl args are still assigned in the VM if the user doesn't state it
  // so the compiler needs to be aware of that
  if (isMainFunction && parameters.size() == 0) {
    compiler.locals.emplace_back("args", true, false, 0);
  }

  Consume(Scanner::TokenType::Colon, "Expected ':' after function signature", compiler);

  if (!VM::VM::GetInstance().AddFunction(std::move(name), compiler.previous.value().GetLine(), parameters.size(), compiler.currentFileName, exportFunction)) {
    Message(funcNameToken, "Duplicate function definitions", LogLevel::Error, compiler);
    return;
  }

  while (!Match(Scanner::TokenType::End, compiler)) {
    Declaration(compiler);
    if (compiler.current.value().GetType() == Scanner::TokenType::EndOfFile) {
      MessageAtCurrent("Expected `end` after function", LogLevel::Error, compiler);
      return;
    }
  }

  // implicitly return if the user didn't write a return so the VM knows to return to the caller
  // functions with no return will implicitly return null, so setting a call equal to a variable is valid
  if (VM::VM::GetInstance().GetLastOp() != VM::Ops::Return) {
    if (!compiler.locals.empty()) {
      EmitConstant(std::int64_t{0});
      EmitOp(VM::Ops::PopLocals, compiler.previous.value().GetLine());
    }

    if (!isMainFunction) {
      EmitConstant(nullptr);
      EmitOp(VM::Ops::LoadConstant, compiler.previous.value().GetLine());
      EmitOp(VM::Ops::Return, compiler.previous.value().GetLine());
    }
  }

  compiler.locals.clear(); 
  
  if (isMainFunction) {
    EmitOp(VM::Ops::Exit, compiler.previous.value().GetLine());
  }

  compiler.codeContextStack.pop_back();
}

static void VarDeclaration(CompilerContext& compiler)
{
  if (compiler.codeContextStack.back() == CodeContext::TopLevel) {
    MessageAtPrevious("Only functions and classes are allowed at top level", LogLevel::Error, compiler);
    return;
  }

  if (!Match(Scanner::TokenType::Identifier, compiler)) {
    MessageAtCurrent("Expected identifier after `var`", LogLevel::Error, compiler);
    return;
  }

  auto nameToken = compiler.previous.value();

  if (Match(Scanner::TokenType::Colon, compiler)) {
    if (!IsValidTypeAnnotation(compiler.current.value().GetType())) {
      MessageAtCurrent("Expected typename after type annotation", LogLevel::Error, compiler);
      return;
    }
    Advance(compiler);
  }

  std::string localName(nameToken.GetText());
  auto it = std::find_if(compiler.locals.begin(), compiler.locals.end(), [&localName] (const Local& l) { return l.name == localName; });
  if (it != compiler.locals.end()) {
    MessageAtPrevious("A local variable with the same name already exists", LogLevel::Error, compiler);
    return;
  }

  auto line = compiler.previous.value().GetLine();

  auto localId = static_cast<std::int64_t>(compiler.locals.size());
  compiler.locals.emplace_back(std::move(localName), false, false, localId);
  EmitOp(VM::Ops::DeclareLocal, line);

  if (Match(Scanner::TokenType::Equal, compiler)) {
    auto prevUsing = compiler.usingExpressionResult;
    compiler.usingExpressionResult = true;
    Expression(false, compiler);
    compiler.usingExpressionResult = prevUsing;
    line = compiler.previous.value().GetLine();
    EmitConstant(localId);
    EmitOp(VM::Ops::AssignLocal, line);
  }
  Consume(Scanner::TokenType::Semicolon, "Expected ';' after `var` declaration", compiler);
}

static void FinalDeclaration(CompilerContext& compiler)
{
  if (compiler.codeContextStack.back() == CodeContext::TopLevel) {
    MessageAtPrevious("Only functions and classes are allowed at top level", LogLevel::Error, compiler);
    return;
  } 

  if (!Match(Scanner::TokenType::Identifier, compiler)) {
    MessageAtCurrent("Expected identifier after `final`", LogLevel::Error, compiler);
    return;
  }

  auto nameToken = compiler.previous.value();

  if (Match(Scanner::TokenType::Colon, compiler)) {
    if (!IsValidTypeAnnotation(compiler.current.value().GetType())) {
      MessageAtCurrent("Expected typename after type annotation", LogLevel::Error, compiler);
      return;
    }
    Advance(compiler);
  }

  std::string localName(nameToken.GetText());
  auto it = std::find_if(compiler.locals.begin(), compiler.locals.end(), [&localName] (const Local& l) { return l.name == localName; });
  if (it != compiler.locals.end()) {
    MessageAtPrevious("A local variable with the same name already exists", LogLevel::Error, compiler);
    return;
  }

  auto line = compiler.previous.value().GetLine();

  auto localId = static_cast<std::int64_t>(compiler.locals.size());
  compiler.locals.emplace_back(std::move(localName), true, false, localId);
  EmitOp(VM::Ops::DeclareLocal, line);

  Consume(Scanner::TokenType::Equal, "Must assign to `final` upon declaration", compiler);
  auto prevUsing = compiler.usingExpressionResult;
  compiler.usingExpressionResult = true;
  Expression(false, compiler);
  compiler.usingExpressionResult = prevUsing;
  line = compiler.previous.value().GetLine();
  EmitConstant(localId);
  EmitOp(VM::Ops::AssignLocal, line);

  Consume(Scanner::TokenType::Semicolon, "Expected ';' after `final` declaration", compiler);
}

static void Expression(bool canAssign, CompilerContext& compiler)
{
  if (IsOperator(compiler.current.value().GetType())) {
    MessageAtCurrent("Expected identifier or literal at start of expression", LogLevel::Error, compiler);
    Advance(compiler);
    return;
  }

  std::string kw;
  if (IsKeyword(compiler.current.value().GetType(), kw)) {
    MessageAtCurrent(fmt::format("'{}' is a keyword and not valid in this context", kw), LogLevel::Error, compiler);
    Advance(compiler);  //consume the illegal identifier
    return;
  }

  if (Check(Scanner::TokenType::Identifier, compiler)) {
    Call(canAssign, compiler);
    if (Check(Scanner::TokenType::Equal, compiler)) {  
      if (compiler.previous.value().GetType() != Scanner::TokenType::Identifier) {
        MessageAtCurrent("Only identifiers can be assigned to", LogLevel::Error, compiler);
        return;
      }

      auto localName = std::string(compiler.previous.value().GetText());
      auto it = std::find_if(compiler.locals.begin(), compiler.locals.end(), [&localName](const Local& l) { return l.name == localName; });
      if (it == compiler.locals.end()) {
        MessageAtPrevious(fmt::format("Cannot find variable '{}' in this scope", localName), LogLevel::Error, compiler);
        return;
      }

      if (it->isFinal) {
        MessageAtPrevious(fmt::format("Cannot reassign to final '{}'", compiler.previous.value().GetText()), LogLevel::Error, compiler);
        return;
      }

      if (it->isIterator && (s_Verbose || s_WarningsError)) {
        MessageAtPrevious(fmt::format("'{}' is an iterator variable and will be reassigned on each loop iteration", compiler.previous.value().GetText()), LogLevel::Warning, compiler);
        if (s_WarningsError) {
          return;
        }
      }

      Advance(compiler);  // consume the equals

      if (!canAssign) {
        MessageAtCurrent("Assignment is not valid in the current context", LogLevel::Error, compiler);
        return;
      }

      auto prevUsing = compiler.usingExpressionResult;
      compiler.usingExpressionResult = true;
      Expression(false, compiler); // disallow x = y = z...
      compiler.usingExpressionResult = prevUsing;

      EmitConstant(it->index);
      EmitOp(VM::Ops::AssignLocal, compiler.previous.value().GetLine());
    } else {
      bool shouldBreak = false;
      while (!shouldBreak) {
        switch (compiler.current.value().GetType()) {
          case Scanner::TokenType::Bar:
            BitwiseOr(false, true, compiler);
            break;
          case Scanner::TokenType::Ampersand:
            BitwiseAnd(false, true, compiler);
            break;
          case Scanner::TokenType::Caret:
            BitwiseXOr(false, true, compiler);
            break;
          case Scanner::TokenType::And:
            And(false, true, compiler);
            break;
          case Scanner::TokenType::Or:
            Or(false, true, compiler);
            break;
          case Scanner::TokenType::EqualEqual:
          case Scanner::TokenType::BangEqual:
            Equality(false, true, compiler);
            break;
          case Scanner::TokenType::GreaterThan:
          case Scanner::TokenType::GreaterEqual:
          case Scanner::TokenType::LessThan:
          case Scanner::TokenType::LessEqual:
            Comparison(false, true, compiler);
            break;
          case Scanner::TokenType::Plus:
          case Scanner::TokenType::Minus:
            Term(false, true, compiler);
            break;
          case Scanner::TokenType::Star:
          case Scanner::TokenType::StarStar:
          case Scanner::TokenType::Slash:
          case Scanner::TokenType::Mod:
            Factor(false, true, compiler);
            break;
          case Scanner::TokenType::ShiftLeft:
          case Scanner::TokenType::ShiftRight:
            Shift(false, true, compiler);
            break;
          case Scanner::TokenType::Semicolon:
          case Scanner::TokenType::RightParen:
          case Scanner::TokenType::Comma:
          case Scanner::TokenType::Colon:
          case Scanner::TokenType::LeftSquareParen:
          case Scanner::TokenType::RightSquareParen:
          case Scanner::TokenType::LeftCurlyParen:
          case Scanner::TokenType::RightCurlyParen:
          case Scanner::TokenType::DotDot:
          case Scanner::TokenType::By:
            shouldBreak = true;
            break;
          default:
            MessageAtCurrent("Invalid token found in expression", LogLevel::Error, compiler);
            Advance(compiler);
            return;
        }
      }
    }
  } else {
    Or(canAssign, false, compiler);
  }
}

static std::optional<std::string> TryParseInt(const Scanner::Token& token, std::int64_t& result, int base = 10, int offset = 0)
{
  auto [ptr, ec] = std::from_chars(token.GetData() + offset, token.GetData() + token.GetLength(), result, base);
  if (ec == std::errc()) {
    return {};
  }
  if (ec == std::errc::invalid_argument) {
    return "Invalid argument";
  }
  if (ec == std::errc::result_out_of_range) {
    return "Out of range";
  }
  GRACE_ASSERT(false, "Unhandled std::errc returned from TryParseInt()");
  return "Unexpected error parsing int";
}

static std::optional<std::exception> TryParseDouble(const Scanner::Token& token, double& result)
{
  try {
    std::string str(token.GetText());
    result = std::stod(str);
    return std::nullopt;
  } catch (const std::invalid_argument& e) {
    return e;
  } catch (const std::out_of_range& e) {
    return e;
  }
}

static bool IsLiteral(Scanner::TokenType token)
{
  static const std::vector<Scanner::TokenType> literalTypes{
    Scanner::TokenType::True,
    Scanner::TokenType::False,
    Scanner::TokenType::Integer,
    Scanner::TokenType::Double,
    Scanner::TokenType::String,
    Scanner::TokenType::Char
  };
  return std::any_of(literalTypes.begin(), literalTypes.end(), [token](Scanner::TokenType t) {
    return t == token;
  });
}

static void ExpressionStatement(CompilerContext& compiler)
{
  if (IsLiteral(compiler.current.value().GetType()) || IsOperator(compiler.current.value().GetType())) {
    MessageAtCurrent("Expected identifier or keyword at start of expression", LogLevel::Error, compiler);
    Advance(compiler);  // consume illegal token
    return;
  }
  Expression(true, compiler);
  Consume(Scanner::TokenType::Semicolon, "Expected ';' after expression", compiler);
}

static void AssertStatement(CompilerContext& compiler)
{
  auto line = compiler.previous.value().GetLine();

  Consume(Scanner::TokenType::LeftParen, "Expected '(' after `assert`", compiler);
  auto prevUsing = compiler.usingExpressionResult;
  compiler.usingExpressionResult = true; 
  Expression(false, compiler);
  compiler.usingExpressionResult = prevUsing;

  if (Match(Scanner::TokenType::Comma, compiler)) {
    Consume(Scanner::TokenType::String, "Expected message", compiler);
    EmitConstant(std::string(compiler.previous.value().GetText()));
    EmitOp(VM::Ops::AssertWithMessage, line);
    Consume(Scanner::TokenType::RightParen, "Expected ')'", compiler);
  } else {
    EmitOp(VM::Ops::Assert, line); 
    Consume(Scanner::TokenType::RightParen, "Expected ')'", compiler);
  }

  Consume(Scanner::TokenType::Semicolon, "Expected ';' after `assert` expression", compiler);
}

static void BreakStatement(CompilerContext& compiler)
{
  bool insideLoop = false;
  for (auto it = compiler.codeContextStack.crbegin(); it != compiler.codeContextStack.crend(); ++it) {
    if (*it == CodeContext::ForLoop || *it == CodeContext::WhileLoop) {
      insideLoop = true;
      break;
    }
  }
  if (!insideLoop) {
    // don't return early from here, so the compiler can synchronize...
    MessageAtPrevious("`break` only allowed inside loops", LogLevel::Error, compiler);
  }

  compiler.breakJumpNeedsIndexes = true;
  auto constIdx = static_cast<std::int64_t>(VM::VM::GetInstance().GetNumConstants());
  EmitConstant(std::int64_t{});
  auto opIdx = static_cast<std::int64_t>(VM::VM::GetInstance().GetNumConstants());
  EmitConstant(std::int64_t{});
  EmitOp(VM::Ops::Jump, compiler.previous.value().GetLine());
  compiler.breakIdxPairs.top().push_back(std::make_pair(constIdx, opIdx));
  Consume(Scanner::TokenType::Semicolon, "Expected ';' after `break`", compiler);
}

static void ContinueStatement(CompilerContext& compiler)
{
  bool insideLoop = false;
  for (auto it = compiler.codeContextStack.crbegin(); it != compiler.codeContextStack.crend(); ++it) {
    if (*it == CodeContext::ForLoop || *it == CodeContext::WhileLoop) {
      insideLoop = true;
      break;
    }
  }
  if (!insideLoop) {
    // don't return early from here, so the compiler can synchronize...
    MessageAtPrevious("`break` only allowed inside loops", LogLevel::Error, compiler);
  }

  compiler.continueJumpNeedsIndexes = true;
  auto constIdx = static_cast<std::int64_t>(VM::VM::GetInstance().GetNumConstants());
  EmitConstant(std::int64_t{});
  auto opIdx = static_cast<std::int64_t>(VM::VM::GetInstance().GetNumConstants());
  EmitConstant(std::int64_t{});
  EmitOp(VM::Ops::Jump, compiler.previous.value().GetLine());
  compiler.continueIdxPairs.top().push_back(std::make_pair(constIdx, opIdx));
  Consume(Scanner::TokenType::Semicolon, "Expected ';' after `break`", compiler);
}

static std::unordered_map<Scanner::TokenType, std::int64_t> s_CastOps = {
  std::make_pair(Scanner::TokenType::IntIdent, 0),
  std::make_pair(Scanner::TokenType::FloatIdent, 1),
  std::make_pair(Scanner::TokenType::BoolIdent, 2),
  std::make_pair(Scanner::TokenType::StringIdent, 3),
  std::make_pair(Scanner::TokenType::CharIdent, 4),
  std::make_pair(Scanner::TokenType::ListIdent, 5),
};

static void ForStatement(CompilerContext& compiler)
{
  compiler.codeContextStack.emplace_back(CodeContext::ForLoop);

  compiler.breakIdxPairs.emplace();
  compiler.continueIdxPairs.emplace();

  // parse terator variable
  auto firstItIsFinal = false;
  if (Match(Scanner::TokenType::Final, compiler)) {
    firstItIsFinal = true;
  }
  Consume(Scanner::TokenType::Identifier, "Expected identifier after `for`", compiler);
  auto iteratorNeedsPop = false, secondIteratorNeedsPop = false, twoIterators = false;
  auto iteratorName = std::string(compiler.previous.value().GetText());
  std::int64_t iteratorId;

  if (Match(Scanner::TokenType::Colon, compiler)) {
    if (!IsValidTypeAnnotation(compiler.current.value().GetType())) {
      MessageAtCurrent("Expected typename after type annotation", LogLevel::Error, compiler);
      return;
    }
    Advance(compiler);
  }

  auto it = std::find_if(
    compiler.locals.begin(),
    compiler.locals.end(),
    [&iteratorName](const Local& l) {
      return l.name == iteratorName;
    });
  if (it == compiler.locals.end()) {
    iteratorId = static_cast<std::int64_t>(compiler.locals.size());
    compiler.locals.emplace_back(std::move(iteratorName), firstItIsFinal, true, iteratorId);
    EmitOp(VM::Ops::DeclareLocal, compiler.previous.value().GetLine());
    iteratorNeedsPop = true;
  } else {
    if (it->isFinal) {
      MessageAtPrevious(fmt::format("Loop variable '{}' has already been declared as `final`", iteratorName), LogLevel::Error, compiler);
      return;
    }

    if (it->isIterator && (s_Verbose || s_WarningsError)) {
      MessageAtPrevious(fmt::format("'{}' is an iterator variable and will be reassigned on each iteration", iteratorName), LogLevel::Warning, compiler);
      if (s_WarningsError) {
        return;
      }
    }

    if (s_Verbose || s_WarningsError) {
      MessageAtPrevious(fmt::format("There is already a local variable called '{}' in this scope which will be reassigned inside the `for` loop", iteratorName),
                        LogLevel::Warning, compiler);
      if (s_WarningsError) {
        return;
      }
    }
    iteratorId = it->index;
  }

  // parse second iterator variable, if it exists
  std::int64_t secondIteratorId{};
  if (Match(Scanner::TokenType::Comma, compiler)) {
    twoIterators = true;
    auto secondItIsFinal = false;
    if (Match(Scanner::TokenType::Final, compiler)) {
      secondItIsFinal = true;
    }
    if (!Match(Scanner::TokenType::Identifier, compiler)) {
      MessageAtCurrent("Expected identifier", LogLevel::Error, compiler);
      return;
    }
    auto secondIteratorName = std::string(compiler.previous.value().GetText());
    auto secondIt = std::find_if(
      compiler.locals.begin(),
      compiler.locals.end(),
      [&secondIteratorName](const Local& l) {
        return l.name == secondIteratorName;
      });

    if (Match(Scanner::TokenType::Colon, compiler)) {
      if (!IsValidTypeAnnotation(compiler.current.value().GetType())) {
        MessageAtCurrent("Expected typename after type annotation", LogLevel::Error, compiler);
        return;
      }
      Advance(compiler);
    }

    if (secondIt == compiler.locals.end()) {
      secondIteratorId = static_cast<std::int64_t>(compiler.locals.size());
      compiler.locals.emplace_back(std::move(secondIteratorName), secondItIsFinal, true, secondIteratorId);
      auto line = compiler.previous.value().GetLine();
      EmitOp(VM::Ops::DeclareLocal, line);
      secondIteratorNeedsPop = true;
    } else {
      if (secondIt->isFinal) {
        MessageAtPrevious(fmt::format("Loop variable '{}' has already been declared as `final`", secondIteratorName), LogLevel::Error, compiler);
        return;
      }

      if (it->isIterator && (s_Verbose || s_WarningsError)) {
        MessageAtPrevious(fmt::format("'{}' is an iterator variable will be reassigned on each iteration", it->name), LogLevel::Warning, compiler);
        if (s_WarningsError) {
          return;
        }
      }

      if (s_Verbose || s_WarningsError) {
        MessageAtPrevious(fmt::format("There is already a local variable called '{}' in this scope which will be reassigned inside the `for` loop", secondIteratorName),
                          LogLevel::Warning, compiler);
        if (s_WarningsError) {
          return;
        }
      }
      secondIteratorId = secondIt->index;
    }
  }

  auto line = compiler.previous.value().GetLine();

  auto numLocalsStart = static_cast<std::int64_t>(compiler.locals.size());

  Consume(Scanner::TokenType::In, "Expected `in` after identifier", compiler);

  auto prevUsing = compiler.usingExpressionResult;
  compiler.usingExpressionResult = true;
  Expression(false, compiler);
  compiler.usingExpressionResult = prevUsing;

  Consume(Scanner::TokenType::Colon, "Expected ':' after `for` statement", compiler);

  line = compiler.previous.value().GetLine();

  EmitConstant(twoIterators);
  EmitConstant(iteratorId);
  EmitConstant(secondIteratorId);
  EmitOp(VM::Ops::AssignIteratorBegin, line);

  // constant and op index to jump to after each iteration
  auto startConstantIdx = static_cast<std::int64_t>(VM::VM::GetInstance().GetNumConstants());
  auto startOpIdx = static_cast<std::int64_t>(VM::VM::GetInstance().GetNumOps());

  // evaluate the condition
  EmitOp(VM::Ops::CheckIteratorEnd, line);

  auto endJumpConstantIndex = VM::VM::GetInstance().GetNumConstants();
  EmitConstant(std::int64_t{});
  auto endJumpOpIndex = VM::VM::GetInstance().GetNumConstants();
  EmitConstant(std::int64_t{});
  EmitOp(VM::Ops::JumpIfFalse, line);

  // parse loop body
  while (!Match(Scanner::TokenType::End, compiler)) {
    Declaration(compiler);

    if (Match(Scanner::TokenType::EndOfFile, compiler)) {
      MessageAtPrevious("Unterminated `for`", LogLevel::Error, compiler);
      return;
    }
  }

  // 'continue' statements bring us here so the iterator is incremented and we hit the jump
  if (compiler.continueJumpNeedsIndexes) {
    for (auto& p : compiler.continueIdxPairs.top()) {
      VM::VM::GetInstance().SetConstantAtIndex(p.first, static_cast<std::int64_t>(VM::VM::GetInstance().GetNumConstants()));
      VM::VM::GetInstance().SetConstantAtIndex(p.second, static_cast<std::int64_t>(VM::VM::GetInstance().GetNumOps()));
    }
    compiler.continueIdxPairs.pop();
    compiler.continueJumpNeedsIndexes = !compiler.continueIdxPairs.empty();
  }

  // increment iterator
  EmitConstant(twoIterators);
  EmitConstant(iteratorId);
  EmitConstant(secondIteratorId);
  EmitOp(VM::Ops::IncrementIterator, line);

  // pop any locals created within the loop scope
  if (compiler.locals.size() != static_cast<std::size_t>(numLocalsStart)) {
    EmitConstant(numLocalsStart);
    EmitOp(VM::Ops::PopLocals, line);
  }

  // always jump back to re-evaluate the condition
  EmitConstant(startConstantIdx);
  EmitConstant(startOpIdx);
  EmitOp(VM::Ops::Jump, line);

  // set indexes for breaks and when the condition fails, the iterator variable will need to be popped (if it's a new variable)
  if (compiler.breakJumpNeedsIndexes) {
    for (auto& p : compiler.breakIdxPairs.top()) {
      VM::VM::GetInstance().SetConstantAtIndex(p.first, static_cast<std::int64_t>(VM::VM::GetInstance().GetNumConstants()));
      VM::VM::GetInstance().SetConstantAtIndex(p.second, static_cast<std::int64_t>(VM::VM::GetInstance().GetNumOps()));
    }
    compiler.breakIdxPairs.pop();
    compiler.breakJumpNeedsIndexes = !compiler.breakIdxPairs.empty();

    if (compiler.locals.size() != static_cast<std::size_t>(numLocalsStart)) {
      EmitConstant(numLocalsStart);
      EmitOp(VM::Ops::PopLocals, line);
    }
  }
  
  while (compiler.locals.size() != static_cast<std::size_t>(numLocalsStart)) {
    compiler.locals.pop_back();
  }

  VM::VM::GetInstance().SetConstantAtIndex(endJumpConstantIndex, static_cast<std::int64_t>(VM::VM::GetInstance().GetNumConstants()));
  VM::VM::GetInstance().SetConstantAtIndex(endJumpOpIndex, static_cast<std::int64_t>(VM::VM::GetInstance().GetNumOps()));


  if (twoIterators) {
    if (secondIteratorNeedsPop) {
      compiler.locals.pop_back();
      EmitOp(VM::Ops::PopLocal, line);
    }
  }

  if (iteratorNeedsPop) {
    compiler.locals.pop_back();
    EmitOp(VM::Ops::PopLocal, line);
  }

  EmitOp(VM::Ops::DestroyHeldIterator, line);
  
  compiler.codeContextStack.pop_back();  
}

static void IfStatement(CompilerContext& compiler)
{
  compiler.codeContextStack.emplace_back(CodeContext::If);

  auto prevUsing = compiler.usingExpressionResult;
  compiler.usingExpressionResult = true;
  Expression(false, compiler);
  compiler.usingExpressionResult = prevUsing;
  Consume(Scanner::TokenType::Colon, "Expected ':' after condition", compiler);
  
  // store indexes of constant and instruction indexes to jump
  auto topConstantIdxToJump = VM::VM::GetInstance().GetNumConstants();
  EmitConstant(std::int64_t{});
  auto topOpIdxToJump = VM::VM::GetInstance().GetNumConstants();
  EmitConstant(std::int64_t{});

  EmitOp(VM::Ops::JumpIfFalse, compiler.previous.value().GetLine());

  // constant index, op index
  std::vector<std::tuple<std::int64_t, std::int64_t>> endJumpIndexPairs;

  auto numLocalsStart = static_cast<std::int64_t>(compiler.locals.size());

  bool topJumpSet = false;
  bool elseBlockFound = false;
  bool elseIfBlockFound = false;
  bool needsElseBlock = true;
  while (true) {
    if (needsElseBlock && Match(Scanner::TokenType::End, compiler)) {
      break;
    }

    if (Match(Scanner::TokenType::Else, compiler)) {
      // make any unreachable 'else' blocks a compiler error
      if (elseBlockFound) {
        MessageAtPrevious("Unreachable `else` due to previous `else`", LogLevel::Error, compiler);
        return;
      }
      
      // if the ifs condition passed and its block executed, it needs to jump to the end
      auto endConstantIdx = VM::VM::GetInstance().GetNumConstants();
      EmitConstant(std::int64_t{});
      auto endOpIdx = VM::VM::GetInstance().GetNumConstants();
      EmitConstant(std::int64_t{});
      EmitOp(VM::Ops::Jump, compiler.previous.value().GetLine());
      
      endJumpIndexPairs.emplace_back(endConstantIdx, endOpIdx);

      auto numConstants = VM::VM::GetInstance().GetNumConstants();
      auto numOps = VM::VM::GetInstance().GetNumOps();
      
      // haven't told the 'if' where to jump to yet if its condition fails 
      if (!topJumpSet) {
        VM::VM::GetInstance().SetConstantAtIndex(topConstantIdxToJump, static_cast<std::int64_t>(numConstants));
        VM::VM::GetInstance().SetConstantAtIndex(topOpIdxToJump, static_cast<std::int64_t>(numOps));
        topJumpSet = true;
      }

      if (Match(Scanner::TokenType::Colon, compiler)) {
        elseBlockFound = true;
      } else if (Check(Scanner::TokenType::If, compiler)) {
        elseIfBlockFound = true;
        needsElseBlock = false;
      } else {
        MessageAtCurrent("Expected `if` or `:` after `else`", LogLevel::Error, compiler);
        return;
      }
    }   

    Declaration(compiler);

    // above call to Declaration() will handle chained if/else
    if (elseIfBlockFound) {
      break;
    }
    
    if (Match(Scanner::TokenType::EndOfFile, compiler)) {
      MessageAtPrevious("Unterminated `if` statement", LogLevel::Error, compiler);
      return;
    }
  }

  auto numConstants = static_cast<std::int64_t>(VM::VM::GetInstance().GetNumConstants());
  auto numOps = static_cast<std::int64_t>(VM::VM::GetInstance().GetNumOps());

  for (auto [constIdx, opIdx] : endJumpIndexPairs) {
    VM::VM::GetInstance().SetConstantAtIndex(constIdx, numConstants);
    VM::VM::GetInstance().SetConstantAtIndex(opIdx, numOps);
  }
  
  // if there was no else or elseif block, jump to here
  if (!topJumpSet) {
    VM::VM::GetInstance().SetConstantAtIndex(topConstantIdxToJump, numConstants);
    VM::VM::GetInstance().SetConstantAtIndex(topOpIdxToJump, numOps);
  }

  if (compiler.locals.size() != static_cast<std::size_t>(numLocalsStart)) {
    auto line = compiler.previous.value().GetLine();
    EmitConstant(numLocalsStart);
    EmitOp(VM::Ops::PopLocals, line);

    while (compiler.locals.size() != static_cast<std::size_t>(numLocalsStart)) {
      compiler.locals.pop_back();
    }
  }

  compiler.codeContextStack.pop_back();
}

static void PrintStatement(CompilerContext& compiler)
{
  Consume(Scanner::TokenType::LeftParen, "Expected '(' after 'print'", compiler);
  if (Match(Scanner::TokenType::RightParen, compiler)) {
    EmitOp(VM::Ops::PrintTab, compiler.current.value().GetLine());
  } else {
    auto prevUsing = compiler.usingExpressionResult;
    compiler.usingExpressionResult = true;
    Expression(false, compiler);
    compiler.usingExpressionResult = prevUsing;
    EmitOp(VM::Ops::Print, compiler.current.value().GetLine());
    Consume(Scanner::TokenType::RightParen, "Expected ')' after expression", compiler);
  }
  Consume(Scanner::TokenType::Semicolon, "Expected ';' after expression", compiler);
}

static void PrintLnStatement(CompilerContext& compiler)
{
  Consume(Scanner::TokenType::LeftParen, "Expected '(' after 'println'", compiler);
  if (Match(Scanner::TokenType::RightParen, compiler)) {
    EmitOp(VM::Ops::PrintEmptyLine, compiler.current.value().GetLine());
  } else {
    auto prevUsing = compiler.usingExpressionResult;
    compiler.usingExpressionResult = true;
    Expression(false, compiler);
    compiler.usingExpressionResult = prevUsing;
    EmitOp(VM::Ops::PrintLn, compiler.current.value().GetLine());
    Consume(Scanner::TokenType::RightParen, "Expected ')' after expression", compiler);
  }
  Consume(Scanner::TokenType::Semicolon, "Expected ';' after expression", compiler);
}

static void EPrintStatement(CompilerContext& compiler)
{
  Consume(Scanner::TokenType::LeftParen, "Expected '(' after 'eprint'", compiler);
  if (Match(Scanner::TokenType::RightParen, compiler)) {
    EmitOp(VM::Ops::EPrintTab, compiler.current.value().GetLine());
  } else {
    auto prevUsing = compiler.usingExpressionResult;
    compiler.usingExpressionResult = true;
    Expression(false, compiler);
    compiler.usingExpressionResult = prevUsing;
    EmitOp(VM::Ops::EPrint, compiler.current.value().GetLine());
    Consume(Scanner::TokenType::RightParen, "Expected ')' after expression", compiler);
  }
  Consume(Scanner::TokenType::Semicolon, "Expected ';' after expression", compiler);
}

static void EPrintLnStatement(CompilerContext& compiler)
{
  Consume(Scanner::TokenType::LeftParen, "Expected '(' after 'eprintln'", compiler);
  if (Match(Scanner::TokenType::RightParen, compiler)) {
    EmitOp(VM::Ops::EPrintEmptyLine, compiler.current.value().GetLine());
  } else {
    auto prevUsing = compiler.usingExpressionResult;
    compiler.usingExpressionResult = true;
    Expression(false, compiler);
    compiler.usingExpressionResult = prevUsing;
    EmitOp(VM::Ops::EPrintLn, compiler.current.value().GetLine());
    Consume(Scanner::TokenType::RightParen, "Expected ')' after expression", compiler);
  }
  Consume(Scanner::TokenType::Semicolon, "Expected ';' after expression", compiler);
}

static void ReturnStatement(CompilerContext& compiler)
{
  if (std::find(compiler.codeContextStack.begin(), compiler.codeContextStack.end(), CodeContext::Function) == compiler.codeContextStack.end()) {
    MessageAtPrevious("`return` only allowed inside functions", LogLevel::Error, compiler);
    return;
  }

  if (VM::VM::GetInstance().GetLastFunctionName() == "main") {
    MessageAtPrevious("Cannot return from main function", LogLevel::Error, compiler);
    return;
  } 

  // don't destroy these locals here in the compiler's list because this could be an early return
  // that is handled at the end of `FuncDeclaration()`
  // but the VM needs to destroy any locals made up until this point
  if (!compiler.locals.empty()) {
    EmitConstant(std::int64_t{0});
    EmitOp(VM::Ops::PopLocals, compiler.previous.value().GetLine());
  }

  if (Match(Scanner::TokenType::Semicolon, compiler)) {
    auto line = compiler.previous.value().GetLine();
    EmitConstant(nullptr);
    EmitOp(VM::Ops::LoadConstant, line);
    EmitOp(VM::Ops::Return, line);
    return;
  }

  auto prevUsing = compiler.usingExpressionResult;
  compiler.usingExpressionResult = true;
  Expression(false, compiler);
  compiler.usingExpressionResult = prevUsing;
  EmitOp(VM::Ops::Return, compiler.previous.value().GetLine());
  Consume(Scanner::TokenType::Semicolon, "Expected ';' after expression", compiler);
}

static void TryStatement(CompilerContext& compiler)
{
  compiler.codeContextStack.emplace_back(CodeContext::Try);

  Consume(Scanner::TokenType::Colon, "Expected `:` after `try`", compiler);

  auto numLocalsStart = static_cast<std::int64_t>(compiler.locals.size());

  auto catchOpJumpIdx = VM::VM::GetInstance().GetNumConstants();
  EmitConstant(std::int64_t{});
  auto catchConstJumpIdx = VM::VM::GetInstance().GetNumConstants();
  EmitConstant(std::int64_t{});
  EmitOp(VM::Ops::EnterTry, compiler.previous.value().GetLine());

  while (!Match(Scanner::TokenType::Catch, compiler)) {
    Declaration(compiler);
    if (Match(Scanner::TokenType::EndOfFile, compiler)) {
      MessageAtPrevious("Unterminated `try` block", LogLevel::Error, compiler);
      return;
    }
  }

  // if we made it this far without the exception, pop locals...
  EmitConstant(numLocalsStart);
  EmitOp(VM::Ops::ExitTry, compiler.previous.value().GetLine());
  
  // then jump to after the catch block
  auto skipCatchConstJumpIdx = VM::VM::GetInstance().GetNumConstants();
  EmitConstant(std::int64_t{});
  auto skipCatchOpJumpIdx = VM::VM::GetInstance().GetNumConstants();
  EmitConstant(std::int64_t{});
  EmitOp(VM::Ops::Jump, compiler.previous.value().GetLine());

  // but if there was an exception, jump here, and also pop the locals but continue into the catch block
  VM::VM::GetInstance().SetConstantAtIndex(catchOpJumpIdx, static_cast<std::int64_t>(VM::VM::GetInstance().GetNumOps()));
  VM::VM::GetInstance().SetConstantAtIndex(catchConstJumpIdx, static_cast<std::int64_t>(VM::VM::GetInstance().GetNumConstants()));

  EmitConstant(numLocalsStart);
  EmitOp(VM::Ops::ExitTry, compiler.previous.value().GetLine());


  if (!Match(Scanner::TokenType::Identifier, compiler)) {
    MessageAtCurrent("Expected identifier after `catch`", LogLevel::Error, compiler);
    return;
  }

  while (compiler.locals.size() != static_cast<std::size_t>(numLocalsStart)) {
    compiler.locals.pop_back();
  }
  numLocalsStart = compiler.locals.size();

  auto exceptionVarName = std::string(compiler.previous.value().GetText());
  std::int64_t exceptionVarId;
  auto it = std::find_if(compiler.locals.begin(), compiler.locals.end(), [&exceptionVarName](const Local& l) { return l.name == exceptionVarName; });
  if (it == compiler.locals.end()) {
    exceptionVarId = static_cast<std::int64_t>(compiler.locals.size());
    compiler.locals.emplace_back(std::move(exceptionVarName), false, false, exceptionVarId);
    EmitOp(VM::Ops::DeclareLocal, compiler.previous.value().GetLine());
  } else {
    if (it->isFinal) {
      MessageAtPrevious(fmt::format("Exception variable '{}' has already been declared as `final`", exceptionVarName), LogLevel::Error, compiler);
      return;
    }

    if (it->isIterator && (s_Verbose || s_WarningsError)) {
      MessageAtPrevious(fmt::format("'{}' is an iterator variable will be reassigned on each loop iteration", exceptionVarName), LogLevel::Warning, compiler);
      if (s_WarningsError) {
        return;
      }
    }

    exceptionVarId = it->index;
    if (s_Verbose || s_WarningsError) {
      MessageAtPrevious(fmt::format("There is already a local variable called '{}' in this scope which will be reassigned inside the `catch` block", exceptionVarName),
          LogLevel::Warning, compiler);
      if (s_WarningsError) {
        return;
      }
    }
  }
  
  EmitConstant(exceptionVarId);
  EmitOp(VM::Ops::AssignLocal, compiler.previous.value().GetLine());

  Consume(Scanner::TokenType::Colon, "Expected `:` after `catch` statement", compiler);

  compiler.codeContextStack.pop_back();
  compiler.codeContextStack.emplace_back(CodeContext::Catch);

  while (!Match(Scanner::TokenType::End, compiler)) {
    Declaration(compiler);
    if (Match(Scanner::TokenType::EndOfFile, compiler)) {
      MessageAtPrevious("Unterminated `catch` block", LogLevel::Error, compiler);
      return;
    }
  }

  if (compiler.locals.size() != static_cast<std::size_t>(numLocalsStart)) {
    EmitConstant(numLocalsStart);
    EmitOp(VM::Ops::PopLocals, compiler.previous.value().GetLine());

    while (compiler.locals.size() != static_cast<std::size_t>(numLocalsStart)) {
      compiler.locals.pop_back();
    }
  }

  VM::VM::GetInstance().SetConstantAtIndex(skipCatchConstJumpIdx, static_cast<std::int64_t>(VM::VM::GetInstance().GetNumConstants()));
  VM::VM::GetInstance().SetConstantAtIndex(skipCatchOpJumpIdx, static_cast<std::int64_t>(VM::VM::GetInstance().GetNumOps()));

  compiler.codeContextStack.pop_back();
}

static void ThrowStatement(CompilerContext& compiler)
{
  Consume(Scanner::TokenType::LeftParen, "Expected '(' after `throw`", compiler);
  auto prevUsing = compiler.usingExpressionResult;
  compiler.usingExpressionResult = true;
  Expression(false, compiler);
  compiler.usingExpressionResult = prevUsing;
  EmitOp(VM::Ops::Throw, compiler.previous.value().GetLine());
  Consume(Scanner::TokenType::RightParen, "Expected ')' after `throw` message", compiler);
  Consume(Scanner::TokenType::Semicolon, "Expected ';' after `throw` statement", compiler);
}

static void WhileStatement(CompilerContext& compiler)
{
  compiler.codeContextStack.emplace_back(CodeContext::WhileLoop);

  compiler.breakIdxPairs.emplace();
  compiler.continueIdxPairs.emplace();

  auto constantIdx = static_cast<std::int64_t>(VM::VM::GetInstance().GetNumConstants());
  auto opIdx = static_cast<std::int64_t>(VM::VM::GetInstance().GetNumOps());

  auto prevUsing = compiler.usingExpressionResult;
  compiler.usingExpressionResult = true;
  Expression(false, compiler);
  compiler.usingExpressionResult = prevUsing;

  auto line = compiler.previous.value().GetLine();

  // evaluate the condition
  auto endConstantJumpIdx = VM::VM::GetInstance().GetNumConstants();
  EmitConstant(std::int64_t{});
  auto endOpJumpIdx = VM::VM::GetInstance().GetNumConstants();
  EmitConstant(std::int64_t{});
  EmitOp(VM::Ops::JumpIfFalse, compiler.previous.value().GetLine());

  Consume(Scanner::TokenType::Colon, "Expected ':' after expression", compiler);

  auto numLocalsStart = static_cast<std::int64_t>(compiler.locals.size());

  while (!Match(Scanner::TokenType::End, compiler)) {
    Declaration(compiler);
    if (Match(Scanner::TokenType::EndOfFile, compiler)) {
      MessageAtPrevious("Unterminated `while` loop", LogLevel::Error, compiler);
      return;
    }
  }

  auto numConstants = static_cast<std::int64_t>(VM::VM::GetInstance().GetNumConstants());
  auto numOps = static_cast<std::int64_t>(VM::VM::GetInstance().GetNumOps());

  if (compiler.continueJumpNeedsIndexes) {
    for (auto& p : compiler.continueIdxPairs.top()) {
      VM::VM::GetInstance().SetConstantAtIndex(p.first, numConstants);
      VM::VM::GetInstance().SetConstantAtIndex(p.second, numOps);
    }
    compiler.continueIdxPairs.pop();
    compiler.continueJumpNeedsIndexes = !compiler.continueIdxPairs.empty();
  }

  if (compiler.locals.size() != static_cast<std::size_t>(numLocalsStart)) {
    EmitConstant(numLocalsStart);
    EmitOp(VM::Ops::PopLocals, line);
  }

  // jump back up to the expression so it can be re-evaluated
  EmitConstant(constantIdx);
  EmitConstant(opIdx);
  EmitOp(VM::Ops::Jump, line);

  numConstants = static_cast<std::int64_t>(VM::VM::GetInstance().GetNumConstants());
  numOps = static_cast<std::int64_t>(VM::VM::GetInstance().GetNumOps());

  if (compiler.breakJumpNeedsIndexes) {
    for (auto& p : compiler.breakIdxPairs.top()) {
      VM::VM::GetInstance().SetConstantAtIndex(p.first, numConstants);
      VM::VM::GetInstance().SetConstantAtIndex(p.second, numOps);
    }
    compiler.breakIdxPairs.pop();
    compiler.breakJumpNeedsIndexes = !compiler.breakIdxPairs.empty();

    // if we broke, we missed the PopLocals instruction before the end of the loop
    // so tell the VM to still pop those locals IF we broke
    if (compiler.locals.size() != static_cast<std::size_t>(numLocalsStart)) {
      EmitConstant(numLocalsStart);
      EmitOp(VM::Ops::PopLocals, line);
    }
  }
  
  while (compiler.locals.size() != static_cast<std::size_t>(numLocalsStart)) {
    compiler.locals.pop_back();
  }

  numConstants = static_cast<std::int64_t>(VM::VM::GetInstance().GetNumConstants());
  numOps = static_cast<std::int64_t>(VM::VM::GetInstance().GetNumOps());

  VM::VM::GetInstance().SetConstantAtIndex(endConstantJumpIdx, numConstants);
  VM::VM::GetInstance().SetConstantAtIndex(endOpJumpIdx, numOps);

  compiler.codeContextStack.pop_back();
}

static void Or(bool canAssign, bool skipFirst, CompilerContext& compiler)
{
  if (!skipFirst) {
    And(canAssign, false, compiler);
  }
  while (Match(Scanner::TokenType::Or, compiler)) {
    And(canAssign, false, compiler);
    EmitOp(VM::Ops::Or, compiler.current.value().GetLine());
  }
}

static void And(bool canAssign, bool skipFirst, CompilerContext& compiler)
{
  if (!skipFirst) {
    BitwiseOr(canAssign, false, compiler);
  }
  while (Match(Scanner::TokenType::And, compiler)) {
    BitwiseOr(canAssign, false, compiler);
    EmitOp(VM::Ops::And, compiler.current.value().GetLine());
  }
}

static void BitwiseOr(bool canAssign, bool skipFirst, CompilerContext& compiler)
{
  if (!skipFirst) {
    BitwiseXOr(canAssign, false, compiler);
  }
  while (Match(Scanner::TokenType::Bar, compiler)) {
    BitwiseXOr(canAssign, false, compiler);
    EmitOp(VM::Ops::BitwiseOr, compiler.current.value().GetLine());
  }
}

static void BitwiseXOr(bool canAssign, bool skipFirst, CompilerContext& compiler)
{
  if (!skipFirst) {
    BitwiseAnd(canAssign, false, compiler);
  }
  while (Match(Scanner::TokenType::Caret, compiler)) {
    BitwiseAnd(canAssign, false, compiler);
    EmitOp(VM::Ops::BitwiseXOr, compiler.current.value().GetLine());
  }
}

static void BitwiseAnd(bool canAssign, bool skipFirst, CompilerContext& compiler)
{
  if (!skipFirst) {
    Equality(canAssign, false, compiler);
  }
  while (Match(Scanner::TokenType::Ampersand, compiler)) {
    Equality(canAssign, false, compiler);
    EmitOp(VM::Ops::BitwiseAnd, compiler.current.value().GetLine());
  }
}

static void Equality(bool canAssign, bool skipFirst, CompilerContext& compiler)
{
  if (!skipFirst) {
    Comparison(canAssign, false, compiler);
  }
  if (Match(Scanner::TokenType::EqualEqual, compiler)) {
    Comparison(canAssign, false, compiler);
    EmitOp(VM::Ops::Equal, compiler.current.value().GetLine());
  } else if (Match(Scanner::TokenType::BangEqual, compiler)) {
    Comparison(canAssign, false, compiler);
    EmitOp(VM::Ops::NotEqual, compiler.current.value().GetLine());
  }
}

static void Comparison(bool canAssign, bool skipFirst, CompilerContext& compiler)
{
  if (!skipFirst) {
    Shift(canAssign, false, compiler);
  }
  if (Match(Scanner::TokenType::GreaterThan, compiler)) {
    Shift(canAssign, false, compiler);
    EmitOp(VM::Ops::Greater, compiler.current.value().GetLine());
  } else if (Match(Scanner::TokenType::GreaterEqual, compiler)) {
    Shift(canAssign, false, compiler);
    EmitOp(VM::Ops::GreaterEqual, compiler.current.value().GetLine());
  } else if (Match(Scanner::TokenType::LessThan, compiler)) {
    Shift(canAssign, false, compiler);
    EmitOp(VM::Ops::Less, compiler.current.value().GetLine());
  } else if (Match(Scanner::TokenType::LessEqual, compiler)) {
    Shift(canAssign, false, compiler);
    EmitOp(VM::Ops::LessEqual, compiler.current.value().GetLine());
  }
}

static void Shift(bool canAssign, bool skipFirst, CompilerContext& compiler)
{
  if (!skipFirst) {
    Term(canAssign, false, compiler);
  }
  if (Match(Scanner::TokenType::ShiftRight, compiler)) {
    Term(canAssign, false, compiler);
    EmitOp(VM::Ops::ShiftRight, compiler.current.value().GetLine());
  } else if (Match(Scanner::TokenType::ShiftLeft, compiler)) {
    Term(canAssign, false, compiler);
    EmitOp(VM::Ops::ShiftLeft, compiler.current.value().GetLine());
  }
}

static void Term(bool canAssign, bool skipFirst, CompilerContext& compiler)
{
  if (!skipFirst) {
    Factor(canAssign, false, compiler);
  }
  while (true) {
    if (Match(Scanner::TokenType::Minus, compiler)) {
      Factor(canAssign, false, compiler);
      EmitOp(VM::Ops::Subtract, compiler.current.value().GetLine());
    } else if (Match(Scanner::TokenType::Plus, compiler)) {
      Factor(canAssign, false, compiler);
      EmitOp(VM::Ops::Add, compiler.current.value().GetLine());
    } else {
      break;
    }
  }
}

static void Factor(bool canAssign, bool skipFirst, CompilerContext& compiler)
{
  if (!skipFirst) {
    Unary(canAssign, compiler);
  }
  while (true) {
    if (Match(Scanner::TokenType::StarStar, compiler)) {
      Unary(canAssign, compiler);
      EmitOp(VM::Ops::Pow, compiler.current.value().GetLine());
    } else if (Match(Scanner::TokenType::Star, compiler)) {
      Unary(canAssign, compiler);
      EmitOp(VM::Ops::Multiply, compiler.current.value().GetLine());
    } else if (Match(Scanner::TokenType::Slash, compiler)) {
      Unary(canAssign, compiler);
      EmitOp(VM::Ops::Divide, compiler.current.value().GetLine());
    } else if (Match(Scanner::TokenType::Mod, compiler)) {
      Unary(canAssign, compiler);
      EmitOp(VM::Ops::Mod, compiler.current.value().GetLine());
    } else {
      break;
    }
  }
}

static void Unary(bool canAssign, CompilerContext& compiler)
{
  if (Match(Scanner::TokenType::Bang, compiler)) {
    auto line = compiler.previous.value().GetLine();
    Unary(canAssign, compiler);
    EmitOp(VM::Ops::Not, line);
  } else if (Match(Scanner::TokenType::Minus, compiler)) {
    auto line = compiler.previous.value().GetLine();
    Unary(canAssign, compiler);
    EmitOp(VM::Ops::Negate, line);
  } else if (Match(Scanner::TokenType::Tilde, compiler)) {
    auto line = compiler.previous.value().GetLine();
    Unary(canAssign, compiler);
    EmitOp(VM::Ops::BitwiseNot, line);
  } else {
    Call(canAssign, compiler);
  }
}

static void Call(bool canAssign, CompilerContext& compiler)
{
  Primary(canAssign, compiler);
}

static bool IsTypeIdent(Scanner::TokenType type)
{
  static const std::vector<Scanner::TokenType> typeIdents {
    Scanner::TokenType::IntIdent,
    Scanner::TokenType::FloatIdent,
    Scanner::TokenType::BoolIdent,
    Scanner::TokenType::StringIdent,
    Scanner::TokenType::CharIdent,
    Scanner::TokenType::ListIdent
  };
  return std::any_of(typeIdents.begin(), typeIdents.end(), [type](Scanner::TokenType t) {
    return t == type;
  });
}

static void Primary(bool canAssign, CompilerContext& compiler)
{
  if (Match(Scanner::TokenType::True, compiler)) {
    EmitOp(VM::Ops::LoadConstant, compiler.previous.value().GetLine());
    EmitConstant(true);
  } else if (Match(Scanner::TokenType::False, compiler)) {
    EmitOp(VM::Ops::LoadConstant, compiler.previous.value().GetLine());
    EmitConstant(false);
  } else if (Match(Scanner::TokenType::This, compiler)) {
    // TODO: this 
  } else if (Match(Scanner::TokenType::Integer, compiler)) {
    std::int64_t value;
    auto result = TryParseInt(compiler.previous.value(), value);
    if (result.has_value()) {
      MessageAtPrevious(fmt::format("Token could not be parsed as an int: {}", result.value()), LogLevel::Error, compiler);
      return;
    }
    EmitOp(VM::Ops::LoadConstant, compiler.previous.value().GetLine());
    EmitConstant(value);
  } else if (Match(Scanner::TokenType::HexLiteral, compiler)) {
    std::int64_t value;
    auto result = TryParseInt(compiler.previous.value(), value, 16, 2);
    if (result.has_value()) {
      MessageAtPrevious(fmt::format("Token could not be parsed as a hex literal int: {}", result.value()), LogLevel::Error, compiler);
      return;
    }
    EmitOp(VM::Ops::LoadConstant, compiler.previous.value().GetLine());
    EmitConstant(value);
  } else if (Match(Scanner::TokenType::BinaryLiteral, compiler)) {
    std::int64_t value;
    auto result = TryParseInt(compiler.previous.value(), value, 2, 2);
    if (result.has_value()) {
      MessageAtPrevious(fmt::format("Token could not be parsed as a binary literal int: {}", result.value()), LogLevel::Error, compiler);
      return;
    }
    EmitOp(VM::Ops::LoadConstant, compiler.previous.value().GetLine());
    EmitConstant(value);
  } else if (Match(Scanner::TokenType::Double, compiler)) {
    double value;
    auto result = TryParseDouble(compiler.previous.value(), value);
    if (result.has_value()) {
      MessageAtPrevious(fmt::format("Token could not be parsed as an float: {}", result.value().what()), LogLevel::Error, compiler);
      return;
    }
    EmitOp(VM::Ops::LoadConstant, compiler.previous.value().GetLine());
    EmitConstant(value);
  } else if (Match(Scanner::TokenType::String, compiler)) {
    String(compiler);
  } else if (Match(Scanner::TokenType::Char, compiler)) {
    Char(compiler);
  } else if (Match(Scanner::TokenType::Identifier, compiler)) {
    Identifier(canAssign, compiler);
  } else if (Match(Scanner::TokenType::Null, compiler)) {
    EmitConstant(nullptr);
    EmitOp(VM::Ops::LoadConstant, compiler.previous.value().GetLine());
  } else if (Match(Scanner::TokenType::LeftParen, compiler)) {
    Expression(canAssign, compiler);
    Consume(Scanner::TokenType::RightParen, "Expected ')'", compiler);
  } else if (Match(Scanner::TokenType::InstanceOf, compiler)) {
    InstanceOf(compiler);
  } else if (Match(Scanner::TokenType::IsObject, compiler)) {
    IsObject(compiler);
  } else if (IsTypeIdent(compiler.current.value().GetType())) {
    Cast(compiler);
  } else if (Match(Scanner::TokenType::LeftSquareParen, compiler)) {
    List(compiler);
  } else if (Match(Scanner::TokenType::LeftCurlyParen, compiler)) {
    Dictionary(compiler);
  } else if (Match(Scanner::TokenType::Typename, compiler)) {
    Typename(compiler);
  } else {
    Expression(canAssign, compiler);
  }
}

static void Identifier(bool canAssign, CompilerContext& compiler)
{
  auto prev = compiler.previous.value();
  auto prevText = std::string(compiler.previous.value().GetText());

  if (prev.GetType() != Scanner::TokenType::Identifier && Check(Scanner::TokenType::LeftParen, compiler)) {
    MessageAtCurrent("'(' only allowed after functions and classes", LogLevel::Error, compiler);
    return;
  }

  if (Match(Scanner::TokenType::LeftParen, compiler)) {
    compiler.namespaceQualifierUsed = true;

    static std::hash<std::string> hasher;
    auto hash = static_cast<std::int64_t>(hasher(prevText));
    auto nativeCall = prevText.starts_with("__");
    std::size_t nativeIndex{};
    if (nativeCall) {
      auto [exists, index] = VM::VM::GetInstance().HasNativeFunction(prevText);
      if (!exists) {
        Message(prev, fmt::format("No native function matching the given signature `{}` was found", prevText), LogLevel::Error, compiler);
        return;
      } else {
        nativeIndex = index;
      }
    }

    std::int64_t numArgs = 0;
    if (!Match(Scanner::TokenType::RightParen, compiler)) {
      while (true) {
        auto prevUsing = compiler.usingExpressionResult;
        compiler.usingExpressionResult = true;      
        Expression(false, compiler);
        compiler.usingExpressionResult = prevUsing;
        numArgs++;
        if (Match(Scanner::TokenType::RightParen, compiler)) {
          break;
        }
        Consume(Scanner::TokenType::Comma, "Expected ',' after function call argument", compiler);
      }
    }

    if (nativeCall) {
      auto arity = VM::VM::GetInstance().GetNativeFunction(nativeIndex).GetArity();
      if (numArgs != arity) {
        MessageAtPrevious(fmt::format("Incorrect number of arguments given to native call - got {} but expected {}", numArgs, arity), LogLevel::Error, compiler);
        return;
      }
    }

    // TODO: in a script that is not being ran as the main script, 
    // there may be a function called 'main' that you should be allowed to call
    if (prevText == "main") {
      Message(prev, "Cannot call the `main` function", LogLevel::Error, compiler);
      return;
    }
    
    EmitConstant(nativeCall ? static_cast<std::int64_t>(nativeIndex) : hash);
    EmitConstant(numArgs);
    if (nativeCall) {
      EmitOp(VM::Ops::NativeCall, compiler.previous.value().GetLine());
    } else {
      EmitConstant(prevText);
      EmitOp(VM::Ops::Call, compiler.previous.value().GetLine());

    }

    if (!compiler.usingExpressionResult) {
      // pop unused return value
      EmitOp(VM::Ops::Pop, compiler.previous.value().GetLine());
    }
  } else if (Match(Scanner::TokenType::Dot, compiler)) {
    // TODO: account for dot
  } else if (Match(Scanner::TokenType::ColonColon, compiler)) {
    if (!Check(Scanner::TokenType::Identifier, compiler)) {
      MessageAtCurrent("Expected identifier after `::`", LogLevel::Error, compiler);
      return;
    }
    if (IsLiteral(compiler.current.value().GetType())) {
      MessageAtCurrent("Expected identifier after `::`", LogLevel::Error, compiler);
      return;
    }

    if (compiler.namespaceQualifierUsed) {
      EmitOp(VM::Ops::StartNewNamespace, prev.GetLine());
      compiler.namespaceQualifierUsed = false;
    }

    static std::hash<std::string> hasher;
    EmitConstant(prevText);
    EmitConstant(static_cast<std::int64_t>(hasher(prevText)));
    EmitOp(VM::Ops::AppendNamespace, prev.GetLine());
    Expression(canAssign, compiler);
  } else {
    // not a call or member access, so we are just trying to call on the value of the local
    // or reassign it 
    // if it's not a reassignment, we are trying to load its value
    // Primary() has already but the variable's id on the stack
    if (!Check(Scanner::TokenType::Equal, compiler)) {
      auto it = std::find_if(compiler.locals.begin(), compiler.locals.end(), [&prevText](const Local& l){ return l.name == prevText; });
      if (it == compiler.locals.end()) {
        MessageAtPrevious(fmt::format("Cannot find variable '{}' in this scope", prevText), LogLevel::Error, compiler);
        return;
      }
      if (compiler.usingExpressionResult) {
        EmitConstant(it->index);
        EmitOp(VM::Ops::LoadLocal, prev.GetLine());
      } else {
        MessageAtPrevious("Expected expression", LogLevel::Error, compiler);
        return;
      }
    }
  }
}

static const char s_EscapeChars[] = {'t', 'b', 'n', 'r', '\'', '"', '\\'};
static const std::unordered_map<char, char> s_EscapeCharsLookup = {
  std::make_pair('t', '\t'),
  std::make_pair('b', '\b'),
  std::make_pair('r', '\r'),
  std::make_pair('n', '\n'),
  std::make_pair('\'', '\''),
  std::make_pair('"', '\"'),  // we need to escape this because it might be in a string
  std::make_pair('\\', '\\'),
};

static bool IsEscapeChar(char c, char& result) 
{
  for (auto escapeChar : s_EscapeChars) {
    if (c == escapeChar) {
      result = s_EscapeCharsLookup.at(escapeChar);
      return true;
    }
  }
  return false;
}

static void Char(CompilerContext& compiler)
{
  auto text = compiler.previous.value().GetText();
  auto trimmed = text.substr(1, text.length() - 2);
  switch (trimmed.length()) {
    case 2:
      if (trimmed[0] != '\\') {
        MessageAtPrevious("`char` must contain a single character or escape character", LogLevel::Error, compiler);
        return;
      }
      char c;
      if (IsEscapeChar(trimmed[1], c)) {
        EmitOp(VM::Ops::LoadConstant, compiler.previous.value().GetLine());
        EmitConstant(c);
      } else {
        MessageAtPrevious("Unrecognised escape character", LogLevel::Error, compiler);
      }
      break;
    case 1:
      if (trimmed[0] == '\\') {
        MessageAtPrevious("Expected escape character after backslash", LogLevel::Error, compiler);
        return;
      }
      EmitOp(VM::Ops::LoadConstant, compiler.previous.value().GetLine());
      EmitConstant(static_cast<char>(trimmed[0]));
      break;
    default:
      MessageAtPrevious("`char` must contain a single character or escape character", LogLevel::Error, compiler);
      break;
  }
}

static void String(CompilerContext& compiler)
{
  auto text = compiler.previous.value().GetText();
  std::string res;
  for (std::size_t i = 1; i < text.length() - 1; i++) {
    if (text[i] == '\\') {
      i++;
      if (i == text.length() - 1) {
        MessageAtPrevious("Expected escape character", LogLevel::Error, compiler);
        return;
      }
      char c;
      if (IsEscapeChar(text[i], c)) {
        res.push_back(c);
      } else {
        MessageAtPrevious("Unrecognised escape character", LogLevel::Error, compiler);
        return;
      }
    } else {
      res.push_back(text[i]);
    }
  }
  EmitOp(VM::Ops::LoadConstant, compiler.previous.value().GetLine());
  EmitConstant(res);
}

static void InstanceOf(CompilerContext& compiler)
{
  Consume(Scanner::TokenType::LeftParen, "Expected '(' after 'instanceof'", compiler);

  auto prevUsing = compiler.usingExpressionResult;
  compiler.usingExpressionResult = true;
  Expression(false, compiler);
  compiler.usingExpressionResult = prevUsing;

  Consume(Scanner::TokenType::Comma, "Expected ',' after expression", compiler);

  switch (compiler.current.value().GetType()) {
    case Scanner::TokenType::BoolIdent:
      EmitConstant(std::int64_t(0));
      break;
    case Scanner::TokenType::CharIdent:
      EmitConstant(std::int64_t(1));
      break;
    case Scanner::TokenType::FloatIdent:
      EmitConstant(std::int64_t(2));
      break;
    case Scanner::TokenType::IntIdent:
      EmitConstant(std::int64_t(3));
      break;
    case Scanner::TokenType::Null:
      EmitConstant(std::int64_t(4));
      if (s_Verbose || s_WarningsError) {
        MessageAtCurrent("Prefer comparison `== null` over `instanceof` call for `null` check", LogLevel::Warning, compiler);
        if (s_WarningsError) {
          return;
        }
      }
      break;
    case Scanner::TokenType::StringIdent:
      EmitConstant(std::int64_t(5));
      break;
    case Scanner::TokenType::ListIdent:
      EmitConstant(std::int64_t(6));
      break;
    case Scanner::TokenType::DictIdent:
      EmitConstant(std::int64_t(7));
      break;
    default:
      MessageAtCurrent("Expected type as second argument for `instanceof`", LogLevel::Error, compiler);
      return;
  }

  EmitOp(VM::Ops::CheckType, compiler.current.value().GetLine());

  Advance(compiler);  // Consume the type ident
  Consume(Scanner::TokenType::RightParen, "Expected ')'", compiler);

  if (!compiler.usingExpressionResult) {
    // pop unused return value
    EmitOp(VM::Ops::Pop, compiler.previous.value().GetLine());
  }
}

static void IsObject(CompilerContext& compiler)
{
  Consume(Scanner::TokenType::LeftParen, "Expected '(' after `isobject`", compiler);
  auto prevUsing = compiler.usingExpressionResult;
  compiler.usingExpressionResult = true;
  Expression(false, compiler);
  EmitOp(VM::Ops::IsObject, compiler.previous.value().GetLine());
  compiler.usingExpressionResult = prevUsing;
  Consume(Scanner::TokenType::RightParen, "Expected ')' after expression", compiler);

  if (!compiler.usingExpressionResult) {
    EmitOp(VM::Ops::Pop, compiler.previous.value().GetLine());
  }
}

static void Cast(CompilerContext& compiler)
{
  auto type = compiler.current.value().GetType();
  Advance(compiler);
  Consume(Scanner::TokenType::LeftParen, "Expected '(' after type ident", compiler);
  auto prevUsing = compiler.usingExpressionResult;
  compiler.usingExpressionResult = true;
  Expression(false, compiler);
  compiler.usingExpressionResult = prevUsing;
  EmitConstant(s_CastOps[type]);
  EmitOp(VM::Ops::Cast, compiler.current.value().GetLine());
  Consume(Scanner::TokenType::RightParen, "Expected ')' after expression", compiler);
  if (!compiler.usingExpressionResult) {
    // pop unused return value
    EmitOp(VM::Ops::Pop, compiler.previous.value().GetLine());
  }
}

static void List(CompilerContext& compiler)
{
  bool singleItemParsed = false, parsedRangeExpression = false;
  std::int64_t numItems = 0;

  while (true) {
    if (Match(Scanner::TokenType::RightSquareParen, compiler)) {
      break;
    }

    auto prevUsing = compiler.usingExpressionResult;
    compiler.usingExpressionResult = true;
    Expression(false, compiler);
    compiler.usingExpressionResult = prevUsing;

    if (Match(Scanner::TokenType::DotDot, compiler)){
      if (singleItemParsed) {
        MessageAtPrevious("Cannot mix single items and range expressions in list declaration", LogLevel::Error, compiler);
        return;
      }

      // max
      prevUsing = compiler.usingExpressionResult;
      compiler.usingExpressionResult = true;
      Expression(false, compiler);
      compiler.usingExpressionResult = prevUsing;

      // check for custom increment
      if (Match(Scanner::TokenType::By, compiler)) {
        prevUsing = compiler.usingExpressionResult;
        compiler.usingExpressionResult = true;
        Expression(false, compiler);
        compiler.usingExpressionResult = prevUsing;
      } else {
        EmitConstant(std::int64_t{1});
        EmitOp(VM::Ops::LoadConstant, compiler.previous.value().GetLine());
      }

      if (!Match(Scanner::TokenType::RightSquareParen, compiler)) {
        MessageAtCurrent("Expected `]` after range expression", LogLevel::Error, compiler);
        return;
      }

      parsedRangeExpression = true;

      break;
    } else {
      singleItemParsed = true;
      numItems++;
    }

    if (Match(Scanner::TokenType::RightSquareParen, compiler)) {
      break;
    }

    Consume(Scanner::TokenType::Comma, "Expected `,` between list items", compiler);
  }

  auto line = compiler.previous.value().GetLine();

  if (numItems == 0) {
    if (parsedRangeExpression) {
      EmitOp(VM::Ops::CreateRangeList, line);
    } else {
      EmitConstant(numItems);
      EmitOp(VM::Ops::CreateList, line);
    }
  } else {
    EmitConstant(numItems);
    EmitOp(VM::Ops::CreateList, line);
  }
}

static void Dictionary(CompilerContext& compiler)
{
  std::int64_t numItems = 0;

  while (true) {
    if (Match(Scanner::TokenType::RightCurlyParen, compiler)) {
      break;
    }

    auto prevUsing = compiler.usingExpressionResult;
    compiler.usingExpressionResult = true;
    Expression(false, compiler);

    if (!Match(Scanner::TokenType::Colon, compiler)) {
      MessageAtCurrent("Expected ':' after key expression", LogLevel::Error, compiler);
      return;
    }

    Expression(false, compiler);
    compiler.usingExpressionResult = prevUsing;

    numItems++;

    if (Match(Scanner::TokenType::RightCurlyParen, compiler)) {
      break;
    }

    Consume(Scanner::TokenType::Comma, "Expected `,` between dictionary pairs", compiler);
  }

  EmitConstant(numItems);
  EmitOp(VM::Ops::CreateDictionary, compiler.previous.value().GetLine());
}

static void Typename(CompilerContext& compiler)
{
  Consume(Scanner::TokenType::LeftParen, "Expected '('", compiler);
  auto prevUsing = compiler.usingExpressionResult;
  compiler.usingExpressionResult = true;
  Expression(false, compiler);
  compiler.usingExpressionResult = prevUsing;
  EmitOp(VM::Ops::Typename, compiler.previous.value().GetLine());
  Consume(Scanner::TokenType::RightParen, "Expected ')'", compiler);
}

static void MessageAtCurrent(const std::string& message, LogLevel level, CompilerContext& compiler)
{
  Message(compiler.current, message, level, compiler);
}

static void MessageAtPrevious(const std::string& message, LogLevel level, CompilerContext& compiler)
{
  Message(compiler.previous, message, level, compiler);
}

static void Message(const std::optional<Scanner::Token>& token, const std::string& message, LogLevel level, CompilerContext& compiler)
{
  if (level == LogLevel::Error || s_WarningsError) {
    if (compiler.panicMode) return;
    compiler.panicMode = true;
  }

  auto colour = fmt::fg(level == LogLevel::Error ? fmt::color::red : fmt::color::orange);
  fmt::print(stderr, colour | fmt::emphasis::bold, level == LogLevel::Error ? "ERROR: " : "WARNING: ");
  
  auto type = token.value().GetType();
  switch (type) {
    case Scanner::TokenType::EndOfFile:
      fmt::print(stderr, "at end: ");
      fmt::print(stderr, "{}\n", message);
      break;
    case Scanner::TokenType::Error:
      fmt::print(stderr, "{}\n", token.value().GetErrorMessage());
      break;
    default:
      fmt::print(stderr, "at '{}': ", token.value().GetText());
      fmt::print(stderr, "{}\n", message);
      break;
  }

  auto lineNo = static_cast<int>(token.value().GetLine());
  auto column = token.value().GetColumn() - token.value().GetLength();  // need the START of the token
  fmt::print(stderr, "       --> {}:{}:{}\n", compiler.currentFileName, lineNo, column + 1); 
  fmt::print(stderr, "        |\n");
  fmt::print(stderr, "{:>7} | {}\n", lineNo, Scanner::GetCodeAtLine(compiler.currentFileName, lineNo));
  fmt::print(stderr, "        | ");
  for (std::size_t i = 0; i < column; i++) {
    fmt::print(stderr, " ");
  }
  for (std::size_t i = 0; i < token.value().GetLength(); i++) {
    fmt::print(stderr, colour, "^");
  }
  fmt::print(stderr, "\n\n");

  if (level == LogLevel::Error) {
    compiler.hadError = true;
  }
  if (level == LogLevel::Warning) {
    compiler.hadWarning = true;
  }
}
