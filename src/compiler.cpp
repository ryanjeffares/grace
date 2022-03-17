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
  }

  if (compiler.HadError()) {
    fmt::print(stderr, "Terminating process due to compilation errors.\n");
  } else {
    if (verbose) {
      auto end = steady_clock::now();
      auto duration = duration_cast<microseconds>(end - start).count();
      fmt::print("Compilation succeeded in {} μs.\n", duration);
    }
    compiler.Finalise();
  }
}

using namespace Grace::Compiler;

Compiler::Compiler(std::string&& fileName, std::string&& code) 
  : m_Scanner(std::move(code)), m_CurrentFileName(std::move(fileName))
{
}

void Compiler::Finalise()
{
//  m_Vm.PrintOps();
  m_Vm.Run();
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
  
}

void Compiler::VarDeclaration() 
{
  
}

void Compiler::FinalDeclaration() 
{
  
}

void Compiler::Expression()
{
  if (Match(TokenType::Identifier)) {
    Consume(TokenType::Equal, "Expected '=' after identifier");
    if (Match(TokenType::Identifier)) {
      Expression();
    } else {
      Or();
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
    m_Vm.PushOp(Ops::PrintTab, m_Current.value().GetLine());
  } else {
    Expression();
    m_Vm.PushOp(Ops::Print, m_Current.value().GetLine());
    m_Vm.PushOp(Ops::Pop, m_Current.value().GetLine());
    Consume(TokenType::RightParen, "Expected ')' after expression");
  }
  Consume(TokenType::Semicolon, "Expected ';'");
}

void Compiler::PrintLnStatement() 
{
  Consume(TokenType::LeftParen, "Expected '(' after 'println'");
  if (Match(TokenType::RightParen)) {
    m_Vm.PushOp(Ops::PrintEmptyLine, m_Current.value().GetLine());
  } else {
    Expression();
    m_Vm.PushOp(Ops::PrintLn, m_Current.value().GetLine());
    m_Vm.PushOp(Ops::Pop, m_Current.value().GetLine());
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

void Compiler::Block() 
{
  
}

void Compiler::And()
{
  Equality();
  while (Match(TokenType::And)) {
    Equality();
    m_Vm.PushOp(Ops::And, m_Current.value().GetLine());
  }
}

void Compiler::Or()
{
  And();
  while (Match(TokenType::Or)) {
    And();
    m_Vm.PushOp(Ops::Or, m_Current.value().GetLine());
  }
}

void Compiler::Equality()
{
  Comparison();
  if (Match(TokenType::EqualEqual)) {
    Comparison();
    m_Vm.PushOp(Ops::Equal, m_Current.value().GetLine());
  } else if (Match(TokenType::BangEqual)) {
    Comparison();
    m_Vm.PushOp(Ops::NotEqual, m_Current.value().GetLine());
  }
}

void Compiler::Comparison()
{
  Term();
  if (Match(TokenType::GreaterThan)) {
    Term();
    m_Vm.PushOp(Ops::Greater, m_Current.value().GetLine());
  } else if (Match(TokenType::GreaterEqual)) {
    Term();
    m_Vm.PushOp(Ops::GreaterEqual, m_Current.value().GetLine());
  } else if (Match(TokenType::LessThan)) {
    Term();
    m_Vm.PushOp(Ops::Less, m_Current.value().GetLine());
  } else if (Match(TokenType::LessEqual)) {
    Term();
    m_Vm.PushOp(Ops::LessEqual, m_Current.value().GetLine());
  }
}

void Compiler::Term()
{
  Factor();
  while (true) {
    if (Match(TokenType::Minus)) {
      Factor();
      m_Vm.PushOp(Ops::Subtract, m_Current.value().GetLine());
    } else if (Match(TokenType::Plus)) {
      Factor();
      m_Vm.PushOp(Ops::Add, m_Current.value().GetLine());
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
      m_Vm.PushOp(Ops::Pow, m_Current.value().GetLine());
    } else if (Match(TokenType::Star)) {
      Unary();
      m_Vm.PushOp(Ops::Multiply, m_Current.value().GetLine());
    } else if (Match(TokenType::Slash)) {
      Unary();
      m_Vm.PushOp(Ops::Divide, m_Current.value().GetLine());
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
  } else if (IsPrimaryToken()) {
    Call();
  }
}

void Compiler::Call()
{
  Primary();
}

void Compiler::Primary()
{
  if (Match(TokenType::True)) {
    m_Vm.PushOp(Ops::LoadConstant, m_Previous.value().GetLine());
    m_Vm.PushConstant(true);
  } else if (Match(TokenType::False)) {
    m_Vm.PushOp(Ops::LoadConstant, m_Previous.value().GetLine());
    m_Vm.PushConstant(false);
  } else if (Match(TokenType::This)) {
    
  } else if (Match(TokenType::Integer)) {
    try {
      std::string str(m_Previous.value().GetText());
      std::int64_t value = std::stoll(str);
      m_Vm.PushOp(Ops::LoadConstant, m_Previous.value().GetLine());
      m_Vm.PushConstant(value);
    } catch (const std::invalid_argument& e) {
      ErrorAtPrevious(fmt::format("Token could not be parsed as an int: {}", e.what()));
    } catch (const std::out_of_range&) {
      ErrorAtPrevious("Int out of range.");
    }
  } else if (Match(TokenType::Double)) {
    try {
      std::string str(m_Previous.value().GetText());
      auto value = std::stod(str);
      m_Vm.PushOp(Ops::LoadConstant, m_Previous.value().GetLine());
      m_Vm.PushConstant(value);
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
        m_Vm.PushOp(Ops::LoadConstant, m_Previous.value().GetLine());
        m_Vm.PushConstant(c);
      } else {
        ErrorAtPrevious("Unrecognised escape character");
      }
      break;
    case 1:
      if (trimmed[0] == '\\') {
        ErrorAtPrevious("Expected escape character after backslash");
        return;
      }
      m_Vm.PushOp(Ops::LoadConstant, m_Previous.value().GetLine());
      m_Vm.PushConstant(static_cast<char>(trimmed[0]));
      break;
    default:
      ErrorAtPrevious("`char` must contain a single character or escape character");
      break;
  }
}

void Compiler::String()
{
  auto text = m_Previous.value().GetText();
  std::string trimmed(text.substr(1, text.length() - 2));
  m_Vm.PushOp(Ops::LoadConstant, m_Previous.value().GetLine());
  m_Vm.PushConstant(trimmed);
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

