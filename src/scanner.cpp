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
#include "value.hpp"

namespace Grace::Scanner
{
  static std::unordered_map<char, TokenType> s_SymbolLookup =
  {
    {';', TokenType::Semicolon},
    {'(', TokenType::LeftParen},
    {')', TokenType::RightParen},
    {'[', TokenType::LeftSquareParen},
    {']', TokenType::RightSquareParen},
    {'{', TokenType::LeftCurlyParen},
    {'}', TokenType::RightCurlyParen},
    {',', TokenType::Comma},
    {'~', TokenType::Tilde},
  };

  static std::unordered_map<std::string, TokenType> s_KeywordLookup =
  {
    {"assert", TokenType::Assert},
    {"and", TokenType::And},
    {"or", TokenType::Or},
    {"break", TokenType::Break},
    {"by", TokenType::By},
    {"class", TokenType::Class},
    {"catch", TokenType::Catch},
    {"const", TokenType::Const},
    {"constructor", TokenType::Constructor},
    {"continue", TokenType::Continue},
    {"end", TokenType::End},
    {"else", TokenType::Else},
    {"false", TokenType::False},
    {"final", TokenType::Final},
    {"for", TokenType::For},
    {"func", TokenType::Func},
    {"if", TokenType::If},
    {"import", TokenType::Import},
    {"in", TokenType::In},
    {"instanceof", TokenType::InstanceOf},
    {"isobject", TokenType::IsObject},
    {"null", TokenType::Null},
    {"print", TokenType::Print},
    {"println", TokenType::PrintLn},
    {"eprint", TokenType::Eprint},
    {"eprintln", TokenType::EprintLn},
    {"export", TokenType::Export},
    {"return", TokenType::Return},
    {"while", TokenType::While},
    {"this", TokenType::This},
    {"throw", TokenType::Throw},
    {"true", TokenType::True},
    {"try", TokenType::Try},
    {"typename", TokenType::Typename},
    {"var", TokenType::Var},
    {"Int", TokenType::IntIdent},
    {"Float", TokenType::FloatIdent},
    {"Bool", TokenType::BoolIdent},
    {"String", TokenType::StringIdent},
    {"Char", TokenType::CharIdent},
    {"List", TokenType::ListIdent},
    {"Dict", TokenType::DictIdent},
    {"Exception", TokenType::ExceptionIdent},
    {"KeyValuePair", TokenType::KeyValuePairIdent},
    {"Set", TokenType::SetIdent},
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

    explicit ScannerContext(std::string&& code)
      : codeString{ std::move(code) }
    {

    }
  };

  static std::stack<ScannerContext> s_ScannerContextStack;
  static std::unordered_map<std::string, std::string> s_CodeStringsLookup;

  bool HasFile(const std::string& fullPath)
  {
    return s_CodeStringsLookup.find(fullPath) != s_CodeStringsLookup.end();
  }

  void InitScanner(const std::string& fullPath, std::string&& code)
  {
    s_CodeStringsLookup.try_emplace(fullPath, code);
    s_ScannerContextStack.emplace(std::move(code));
  }

  void PopScanner()
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

  Token ScanToken()
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

    switch (c) {
      case '!':
        return MatchChars({ {'=', TokenType::BangEqual} }, TokenType::Bang);
      case '=':
        return MatchChars({ {'=', TokenType::EqualEqual} }, TokenType::Equal);
      case '<':
        if (Peek() == '<' && PeekNext() == '=') {
          Advance();
          Advance();
          return MakeToken(TokenType::ShiftLeftEquals);
        }
        return MatchChars({ {'=', TokenType::LessEqual}, {'<', TokenType::ShiftLeft} }, TokenType::LessThan);
      case '>':
        if (Peek() == '>' && PeekNext() == '=') {
          Advance();
          Advance();
          return MakeToken(TokenType::ShiftRightEquals);
        }
        return MatchChars({ {'=', TokenType::GreaterEqual}, {'>', TokenType::ShiftRight} }, TokenType::GreaterThan);
      case '+':
        return MatchChars({ {'=', TokenType::PlusEquals} }, TokenType::Plus);
      case '-':
        return MatchChars({ {'=', TokenType::MinusEquals} }, TokenType::Minus);
      case '*':
        if (Peek() == '*' && PeekNext() == '=') {
          Advance();
          Advance();
          return MakeToken(TokenType::StarStarEquals);
        }
        return MatchChars({ {'*', TokenType::StarStar}, {'=', TokenType::StarEquals} }, TokenType::Star);
      case '/':
        return MatchChars({ {'=', TokenType::SlashEquals} }, TokenType::Slash);
      case '&':
        return MatchChars({ {'=', TokenType::AmpersandEquals} }, TokenType::Ampersand);
      case '|':
        return MatchChars({ {'=', TokenType::BarEquals} }, TokenType::Bar);
      case '^':
        return MatchChars({ {'=', TokenType::CaretEquals} }, TokenType::Caret);
      case '%':
        return MatchChars({ {'=', TokenType::ModEquals} }, TokenType::Mod);
      case '.':
        return MatchChars({ {'.', TokenType::DotDot} }, TokenType::Dot);
      case ':':
        return MatchChars({ {':', TokenType::ColonColon} }, TokenType::Colon);
      case '"':
        return MakeString();
      case '\'':
        return MakeChar();
      default:
        if (s_SymbolLookup.find(c) != s_SymbolLookup.end()) {
          return MakeToken(s_SymbolLookup[c]);
        }
        return ErrorToken(fmt::format("Unexpected character: {}", c));
    }
  }

  std::string GetCodeAtLine(const std::string& fileName, std::size_t line)
  {
    if (s_CodeStringsLookup.find(fileName) == s_CodeStringsLookup.end()) {
      return fmt::format("Couldn't find file `{}`\n", fileName);
    }

    const auto& code = s_CodeStringsLookup.at(fileName);
    std::size_t curr = 1;
    std::size_t strIndex = 0;
    while (curr < line) {
      if (strIndex >= code.length()) {
        return "";
      }
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
          s_ScannerContextStack.top().scannerCurrent++;
          break;
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

  static Token BinaryLiteral()
  {
    while (!IsAtEnd() && (Peek() == '0' || Peek() == '1')) {
      Advance();
    }

    return MakeToken(TokenType::BinaryLiteral);
  }

  static bool IsHexCharacter(char c)
  {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
  }

  static Token HexLiteral()
  {
    while (!IsAtEnd() && IsHexCharacter(Peek())) {
      Advance();
    }

    return MakeToken(TokenType::HexLiteral);
  }

  static Token Number()
  {
    if (Peek() == 'b' || Peek() == 'B') {
      Advance();
      return BinaryLiteral();
    }
    if (Peek() == 'x' || Peek() == 'X') {
      Advance();
      return HexLiteral();
    }

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
}
