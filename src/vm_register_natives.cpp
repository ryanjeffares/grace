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

#include "grace.hpp"
#include "vm.hpp"
#include "objects/grace_list.hpp"

using namespace Grace::VM;

static Value SqrtFloat(const std::vector<Value>& args);
static Value SqrtInt(const std::vector<Value>& args);

static Value TimeSeconds(GRACE_MAYBE_UNUSED const std::vector<Value>& args);
static Value TimeMilliSeconds(GRACE_MAYBE_UNUSED const std::vector<Value>& args);
static Value TimeNanoSeconds(GRACE_MAYBE_UNUSED const std::vector<Value>& args);

static Value AppendList(const std::vector<Value>& args);
static Value SetListAtIndex(const std::vector<Value>& args);
static Value GetListAtIndex(const std::vector<Value>& args);
static Value ListLength(const std::vector<Value>& args);

void VM::RegisterNatives()
{
  // Math functions
  m_NativeFunctions.emplace_back("__NATIVE_SQRT_FLOAT", 1, &SqrtFloat);
  m_NativeFunctions.emplace_back("__NATIVE_SQRT_INT", 1, &SqrtInt);

  // Time functions
  m_NativeFunctions.emplace_back("__NATIVE_TIME_S", 0, &TimeSeconds);
  m_NativeFunctions.emplace_back("__NATIVE_TIME_MS", 0, &TimeMilliSeconds);
  m_NativeFunctions.emplace_back("__NATIVE_TIME_NS", 0, &TimeNanoSeconds);

  // List functions
  m_NativeFunctions.emplace_back("__NATIVE_APPEND_LIST", 2, &AppendList);
  m_NativeFunctions.emplace_back("__NATIVE_SET_LIST_AT_INDEX", 3, &SetListAtIndex);
  m_NativeFunctions.emplace_back("__NATIVE_GET_LIST_AT_INDEX", 2, &GetListAtIndex);
  m_NativeFunctions.emplace_back("__NATIVE_LIST_LENGTH", 1, &ListLength);
}

static Value SqrtFloat(const std::vector<Value>& args)
{
  return Value(std::sqrt(args[0].Get<double>()));
}

static Value SqrtInt(const std::vector<Value>& args)
{
  return Value(std::sqrt(args[0].Get<std::int64_t>()));
}

static Value TimeSeconds(GRACE_MAYBE_UNUSED const std::vector<Value>& args)
{
  return Value(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

static Value TimeMilliSeconds(GRACE_MAYBE_UNUSED const std::vector<Value>& args)
{
  return Value(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

static Value TimeNanoSeconds(GRACE_MAYBE_UNUSED const std::vector<Value>& args)
{
  return Value(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

static Value AppendList(const std::vector<Value>& args)
{
  dynamic_cast<Grace::GraceList*>(args[0].GetObject())->Append(args[1]);
  return Value();
}

static Value SetListAtIndex(const std::vector<Value>& args)
{
  auto l = dynamic_cast<Grace::GraceList*>(args[0].GetObject());
  (*l)[args[1].Get<std::int64_t>()] = args[2];
  return Value();
}

static Value GetListAtIndex(const std::vector<Value>& args)
{
  return (*dynamic_cast<Grace::GraceList*>(args[0].GetObject()))[args[1].Get<std::int64_t>()];
}

static Value ListLength(const std::vector<Value>& args)
{
  return Value(static_cast<std::int64_t>(dynamic_cast<Grace::GraceList*>(args[0].GetObject())->Length()));
}
