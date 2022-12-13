/*
 *  The Grace Programming Language.
 *
 *  This file contains declarations of helper functions for the Grace Compiler.
 *  
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#ifndef GRACE_COMPILER_HELPERS_HPP
#define GRACE_COMPILER_HELPERS_HPP

#include "../scanner/scanner.hpp"

#include <optional>

namespace Grace::Compiler
{
  GRACE_NODISCARD bool IsKeyword(Scanner::TokenType type, std::string& outKeyword);
  GRACE_NODISCARD bool IsOperator(Scanner::TokenType type);
  GRACE_NODISCARD bool IsCompoundAssignment(Scanner::TokenType type);
  GRACE_NODISCARD bool IsTypeIdent(Scanner::TokenType type);
  GRACE_NODISCARD bool IsValidTypeAnnotation(Scanner::TokenType type);
  GRACE_NODISCARD bool IsEscapeChar(char c, char& outChar);
  GRACE_NODISCARD bool IsLiteral(Scanner::TokenType type);
  GRACE_NODISCARD bool IsNumber(Scanner::TokenType type);

  std::optional<std::string> TryParseChar(const Scanner::Token& token, char& outChar);
  std::optional<std::string> TryParseString(const Scanner::Token& token, std::string& outChar);
  std::optional<std::string> TryParseInt(const Scanner::Token& token, std::int64_t& result, int base = 10, int offset = 0);
  std::optional<std::string> TryParseDouble(const Scanner::Token& token, double& result);

  std::size_t GetEditDistance(const std::string& first, const std::string& second);
}

#endif