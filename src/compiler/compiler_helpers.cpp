/*
 *  The Grace Programming Language.
 *
 *  This file contains out of line definitions of helper functions for the Grace Compiler.
 *  
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#include "compiler_helpers.hpp"

#include <charconv>
#include <unordered_map>

using namespace Grace;

GRACE_NODISCARD bool Compiler::IsKeyword(Scanner::TokenType type, std::string& outKeyword)
{
  switch (type) {
    case Scanner::TokenType::And: outKeyword = "and"; return true;
    case Scanner::TokenType::By: outKeyword = "by"; return true;
    case Scanner::TokenType::Catch: outKeyword = "catch"; return true;
    case Scanner::TokenType::Class: outKeyword = "class"; return true;
    case Scanner::TokenType::Const: outKeyword = "const"; return true;
    case Scanner::TokenType::Constructor: outKeyword = "constructor"; return true;
    case Scanner::TokenType::End: outKeyword = "end"; return true;
    case Scanner::TokenType::Final: outKeyword = "final"; return true;
    case Scanner::TokenType::For: outKeyword = "for"; return true;
    case Scanner::TokenType::Func: outKeyword = "func"; return true;
    case Scanner::TokenType::If: outKeyword = "if"; return true;
    case Scanner::TokenType::In: outKeyword = "in"; return true;
    case Scanner::TokenType::Or: outKeyword = "or"; return true;
    case Scanner::TokenType::Print: outKeyword = "print"; return true;
    case Scanner::TokenType::PrintLn: outKeyword = "println"; return true;
    case Scanner::TokenType::Eprint: outKeyword = "eprint"; return true;
    case Scanner::TokenType::EprintLn: outKeyword = "eprintln"; return true;
    case Scanner::TokenType::Export: outKeyword = "export"; return true;
    case Scanner::TokenType::Return: outKeyword = "return"; return true;
    case Scanner::TokenType::Throw: outKeyword = "throw"; return true;
    case Scanner::TokenType::This: outKeyword = "this"; return true;
    case Scanner::TokenType::Try: outKeyword = "try"; return true;
    case Scanner::TokenType::Var: outKeyword = "var"; return true;
    case Scanner::TokenType::While: outKeyword = "while"; return true;
    default:
      return false;
  }
}

GRACE_NODISCARD bool Compiler::IsOperator(Scanner::TokenType type)
{
  static const std::vector<Scanner::TokenType> symbols {
    Scanner::TokenType::Colon,
    Scanner::TokenType::Semicolon,
    Scanner::TokenType::RightParen,
    Scanner::TokenType::Comma,
    Scanner::TokenType::Dot,
    Scanner::TokenType::DotDot,
    Scanner::TokenType::Plus,
    Scanner::TokenType::Slash,
    Scanner::TokenType::Star,
    Scanner::TokenType::StarStar,
    Scanner::TokenType::BangEqual,
    Scanner::TokenType::Equal,
    Scanner::TokenType::EqualEqual,
    Scanner::TokenType::LessThan,
    Scanner::TokenType::GreaterThan,
    Scanner::TokenType::LessEqual,
    Scanner::TokenType::GreaterEqual,
    Scanner::TokenType::Bar,
    Scanner::TokenType::Ampersand,
    Scanner::TokenType::Caret,
    Scanner::TokenType::ShiftRight,
    Scanner::TokenType::ShiftLeft,
  };

  return std::any_of(symbols.begin(), symbols.end(), [type](Scanner::TokenType t) {
    return t == type;
  });
}

GRACE_NODISCARD bool Compiler::IsCompoundAssignment(Scanner::TokenType type)
{
  static const std::vector<Scanner::TokenType> compoundAssignOps = {
    Scanner::TokenType::PlusEquals,
    Scanner::TokenType::MinusEquals,
    Scanner::TokenType::StarEquals,
    Scanner::TokenType::SlashEquals,
    Scanner::TokenType::AmpersandEquals,
    Scanner::TokenType::CaretEquals,
    Scanner::TokenType::BarEquals,
    Scanner::TokenType::ModEquals,
    Scanner::TokenType::ShiftLeftEquals,
    Scanner::TokenType::ShiftRightEquals,
    Scanner::TokenType::StarStarEquals,
  };

  return std::any_of(compoundAssignOps.begin(), compoundAssignOps.end(), [type](Scanner::TokenType toCheck) {
    return toCheck == type;
  });
}

GRACE_NODISCARD bool Compiler::IsTypeIdent(Scanner::TokenType type)
{
  static const std::vector<Scanner::TokenType> typeIdents = {
    Scanner::TokenType::IntIdent,
    Scanner::TokenType::FloatIdent,
    Scanner::TokenType::BoolIdent,
    Scanner::TokenType::StringIdent,
    Scanner::TokenType::CharIdent,
    Scanner::TokenType::ListIdent,
    Scanner::TokenType::DictIdent,
    Scanner::TokenType::KeyValuePairIdent,
    Scanner::TokenType::SetIdent,
    Scanner::TokenType::ExceptionIdent,
    // Scanner::TokenType::RangeIdent,
  };

  return std::any_of(typeIdents.begin(), typeIdents.end(), [type](Scanner::TokenType t) {
    return t == type;
  });
}

GRACE_NODISCARD bool Compiler::IsValidTypeAnnotation(Scanner::TokenType type)
{
  static const std::vector<Scanner::TokenType> valid = {
    Scanner::TokenType::Identifier,
    Scanner::TokenType::IntIdent,
    Scanner::TokenType::FloatIdent,
    Scanner::TokenType::BoolIdent,
    Scanner::TokenType::CharIdent,
    Scanner::TokenType::Null,
    Scanner::TokenType::StringIdent,
    Scanner::TokenType::ListIdent,
    Scanner::TokenType::DictIdent,
    Scanner::TokenType::ExceptionIdent,
    Scanner::TokenType::KeyValuePairIdent,
    Scanner::TokenType::SetIdent,
  };

  return std::any_of(valid.begin(), valid.end(), [type](Scanner::TokenType t) {
    return t == type;
  });
}

GRACE_NODISCARD bool Compiler::IsEscapeChar(char c, char& outChar)
{
  static const char escapeChars[] = { 't', 'b', 'n', 'r', '\'', '"', '\\' };
  static const std::unordered_map<char, char> escapeCharsLookup = {
    { 't', '\t' },
    { 'b', '\b' },
    { 'r', '\r' },
    { 'n', '\n' },
    { '\'', '\'' },
    { '"', '\"' },
    { '\\', '\\' },
  };

  for (auto escapeChar : escapeChars) {
    if (c == escapeChar) {
      outChar = escapeCharsLookup.at(escapeChar);
      return true;
    }
  }
  return false;
}

GRACE_NODISCARD bool Compiler::IsLiteral(Scanner::TokenType type)
{
  static const std::vector<Scanner::TokenType> literalTypes {
    Scanner::TokenType::True,
    Scanner::TokenType::False,
    Scanner::TokenType::Integer,
    Scanner::TokenType::Double,
    Scanner::TokenType::String,
    Scanner::TokenType::Char
  };

  return std::any_of(literalTypes.begin(), literalTypes.end(), [type](Scanner::TokenType t) {
    return t == type;
  });
}

GRACE_NODISCARD bool Compiler::IsNumber(Scanner::TokenType type)
{
  return type== Scanner::TokenType::Integer || type== Scanner::TokenType::Double;
}

std::optional<std::string> Compiler::TryParseChar(const Scanner::Token& token, char& outChar)
{
  auto text = token.GetText();
  auto trimmed = text.substr(1, text.length() - 2);
  switch (trimmed.length()) {
    case 2:
      if (trimmed[0] != '\\') {
        return "`char` must contain a single character or escape character";
      }
      char c;
      if (IsEscapeChar(trimmed[1], c)) {
        outChar = c;
      } else {
        return "Unrecognised escape character";
      }
      return {};
    case 1:
      if (trimmed[0] == '\\') {
        return "Expected escape character after backslash";
      }
      outChar = trimmed[0];
      return {};
    default:
      return "`char` must contain a single character or escape character";
  }
}

std::optional<std::string> Compiler::TryParseString(const Scanner::Token& token, std::string& outString)
{
  auto text = token.GetText();
  std::string res;
  for (std::size_t i = 1; i < text.length() - 1; i++) {
    if (text[i] == '\\') {
      i++;
      if (i == text.length() - 1) {
        return "Expected escape character but string terminated";
      }
      char c;
      if (IsEscapeChar(text[i], c)) {
        res.push_back(c);
      } else {
        return fmt::format("Unrecognised escape character '{}'", text[i]);
      }
    } else {
      res.push_back(text[i]);
    }
  }

  outString = std::move(res);
  return {};
}

std::optional<std::string> Compiler::TryParseInt(const Scanner::Token& token, std::int64_t& result, int base, int offset)
{
  auto [ptr, ec] = std::from_chars(token.GetData() + offset, token.GetData() + token.GetLength(), result, base);
  if (ec == std::errc()) {
    return {};
  }
  if (ec == std::errc::invalid_argument) {
    return "Invalid argument";
  }
  if (ec == std::errc::result_out_of_range) {
    return "Out of range";
  }
  GRACE_ASSERT(false, "Unhandled std::errc returned from TryParseInt()");
  return "Unexpected error parsing int";
}

std::optional<std::string> Compiler::TryParseDouble(const Scanner::Token& token, double& result)
{
  try {
    result = std::stod(token.GetString());
    return std::nullopt;
  } catch (const std::invalid_argument& e) {
    return e.what();
  } catch (const std::out_of_range& e) {
    return e.what();
  }
}

std::size_t Compiler::GetEditDistance(const std::string& first, const std::string& second)
{
  auto l1 = first.length();
  auto l2 = second.length();

  if (l1 == 0) {
    return l2;
  }

  if (l2 == 0) {
    return l1;
  }

  std::vector<std::vector<std::size_t>> matrix(l1 + 1);
  for (auto& i : matrix) {
    i.resize(l2 + 1);
  }

  for (std::size_t i = 1; i <= l1; i++) {
    matrix[i][0] = i;
  }
  for (std::size_t j = 1; j <= l2; j++) {
    matrix[0][j] = j;
  }

  for (std::size_t i = 1; i <= l1; i++) {
    for (std::size_t j = 1; j <= l2; j++) {
      std::size_t weight = first[i - 1] == second[j - 1] ? 0 : 1;
      matrix[i][j] = std::min({ matrix[i - 1][j] + 1, matrix[i][j - 1] + 1, matrix[i - 1][j - 1] + weight });
    }
  }

  return matrix[l1][l2];
}