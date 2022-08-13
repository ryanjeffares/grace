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
#include <limits>
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
  Class,
  Constructor,
  ForLoop,
  Function,
  If,
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
  std::string currentNamespaceLookup;

  bool usingExpressionResult = false;

  bool continueJumpNeedsIndexes = false;
  bool breakJumpNeedsIndexes = false;

  // const idx, op idx
  using IndexStack = std::stack<std::vector<std::pair<std::size_t, std::size_t>>>;
  IndexStack breakIdxPairs, continueIdxPairs;
};

// If the compiler's current token matches the given type, consume it and advance
// Otherwise return false and do nothing
GRACE_NODISCARD static bool Match(Scanner::TokenType expected, CompilerContext& compiler);

// Checks if the compiler#s current token matches the given type without consuming it
GRACE_NODISCARD static bool Check(Scanner::TokenType expected, CompilerContext& compiler);

// Consumes the compiler's current token if it matches the given type, otherwise reports an error with the given message
static void Consume(Scanner::TokenType expected, const std::string& message, CompilerContext& compiler);

// Advances the compiler until it meets the start of a new declaration to avoid error spam after one is detected
static void Synchronize(CompilerContext& compiler);

static void EmitOp(VM::Ops op, std::size_t line)
{
  VM::VM::PushOp(op, line);
}

template<VM::BuiltinGraceType T>
static void EmitConstant(const T& value)
{
  VM::VM::PushConstant(value);
}

static void EmitConstant(const VM::Value& value)
{
  VM::VM::PushConstant(value);
}

// Advance to the next token
static void Advance(CompilerContext& compiler);

// All of the following functions parse the grammar of the language in a recursive descent pattern
static void Declaration(CompilerContext& compiler);
static void ImportDeclaration(CompilerContext& compiler);
static void ClassDeclaration(CompilerContext& compiler);
static void FuncDeclaration(CompilerContext& compiler);
static void VarDeclaration(CompilerContext& compiler, bool isFinal);
static void ConstDeclaration(CompilerContext& compiler);

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

static void FreeFunctionCall(const Scanner::Token& funcNameToken, CompilerContext& compiler);
static void DotFunctionCall(const Scanner::Token& funcNameToken, CompilerContext& compiler);
static void Dot(bool canAssign, CompilerContext& compiler);
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
static void Message(const Scanner::Token& token, const std::string& message, LogLevel level, CompilerContext& compiler);

GRACE_NODISCARD static VM::InterpretResult Finalise(const std::string& mainFileName, bool verbose, const std::vector<std::string>& args);

static bool s_Verbose, s_WarningsError;
static std::stack<CompilerContext> s_CompilerContextStack;

struct Constant
{
  VM::Value value;
  bool isExported;
};

static std::unordered_map<std::string, std::unordered_map<std::string, Constant>> s_FileConstantsLookup;

VM::InterpretResult Grace::Compiler::Compile(const std::string& fileName, std::string&& code, bool verbose, bool warningsError, const std::vector<std::string>& args)
{
  using namespace std::chrono;

  auto start = steady_clock::now();

  s_Verbose = verbose;
  s_WarningsError = warningsError;
 
  VM::VM::RegisterNatives();

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
     VM::VM::PrintOps();
   }
 #endif
   if (VM::VM::CombineFunctions(mainFileName, verbose)) {
     return VM::VM::Start(mainFileName, verbose, args);
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

    // go until we find the start of a new declaration
    switch (compiler.current.value().GetType()) {
      case Scanner::TokenType::Class:
      case Scanner::TokenType::Constructor:
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
    case Scanner::TokenType::Const: outKeyword = "const"; return true;
    case Scanner::TokenType::Constructor: outKeyword = "constructor"; return true;
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
    case Scanner::TokenType::This: outKeyword = "this"; return true;
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
  } else if (Match(Scanner::TokenType::Var, compiler) || Match(Scanner::TokenType::Final, compiler)) {
    compiler.passedImports = true;
    VarDeclaration(compiler, compiler.previous.value().GetType() == Scanner::TokenType::Final);
  } else if (Match(Scanner::TokenType::Const, compiler)) {
    compiler.passedImports = true;
    ConstDeclaration(compiler);
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

static bool IsTypeIdent(Scanner::TokenType type)
{
  static const std::vector<Scanner::TokenType> typeIdents = {
    Scanner::TokenType::IntIdent,
    Scanner::TokenType::FloatIdent,
    Scanner::TokenType::BoolIdent,
    Scanner::TokenType::StringIdent,
    Scanner::TokenType::CharIdent,
    Scanner::TokenType::ListIdent,
    Scanner::TokenType::DictIdent,
    Scanner::TokenType::KeyValuePairIdent,
    Scanner::TokenType::ExceptionIdent,
  };
  return std::any_of(typeIdents.begin(), typeIdents.end(), [type](Scanner::TokenType t) {
    return t == type;
  });
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
    Scanner::TokenType::ExceptionIdent,
    Scanner::TokenType::KeyValuePairIdent,
  };
  return std::any_of(valid.begin(), valid.end(), [token] (Scanner::TokenType t) {
    return t == token;
  });
}

static const char s_EscapeChars[] = { 't', 'b', 'n', 'r', '\'', '"', '\\' };
static const std::unordered_map<char, char> s_EscapeCharsLookup = {
  {'t', '\t'},
  {'b', '\b'},
  {'r', '\r'},
  {'n', '\n'},
  {'\'', '\''},
  {'"', '\"'},
  {'\\', '\\'},
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

static std::optional<std::string> TryParseChar(const Scanner::Token& token, char& outValue)
{
  auto text = token.GetText();
  auto trimmed = text.substr(1, text.length() - 2);
  switch (trimmed.length()) {
    case 2:
      if (trimmed[0] != '\\') {
        return "`char` must contain a single character or escape character";
      }
      char c;
      if (IsEscapeChar(trimmed[1], c)) {
        outValue = c;
      } else {
        return "Unrecognised escape character";
      }
      return {};
    case 1:
      if (trimmed[0] == '\\') {
        return "Expected escape character after backslash";
      }
      outValue = trimmed[0];
      return {};
    default:
      return "`char` must contain a single character or escape character";
  }
}

static std::optional<std::string> TryParseString(const Scanner::Token& token, std::string& outValue)
{
  auto text = token.GetText();
  std::string res;
  for (std::size_t i = 1; i < text.length() - 1; i++) {
    if (text[i] == '\\') {
      i++;
      if (i == text.length() - 1) {
        return "Expected escape character but string terminated";
      }
      char c;
      if (IsEscapeChar(text[i], c)) {
        res.push_back(c);
      } else {
        return fmt::format("Unrecognised escape character '{}'", text[i]);
      }
    } else {
      res.push_back(text[i]);
    }
  }

  outValue = std::move(res);
  return {};
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
    auto str = token.GetString();
    result = std::stod(str);
    return std::nullopt;
  } catch (const std::invalid_argument& e) {
    return e;
  } catch (const std::out_of_range& e) {
    return e;
  }
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
    // windows warns about std::getenv() with /W4 so do all this nonsense
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
    Message(lastPathToken.value(), fmt::format("Could not find file `{}` to import", importPath), LogLevel::Error, compiler);
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

static void ClassDeclaration(CompilerContext& compiler)
{
  if (compiler.codeContextStack.back() != CodeContext::TopLevel) {
    MessageAtPrevious("Classes only allowed at top level", LogLevel::Error, compiler);
    return;
  }

  compiler.codeContextStack.push_back(CodeContext::Class);

  auto exported = false;
  if (Match(Scanner::TokenType::Export, compiler)) {
    exported = true;
  }

  if (!Match(Scanner::TokenType::Identifier, compiler)) {
    MessageAtCurrent("Expected identifier after `class`", LogLevel::Error, compiler);
    return;
  }

  auto classNameToken = compiler.previous.value();

  if (!Match(Scanner::TokenType::Colon, compiler)) {
    MessageAtCurrent("Expected ':' after class name", LogLevel::Error, compiler);
    return;
  }

  auto hasDefinedConstructor = false;  
  std::vector<std::string> classMembers;

  while (!Match(Scanner::TokenType::End, compiler)) {
    if (Match(Scanner::TokenType::EndOfFile, compiler)) {
      MessageAtPrevious("Unterminated class", LogLevel::Error, compiler);
      return;
    }

    if (Match(Scanner::TokenType::Var, compiler)) {
      // member variable

      if (hasDefinedConstructor) {
        MessageAtPrevious("Member variable declarations can only come before the constructor", LogLevel::Error, compiler);
        return;
      }

      Consume(Scanner::TokenType::Identifier, "Expected identifier after `var`", compiler);
      
      classMembers.push_back(compiler.previous.value().GetString());

      auto nameToken = compiler.previous.value();

      if (Match(Scanner::TokenType::Colon, compiler)) {
        if (!IsValidTypeAnnotation(compiler.current.value().GetType())) {
          MessageAtCurrent("Expected typename after type annotation", LogLevel::Error, compiler);
          return;
        }
        Advance(compiler);
      }

      auto localName = nameToken.GetString();

      if (localName.starts_with("__")) {
        MessageAtPrevious("Names beginning with double underscore `__` are reserved for internal use", LogLevel::Error, compiler);
        return;
      }

      auto it = std::find_if(compiler.locals.begin(), compiler.locals.end(), [&localName](const Local& l) { return l.name == localName; });
      if (it != compiler.locals.end()) {
        MessageAtPrevious("A class member with the same name already exists", LogLevel::Error, compiler);
        return;
      }
      
      Consume(Scanner::TokenType::Semicolon, "Expected ';'", compiler);
    } else if (Match(Scanner::TokenType::Constructor, compiler)) {
      // class constructor

      compiler.codeContextStack.push_back(CodeContext::Constructor);
      hasDefinedConstructor = true;
      Consume(Scanner::TokenType::LeftParen, "Expected '(' after `constructor`", compiler);

      // we will treat the constructor as a function in the VM with the name of the class name that returns the instance
      std::vector<std::string> parameters;
      while (!Match(Scanner::TokenType::RightParen, compiler)) {
        if (Match(Scanner::TokenType::Identifier, compiler) || Match(Scanner::TokenType::Final, compiler)) {
          auto isFinal = compiler.previous.value().GetType() == Scanner::TokenType::Final;
          if (isFinal) {
            Consume(Scanner::TokenType::Identifier, "Expected identifier after `final`", compiler);
          }
          auto p = compiler.previous.value().GetString();

          if (p.starts_with("__")) {
            MessageAtPrevious("Names beginning with double underscore `__` are reserved for internal use", LogLevel::Error, compiler);
            return;
          }

          if (std::find(parameters.begin(), parameters.end(), p) != parameters.end()) {
            MessageAtPrevious("Function parameters with the same name already defined", LogLevel::Error, compiler);
            return;
          }

          if (std::find(classMembers.begin(), classMembers.end(), p) != classMembers.end()) {
            MessageAtPrevious("Function parameter shadows class member variable", LogLevel::Error, compiler);
            return;
          }

          parameters.push_back(p);
          compiler.locals.emplace_back(std::move(p), isFinal, false, compiler.locals.size());

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

      Consume(Scanner::TokenType::Colon, "Expected ':' after constructor declaration", compiler);

      if (!VM::VM::AddFunction(classNameToken.GetString(), parameters.size(), compiler.currentFileName, exported, false)) {
        Message(classNameToken, "A function or class in the same namespace already exists with the same name as this class", LogLevel::Error, compiler);
        return;
      }
      
      // make the class' members at the start of the constructor
      for (std::size_t i = 0; i < classMembers.size(); i++) {
        EmitOp(VM::Ops::DeclareLocal, compiler.previous.value().GetLine());
        auto localId = static_cast<std::int64_t>(compiler.locals.size());
        auto memberName = classMembers[i];
        compiler.locals.emplace_back(std::move(memberName), false, false, localId);
      }
      
      auto numLocalsStart = compiler.locals.size();
      // we can parse declarations in here as normal, but the "locals" will be the members of the newly created instance
      while (!Match(Scanner::TokenType::End, compiler)) {
        // so don't allow return statements here, we will implicitly return the instance at the end
        if (Match(Scanner::TokenType::Return, compiler)) {
          MessageAtPrevious("Cannot return from a constructor", LogLevel::Error, compiler);
          return;
        }
        Declaration(compiler);
        if (compiler.current.value().GetType() == Scanner::TokenType::EndOfFile) {
          MessageAtCurrent("Expected `end` after constructor", LogLevel::Error, compiler);
          return;
        }
      }

      if (compiler.locals.size() > numLocalsStart) {
        // pop locals we made inside the constructor
        EmitConstant(static_cast<std::int64_t>(parameters.size()));
        EmitOp(VM::Ops::PopLocals, compiler.previous.value().GetLine());
      }

      compiler.codeContextStack.pop_back();
    } else {
      MessageAtCurrent("Expected `var` or `constructor` inside class", LogLevel::Error, compiler);
      return;
    }
  }

  // make an empty constructor if the user doesn't define one
  if (!hasDefinedConstructor) {
    if (!VM::VM::AddFunction(classNameToken.GetString(), 0, compiler.currentFileName, exported, false)) {
      Message(classNameToken, "A function or class in the same namespace already exists with the same name as this class", LogLevel::Error, compiler);
      return;
    }

    for (std::size_t i = 0; i < classMembers.size(); i++) {
      // and declare the class' members as locals in this function that will be used by the VM to assign to the instance
      EmitOp(VM::Ops::DeclareLocal, compiler.previous.value().GetLine());
      auto localId = static_cast<std::int64_t>(compiler.locals.size());
      auto memberName = classMembers[i];
      compiler.locals.emplace_back(std::move(memberName), false, false, localId);
    }
  }

  if (!VM::VM::AddClass(classNameToken.GetString(), classMembers, compiler.currentFileName, exported)) {
    // technically unreachable, since it would have been caught by trying to add the constructor as a function
    Message(classNameToken, "A class in the same namespace already exists with the same name", LogLevel::Error, compiler);
    return;
  } 

  // ok so how do we return the instance...
  // we tell the VM which type of class it should be, how many locals to rip off the stack to assign to its members
  // the indexes of the members will line up with the indexes of the locals
  // emit the ops here before we move classMembers

  // at this point, the class' members we need to assign to are on the top of the stack
  // with the constructor's parameters below them
  EmitConstant(static_cast<std::int64_t>(classMembers.size()));
  for (const auto& m : classMembers) {
    EmitConstant(m);
  }

  static std::hash<std::string> hasher;
  EmitConstant(static_cast<std::int64_t>(hasher(classNameToken.GetString())));
  EmitConstant(static_cast<std::int64_t>(hasher(compiler.currentFileName)));  

  EmitOp(VM::Ops::CreateInstance, compiler.previous.value().GetLine());

  EmitConstant(std::int64_t{ 0 });
  EmitOp(VM::Ops::PopLocals, compiler.previous.value().GetLine());

  EmitOp(VM::Ops::Return, compiler.previous.value().GetLine());

  compiler.locals.clear();
  compiler.codeContextStack.pop_back();
}

static void FuncDeclaration(CompilerContext& compiler)
{
  if (compiler.codeContextStack.back() != CodeContext::TopLevel) {
    MessageAtPrevious("Functions are only allowed at top level", LogLevel::Error, compiler);
    return;
  }

  auto exportFunction = false;
  compiler.codeContextStack.emplace_back(CodeContext::Function);

  if (Match(Scanner::TokenType::Export, compiler)) {
    exportFunction = true;
  }

  Consume(Scanner::TokenType::Identifier, "Expected function name", compiler);
  auto funcNameToken = compiler.previous.value();
  auto name = compiler.previous.value().GetString();
  if (name.starts_with("__")) {
    MessageAtPrevious("Names beginning with double underscore `__` are reserved for internal use", LogLevel::Error, compiler);
    return;
  }
  auto isMainFunction = name == "main";

  Consume(Scanner::TokenType::LeftParen, "Expected '(' after function name", compiler);

  std::size_t extensionObjectNameHash{};
  auto isExtensionMethod = false;

  std::vector<std::string> parameters;
  while (!Match(Scanner::TokenType::RightParen, compiler)) {
    if (isMainFunction && parameters.size() > 1) {
      Message(funcNameToken, fmt::format("`main` function can only take 0 or 1 parameter(s) but got {}", parameters.size()), LogLevel::Error, compiler);
      return;
    }

    if (Match(Scanner::TokenType::Identifier, compiler) || Match(Scanner::TokenType::Final, compiler)) {
      auto isFinal = compiler.previous.value().GetType() == Scanner::TokenType::Final;
      if (isFinal) {
        Consume(Scanner::TokenType::Identifier, "Expected identifier after `final`", compiler);
      }
      auto p = compiler.previous.value().GetString();

      if (p.starts_with("__")) {
        MessageAtPrevious("Names beginning with double underscore `__` are reserved for internal use", LogLevel::Error, compiler);
        return;
      }

      if (std::find(parameters.begin(), parameters.end(), p) != parameters.end()) {
        MessageAtPrevious("Function parameters with the same name already defined", LogLevel::Error, compiler);
        return;
      }
      parameters.push_back(p);
      compiler.locals.emplace_back(std::move(p), isFinal, false, compiler.locals.size());

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
    } else if (Match(Scanner::TokenType::This, compiler)) {
      if (isMainFunction) {
        MessageAtPrevious("`this` not allowed in main function", LogLevel::Error, compiler);
        return;
      }

      if (!parameters.empty()) {
        MessageAtPrevious("`this` can only appear before the first function parameter to make an extension method", LogLevel::Error, compiler);
        return;
      }

      auto type = compiler.current.value().GetType();
      if (!IsTypeIdent(type) && type != Scanner::TokenType::Identifier) {
        MessageAtCurrent("Expected type name for extension method", LogLevel::Error, compiler);
        return;
      }

      static std::hash<std::string> hasher;
      extensionObjectNameHash = hasher(compiler.current.value().GetString());
      
      Advance(compiler);
      isExtensionMethod = true;

      Consume(Scanner::TokenType::Identifier, "Expected identifier after type identifier", compiler);

      auto p = compiler.previous.value().GetString();
      
      if (p.starts_with("__")) {
        MessageAtPrevious("Names beginning with double underscore `__` are reserved for internal use", LogLevel::Error, compiler);
        return;
      }

      if (std::find(parameters.begin(), parameters.end(), p) != parameters.end()) {
        MessageAtPrevious("Function parameters with the same name already defined", LogLevel::Error, compiler);
        return;
      }

      parameters.push_back(p);
      compiler.locals.emplace_back(std::move(p), false, false, compiler.locals.size());

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
  // but don't use up the name "args"
  // by giving it a __ prefix here we can stop the user from double declaring
  // since identifiers starting with __ aren't allowed
  if (isMainFunction && parameters.size() == 0) {
    compiler.locals.emplace_back("__ARGS", true, false, 0);
  }

  Consume(Scanner::TokenType::Colon, "Expected ':' after function signature", compiler);

  if (!VM::VM::AddFunction(std::move(name), parameters.size(), compiler.currentFileName, exportFunction, isExtensionMethod, extensionObjectNameHash)) {
    Message(funcNameToken, "A function or class in the same namespace already exists with the same name as this function", LogLevel::Error, compiler);
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
  if (VM::VM::GetLastOp() != VM::Ops::Return) {
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

static void VarDeclaration(CompilerContext& compiler, bool isFinal)
{
  if (compiler.codeContextStack.back() == CodeContext::TopLevel) {
    MessageAtPrevious("Only functions and classes are allowed at top level", LogLevel::Error, compiler);
    return;
  }

  auto diagnosticName = isFinal ? "final" : "var";

  if (!Match(Scanner::TokenType::Identifier, compiler)) {
    MessageAtCurrent(fmt::format("Expected identifier after `{}`", diagnosticName), LogLevel::Error, compiler);
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

  auto localName = nameToken.GetString();

  if (localName.starts_with("__")) {
    MessageAtPrevious("Names beginning with double underscore `__` are reserved for internal use", LogLevel::Error, compiler);
    return;
  }

  auto it = std::find_if(compiler.locals.begin(), compiler.locals.end(), [&localName] (const Local& l) { return l.name == localName; });
  if (it != compiler.locals.end()) {
    MessageAtPrevious("A local variable with the same name already exists", LogLevel::Error, compiler);
    return;
  }

  auto line = nameToken.GetLine();

  auto localId = static_cast<std::int64_t>(compiler.locals.size());
  compiler.locals.emplace_back(std::move(localName), isFinal, false, localId);
  EmitOp(VM::Ops::DeclareLocal, line);

  if (Match(Scanner::TokenType::Equal, compiler)) {
    auto prevUsing = compiler.usingExpressionResult;
    compiler.usingExpressionResult = true;
    Expression(false, compiler);
    compiler.usingExpressionResult = prevUsing;
    line = compiler.previous.value().GetLine();
    EmitConstant(localId);
    EmitOp(VM::Ops::AssignLocal, line);
  } else {
    if (isFinal) {
      MessageAtCurrent("Must assign to `final` upon declaration", LogLevel::Error, compiler);
      return;
    }
  }

  Consume(Scanner::TokenType::Semicolon, fmt::format("Expected ';' after `{}` declaration", diagnosticName), compiler);
}

static void ConstDeclaration(CompilerContext& compiler)
{
  if (compiler.codeContextStack.back() != CodeContext::TopLevel) {
    MessageAtPrevious("`const` declarations are only allowed at top level", LogLevel::Error, compiler);
    return;
  }

  bool isExport = false;
  if (Match(Scanner::TokenType::Export, compiler)) {
    isExport = true;
  }

  if (!Match(Scanner::TokenType::Identifier, compiler)) {
    MessageAtCurrent("Expected identifier after `const`", LogLevel::Error, compiler);
    return;
  }

  auto nameToken = compiler.previous.value();

  if (!Match(Scanner::TokenType::Equal, compiler)) {
    MessageAtCurrent("Must assign to `const` upon declaration", LogLevel::Error, compiler);
    return;
  }

  if (!IsLiteral(compiler.current.value().GetType())) {
    MessageAtCurrent("Must assign a primitive literal value to `const`", LogLevel::Error, compiler);
    return;
  }

  auto valueToken = compiler.current.value();
  Advance(compiler);

  switch (valueToken.GetType()) {
    case Scanner::TokenType::True:
      s_FileConstantsLookup[compiler.currentFileName][nameToken.GetString()] = { VM::Value(true), isExport };
      break;
    case Scanner::TokenType::False:
      s_FileConstantsLookup[compiler.currentFileName][nameToken.GetString()] = { VM::Value(false), isExport };
      break;
    case Scanner::TokenType::Integer: {
      std::int64_t value;
      auto result = TryParseInt(valueToken, value);
      if (result.has_value()) {
        Message(valueToken, fmt::format("Token could not be parsed as an int: {}", result.value()), LogLevel::Error, compiler);
        return;
      }
      s_FileConstantsLookup[compiler.currentFileName][nameToken.GetString()] = { VM::Value(value), isExport };
      break;
    }
    case Scanner::TokenType::Double: {
      double value;
      auto result = TryParseDouble(valueToken, value);
      if (result.has_value()) {
        Message(valueToken, fmt::format("Token could not be parsed as an float: {}", result.value().what()), LogLevel::Error, compiler);
        return;
      }
      s_FileConstantsLookup[compiler.currentFileName][nameToken.GetString()] = { VM::Value(value), isExport };
      break;
    }
    case Scanner::TokenType::String: {
      std::string res;
      auto err = TryParseString(valueToken, res);
      if (err.has_value()) {
        Message(valueToken, fmt::format("Token could not be parsed as string: {}", err.value()), LogLevel::Error, compiler);
        return;
      }
      s_FileConstantsLookup[compiler.currentFileName][nameToken.GetString()] = { VM::Value(res), isExport };
      break;
    }
    case Scanner::TokenType::Char: {
      char res;
      auto err = TryParseChar(valueToken, res);
      if (err.has_value()) {
        Message(valueToken, fmt::format("Token could not be parsed as char: {}", err.value()), LogLevel::Error, compiler);
        return;
      }
      s_FileConstantsLookup[compiler.currentFileName][nameToken.GetString()] = { VM::Value(res), isExport };
      break;
    }
    default:
      GRACE_UNREACHABLE();
      break;
  }

  if (!Match(Scanner::TokenType::Semicolon, compiler)) {
    MessageAtCurrent("Expected ';'", LogLevel::Error, compiler);
    return;
  }
}

// returns higher values for less similar strings
static std::size_t GetEditDistance(const std::string& first, const std::string& second)
{
  auto l1 = first.length();
  auto l2 = second.length();

  if (l1 == 0) {
    return l2;
  }

  if (l2 == 0) {
    return l1;
  }

  std::vector<std::vector<std::size_t>> matrix(l1 + 1);
  for (std::size_t i = 0; i < matrix.size(); i++) {
    matrix[i].resize(l2 + 1);
  }

  for (std::size_t i = 1; i <= l1; i++) {
    matrix[i][0] = i;
  }
  for (std::size_t j = 1; j <= l2; j++) {
    matrix[0][j] = j;
  }

  for (std::size_t i = 1; i <= l1; i++) {
    for (std::size_t j = 1; j <= l2; j++) {
      std::size_t weight = first[i - 1] == second[j - 1] ? 0 : 1;
      matrix[i][j] = std::min({ matrix[i - 1][j] + 1, matrix[i][j - 1] + 1, matrix[i - 1][j - 1] + weight });
    }
  }

  return matrix[l1][l2];
}

// used to give helpful diagnostics if the user tries to use a variable that doesn't exist
// TODO: figure out what kind of range this thing produces and emit suggestions below a certain value
// since right now if you only have 1 variable called "zebra" and you try and call on the value of "pancakes"
// the compiler will eagerly ask you if you meant "zebra"
static std::optional<std::string> FindMostSimilarVarName(const std::string& varName, const std::vector<Local>& localList)
{
  std::optional<std::string> res = std::nullopt;
  std::size_t current = std::numeric_limits<std::size_t>::max();

  for (const auto& l : localList) {
    if (l.name == "__ARGS") continue;
    auto editDistance = GetEditDistance(varName, l.name);
    if (editDistance < current) {
      current = editDistance;
      res = l.name;
    }
  }

  return res;
}

static void Expression(bool canAssign, CompilerContext& compiler)
{
  // can't start an expression with an operator...
  if (IsOperator(compiler.current.value().GetType())) {
    MessageAtCurrent("Expected identifier or literal at start of expression", LogLevel::Error, compiler);
    Advance(compiler);
    return;
  }

  // ... or a keyword
  std::string kw;
  if (IsKeyword(compiler.current.value().GetType(), kw)) {
    MessageAtCurrent(fmt::format("'{}' is a keyword and not valid in this context", kw), LogLevel::Error, compiler);
    Advance(compiler);  //consume the illegal identifier
    return;
  }

  // only identifiers or literals
  if (Check(Scanner::TokenType::Identifier, compiler)) {
    Call(canAssign, compiler);

    if (Check(Scanner::TokenType::Equal, compiler)) {  
      if (compiler.previous.value().GetType() != Scanner::TokenType::Identifier) {
        MessageAtCurrent("Only identifiers can be assigned to", LogLevel::Error, compiler);
        return;
      }

      auto localName = compiler.previous.value().GetString();
      auto it = std::find_if(compiler.locals.begin(), compiler.locals.end(), [&localName](const Local& l) { return l.name == localName; });
      if (it == compiler.locals.end()) {
        auto mostSimilarVar = FindMostSimilarVarName(localName, compiler.locals);
        if (mostSimilarVar.has_value()) {
          MessageAtPrevious(fmt::format("Cannot find variable '{}' in this scope, did you mean '{}'?", localName, *mostSimilarVar), LogLevel::Error, compiler);
        }
        else {
          MessageAtPrevious(fmt::format("Cannot find variable '{}' in this scope", localName), LogLevel::Error, compiler);
        }
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

      if (!canAssign) {
        MessageAtCurrent("Assignment is not valid in the current context", LogLevel::Error, compiler);
        return;
      }

      Advance(compiler);  // consume the equals

      auto prevUsing = compiler.usingExpressionResult;
      compiler.usingExpressionResult = true;
      Expression(false, compiler); // disallow x = y = z...
      compiler.usingExpressionResult = prevUsing;

      EmitConstant(it->index);
      EmitOp(VM::Ops::AssignLocal, compiler.previous.value().GetLine());
    } else {
      // not an assignment, so just keep parsing this expression
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
          case Scanner::TokenType::Dot:
            Advance(compiler);  // consume the .
            Dot(canAssign, compiler);
            break;
          default:
            MessageAtCurrent("Invalid token found in expression", LogLevel::Error, compiler);
            Advance(compiler);
            return;
        }
      }
    }
  } else {
    // this expression started with a value literal, so start the recursive descent
    Or(canAssign, false, compiler);
  }
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
    EmitConstant(compiler.previous.value().GetString());
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
  auto constIdx = VM::VM::GetNumConstants();
  EmitConstant(std::int64_t{});
  auto opIdx = VM::VM::GetNumConstants();
  EmitConstant(std::int64_t{});
  EmitOp(VM::Ops::Jump, compiler.previous.value().GetLine());

  // we will patch in these indexes when the containing loop finishes
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
  auto constIdx = VM::VM::GetNumConstants();
  EmitConstant(std::int64_t{});
  auto opIdx = VM::VM::GetNumConstants();
  EmitConstant(std::int64_t{});
  EmitOp(VM::Ops::Jump, compiler.previous.value().GetLine());

  // we will patch in these indexes when the containing loop finishes
  compiler.continueIdxPairs.top().push_back(std::make_pair(constIdx, opIdx));
  Consume(Scanner::TokenType::Semicolon, "Expected ';' after `break`", compiler);
}

static std::unordered_map<Scanner::TokenType, std::int64_t> s_CastOps = {
  std::make_pair(Scanner::TokenType::IntIdent, 0),
  std::make_pair(Scanner::TokenType::FloatIdent, 1),
  std::make_pair(Scanner::TokenType::BoolIdent, 2),
  std::make_pair(Scanner::TokenType::StringIdent, 3),
  std::make_pair(Scanner::TokenType::CharIdent, 4),
  std::make_pair(Scanner::TokenType::ExceptionIdent, 5),
  std::make_pair(Scanner::TokenType::KeyValuePairIdent, 6),
};

static void ForStatement(CompilerContext& compiler)
{
  compiler.codeContextStack.emplace_back(CodeContext::ForLoop);

  compiler.breakIdxPairs.emplace();
  compiler.continueIdxPairs.emplace();

  // parse iterator variable
  auto firstItIsFinal = false;
  if (Match(Scanner::TokenType::Final, compiler)) {
    firstItIsFinal = true;
  }
  Consume(Scanner::TokenType::Identifier, "Expected identifier after `for`", compiler);
  auto iteratorNeedsPop = false, secondIteratorNeedsPop = false, twoIterators = false;
  auto iteratorName = compiler.previous.value().GetString();
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
    auto secondIteratorName = compiler.previous.value().GetString();
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
  auto startConstantIdx = static_cast<std::int64_t>(VM::VM::GetNumConstants());
  auto startOpIdx = static_cast<std::int64_t>(VM::VM::GetNumOps());

  // evaluate the condition
  EmitOp(VM::Ops::CheckIteratorEnd, line);

  auto endJumpConstantIndex = VM::VM::GetNumConstants();
  EmitConstant(std::int64_t{});
  auto endJumpOpIndex = VM::VM::GetNumConstants();
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
    for (auto& [constantIdx, opIdx] : compiler.continueIdxPairs.top()) {
      VM::VM::SetConstantAtIndex(constantIdx, static_cast<std::int64_t>(VM::VM::GetNumConstants()));
      VM::VM::SetConstantAtIndex(opIdx, static_cast<std::int64_t>(VM::VM::GetNumOps()));
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
    for (auto& [constantIdx, opIdx] : compiler.breakIdxPairs.top()) {
      VM::VM::SetConstantAtIndex(constantIdx, static_cast<std::int64_t>(VM::VM::GetNumConstants()));
      VM::VM::SetConstantAtIndex(opIdx, static_cast<std::int64_t>(VM::VM::GetNumOps()));
    }
    compiler.breakIdxPairs.pop();
    compiler.breakJumpNeedsIndexes = !compiler.breakIdxPairs.empty();

    if (compiler.locals.size() != static_cast<std::size_t>(numLocalsStart)) {
      EmitConstant(numLocalsStart);
      EmitOp(VM::Ops::PopLocals, line);
    }
  }
  
  VM::VM::SetConstantAtIndex(endJumpConstantIndex, static_cast<std::int64_t>(VM::VM::GetNumConstants()));
  VM::VM::SetConstantAtIndex(endJumpOpIndex, static_cast<std::int64_t>(VM::VM::GetNumOps()));
  
  // get rid of any variables made within the loop scope
  while (compiler.locals.size() != static_cast<std::size_t>(numLocalsStart)) {
    compiler.locals.pop_back();
  }

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
  auto topConstantIdxToJump = VM::VM::GetNumConstants();
  EmitConstant(std::int64_t{});
  auto topOpIdxToJump = VM::VM::GetNumConstants();
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
      auto endConstantIdx = VM::VM::GetNumConstants();
      EmitConstant(std::int64_t{});
      auto endOpIdx = VM::VM::GetNumConstants();
      EmitConstant(std::int64_t{});
      EmitOp(VM::Ops::Jump, compiler.previous.value().GetLine());
      
      endJumpIndexPairs.emplace_back(endConstantIdx, endOpIdx);

      auto numConstants = VM::VM::GetNumConstants();
      auto numOps = VM::VM::GetNumOps();
      
      // haven't told the 'if' where to jump to yet if its condition fails 
      if (!topJumpSet) {
        VM::VM::SetConstantAtIndex(topConstantIdxToJump, static_cast<std::int64_t>(numConstants));
        VM::VM::SetConstantAtIndex(topOpIdxToJump, static_cast<std::int64_t>(numOps));
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

  auto numConstants = static_cast<std::int64_t>(VM::VM::GetNumConstants());
  auto numOps = static_cast<std::int64_t>(VM::VM::GetNumOps());

  for (auto [constIdx, opIdx] : endJumpIndexPairs) {
    VM::VM::SetConstantAtIndex(constIdx, numConstants);
    VM::VM::SetConstantAtIndex(opIdx, numOps);
  }
  
  // if there was no else or elseif block, jump to here
  if (!topJumpSet) {
    VM::VM::SetConstantAtIndex(topConstantIdxToJump, numConstants);
    VM::VM::SetConstantAtIndex(topOpIdxToJump, numOps);
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

  if (VM::VM::GetLastFunctionName() == "main") {
    MessageAtPrevious("Cannot return from main function", LogLevel::Error, compiler);
    return;
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

  // don't destroy these locals here in the compiler's list because this could be an early return
  // that is handled at the end of `FuncDeclaration()`
  // but the VM needs to destroy any locals made up until this point
  // emit this instruction here since a local may have been used in the return expression
  // the result of the expression will be living on the stack still after PopLocals for Return to use
  if (!compiler.locals.empty()) {
    EmitConstant(std::int64_t{ 0 });
    EmitOp(VM::Ops::PopLocals, compiler.previous.value().GetLine());
  }

  EmitOp(VM::Ops::Return, compiler.previous.value().GetLine());
  Consume(Scanner::TokenType::Semicolon, "Expected ';' after expression", compiler);
}

static void TryStatement(CompilerContext& compiler)
{
  compiler.codeContextStack.emplace_back(CodeContext::Try);

  Consume(Scanner::TokenType::Colon, "Expected `:` after `try`", compiler);

  auto numLocalsStart = static_cast<std::int64_t>(compiler.locals.size());

  auto catchOpJumpIdx = VM::VM::GetNumConstants();
  EmitConstant(std::int64_t{});
  auto catchConstJumpIdx = VM::VM::GetNumConstants();
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
  auto skipCatchConstJumpIdx = VM::VM::GetNumConstants();
  EmitConstant(std::int64_t{});
  auto skipCatchOpJumpIdx = VM::VM::GetNumConstants();
  EmitConstant(std::int64_t{});
  EmitOp(VM::Ops::Jump, compiler.previous.value().GetLine());

  // but if there was an exception, jump here, and also pop the locals but continue into the catch block
  VM::VM::SetConstantAtIndex(catchOpJumpIdx, static_cast<std::int64_t>(VM::VM::GetNumOps()));
  VM::VM::SetConstantAtIndex(catchConstJumpIdx, static_cast<std::int64_t>(VM::VM::GetNumConstants()));

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

  auto exceptionVarName = compiler.previous.value().GetString();
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

  VM::VM::SetConstantAtIndex(skipCatchConstJumpIdx, static_cast<std::int64_t>(VM::VM::GetNumConstants()));
  VM::VM::SetConstantAtIndex(skipCatchOpJumpIdx, static_cast<std::int64_t>(VM::VM::GetNumOps()));

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

  auto constantIdx = static_cast<std::int64_t>(VM::VM::GetNumConstants());
  auto opIdx = static_cast<std::int64_t>(VM::VM::GetNumOps());

  auto prevUsing = compiler.usingExpressionResult;
  compiler.usingExpressionResult = true;
  Expression(false, compiler);
  compiler.usingExpressionResult = prevUsing;

  auto line = compiler.previous.value().GetLine();

  // evaluate the condition
  auto endConstantJumpIdx = VM::VM::GetNumConstants();
  EmitConstant(std::int64_t{});
  auto endOpJumpIdx = VM::VM::GetNumConstants();
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

  auto numConstants = static_cast<std::int64_t>(VM::VM::GetNumConstants());
  auto numOps = static_cast<std::int64_t>(VM::VM::GetNumOps());

  if (compiler.continueJumpNeedsIndexes) {
    for (auto& [c, o] : compiler.continueIdxPairs.top()) {
      VM::VM::SetConstantAtIndex(c, numConstants);
      VM::VM::SetConstantAtIndex(o, numOps);
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

  numConstants = static_cast<std::int64_t>(VM::VM::GetNumConstants());
  numOps = static_cast<std::int64_t>(VM::VM::GetNumOps());

  if (compiler.breakJumpNeedsIndexes) {
    for (auto& [c, o] : compiler.breakIdxPairs.top()) {
      VM::VM::SetConstantAtIndex(c, numConstants);
      VM::VM::SetConstantAtIndex(o, numOps);
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

  numConstants = static_cast<std::int64_t>(VM::VM::GetNumConstants());
  numOps = static_cast<std::int64_t>(VM::VM::GetNumOps());

  VM::VM::SetConstantAtIndex(endConstantJumpIdx, numConstants);
  VM::VM::SetConstantAtIndex(endOpJumpIdx, numOps);

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
    // Unreachable?
    Expression(canAssign, compiler);
  }

  if (Match(Scanner::TokenType::Dot, compiler)) {
    // TODO: account for class member access, not just function calls...
    Dot(canAssign, compiler);
  }
}

static void Dot(bool canAssign, CompilerContext& compiler)
{
  if (!Match(Scanner::TokenType::Identifier, compiler)) {
    MessageAtCurrent("Expected identifier after '.'", LogLevel::Error, compiler);
    return;
  }
    
  auto memberNameToken = compiler.previous.value();
  if (Match(Scanner::TokenType::LeftParen, compiler)) {
    // object.function()
    DotFunctionCall(memberNameToken, compiler);
  } else {
    // accessing a member
    // object.member
    // could either be an assignment, loading the value, or more dots follow
    // no other tokens would be valid
    if (Match(Scanner::TokenType::Equal, compiler)) {
      if (!canAssign) {
        MessageAtPrevious("Assignment is not valid here", LogLevel::Error, compiler);
        return;
      }

      // todo..
      auto prevUsing = compiler.usingExpressionResult;
      compiler.usingExpressionResult = true;
      Expression(false, compiler);
      compiler.usingExpressionResult = prevUsing;
      EmitConstant(memberNameToken.GetString());
      EmitOp(VM::Ops::AssignMember, compiler.previous.value().GetLine());

    } else if (Match(Scanner::TokenType::Dot, compiler)) {
      // load the member, then recurse
      // the parent is on the top of the stack
      EmitConstant(memberNameToken.GetString());
      EmitOp(VM::Ops::LoadMember, memberNameToken.GetLine());
      Dot(canAssign, compiler);
    } else {
      // no further access, end of expression
      // so we need to just load the local
      if (canAssign) {
        // this was supposed to be an assignment
        MessageAtCurrent("Expected expression", LogLevel::Error, compiler);
        return;
      }

      EmitConstant(memberNameToken.GetString());
      EmitOp(VM::Ops::LoadMember, memberNameToken.GetLine());
    }
  }    
}

static void DotFunctionCall(const Scanner::Token& funcNameToken, CompilerContext& compiler)
{
  // the last object on the stack is the object we are calling
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

  static std::hash<std::string> hasher;
  auto funcName = funcNameToken.GetString();
  EmitConstant(funcName);
  EmitConstant(static_cast<std::int64_t>(hasher(funcName)));
  EmitConstant(numArgs);
  EmitOp(VM::Ops::MemberCall, funcNameToken.GetLine());

  if (!compiler.usingExpressionResult) {
    // pop unused return value
    EmitOp(VM::Ops::Pop, compiler.previous.value().GetLine());
  }
}

static void Identifier(bool canAssign, CompilerContext& compiler)
{
  auto prev = compiler.previous.value();
  auto prevText = compiler.previous.value().GetString();

  if (Match(Scanner::TokenType::LeftParen, compiler)) {
    FreeFunctionCall(prev, compiler);
  } else if (Match(Scanner::TokenType::ColonColon, compiler)) {
    if (!Check(Scanner::TokenType::Identifier, compiler)) {
      MessageAtCurrent("Expected identifier after `::`", LogLevel::Error, compiler);
      return;
    }
    
    if (IsLiteral(compiler.current.value().GetType())) {
      MessageAtCurrent("Expected identifier after `::`", LogLevel::Error, compiler);
      return;
    }

    if (!compiler.currentNamespaceLookup.empty()) {
      compiler.currentNamespaceLookup += "/";
    }

    if (compiler.namespaceQualifierUsed) {
      EmitOp(VM::Ops::StartNewNamespace, prev.GetLine());
      compiler.namespaceQualifierUsed = false;
      compiler.currentNamespaceLookup.clear();
    }

    static std::hash<std::string> hasher;
    EmitConstant(prevText);
    EmitConstant(static_cast<std::int64_t>(hasher(prevText)));
    EmitOp(VM::Ops::AppendNamespace, prev.GetLine());
    
    compiler.currentNamespaceLookup += prevText;
    
    Expression(canAssign, compiler);
  } else {
    // not a call or member access, so we are just trying to call on the value of the local
    // or reassign it 
    // if it's not a reassignment, we are trying to load its value
    // Primary() has already but the variable's id on the stack
    if (!Check(Scanner::TokenType::Equal, compiler)) {
      auto localIt = std::find_if(compiler.locals.begin(), compiler.locals.end(), [&prevText](const Local& l){ return l.name == prevText; });
      if (localIt == compiler.locals.end()) {
        // now check for constants...
        auto constantIt = s_FileConstantsLookup[compiler.currentFileName].find(prevText);
        if (constantIt == s_FileConstantsLookup[compiler.currentFileName].end()) {
          // might be in a namespace...
          auto importedConstantIt = s_FileConstantsLookup[compiler.currentNamespaceLookup + ".gr"].find(prevText);
          if (importedConstantIt == s_FileConstantsLookup[compiler.currentNamespaceLookup + ".gr"].end()) {
            auto mostSimilarVar = FindMostSimilarVarName(prevText, compiler.locals);
            if (mostSimilarVar.has_value()) {
              MessageAtPrevious(fmt::format("Cannot find variable '{}' in this scope, did you mean '{}'?", prevText, *mostSimilarVar), LogLevel::Error, compiler);
            } else {
              MessageAtPrevious(fmt::format("Cannot find variable '{}' in this scope", prevText), LogLevel::Error, compiler);
            }
          } else {
            if (!importedConstantIt->second.isExported) {
              MessageAtPrevious(fmt::format("Constant '{}' has not been exported", prevText), LogLevel::Error, compiler);
              return;
            }
            compiler.namespaceQualifierUsed = true;
            EmitConstant(importedConstantIt->second.value);
            EmitOp(VM::Ops::LoadConstant, prev.GetLine());
          }
        } else {
          EmitConstant(constantIt->second.value);
          EmitOp(VM::Ops::LoadConstant, prev.GetLine());
        }
      } else {
        EmitConstant(localIt->index);
        EmitOp(VM::Ops::LoadLocal, prev.GetLine());
      }
    }
  }
}

static void FreeFunctionCall(const Scanner::Token& funcNameToken, CompilerContext& compiler)
{
  compiler.namespaceQualifierUsed = true;

  static std::hash<std::string> hasher;
  auto funcNameText = funcNameToken.GetString();
  auto hash = static_cast<std::int64_t>(hasher(funcNameText));
  auto nativeCall = funcNameText.starts_with("__");
  std::size_t nativeIndex{};
  if (nativeCall) {
    auto [exists, index] = VM::VM::HasNativeFunction(funcNameText);
    if (!exists) {
      Message(funcNameToken, fmt::format("No native function matching the given signature `{}` was found", funcNameText), LogLevel::Error, compiler);
      return;
    }
    else {
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
    auto arity = VM::VM::GetNativeFunction(nativeIndex).GetArity();
    if (numArgs != arity) {
      MessageAtPrevious(fmt::format("Incorrect number of arguments given to native call - got {} but expected {}", numArgs, arity), LogLevel::Error, compiler);
      return;
    }
  }

  // TODO: in a script that is not being ran as the main script, 
  // there may be a function called 'main' that you should be allowed to call
  if (funcNameText == "main") {
    Message(funcNameToken, "Cannot call the `main` function", LogLevel::Error, compiler);
    return;
  }

  EmitConstant(nativeCall ? static_cast<std::int64_t>(nativeIndex) : hash);
  EmitConstant(numArgs);
  if (nativeCall) {
    EmitOp(VM::Ops::NativeCall, compiler.previous.value().GetLine());
  }
  else {
    EmitConstant(funcNameText);
    EmitOp(VM::Ops::Call, compiler.previous.value().GetLine());
  }

  if (Check(Scanner::TokenType::Semicolon, compiler) && !compiler.usingExpressionResult) {
    // pop unused return value
    EmitOp(VM::Ops::Pop, compiler.previous.value().GetLine());
  }
}

static void Char(CompilerContext& compiler)
{
  char res;
  auto err = TryParseChar(compiler.previous.value(), res);
  if (err.has_value()) {
    MessageAtPrevious(fmt::format("Token could not be parsed as char: {}", err.value()), LogLevel::Error, compiler);
    return;
  }
  EmitOp(VM::Ops::LoadConstant, compiler.previous.value().GetLine());
  EmitConstant(res);  
}

static void String(CompilerContext& compiler)
{
  std::string res;
  auto err = TryParseString(compiler.previous.value(), res);
  if (err.has_value()) {
    MessageAtPrevious(fmt::format("Token could not be parsed as string: {}", err.value()), LogLevel::Error, compiler);
    return;
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
    case Scanner::TokenType::ExceptionIdent:
      EmitConstant(std::int64_t(8));
      break;
    case Scanner::TokenType::KeyValuePairIdent:
      EmitConstant(std::int64_t(9));
      break;
    case Scanner::TokenType::Identifier:
      EmitConstant(std::int64_t(10));
      EmitConstant(compiler.current.value().GetString());
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
  auto typeToken = compiler.current.value();
  Advance(compiler);
  Consume(Scanner::TokenType::LeftParen, "Expected '(' after type ident", compiler);

  auto isList = false;
  std::int64_t numListItems = 0;

  switch (typeToken.GetType()) {
    // these "Casts" construct the type with a single expression
    case Scanner::TokenType::IntIdent:
    case Scanner::TokenType::FloatIdent:
    case Scanner::TokenType::BoolIdent:
    case Scanner::TokenType::StringIdent:
    case Scanner::TokenType::CharIdent:
    // NB: Exception will convert the expression to a string
    case Scanner::TokenType::ExceptionIdent: {
      auto prevUsing = compiler.usingExpressionResult;
      compiler.usingExpressionResult = true;
      Expression(false, compiler);
      compiler.usingExpressionResult = prevUsing;
  
      EmitConstant(s_CastOps[typeToken.GetType()]);
      EmitOp(VM::Ops::Cast, compiler.current.value().GetLine());
      break;
    }
    case Scanner::TokenType::ListIdent: {
      isList = true;

      auto prevUsing = compiler.usingExpressionResult;
      compiler.usingExpressionResult = true;
      while (true) {
        if (Check(Scanner::TokenType::RightParen, compiler)) {
          break;
        }
        
        if (Match(Scanner::TokenType::EndOfFile, compiler)) {
          MessageAtPrevious("Unterminated `List` constructor", LogLevel::Error, compiler);
          return;
        }

        Expression(false, compiler);
        numListItems++;

        if (Check(Scanner::TokenType::RightParen, compiler)) {
          break;
        }

        if (!Match(Scanner::TokenType::Comma, compiler)) {
          MessageAtPrevious("Expected ',' between `List` items", LogLevel::Error, compiler);
          return;
        }
      }
      compiler.usingExpressionResult = prevUsing;
      break;
    }
    case Scanner::TokenType::DictIdent: {
      // TODO: maybe there's some special functionality we want to include in the future with the syntax `Dict(...)`
      Message(typeToken, "Cannot use `Dict` like a constructor, use literal expression `{ key: value, ... }`", LogLevel::Error, compiler);
      return;
    }
    case Scanner::TokenType::KeyValuePairIdent: {
      auto prevUsing = compiler.usingExpressionResult;
      compiler.usingExpressionResult = true;
      Expression(false, compiler);

      Consume(Scanner::TokenType::Comma, "Expected ',' between key and value in `KeyValuePair` constructor", compiler);

      Expression(false, compiler);
      compiler.usingExpressionResult = prevUsing;

      EmitConstant(s_CastOps[typeToken.GetType()]);
      EmitOp(VM::Ops::Cast, compiler.current.value().GetLine());
      break;
    }
    default:
      GRACE_UNREACHABLE();
      break;
  }
  
  Consume(Scanner::TokenType::RightParen, "Expected ')' after expression", compiler);

  if (isList) {
    EmitConstant(numListItems);
    EmitOp(VM::Ops::CreateList, compiler.previous.value().GetLine());
  }

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
  Message(compiler.current.value(), message, level, compiler);
}

static void MessageAtPrevious(const std::string& message, LogLevel level, CompilerContext& compiler)
{
  Message(compiler.previous.value(), message, level, compiler);
}

static void Message(const Scanner::Token& token, const std::string& message, LogLevel level, CompilerContext& compiler)
{
  if (level == LogLevel::Error || s_WarningsError) {
    if (compiler.panicMode) return;
    compiler.panicMode = true;
  }

  auto colour = fmt::fg(level == LogLevel::Error ? fmt::color::red : fmt::color::orange);
  fmt::print(stderr, colour | fmt::emphasis::bold, level == LogLevel::Error ? "ERROR: " : "WARNING: ");
  
  auto type = token.GetType();
  switch (type) {
    case Scanner::TokenType::EndOfFile:
      fmt::print(stderr, "at end: ");
      fmt::print(stderr, "{}\n", message);
      break;
    case Scanner::TokenType::Error:
      fmt::print(stderr, "{}\n", token.GetErrorMessage());
      break;
    default:
      fmt::print(stderr, "at '{}': ", token.GetText());
      fmt::print(stderr, "{}\n", message);
      break;
  }

  auto lineNo = static_cast<int>(token.GetLine());
  auto column = token.GetColumn() - token.GetLength();  // need the START of the token
  fmt::print(stderr, "       --> {}:{}:{}\n", compiler.currentFileName, lineNo, column + 1); 
  fmt::print(stderr, "        |\n");

  if (lineNo > 1) {
    fmt::print(stderr, "{:>7} | {}\n", lineNo - 1, Scanner::GetCodeAtLine(compiler.currentFileName, lineNo - 1));
  }

  fmt::print(stderr, "{:>7} | {}\n", lineNo, Scanner::GetCodeAtLine(compiler.currentFileName, lineNo));
  fmt::print(stderr, "        | ");
  for (std::size_t i = 0; i < column; i++) {
    fmt::print(stderr, " ");
  }
  for (std::size_t i = 0; i < token.GetLength(); i++) {
    fmt::print(stderr, colour, "^");
  }

  fmt::print(stderr, "\n");
  fmt::print(stderr, "{:>7} | {}\n", lineNo + 1, Scanner::GetCodeAtLine(compiler.currentFileName, lineNo + 1));
  fmt::print(stderr, "        |\n\n");

  if (level == LogLevel::Error) {
    compiler.hadError = true;
  }
  if (level == LogLevel::Warning) {
    compiler.hadWarning = true;
  }
}
