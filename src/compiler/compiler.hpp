/*
 *  The Grace Programming Language.
 *
 *  This file contains the Grace Compiler, which outputs Grace bytecode based on Tokens provided by the Scanner.
 *  
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#ifndef GRACE_COMPILER_HPP
#define GRACE_COMPILER_HPP

#include "../grace.hpp"
#include "../vm/vm.hpp"

#include <string>
#include <vector>

namespace Grace::Compiler
{
  /*
   *  Starts the compilation process.
   *
   *  @param fileName         Name of the file to be read
   *  @param code             The code to be compiled.
   *  @param verbose          Verbose mode (display compilation time and compiler warnings).
   *  @param warningsError    Display compiler warnings, warnings result in errors
   */
  GRACE_NODISCARD VM::InterpretResult Compile(std::string fileName, bool verbose, bool warningsError, std::vector<std::string> args);
} // namespace Grace::Compiler

#endif // ifndef GRACE_COMPILER_HPP
