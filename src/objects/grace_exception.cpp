/*
 *  The Grace Programming Language.
 *
 *  This file contains the out of line definitions for the GraceException class, used for reporting runtime errors in Grace.
 *    
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#include <fmt/core.h>

#include "grace_exception.hpp"

using namespace Grace::VM;

std::unordered_map<GraceException::Type, const char*> GraceException::s_ExceptionMessages = {
  {GraceException::Type::AssertionFailed, "Assertion failed"},
  {GraceException::Type::FunctionNotFound, "Function not found"},
  {GraceException::Type::IncorrectArgCount, "Incorrect argument count"},
  {GraceException::Type::InvalidArgument, "Invalid argument"},
  {GraceException::Type::InvalidCast, "Invalid cast"},
  {GraceException::Type::InvalidOperand, "Invalid operand"},
  {GraceException::Type::InvalidType, "Invalid type"},
  {GraceException::Type::ThrownException, "Thrown exception"},
};

void GraceException::DebugPrint() const
{
  fmt::print("GraceException: {}: {}\n", what(), m_Message);
}

void GraceException::Print() const
{
  fmt::print("{}: {}", what(), m_Message);
}

void GraceException::PrintLn() const
{
  fmt::print("{}: {}\n", what(), m_Message);
}

std::string GraceException::ToString() const
{
  return fmt::format("{}: {}", what(), m_Message);
}

bool GraceException::AsBool() const
{
  return true;
}