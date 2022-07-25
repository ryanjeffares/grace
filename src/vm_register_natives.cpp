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
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <thread>

#include "grace.hpp"
#include "vm.hpp"
#include "objects/grace_dictionary.hpp"
#include "objects/grace_exception.hpp"
#include "objects/grace_list.hpp"

using namespace Grace::VM;

static Value SqrtFloat(std::vector<Value>& args);
static Value SqrtInt(std::vector<Value>& args);

static Value TimeHours(GRACE_MAYBE_UNUSED std::vector<Value>& args);
static Value TimeMinutes(GRACE_MAYBE_UNUSED std::vector<Value>& args);
static Value TimeSeconds(GRACE_MAYBE_UNUSED std::vector<Value>& args);
static Value TimeMilliSeconds(GRACE_MAYBE_UNUSED std::vector<Value>& args);
static Value TimeMicroSeconds(GRACE_MAYBE_UNUSED std::vector<Value>& args);
static Value TimeNanoSeconds(GRACE_MAYBE_UNUSED std::vector<Value>& args);
static Value Sleep(std::vector<Value>& args);

static Value ListAppend(std::vector<Value>& args);
static Value ListSetAtIndex(std::vector<Value>& args);
static Value ListGetAtIndex(std::vector<Value>& args);
static Value ListLength(std::vector<Value>& args);

static Value DictionaryInsert(std::vector<Value>& args);
static Value DictionaryGet(std::vector<Value>& args);
static Value DictionaryContainsKey(std::vector<Value>& args);

static Value FileWrite(std::vector<Value>& args);

static Value FlushStdout(GRACE_MAYBE_UNUSED std::vector<Value>& args);
static Value FlushStderr(GRACE_MAYBE_UNUSED std::vector<Value>& args);

static Value SystemExit(std::vector<Value>& args);
static Value SystemRun(std::vector<Value>& args);

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
  m_NativeFunctions.emplace_back("__NATIVE_TIME_SLEEP", 1, &Sleep);

  // List functions
  m_NativeFunctions.emplace_back("__NATIVE_APPEND_LIST", 2, &ListAppend);
  m_NativeFunctions.emplace_back("__NATIVE_SET_LIST_AT_INDEX", 3, &ListSetAtIndex);
  m_NativeFunctions.emplace_back("__NATIVE_GET_LIST_AT_INDEX", 2, &ListGetAtIndex);
  m_NativeFunctions.emplace_back("__NATIVE_LIST_LENGTH", 1, &ListLength);

  // Dictionary functions
  m_NativeFunctions.emplace_back("__NATIVE_DICTIONARY_INSERT", 3, &DictionaryInsert);
  m_NativeFunctions.emplace_back("__NATIVE_DICTIONARY_GET", 2, &DictionaryGet);
  m_NativeFunctions.emplace_back("__NATIVE_DICTIONARY_CONTAINS_KEY", 2, &DictionaryContainsKey);

  // File functions
  m_NativeFunctions.emplace_back("__NATIVE_FILE_WRITE", 2, &FileWrite);

  // Console IO functions
  m_NativeFunctions.emplace_back("__NATIVE_FLUSH_STDOUT", 0, &FlushStdout);
  m_NativeFunctions.emplace_back("__NATIVE_FLUSH_STDERR", 0, &FlushStderr);

  // System functions
  m_NativeFunctions.emplace_back("__NATIVE_SYSTEM_EXIT", 1, &SystemExit);
  m_NativeFunctions.emplace_back("__NATIVE_SYSTEM_RUN", 1, &SystemRun);
}

static Value SqrtFloat(std::vector<Value>& args)
{
  return Value(std::sqrt(args[0].Get<double>()));
}

static Value SqrtInt(std::vector<Value>& args)
{
  return Value(std::sqrt(args[0].Get<std::int64_t>()));
}

static Value TimeHours(GRACE_MAYBE_UNUSED std::vector<Value>& args)
{
  return Value(static_cast<std::int64_t>(std::chrono::duration_cast<std::chrono::hours>(std::chrono::steady_clock::now().time_since_epoch()).count()));
}

static Value TimeMinutes(GRACE_MAYBE_UNUSED std::vector<Value>& args)
{
  return Value(static_cast<std::int64_t>(std::chrono::duration_cast<std::chrono::minutes>(std::chrono::steady_clock::now().time_since_epoch()).count()));
}

static Value TimeSeconds(GRACE_MAYBE_UNUSED std::vector<Value>& args)
{
  return Value(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

static Value TimeMilliSeconds(GRACE_MAYBE_UNUSED std::vector<Value>& args)
{
  return Value(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

static Value TimeMicroSeconds(GRACE_MAYBE_UNUSED std::vector<Value>& args)
{
  return Value(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

static Value TimeNanoSeconds(GRACE_MAYBE_UNUSED std::vector<Value>& args)
{
  return Value(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

static Value Sleep(std::vector<Value>& args)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(args[0].Get<std::int64_t>()));
  return Value();
}

static Value ListAppend(std::vector<Value>& args)
{
  dynamic_cast<Grace::GraceList*>(args[0].GetObject())->Append(std::move(args[1]));
  return Value();
}

static Value ListSetAtIndex(std::vector<Value>& args)
{
  auto l = dynamic_cast<Grace::GraceList*>(args[0].GetObject());
  (*l)[args[1].Get<std::int64_t>()] = std::move(args[2]);
  return Value();
}

static Value ListGetAtIndex(std::vector<Value>& args)
{
  return (*dynamic_cast<Grace::GraceList*>(args[0].GetObject()))[args[1].Get<std::int64_t>()];
}

static Value ListLength(std::vector<Value>& args)
{
  return Value(static_cast<std::int64_t>(dynamic_cast<Grace::GraceList*>(args[0].GetObject())->Length()));
}

static Value DictionaryInsert(std::vector<Value>& args)
{
  auto dict = dynamic_cast<Grace::GraceDictionary*>(args[0].GetObject());
  return Value(dict->Insert(std::move(args[1]), std::move(args[2])));
}

static Value DictionaryGet(std::vector<Value>& args)
{
  auto dict = dynamic_cast<Grace::GraceDictionary*>(args[0].GetObject());
  return dict->Get(args[1]);
}

static Value DictionaryContainsKey(std::vector<Value>& args)
{
  auto dict = dynamic_cast<Grace::GraceDictionary*>(args[0].GetObject());
  return Value(dict->ContainsKey(args[1]));
}

static Value FileWrite(std::vector<Value>& args)
{
  auto& path = args[0];
  auto& text = args[1];

  std::ofstream outStream(path.AsString());
  outStream << text.AsString();

  if (outStream.fail()) {
    throw Grace::GraceException(
      Grace::GraceException::Type::FileWriteFailed,
      fmt::format("failed to write to {}", path)
    );
  }

  return Value();
}

static Value FlushStdout(GRACE_MAYBE_UNUSED std::vector<Value>& args)
{
  std::fflush(stdout);
  return Value();
}

static Value FlushStderr(GRACE_MAYBE_UNUSED std::vector<Value>& args)
{
  std::fflush(stderr);
  return Value();
}

static Value SystemExit(std::vector<Value>& args)
{
  std::exit(static_cast<int>(args[0].Get<std::int64_t>()));
}

static Value SystemRun(std::vector<Value>& args)
{
  auto res = std::system(args[0].Get<std::string>().c_str());
  return Value(std::int64_t(res));
}
