/*
 *  The Grace Programming Language.
 *
 *  This file contains the out of line definitions for the Scanner class, which 
 *  produces Tokens based on inputted source code, as well as some static helper functions. 
 *  
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#include <unordered_map>

#include "scanner.hpp"

using namespace Grace::Scanner;

static std::unordered_map<char, TokenType> s_SymbolLookup = 
{   
  std::make_pair(':', TokenType::Colon),
  std::make_pair(';', TokenType::Semicolon),
  std::make_pair('(', TokenType::LeftParen),
  std::make_pair(')', TokenType::RightParen),
  std::make_pair('[', TokenType::LeftSquareParen),
  std::make_pair(']', TokenType::RightSquareParen),
  std::make_pair(',', TokenType::Comma),
  std::make_pair('-', TokenType::Minus),
  std::make_pair('%', TokenType::Mod),
  std::make_pair('+', TokenType::Plus),
  std::make_pair('/', TokenType::Slash),
};

static std::unordered_map<std::string, TokenType> s_KeywordLookup = 
{
  std::make_pair("assert", TokenType::Assert),
  std::make_pair("and", TokenType::And),
  std::make_pair("or", TokenType::Or),
  std::make_pair("break", TokenType::Break),
  std::make_pair("by", TokenType::By),
  std::make_pair("class", TokenType::Class),
  std::make_pair("end", TokenType::End),
  std::make_pair("else", TokenType::Else),
  std::make_pair("false", TokenType::False),
  std::make_pair("final", TokenType::Final),
  std::make_pair("for", TokenType::For),
  std::make_pair("func", TokenType::Func),
  std::make_pair("if", TokenType::If),
  std::make_pair("in", TokenType::In),
  std::make_pair("instanceof", TokenType::InstanceOf),
  std::make_pair("null", TokenType::Null),
  std::make_pair("print", TokenType::Print),
  std::make_pair("println", TokenType::PrintLn),
  std::make_pair("return", TokenType::Return),
  std::make_pair("while", TokenType::While),
  std::make_pair("this", TokenType::This),
  std::make_pair("true", TokenType::True),
  std::make_pair("var", TokenType::Var),
  std::make_pair("int", TokenType::IntIdent),
  std::make_pair("float", TokenType::FloatIdent),
  std::make_pair("bool", TokenType::BoolIdent),
  std::make_pair("string", TokenType::StringIdent),
  std::make_pair("char", TokenType::CharIdent),
  std::make_pair("list", TokenType::ListIdent),
};

static bool IsIdentifierChar(char c)
{
  return std::isalpha(c) || c == '_';
}

static bool IsSingleCharToken(TokenType type)
{
  static const std::vector<TokenType> tokens = {
    TokenType::Equal,
    TokenType::LessThan,
    TokenType::GreaterThan,
    TokenType::Bang,
    TokenType::Dot,
    TokenType::Star,
  };

  return std::any_of(tokens.begin(), tokens.end(), [type](TokenType t) {
      return t == type;
  });
}

Token::Token(TokenType type,
  std::size_t start, 
  std::size_t length, 
  int line, 
  int column,
  const std::string& code
) : m_Type(type), m_Start(start), m_Line(line), 
  m_Text(code.c_str() + start, length)
{
  if (IsSingleCharToken(type)) {
    m_Length = 1;
    m_Column = column - 1;
  } else {
    m_Length = length;
    m_Column = column;
  }
}

Token::Token(TokenType type, int line, int column, const std::string& errorMessage)
  : m_Type(type), m_Start(0), m_Length(1), m_Line(line), m_Column(column), m_ErrorMessage(errorMessage)
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
    return Token(TokenType::EndOfFile, 0, 0, m_Line - 1, m_Column - 1, "");
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
      // for some reason, using MatchChar was fucking this up
      if (Peek() == '=') {
        Advance();
        return MakeToken(TokenType::BangEqual);
      } else {
        return MakeToken(TokenType::Bang);
      }
    case '=':
      return MakeToken(MatchChar('=') ? TokenType::EqualEqual : TokenType::Equal);
    case '<':
      return MakeToken(MatchChar('=') ? TokenType::LessEqual : TokenType::LessThan);
    case '>':
      return MakeToken(MatchChar('=') ? TokenType::GreaterEqual : TokenType::GreaterThan);
    case '*':
      return MakeToken(MatchChar('*') ? TokenType::StarStar: TokenType::Star);
    case '.':
      return MakeToken(MatchChar('.') ? TokenType::DotDot : TokenType::Dot);
    case '"':
      return MakeString();
    case '\'':
      return MakeChar();
    default:
      return ErrorToken(fmt::format("Unexpected character: {}", c));
  }
}

std::string Scanner::GetCodeAtLine(int line) const
{
  int curr = 1;
  int strIndex = 0;
  while (curr < line) {
    if (m_CodeString[strIndex++] == '\n') {
      curr++;
    }
  }

  std::string codeSubStr = m_CodeString.substr(strIndex, m_CodeString.length() - strIndex);
  codeSubStr = codeSubStr.substr(0, codeSubStr.find_first_of('\n'));
  return codeSubStr;
}

void Scanner::SkipWhitespace()
{
  while (true) {
    if (IsAtEnd()) {
      return;
    }

    switch (Peek()) {
      case '\t':
        m_Column += 8;
      case ' ':
      case '\r':
        Advance();
        break;
      case '\n':
        m_Line++;
        m_Column = 0;
        Advance();
        break;
      case '/': {
        if (PeekNext() == '/') {
          while (!IsAtEnd() && Peek() != '\n') {
            Advance();
          }
        } else if (PeekNext() == '*') {
          while (!IsAtEnd()) {
            if (Peek() == '*') {
              Advance();
              if (Peek() == '/') {
                Advance();
                break;
              }
            } else {
              Advance();
            }
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
  m_Column++;
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

char Scanner::PeekPrevious()
{
  return m_Current == 0 ? '\0' : m_CodeString[m_Current - 1];
}

Token Scanner::ErrorToken(const std::string& message)
{
  return Token(TokenType::Error, m_Line, m_Column, message);
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

    return MakeToken(TokenType::Double);
  } else {
    return MakeToken(TokenType::Integer);
  }
}

Token Scanner::MakeToken(TokenType type) const
{
  auto length = m_Current - m_Start;
  return Token(type, m_Start, length, m_Line, m_Column - 1, m_CodeString);
}

Token Scanner::MakeString()
{
  while (!IsAtEnd()) {
    if (Peek() == '"') {
      if (PeekPrevious() != '\\') {
        break;
      }
    }
    if (Peek() == '\n') {
      m_Line++;
    }
    Advance();
  }

  if (IsAtEnd()) {
    return ErrorToken("Unterminated string");
  }

  Advance();
  return MakeToken(TokenType::String);
}

// not doing error checking here to do better error reporting in the compiler
Token Scanner::MakeChar()
{
  while (!IsAtEnd()) {
    if (Peek() == '\'') {
      if (PeekNext() != '\'') {
        break;
      }
    }
    if (Peek() == '\n') {
      m_Line++;
    }
    Advance();
  }

  if (IsAtEnd()) {
    return ErrorToken("Unterminated char");
  }

  Advance();
  return MakeToken(TokenType::Char);
}
