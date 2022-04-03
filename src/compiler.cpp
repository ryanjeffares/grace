#include <charconv>
#include <chrono>

#include "compiler.hpp"

using namespace Grace::Scanner;
using namespace Grace::VM;

void Grace::Compiler::Compile(std::string&& fileName, std::string&& code, bool verbose)
{
  using namespace std::chrono;

  auto start = steady_clock::now();
 
  Compiler compiler(std::move(fileName), std::move(code), verbose);
  compiler.Advance();
  
  while (!compiler.Match(TokenType::EndOfFile)) {
    compiler.Declaration();
    if (compiler.HadError()) {
      break;
    }
  }

  if (compiler.HadError()) {
    fmt::print(stderr, "Terminating process due to compilation errors.\n");
  } else {
    if (verbose) {
      auto end = steady_clock::now();
      auto duration = duration_cast<microseconds>(end - start).count();
      fmt::print("Compilation succeeded in {} μs.\n", duration);
    }
    compiler.Finalise(verbose);
  }
}

using namespace Grace::Compiler;

Compiler::Compiler(std::string&& fileName, std::string&& code, bool verbose) 
  : m_Scanner(std::move(code)), 
  m_CurrentFileName(std::move(fileName)),
  m_CurrentContext(Context::TopLevel),
  m_Vm(*this),
  m_Verbose(verbose)
{

}

void Compiler::Finalise(bool verbose)
{
#ifdef GRACE_DEBUG
  if (verbose) {
    m_Vm.PrintOps();
  }
#endif
   m_Vm.Start(verbose);
}

void Compiler::Advance()
{
  m_Previous = m_Current;
  m_Current = m_Scanner.ScanToken();

#ifdef GRACE_DEBUG
  if (m_Verbose) {
    fmt::print("{}\n", m_Current.value().ToString());
  }
#endif 

  if (m_Current.value().GetType() == TokenType::Error) {
    ErrorAtCurrent("Unexpected token");
  }
}

bool Compiler::Match(TokenType type)
{
  if (!Check(type)) {
    return false;
  }

  Advance();
  return true;
}

bool Compiler::Check(TokenType type) const
{
  return m_Current.has_value() && m_Current.value().GetType() == type;
}

void Compiler::Consume(TokenType expected, const std::string& message)
{
  if (m_Current.value().GetType() == expected) {
    Advance();
    return;
  }

  ErrorAtCurrent(message);
}

void Compiler::Synchronize()
{
  m_PanicMode = false;

  while (m_Current.value().GetType() != TokenType::EndOfFile) {
    if (m_Previous.has_value() && m_Previous.value().GetType() == TokenType::Semicolon) {
      return;
    }

    switch (m_Current.value().GetType()) {
      case TokenType::Class:
      case TokenType::Func:
      case TokenType::Final:
      case TokenType::For:
      case TokenType::If:
      case TokenType::While:
      case TokenType::Print:
      case TokenType::PrintLn:
      case TokenType::Return:
      case TokenType::Var:
        return;
      default:
        break;
    }

    Advance();
  }
}

static bool IsKeyword(TokenType type, std::string& outKeyword)
{
  switch (type) {
    case TokenType::And: outKeyword = "and"; return true;
    case TokenType::As: outKeyword = "as"; return true;
    case TokenType::Class: outKeyword = "class"; return true;
    case TokenType::End: outKeyword = "end"; return true;
    case TokenType::Final: outKeyword = "final"; return true;
    case TokenType::For: outKeyword = "for"; return true;
    case TokenType::Func: outKeyword = "func"; return true;
    case TokenType::If: outKeyword = "if"; return true;
    case TokenType::Or: outKeyword = "or"; return true;
    case TokenType::Print: outKeyword = "print"; return true;
    case TokenType::PrintLn: outKeyword = "println"; return true;
    case TokenType::Return: outKeyword = "return"; return true;
    case TokenType::Var: outKeyword = "var"; return true;
    case TokenType::While: outKeyword = "while"; return true;
    default:
      return false;
  }
}

static bool IsOperator(TokenType type)
{
  static const std::vector<TokenType> symbols = {
    TokenType::Colon,
    TokenType::Semicolon,
    TokenType::RightParen,
    TokenType::Comma,
    TokenType::Dot,
    TokenType::Plus,
    TokenType::Slash,
    TokenType::Star,
    TokenType::StarStar,
    TokenType::BangEqual,
    TokenType::Equal,
    TokenType::EqualEqual,
    TokenType::LessThan,
    TokenType::GreaterThan,
    TokenType::LessEqual,
    TokenType::GreaterEqual,
  };

  return std::any_of(symbols.begin(), symbols.end(), [type](TokenType t) {
      return t == type;
  });
}

void Compiler::EmitOp(VM::Ops op, int line)
{
  m_Vm.PushOp(op, line);
}

void Compiler::Declaration()
{
  if (Match(TokenType::Class)) {
    ClassDeclaration();
  } else if (Match(TokenType::Func)) {
    FuncDeclaration();
  } else if (Match(TokenType::Var)) {
    VarDeclaration();
  } else if (Match(TokenType::Final)) {
    FinalDeclaration();
  } else if (Match(TokenType::Break)) {
    if (m_CurrentContext != Context::Loop) {
      // don't return early from here, so the compiler can synchronize...
      ErrorAtPrevious("`break` only allowed inside `for` and `while` loops");
    } else {
      m_BreakJumpNeedsIndexes = true;
      auto constIdx = static_cast<std::int64_t>(m_Vm.GetNumConstants());
      EmitConstant(std::int64_t{});
      auto opIdx = static_cast<std::int64_t>(m_Vm.GetNumConstants());
      EmitConstant(std::int64_t{});
      EmitOp(Ops::Jump, m_Previous.value().GetLine());
      m_BreakIdxPairs.top().push_back(std::make_pair(constIdx, opIdx));
      Consume(TokenType::Semicolon, "Expected ';' after `break`");
    }
  } else {
    Statement();
  }
 
  if (m_PanicMode) {
    Synchronize();
  }
}

void Compiler::Statement()
{
  if (m_CurrentContext == Context::TopLevel) {
    ErrorAtCurrent("Only functions and classes are allowed at top level");
    return;
  }

  if (Match(TokenType::For)) {
    ForStatement();
  } else if (Match(TokenType::If)) {
    IfStatement();
  } else if (Match(TokenType::Print)) {
    PrintStatement();
  } else if (Match(TokenType::PrintLn)) {
    PrintLnStatement();
  } else if (Match(TokenType::Return)) {
    ReturnStatement();
  } else if (Match(TokenType::While)) {
    WhileStatement();
  } else {
    ExpressionStatement();
  }
}

void Compiler::ClassDeclaration() 
{
  GRACE_NOT_IMPLEMENTED();  
}

void Compiler::FuncDeclaration() 
{
  auto previous = m_CurrentContext;
  m_CurrentContext = Context::Function;  

  Consume(TokenType::Identifier, "Expected function name");
  auto name = std::string(m_Previous.value().GetText());

  Consume(TokenType::LeftParen, "Expected '(' after function name");

  std::vector<std::string> parameters;
  while (true) {
    if (Match(TokenType::Final)) {
      Consume(TokenType::Identifier, "Expected identifier after `final`");
      auto p = std::string(m_Previous.value().GetText());
      if (std::find(parameters.begin(), parameters.end(), p) != parameters.end()) {
        ErrorAtPrevious("Function parameters with the same name already defined");
        return;
      }
      m_Locals.insert(std::make_pair(p, std::make_pair(true, m_Locals.size())));
      parameters.push_back(p);
    } else if (Match(TokenType::Identifier)) {
      auto p = std::string(m_Previous.value().GetText());
      if (std::find(parameters.begin(), parameters.end(), p) != parameters.end()) {
        ErrorAtPrevious("Function parameters with the same name already defined");
        return;
      }
      m_Locals.insert(std::make_pair(p, std::make_pair(false, m_Locals.size())));
      parameters.push_back(p);
    } else if (Match(TokenType::RightParen)) {
      break;
    } else {
      Consume(TokenType::Comma, "Expected ',' after function parameter");
    }
  }

  Consume(TokenType::Colon, "Expected ':' after function signature");

  if (!m_Vm.AddFunction(name, m_Previous.value().GetLine(), std::move(parameters))) {
    ErrorAtPrevious("Duplicate function definitions");
    return;
  }

  m_FunctionHadReturn = false;
  while (!Match(TokenType::End)) {
    Declaration();
    if (m_Current.value().GetType() == TokenType::EndOfFile) {
      ErrorAtCurrent("Expected `end` after function");
      return;
    }
  }

  // implicitly return if the user didnt write a return so the VM knows to return to the caller
  if (!m_FunctionHadReturn && m_Vm.GetLastFunctionName() != "main") {
    EmitConstant((void*)nullptr);
    EmitOp(Ops::LoadConstant, m_Previous.value().GetLine());
    EmitOp(Ops::Return, m_Previous.value().GetLine());
  }
  
  m_Locals.clear();
  m_CurrentContext = previous;
}

void Compiler::VarDeclaration() 
{
  if (m_CurrentContext == Context::TopLevel) {
    ErrorAtPrevious("Only functions and classes are allowed at top level");
    return;
  }

  Consume(TokenType::Identifier, "Expected identifier after `var`");
  if (m_Locals.find(std::string(m_Previous.value().GetText())) != m_Locals.end()) {
    ErrorAtPrevious("A local variable with the same name already exists");
    return;
  }

  std::string localName(m_Previous.value().GetText());
  int line = m_Previous.value().GetLine();

  std::int64_t localId = m_Locals.size();
  m_Locals.insert(std::make_pair(localName, std::make_pair(false, localId)));
  EmitOp(Ops::DeclareLocal, line);

  if (Match(TokenType::Equal)) {
    m_IsInAssignmentExpression = true;
    Expression(false);
    m_IsInAssignmentExpression = false;
    line = m_Previous.value().GetLine();
    EmitConstant(localId);
    EmitOp(Ops::AssignLocal, line);
  }
  Consume(TokenType::Semicolon, "Expected ';' after `var` declaration");
}

void Compiler::FinalDeclaration() 
{
  if (m_CurrentContext == Context::TopLevel) {
    ErrorAtPrevious("Only functions and classes are allowed at top level");
    return;
  } 

  Consume(TokenType::Identifier, "Expected identifier after `final`");

  if (m_Locals.find(std::string(m_Previous.value().GetText())) != m_Locals.end()) {
    ErrorAtPrevious("A local variable with the same name already exists");
    return;
  }

  std::string localName(m_Previous.value().GetText());
  int line = m_Previous.value().GetLine();

  auto localId = static_cast<std::int64_t>(m_Locals.size());
  m_Locals.insert(std::make_pair(localName, std::make_pair(true, localId)));
  EmitOp(Ops::DeclareLocal, line);

  Consume(TokenType::Equal, "Must assign to `final` upon declaration");
  m_IsInAssignmentExpression = true;
  Expression(false);
  m_IsInAssignmentExpression = false;
  line = m_Previous.value().GetLine();
  EmitConstant(localId);
  EmitOp(Ops::AssignLocal, line);

  Consume(TokenType::Semicolon, "Expected ';' after `final` declaration");
}

void Compiler::Expression(bool canAssign)
{
  if (IsOperator(m_Current.value().GetType())) {
    ErrorAtCurrent("Expected identifier or literal at start of expression");
    Advance();
    return;
  }

  std::string kw;
  if (IsKeyword(m_Current.value().GetType(), kw)) {
    ErrorAtCurrent(fmt::format("'{}' is a keyword and not valid in this context", kw));
    Advance();  //consume the illegal identifier
    return;
  }

  if (Check(TokenType::Identifier)) {
    Call(canAssign);
    if (Check(TokenType::Equal)) {
      if (m_Previous.value().GetType() != TokenType::Identifier) {
        ErrorAtCurrent("Only identifiers can be assigned to");
        return;
      }

      auto localName = std::string(m_Previous.value().GetText());
      if (m_Locals.at(localName).first) {
        ErrorAtPrevious(fmt::format("Cannot reassign to final '{}'", m_Previous.value().GetText()));
        return;
      }

      Advance();  // consume the equals

      if (!canAssign) {
        ErrorAtCurrent("Assignment is not valid in the current context");
        return;
      }

      m_IsInAssignmentExpression = true;
      Expression(false); // disallow x = y = z...
      m_IsInAssignmentExpression = false;

      int line = m_Previous.value().GetLine();
      EmitConstant(m_Locals.at(localName).second);
      EmitOp(Ops::AssignLocal, line);
    } else {
      bool shouldBreak = false;
      while (!shouldBreak) {
        switch (m_Current.value().GetType()) {
          case TokenType::And:
            And(false, true);
            break;
          case TokenType::Or:
            Or(false, true);
            break;
          case TokenType::EqualEqual:
          case TokenType::BangEqual:
            Equality(false, true);
            break;
          case TokenType::GreaterThan:
          case TokenType::GreaterEqual:
          case TokenType::LessThan:
          case TokenType::LessEqual:
            Comparison(false, true);
            break;
          case TokenType::Plus:
          case TokenType::Minus:
            Term(false, true);
            break;
          case TokenType::Star:
          case TokenType::StarStar:
          case TokenType::Slash:
          case TokenType::Mod:
            Factor(false, true);
            break;
          case TokenType::Semicolon:
          case TokenType::RightParen:
          case TokenType::Comma:
          case TokenType::Colon:
            shouldBreak = true;
            break;
          default:
            ErrorAtCurrent("Invalid token found in expression");
            Advance();
            return;
        }
      }
    }
  } else {
    Or(canAssign, false);
  }
}

std::optional<std::exception> TryParseInt(const Token& token, std::int64_t& result)
{
  try {
    std::string str(token.GetText());
    result = std::stoll(str);
    return std::nullopt;
  } catch (const std::invalid_argument& e) {
    return e;
  } catch (const std::out_of_range& e) {
    return e;
  }
}

std::optional<std::exception> TryParseDouble(const Token& token, double& result)
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

void Compiler::ExpressionStatement() 
{
  Expression(true);
  Consume(TokenType::Semicolon, "Expected ';' after expression");
}

void Compiler::ForStatement() 
{
  auto previousContext = m_CurrentContext;
  m_CurrentContext = Context::Loop;

  m_BreakIdxPairs.emplace();

  Consume(TokenType::Identifier, "Expected identifier after `for`");
  auto iteratorName = std::string(m_Previous.value().GetText());
  std::int64_t iteratorId;
  if (m_Locals.find(iteratorName) == m_Locals.end()) {
    iteratorId = m_Locals.size();
    m_Locals.insert(std::make_pair(iteratorName, std::make_pair(false, iteratorId)));
    EmitOp(Ops::DeclareLocal, m_Previous.value().GetLine());
  } else {
    iteratorId = m_Locals.at(iteratorName).second;
  }

  Consume(TokenType::In, "Expected `in` after identifier");

  auto line = m_Previous.value().GetLine();
  if (Match(TokenType::Integer)) {
    std::int64_t value;
    auto result = TryParseInt(m_Previous.value(), value);
    if (result.has_value()) {
      ErrorAtPrevious(fmt::format("Token could not be parsed as integer: {}", result.value().what()));
      return;
    }
    EmitConstant(value);
    EmitOp(Ops::LoadConstant, line);
    EmitConstant(iteratorId);
    EmitOp(Ops::AssignLocal, line);
  } else if (Match(TokenType::Double)) {
    double value;
    auto result = TryParseDouble(m_Previous.value(), value);
    if (result.has_value()) {
      ErrorAtPrevious(fmt::format("Token could not be parsed as float: {}", result.value().what()));
      return;
    }
    EmitConstant(value);
    EmitOp(Ops::LoadConstant, line);
    EmitConstant(iteratorId);
    EmitOp(Ops::AssignLocal, line);
  } else if (Match(TokenType::Identifier)) {
    auto localName = std::string(m_Previous.value().GetText());
    if (m_Locals.find(localName) == m_Locals.end()) {
      ErrorAtPrevious(fmt::format("Cannot find variable '{}' in this scope.", localName));
      return;
    }
    auto localId = m_Locals.at(localName).second;
    EmitConstant(localId);
    EmitOp(Ops::LoadConstant, line);
    EmitOp(Ops::LoadLocal, line);
    EmitConstant(iteratorId);
    EmitOp(Ops::AssignLocal, line);
  } else {
    ErrorAtCurrent("Expected identifier or number as range min");
    return;
  }

  Consume(TokenType::DotDot, "Expected '..' after range min");

  union {
    std::int64_t m_Int;
    double m_Double;
    std::int64_t m_LocalId;
  } max;

  enum MaxType {
    Int,
    Double,
    Local,
  } maxType;

  if (Match(TokenType::Integer)) {
    std::int64_t value;
    auto result = TryParseInt(m_Previous.value(), value);
    if (result.has_value()) {
      ErrorAtPrevious(fmt::format("Token could not be parsed as integer: {}", result.value().what()));
      return;
    }
    max.m_Int = value;
    maxType = MaxType::Int;
  } else if (Match(TokenType::Double)) {
    double value;
    auto result = TryParseDouble(m_Previous.value(), value);
    if (result.has_value()) {
      ErrorAtPrevious(fmt::format("Token could not be parsed as float: {}", result.value().what()));
      return;
    }
    max.m_Double = value;
    maxType = MaxType::Double;
  } else if (Match(TokenType::Identifier)) {
    auto localName = std::string(m_Previous.value().GetText());
    if (m_Locals.find(localName) == m_Locals.end()) {
      ErrorAtPrevious(fmt::format("Cannot find variable '{}' in this scope.", localName));
      return;
    }
    auto localId = m_Locals.at(localName).second;
    max.m_LocalId = localId;
    maxType = MaxType::Local;
  } else {
    ErrorAtCurrent("Expected identifier or integer as range max");
    return;
  }

  union {
    std::int64_t m_Int;
    double m_Double;
  } increment;
  bool incrementIsInt;

  if (Match(TokenType::By)) {
    if (Match(TokenType::Integer)) {
      std::int64_t value;
      auto result = TryParseInt(m_Previous.value(), value);
      if (result.has_value()) {
        ErrorAtPrevious(fmt::format("Token could not be parsed as integer: {}", result.value().what()));
        return;
      }
      increment.m_Int = value;
      incrementIsInt = true;
    } else if (Match(TokenType::Double)) {
      double value;
      auto result = TryParseDouble(m_Previous.value(), value);
      if (result.has_value()) {
        ErrorAtPrevious(fmt::format("Token could not be parsed as float: {}", result.value().what()));
        return;
      }
      increment.m_Double = value;
      incrementIsInt = false;
    } else {
      ErrorAtPrevious("Increment must be a number");
      return;
    }
  } else {
    increment.m_Int = 1;
    incrementIsInt = true;
  }

  Consume(TokenType::Colon, "Expected ':' after `for` statement");

  auto constantIdx = static_cast<std::int64_t>(m_Vm.GetNumConstants());
  auto opIdx = static_cast<std::int64_t>(m_Vm.GetNumOps());

  while (!Match(TokenType::End)) {
    Declaration();

    if (Match(TokenType::EndOfFile)) {
      ErrorAtPrevious("Unterminated `for`");
      return;
    }
  }

  EmitConstant(iteratorId);
  EmitOp(Ops::LoadConstant, line);
  EmitOp(Ops::LoadLocal, line);
  if (incrementIsInt) {
    EmitConstant(increment.m_Int);
  } else {
    EmitConstant(increment.m_Double);
  }
  EmitOp(Ops::LoadConstant, line);
  EmitOp(Ops::Add, line);
  EmitConstant(iteratorId);
  EmitOp(Ops::AssignLocal, line);

  EmitConstant(iteratorId);
  EmitOp(Ops::LoadConstant, line);
  EmitOp(Ops::LoadLocal, line);

  switch (maxType) {
    case MaxType::Int:
      EmitConstant(max.m_Int);
      EmitOp(Ops::LoadConstant, line);
      break;
    case MaxType::Double:
      EmitConstant(max.m_Double);
      EmitOp(Ops::LoadConstant, line);
      break;
    case MaxType::Local:
      EmitConstant(max.m_LocalId);
      EmitOp(Ops::LoadConstant, line);
      EmitOp(Ops::LoadLocal, line);
      break;
  }

  EmitOp(Ops::GreaterEqual, line);

  EmitConstant(constantIdx);
  EmitConstant(opIdx);
  EmitOp(Ops::JumpIfFalse, line);

  if (m_BreakJumpNeedsIndexes) {
    for (auto& p : m_BreakIdxPairs.top()) {
      m_Vm.SetConstantAtIndex(p.first, static_cast<std::int64_t>(m_Vm.GetNumConstants()));
      m_Vm.SetConstantAtIndex(p.second, static_cast<std::int64_t>(m_Vm.GetNumOps()));
    }
    m_BreakJumpNeedsIndexes = !m_BreakIdxPairs.empty();
    m_BreakIdxPairs.pop();
  }

  m_CurrentContext = previousContext;
}

void Compiler::IfStatement() 
{
  m_IsInAssignmentExpression = true;
  Expression(false);
  m_IsInAssignmentExpression = false;
  Consume(TokenType::Colon, "Expected ':' after condition");
  
  // store indexes of constant and instruction indexes to jump
  auto topConstantIdxToJump = m_Vm.GetNumConstants();
  EmitConstant(std::int64_t{});
  auto topOpIdxToJump = m_Vm.GetNumConstants();
  EmitConstant(std::int64_t{});

  EmitOp(Ops::JumpIfFalse, m_Previous.value().GetLine());

  // constant index, op index
  std::vector<std::tuple<std::int64_t, std::int64_t>> endJumpIndexPairs;

  bool topJumpSet = false;
  bool elseBlockFound = false;
  bool elseIfBlockFound = false;
  bool needsElseBlock = true;
  while (true) {
    if (Match(TokenType::Else)) {
      // make any unreachables 'else' blocks a compiler error
      if (elseBlockFound) {
        ErrorAtPrevious("Unreachable `else` due to previous `else`");
        return;
      }
      
      // if the ifs condition passed and its block executed, it needs to jump to the end
      auto endConstantIdx = m_Vm.GetNumConstants();
      EmitConstant(std::int64_t{});
      auto endOpIdx = m_Vm.GetNumConstants();
      EmitConstant(std::int64_t{});
      EmitOp(Ops::Jump, m_Previous.value().GetLine());
      
      endJumpIndexPairs.emplace_back(endConstantIdx, endOpIdx);

      auto numConstants = m_Vm.GetNumConstants();
      auto numOps = m_Vm.GetNumOps();
      
      // haven't told the 'if' where to jump to yet if its condition fails 
      if (!topJumpSet) {
        m_Vm.SetConstantAtIndex(topConstantIdxToJump, static_cast<std::int64_t>(numConstants));
        m_Vm.SetConstantAtIndex(topOpIdxToJump, static_cast<std::int64_t>(numOps));
        topJumpSet = true;
      }

      if (Match(TokenType::Colon)) {
        elseBlockFound = true;
      } else if (Check(TokenType::If)) {
        elseIfBlockFound = true;
        needsElseBlock = false;
      } else {
        ErrorAtCurrent("Expected `if` or `:` after `else`");
        return;
      }
    }   

    Declaration();

    // above call to Declaration() will handle chained if/else
    if (elseIfBlockFound) {
      break;
    }
    
    if (Match(TokenType::EndOfFile)) {
      ErrorAtPrevious("Unterminated `if` statement");
      return;
    }

    if (needsElseBlock && Match(TokenType::End)) {
      break;
    }
  }

  auto numConstants = static_cast<std::int64_t>(m_Vm.GetNumConstants());
  auto numOps = static_cast<std::int64_t>(m_Vm.GetNumOps());

  for (auto [constIdx, opIdx] : endJumpIndexPairs) {
    m_Vm.SetConstantAtIndex(constIdx, numConstants);
    m_Vm.SetConstantAtIndex(opIdx, numOps);
  }
  
  // if there was no else or elseif block, jump to here
  if (!topJumpSet) {
    m_Vm.SetConstantAtIndex(topConstantIdxToJump, numConstants);
    m_Vm.SetConstantAtIndex(topOpIdxToJump, numOps);
  }
}

void Compiler::PrintStatement() 
{
  Consume(TokenType::LeftParen, "Expected '(' after 'print'");
  if (Match(TokenType::RightParen)) {
    EmitOp(Ops::PrintTab, m_Current.value().GetLine());
  } else {
    m_IsInAssignmentExpression = true;
    Expression(false);
    m_IsInAssignmentExpression = false;
    EmitOp(Ops::Print, m_Current.value().GetLine());
    EmitOp(Ops::Pop, m_Current.value().GetLine());
    Consume(TokenType::RightParen, "Expected ')' after expression");
  }
  Consume(TokenType::Semicolon, "Expected ';' after expression");
}

void Compiler::PrintLnStatement() 
{
  Consume(TokenType::LeftParen, "Expected '(' after 'println'");
  if (Match(TokenType::RightParen)) {
    EmitOp(Ops::PrintEmptyLine, m_Current.value().GetLine());
  } else {
    m_IsInAssignmentExpression = true;
    Expression(false);
    m_IsInAssignmentExpression = false;
    EmitOp(Ops::PrintLn, m_Current.value().GetLine());
    EmitOp(Ops::Pop, m_Current.value().GetLine());
    Consume(TokenType::RightParen, "Expected ')' after expression");
  }
  Consume(TokenType::Semicolon, "Expected ';' after expression");
}

void Compiler::ReturnStatement() 
{
  if (m_CurrentContext != Context::Function) {
    ErrorAtPrevious("`return` only allowed inside functions");
    return;
  }

  if (m_Vm.GetLastFunctionName() == "main") {
    ErrorAtPrevious("Cannot return from main function");
    return;
  } 

  if (Match(TokenType::Semicolon)) {
    EmitConstant((void*)nullptr);
    EmitOp(Ops::LoadConstant, m_Previous.value().GetLine());
    EmitOp(Ops::Return, m_Previous.value().GetLine());
    return;
  }

  m_IsInAssignmentExpression = true;
  Expression(false);
  m_IsInAssignmentExpression = false;
  EmitOp(Ops::Return, m_Previous.value().GetLine());
  Consume(TokenType::Semicolon, "Expected ';' after expression");
  m_FunctionHadReturn = true;
}

void Compiler::WhileStatement() 
{
  auto previousContext = m_CurrentContext;
  m_CurrentContext = Context::Loop;

  m_BreakIdxPairs.emplace();

  auto constantIdx = static_cast<std::int64_t>(m_Vm.GetNumConstants());
  auto opIdx = static_cast<std::int64_t>(m_Vm.GetNumOps());

  m_IsInAssignmentExpression = true;
  Expression(false);
  m_IsInAssignmentExpression = false;

  auto line = m_Previous.value().GetLine();

  auto endConstantJumpIdx = m_Vm.GetNumConstants();
  EmitConstant(std::int64_t{});
  auto endOpJumpIdx = m_Vm.GetNumConstants();
  EmitConstant(std::int64_t{});
  EmitOp(Ops::JumpIfFalse, m_Previous.value().GetLine());

  Consume(TokenType::Colon, "Expected ':' after expression");

  while (!Match(TokenType::End)) {
    Declaration();
    if (Match(TokenType::EndOfFile)) {
      ErrorAtPrevious("Unterminated `while` loop");
      return;
    }
  }

  // jump back up to the expression so it can be re-evaluated
  EmitConstant(constantIdx);
  EmitConstant(opIdx);
  EmitOp(Ops::Jump, line);

  auto numConstants = static_cast<std::int64_t>(m_Vm.GetNumConstants());
  auto numOps = static_cast<std::int64_t>(m_Vm.GetNumOps());
  m_Vm.SetConstantAtIndex(endConstantJumpIdx, numConstants);
  m_Vm.SetConstantAtIndex(endOpJumpIdx, numOps);

  if (m_BreakJumpNeedsIndexes) {
    for (auto& p : m_BreakIdxPairs.top()) {
      m_Vm.SetConstantAtIndex(p.first, numConstants);
      m_Vm.SetConstantAtIndex(p.second, numOps);
    }
    m_BreakJumpNeedsIndexes = !m_BreakIdxPairs.empty();
    m_BreakIdxPairs.pop();
  }
  
  m_CurrentContext = previousContext;
}

void Compiler::Or(bool canAssign, bool skipFirst)
{
  if (!skipFirst) {
    And(canAssign, false);
  }
  while (Match(TokenType::Or)) {
    And(canAssign, false);
    EmitOp(Ops::Or, m_Current.value().GetLine());
  }
}

void Compiler::And(bool canAssign, bool skipFirst)
{
  if (!skipFirst) {
    Equality(canAssign, false);
  }
  while (Match(TokenType::And)) {
    Equality(canAssign, false);
    EmitOp(Ops::And, m_Current.value().GetLine());
  }
}

void Compiler::Equality(bool canAssign, bool skipFirst)
{
  if (!skipFirst) {
    Comparison(canAssign, false);
  }
  if (Match(TokenType::EqualEqual)) {
    Comparison(canAssign, false);
    EmitOp(Ops::Equal, m_Current.value().GetLine());
  } else if (Match(TokenType::BangEqual)) {
    Comparison(canAssign, false);
    EmitOp(Ops::NotEqual, m_Current.value().GetLine());
  }
}

void Compiler::Comparison(bool canAssign, bool skipFirst)
{
  if (!skipFirst) {
    Term(canAssign, false);
  }
  if (Match(TokenType::GreaterThan)) {
    Term(canAssign, false);
    EmitOp(Ops::Greater, m_Current.value().GetLine());
  } else if (Match(TokenType::GreaterEqual)) {
    Term(canAssign, false);
    EmitOp(Ops::GreaterEqual, m_Current.value().GetLine());
  } else if (Match(TokenType::LessThan)) {
    Term(canAssign, false);
    EmitOp(Ops::Less, m_Current.value().GetLine());
  } else if (Match(TokenType::LessEqual)) {
    Term(canAssign, false);
    EmitOp(Ops::LessEqual, m_Current.value().GetLine());
  }
}

void Compiler::Term(bool canAssign, bool skipFirst)
{
  if (!skipFirst) {
    Factor(canAssign, false);
  }
  while (true) {
    if (Match(TokenType::Minus)) {
      Factor(canAssign, false);
      EmitOp(Ops::Subtract, m_Current.value().GetLine());
    } else if (Match(TokenType::Plus)) {
      Factor(canAssign, false);
      EmitOp(Ops::Add, m_Current.value().GetLine());
    } else {
      break;
    }
  }
}

void Compiler::Factor(bool canAssign, bool skipFirst)
{
  if (!skipFirst) {
    Unary(canAssign);
  }
  while (true) {
    if (Match(TokenType::StarStar)) {
      Unary(canAssign);
      EmitOp(Ops::Pow, m_Current.value().GetLine());
    } else if (Match(TokenType::Star)) {
      Unary(canAssign);
      EmitOp(Ops::Multiply, m_Current.value().GetLine());
    } else if (Match(TokenType::Slash)) {
      Unary(canAssign);
      EmitOp(Ops::Divide, m_Current.value().GetLine());
    } else if (Match(TokenType::Mod)) {
      Unary(canAssign);
      EmitOp(Ops::Mod, m_Current.value().GetLine());
    } else {
      break;
    }
  }
}

static std::unordered_map<TokenType, Ops> s_CastOps = {
  std::make_pair(TokenType::IntIdent, Ops::CastAsInt),
  std::make_pair(TokenType::FloatIdent, Ops::CastAsFloat),
  std::make_pair(TokenType::BoolIdent, Ops::CastAsBool),
  std::make_pair(TokenType::StringIdent, Ops::CastAsString),
  std::make_pair(TokenType::CharIdent, Ops::CastAsChar),
};

static bool IsPrimaryToken(const Token& token)
{
  auto type = token.GetType();
  return type == TokenType::True || type == TokenType::False 
    || type == TokenType::This || type == TokenType::Integer 
    || type == TokenType::Double || type == TokenType::String 
    || type == TokenType::Char || type == TokenType::Identifier;
}

static bool IsUnaryOp(const Token& token)
{
  auto type = token.GetType();
  return type == TokenType::Bang || type == TokenType::Minus;
}

void Compiler::Unary(bool canAssign)
{
  if (Match(TokenType::Bang)) {
    auto line = m_Previous.value().GetLine();
    Unary(canAssign);
    EmitOp(Ops::Not, line);
  } else if (Match(TokenType::Minus)) {
    auto line = m_Previous.value().GetLine();
    Unary(canAssign);
    EmitOp(Ops::Negate, line);
  } else {
    Call(canAssign);
  }
}

void Compiler::Call(bool canAssign)
{
  Primary(canAssign);
  auto prev = m_Previous.value();
  auto prevText = std::string(m_Previous.value().GetText());

  if (prev.GetType() != TokenType::Identifier && Check(TokenType::LeftParen)) {
    ErrorAtCurrent("'(' only allowed after functions and classes");
    return;
  }

  if (prev.GetType() == TokenType::Identifier) {
    if (Match(TokenType::LeftParen)) {
      std::int64_t numArgs = 0;
      if (!Match(TokenType::RightParen)) {
        while (true) {        
          Expression(false);
          numArgs++;
          if (Match(TokenType::RightParen)) {
            break;
          }
          Consume(TokenType::Comma, "Expected ',' after funcion call argument");
        }
      }
      
      auto hash = static_cast<std::int64_t>(m_Hasher(prevText));
      EmitConstant(hash);
      EmitConstant(numArgs);
      EmitOp(Ops::Call, m_Previous.value().GetLine());

      if (!m_IsInAssignmentExpression) {
        // pop unused return value
        EmitOp(Ops::Pop, m_Previous.value().GetLine());
      }
    } else if (Match(TokenType::Dot)) {
      // TODO: account for dot
    } else {
      if (m_Locals.find(std::string(prev.GetText())) == m_Locals.end()) {
        ErrorAtPrevious(fmt::format("Cannot find variable '{}' in this scope.", prev.GetText()));
        return;
      }

      // not a call or member access, so we are just trying to call on the value of the local
      // or reassign it 
      // if its not a reassignment, we are trying to load its value 
      // Primary() has already but the variable's id on the stack
      if (!Check(TokenType::Equal)) {
        EmitOp(Ops::LoadLocal, prev.GetLine());
      }
    }
  }
}

static bool IsTypeIdent(const Token& token)
{
  auto type = token.GetType();
  return type == TokenType::IntIdent || type == TokenType::FloatIdent 
    || type == TokenType::BoolIdent || type == TokenType::StringIdent 
    || type == TokenType::CharIdent;
}

void Compiler::Primary(bool canAssign)
{
  if (Match(TokenType::True)) {
    EmitOp(Ops::LoadConstant, m_Previous.value().GetLine());
    EmitConstant(true);
  } else if (Match(TokenType::False)) {
    EmitOp(Ops::LoadConstant, m_Previous.value().GetLine());
    EmitConstant(false);
  } else if (Match(TokenType::This)) {
    // TODO: this 
  } else if (Match(TokenType::Integer)) {
    try {
      std::string str(m_Previous.value().GetText());
      std::int64_t value = std::stoll(str);
      EmitOp(Ops::LoadConstant, m_Previous.value().GetLine());
      EmitConstant(value);
    } catch (const std::invalid_argument& e) {
      ErrorAtPrevious(fmt::format("Token could not be parsed as an int: {}", e.what()));
      return;
    } catch (const std::out_of_range&) {
      ErrorAtPrevious("Int out of range.");
      return;
    }
  } else if (Match(TokenType::Double)) {
    try {
      std::string str(m_Previous.value().GetText());
      auto value = std::stod(str);
      EmitOp(Ops::LoadConstant, m_Previous.value().GetLine());
      EmitConstant(value);
    } catch (const std::invalid_argument& e) {
      ErrorAtPrevious(fmt::format("Token could not be parsed as an float: {}", e.what()));
      return;
    } catch (const std::out_of_range&) {
      ErrorAtPrevious("Float out of range.");
      return;
    }
  } else if (Match(TokenType::String)) {
    String();
  } else if (Match(TokenType::Char)) {
    Char();
  } else if (Match(TokenType::Identifier)) {
    auto identName = std::string(m_Previous.value().GetText());
    if (m_Locals.find(identName) != m_Locals.end()) {
      // local variable access or reassignment, put its ID on the stack 
      auto localId = m_Locals.at(identName).second;
      EmitConstant(localId);
      EmitOp(Ops::LoadConstant, m_Previous.value().GetLine());
    }
    // if its not a local, it might be a function
    // do nothing here
  } else if (Match(TokenType::Null)) {
    EmitConstant((void*)nullptr);
    EmitOp(Ops::LoadConstant, m_Previous.value().GetLine());
  } else if (Match(TokenType::LeftParen)) {
    Expression(canAssign);
    Consume(TokenType::RightParen, "Expected ')'");
  } else if (Match(TokenType::InstanceOf)) {
    InstanceOf();
  } else if (IsTypeIdent(m_Current.value())) {
    Cast();
  } else {
    Expression(canAssign);
  }
}

static const char s_EscapeChars[] = {'t', 'b', 'n', 'r', 'f', '\'', '"', '\\'};
static const std::unordered_map<char, char> s_EscapeCharsLookup = {
  std::make_pair('t', '\t'),
  std::make_pair('b', '\b'),
  std::make_pair('r', '\r'),
  std::make_pair('n', '\n'),
  std::make_pair('f', '\f'),
  std::make_pair('\'', '\''),
  std::make_pair('"', '\"'),
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

void Compiler::Char()
{
  auto text = m_Previous.value().GetText();
  auto trimmed = text.substr(1, text.length() - 2);
  switch (trimmed.length()) {
    case 2:
      if (trimmed[0] != '\\') {
        ErrorAtPrevious("`char` must contain a single character or escape character");
        return;
      }
      char c;
      if (IsEscapeChar(trimmed[1], c)) {
        EmitOp(Ops::LoadConstant, m_Previous.value().GetLine());
        EmitConstant(c);
      } else {
        ErrorAtPrevious("Unrecognised escape character");
      }
      break;
    case 1:
      if (trimmed[0] == '\\') {
        ErrorAtPrevious("Expected escape character after backslash");
        return;
      }
      EmitOp(Ops::LoadConstant, m_Previous.value().GetLine());
      EmitConstant(static_cast<char>(trimmed[0]));
      break;
    default:
      ErrorAtPrevious("`char` must contain a single character or escape character");
      break;
  }
}

void Compiler::String()
{
  auto text = m_Previous.value().GetText();
  std::string res;
  for (auto i = 1; i < text.length() - 1; i++) {
    if (text[i] == '\\') {
      i++;
      if (i == text.length() - 2) {
        ErrorAtPrevious("Expected escape character");
        return;
      }
      char c;
      if (IsEscapeChar(text[i], c)) {
        res.push_back(c);
      } else {
        ErrorAtPrevious("Unrecognised escape character");
        return;
      }
    } else {
      res.push_back(text[i]);
    }
  }
  EmitOp(Ops::LoadConstant, m_Previous.value().GetLine());
  EmitConstant(res);
}

void Compiler::InstanceOf()
{
  Consume(TokenType::LeftParen, "Expected '(' after 'instanceof'");
  Expression(false);
  Consume(TokenType::Comma, "Expected ',' after expression");

  switch (m_Current.value().GetType()) {
    case TokenType::BoolIdent:
      EmitConstant(std::int64_t(0));
      break;
    case TokenType::CharIdent:
      EmitConstant(std::int64_t(1));
      break;
    case TokenType::FloatIdent:
      EmitConstant(std::int64_t(2));
      break;
    case TokenType::IntIdent:
      EmitConstant(std::int64_t(3));
      break;
    case TokenType::Null:
      EmitConstant(std::int64_t(4));
      break;
    case TokenType::StringIdent:
      EmitConstant(std::int64_t(5));
      break;
    default:
      ErrorAtCurrent("Expected type as second argument for `instanceof`");
      return;
  }

  EmitOp(Ops::CheckType, m_Current.value().GetLine());

  Advance();  // Consume the type ident
  Consume(TokenType::RightParen, "Expected ')'");

  if (!m_IsInAssignmentExpression) {
    // pop unused return value
    EmitOp(Ops::Pop, m_Previous.value().GetLine());
  }
}

void Compiler::Cast()
{
  auto type = m_Current.value().GetType();
  Advance();
  Consume(TokenType::LeftParen, "Expected '(' after type ident");
  Expression(false);
  EmitOp(s_CastOps[type], m_Current.value().GetLine());
  Consume(TokenType::RightParen, "Expected ')' after expression");
  if (!m_IsInAssignmentExpression) {
    // pop unused return value
    EmitOp(Ops::Pop, m_Previous.value().GetLine());
  }
}

void Compiler::ErrorAtCurrent(const std::string& message)
{
  Error(m_Current, message);
}

void Compiler::ErrorAtPrevious(const std::string& message)
{
  Error(m_Previous, message);
}

void Compiler::Error(const std::optional<Token>& token, const std::string& message)
{
  if (m_PanicMode) return;

  m_PanicMode = true;
  fmt::print(stderr, "[line {}] ", token.value().GetLine());
  fmt::print(stderr, fmt::fg(fmt::color::red) | fmt::emphasis::bold, "ERROR: ");
  
  auto type = token.value().GetType();
  switch (type) {
    case TokenType::EndOfFile:
      fmt::print(stderr, "at end: ");
      fmt::print(stderr, "{}\n", message);
      break;
    case TokenType::Error:
      fmt::print(stderr, "{}\n", token.value().GetErrorMessage());
      break;
    default:
      fmt::print(stderr, "at '{}': ", token.value().GetText());
      fmt::print(stderr, "{}\n", message);
      break;
  }

  auto lineNo = token.value().GetLine();
  auto column = token.value().GetColumn() - token.value().GetLength();  // need the START of the token
  fmt::print(stderr, "       --> {}:{}:{}\n", m_CurrentFileName, lineNo, column + 1); 
  fmt::print(stderr, "        |\n");
  fmt::print(stderr, "{:>8}| {}\n", lineNo, m_Scanner.GetCodeAtLine(lineNo));
  fmt::print(stderr, "        | ");
  for (auto i = 0; i < column; i++) {
    fmt::print(stderr, " ");
  }
  for (auto i = 0; i < token.value().GetLength(); i++) {
    fmt::print(stderr, fmt::fg(fmt::color::red), "^");
  }
  fmt::print("\n\n");

  m_HadError = true;
}

void Compiler::WarningAtCurrent(const std::string& message)
{
  Warning(m_Current, message);
}

void Compiler::WarningAtPrevious(const std::string& message)
{
  Warning(m_Previous, message);
}

void Compiler::Warning(const std::optional<Token>& token, const std::string& message)
{
  fmt::print(stderr, "[line {}] ", token.value().GetLine());
  fmt::print(stderr, fmt::fg(fmt::color::yellow) | fmt::emphasis::bold, "WARNING: ");
  
  auto type = token.value().GetType();
  switch (type) {
    case TokenType::EndOfFile:
      fmt::print(stderr, "at end: ");
      fmt::print(stderr, "{}\n", message);
      break;
    case TokenType::Error:
      fmt::print(stderr, "{}\n", token.value().GetErrorMessage());
      break;
    default:
      fmt::print(stderr, "at '{}': ", token.value().GetText());
      fmt::print(stderr, "{}\n", message);
      break;
  }

  auto lineNo = token.value().GetLine();
  auto column = token.value().GetColumn() - token.value().GetLength();  // need the START of the token
  fmt::print(stderr, "       --> {}:{}:{}\n", m_CurrentFileName, lineNo, column + 1); 
  fmt::print(stderr, "        |\n");
  fmt::print(stderr, "{:>8}| {}\n", lineNo, m_Scanner.GetCodeAtLine(lineNo));
  fmt::print(stderr, "        | ");
  for (auto i = 0; i < column; i++) {
    fmt::print(stderr, " ");
  }
  for (auto i = 0; i < token.value().GetLength(); i++) {
    fmt::print(stderr, fmt::fg(fmt::color::yellow), "^");
  }
  fmt::print("\n\n");
}

