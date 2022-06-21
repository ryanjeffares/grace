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

#include <stack>
#include <unordered_map>
#include <utility>

#include "scanner.hpp"

using namespace Grace::Scanner;

static std::unordered_map<char, TokenType> s_SymbolLookup = 
{   
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
  std::make_pair('|', TokenType::Bar),
  std::make_pair('~', TokenType::Tilde),
  std::make_pair('^', TokenType::Caret),
  std::make_pair('&', TokenType::Ampersand),
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
  std::make_pair("import", TokenType::Import),
  std::make_pair("in", TokenType::In),
  std::make_pair("instanceof", TokenType::InstanceOf),
  std::make_pair("null", TokenType::Null),
  std::make_pair("print", TokenType::Print),
  std::make_pair("println", TokenType::PrintLn),
  std::make_pair("eprint", TokenType::Eprint),
  std::make_pair("eprintln", TokenType::EprintLn),
  std::make_pair("export", TokenType::Export),
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

struct ScannerContext
{
  std::string codeString;
  std::size_t scannerStart = 0, scannerCurrent = 0;
  std::size_t scannerLine = 1, scannerColumn = 1;

  ScannerContext(std::string&& code)
    : codeString(std::move(code))
  {

  }
};

static std::stack<ScannerContext> s_ScannerContextStack;
static std::unordered_map<std::string, std::string> s_CodeStringsLookup;

bool Grace::Scanner::HasFile(const std::string& fileName)
{
  return s_CodeStringsLookup.find(fileName) != s_CodeStringsLookup.end();
}

void Grace::Scanner::InitScanner(const std::string& fileName, std::string&& code)
{
  s_CodeStringsLookup.try_emplace(fileName, code);
  s_ScannerContextStack.emplace(std::move(code));
}

void Grace::Scanner::PopScanner()
{
  s_ScannerContextStack.pop();
}

static Token MatchChars(const std::unordered_map<char, TokenType>& pairs, TokenType defaultType)
{
  for (const auto& [c, type] : pairs) {
    if (Peek() == c) {
      Advance();
      return MakeToken(type);
    }
  }
  return MakeToken(defaultType);
}

Token Grace::Scanner::ScanToken()
{
  SkipWhitespace();
  s_ScannerContextStack.top().scannerStart = s_ScannerContextStack.top().scannerCurrent;

  auto c = Advance();

  if (IsAtEnd()) {
    return Token(TokenType::EndOfFile, 0, 0, s_ScannerContextStack.top().scannerLine - 1, s_ScannerContextStack.top().scannerColumn - 1, "");
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
      return MatchChars({ {'=', TokenType::BangEqual} }, TokenType::Bang);
    case '=':
      return MatchChars({ {'=', TokenType::EqualEqual} }, TokenType::Equal);
    case '<':
      return MatchChars({ {'=', TokenType::LessEqual}, {'<', TokenType::ShiftLeft} }, TokenType::LessThan);
    case '>':
      return MatchChars({ {'=', TokenType::GreaterEqual}, {'>', TokenType::ShiftRight} }, TokenType::GreaterThan);
    case '*':
      return MatchChars({ {'*', TokenType::StarStar} }, TokenType::Star);
    case '.':
      return MatchChars({ {'.', TokenType::DotDot} }, TokenType::Dot);
    case ':':
      return MatchChars({ {':', TokenType::ColonColon} }, TokenType::Colon);
    case '"':
      return MakeString();
    case '\'':
      return MakeChar();
    default:
      return ErrorToken(fmt::format("Unexpected character: {}", c));
  }
}

std::string Grace::Scanner::GetCodeAtLine(const std::string& fileName, std::size_t line)
{
  if (s_CodeStringsLookup.find(fileName) == s_CodeStringsLookup.end()) {
    return fmt::format("Couldn't find file `{}`\n", fileName);
  }

  const auto& code = s_CodeStringsLookup.at(fileName);
  std::size_t curr = 1;
  std::size_t strIndex = 0;
  while (curr < line) {
    if (code[strIndex++] == '\n') {
      curr++;
    }
  }

  std::string codeSubStr = code.substr(strIndex, code.length() - strIndex);
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
        s_ScannerContextStack.top().scannerColumn += 8;
      case ' ':
      case '\r':
        Advance();
        break;
      case '\n':
        s_ScannerContextStack.top().scannerLine++;
        s_ScannerContextStack.top().scannerColumn = 0;
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
              s_ScannerContextStack.top().scannerLine++;
              s_ScannerContextStack.top().scannerColumn = 0;
            } else if (Peek() == '\t') {
              s_ScannerContextStack.top().scannerCurrent += 8;
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

  s_ScannerContextStack.top().scannerCurrent++;
  s_ScannerContextStack.top().scannerColumn++;
  return s_ScannerContextStack.top().codeString[s_ScannerContextStack.top().scannerCurrent - 1];
}

static bool IsAtEnd()
{
  return s_ScannerContextStack.top().scannerCurrent >= s_ScannerContextStack.top().codeString.length();
}

static char Peek()
{
  return s_ScannerContextStack.top().codeString[s_ScannerContextStack.top().scannerCurrent];
}

static char PeekNext()
{
  if (IsAtEnd() || s_ScannerContextStack.top().scannerCurrent == s_ScannerContextStack.top().codeString.length() - 1) {
    return '\0';
  }

  return s_ScannerContextStack.top().codeString[s_ScannerContextStack.top().scannerCurrent + 1];
}

static char PeekPrevious()
{
  return s_ScannerContextStack.top().scannerCurrent == 0 ? '\0' : s_ScannerContextStack.top().codeString[s_ScannerContextStack.top().scannerCurrent - 1];
}

static Token ErrorToken(std::string&& message)
{
  return Token(TokenType::Error, s_ScannerContextStack.top().scannerLine, s_ScannerContextStack.top().scannerColumn, std::move(message));
}

static Token Identifier()
{
  while (!IsAtEnd() && (IsIdentifierChar(Peek()) || std::isdigit(Peek()))) {
    Advance();
  }

  std::string tokenStr = s_ScannerContextStack.top().codeString.substr(s_ScannerContextStack.top().scannerStart, s_ScannerContextStack.top().scannerCurrent - s_ScannerContextStack.top().scannerStart);
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
  auto length = s_ScannerContextStack.top().scannerCurrent - s_ScannerContextStack.top().scannerStart;
  return Token(type, s_ScannerContextStack.top().scannerStart, length, s_ScannerContextStack.top().scannerLine, s_ScannerContextStack.top().scannerColumn - 1, s_ScannerContextStack.top().codeString);
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
      s_ScannerContextStack.top().scannerLine++;
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
      s_ScannerContextStack.top().scannerLine++;
    }
    Advance();
  }

  if (IsAtEnd()) {
    return ErrorToken("Unterminated char");
  }

  Advance();
  return MakeToken(TokenType::Char);
}
