#include <charconv>
#include <chrono>

#include "compiler.hpp"

using namespace Grace::Scanner;
using namespace Grace::VM;

void Grace::Compiler::Compile(std::string&& fileName, std::string&& code, bool verbose)
{
  using namespace std::chrono;

  auto start = steady_clock::now();
 
  Compiler compiler(std::move(fileName), std::move(code));
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

Compiler::Compiler(std::string&& fileName, std::string&& code) 
  : m_Scanner(std::move(code)), 
  m_CurrentFileName(std::move(fileName)),
  m_CurrentContext(Context::TopLevel)
{
}

void Compiler::Finalise(bool verbose)
{
#ifdef GRACE_DEBUG
  if (verbose) {
    m_Vm.PrintOps();
  }
#endif
  m_Vm.Run("main", 1, verbose);
}

void Compiler::Advance()
{
  m_Previous = m_Current;
  m_Current = m_Scanner.ScanToken();
//  fmt::print("{}\n", m_Current.value().ToString());

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
        return;
      default:
        break;
    }

    Advance();
  }
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
  
}

void Compiler::FuncDeclaration() 
{
  auto previous = m_CurrentContext;
  m_CurrentContext = Context::Function;  

  Consume(TokenType::Identifier, "Expected function name");
  Grace::String name = std::string(m_Previous.value().GetText());
  if (!m_Vm.AddFunction(name)) {
    ErrorAtPrevious("Duplicate function definitions");
    return;
  }

  Consume(TokenType::LeftParen, "Expected '(' after function name");
  Consume(TokenType::RightParen, "Expected ')'");
  Consume(TokenType::Colon, "Expected ':' after function signature");

  while (!Match(TokenType::End)) {
    Declaration();
    if (m_Current.value().GetType() == TokenType::EndOfFile) {
      ErrorAtCurrent("Expected `end` after function");
      return;
    }
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
  if (std::find(m_Locals.begin(), m_Locals.end(), m_Previous.value().GetText()) 
      != m_Locals.end()) {
    ErrorAtPrevious("A local variable with the same name already exists");
    return;
  }

  std::string localName(m_Previous.value().GetText());
  int line = m_Previous.value().GetLine();

  m_Locals.push_back(localName);
  EmitConstant(localName);
  EmitOp(Ops::LoadConstant, line);
  EmitOp(Ops::DeclareLocal, line);
  EmitOp(Ops::Pop, line);

  if (Match(TokenType::Equal)) {
    EmitConstant(localName);
    EmitOp(Ops::LoadConstant, line);
    Expression(); 
    line = m_Previous.value().GetLine();
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
}

void Compiler::Expression()
{
  Assignment();
}

void Compiler::Assignment()
{
  if (Check(TokenType::Identifier)) {
    if (IsPrimaryToken()) {
      Call();
    }

    if (Check(TokenType::Equal)) {
      if (m_Previous.value().GetType() != TokenType::Identifier) {
        ErrorAtCurrent("Only identifiers can be assigned to");
        return;
      }
      Advance();  // consume the equals

      if (Match(TokenType::Identifier)) {
        Assignment();
      } else {
        Or();
      }

      int line = m_Previous.value().GetLine();
      EmitOp(Ops::AssignLocal, line);
    }
  } else {
    Or();
  }
}

void Compiler::ExpressionStatement() 
{
  Expression();
  Consume(TokenType::Semicolon, "Expected ';' after expression");
}

void Compiler::ForStatement() 
{
  
}

void Compiler::IfStatement() 
{
  
}

void Compiler::PrintStatement() 
{
  Consume(TokenType::LeftParen, "Expected '(' after 'print'");
  if (Match(TokenType::RightParen)) {
    EmitOp(Ops::PrintTab, m_Current.value().GetLine());
  } else {
    Expression();
    EmitOp(Ops::Print, m_Current.value().GetLine());
    EmitOp(Ops::Pop, m_Current.value().GetLine());
    Consume(TokenType::RightParen, "Expected ')' after expression");
  }
  Consume(TokenType::Semicolon, "Expected ';'");
}

void Compiler::PrintLnStatement() 
{
  Consume(TokenType::LeftParen, "Expected '(' after 'println'");
  if (Match(TokenType::RightParen)) {
    EmitOp(Ops::PrintEmptyLine, m_Current.value().GetLine());
  } else {
    Expression();
    EmitOp(Ops::PrintLn, m_Current.value().GetLine());
    EmitOp(Ops::Pop, m_Current.value().GetLine());
    Consume(TokenType::RightParen, "Expected ')' after expression");
  }
  Consume(TokenType::Semicolon, "Expected ';'");
}

void Compiler::ReturnStatement() 
{
  
}

void Compiler::WhileStatement() 
{
  
}

void Compiler::And()
{
  Equality();
  while (Match(TokenType::And)) {
    Equality();
    EmitOp(Ops::And, m_Current.value().GetLine());
  }
}

void Compiler::Or()
{
  And();
  while (Match(TokenType::Or)) {
    And();
    EmitOp(Ops::Or, m_Current.value().GetLine());
  }
}

void Compiler::Equality()
{
  Comparison();
  if (Match(TokenType::EqualEqual)) {
    Comparison();
    EmitOp(Ops::Equal, m_Current.value().GetLine());
  } else if (Match(TokenType::BangEqual)) {
    Comparison();
    EmitOp(Ops::NotEqual, m_Current.value().GetLine());
  }
}

void Compiler::Comparison()
{
  Term();
  if (Match(TokenType::GreaterThan)) {
    Term();
    EmitOp(Ops::Greater, m_Current.value().GetLine());
  } else if (Match(TokenType::GreaterEqual)) {
    Term();
    EmitOp(Ops::GreaterEqual, m_Current.value().GetLine());
  } else if (Match(TokenType::LessThan)) {
    Term();
    EmitOp(Ops::Less, m_Current.value().GetLine());
  } else if (Match(TokenType::LessEqual)) {
    Term();
    EmitOp(Ops::LessEqual, m_Current.value().GetLine());
  }
}

void Compiler::Term()
{
  Factor();
  while (true) {
    if (Match(TokenType::Minus)) {
      Factor();
      EmitOp(Ops::Subtract, m_Current.value().GetLine());
    } else if (Match(TokenType::Plus)) {
      Factor();
      EmitOp(Ops::Add, m_Current.value().GetLine());
    } else {
      break;
    }
  }
}

void Compiler::Factor()
{
  Unary();
  while (true) {
    if (Match(TokenType::StarStar)) {
      Unary();
      EmitOp(Ops::Pow, m_Current.value().GetLine());
    } else if (Match(TokenType::Star)) {
      Unary();
      EmitOp(Ops::Multiply, m_Current.value().GetLine());
    } else if (Match(TokenType::Slash)) {
      Unary();
      EmitOp(Ops::Divide, m_Current.value().GetLine());
    } else {
      break;
    }
  }
}

void Compiler::Unary()
{
  if (Match(TokenType::Bang) || Match(TokenType::Minus)) {
    // do something
    Unary();
  } else {
    Call();
  }
}

void Compiler::Call()
{
  Primary();
  auto prev = m_Previous.value();
  if (prev.GetType() != TokenType::Identifier && Check(TokenType::LeftParen)) {
    ErrorAtCurrent("'(' only allowed after functions and classes");
    return;
  }
  if (prev.GetType() == TokenType::Identifier) {
    if (Match(TokenType::LeftParen)) {
      // parsing the identifier puts the identifier name at the top of the stack
      // so we just need to validate the call syntax here
      // TODO: account for function args
      Consume(TokenType::RightParen, "Expected ')'");
      EmitOp(Ops::Call, m_Previous.value().GetLine());
    } else if (Match(TokenType::Dot)) {
      // TODO: account for dot
    } else {
      if (std::find(m_Locals.begin(), m_Locals.end(), prev.GetText()) == m_Locals.end()) {
        ErrorAtPrevious(fmt::format("Cannot find variable '{}' in this scope.", prev.GetText()));
        return;
      }
      // not a call or member access, so we are just trying to call on the value of the local
      // or reassign it 
      // if its not a reassignment, we are trying to load its value 
      // Primary() has already but the variable's name on the stack
      if (!Check(TokenType::Equal)) {
        EmitOp(Ops::LoadLocal, prev.GetLine());
      }
    }
  }
}

void Compiler::Primary()
{
  if (Match(TokenType::True)) {
    EmitOp(Ops::LoadConstant, m_Previous.value().GetLine());
    EmitConstant(true);
  } else if (Match(TokenType::False)) {
    EmitOp(Ops::LoadConstant, m_Previous.value().GetLine());
    EmitConstant(false);
  } else if (Match(TokenType::This)) {
    
  } else if (Match(TokenType::Integer)) {
    try {
      std::string str(m_Previous.value().GetText());
      std::int64_t value = std::stoll(str);
      EmitOp(Ops::LoadConstant, m_Previous.value().GetLine());
      EmitConstant(value);
    } catch (const std::invalid_argument& e) {
      ErrorAtPrevious(fmt::format("Token could not be parsed as an int: {}", e.what()));
    } catch (const std::out_of_range&) {
      ErrorAtPrevious("Int out of range.");
    }
  } else if (Match(TokenType::Double)) {
    try {
      std::string str(m_Previous.value().GetText());
      auto value = std::stod(str);
      EmitOp(Ops::LoadConstant, m_Previous.value().GetLine());
      EmitConstant(value);
    } catch (const std::invalid_argument& e) {
      ErrorAtPrevious(fmt::format("Token could not be parsed as an float: {}", e.what()));
    } catch (const std::out_of_range&) {
      ErrorAtPrevious("Float out of range.");
    }
  } else if (Match(TokenType::String)) {
    String();
  } else if (Match(TokenType::Char)) {
    Char();
  } else if (Match(TokenType::Identifier)) {
    EmitConstant(std::string(m_Previous.value().GetText()));
    EmitOp(Ops::LoadConstant, m_Previous.value().GetLine());
  } else {
    Expression();
  }
}

static char s_EscapeChars[] = {'t', 'b', 'n', 'r', 'f', '\'', '"', '\\'};
static std::unordered_map<char, char> s_EscapeCharsLookup = {
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
  for (auto i = 0; i < sizeof(s_EscapeChars) / sizeof(char); i++) {
    if (c == s_EscapeChars[i]) {
      result = s_EscapeCharsLookup[s_EscapeChars[i]];
      return true;
    }
  }
  return false;
}

bool Compiler::IsPrimaryToken()
{
  auto t = m_Current.value().GetType();
  return t == TokenType::True || t == TokenType::False || t == TokenType::This
    || t == TokenType::Integer || t == TokenType::Double || t == TokenType::String
    || t == TokenType::Char || t == TokenType::Identifier || t == TokenType::LeftParen;
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
  fmt::print(stderr, "{:>7} | {}\n", lineNo, m_Scanner.GetCodeAtLine(lineNo));
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

