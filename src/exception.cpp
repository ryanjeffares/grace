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

#include "exception.hpp"

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
