#include "scanner.h"

using namespace Grace::Scanner;

static std::unordered_map<char, TokenType> s_SymbolLookup = 
{   
  std::make_pair(':', TokenType::Colon),
  std::make_pair(';', TokenType::Semicolon),
  std::make_pair('(', TokenType::LeftParen),
  std::make_pair(')', TokenType::RightParen),
  std::make_pair(',', TokenType::Comma),
  std::make_pair('.', TokenType::Dot),
  std::make_pair('-', TokenType::Minus),
  std::make_pair('+', TokenType::Plus),
  std::make_pair('/', TokenType::Slash),
  std::make_pair('*', TokenType::Star),
};

static std::unordered_map<std::string, TokenType> s_KeywordLookup = 
{
  std::make_pair("and", TokenType::And),
  std::make_pair("or", TokenType::Or),
  std::make_pair("as", TokenType::As),
  std::make_pair("class", TokenType::Class),
  std::make_pair("end", TokenType::End),
  std::make_pair("false", TokenType::False),
  std::make_pair("final", TokenType::Final),
  std::make_pair("for", TokenType::For),
  std::make_pair("func", TokenType::Func),
  std::make_pair("if", TokenType::If),
  std::make_pair("print", TokenType::Print),
  std::make_pair("while", TokenType::While),
  std::make_pair("this", TokenType::This),
  std::make_pair("true", TokenType::True),
  std::make_pair("var", TokenType::Var),
};

static Token EndOfFileToken()
{
  return Token(TokenType::EndOfFile, 0, 0, 0, "");
}

static bool IsIdentifierChar(char c)
{
  return std::isalpha(c) || c == '_';
}

Token::Token(TokenType type,
  std::size_t start, 
  std::size_t length, 
  std::size_t line, 
  const std::string& code
) : m_Type(type), m_Start(start), m_Length(length), m_Line(line), 
  m_Text(code.c_str() + start, length)
{
}

std::string Token::ToString() const
{
  return fmt::format("Token [ type: {}, start: {}, length: {}, line: {}, text: '{}' ]", m_Type, m_Start, m_Length, m_Line, m_Text);
}

Scanner::Scanner(std::string&& code) 
  : m_CodeString(std::move(code))
{

}

Token Scanner::ScanToken()
{
  SkipWhitespace();
  m_Start = m_Current;

  auto c = Advance();
  if (IsAtEnd()) {
    return EndOfFileToken();
  }

  if (std::isalpha(c) || c == '_') {
    return Identifier();
  }
  if (std::isdigit(c)) {
    return Number();
  }

  if (s_SymbolLookup.find(c) != s_SymbolLookup.end()) {
    return MakeToken(s_SymbolLookup[c]);
  }

  switch (c) {
    case '!':
      return MakeToken(MatchChar('=') ? TokenType::BangEqual : TokenType::Bang);
    case '=':
      return MakeToken(MatchChar('=') ? TokenType::EqualEqual : TokenType::Equal);
    case '<':
      return MakeToken(MatchChar('=') ? TokenType::LessEqual : TokenType::LessThan);
    case '>':
      return MakeToken(MatchChar('=') ? TokenType::GreaterEqual : TokenType::GreaterThan);
    case '"':
      return MakeString();
    default:
      return ErrorToken(fmt::format("Unexpected character: {}", c));
  }
}

void Scanner::SkipWhitespace()
{
  while (true) {
    if (IsAtEnd()) {
      return;
    }

    switch (Peek()) {
      case ' ':
      case '\r':
      case '\t':
        Advance();
        break;
      case '\n':
        m_Line++;
        Advance();
        break;
      case '/': {
        if (PeekNext() == '/') {
          while (!IsAtEnd() && Peek() != '\n') {
            Advance();
          }
        } else {
          return;
        }
        break;
      }
      default:
        return;
    }
  }
}

char Scanner::Advance()
{
  if (IsAtEnd()) {
    return '\0';
  }

  m_Current++;
  return m_CodeString[m_Current - 1];
}

bool Scanner::IsAtEnd() const
{
  return m_Current >= m_CodeString.length();
}

bool Scanner::MatchChar(char toMatch)
{
  if (IsAtEnd()) {
    return false;
  }
  
  return Advance() == toMatch;
}

char Scanner::Peek()
{
  return m_CodeString[m_Current];
}

char Scanner::PeekNext()
{
  if (IsAtEnd() || m_Current == m_CodeString.length() - 1) {
    return '\0';
  }

  return m_CodeString[m_Current + 1];
}

Token Scanner::ErrorToken(const std::string& message)
{
  return Token(TokenType::Error, m_Start, message.length(), m_Line, message);
}

Token Scanner::Identifier()
{
  while (!IsAtEnd() && (IsIdentifierChar(Peek()) || std::isdigit(Peek()))) {
    Advance();
  }

  std::string tokenStr = m_CodeString.substr(m_Start, m_Current - m_Start);
  if (s_KeywordLookup.find(tokenStr) != s_KeywordLookup.end()) {
    return MakeToken(s_KeywordLookup[tokenStr]);
  } else {
    return MakeToken(TokenType::Identifier);
  }
}

Token Scanner::Number()
{
  while (!IsAtEnd() && std::isdigit(Peek())) {
    Advance();
  }

  if (!IsAtEnd() && Peek() == '.' && std::isdigit(PeekNext())) {
    Advance();
    while (std::isdigit(Peek())) {
      Advance();
    }

    return MakeToken(TokenType::Float);
  } else {
    return MakeToken(TokenType::Integer);
  }
}

Token Scanner::MakeToken(TokenType type) const
{
  auto length = m_Current - m_Start;
  return Token(type, m_Start, length, m_Line, m_CodeString);
}

Token Scanner::MakeString()
{
  while (!IsAtEnd() && Peek() != '"') {
    if (Peek() == '\n') {
      m_Line++;
    }
    Advance();
  }

  if (IsAtEnd()) {
    return ErrorToken("Unterminated string.");
  }

  Advance();
  return MakeToken(TokenType::String);
}

