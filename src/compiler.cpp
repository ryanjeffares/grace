#include <charconv>
#include <chrono>

#include "compiler.h"

using namespace Grace::Scanner;
using namespace Grace::VM;

void Grace::Compiler::Compile(std::string&& code, bool verbose)
{
  using namespace std::chrono;

  auto start = steady_clock::now();
 
  Detail::Compiler compiler(std::move(code));
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

using namespace Grace::Compiler::Detail;

Compiler::Compiler(std::string&& code) 
  : m_Scanner(std::move(code))
{
}

void Compiler::Finalise()
{
  m_Vm.PrintOps();
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
  Expression();
  m_Vm.PushOp(Ops::Print);
  m_Vm.PushOp(Ops::Pop);
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

void Compiler::And()
{
  Equality();
  while (Match(TokenType::And)) {
    Equality();
  }
}

void Compiler::Or()
{
  And();
  while (Match(TokenType::Or)) {
    And();
  }
}

void Compiler::Equality()
{
  Comparison();
  while (Match(TokenType::BangEqual) || Match(TokenType::EqualEqual)) {
    Comparison();
  }
}

void Compiler::Comparison()
{
  Term();
  while (Match(TokenType::GreaterThan) || Match(TokenType::GreaterEqual)
      || Match(TokenType::LessThan) || Match(TokenType::LessEqual)) {
    Term();
  }
}

void Compiler::Term()
{
  Factor();
  while (true) {
    if (Match(TokenType::Minus)) {
      Factor();
      m_Vm.PushOp(Ops::Subtract);
    } else if (Match(TokenType::Plus)) {
      Factor();
      m_Vm.PushOp(Ops::Add);
    } else {
      break;
    }
  }
}

void Compiler::Factor()
{
  Unary();
  while (true) {
    if (Match(TokenType::Slash)) {
      Unary();
      m_Vm.PushOp(Ops::Divide);
    } else if (Match(TokenType::Star)) {
      Unary();
      m_Vm.PushOp(Ops::Multiply);
    } else {
      break;
    }
  }
}

void Compiler::Unary()
{
  if (Match(TokenType::Bang) || Match(TokenType::Minus)) {

  }
  if (IsPrimaryToken()) {
    Call();
  } else {
    Unary();
  }
}

void Compiler::Call()
{
  Primary();
}

void Compiler::Primary()
{
  if (Match(TokenType::True)) {
    m_Vm.PushOp(Ops::LoadBool);
    m_Vm.PushConstant(true);
  } else if (Match(TokenType::False)) {
    m_Vm.PushOp(Ops::LoadBool);
    m_Vm.PushConstant(false);
  } else if (Match(TokenType::This)) {
    
  } else if (Match(TokenType::Integer)) {
    int value;
    const char* data = m_Previous.value().GetData();
    auto length = m_Previous.value().GetLength();
    auto [ptr, ec] { std::from_chars(data, data + length, value) };
    if (ec == std::errc()) {
      m_Vm.PushOp(Ops::LoadInteger);
      m_Vm.PushConstant(value);
    } else {
      switch (ec) {
        case std::errc::invalid_argument:
          ErrorAtPrevious(fmt::format("Token could not be parsed as an integer, failed at {}", *ptr));
          break;
        case std::errc::result_out_of_range:
          ErrorAtPrevious("Integer out of range.");
          break;
        default:
          ErrorAtPrevious(fmt::format("Unexpected error occurred at {}", *ptr));
          break;
      }
    }
  } else if (Match(TokenType::Float)) {
    try {
      std::string str(m_Previous.value().GetText());
      auto value = std::stof(str);
      m_Vm.PushOp(Ops::LoadFloat);
      m_Vm.PushConstant(value);
    } catch (const std::invalid_argument& e) {
      ErrorAtPrevious(fmt::format("Token could not be parsed as an float: {}", e.what()));
    } catch (const std::out_of_range&) {
      ErrorAtPrevious("Float out of range.");
    }
  } else if (Match(TokenType::String)) {

  } else if (Match(TokenType::Identifier)) {

  } else {
    Expression();
  }
}

bool Compiler::IsPrimaryToken()
{
  auto t = m_Current.value().GetType();
  return t == TokenType::True || t == TokenType::False || t == TokenType::This
    || t == TokenType::Integer || t == TokenType::Float || t == TokenType::String
    || t == TokenType::Identifier || t == TokenType::LeftParen;
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

