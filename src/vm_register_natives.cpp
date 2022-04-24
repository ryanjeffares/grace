/*
 *  The Grace Programming Language.
 *
 *  This file contains the out of line definitions for the VM class, specifically registering native functions. 
 *
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#include <chrono>

#include "vm.hpp"

using namespace Grace::VM;

void VM::RegisterNatives()
{
#define REGISTER_NATIVE(name, arity, function)                            \
  do {                                                                    \
    m_NativeFunctions.emplace(static_cast<std::int64_t>(m_Hasher(name)),  \
        Native::NativeFunction(name, arity, function));                   \
  } while (false)                                                         \

  // Math functions
  REGISTER_NATIVE("__NATIVE_SQRT_FLOAT", 1, [](const std::vector<Value>& args) {
        return Value(std::sqrt(args[0].Get<double>()));
      });
  REGISTER_NATIVE("__NATIVE_SQRT_INT", 1, [](const std::vector<Value>& args) {
        return Value(std::sqrt(args[0].Get<std::int64_t>()));
      });

  // Time functions
#define GET_TIME(division) std::chrono::duration_cast<std::chrono::division>(std::chrono::steady_clock::now().time_since_epoch()).count()

  REGISTER_NATIVE("__NATIVE_TIME_S", 0, [](GRACE_UNUSED const std::vector<Value>& args) {
        return Value(GET_TIME(seconds));
      });
  REGISTER_NATIVE("__NATIVE_TIME_MS", 0, [](GRACE_UNUSED const std::vector<Value>& args) {
        return Value(GET_TIME(milliseconds));
      });
  REGISTER_NATIVE("__NATIVE_TIME_NS", 0, [](GRACE_UNUSED const std::vector<Value>& args) {
        return Value(GET_TIME(nanoseconds));
      });

#undef GET_TIME

#undef REGISTER_NATIVE
}
