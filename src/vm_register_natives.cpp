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
#include "objects/grace_dictionary.hpp"

using namespace Grace::VM;

static Value SqrtFloat(const std::vector<Value>& args);
static Value SqrtInt(const std::vector<Value>& args);

static Value TimeHours(GRACE_MAYBE_UNUSED const std::vector<Value>& args);
static Value TimeMinutes(GRACE_MAYBE_UNUSED const std::vector<Value>& args);
static Value TimeSeconds(GRACE_MAYBE_UNUSED const std::vector<Value>& args);
static Value TimeMilliSeconds(GRACE_MAYBE_UNUSED const std::vector<Value>& args);
static Value TimeMicroSeconds(GRACE_MAYBE_UNUSED const std::vector<Value>& args);
static Value TimeNanoSeconds(GRACE_MAYBE_UNUSED const std::vector<Value>& args);

static Value ListAppend(const std::vector<Value>& args);
static Value ListSetAtIndex(const std::vector<Value>& args);
static Value ListGetAtIndex(const std::vector<Value>& args);
static Value ListLength(const std::vector<Value>& args);

static Value DictionaryInsert(const std::vector<Value>& args);

void VM::RegisterNatives()
{
  // Math functions
  m_NativeFunctions.emplace_back("__NATIVE_SQRT_FLOAT", 1, &SqrtFloat);
  m_NativeFunctions.emplace_back("__NATIVE_SQRT_INT", 1, &SqrtInt);

  // Time functions
  m_NativeFunctions.emplace_back("__NATIVE_TIME_H", 0, &TimeHours);
  m_NativeFunctions.emplace_back("__NATIVE_TIME_M", 0, &TimeMinutes);
  m_NativeFunctions.emplace_back("__NATIVE_TIME_S", 0, &TimeSeconds);
  m_NativeFunctions.emplace_back("__NATIVE_TIME_MS", 0, &TimeMilliSeconds);
  m_NativeFunctions.emplace_back("__NATIVE_TIME_US", 0, &TimeMicroSeconds);
  m_NativeFunctions.emplace_back("__NATIVE_TIME_NS", 0, &TimeNanoSeconds);

  // List functions
  m_NativeFunctions.emplace_back("__NATIVE_APPEND_LIST", 2, &ListAppend);
  m_NativeFunctions.emplace_back("__NATIVE_SET_LIST_AT_INDEX", 3, &ListSetAtIndex);
  m_NativeFunctions.emplace_back("__NATIVE_GET_LIST_AT_INDEX", 2, &ListGetAtIndex);
  m_NativeFunctions.emplace_back("__NATIVE_LIST_LENGTH", 1, &ListLength);

  // Dictionary functions
  m_NativeFunctions.emplace_back("__NATIVE_DICTIONARY_INSERT", 3, &DictionaryInsert);
}

static Value SqrtFloat(const std::vector<Value>& args)
{
  return Value(std::sqrt(args[0].Get<double>()));
}

static Value SqrtInt(const std::vector<Value>& args)
{
  return Value(std::sqrt(args[0].Get<std::int64_t>()));
}

static Value TimeHours(GRACE_MAYBE_UNUSED const std::vector<Value>& args)
{
  return Value(static_cast<std::int64_t>(std::chrono::duration_cast<std::chrono::hours>(std::chrono::steady_clock::now().time_since_epoch()).count()));
}

static Value TimeMinutes(GRACE_MAYBE_UNUSED const std::vector<Value>& args)
{
  return Value(static_cast<std::int64_t>(std::chrono::duration_cast<std::chrono::minutes>(std::chrono::steady_clock::now().time_since_epoch()).count()));
}

static Value TimeSeconds(GRACE_MAYBE_UNUSED const std::vector<Value>& args)
{
  return Value(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

static Value TimeMilliSeconds(GRACE_MAYBE_UNUSED const std::vector<Value>& args)
{
  return Value(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

static Value TimeMicroSeconds(GRACE_MAYBE_UNUSED const std::vector<Value>& args)
{
  return Value(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

static Value TimeNanoSeconds(GRACE_MAYBE_UNUSED const std::vector<Value>& args)
{
  return Value(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

static Value ListAppend(const std::vector<Value>& args)
{
  dynamic_cast<Grace::GraceList*>(args[0].GetObject())->Append(args[1]);
  return Value();
}

static Value ListSetAtIndex(const std::vector<Value>& args)
{
  auto l = dynamic_cast<Grace::GraceList*>(args[0].GetObject());
  (*l)[args[1].Get<std::int64_t>()] = args[2];
  return Value();
}

static Value ListGetAtIndex(const std::vector<Value>& args)
{
  return (*dynamic_cast<Grace::GraceList*>(args[0].GetObject()))[args[1].Get<std::int64_t>()];
}

static Value ListLength(const std::vector<Value>& args)
{
  return Value(static_cast<std::int64_t>(dynamic_cast<Grace::GraceList*>(args[0].GetObject())->Length()));
}

static Value DictionaryInsert(const std::vector<Value>& args)
{
  auto dict = dynamic_cast<Grace::GraceDictionary*>(args[0].GetObject());
  auto key = args[1];
  auto value = args[2];
  dict->Insert(std::move(key), std::move(value));
  return Value();
}
