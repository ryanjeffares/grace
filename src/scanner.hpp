/*
 *  The Grace Programming Language.
 *
 *  This file contains the Scanner class, which produces Tokens based on inputted source code. 
 *  
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#ifndef GRACE_SCANNER_HPP
#define GRACE_SCANNER_HPP

#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>

#include "grace.hpp"

namespace Grace::Scanner
{
  enum class TokenType
  {
    // Lexical tokens
    Char,
    EndOfFile,
    Error,
    Double,
    Identifier,
    Integer,
    String,
    IntIdent,
    FloatIdent,
    BoolIdent,
    StringIdent,
    CharIdent,
    ListIdent,
    DictIdent,

    // Symbols
    Colon,
    ColonColon,
    Semicolon,
    LeftParen,
    RightParen,
    LeftSquareParen,
    RightSquareParen,
    LeftCurlyParen,
    RightCurlyParen,
    Comma,
    Dot,
    DotDot,
    Minus,
    Mod,
    Plus,
    Slash,
    Star,
    StarStar,
    Bang,
    BangEqual,
    Equal,
    EqualEqual,
    LessThan,
    GreaterThan,
    LessEqual,
    GreaterEqual,
    ShiftLeft,
    ShiftRight,
    Bar,
    Tilde,
    Caret,
    Ampersand,

    // Keywords
    And,
    Assert,
    Break,
    By,
    Catch,
    Class,
    Continue,
    Else,
    End,
    Eprint,
    EprintLn,
    False,
    Final,
    For,
    Func,
    If,
    Import,
    In,
    InstanceOf,
    Null,
    Or,
    Print,
    PrintLn,
    Return,
    This,
    Throw,
    True,
    Try,
    Typename,
    Var,
    While,
  };

  class Token
  {
    public:

      Token(TokenType,
          std::size_t start,
          std::size_t length,
          std::size_t line,
          std::size_t column,
          const std::string& code
      );

      Token(TokenType, std::size_t line, std::size_t column, std::string&& errorMessage);

      GRACE_NODISCARD std::string ToString() const;
      GRACE_NODISCARD GRACE_INLINE TokenType GetType() const { return m_Type; }
      GRACE_NODISCARD GRACE_INLINE std::size_t GetLine() const { return m_Line; }
      GRACE_NODISCARD GRACE_INLINE std::size_t GetColumn() const { return m_Column; }
      GRACE_NODISCARD GRACE_INLINE std::string GetErrorMessage() const { return m_ErrorMessage; }
      GRACE_NODISCARD GRACE_INLINE std::size_t GetLength() const { return m_Length; }
      GRACE_NODISCARD GRACE_INLINE std::string_view GetText() const { return m_Text.substr(0, m_Length); }
      GRACE_NODISCARD const char* GetData() const { return m_Text.data(); }

    private:

      TokenType m_Type;
      std::size_t m_Start, m_Length;
      std::size_t m_Line, m_Column;
      std::string_view m_Text;
      std::string m_ErrorMessage;
  };

  void InitScanner(const std::string& fileName, std::string&& code);
  void PopScanner();
  Token ScanToken();
  std::string GetCodeAtLine(const std::string& fileName, std::size_t line);
} // namespace Grace::Scanner

template<>
struct fmt::formatter<Grace::Scanner::TokenType> : fmt::formatter<std::string_view>
{
  template<typename FormatContext>
  constexpr auto format(Grace::Scanner::TokenType type, FormatContext& context) -> decltype(context.out())
  {
    using namespace Grace::Scanner;

    std::string_view name = "unknown";
    switch (type) {
      case TokenType::Char: name = "TokenType::Char"; break;
      case TokenType::EndOfFile: name = "TokenType::EndOfFile"; break;
      case TokenType::Error: name = "TokenType::Error"; break;
      case TokenType::Double: name = "TokenType::Float"; break;
      case TokenType::Identifier: name = "TokenType::Identifier"; break;
      case TokenType::Integer: name = "TokenType::Integer"; break;
      case TokenType::String: name = "TokenType::String"; break;
      case TokenType::Colon: name = "TokenType::Colon"; break;
      case TokenType::ColonColon: name = "TokenType::ColonColon"; break;
      case TokenType::Semicolon: name = "TokenType::Semicolon"; break;
      case TokenType::LeftParen: name = "TokenType::LeftParen"; break;
      case TokenType::RightParen: name = "TokenType::RightParen"; break;
      case TokenType::LeftSquareParen: name = "TokenType::LeftSquareParen"; break;
      case TokenType::RightSquareParen: name = "TokenType::RightSquareParen"; break;
      case TokenType::LeftCurlyParen: name = "TokenType::LeftCurlyParen"; break;
      case TokenType::RightCurlyParen: name = "TokenType::RightCurlyParen"; break;
      case TokenType::Comma: name = "TokenType::Comma"; break;
      case TokenType::Dot: name = "TokenType::Dot"; break;
      case TokenType::DotDot: name = "TokenType::DotDot"; break;
      case TokenType::Minus: name = "TokenType::Minus"; break;
      case TokenType::Mod: name = "TokenType::Mod"; break;
      case TokenType::Plus: name = "TokenType::Plus"; break;
      case TokenType::Slash: name = "TokenType::Slash"; break;
      case TokenType::Star: name = "TokenType::Star"; break;
      case TokenType::StarStar: name = "TokenType::StarStar"; break;
      case TokenType::Bang: name = "TokenType::Bang"; break;
      case TokenType::BangEqual: name = "TokenType::BangEqual"; break;
      case TokenType::Equal: name = "TokenType::Equal"; break;
      case TokenType::EqualEqual: name = "TokenType::EqualEqual"; break;
      case TokenType::LessThan: name = "TokenType::LessThan"; break;
      case TokenType::GreaterThan: name = "TokenType::GreaterThan"; break;
      case TokenType::LessEqual: name = "TokenType::LessEqual"; break;
      case TokenType::GreaterEqual: name = "TokenType::GreaterEqual"; break;
      case TokenType::Assert: name = "TokenType::Assert"; break;
      case TokenType::Break: name = "TokenType::Break"; break;
      case TokenType::By: name = "TokenType::By"; break;
      case TokenType::Catch: name = "TokenType::Catch"; break;
      case TokenType::Class: name = "TokenType::Class"; break;
      case TokenType::Continue: name = "TokenType::Continue"; break;
      case TokenType::End: name = "TokenType::End"; break;
      case TokenType::Else: name = "TokenType::Else"; break;
      case TokenType::False: name = "TokenType::False"; break;
      case TokenType::Final: name = "TokenType::Final"; break;
      case TokenType::For: name = "TokenType::For"; break;
      case TokenType::Func: name = "TokenType::Func"; break;
      case TokenType::If: name = "TokenType::If"; break;
      case TokenType::Import: name = "TokenType::Import"; break;
      case TokenType::In: name = "TokenType::In"; break;
      case TokenType::InstanceOf: name = "TokenType::InstanceOf"; break;
      case TokenType::Null: name = "TokenType::Null"; break;
      case TokenType::While: name = "TokenType::While"; break;
      case TokenType::Print: name = "TokenType::Print"; break;
      case TokenType::PrintLn: name = "TokenType::PrintLn"; break;
      case TokenType::Eprint: name = "TokenType::Eprint"; break;
      case TokenType::EprintLn: name = "TokenType::EprintLn"; break;
      case TokenType::Return: name = "TokenType::Return"; break;
      case TokenType::This: name = "TokenType::This"; break;
      case TokenType::Throw: name = "TokenType::Throw"; break;
      case TokenType::True: name = "TokenType::True"; break;
      case TokenType::Try: name = "TokenType::Try"; break;
      case TokenType::Typename: name = "TokenType::Typename"; break;
      case TokenType::Var: name = "TokenType::Var"; break;
      case TokenType::Or: name = "TokenType::Or"; break;
      case TokenType::And: name = "TokenType::And"; break;
      case TokenType::IntIdent: name = "TokenType::IntIdent"; break;
      case TokenType::FloatIdent: name = "TokenType::FloatIdent"; break;
      case TokenType::BoolIdent: name = "TokenType::BoolIdent"; break;
      case TokenType::StringIdent: name = "TokenType::StringIdent"; break;
      case TokenType::CharIdent: name = "TokenType::CharIdent"; break;
      case TokenType::ListIdent: name = "TokenType::ListIdent"; break;
      case TokenType::DictIdent: name = "TokenType::DictIdent"; break;
      case TokenType::ShiftLeft: name = "TokenType::ShiftLeft"; break;
      case TokenType::ShiftRight: name = "TokenType::ShiftRight"; break;
      case TokenType::Bar: name = "TokenType::Bar"; break;
      case TokenType::Tilde: name = "TokenType::Tilde"; break;
      case TokenType::Caret: name = "TokenType::Caret"; break;
      case TokenType::Ampersand: name = "TokenType::Ampersand"; break;
    }
    return fmt::formatter<std::string_view>::format(name, context);
  }
};

#endif  // ifndef GRACE_SCANNER_HPP
