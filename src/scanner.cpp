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
#include <utility>

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
  std::make_pair('{', TokenType::LeftCurlyParen),
  std::make_pair('}', TokenType::RightCurlyParen),
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
  std::make_pair("catch", TokenType::Catch),
  std::make_pair("continue", TokenType::Continue),
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
  std::make_pair("throw", TokenType::Throw),
  std::make_pair("true", TokenType::True),
  std::make_pair("try", TokenType::Try),
  std::make_pair("typename", TokenType::Typename),
  std::make_pair("var", TokenType::Var),
  std::make_pair("Int", TokenType::IntIdent),
  std::make_pair("Float", TokenType::FloatIdent),
  std::make_pair("Bool", TokenType::BoolIdent),
  std::make_pair("String", TokenType::StringIdent),
  std::make_pair("Char", TokenType::CharIdent),
  std::make_pair("List", TokenType::ListIdent),
  std::make_pair("Dict", TokenType::DictIdent),
};

static bool IsIdentifierChar(char c)
{
  return std::isalpha(c) || c == '_';
}

Token::Token(TokenType type,
  std::size_t start, 
  std::size_t length, 
  std::size_t line,
  std::size_t column,
  const std::string& code
) : m_Type(type), m_Start(start), m_Length(length),
  m_Line(line), m_Column(column), m_Text(code.c_str() + start, length)
{

}

Token::Token(TokenType type, std::size_t line, std::size_t column, std::string&& errorMessage)
  : m_Type(type), m_Start(0), m_Length(1), m_Line(line), m_Column(column), m_ErrorMessage(std::move(errorMessage))
{

}

std::string Token::ToString() const
{
  return fmt::format("Token [ type: {}, start: {}, length: {}, line: {}, text: '{}' ]", m_Type, m_Start, m_Length, m_Line, m_Text);
}

static void SkipWhitespace();

GRACE_NODISCARD static bool IsAtEnd();

static char Advance();
GRACE_NODISCARD static char Peek();
GRACE_NODISCARD static char PeekNext();
GRACE_NODISCARD static char PeekPrevious();

GRACE_NODISCARD static Token ErrorToken(std::string&& message);
GRACE_NODISCARD static Token Identifier();
GRACE_NODISCARD static Token Number();
GRACE_NODISCARD static Token MakeToken(TokenType);
GRACE_NODISCARD static Token MakeString();
GRACE_NODISCARD static Token MakeChar();

static std::string s_CodeString;
static std::size_t s_ScannerStart = 0, s_ScannerCurrent = 0;
static std::size_t s_ScannerLine = 1, s_ScannerColumn = 1;

void Grace::Scanner::InitScanner(std::string&& code)
{
  s_CodeString = std::move(code);
}

Token Grace::Scanner::ScanToken()
{
  SkipWhitespace();
  s_ScannerStart = s_ScannerCurrent;

  auto c = Advance();

  if (IsAtEnd()) {
    return Token(TokenType::EndOfFile, 0, 0, s_ScannerLine - 1, s_ScannerColumn - 1, "");
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
      if (Peek() == '=') {
        Advance();
        return MakeToken(TokenType::BangEqual);
      }
      return MakeToken(TokenType::Bang);
    case '=':
      if (Peek() == '=') {
        Advance();
        return MakeToken(TokenType::EqualEqual);
      }
      return MakeToken(TokenType::Equal);
    case '<':
      if (Peek() == '=') {
        Advance();
        return MakeToken(TokenType::LessEqual);
      }
      return MakeToken(TokenType::LessThan);
    case '>':
      if (Peek() == '=') {
        Advance();
        return MakeToken(TokenType::GreaterEqual);
      }
      return MakeToken(TokenType::GreaterThan);
    case '*':
      if (Peek() == '*') {
        Advance();
        return MakeToken(TokenType::StarStar);
      }
      return MakeToken(TokenType::Star);
    case '.':
      if (Peek() == '.') {
        Advance();
        return MakeToken(TokenType::DotDot);
      }
      return MakeToken(TokenType::Dot);
    case '"':
      return MakeString();
    case '\'':
      return MakeChar();
    default:
      return ErrorToken(fmt::format("Unexpected character: {}", c));
  }
}

std::string Grace::Scanner::GetCodeAtLine(int line)
{
  int curr = 1;
  int strIndex = 0;
  while (curr < line) {
    if (s_CodeString[strIndex++] == '\n') {
      curr++;
    }
  }

  std::string codeSubStr = s_CodeString.substr(strIndex, s_CodeString.length() - strIndex);
  codeSubStr = codeSubStr.substr(0, codeSubStr.find_first_of('\n'));
  return codeSubStr;
}

static void SkipWhitespace()
{
  while (true) {
    if (IsAtEnd()) {
      return;
    }

    switch (Peek()) {
      case '\t':
        s_ScannerColumn += 8;
      case ' ':
      case '\r':
        Advance();
        break;
      case '\n':
        s_ScannerLine++;
        s_ScannerColumn = 0;
        Advance();
        break;
      case '/': {
        if (PeekNext() == '/') {
          while (!IsAtEnd() && Peek() != '\n') {
            Advance();
          }
        } else if (PeekNext() == '*') {
          while (!IsAtEnd()) {
            if (Peek() == '\n') {
              s_ScannerLine++;
            }
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

static char Advance()
{
  if (IsAtEnd()) {
    return '\0';
  }

  s_ScannerCurrent++;
  s_ScannerColumn++;
  return s_CodeString[s_ScannerCurrent - 1];
}

static bool IsAtEnd()
{
  return s_ScannerCurrent >= s_CodeString.length();
}

static char Peek()
{
  return s_CodeString[s_ScannerCurrent];
}

static char PeekNext()
{
  if (IsAtEnd() || s_ScannerCurrent == s_CodeString.length() - 1) {
    return '\0';
  }

  return s_CodeString[s_ScannerCurrent + 1];
}

static char PeekPrevious()
{
  return s_ScannerCurrent == 0 ? '\0' : s_CodeString[s_ScannerCurrent - 1];
}

static Token ErrorToken(std::string&& message)
{
  return Token(TokenType::Error, s_ScannerLine, s_ScannerColumn, std::move(message));
}

static Token Identifier()
{
  while (!IsAtEnd() && (IsIdentifierChar(Peek()) || std::isdigit(Peek()))) {
    Advance();
  }

  std::string tokenStr = s_CodeString.substr(s_ScannerStart, s_ScannerCurrent - s_ScannerStart);
  if (s_KeywordLookup.find(tokenStr) != s_KeywordLookup.end()) {
    return MakeToken(s_KeywordLookup[tokenStr]);
  } else {
    return MakeToken(TokenType::Identifier);
  }
}

static Token Number()
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

static Token MakeToken(TokenType type)
{
  auto length = s_ScannerCurrent - s_ScannerStart;
  return Token(type, s_ScannerStart, length, s_ScannerLine, s_ScannerColumn - 1, s_CodeString);
}

static Token MakeString()
{
  while (!IsAtEnd()) {
    if (Peek() == '"') {
      if (PeekPrevious() != '\\') {
        break;
      }
    }
    if (Peek() == '\n') {
      s_ScannerLine++;
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
static Token MakeChar()
{
  while (!IsAtEnd()) {
    if (Peek() == '\'') {
      if (PeekNext() != '\'') {
        break;
      }
    }
    if (Peek() == '\n') {
      s_ScannerLine++;
    }
    Advance();
  }

  if (IsAtEnd()) {
    return ErrorToken("Unterminated char");
  }

  Advance();
  return MakeToken(TokenType::Char);
}
