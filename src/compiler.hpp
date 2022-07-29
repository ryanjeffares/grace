/*
 *  The Grace Programming Language.
 *
 *  This file contains the Compiler class, which outputs Grace bytecode based on Tokens provided by the Scanner. 
 *  
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#ifndef GRACE_COMPILER_HPP
#define GRACE_COMPILER_HPP

#include <string>
#include <vector>

#include "grace.hpp"
#include "vm.hpp"

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
  GRACE_NODISCARD VM::InterpretResult Compile(const std::string& fileName, std::string&& code, bool verbose, bool warningsError, const std::vector<std::string>& args);
} // namespace Grace::Compiler

#endif  // ifndef GRACE_COMPILER_HPP
