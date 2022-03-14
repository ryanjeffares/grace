#include <limits>

#include "compiler.h"

using namespace Grace::Scanner;

void Grace::Compiler::Compile(std::string&& code, bool verbose)
{
  using namespace std::chrono;

  auto start = steady_clock::now();
 
  Detail::Compiler compiler(std::move(code));
  compiler.Advance();
  
  while (!compiler.Match(TokenType::EndOfFile)) {
    compiler.Declaration();
  }

  auto end = steady_clock::now();
  auto duration = duration_cast<microseconds>(end - start).count();
  if (verbose) {
    fmt::print("Compilation succeeded in {} μs.\n", duration);
  }
}

using namespace Grace::Compiler::Detail;

Compiler::Compiler(std::string&& code) 
  : m_Scanner(std::move(code)),
  m_ParseRules({
    std::make_pair(TokenType::LeftParen, ParseRule{&Compiler::Grouping, &Compiler::Call, Precedence::Call}),
    std::make_pair(TokenType::RightParen, ParseRule{{}, {}, Precedence::None}),
    std::make_pair(TokenType::Comma, ParseRule{{}, {}, Precedence::None}),
    std::make_pair(TokenType::Dot, ParseRule{{}, &Compiler::Dot, Precedence::Call}),
    std::make_pair(TokenType::Minus, ParseRule{&Compiler::Unary, &Compiler::Binary, Precedence::Term}),
    std::make_pair(TokenType::Plus, ParseRule{{}, &Compiler::Binary, Precedence::Term}),
    std::make_pair(TokenType::Semicolon, ParseRule{{}, {}, Precedence::None}),
    std::make_pair(TokenType::Slash, ParseRule{{}, &Compiler::Binary, Precedence::Factor}),
    std::make_pair(TokenType::Star, ParseRule{{}, &Compiler::Binary, Precedence::Factor}),
    std::make_pair(TokenType::Bang, ParseRule{&Compiler::Unary, {}, Precedence::None}),
    std::make_pair(TokenType::BangEqual, ParseRule{{}, &Compiler::Binary, Precedence::None}),
    std::make_pair(TokenType::Equal, ParseRule{{}, {}, Precedence::Equality}),
    std::make_pair(TokenType::EqualEqual, ParseRule{{}, &Compiler::Binary, Precedence::Comparison}),
  })
{
}

void Compiler::Advance()
{
  m_Previous = m_Current;
  m_Current = m_Scanner.ScanToken();
  fmt::print("{}\n", m_Current.value().ToString());

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
  Expression();
  Consume(TokenType::RightParen, "Expected ')' after expression");
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

void Compiler::Grouping(bool canAssign)
{
  
}

void Compiler::Call(bool canAssign)
{
  
}

void Compiler::Dot(bool canAssign)
{
  
}

void Compiler::Unary(bool canAssign)
{
  
}

void Compiler::Binary(bool canAssign)
{
  
}

void Compiler::Variable(bool canAssign)
{
  
}

void Compiler::String(bool canAssign)
{
  
}

void Compiler::Number(bool canAssign)
{
  
}

void Compiler::And(bool canAssign)
{
  
}

void Compiler::Or(bool canAssign)
{
  
}

void Compiler::Literal(bool canAssign)
{
  
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
  
  switch (token.value().GetType()) {
    case TokenType::EndOfFile:
      fmt::print(stderr, "at end: ");
      break;
    case TokenType::Error:
      break;
    default:
      fmt::print(stderr, "at `{}`: ", token.value().GetText());
      break;
  }

  fmt::print(stderr, "{}\n", message);
  m_HadError = true;
}

