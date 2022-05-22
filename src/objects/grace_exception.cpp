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
  {GraceException::Type::InvalidOperand, "Invalid operaned"},
  {GraceException::Type::InvalidType, "Invalid type"},
};

void GraceException::DebugPrint() const
{
  fmt::print("GraceException: {}: {}", m_Type, m_Message);
}

void GraceException::Print() const
{
  fmt::print("{}: {}", m_Type, m_Message);
}

void GraceException::PrintLn() const
{
  fmt::print("{}: {}\n", m_Type, m_Message);
}

std::string GraceException::ToString() const
{
  return fmt::format("{}: {}", m_Type, m_Message);
}

bool GraceException::AsBool() const
{
  return true;
}