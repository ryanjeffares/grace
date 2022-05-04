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
#include "objects/grace_list.hpp"

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

  REGISTER_NATIVE("__NATIVE_TIME_S", 0, [](GRACE_MAYBE_UNUSED const std::vector<Value>& args) {
        return Value(GET_TIME(seconds));
      });
  REGISTER_NATIVE("__NATIVE_TIME_MS", 0, [](GRACE_MAYBE_UNUSED const std::vector<Value>& args) {
        return Value(GET_TIME(milliseconds));
      });
  REGISTER_NATIVE("__NATIVE_TIME_NS", 0, [](GRACE_MAYBE_UNUSED const std::vector<Value>& args) {
        return Value(GET_TIME(nanoseconds));
      });

#undef GET_TIME

  // List functions
  REGISTER_NATIVE("__NATIVE_APPEND_LIST", 2, [](const std::vector<Value>& args) {
        dynamic_cast<GraceList*>(args[0].GetObject())->Append(args[1]);
        return Value();
      });
  REGISTER_NATIVE("__NATIVE_SET_LIST_AT_INDEX", 3, [](const std::vector<Value>& args) {
        auto l = dynamic_cast<GraceList*>(args[0].GetObject());
        (*l)[args[1].Get<std::int64_t>()] = args[2];
        return Value();
      });
  REGISTER_NATIVE("__NATIVE_GET_LIST_AT_INDEX", 2, [](const std::vector<Value>& args) {
        return (*dynamic_cast<GraceList*>(args[0].GetObject()))[args[1].Get<std::int64_t>()];
      });
  REGISTER_NATIVE("__NATIVE_LIST_LENGTH", 1, [](const std::vector<Value>& args) {
        return Value(static_cast<std::int64_t>(dynamic_cast<GraceList*>(args[0].GetObject())->Length()));
      });

#undef REGISTER_NATIVE
}
