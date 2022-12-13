/*
 *  The Grace Programming Language.
 *
 *  This file contains the out of line definitions for the Grace Compiler, which outputs Grace bytecode based on Tokens provided by the Scanner.
 *  
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#include "compiler.hpp"
#include "compiler_helpers.hpp"

#include <fmt/color.h>

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

using namespace Grace;
using namespace Grace::Compiler;

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
      : name(std::move(name))
      , isFinal(final)
      , isIterator(iterator)
      , index(index)
  {
  }
};

struct CompilerContext
{
  CompilerContext(std::string fileName_, const std::filesystem::path& parentPath_, std::string&& code)
      : fileName { std::move(fileName_) }
  {
    parentPath = std::filesystem::absolute(parentPath_);
    fullPath = std::filesystem::absolute(parentPath / std::filesystem::path(fileName).filename());
    Scanner::InitScanner(fullPath.string(), std::move(code));
    codeContextStack.push_back(CodeContext::TopLevel);
  }

  ~CompilerContext()
  {
    Scanner::PopScanner();
  }

  std::vector<CodeContext> codeContextStack;
  std::string fileName;
  std::filesystem::path fullPath, parentPath;

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
  VM::VM::Instance().PushOp(op, line);
}

template<VM::BuiltinGraceType T>
static void EmitConstant(T value)
{
  VM::VM::Instance().PushConstant(std::move(value));
}

static void EmitConstant(VM::Value value)
{
  VM::VM::Instance().PushConstant(std::move(value));
}

static bool AddFunction(std::string name, std::size_t arity, std::string fileName, bool exported, bool extension, std::size_t objectNameHash = {})
{
  return VM::VM::Instance().AddFunction(std::move(name), arity, std::move(fileName), exported, extension, objectNameHash);
}

static bool AddClass(std::string name, std::string fileName)
{
  return VM::VM::Instance().AddClass(std::move(name), std::move(fileName));
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
static bool ParseCallParameters(CompilerContext& compiler, int64_t& numArgs);
static void Dot(bool canAssign, CompilerContext& compiler);
static void Subscript(bool canAssign, CompilerContext& compiler);
static void Identifier(bool canAssign, CompilerContext& compiler);
static void Char(CompilerContext& compiler);
static void String(CompilerContext& compiler);
static void InstanceOf(CompilerContext& compiler);
static void IsObject(CompilerContext& compiler);
static void Cast(CompilerContext& compiler);
static void List(CompilerContext& compiler);
static void Dictionary(CompilerContext& compiler);
// static void Range(CompilerContext& compiler);
static void Typename(CompilerContext& compiler);

enum class LogLevel
{
  Warning,
  Error,
};

static void MessageAtCurrent(const std::string& message, LogLevel level, CompilerContext& compiler);
static void MessageAtPrevious(const std::string& message, LogLevel level, CompilerContext& compiler);
static void Message(const Scanner::Token& token, const std::string& message, LogLevel level, CompilerContext& compiler);

GRACE_NODISCARD static VM::InterpretResult Finalise(std::string mainFileName, bool verbose, std::vector<std::string> args);

static bool s_Verbose, s_WarningsError;
static std::stack<CompilerContext> s_CompilerContextStack;

struct Constant
{
  VM::Value value;
  bool isExported;
};

static std::unordered_map<std::string, std::unordered_map<std::string, Constant>> s_FileConstantsLookup;

VM::InterpretResult Grace::Compiler::Compile(std::string fileName, bool verbose, bool warningsError, std::vector<std::string> args)
{
  using namespace std::chrono;

  auto start = steady_clock::now();

  std::stringstream inFileStream;
  std::ifstream inFile;
  inFile.open(fileName);

  if (inFile.fail()) {
    fmt::print(stderr, "Error reading file `{}`\n", fileName);
    return VM::InterpretResult::RuntimeError;
  }

  inFileStream << inFile.rdbuf();

  s_Verbose = verbose;
  s_WarningsError = warningsError;

  s_CompilerContextStack.emplace(fileName, std::filesystem::absolute(std::filesystem::path(fileName)).parent_path(), inFileStream.str());

  auto fullPath = s_CompilerContextStack.top().fullPath.string();
  s_FileConstantsLookup[fullPath]["__FILE"] = { VM::Value(fullPath), false };

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
#ifdef GRACE_MSC
        fmt::print("Compilation succeeded in {} \xE6s.\n", duration);
#else
        fmt::print("Compilation succeeded in {} µs.\n", duration);
#endif
      }
    }
    return Finalise(std::move(fileName), verbose, std::move(args));
  }

  return VM::InterpretResult::RuntimeError;
}

static VM::InterpretResult Finalise(std::string mainFileName, bool verbose, std::vector<std::string> args)
{
#ifdef GRACE_DEBUG
  if (verbose) {
    VM::VM::Instance().PrintOps();
  }
#endif
  if (VM::VM::Instance().CombineFunctions(mainFileName, verbose)) {
    return VM::VM::Instance().Start(mainFileName, verbose, args);
  }
  return VM::InterpretResult::RuntimeError;
}

static void Advance(CompilerContext& compiler)
{
  compiler.previous = compiler.current;
  compiler.current = Scanner::ScanToken();

#ifdef GRACE_DEBUG
  if (s_Verbose) {
    fmt::print("{}\n", compiler.current->ToString());
  }
#endif

  if (compiler.current->GetType() == Scanner::TokenType::Error) {
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
  return compiler.current && compiler.current->GetType() == expected;
}

static void Consume(Scanner::TokenType expected, const std::string& message, CompilerContext& compiler)
{
  if (compiler.current->GetType() == expected) {
    Advance(compiler);
    return;
  }

  MessageAtCurrent(message, LogLevel::Error, compiler);
}

static void Synchronize(CompilerContext& compiler)
{
  compiler.panicMode = false;

  while (compiler.current->GetType() != Scanner::TokenType::EndOfFile) {
    if (compiler.previous && compiler.previous->GetType() == Scanner::TokenType::Semicolon) {
      return;
    }

    // go until we find the start of a new declaration
    switch (compiler.current->GetType()) {
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
    VarDeclaration(compiler, compiler.previous->GetType() == Scanner::TokenType::Final);
  } else if (Match(Scanner::TokenType::Const, compiler)) {
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
      Advance(compiler); // consume illegal catch
      return;
    }
  } else {
    ExpressionStatement(compiler);
  }
}

// returns true if the name is a duplicate
static bool CheckForDuplicateLocalName(const std::string& varName, const CompilerContext& compiler)
{
  auto localsIt = std::find_if(compiler.locals.begin(), compiler.locals.end(), [&varName](const Local& local) { return local.name == varName; });
  return localsIt != compiler.locals.end();
}

// returns true if the name is a duplicate
static bool CheckForDuplicateConstantName(const std::string& constName, const CompilerContext& compiler)
{
  auto& constantsList = s_FileConstantsLookup[compiler.fullPath.string()];
  auto it = constantsList.find(constName);
  return it != constantsList.end();
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

    auto txt = compiler.previous->GetText();
    if (!isStdImport && txt == "std") {
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
  if (isStdImport) {
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

    inPath = fs::path(stdPath) / fs::path(importPath.substr(4)); // trim off 'std/' because that's contained within the path environment variable
  } else {
    inPath = std::filesystem::absolute(compiler.parentPath / importPath);
  }

  if (!fs::exists(inPath)) {
    Message(*lastPathToken, fmt::format("Could not find file `{}` to import", importPath), LogLevel::Error, compiler);
    return;
  }

  if (Scanner::HasFile(inPath.string())) {
    // don't produce a warning, but silently ignore the duplicate import
    // TODO: maybe ignore this in the std, but warn the user if they have duplicate imports in one file?
    return;
  }

  std::stringstream inFileStream;
  std::ifstream inFile;
  inFile.open(inPath);

  if (inFile.fail()) {
    Message(*lastPathToken, fmt::format("Error reading imported file `{}`\n", inPath.string()), LogLevel::Error, compiler);
    return;
  }

  inFileStream << inFile.rdbuf();

  s_CompilerContextStack.emplace(std::move(importPath), inPath.parent_path(), inFileStream.str());

  auto fullPath = s_CompilerContextStack.top().fullPath.string();
  s_FileConstantsLookup[fullPath]["__FILE"] = { VM::Value(fullPath), false };
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

  auto classNameToken = *compiler.previous;

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

      auto memberName = compiler.previous->GetString();

      if (Match(Scanner::TokenType::Colon, compiler)) {
        if (!IsValidTypeAnnotation(compiler.current->GetType())) {
          MessageAtCurrent("Expected typename after type annotation", LogLevel::Error, compiler);
          return;
        }
        Advance(compiler);
      }

      if (memberName.starts_with("__")) {
        MessageAtPrevious("Names beginning with double underscore `__` are reserved for internal use", LogLevel::Error, compiler);
        return;
      }

      auto it = std::find_if(classMembers.begin(), classMembers.end(), [&memberName](const std::string& l) { return l == memberName; });
      if (it != classMembers.end()) {
        MessageAtPrevious("A class member with the same name already exists", LogLevel::Error, compiler);
        return;
      }

      if (CheckForDuplicateConstantName(memberName, compiler)) {
        MessageAtPrevious("A constant with the same name already exists", LogLevel::Error, compiler);
        return;
      }

      classMembers.push_back(memberName);

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
          auto isFinal = compiler.previous->GetType() == Scanner::TokenType::Final;
          if (isFinal) {
            Consume(Scanner::TokenType::Identifier, "Expected identifier after `final`", compiler);
          }
          auto p = compiler.previous->GetString();

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

          if (CheckForDuplicateConstantName(p, compiler)) {
            MessageAtPrevious("A constant with the same name already exists", LogLevel::Error, compiler);
            return;
          }

          parameters.push_back(p);
          compiler.locals.emplace_back(std::move(p), isFinal, false, compiler.locals.size());

          if (Match(Scanner::TokenType::Colon, compiler)) {
            if (!IsValidTypeAnnotation(compiler.current->GetType())) {
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

      if (!AddFunction(classNameToken.GetString(), parameters.size(), compiler.fileName, exported, false)) {
        Message(classNameToken, "A function or class in the same namespace already exists with the same name as this class", LogLevel::Error, compiler);
        return;
      }

      // make the class' members at the start of the constructor
      for (auto memberName : classMembers) {
        EmitOp(VM::Ops::DeclareLocal, compiler.previous->GetLine());
        auto localId = compiler.locals.size();
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
        if (compiler.current->GetType() == Scanner::TokenType::EndOfFile) {
          MessageAtCurrent("Expected `end` after constructor", LogLevel::Error, compiler);
          return;
        }
      }

      if (compiler.locals.size() > numLocalsStart) {
        // pop locals we made inside the constructor
        EmitConstant(parameters.size());
        EmitOp(VM::Ops::PopLocals, compiler.previous->GetLine());
      }

      compiler.codeContextStack.pop_back();
    } else {
      MessageAtCurrent("Expected `var` or `constructor` inside class", LogLevel::Error, compiler);
      return;
    }
  }

  // make an empty constructor if the user doesn't define one
  if (!hasDefinedConstructor) {
    if (!AddFunction(classNameToken.GetString(), 0, compiler.fileName, exported, false)) {
      Message(classNameToken, "A function or class in the same namespace already exists with the same name as this class", LogLevel::Error, compiler);
      return;
    }

    for (auto memberName : classMembers) {
      // and declare the class' members as locals in this function that will be used by the VM to assign to the instance
      EmitOp(VM::Ops::DeclareLocal, compiler.previous->GetLine());
      auto localId = compiler.locals.size();
      compiler.locals.emplace_back(std::move(memberName), false, false, localId);
    }
  }

  if (!AddClass(classNameToken.GetString(), compiler.fileName)) {
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
  EmitConstant(classMembers.size());
  for (auto m : classMembers) {
    EmitConstant(std::move(m));
  }

  static std::hash<std::string> hasher;
  EmitConstant(hasher(classNameToken.GetString()));
  EmitConstant(hasher(compiler.fileName));

  EmitOp(VM::Ops::CreateInstance, compiler.previous->GetLine());

  EmitConstant(std::int64_t { 0 });
  EmitOp(VM::Ops::PopLocals, compiler.previous->GetLine());

  EmitOp(VM::Ops::Return, compiler.previous->GetLine());

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
  auto funcNameToken = *compiler.previous;
  auto name = compiler.previous->GetString();
  if (name.starts_with("__")) {
    MessageAtPrevious("Names beginning with double underscore `__` are reserved for internal use", LogLevel::Error, compiler);
    return;
  }
  auto isMainFunction = name == "main";

  Consume(Scanner::TokenType::LeftParen, "Expected '(' after function name", compiler);

  std::size_t extensionObjectNameHash {};
  auto isExtensionMethod = false;

  std::vector<std::string> parameters;
  while (!Match(Scanner::TokenType::RightParen, compiler)) {
    if (isMainFunction && parameters.size() > 1) {
      Message(funcNameToken, fmt::format("`main` function can only take 0 or 1 parameter(s) but got {}", parameters.size()), LogLevel::Error, compiler);
      return;
    }

    if (Match(Scanner::TokenType::Identifier, compiler) || Match(Scanner::TokenType::Final, compiler)) {
      auto isFinal = compiler.previous->GetType() == Scanner::TokenType::Final;
      if (isFinal) {
        Consume(Scanner::TokenType::Identifier, "Expected identifier after `final`", compiler);
      }
      auto p = compiler.previous->GetString();

      if (p.starts_with("__")) {
        MessageAtPrevious("Names beginning with double underscore `__` are reserved for internal use", LogLevel::Error, compiler);
        return;
      }

      if (std::find(parameters.begin(), parameters.end(), p) != parameters.end()) {
        MessageAtPrevious("Function parameters with the same name already defined", LogLevel::Error, compiler);
        return;
      }

      if (CheckForDuplicateConstantName(p, compiler)) {
        MessageAtPrevious("A constant with the same name already exists", LogLevel::Error, compiler);
        return;
      }

      parameters.push_back(p);
      compiler.locals.emplace_back(std::move(p), isFinal, false, compiler.locals.size());

      if (Match(Scanner::TokenType::Colon, compiler)) {
        if (!IsValidTypeAnnotation(compiler.current->GetType())) {
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

      auto type = compiler.current->GetType();
      if (!IsTypeIdent(type) && type != Scanner::TokenType::Identifier) {
        MessageAtCurrent("Expected type name for extension method", LogLevel::Error, compiler);
        return;
      }

      static std::hash<std::string> hasher;
      extensionObjectNameHash = hasher(compiler.current->GetString());

      Advance(compiler);
      isExtensionMethod = true;

      Consume(Scanner::TokenType::Identifier, "Expected identifier after type identifier", compiler);

      auto p = compiler.previous->GetString();

      if (p.starts_with("__")) {
        MessageAtPrevious("Names beginning with double underscore `__` are reserved for internal use", LogLevel::Error, compiler);
        return;
      }

      if (std::find(parameters.begin(), parameters.end(), p) != parameters.end()) {
        MessageAtPrevious("Function parameters with the same name already defined", LogLevel::Error, compiler);
        return;
      }

      if (CheckForDuplicateConstantName(p, compiler)) {
        MessageAtPrevious("A constant with the same name already exists", LogLevel::Error, compiler);
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
    if (!IsValidTypeAnnotation(compiler.current->GetType())) {
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
  if (isMainFunction && parameters.empty()) {
    compiler.locals.emplace_back("__ARGS", true, false, 0);
  }

  if (!Match(Scanner::TokenType::Colon, compiler)) {
    MessageAtCurrent("Expected ':' after function signature", LogLevel::Error, compiler);
    return;
  }

  if (!AddFunction(std::move(name), parameters.size(), compiler.fileName, exportFunction, isExtensionMethod, extensionObjectNameHash)) {
    Message(funcNameToken, "A function or class in the same namespace already exists with the same name as this function", LogLevel::Error, compiler);
    return;
  }

  while (!Match(Scanner::TokenType::End, compiler)) {
    Declaration(compiler);
    if (compiler.current->GetType() == Scanner::TokenType::EndOfFile) {
      MessageAtCurrent("Expected `end` after function", LogLevel::Error, compiler);
      return;
    }
  }

  // implicitly return if the user didn't write a return so the VM knows to return to the caller
  // functions with no return will implicitly return null, so setting a call equal to a variable is valid
  if (VM::VM::Instance().GetLastOp() != VM::Ops::Return) {
    if (!compiler.locals.empty()) {
      EmitConstant(std::int64_t { 0 });
      EmitOp(VM::Ops::PopLocals, compiler.previous->GetLine());
    }

    if (!isMainFunction) {
      EmitConstant(nullptr);
      EmitOp(VM::Ops::LoadConstant, compiler.previous->GetLine());
      EmitOp(VM::Ops::Return, compiler.previous->GetLine());
    }
  }

  compiler.locals.clear();

  if (isMainFunction) {
    EmitOp(VM::Ops::Exit, compiler.previous->GetLine());
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

  auto nameToken = *compiler.previous;

  if (Match(Scanner::TokenType::Colon, compiler)) {
    if (!IsValidTypeAnnotation(compiler.current->GetType())) {
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

  if (CheckForDuplicateLocalName(localName, compiler)) {
    MessageAtPrevious("A local variable with the same name already exists", LogLevel::Error, compiler);
    return;
  }

  if (CheckForDuplicateConstantName(localName, compiler)) {
    MessageAtPrevious("A constant with the same name already exists", LogLevel::Error, compiler);
    return;
  }

  auto line = nameToken.GetLine();

  auto localId = compiler.locals.size();
  EmitOp(VM::Ops::DeclareLocal, line);

  if (Match(Scanner::TokenType::Equal, compiler)) {
    auto prevUsing = compiler.usingExpressionResult;
    compiler.usingExpressionResult = true;
    Expression(false, compiler);
    compiler.usingExpressionResult = prevUsing;
    line = compiler.previous->GetLine();
    EmitConstant(localId);
    EmitOp(VM::Ops::AssignLocal, line);
  } else {
    if (isFinal) {
      MessageAtCurrent("Must assign to `final` upon declaration", LogLevel::Error, compiler);
      return;
    }
  }

  compiler.locals.emplace_back(std::move(localName), isFinal, false, localId);
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

  auto constantName = compiler.previous->GetString();

  if (CheckForDuplicateConstantName(constantName, compiler)) {
    MessageAtPrevious("A constant with the same name already exists", LogLevel::Error, compiler);
    return;
  }

  if (Match(Scanner::TokenType::Colon, compiler)) {
    if (!IsValidTypeAnnotation(compiler.current->GetType())) {
      MessageAtCurrent("Expected type name after type annotation", LogLevel::Error, compiler);
      return;
    }
    Advance(compiler);
  }

  if (!Match(Scanner::TokenType::Equal, compiler)) {
    MessageAtCurrent("Must assign to `const` upon declaration", LogLevel::Error, compiler);
    return;
  }

  bool isNegativeNumber = false;
  std::optional<Scanner::Token> valueToken {};

  if (IsLiteral(compiler.current->GetType())) {
    valueToken = *compiler.current;
    Advance(compiler);
  } else {
    if (Match(Scanner::TokenType::Minus, compiler)) {
      if (IsNumber(compiler.current->GetType())) {
        valueToken = *compiler.current;
        isNegativeNumber = true;
        Advance(compiler);
      } else {
        MessageAtCurrent(fmt::format("Cannot negate `{}`", compiler.current->GetType()), LogLevel::Error, compiler);
        return;
      }
    }
  }

  auto fullFilePath = compiler.fullPath.string();

  switch (valueToken->GetType()) {
    case Scanner::TokenType::True:
      s_FileConstantsLookup[fullFilePath][constantName] = { VM::Value(true), isExport };
      break;
    case Scanner::TokenType::False:
      s_FileConstantsLookup[fullFilePath][constantName] = { VM::Value(false), isExport };
      break;
    case Scanner::TokenType::Integer: {
      std::int64_t value;
      auto result = isNegativeNumber ? TryParseInt(*valueToken, value, 10, -1) : TryParseInt(*valueToken, value);
      if (result) {
        Message(*valueToken, fmt::format("Token could not be parsed as an int: {}", *result), LogLevel::Error, compiler);
        return;
      }
      s_FileConstantsLookup[fullFilePath][constantName] = { VM::Value(value), isExport };
      break;
    }
    case Scanner::TokenType::Double: {
      double value;
      auto result = TryParseDouble(*valueToken, value);
      if (result) {
        Message(*valueToken, fmt::format("Token could not be parsed as an float: {}", *result), LogLevel::Error, compiler);
        return;
      }
      s_FileConstantsLookup[fullFilePath][constantName] = { VM::Value(value * (isNegativeNumber ? -1.0 : 1.0)), isExport };
      break;
    }
    case Scanner::TokenType::String: {
      std::string res;
      auto err = TryParseString(*valueToken, res);
      if (err) {
        Message(*valueToken, fmt::format("Token could not be parsed as string: {}", *err), LogLevel::Error, compiler);
        return;
      }
      s_FileConstantsLookup[fullFilePath][constantName] = { VM::Value(res), isExport };
      break;
    }
    case Scanner::TokenType::Char: {
      char res;
      auto err = TryParseChar(*valueToken, res);
      if (err) {
        Message(*valueToken, fmt::format("Token could not be parsed as char: {}", *err), LogLevel::Error, compiler);
        return;
      }
      s_FileConstantsLookup[fullFilePath][constantName] = { VM::Value(res), isExport };
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

// used to give helpful diagnostics if the user tries to use a variable that doesn't exist
// TODO: figure out what kind of range this thing produces and emit suggestions below a certain value
// since right now if you only have 1 variable called "zebra" and you try and call on the value of "pancakes"
// the compiler will eagerly ask you if you meant "zebra"
static std::optional<std::string> FindMostSimilarVarName(const std::string& varName, const std::vector<Local>& localList)
{
  std::optional<std::string> res = std::nullopt;
  std::size_t current = std::numeric_limits<std::size_t>::max();

  for (const auto& l : localList) {
    if (l.name == "__ARGS")
      continue;
    auto editDistance = GetEditDistance(varName, l.name);
    if (editDistance < current) {
      current = editDistance;
      res = l.name;
    }
  }

  return res;
}

static void ExpressionStatement(CompilerContext& compiler)
{
  if (IsLiteral(compiler.current->GetType()) || IsOperator(compiler.current->GetType())) {
    MessageAtCurrent("Expected identifier or keyword at start of expression", LogLevel::Error, compiler);
    Advance(compiler); // consume illegal token
    return;
  }
  Expression(true, compiler);
  Consume(Scanner::TokenType::Semicolon, "Expected ';' after expression", compiler);
}

static void AssertStatement(CompilerContext& compiler)
{
  auto line = compiler.previous->GetLine();

  Consume(Scanner::TokenType::LeftParen, "Expected '(' after `assert`", compiler);
  auto prevUsing = compiler.usingExpressionResult;
  compiler.usingExpressionResult = true;
  Expression(false, compiler);
  compiler.usingExpressionResult = prevUsing;

  if (Match(Scanner::TokenType::Comma, compiler)) {
    Consume(Scanner::TokenType::String, "Expected message", compiler);
    EmitConstant(compiler.previous->GetString());
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
  auto constIdx = VM::VM::Instance().GetNumConstants();
  EmitConstant(std::int64_t {});
  auto opIdx = VM::VM::Instance().GetNumConstants();
  EmitConstant(std::int64_t {});
  EmitOp(VM::Ops::Jump, compiler.previous->GetLine());

  // we will patch in these indexes when the containing loop finishes
  compiler.breakIdxPairs.top().emplace_back(constIdx, opIdx);
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
  auto constIdx = VM::VM::Instance().GetNumConstants();
  EmitConstant(std::int64_t {});
  auto opIdx = VM::VM::Instance().GetNumConstants();
  EmitConstant(std::int64_t {});
  EmitOp(VM::Ops::Jump, compiler.previous->GetLine());

  // we will patch in these indexes when the containing loop finishes
  compiler.continueIdxPairs.top().emplace_back(constIdx, opIdx);
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
  std::make_pair(Scanner::TokenType::SetIdent, 7),
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
  auto iteratorName = compiler.previous->GetString();
  std::int64_t iteratorId;

  if (Match(Scanner::TokenType::Colon, compiler)) {
    if (!IsValidTypeAnnotation(compiler.current->GetType())) {
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
    if (CheckForDuplicateConstantName(iteratorName, compiler)) {
      MessageAtPrevious("A constant with the same name already exists", LogLevel::Error, compiler);
      return;
    }

    iteratorId = compiler.locals.size();
    compiler.locals.emplace_back(std::move(iteratorName), firstItIsFinal, true, iteratorId);
    EmitOp(VM::Ops::DeclareLocal, compiler.previous->GetLine());
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
        LogLevel::Warning,
        compiler);
      if (s_WarningsError) {
        return;
      }
    }
    iteratorId = it->index;
  }

  // parse second iterator variable, if it exists
  std::int64_t secondIteratorId {};
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
    auto secondIteratorName = compiler.previous->GetString();
    auto secondIt = std::find_if(
      compiler.locals.begin(),
      compiler.locals.end(),
      [&secondIteratorName](const Local& l) {
        return l.name == secondIteratorName;
      });

    if (Match(Scanner::TokenType::Colon, compiler)) {
      if (!IsValidTypeAnnotation(compiler.current->GetType())) {
        MessageAtCurrent("Expected typename after type annotation", LogLevel::Error, compiler);
        return;
      }
      Advance(compiler);
    }

    if (secondIt == compiler.locals.end()) {
      if (CheckForDuplicateConstantName(secondIteratorName, compiler)) {
        MessageAtPrevious("A constant with the same name already exists", LogLevel::Error, compiler);
        return;
      }

      secondIteratorId = compiler.locals.size();
      compiler.locals.emplace_back(std::move(secondIteratorName), secondItIsFinal, true, secondIteratorId);
      auto line = compiler.previous->GetLine();
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
          LogLevel::Warning,
          compiler);
        if (s_WarningsError) {
          return;
        }
      }
      secondIteratorId = secondIt->index;
    }
  }

  auto line = compiler.previous->GetLine();

  auto numLocalsStart = compiler.locals.size();

  Consume(Scanner::TokenType::In, "Expected `in` after identifier", compiler);

  auto prevUsing = compiler.usingExpressionResult;
  compiler.usingExpressionResult = true;
  Expression(false, compiler);
  compiler.usingExpressionResult = prevUsing;

  Consume(Scanner::TokenType::Colon, "Expected ':' after `for` statement", compiler);

  line = compiler.previous->GetLine();

  EmitConstant(twoIterators);
  EmitConstant(iteratorId);
  EmitConstant(secondIteratorId);
  EmitOp(VM::Ops::AssignIteratorBegin, line);

  // constant and op index to jump to after each iteration
  auto startConstantIdx = VM::VM::Instance().GetNumConstants();
  auto startOpIdx = VM::VM::Instance().GetNumOps();

  // evaluate the condition
  EmitOp(VM::Ops::CheckIteratorEnd, line);

  auto endJumpConstantIndex = VM::VM::Instance().GetNumConstants();
  EmitConstant(std::int64_t {});
  auto endJumpOpIndex = VM::VM::Instance().GetNumConstants();
  EmitConstant(std::int64_t {});
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
      VM::VM::Instance().SetConstantAtIndex(constantIdx, VM::VM::Instance().GetNumConstants());
      VM::VM::Instance().SetConstantAtIndex(opIdx, VM::VM::Instance().GetNumOps());
    }
    compiler.continueIdxPairs.pop();
    compiler.continueJumpNeedsIndexes = !compiler.continueIdxPairs.empty();
  }

  // pop any locals created within the loop scope
  if (compiler.locals.size() != numLocalsStart) {
    EmitConstant(numLocalsStart);
    EmitOp(VM::Ops::PopLocals, line);
  }

  // increment iterator
  EmitConstant(twoIterators);
  EmitConstant(iteratorId);
  EmitConstant(secondIteratorId);
  EmitOp(VM::Ops::IncrementIterator, line);

  // always jump back to re-evaluate the condition
  EmitConstant(startConstantIdx);
  EmitConstant(startOpIdx);
  EmitOp(VM::Ops::Jump, line);

  // set indexes for breaks and when the condition fails, the iterator variable will need to be popped (if it's a new variable)
  if (compiler.breakJumpNeedsIndexes) {
    for (auto& [constantIdx, opIdx] : compiler.breakIdxPairs.top()) {
      VM::VM::Instance().SetConstantAtIndex(constantIdx, VM::VM::Instance().GetNumConstants());
      VM::VM::Instance().SetConstantAtIndex(opIdx, VM::VM::Instance().GetNumOps());
    }
    compiler.breakIdxPairs.pop();
    compiler.breakJumpNeedsIndexes = !compiler.breakIdxPairs.empty();
  }

  VM::VM::Instance().SetConstantAtIndex(endJumpConstantIndex, VM::VM::Instance().GetNumConstants());
  VM::VM::Instance().SetConstantAtIndex(endJumpOpIndex, VM::VM::Instance().GetNumOps());

  if (compiler.locals.size() != numLocalsStart) {
    EmitConstant(numLocalsStart);
    EmitOp(VM::Ops::PopLocals, line);
  }

  // get rid of any variables made within the loop scope
  while (compiler.locals.size() != numLocalsStart) {
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
  auto topConstantIdxToJump = VM::VM::Instance().GetNumConstants();
  EmitConstant(std::int64_t {});
  auto topOpIdxToJump = VM::VM::Instance().GetNumConstants();
  EmitConstant(std::int64_t {});

  EmitOp(VM::Ops::JumpIfFalse, compiler.previous->GetLine());

  // constant index, op index
  std::vector<std::tuple<std::int64_t, std::int64_t>> endJumpIndexPairs;

  auto numLocalsStart = compiler.locals.size();

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

      while (compiler.locals.size() != numLocalsStart) {
        compiler.locals.pop_back();
      }

      // if the ifs condition passed and its block executed, it needs to jump to the end
      auto endConstantIdx = VM::VM::Instance().GetNumConstants();
      EmitConstant(std::int64_t {});
      auto endOpIdx = VM::VM::Instance().GetNumConstants();
      EmitConstant(std::int64_t {});
      EmitOp(VM::Ops::Jump, compiler.previous->GetLine());

      endJumpIndexPairs.emplace_back(endConstantIdx, endOpIdx);

      auto numConstants = VM::VM::Instance().GetNumConstants();
      auto numOps = VM::VM::Instance().GetNumOps();

      // haven't told the 'if' where to jump to yet if its condition fails
      if (!topJumpSet) {
        VM::VM::Instance().SetConstantAtIndex(topConstantIdxToJump, numConstants);
        VM::VM::Instance().SetConstantAtIndex(topOpIdxToJump, numOps);
        topJumpSet = true;
      }

      if (Match(Scanner::TokenType::Colon, compiler)) {
        elseBlockFound = true;
        if (Match(Scanner::TokenType::End, compiler)) {
          break; // Declaration() will fail if this block is empty
        }
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

  auto numConstants = VM::VM::Instance().GetNumConstants();
  auto numOps = VM::VM::Instance().GetNumOps();

  for (auto [constIdx, opIdx] : endJumpIndexPairs) {
    VM::VM::Instance().SetConstantAtIndex(constIdx, numConstants);
    VM::VM::Instance().SetConstantAtIndex(opIdx, numOps);
  }

  // if there was no else or elseif block, jump to here
  if (!topJumpSet) {
    VM::VM::Instance().SetConstantAtIndex(topConstantIdxToJump, numConstants);
    VM::VM::Instance().SetConstantAtIndex(topOpIdxToJump, numOps);
  }

  if (compiler.locals.size() != numLocalsStart) {
    auto line = compiler.previous->GetLine();
    EmitConstant(numLocalsStart);
    EmitOp(VM::Ops::PopLocals, line);

    while (compiler.locals.size() != numLocalsStart) {
      compiler.locals.pop_back();
    }
  }

  compiler.codeContextStack.pop_back();
}

static void PrintStatement(CompilerContext& compiler)
{
  Consume(Scanner::TokenType::LeftParen, "Expected '(' after 'print'", compiler);
  if (Match(Scanner::TokenType::RightParen, compiler)) {
    EmitOp(VM::Ops::PrintTab, compiler.current->GetLine());
  } else {
    auto prevUsing = compiler.usingExpressionResult;
    compiler.usingExpressionResult = true;
    Expression(false, compiler);
    compiler.usingExpressionResult = prevUsing;
    EmitOp(VM::Ops::Print, compiler.current->GetLine());
    Consume(Scanner::TokenType::RightParen, "Expected ')' after expression", compiler);
  }
  Consume(Scanner::TokenType::Semicolon, "Expected ';' after expression", compiler);
}

static void PrintLnStatement(CompilerContext& compiler)
{
  Consume(Scanner::TokenType::LeftParen, "Expected '(' after 'println'", compiler);
  if (Match(Scanner::TokenType::RightParen, compiler)) {
    EmitOp(VM::Ops::PrintEmptyLine, compiler.current->GetLine());
  } else {
    auto prevUsing = compiler.usingExpressionResult;
    compiler.usingExpressionResult = true;
    Expression(false, compiler);
    compiler.usingExpressionResult = prevUsing;
    EmitOp(VM::Ops::PrintLn, compiler.current->GetLine());
    Consume(Scanner::TokenType::RightParen, "Expected ')' after expression", compiler);
  }
  Consume(Scanner::TokenType::Semicolon, "Expected ';' after expression", compiler);
}

static void EPrintStatement(CompilerContext& compiler)
{
  Consume(Scanner::TokenType::LeftParen, "Expected '(' after 'eprint'", compiler);
  if (Match(Scanner::TokenType::RightParen, compiler)) {
    EmitOp(VM::Ops::EPrintTab, compiler.current->GetLine());
  } else {
    auto prevUsing = compiler.usingExpressionResult;
    compiler.usingExpressionResult = true;
    Expression(false, compiler);
    compiler.usingExpressionResult = prevUsing;
    EmitOp(VM::Ops::EPrint, compiler.current->GetLine());
    Consume(Scanner::TokenType::RightParen, "Expected ')' after expression", compiler);
  }
  Consume(Scanner::TokenType::Semicolon, "Expected ';' after expression", compiler);
}

static void EPrintLnStatement(CompilerContext& compiler)
{
  Consume(Scanner::TokenType::LeftParen, "Expected '(' after 'eprintln'", compiler);
  if (Match(Scanner::TokenType::RightParen, compiler)) {
    EmitOp(VM::Ops::EPrintEmptyLine, compiler.current->GetLine());
  } else {
    auto prevUsing = compiler.usingExpressionResult;
    compiler.usingExpressionResult = true;
    Expression(false, compiler);
    compiler.usingExpressionResult = prevUsing;
    EmitOp(VM::Ops::EPrintLn, compiler.current->GetLine());
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

  if (VM::VM::Instance().GetLastFunctionName() == "main") {
    MessageAtPrevious("Cannot return from main function", LogLevel::Error, compiler);
    return;
  }

  if (Match(Scanner::TokenType::Semicolon, compiler)) {
    auto line = compiler.previous->GetLine();
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
    EmitConstant(std::int64_t { 0 });
    EmitOp(VM::Ops::PopLocals, compiler.previous->GetLine());
  }

  EmitOp(VM::Ops::Return, compiler.previous->GetLine());
  Consume(Scanner::TokenType::Semicolon, "Expected ';' after expression", compiler);
}

static void TryStatement(CompilerContext& compiler)
{
  compiler.codeContextStack.emplace_back(CodeContext::Try);

  Consume(Scanner::TokenType::Colon, "Expected `:` after `try`", compiler);

  auto numLocalsStart = compiler.locals.size();

  auto catchOpJumpIdx = VM::VM::Instance().GetNumConstants();
  EmitConstant(std::int64_t {});
  auto catchConstJumpIdx = VM::VM::Instance().GetNumConstants();
  EmitConstant(std::int64_t {});
  EmitOp(VM::Ops::EnterTry, compiler.previous->GetLine());

  while (!Match(Scanner::TokenType::Catch, compiler)) {
    Declaration(compiler);
    if (Match(Scanner::TokenType::EndOfFile, compiler)) {
      MessageAtPrevious("Unterminated `try` block", LogLevel::Error, compiler);
      return;
    }
  }

  // if we made it this far without the exception, pop locals...
  EmitConstant(numLocalsStart);
  EmitOp(VM::Ops::ExitTry, compiler.previous->GetLine());

  // then jump to after the catch block
  auto skipCatchConstJumpIdx = VM::VM::Instance().GetNumConstants();
  EmitConstant(std::int64_t {});
  auto skipCatchOpJumpIdx = VM::VM::Instance().GetNumConstants();
  EmitConstant(std::int64_t {});
  EmitOp(VM::Ops::Jump, compiler.previous->GetLine());

  // but if there was an exception, jump here, and also pop the locals but continue into the catch block
  VM::VM::Instance().SetConstantAtIndex(catchOpJumpIdx, VM::VM::Instance().GetNumOps());
  VM::VM::Instance().SetConstantAtIndex(catchConstJumpIdx, VM::VM::Instance().GetNumConstants());

  EmitConstant(numLocalsStart);
  EmitOp(VM::Ops::ExitTry, compiler.previous->GetLine());


  if (!Match(Scanner::TokenType::Identifier, compiler)) {
    MessageAtCurrent("Expected identifier after `catch`", LogLevel::Error, compiler);
    return;
  }

  while (compiler.locals.size() != numLocalsStart) {
    compiler.locals.pop_back();
  }
  numLocalsStart = compiler.locals.size();

  auto exceptionVarName = compiler.previous->GetString();
  std::int64_t exceptionVarId;
  auto it = std::find_if(compiler.locals.begin(), compiler.locals.end(), [&exceptionVarName](const Local& l) { return l.name == exceptionVarName; });
  if (it == compiler.locals.end()) {
    if (CheckForDuplicateConstantName(exceptionVarName, compiler)) {
      MessageAtPrevious("A constant with the same name already exists", LogLevel::Error, compiler);
      return;
    }

    exceptionVarId = compiler.locals.size();
    compiler.locals.emplace_back(std::move(exceptionVarName), false, false, exceptionVarId);
    EmitOp(VM::Ops::DeclareLocal, compiler.previous->GetLine());
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
        LogLevel::Warning,
        compiler);
      if (s_WarningsError) {
        return;
      }
    }
  }

  EmitConstant(exceptionVarId);
  EmitOp(VM::Ops::AssignLocal, compiler.previous->GetLine());

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

  if (compiler.locals.size() != numLocalsStart) {
    EmitConstant(numLocalsStart);
    EmitOp(VM::Ops::PopLocals, compiler.previous->GetLine());

    while (compiler.locals.size() != numLocalsStart) {
      compiler.locals.pop_back();
    }
  }

  VM::VM::Instance().SetConstantAtIndex(skipCatchConstJumpIdx, VM::VM::Instance().GetNumConstants());
  VM::VM::Instance().SetConstantAtIndex(skipCatchOpJumpIdx, VM::VM::Instance().GetNumOps());

  compiler.codeContextStack.pop_back();
}

static void ThrowStatement(CompilerContext& compiler)
{
  Consume(Scanner::TokenType::LeftParen, "Expected '(' after `throw`", compiler);
  auto prevUsing = compiler.usingExpressionResult;
  compiler.usingExpressionResult = true;
  Expression(false, compiler);
  compiler.usingExpressionResult = prevUsing;
  EmitOp(VM::Ops::Throw, compiler.previous->GetLine());
  Consume(Scanner::TokenType::RightParen, "Expected ')' after `throw` message", compiler);
  Consume(Scanner::TokenType::Semicolon, "Expected ';' after `throw` statement", compiler);
}

static void WhileStatement(CompilerContext& compiler)
{
  compiler.codeContextStack.emplace_back(CodeContext::WhileLoop);

  compiler.breakIdxPairs.emplace();
  compiler.continueIdxPairs.emplace();

  auto constantIdx = VM::VM::Instance().GetNumConstants();
  auto opIdx = VM::VM::Instance().GetNumOps();

  auto prevUsing = compiler.usingExpressionResult;
  compiler.usingExpressionResult = true;
  Expression(false, compiler);
  compiler.usingExpressionResult = prevUsing;

  auto line = compiler.previous->GetLine();

  // evaluate the condition
  auto endConstantJumpIdx = VM::VM::Instance().GetNumConstants();
  EmitConstant(std::int64_t {});
  auto endOpJumpIdx = VM::VM::Instance().GetNumConstants();
  EmitConstant(std::int64_t {});
  EmitOp(VM::Ops::JumpIfFalse, compiler.previous->GetLine());

  Consume(Scanner::TokenType::Colon, "Expected ':' after expression", compiler);

  auto numLocalsStart = compiler.locals.size();

  while (!Match(Scanner::TokenType::End, compiler)) {
    Declaration(compiler);
    if (Match(Scanner::TokenType::EndOfFile, compiler)) {
      MessageAtPrevious("Unterminated `while` loop", LogLevel::Error, compiler);
      return;
    }
  }

  auto numConstants = VM::VM::Instance().GetNumConstants();
  auto numOps = VM::VM::Instance().GetNumOps();

  if (compiler.continueJumpNeedsIndexes) {
    for (auto& [c, o] : compiler.continueIdxPairs.top()) {
      VM::VM::Instance().SetConstantAtIndex(c, numConstants);
      VM::VM::Instance().SetConstantAtIndex(o, numOps);
    }
    compiler.continueIdxPairs.pop();
    compiler.continueJumpNeedsIndexes = !compiler.continueIdxPairs.empty();
  }

  if (compiler.locals.size() != numLocalsStart) {
    EmitConstant(numLocalsStart);
    EmitOp(VM::Ops::PopLocals, line);
  }

  // jump back up to the expression so it can be re-evaluated
  EmitConstant(constantIdx);
  EmitConstant(opIdx);
  EmitOp(VM::Ops::Jump, line);

  numConstants = VM::VM::Instance().GetNumConstants();
  numOps = VM::VM::Instance().GetNumOps();

  if (compiler.breakJumpNeedsIndexes) {
    for (auto& [c, o] : compiler.breakIdxPairs.top()) {
      VM::VM::Instance().SetConstantAtIndex(c, numConstants);
      VM::VM::Instance().SetConstantAtIndex(o, numOps);
    }
    compiler.breakIdxPairs.pop();
    compiler.breakJumpNeedsIndexes = !compiler.breakIdxPairs.empty();

    // if we broke, we missed the PopLocals instruction before the end of the loop
    // so tell the VM to still pop those locals IF we broke
    if (compiler.locals.size() != numLocalsStart) {
      EmitConstant(numLocalsStart);
      EmitOp(VM::Ops::PopLocals, line);
    }
  }

  while (compiler.locals.size() != numLocalsStart) {
    compiler.locals.pop_back();
  }

  numConstants = VM::VM::Instance().GetNumConstants();
  numOps = VM::VM::Instance().GetNumOps();

  VM::VM::Instance().SetConstantAtIndex(endConstantJumpIdx, numConstants);
  VM::VM::Instance().SetConstantAtIndex(endOpJumpIdx, numOps);

  compiler.codeContextStack.pop_back();
}

static void Expression(bool canAssign, CompilerContext& compiler)
{
  // can't start an expression with an operator...
  if (IsOperator(compiler.current->GetType())) {
    MessageAtCurrent("Expected identifier or literal at start of expression", LogLevel::Error, compiler);
    Advance(compiler);
    return;
  }

  // ... or a keyword
  std::string kw;
  if (IsKeyword(compiler.current->GetType(), kw)) {
    MessageAtCurrent(fmt::format("'{}' is a keyword and not valid in this context", kw), LogLevel::Error, compiler);
    Advance(compiler); //consume the illegal identifier
    return;
  }

  // only identifiers or literals
  if (Check(Scanner::TokenType::Identifier, compiler)) {
    Call(canAssign, compiler);

    if (Check(Scanner::TokenType::Equal, compiler) || IsCompoundAssignment(compiler.current->GetType())) {
      if (compiler.previous->GetType() != Scanner::TokenType::Identifier) {
        MessageAtCurrent("Only identifiers can be assigned to", LogLevel::Error, compiler);
        return;
      }

      auto localName = compiler.previous->GetString();
      auto it = std::find_if(compiler.locals.begin(), compiler.locals.end(), [&localName](const Local& l) { return l.name == localName; });
      if (it == compiler.locals.end()) {
        auto mostSimilarVar = FindMostSimilarVarName(localName, compiler.locals);
        if (mostSimilarVar) {
          MessageAtPrevious(fmt::format("Cannot find variable '{}' in this scope, did you mean '{}'?", localName, *mostSimilarVar), LogLevel::Error, compiler);
        } else {
          MessageAtPrevious(fmt::format("Cannot find variable '{}' in this scope", localName), LogLevel::Error, compiler);
        }
        return;
      }

      if (it->isFinal) {
        MessageAtPrevious(fmt::format("Cannot reassign to final '{}'", compiler.previous->GetText()), LogLevel::Error, compiler);
        return;
      }

      if (it->isIterator && (s_Verbose || s_WarningsError)) {
        MessageAtPrevious(fmt::format("'{}' is an iterator variable and will be reassigned on each loop iteration", compiler.previous->GetText()), LogLevel::Warning, compiler);
        if (s_WarningsError) {
          return;
        }
      }

      if (!canAssign) {
        MessageAtCurrent("Assignment is not valid in the current context", LogLevel::Error, compiler);
        return;
      }

      Advance(compiler); // consume the operator
      auto opToken = compiler.previous->GetType();

      auto prevUsing = compiler.usingExpressionResult;
      compiler.usingExpressionResult = true;
      Expression(false, compiler); // disallow x = y = z...
      compiler.usingExpressionResult = prevUsing;

      EmitConstant(it->index);

      switch (opToken) {
        case Scanner::TokenType::Equal:
          EmitOp(VM::Ops::AssignLocal, compiler.previous->GetLine());
          break;
        case Scanner::TokenType::PlusEquals:
          EmitOp(VM::Ops::AddAssign, compiler.previous->GetLine());
          break;
        case Scanner::TokenType::MinusEquals:
          EmitOp(VM::Ops::SubtractAssign, compiler.previous->GetLine());
          break;
        case Scanner::TokenType::StarEquals:
          EmitOp(VM::Ops::MultiplyAssign, compiler.previous->GetLine());
          break;
        case Scanner::TokenType::SlashEquals:
          EmitOp(VM::Ops::DivideAssign, compiler.previous->GetLine());
          break;
        case Scanner::TokenType::AmpersandEquals:
          EmitOp(VM::Ops::BitwiseAndAssign, compiler.previous->GetLine());
          break;
        case Scanner::TokenType::CaretEquals:
          EmitOp(VM::Ops::BitwiseXOrAssign, compiler.previous->GetLine());
          break;
        case Scanner::TokenType::BarEquals:
          EmitOp(VM::Ops::BitwiseOrAssign, compiler.previous->GetLine());
          break;
        case Scanner::TokenType::ModEquals:
          EmitOp(VM::Ops::ModAssign, compiler.previous->GetLine());
          break;
        case Scanner::TokenType::ShiftLeftEquals:
          EmitOp(VM::Ops::ShiftLeftAssign, compiler.previous->GetLine());
          break;
        case Scanner::TokenType::ShiftRightEquals:
          EmitOp(VM::Ops::ShiftRightAssign, compiler.previous->GetLine());
          break;
        case Scanner::TokenType::StarStarEquals:
          EmitOp(VM::Ops::PowAssign, compiler.previous->GetLine());
          break;
        default:
          GRACE_UNREACHABLE();
          break;
      }
    } else {
      // not an assignment, so just keep parsing this expression
      bool shouldBreak = false;
      while (!shouldBreak) {
        switch (compiler.current->GetType()) {
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
          case Scanner::TokenType::RightSquareParen:
          case Scanner::TokenType::LeftCurlyParen:
          case Scanner::TokenType::RightCurlyParen:
          case Scanner::TokenType::DotDot:
          case Scanner::TokenType::By:
            shouldBreak = true;
            break;
          case Scanner::TokenType::Dot:
            Advance(compiler); // consume the .
            Dot(canAssign, compiler);
            break;
          case Scanner::TokenType::LeftSquareParen:
            Advance(compiler); // consume the [
            Subscript(canAssign, compiler);
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

static void Or(bool canAssign, bool skipFirst, CompilerContext& compiler)
{
  if (!skipFirst) {
    And(canAssign, false, compiler);
  }
  while (Match(Scanner::TokenType::Or, compiler)) {
    And(canAssign, false, compiler);
    EmitOp(VM::Ops::Or, compiler.current->GetLine());
  }
}

static void And(bool canAssign, bool skipFirst, CompilerContext& compiler)
{
  if (!skipFirst) {
    BitwiseOr(canAssign, false, compiler);
  }
  while (Match(Scanner::TokenType::And, compiler)) {
    BitwiseOr(canAssign, false, compiler);
    EmitOp(VM::Ops::And, compiler.current->GetLine());
  }
}

static void BitwiseOr(bool canAssign, bool skipFirst, CompilerContext& compiler)
{
  if (!skipFirst) {
    BitwiseXOr(canAssign, false, compiler);
  }
  while (Match(Scanner::TokenType::Bar, compiler)) {
    BitwiseXOr(canAssign, false, compiler);
    EmitOp(VM::Ops::BitwiseOr, compiler.current->GetLine());
  }
}

static void BitwiseXOr(bool canAssign, bool skipFirst, CompilerContext& compiler)
{
  if (!skipFirst) {
    BitwiseAnd(canAssign, false, compiler);
  }
  while (Match(Scanner::TokenType::Caret, compiler)) {
    BitwiseAnd(canAssign, false, compiler);
    EmitOp(VM::Ops::BitwiseXOr, compiler.current->GetLine());
  }
}

static void BitwiseAnd(bool canAssign, bool skipFirst, CompilerContext& compiler)
{
  if (!skipFirst) {
    Equality(canAssign, false, compiler);
  }
  while (Match(Scanner::TokenType::Ampersand, compiler)) {
    Equality(canAssign, false, compiler);
    EmitOp(VM::Ops::BitwiseAnd, compiler.current->GetLine());
  }
}

static void Equality(bool canAssign, bool skipFirst, CompilerContext& compiler)
{
  if (!skipFirst) {
    Comparison(canAssign, false, compiler);
  }
  if (Match(Scanner::TokenType::EqualEqual, compiler)) {
    Comparison(canAssign, false, compiler);
    EmitOp(VM::Ops::Equal, compiler.current->GetLine());
  } else if (Match(Scanner::TokenType::BangEqual, compiler)) {
    Comparison(canAssign, false, compiler);
    EmitOp(VM::Ops::NotEqual, compiler.current->GetLine());
  }
}

static void Comparison(bool canAssign, bool skipFirst, CompilerContext& compiler)
{
  if (!skipFirst) {
    Shift(canAssign, false, compiler);
  }
  if (Match(Scanner::TokenType::GreaterThan, compiler)) {
    Shift(canAssign, false, compiler);
    EmitOp(VM::Ops::Greater, compiler.current->GetLine());
  } else if (Match(Scanner::TokenType::GreaterEqual, compiler)) {
    Shift(canAssign, false, compiler);
    EmitOp(VM::Ops::GreaterEqual, compiler.current->GetLine());
  } else if (Match(Scanner::TokenType::LessThan, compiler)) {
    Shift(canAssign, false, compiler);
    EmitOp(VM::Ops::Less, compiler.current->GetLine());
  } else if (Match(Scanner::TokenType::LessEqual, compiler)) {
    Shift(canAssign, false, compiler);
    EmitOp(VM::Ops::LessEqual, compiler.current->GetLine());
  }
}

static void Shift(bool canAssign, bool skipFirst, CompilerContext& compiler)
{
  if (!skipFirst) {
    Term(canAssign, false, compiler);
  }
  if (Match(Scanner::TokenType::ShiftRight, compiler)) {
    Term(canAssign, false, compiler);
    EmitOp(VM::Ops::ShiftRight, compiler.current->GetLine());
  } else if (Match(Scanner::TokenType::ShiftLeft, compiler)) {
    Term(canAssign, false, compiler);
    EmitOp(VM::Ops::ShiftLeft, compiler.current->GetLine());
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
      EmitOp(VM::Ops::Subtract, compiler.current->GetLine());
    } else if (Match(Scanner::TokenType::Plus, compiler)) {
      Factor(canAssign, false, compiler);
      EmitOp(VM::Ops::Add, compiler.current->GetLine());
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
      EmitOp(VM::Ops::Pow, compiler.current->GetLine());
    } else if (Match(Scanner::TokenType::Star, compiler)) {
      Unary(canAssign, compiler);
      EmitOp(VM::Ops::Multiply, compiler.current->GetLine());
    } else if (Match(Scanner::TokenType::Slash, compiler)) {
      Unary(canAssign, compiler);
      EmitOp(VM::Ops::Divide, compiler.current->GetLine());
    } else if (Match(Scanner::TokenType::Mod, compiler)) {
      Unary(canAssign, compiler);
      EmitOp(VM::Ops::Mod, compiler.current->GetLine());
    } else {
      break;
    }
  }
}

static void Unary(bool canAssign, CompilerContext& compiler)
{
  if (Match(Scanner::TokenType::Bang, compiler)) {
    auto line = compiler.previous->GetLine();
    Unary(canAssign, compiler);
    EmitOp(VM::Ops::Not, line);
  } else if (Match(Scanner::TokenType::Minus, compiler)) {
    auto line = compiler.previous->GetLine();
    Unary(canAssign, compiler);
    EmitOp(VM::Ops::Negate, line);
  } else if (Match(Scanner::TokenType::Tilde, compiler)) {
    auto line = compiler.previous->GetLine();
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
    EmitOp(VM::Ops::LoadConstant, compiler.previous->GetLine());
    EmitConstant(true);
  } else if (Match(Scanner::TokenType::False, compiler)) {
    EmitOp(VM::Ops::LoadConstant, compiler.previous->GetLine());
    EmitConstant(false);
  } else if (Match(Scanner::TokenType::This, compiler)) {
    // TODO: this
  } else if (Match(Scanner::TokenType::Integer, compiler)) {
    std::int64_t value;
    auto result = TryParseInt(*compiler.previous, value);
    if (result) {
      MessageAtPrevious(fmt::format("Token could not be parsed as an int: {}", *result), LogLevel::Error, compiler);
      return;
    }
    EmitOp(VM::Ops::LoadConstant, compiler.previous->GetLine());
    EmitConstant(value);
  } else if (Match(Scanner::TokenType::HexLiteral, compiler)) {
    std::int64_t value;
    auto result = TryParseInt(*compiler.previous, value, 16, 2);
    if (result) {
      MessageAtPrevious(fmt::format("Token could not be parsed as a hex literal int: {}", *result), LogLevel::Error, compiler);
      return;
    }
    EmitOp(VM::Ops::LoadConstant, compiler.previous->GetLine());
    EmitConstant(value);
  } else if (Match(Scanner::TokenType::BinaryLiteral, compiler)) {
    std::int64_t value;
    auto result = TryParseInt(*compiler.previous, value, 2, 2);
    if (result) {
      MessageAtPrevious(fmt::format("Token could not be parsed as a binary literal int: {}", *result), LogLevel::Error, compiler);
      return;
    }
    EmitOp(VM::Ops::LoadConstant, compiler.previous->GetLine());
    EmitConstant(value);
  } else if (Match(Scanner::TokenType::Double, compiler)) {
    double value;
    auto result = TryParseDouble(*compiler.previous, value);
    if (result) {
      MessageAtPrevious(fmt::format("Token could not be parsed as an float: {}", *result), LogLevel::Error, compiler);
      return;
    }
    EmitOp(VM::Ops::LoadConstant, compiler.previous->GetLine());
    EmitConstant(value);
  } else if (Match(Scanner::TokenType::String, compiler)) {
    String(compiler);
  } else if (Match(Scanner::TokenType::Char, compiler)) {
    Char(compiler);
  } else if (Match(Scanner::TokenType::Identifier, compiler)) {
    Identifier(canAssign, compiler);
  } else if (Match(Scanner::TokenType::Null, compiler)) {
    EmitConstant(nullptr);
    EmitOp(VM::Ops::LoadConstant, compiler.previous->GetLine());
  } else if (Match(Scanner::TokenType::LeftParen, compiler)) {
    Expression(canAssign, compiler);
    Consume(Scanner::TokenType::RightParen, "Expected ')'", compiler);
  } else if (Match(Scanner::TokenType::InstanceOf, compiler)) {
    InstanceOf(compiler);
  } else if (Match(Scanner::TokenType::IsObject, compiler)) {
    IsObject(compiler);
  } else if (IsTypeIdent(compiler.current->GetType())) {
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

  while (true) {
    if (Match(Scanner::TokenType::Dot, compiler)) {
      Dot(canAssign, compiler);
    } else if (Match(Scanner::TokenType::LeftSquareParen, compiler)) {
      Subscript(canAssign, compiler);
    } else {
      break;
    }
  }
}

static void Subscript(bool canAssign, CompilerContext& compiler)
{
  auto prevUsing = compiler.usingExpressionResult;
  compiler.usingExpressionResult = true;
  Expression(false, compiler);
  compiler.usingExpressionResult = prevUsing;

  if (!Match(Scanner::TokenType::RightSquareParen, compiler)) {
    MessageAtCurrent("Expected ']' after subscript expression", LogLevel::Error, compiler);
    return;
  }

  if (Match(Scanner::TokenType::Equal, compiler)) {
    if (!canAssign) {
      MessageAtPrevious("Assignment is not valid in the current context", LogLevel::Error, compiler);
      return;
    }

    prevUsing = compiler.usingExpressionResult;
    compiler.usingExpressionResult = true;
    Expression(false, compiler);
    compiler.usingExpressionResult = prevUsing;

    EmitOp(VM::Ops::AssignSubscript, compiler.previous->GetLine());
  } else {
    EmitOp(VM::Ops::GetSubscript, compiler.previous->GetLine());
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
  } else if (Match(Scanner::TokenType::Equal, compiler)) {
    if (!canAssign) {
      MessageAtPrevious("Assignment is not valid here", LogLevel::Error, compiler);
      return;
    }

    auto prevUsing = compiler.usingExpressionResult;
    compiler.usingExpressionResult = true;
    Expression(false, compiler);
    compiler.usingExpressionResult = prevUsing;
    EmitConstant(memberNameToken.GetString());
    EmitOp(VM::Ops::AssignMember, compiler.previous.value().GetLine());
  } else {
    EmitConstant(memberNameToken.GetString());
    EmitOp(VM::Ops::LoadMember, memberNameToken.GetLine());
  }
}

static bool ParseCallParameters(CompilerContext& compiler, int64_t& numArgs)
{
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
      if (!Match(Scanner::TokenType::Comma, compiler)) {
        MessageAtCurrent("Expected ',' after function call argument", LogLevel::Error, compiler);
        return false;
      }
    }
  }

  return true;
}

static void DotFunctionCall(const Scanner::Token& funcNameToken, CompilerContext& compiler)
{
  // the last object on the stack is the object we are calling
  std::int64_t numArgs = 0;
  if (!ParseCallParameters(compiler, numArgs)) {
    return;
  }

  static std::hash<std::string> hasher;
  auto funcName = funcNameToken.GetString();
  EmitConstant(funcName);
  EmitConstant(hasher(funcName));
  EmitConstant(numArgs);
  EmitOp(VM::Ops::MemberCall, funcNameToken.GetLine());

  if (Check(Scanner::TokenType::Semicolon, compiler) && !compiler.usingExpressionResult) {
    // pop unused return value
    EmitOp(VM::Ops::Pop, compiler.previous->GetLine());
  }
}

static void FreeFunctionCall(const Scanner::Token& funcNameToken, CompilerContext& compiler)
{
  compiler.namespaceQualifierUsed = true;

  static std::hash<std::string> hasher;
  auto funcNameText = funcNameToken.GetString();
  auto hash = hasher(funcNameText);
  auto nativeCall = funcNameText.starts_with("__");
  std::size_t nativeIndex {};
  if (nativeCall) {
    auto [exists, index] = VM::VM::Instance().HasNativeFunction(funcNameText);
    if (!exists) {
      Message(funcNameToken, fmt::format("No native function matching the given signature `{}` was found", funcNameText), LogLevel::Error, compiler);
      return;
    } else {
      nativeIndex = index;
    }
  }

  std::int64_t numArgs = 0;
  if (!ParseCallParameters(compiler, numArgs)) {
    return;
  }

  if (nativeCall) {
    auto arity = VM::VM::Instance().GetNativeFunction(nativeIndex).GetArity();
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

  EmitConstant(nativeCall ? nativeIndex : hash);
  EmitConstant(numArgs);
  if (nativeCall) {
    EmitOp(VM::Ops::NativeCall, compiler.previous->GetLine());
  } else {
    EmitConstant(std::move(funcNameText));
    EmitOp(VM::Ops::Call, compiler.previous->GetLine());
  }

  if (Check(Scanner::TokenType::Semicolon, compiler) && !compiler.usingExpressionResult) {
    // pop unused return value
    EmitOp(VM::Ops::Pop, compiler.previous->GetLine());
  }
}

static void Identifier(bool canAssign, CompilerContext& compiler)
{
  auto prev = *compiler.previous;
  auto prevText = compiler.previous->GetString();

  if (Match(Scanner::TokenType::LeftParen, compiler)) {
    FreeFunctionCall(prev, compiler);
  } else if (Match(Scanner::TokenType::ColonColon, compiler)) {
    if (!Check(Scanner::TokenType::Identifier, compiler)) {
      MessageAtCurrent("Expected identifier after `::`", LogLevel::Error, compiler);
      return;
    }

    if (IsLiteral(compiler.current->GetType())) {
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
    EmitConstant(hasher(prevText));
    EmitOp(VM::Ops::AppendNamespace, prev.GetLine());

    compiler.currentNamespaceLookup += prevText;

    Expression(canAssign, compiler);
  } else {
    // not a call or member access, so we are just trying to call on the value of the local
    // or reassign it
    // if it's not a reassignment, we are trying to load its value
    // Primary() has already but the variable's id on the stack
    if (!Check(Scanner::TokenType::Equal, compiler) && !IsCompoundAssignment(compiler.current->GetType())) {
      auto localIt = std::find_if(compiler.locals.begin(), compiler.locals.end(), [&prevText](const Local& l) { return l.name == prevText; });
      if (localIt == compiler.locals.end()) {
        // now check for constants...
        auto constantIt = s_FileConstantsLookup[compiler.fullPath.string()].find(prevText);

        if (constantIt == s_FileConstantsLookup[compiler.fullPath.string()].end()) {
          // might be in a namespace...
          auto importPath = (compiler.parentPath / (compiler.currentNamespaceLookup + ".gr")).string();
          auto importedConstantIt = s_FileConstantsLookup[importPath].find(prevText);

          if (importedConstantIt == s_FileConstantsLookup[importPath].end()) {
            auto mostSimilarVar = FindMostSimilarVarName(prevText, compiler.locals);
            if (mostSimilarVar) {
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

static void Char(CompilerContext& compiler)
{
  char res;
  auto err = TryParseChar(*compiler.previous, res);
  if (err) {
    MessageAtPrevious(fmt::format("Token could not be parsed as char: {}", *err), LogLevel::Error, compiler);
    return;
  }
  EmitOp(VM::Ops::LoadConstant, compiler.previous->GetLine());
  EmitConstant(res);
}

static void String(CompilerContext& compiler)
{
  std::string res;
  auto err = TryParseString(*compiler.previous, res);
  if (err) {
    MessageAtPrevious(fmt::format("Token could not be parsed as string: {}", *err), LogLevel::Error, compiler);
    return;
  }
  EmitOp(VM::Ops::LoadConstant, compiler.previous->GetLine());
  EmitConstant(std::move(res));
}

static void InstanceOf(CompilerContext& compiler)
{
  Consume(Scanner::TokenType::LeftParen, "Expected '(' after 'instanceof'", compiler);

  auto prevUsing = compiler.usingExpressionResult;
  compiler.usingExpressionResult = true;
  Expression(false, compiler);
  compiler.usingExpressionResult = prevUsing;

  Consume(Scanner::TokenType::Comma, "Expected ',' after expression", compiler);

  switch (compiler.current->GetType()) {
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
    case Scanner::TokenType::SetIdent:
      EmitConstant(std::int64_t(10));
      break;
    case Scanner::TokenType::Identifier:
      EmitConstant(std::int64_t(11));
      EmitConstant(compiler.current.value().GetString());
      break;
    default:
      MessageAtCurrent("Expected type as second argument for `instanceof`", LogLevel::Error, compiler);
      return;
  }

  EmitOp(VM::Ops::CheckType, compiler.current->GetLine());

  Advance(compiler); // Consume the type ident
  Consume(Scanner::TokenType::RightParen, "Expected ')'", compiler);

  if (Check(Scanner::TokenType::Semicolon, compiler) && !compiler.usingExpressionResult) {
    // pop unused return value
    EmitOp(VM::Ops::Pop, compiler.previous->GetLine());
  }
}

static void IsObject(CompilerContext& compiler)
{
  Consume(Scanner::TokenType::LeftParen, "Expected '(' after `isobject`", compiler);

  auto prevUsing = compiler.usingExpressionResult;
  compiler.usingExpressionResult = true;
  Expression(false, compiler);
  compiler.usingExpressionResult = prevUsing;

  EmitOp(VM::Ops::IsObject, compiler.previous->GetLine());
  Consume(Scanner::TokenType::RightParen, "Expected ')' after expression", compiler);

  if (Check(Scanner::TokenType::Semicolon, compiler) && !compiler.usingExpressionResult) {
    EmitOp(VM::Ops::Pop, compiler.previous->GetLine());
  }
}

static void Cast(CompilerContext& compiler)
{
  auto typeToken = *compiler.current;
  Advance(compiler);
  Consume(Scanner::TokenType::LeftParen, "Expected '(' after type ident", compiler);

  bool isList = false, isSet = false;
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
      EmitOp(VM::Ops::Cast, compiler.current->GetLine());
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
      EmitOp(VM::Ops::Cast, compiler.current->GetLine());
      break;
    }
    case Scanner::TokenType::SetIdent: {
      isSet = true;

      auto prevUsing = compiler.usingExpressionResult;
      compiler.usingExpressionResult = true;
      while (true) {
        if (Check(Scanner::TokenType::RightParen, compiler)) {
          break;
        }

        if (Match(Scanner::TokenType::EndOfFile, compiler)) {
          MessageAtPrevious("Unterminated `Set` constructor", LogLevel::Error, compiler);
          return;
        }

        Expression(false, compiler);
        numListItems++;

        if (Check(Scanner::TokenType::RightParen, compiler)) {
          break;
        }

        if (!Match(Scanner::TokenType::Comma, compiler)) {
          MessageAtPrevious("Expected ',' between `Set` items", LogLevel::Error, compiler);
          return;
        }
      }
      compiler.usingExpressionResult = prevUsing;
      break;
      break;
    }
    default:
      GRACE_UNREACHABLE();
      break;
  }

  Consume(Scanner::TokenType::RightParen, "Expected ')' after expression", compiler);

  if (isList) {
    EmitConstant(numListItems);
    EmitOp(VM::Ops::CreateListFromCast, compiler.previous.value().GetLine());
  } else if (isSet) {
    EmitConstant(numListItems);
    EmitOp(VM::Ops::CreateSet, compiler.previous.value().GetLine());
  }

  if (Check(Scanner::TokenType::Semicolon, compiler) && !compiler.usingExpressionResult) {
    // pop unused return value
    EmitOp(VM::Ops::Pop, compiler.previous->GetLine());
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

    if (Match(Scanner::TokenType::DotDot, compiler)) {
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
        EmitConstant(std::int64_t { 1 });
        EmitOp(VM::Ops::LoadConstant, compiler.previous->GetLine());
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

  auto line = compiler.previous->GetLine();

  if (parsedRangeExpression) {
    EmitOp(VM::Ops::CreateRange, line);
  } else {
    EmitConstant(numItems);
    EmitOp(VM::Ops::CreateList, line);
  }
}

static void Dictionary(CompilerContext& compiler)
{
  std::int64_t numItems = 0;

  auto prevUsing = compiler.usingExpressionResult;
  compiler.usingExpressionResult = true;

  while (true) {
    if (Match(Scanner::TokenType::RightCurlyParen, compiler)) {
      break;
    }

    Expression(false, compiler);

    if (!Match(Scanner::TokenType::Colon, compiler)) {
      MessageAtCurrent("Expected ':' after key expression", LogLevel::Error, compiler);
      return;
    }

    Expression(false, compiler);

    numItems++;

    if (Match(Scanner::TokenType::RightCurlyParen, compiler)) {
      break;
    }

    Consume(Scanner::TokenType::Comma, "Expected `,` between dictionary pairs", compiler);
  }

  compiler.usingExpressionResult = prevUsing;

  EmitConstant(numItems);
  EmitOp(VM::Ops::CreateDictionary, compiler.previous->GetLine());
}

static void Typename(CompilerContext& compiler)
{
  Consume(Scanner::TokenType::LeftParen, "Expected '('", compiler);
  auto prevUsing = compiler.usingExpressionResult;
  compiler.usingExpressionResult = true;
  Expression(false, compiler);
  compiler.usingExpressionResult = prevUsing;
  EmitOp(VM::Ops::Typename, compiler.previous->GetLine());
  Consume(Scanner::TokenType::RightParen, "Expected ')'", compiler);
}

static void MessageAtCurrent(const std::string& message, LogLevel level, CompilerContext& compiler)
{
  Message(*compiler.current, message, level, compiler);
}

static void MessageAtPrevious(const std::string& message, LogLevel level, CompilerContext& compiler)
{
  Message(*compiler.previous, message, level, compiler);
}

static void Message(const Scanner::Token& token, const std::string& message, LogLevel level, CompilerContext& compiler)
{
  if (level == LogLevel::Error || s_WarningsError) {
    if (compiler.panicMode)
      return;
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
  auto column = token.GetColumn() - token.GetLength(); // need the START of the token
  auto filePath = compiler.fullPath.string();
  fmt::print(stderr, "       --> {}:{}:{}\n", filePath, lineNo, column + 1);
  fmt::print(stderr, "        |\n");

  if (lineNo > 1) {
    fmt::print(stderr, "{:>7} | {}\n", lineNo - 1, Scanner::GetCodeAtLine(filePath, lineNo - 1));
  }

  fmt::print(stderr, "{:>7} | {}\n", lineNo, Scanner::GetCodeAtLine(filePath, lineNo));
  fmt::print(stderr, "        | ");
  for (std::size_t i = 0; i < column; i++) {
    fmt::print(stderr, " ");
  }
  for (std::size_t i = 0; i < token.GetLength(); i++) {
    fmt::print(stderr, colour, "^");
  }

  fmt::print(stderr, "\n");
  fmt::print(stderr, "{:>7} | {}\n", lineNo + 1, Scanner::GetCodeAtLine(filePath, lineNo + 1));
  fmt::print(stderr, "        |\n\n");

  if (level == LogLevel::Error) {
    compiler.hadError = true;
  }
  if (level == LogLevel::Warning) {
    compiler.hadWarning = true;
  }
}
