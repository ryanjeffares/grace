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
#include <filesystem>
#include <fstream>
#include <thread>

#include <dyncall.h>
#include <dynload.h>

#include "grace.hpp"
#include "vm.hpp"
#include "objects/grace_dictionary.hpp"
#include "objects/grace_set.hpp"
#include "objects/grace_exception.hpp"
#include "objects/grace_instance.hpp"
#include "objects/grace_list.hpp"
#include "objects/object_tracker.hpp"

using namespace Grace::VM;

using Args = std::vector<Value>&;

static Value SqrtFloat(Args args);
static Value SqrtInt(Args args);

static Value TimeHours(GRACE_MAYBE_UNUSED Args args);
static Value TimeMinutes(GRACE_MAYBE_UNUSED Args args);
static Value TimeSeconds(GRACE_MAYBE_UNUSED Args args);
static Value TimeMilliSeconds(GRACE_MAYBE_UNUSED Args args);
static Value TimeMicroSeconds(GRACE_MAYBE_UNUSED Args args);
static Value TimeNanoSeconds(GRACE_MAYBE_UNUSED Args args);
static Value Sleep(Args args);

static Value ListAppend(Args args);
static Value ListInsert(Args args);
static Value ListRemove(Args args);
static Value ListPop(Args args);
static Value ListSetAtIndex(Args args);
static Value ListGetAtIndex(Args args);
static Value ListLength(Args args);
static Value ListSort(Args args);
static Value ListSortDescending(Args args);
static Value ListSorted(Args args);
static Value ListSortedDescending(Args args);
static Value ListFirst(Args args);
static Value ListLast(Args args);

static Value DictionaryInsert(Args args);
static Value DictionaryGet(Args args);
static Value DictionaryContainsKey(Args args);
static Value DictionaryRemove(Args args);

static Value KeyValuePairKey(Args args);
static Value KeyValuePairValue(Args args);

static Value SetAdd(Args args);
static Value SetContains(Args args);
static Value SetSize(Args args);

static Value FileWrite(Args args);
static Value FileReadAllText(Args args);
static Value FileReadAllLines(Args args);

static Value FlushStdout(GRACE_MAYBE_UNUSED Args args);
static Value FlushStderr(GRACE_MAYBE_UNUSED Args args);

static Value SystemExit(Args args);
static Value SystemRun(Args args);
static Value SystemPlatform(GRACE_MAYBE_UNUSED Args args);

static Value DirectoryExists(Args args);
static Value DirectoryCreate(Args args);
static Value DirectoryGetDirectories(Args args);

static Value InteropLoadLibrary(Args args);
static Value InteropDoCall(Args args);

static Value StringLength(Args args);
static Value StringSplit(Args args);
static Value StringSubstring(Args args);

static Value CharIsLower(Args args);
static Value CharIsUpper(Args args);
static Value CharToLower(Args args);
static Value CharToUpper(Args args);

static Value GcSetEnabled(Args args);
static Value GcGetEnabled(GRACE_MAYBE_UNUSED Args args);
static Value GcSetVerbose(Args args);
static Value GcGetVerbose(GRACE_MAYBE_UNUSED Args args);
static Value GcCollect(GRACE_MAYBE_UNUSED Args args);
static Value GcSetThreshold(Args args);
static Value GcGetThreshold(GRACE_MAYBE_UNUSED Args args);
static Value GcSetGrowFactor(Args args);
static Value GcGetGrowFactor(GRACE_MAYBE_UNUSED Args args);

static Value PathGetFileName(Args args);
static Value PathGetFileNameWithoutExtension(Args args);
static Value PathGetDirectory(Args args);
static Value PathCombine(Args args);
static Value PathExists(Args args);

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
  m_NativeFunctions.emplace_back("__NATIVE_LIST_APPEND", 2, &ListAppend);
  m_NativeFunctions.emplace_back("__NATIVE_LIST_INSERT", 3, &ListInsert);
  m_NativeFunctions.emplace_back("__NATIVE_LIST_REMOVE", 2, &ListRemove);
  m_NativeFunctions.emplace_back("__NATIVE_LIST_POP", 1, &ListPop);
  m_NativeFunctions.emplace_back("__NATIVE_LIST_SET_AT_INDEX", 3, &ListSetAtIndex);
  m_NativeFunctions.emplace_back("__NATIVE_LIST_GET_AT_INDEX", 2, &ListGetAtIndex);
  m_NativeFunctions.emplace_back("__NATIVE_LIST_LENGTH", 1, &ListLength);
  m_NativeFunctions.emplace_back("__NATIVE_LIST_SORT", 1, &ListSort);
  m_NativeFunctions.emplace_back("__NATIVE_LIST_SORT_DESCENDING", 1, &ListSortDescending);
  m_NativeFunctions.emplace_back("__NATIVE_LIST_SORTED", 1, &ListSorted);
  m_NativeFunctions.emplace_back("__NATIVE_LIST_SORTED_DESCENDING", 1, &ListSortedDescending);
  m_NativeFunctions.emplace_back("__NATIVE_LIST_FIRST", 1, &ListFirst);
  m_NativeFunctions.emplace_back("__NATIVE_LIST_LAST", 1, &ListLast);

  // Dictionary functions
  m_NativeFunctions.emplace_back("__NATIVE_DICTIONARY_INSERT", 3, &DictionaryInsert);
  m_NativeFunctions.emplace_back("__NATIVE_DICTIONARY_GET", 2, &DictionaryGet);
  m_NativeFunctions.emplace_back("__NATIVE_DICTIONARY_CONTAINS_KEY", 2, &DictionaryContainsKey);
  m_NativeFunctions.emplace_back("__NATIVE_DICTIONARY_REMOVE", 2, &DictionaryRemove);

  m_NativeFunctions.emplace_back("__NATIVE_KEYVALUEPAIR_KEY", 1, &KeyValuePairKey);
  m_NativeFunctions.emplace_back("__NATIVE_KEYVALUEPAIR_VALUE", 1, &KeyValuePairValue);

  m_NativeFunctions.emplace_back("__NATIVE_SET_ADD", 2, &SetAdd);
  m_NativeFunctions.emplace_back("__NATIVE_SET_CONTAINS", 2, &SetContains);
  m_NativeFunctions.emplace_back("__NATIVE_SET_SIZE", 1, &SetSize);
  // File functions
  m_NativeFunctions.emplace_back("__NATIVE_FILE_WRITE", 2, &FileWrite);
  m_NativeFunctions.emplace_back("__NATIVE_FILE_READ_ALL_TEXT", 1, &FileReadAllText);
  m_NativeFunctions.emplace_back("__NATIVE_FILE_READ_ALL_LINES", 1, &FileReadAllLines);

  // Console IO functions
  m_NativeFunctions.emplace_back("__NATIVE_FLUSH_STDOUT", 0, &FlushStdout);
  m_NativeFunctions.emplace_back("__NATIVE_FLUSH_STDERR", 0, &FlushStderr);

  // System functions
  m_NativeFunctions.emplace_back("__NATIVE_SYSTEM_EXIT", 1, &SystemExit);
  m_NativeFunctions.emplace_back("__NATIVE_SYSTEM_RUN", 1, &SystemRun);
  m_NativeFunctions.emplace_back("__NATIVE_SYSTEM_PLATFORM", 0, &SystemPlatform);

  // Directory functions
  m_NativeFunctions.emplace_back("__NATIVE_DIRECTORY_EXISTS", 1, &DirectoryExists);
  m_NativeFunctions.emplace_back("__NATIVE_DIRECTORY_CREATE", 1, &DirectoryCreate);
  m_NativeFunctions.emplace_back("__NATIVE_DIRECTORY_GET_DIRECTORIES", 1, &DirectoryGetDirectories);

  // Interop functions
  m_NativeFunctions.emplace_back("__NATIVE_INTEROP_LOAD_LIBRARY", 1, &InteropLoadLibrary);
  m_NativeFunctions.emplace_back("__NATIVE_INTEROP_DO_CALL", 4, &InteropDoCall);

  // String functions
  m_NativeFunctions.emplace_back("__NATIVE_STRING_LENGTH", 1, &StringLength);
  m_NativeFunctions.emplace_back("__NATIVE_STRING_SPLIT", 2, &StringSplit);
  m_NativeFunctions.emplace_back("__NATIVE_STRING_SUBSTRING", 3, &StringSubstring);

  // Char functions
  m_NativeFunctions.emplace_back("__NATIVE_CHAR_IS_LOWER", 1, &CharIsLower);
  m_NativeFunctions.emplace_back("__NATIVE_CHAR_IS_UPPER", 1, &CharIsUpper);
  m_NativeFunctions.emplace_back("__NATIVE_CHAR_TO_LOWER", 1, &CharToLower);
  m_NativeFunctions.emplace_back("__NATIVE_CHAR_TO_UPPER", 1, &CharToUpper);

  // GC functions
  m_NativeFunctions.emplace_back("__NATIVE_GC_SET_ENABLED", 1, &GcSetEnabled);
  m_NativeFunctions.emplace_back("__NATIVE_GC_GET_ENABLED", 0, &GcGetEnabled);
  m_NativeFunctions.emplace_back("__NATIVE_GC_SET_VERBOSE", 1, &GcSetVerbose);
  m_NativeFunctions.emplace_back("__NATIVE_GC_GET_VERBOSE", 0, &GcGetVerbose);
  m_NativeFunctions.emplace_back("__NATIVE_GC_COLLECT", 0, &GcCollect);
  m_NativeFunctions.emplace_back("__NATIVE_GC_SET_THRESHOLD", 1, &GcSetThreshold);
  m_NativeFunctions.emplace_back("__NATIVE_GC_GET_THRESHOLD", 0, &GcGetThreshold);
  m_NativeFunctions.emplace_back("__NATIVE_GC_SET_GROW_FACTOR", 1, &GcSetGrowFactor);
  m_NativeFunctions.emplace_back("__NATIVE_GC_GET_GROW_FACTOR", 0, &GcGetGrowFactor);

  // Path functions
  m_NativeFunctions.emplace_back("__NATIVE_PATH_GET_FILE_NAME", 1, &PathGetFileName);
  m_NativeFunctions.emplace_back("__NATIVE_PATH_GET_FILE_NAME_WITHOUT_EXTENSION", 1, &PathGetFileNameWithoutExtension);
  m_NativeFunctions.emplace_back("__NATIVE_PATH_GET_DIRECTORY", 1, &PathGetDirectory);
  m_NativeFunctions.emplace_back("__NATIVE_PATH_COMBINE", 2, &PathCombine);
  m_NativeFunctions.emplace_back("__NATIVE_PATH_EXISTS", 1, &PathExists);
}

static Value SqrtFloat(Args args)
{
  if (args[0].GetType() != Value::Type::Double) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `Float` for `std::float::sqrt(f)` but got `{}`", args[0].GetTypeName())
    );
  }

  return Value(std::sqrt(args[0].Get<double>()));
}

static Value SqrtInt(Args args)
{
  if (args[0].GetType() != Value::Type::Int) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `Int` for `std::int::sqrt(i)` but got `{}`", args[0].GetTypeName())
    );
  }

  return Value(std::sqrt(args[0].Get<std::int64_t>()));
}

static Value TimeHours(GRACE_MAYBE_UNUSED Args args)
{
  return Value(static_cast<std::int64_t>(std::chrono::duration_cast<std::chrono::hours>(std::chrono::steady_clock::now().time_since_epoch()).count()));
}

static Value TimeMinutes(GRACE_MAYBE_UNUSED Args args)
{
  return Value(static_cast<std::int64_t>(std::chrono::duration_cast<std::chrono::minutes>(std::chrono::steady_clock::now().time_since_epoch()).count()));
}

static Value TimeSeconds(GRACE_MAYBE_UNUSED Args args)
{
  return Value(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

static Value TimeMilliSeconds(GRACE_MAYBE_UNUSED Args args)
{
  return Value(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

static Value TimeMicroSeconds(GRACE_MAYBE_UNUSED Args args)
{
  return Value(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

static Value TimeNanoSeconds(GRACE_MAYBE_UNUSED Args args)
{
  return Value(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

static Value Sleep(Args args)
{
  if (args[0].GetType() != Value::Type::Int) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `Int` for `std::time::sleep(time_ns)` but got `{}`", args[0].GetTypeName())
    );
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(args[0].Get<std::int64_t>()));
  return {};
}

static Value ListAppend(Args args)
{
  if (auto list = args[0].GetObject()->GetAsList()) {
    list->Append(std::move(args[1]));
    return {};
  }

  throw Grace::GraceException(
    Grace::GraceException::Type::InvalidType,
    fmt::format("Expected `List` for `std::list::append(list, value)` but got `{}`", args[0].GetTypeName())
  );
}

static Value ListInsert(Args args)
{
  if (auto list = args[0].GetObject()->GetAsList()) {
    if (args[1].GetType() != Value::Type::Int) {
      throw Grace::GraceException(
        Grace::GraceException::Type::InvalidType,
        fmt::format("Expected `Int` for `std::list::insert(list, index, value)` but got `{}`", args[1].GetTypeName())
      );
    }

    list->Insert(std::move(args[2]), static_cast<std::size_t>(args[1].Get<std::int64_t>()));
    return {};
  }

  throw Grace::GraceException(
    Grace::GraceException::Type::InvalidType,
    fmt::format("Expected `List` for `std::list::insert(list, index, value)` but got `{}`", args[0].GetTypeName())
  );
}

static Value ListRemove(Args args)
{
  if (auto list = args[0].GetObject()->GetAsList()) {
    if (args[1].GetType() != Value::Type::Int) {
      throw Grace::GraceException(
        Grace::GraceException::Type::InvalidType,
        fmt::format("Expected `Int` for `std::list::remove(list, index)` but got `{}`", args[1].GetTypeName())
      );
    }

    return list->Remove(static_cast<std::size_t>(args[1].Get<std::int64_t>()));
  }

  throw Grace::GraceException(
    Grace::GraceException::Type::InvalidType,
    fmt::format("Expected `List` for `std::list::remove(list, index)` but got `{}`", args[0].GetTypeName())
  );
}

static Value ListPop(Args args)
{
  if (auto list = args[0].GetObject()->GetAsList()) {
    return list->Pop();
  }

  throw Grace::GraceException(
    Grace::GraceException::Type::InvalidType,
    fmt::format("Expected `List` for `std::list::pop(list)` but got `{}`", args[0].GetTypeName())
  );
}

static Value ListSetAtIndex(Args args)
{
  if (auto list = args[0].GetObject()->GetAsList()) {
    if (args[1].GetType() != Value::Type::Int) {
      throw Grace::GraceException(
        Grace::GraceException::Type::InvalidType,
        fmt::format("Expected `Int` for `std::list::set(list, index, value)` but got `{}`", args[1].GetTypeName())
      );
    }

    (*list)[args[1].Get<std::int64_t>()] = std::move(args[2]);
    return {};
  }

  throw Grace::GraceException(
    Grace::GraceException::Type::InvalidType,
    fmt::format("Expected `List` for `std::list::set(list, index, value)` but got `{}`", args[0].GetTypeName())
  );
}

static Value ListGetAtIndex(Args args)
{
  if (auto list = args[0].GetObject()->GetAsList()) {
    if (args[1].GetType() != Value::Type::Int) {
      throw Grace::GraceException(
        Grace::GraceException::Type::InvalidType,
        fmt::format("Expected `Int` for `std::list::get(list, index)` but got `{}`", args[1].GetTypeName())
      );
    }

    return (*list)[args[1].Get<std::int64_t>()];
  }

  throw Grace::GraceException(
    Grace::GraceException::Type::InvalidType,
    fmt::format("Expected `List` for `std::list::get(list, index)` but got `{}`", args[0].GetTypeName())
  );
}

static Value ListLength(Args args)
{
  if (auto list = args[0].GetObject()->GetAsList()) {
    return Value(static_cast<std::int64_t>(list->Length()));
  }

  throw Grace::GraceException(
    Grace::GraceException::Type::InvalidType,
    fmt::format("Expected `List` for `std::list::get(list, index)` but got `{}`", args[0].GetTypeName())
  );
}

static Value ListSort(Args args)
{
  if (auto list = args[0].GetObject()->GetAsList()) {
    list->Sort();
    return {};
  }

  throw Grace::GraceException(
    Grace::GraceException::Type::InvalidType,
    fmt::format("Expected `List` for `std::list::sort(list)` but got `{}`", args[0].GetTypeName())
  );
}

static Value ListSortDescending(Args args)
{
  if (auto list = args[0].GetObject()->GetAsList()) {
    list->SortDescending();
    return {};
  }

  throw Grace::GraceException(
    Grace::GraceException::Type::InvalidType,
    fmt::format("Expected `List` for `std::list::sort_descending(list)` but got `{}`", args[0].GetTypeName())
  );
}

static Value ListSorted(Args args)
{
  if (auto list = args[0].GetObject()->GetAsList()) {
    auto newList = Value::CreateObject<Grace::GraceList>(*list);
    newList.GetObject()->GetAsList()->Sort();
    return newList;
  }

  throw Grace::GraceException(
    Grace::GraceException::Type::InvalidType,
    fmt::format("Expected `List` for `std::list::sort(list)` but got `{}`", args[0].GetTypeName())
  );
}

static Value ListSortedDescending(Args args)
{
  if (auto list = args[0].GetObject()->GetAsList()) {
    auto newList = Value::CreateObject<Grace::GraceList>(*list);
    newList.GetObject()->GetAsList()->SortDescending();
    return newList;
  }

  throw Grace::GraceException(
    Grace::GraceException::Type::InvalidType,
    fmt::format("Expected `List` for `std::list::sort_descending(list)` but got `{}`", args[0].GetTypeName())
  );
}

static Value ListFirst(Args args)
{
  if (auto list = args[0].GetObject()->GetAsList()) {
    return list->First();
  }

  throw Grace::GraceException(
    Grace::GraceException::Type::InvalidType,
    fmt::format("Expected `List` for `std::list::first(list)` but got `{}`", args[0].GetTypeName())
  );
}

static Value ListLast(Args args)
{
  if (auto list = args[0].GetObject()->GetAsList()) {
    return list->Last();
  }

  throw Grace::GraceException(
    Grace::GraceException::Type::InvalidType,
    fmt::format("Expected `List` for `std::list::last(list)` but got `{}`", args[0].GetTypeName())
  );
}

static Value DictionaryInsert(Args args)
{
  if (args[0].GetObject()->GetAsDictionary() == nullptr) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `Dict` for `std::dict::insert(dict, key, value)` but got `{}`", args[0].GetTypeName())
    );
  }

  auto dict = args[0].GetObject()->GetAsDictionary();
  dict->Insert(std::move(args[1]), std::move(args[2]));
  return {};
}

static Value DictionaryGet(Args args)
{
  if (args[0].GetObject()->GetAsDictionary() == nullptr) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `Dict` for `std::dict::get(dict, key)` but got `{}`", args[0].GetTypeName())
    );
  }

  auto dict = args[0].GetObject()->GetAsDictionary();
  return dict->Get(args[1]);
}

static Value DictionaryContainsKey(Args args)
{
  if (args[0].GetObject()->GetAsDictionary() == nullptr) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `Dict` for `std::dict::contains_key(dict, key)` but got `{}`", args[0].GetTypeName())
    );
  }

  auto dict = args[0].GetObject()->GetAsDictionary();
  return Value(dict->ContainsKey(args[1]));
}

static Value DictionaryRemove(Args args)
{
  if (args[0].GetObject()->GetAsDictionary() == nullptr) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `Dict` for `std::dict::remove(dict, key)` but got `{}`", args[0].GetTypeName())
    );
  }

  auto dict = args[0].GetObject()->GetAsDictionary();
  return Value(dict->Remove(args[1]));
}

static Value KeyValuePairKey(Args args)
{
  if (args[0].GetObject()->GetAsKeyValuePair() == nullptr) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `KeyValuePair` for `std::keyvaluepair::key(pair)` but got `{}`", args[0].GetTypeName())
    );
  }

  auto kvp = args[0].GetObject()->GetAsKeyValuePair();
  return kvp->Key();
}

static Value KeyValuePairValue(Args args)
{
  if (args[0].GetObject()->GetAsKeyValuePair() == nullptr) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `KeyValuePair` for `std::keyvaluepair::value(pair)` but got `{}`", args[0].GetTypeName())
    );
  }

  auto kvp = args[0].GetObject()->GetAsKeyValuePair();
  return kvp->Value();
}

static Value SetAdd(Args args)
{
  if (auto set = args[0].GetObject()->GetAsSet()) {
    set->Add(std::move(args[1]));
    return {};
  }

  throw Grace::GraceException(
    Grace::GraceException::Type::InvalidType,
    fmt::format("Expected `Set` for `std::set::add(set, value)` but got `{}`", args[0].GetTypeName())
  );
}

static Value SetContains(Args args)
{
  if (auto set = args[0].GetObject()->GetAsSet()) {
    return Value(set->Contains(args[1]));
  }

  throw Grace::GraceException(
    Grace::GraceException::Type::InvalidType,
    fmt::format("Expected `Set` for `std::set::contains(set, value)` but got `{}`", args[0].GetTypeName())
  );
}

static Value SetSize(Args args)
{
  if (auto set = args[0].GetObject()->GetAsSet()) {
    return Value(static_cast<std::int64_t>(set->Size()));
  }

  throw Grace::GraceException(
    Grace::GraceException::Type::InvalidType,
    fmt::format("Expected `Set` for `std::set::size(set)` but got `{}`", args[0].GetTypeName())
  );
}

static Value FileWrite(Args args)
{
  if (args[0].GetType() != Value::Type::String) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `String` for `std::file::write(path, contents)` but got `{}`", args[0].GetTypeName())
    );
  }

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

  return {};
}

static Value FileReadAllText(Args args)
{
  if (args[0].GetType() != Value::Type::String) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `String` for `std::file::read_all_text(path)` but got `{}`", args[0].GetTypeName())
    );
  }

  auto& path = args[0];

  std::ifstream inStream;
  inStream.open(path.AsString());

  if (inStream.fail()) {
    throw Grace::GraceException(
      Grace::GraceException::Type::FileReadFailed,
      fmt::format("Failed to open file '{}'", path)
    );
  }

  std::stringstream ss;
  ss << inStream.rdbuf();
  return Value(ss.str());
}

static Value FileReadAllLines(Args args)
{
  if (args[0].GetType() != Value::Type::String) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `String` for `std::file::read_all_lines(path)` but got `{}`", args[0].GetTypeName())
    );
  }

  auto& path = args[0];

  std::ifstream inStream;
  inStream.open(path.AsString());

  if (inStream.fail()) {
    throw Grace::GraceException(
      Grace::GraceException::Type::FileReadFailed,
      fmt::format("Failed to open file '{}'", path)
    );
  }

  auto res = Value::CreateObject<Grace::GraceList>();
  auto list = res.GetObject()->GetAsList();
  for (std::string line; std::getline(inStream, line);) {
    list->Append(line);
  }

  return res;
}

static Value FlushStdout(GRACE_MAYBE_UNUSED Args args)
{
  std::fflush(stdout);
  return {};
}

static Value FlushStderr(GRACE_MAYBE_UNUSED Args args)
{
  std::fflush(stderr);
  return {};
}

static Value SystemExit(Args args)
{
  if (args[0].GetType() != Value::Type::Int) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `Int` for `std::system::exit(exit_code)` but got `{}`", args[0].GetTypeName())
    );
  }

  std::exit(static_cast<int>(args[0].Get<std::int64_t>()));
}

static Value SystemRun(Args args)
{
  if (args[0].GetType() == Value::Type::String) {
    auto res = std::system(args[0].Get<std::string>().c_str());
    return Value(std::int64_t(res));
  }

  throw Grace::GraceException(
    Grace::GraceException::Type::InvalidType,
    fmt::format("Expected `String` for `std::system::run(command)` but got `{}`", args[0].GetTypeName())
  );
}

static Value SystemPlatform(GRACE_MAYBE_UNUSED Args args)
{
#ifdef _WIN32
  return Value(std::string("Win32"));
#elif _WIN64
  return Value(std::string("Win64"));
#elif __APPLE__ || __MACH__
  return Value(std::string("macOS"));
#elif __linux__
  return Value(std::string("Linux"));
#elif __FreeBSD__
  return Value(std::string("FreeBSD"));
#elif __unix || __unix__
  return Value(std::string("Unix"));
#else
  return Value(std::string("Other"));
#endif
}

static Value DirectoryExists(Args args)
{
  if (args[0].GetType() != Value::Type::String) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `String` for `std::directory::exists(path)` but got `{}`", args[0].GetTypeName())
    );
  }

  const auto& path = args[0].Get<std::string>();
  auto exists = std::filesystem::is_directory(path);
  return Value(exists);
}

static Value DirectoryCreate(Args args)
{
  if (args[0].GetType() != Value::Type::String) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `String` for `std::directory::create(path)` but got `{}`", args[0].GetTypeName())
    );
  }

  const auto& path = args[0].Get<std::string>();
  auto result = std::filesystem::create_directories(path);
  return Value(result);
}

static Value DirectoryGetDirectories(Args args)
{
  namespace fs = std::filesystem;

  if (args[0].GetType() == Value::Type::String) {
    fs::path path(args[0].Get<std::string>());

    if (!fs::is_directory(path)) {
      throw Grace::GraceException(
        Grace::GraceException::Type::PathError,
        fmt::format("{} is not a directory", path.string())
      );
    }

    auto res = Value::CreateObject<Grace::GraceList>();
    auto list = res.GetObject()->GetAsList();
    for (const auto& dir : fs::directory_iterator(path)) {
      if (dir.is_directory()) {
        list->Append(dir.path().string());
      }
    }

    return res;
  }

  throw Grace::GraceException(
    Grace::GraceException::Type::InvalidType,
    fmt::format("Expected `String` for `std::directory::get_directories(path)` but got `{}`", args[0].GetTypeName())
  );
}

static Value InteropLoadLibrary(Args args)
{
  if (args[0].GetType() != Value::Type::String) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `String` for `std::interop::load_library(library_path)` but got `{}`", args[0].GetTypeName())
    );
  }

  const auto& libName = args[0].Get<std::string>();

  auto handle = dlLoadLibrary(libName.c_str());
  if (handle == nullptr) {
    throw Grace::GraceException(Grace::GraceException::Type::LibraryLoadFailure, fmt::format("Failed to load dynamic library {}", libName));
  }

  auto ptrAsInt = (std::int64_t)handle;
  return Value(ptrAsInt);
}

static Value InteropDoCall(Args args)
{
#define CDECL_ARG_BOOL 0
#define CDECL_ARG_CHAR 1
#define CDECL_ARG_SHORT 2
#define CDECL_ARG_INT 3
#define CDECL_ARG_LONG 4
#define CDECL_ARG_LONG_LONG 5
#define CDECL_ARG_FLOAT 6
#define CDECL_ARG_DOUBLE 7
#define CDECL_ARG_POINTER 8

#define CDECL_CALL_BOOL 0
#define CDECL_CALL_CHAR 1
#define CDECL_CALL_SHORT 2
#define CDECL_CALL_INT 3
#define CDECL_CALL_LONG 4
#define CDECL_CALL_LONG_LONG 5
#define CDECL_CALL_FLOAT 6
#define CDECL_CALL_DOUBLE 7
#define CDECL_CALL_POINTER 8
#define CDECL_CALL_VOID 9

  auto handleInstance = args[0].GetObject()->GetAsInstance();

  if (handleInstance == nullptr || handleInstance->ObjectName() != "LibraryHandle") {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `std::interop::LibraryHandle` for `handle` in `std::interop::do_call(handle, func_name, args, call_type)` but got `{}`", args[0].GetTypeName())
    );
  }

  if (args[1].GetType() != Value::Type::String) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `String` for `func_name` in `std::interop::do_call(handle, func_name, args, call_type)` but got `{}`", args[1].GetTypeName())
    );
  }

  const auto& funcName = args[1].Get<std::string>();

  auto argsObject = args[2].GetObject();
  if (argsObject == nullptr || argsObject->GetAsList() == nullptr) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `List` for `args` in `std::interop::do_call(handle, func_name, args, call_type)` but got `{}`", args[2].GetTypeName())
    );
  }
  auto argList = argsObject->GetAsList();

  if (args[3].GetType() != Value::Type::Int) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `Int` for `call_type` in `std::interop::do_call(handle, func_name, args, call_type)` but got `{}`", args[3].GetTypeName())
    );
  }
  auto callType = args[3].Get<std::int64_t>();

  auto addressAsInt = (DLLib*)(handleInstance->LoadMember("lib_address").Get<std::int64_t>());
  void* funcPtr = dlFindSymbol(addressAsInt, funcName.c_str());
  if (funcPtr == nullptr) {
    throw Grace::GraceException(
      Grace::GraceException::Type::FunctionNotFound,
      fmt::format("Failed to load function `{}` from dynamic library `{}`", funcName, handleInstance->LoadMember("library_name").Get<std::string>())
    );
  }

  auto stackSize = static_cast<std::size_t>(handleInstance->LoadMember("stack_size").Get<std::int64_t>());
  DCCallVM* vm = dcNewCallVM(stackSize);
  dcMode(vm, DC_CALL_C_DEFAULT);
  dcReset(vm);

  std::vector<char*> argsToDelete;

  for (auto it = argList->Begin(); it != argList->End(); it++) {
    auto pair = it->GetObject()->GetAsKeyValuePair();
    if (pair == nullptr) {
      throw Grace::GraceException(Grace::GraceException::Type::InvalidType, fmt::format("Expected all args in arg list to be `KeyValuePair`s but got `{}`", it->GetTypeName()));
    }

    auto& argType = pair->Key();
    switch (argType.Get<std::int64_t>()) {
      case CDECL_ARG_BOOL:
        dcArgBool(vm, pair->Value().Get<bool>());
        break;
      case CDECL_ARG_CHAR:
        dcArgChar(vm, pair->Value().Get<char>());
        break;
      case CDECL_ARG_SHORT:
        dcArgShort(vm, static_cast<short>(pair->Value().Get<std::int64_t>()));
        break;
      case CDECL_ARG_INT:
        dcArgInt(vm, static_cast<int>(pair->Value().Get<std::int64_t>()));
        break;
      case CDECL_ARG_LONG:
        dcArgLong(vm, static_cast<long>(pair->Value().Get<std::int64_t>()));
        break;
      case CDECL_ARG_LONG_LONG:
        dcArgLongLong(vm, static_cast<long long>(pair->Value().Get<std::int64_t>()));
        break;
      case CDECL_ARG_FLOAT:
        dcArgFloat(vm, static_cast<float>(pair->Value().Get<double>()));
        break;
      case CDECL_ARG_DOUBLE:
        dcArgDouble(vm, pair->Value().Get<double>());
        break;
      case CDECL_ARG_POINTER: {
        auto& str = pair->Value().Get<std::string>();
        auto charArray = new char[str.size() + 1];  // + 1 for null terminated char
        std::memcpy(charArray, str.data(), str.size());
        charArray[str.size()] = '\0';
        argsToDelete.push_back(charArray);
        dcArgPointer(vm, charArray);
        break;
      }
      default:
        throw Grace::GraceException(Grace::GraceException::Type::InvalidArgument, "Invalid type given for cdecl argument type");
    }
  }

  Value returnValue;

  switch (callType) {
    case CDECL_CALL_BOOL:
      returnValue = static_cast<bool>(dcCallBool(vm, funcPtr));
      break;
    case CDECL_CALL_CHAR:
      returnValue = static_cast<char>(dcCallChar(vm, funcPtr));
      break;
    case CDECL_CALL_SHORT:
      returnValue = static_cast<std::int64_t>(dcCallShort(vm, funcPtr));
      break;
    case CDECL_CALL_INT:
      returnValue = static_cast<std::int64_t>(dcCallInt(vm, funcPtr));
      break;
    case CDECL_CALL_LONG:
      returnValue = static_cast<std::int64_t>(dcCallLong(vm, funcPtr));
      break;
    case CDECL_CALL_LONG_LONG:
      returnValue = static_cast<std::int64_t>(dcCallLongLong(vm, funcPtr));
      break;
    case CDECL_CALL_FLOAT:
      returnValue = static_cast<double>(dcCallFloat(vm, funcPtr));
      break;
    case CDECL_CALL_DOUBLE:
      returnValue = static_cast<double>(dcCallDouble(vm, funcPtr));
      break;
    case CDECL_CALL_POINTER:
      GRACE_NOT_IMPLEMENTED();
      break;
    case CDECL_CALL_VOID:
      dcCallVoid(vm, funcPtr);
      break;
    default:
      throw Grace::GraceException(Grace::GraceException::Type::InvalidArgument, "Invalid return type given for cdecl library call");
  }

  for (auto p : argsToDelete) {
    delete[] p;
  }

  return returnValue;

#undef CDECL_ARG_BOOL
#undef CDECL_ARG_CHAR
#undef CDECL_ARG_SHORT
#undef CDECL_ARG_INT
#undef CDECL_ARG_LONG
#undef CDECL_ARG_LONG_LONG
#undef CDECL_ARG_FLOAT
#undef CDECL_ARG_DOUBLE
#undef CDECL_ARG_POINTER

#undef CDECL_CALL_BOOL
#undef CDECL_CALL_CHAR
#undef CDECL_CALL_SHORT
#undef CDECL_CALL_INT
#undef CDECL_CALL_LONG
#undef CDECL_CALL_LONG_LONG
#undef CDECL_CALL_FLOAT
#undef CDECL_CALL_DOUBLE
#undef CDECL_CALL_POINTER
#undef CDECL_CALL_VOID
}

static Value StringLength(Args args)
{
  auto& s = args[0];
  if (s.GetType() != Value::Type::String) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `String` for `std::string::length(s)` but got `{}`", args[0].GetTypeName())
    );
  }

  return Value(static_cast<std::int64_t>(s.Get<std::string>().length()));
}

static Value StringSplit(Args args)
{
  if (args[0].GetType() != Value::Type::String) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `String` for `std::string::split(s, separator)` but got `{}`", args[0].GetTypeName())
    );
  }

  if (args[1].GetType() != Value::Type::String && args[1].GetType() != Value::Type::Char) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `String` or `Char` for `std::string::split(s, separator)` but got `{}`", args[1].GetTypeName())
    );
  }

  const auto& s = args[0].Get<std::string>();
  auto separator = args[1].AsString();

  if (separator.length() == 0) {
    throw Grace::GraceException(Grace::GraceException::Type::InvalidArgument, "Separator has 0 length");
  }

  auto res = Value::CreateObject<Grace::GraceList>();
  auto list = res.GetObject()->GetAsList();
  if (s.find(separator) == std::string::npos) {
    list->Append(s);
    return res;
  }
  
  std::size_t lastSplit = 0;
  for (std::size_t i = 0; i < s.length();) {
    if (s.substr(i, separator.length()) == separator) {
      list->Append(s.substr(lastSplit, i - lastSplit));
      lastSplit = i + separator.length();
      i = lastSplit;
    } else {
      i++;
    }
  }

  list->Append(s.substr(lastSplit));
  return res;
}

static Value StringSubstring(Args args)
{
  if (args[0].GetType() == Value::Type::String) {
    if (args[1].GetType() != Value::Type::Int) {
      throw Grace::GraceException(
        Grace::GraceException::Type::InvalidType,
        fmt::format("Expected `Int` for `start` in `std::string::substring(string, start, length)` but got `{}`", args[1].GetType())
      );
    }

    if (args[2].GetType() != Value::Type::Int) {
      throw Grace::GraceException(
        Grace::GraceException::Type::InvalidType,
        fmt::format("Expected `Int` for `length` in `std::string::substring(string, start, length)` but got `{}`", args[2].GetType())
      );
    }

    return Value(args[0].Get<std::string>().substr(static_cast<std::size_t>(args[1].Get<std::int64_t>()), static_cast<std::size_t>(args[2].Get<std::int64_t>())));
  }

  throw Grace::GraceException(
    Grace::GraceException::Type::InvalidType,
    fmt::format("Expected `String` for `std::string::substring(string, start, length)` but got `{}`", args[0].GetType())
  );  
}

static Value CharIsLower(Args args)
{
  if (args[0].GetType() == Value::Type::Char) {
    return Value(static_cast<bool>(std::islower(args[0].Get<char>())));
  }

  throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `Char` for `std::char::is_lower(char)` but got `{}`", args[0].GetTypeName())
    );
}

static Value CharIsUpper(Args args)
{
  if (args[0].GetType() == Value::Type::Char) {
    return Value(static_cast<bool>(std::isupper(args[0].Get<char>())));
  }

  throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `Char` for `std::char::is_upper(char)` but got `{}`", args[0].GetTypeName())
    );
}

static Value CharToLower(Args args)
{
  if (args[0].GetType() == Value::Type::Char) {
    return Value(static_cast<char>(std::tolower(args[0].Get<char>())));
  }

  throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `Char` for `std::char::to_lower(char)` but got `{}`", args[0].GetTypeName())
    );
}

static Value CharToUpper(Args args)
{
  if (args[0].GetType() == Value::Type::Char) {
    return Value(static_cast<char>(std::toupper(args[0].Get<char>())));
  }

  throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `Char` for `std::char::to_upper(char)` but got `{}`", args[0].GetTypeName())
    );
}

static Value GcSetEnabled(Args args)
{
  if (args[0].GetType() != Value::Type::Bool) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `Bool` for `std::gc::set_enabled(state)` but got `{}`", args[0].GetTypeName())
    );
  }

  Grace::ObjectTracker::SetEnabled(args[0].Get<bool>());
  return {};
}

static Value GcGetEnabled(GRACE_MAYBE_UNUSED Args args)
{
  return Value(Grace::ObjectTracker::GetEnabled());
}

static Value GcSetVerbose(Args args)
{
  if (args[0].GetType() != Value::Type::Bool) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `Bool` for `std::gc::set_debug(state)` but got `{}`", args[0].GetTypeName())
    );
  }

  Grace::ObjectTracker::SetVerbose(args[0].Get<bool>());
  return {};
}

static Value GcGetVerbose(GRACE_MAYBE_UNUSED Args args)
{
  return Value(Grace::ObjectTracker::GetVerbose());
}

static Value GcCollect(GRACE_MAYBE_UNUSED Args args)
{
  Grace::ObjectTracker::Collect();
  return {};
}

static Value GcSetThreshold(Args args)
{
  if (args[0].GetType() != Value::Type::Int) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `Int` for `std::gc::set_threshold(threshold)` but got `{}`", args[0].GetTypeName())
    );
  }

  auto value = args[0].Get<std::int64_t>();
  if (value <= 0) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidArgument,
      fmt::format("Expected positive number for `std::gc::set_threshold(threshold)` but got `{}`", value)
    );
  }

  Grace::ObjectTracker::SetThreshold(static_cast<std::size_t>(value));
  return {};
}

static Value GcGetThreshold(GRACE_MAYBE_UNUSED Args args)
{
  return Value(static_cast<std::int64_t>(Grace::ObjectTracker::GetThreshold()));
}

static Value GcSetGrowFactor(Args args)
{
  if (args[0].GetType() != Value::Type::Int) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `Int` for `std::gc::set_grow_factor(grow_factor)` but got `{}`", args[0].GetTypeName())
    );
  }

  auto value = args[0].Get<std::int64_t>();
  if (value <= 0) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidArgument,
      fmt::format("Expected positive number for `std::gc::set_grow_factor(grow_factor)` but got `{}`", value)
    );
  }

  Grace::ObjectTracker::SetGrowFactor(static_cast<std::size_t>(value));
  return {};
}

static Value GcGetGrowFactor(GRACE_MAYBE_UNUSED Args args)
{
  return Value(static_cast<std::int64_t>(Grace::ObjectTracker::GetGrowFactor()));
}

static Value PathGetFileName(Args args)
{
  namespace fs = std::filesystem;

  if (args[0].GetType() == Value::Type::Object) {
    auto instance = args[0].GetObject()->GetAsInstance();
  	if (instance != nullptr && instance->HasMember("data")) {
      const auto& path = instance->LoadMember("data");
      if (path.GetType() != Value::Type::String) {
        throw Grace::GraceException(
          Grace::GraceException::Type::InvalidType,
          fmt::format("Expected type of member `data` of `std::path::Path` to be `String` but got `{}`", path.GetTypeName())
        );
      }

      try {
        return Value(fs::path(path.Get<std::string>()).filename().string());
      } catch (const std::exception&) {
        throw Grace::GraceException(
          Grace::GraceException::Type::PathError,
          "Invalid path"
        );
      }
    }
  }

  throw Grace::GraceException(
    Grace::GraceException::Type::InvalidType,
    fmt::format("Expected `Path` for `std::path::get_file_name(path)` but got `{}`", args[0].GetTypeName())
  );
}

static Value PathGetFileNameWithoutExtension(Args args)
{
  namespace fs = std::filesystem;

  if (args[0].GetType() == Value::Type::Object) {
    auto instance = args[0].GetObject()->GetAsInstance();
  	if (instance != nullptr && instance->HasMember("data")) {
      const auto& path = instance->LoadMember("data");
      if (path.GetType() != Value::Type::String) {
        throw Grace::GraceException(
          Grace::GraceException::Type::InvalidType,
          fmt::format("Expected type of member `data` of `std::path::Path` to be `String` but got `{}`", path.GetTypeName())
        );
      }

      try {
        return Value(fs::path(path.Get<std::string>()).stem().string());
      } catch (const std::exception&) {
        throw Grace::GraceException(
          Grace::GraceException::Type::PathError,
          "Invalid path"
        );
      }
    }
  }

  throw Grace::GraceException(
    Grace::GraceException::Type::InvalidType,
    fmt::format("Expected `Path` for `std::path::get_file_name_without_extension(path)` but got `{}`", args[0].GetTypeName())
  );
}

static Value PathGetDirectory(Args args)
{
  namespace fs = std::filesystem;

  if (args[0].GetType() == Value::Type::Object) {
    auto instance = args[0].GetObject()->GetAsInstance();
  	if (instance != nullptr && instance->HasMember("data")) {
      const auto& path = instance->LoadMember("data");
      if (path.GetType() != Value::Type::String) {
        throw Grace::GraceException(
          Grace::GraceException::Type::InvalidType,
          fmt::format("Expected type of member `data` of `std::path::Path` to be `String` but got `{}`", path.GetTypeName())
        );
      }

      try {
        return Value(fs::path(path.Get<std::string>()).parent_path().string());
      } catch (const std::exception&) {
        throw Grace::GraceException(
          Grace::GraceException::Type::PathError,
          "Invalid path"
        );
      }
    }
  }

  throw Grace::GraceException(
    Grace::GraceException::Type::InvalidType,
    fmt::format("Expected `Path` for `std::path::get_directory(path)` but got `{}`", args[0].GetTypeName())
  );
}

static Value PathCombine(Args args)
{
  namespace fs = std::filesystem;

  if (args[0].GetType() == Value::Type::Object) {
    auto instance = args[0].GetObject()->GetAsInstance();
  	if (instance != nullptr && instance->HasMember("data")) {
      const auto& path = instance->LoadMember("data");
      if (path.GetType() != Value::Type::String) {
        throw Grace::GraceException(
          Grace::GraceException::Type::InvalidType,
          fmt::format("Expected type of member `data` of `std::path::Path` to be `String` but got `{}`", path.GetTypeName())
        );
      }

      if (args[1].GetType() == Value::Type::String) {
        return Value((fs::path(path.Get<std::string>()) / args[1].Get<std::string>()).string());
      } else if (args[1].GetType() == Value::Type::Object) {
        if (auto list = args[1].GetObject()->GetAsList()) {
          fs::path base(path.Get<std::string>());

          for (std::size_t i = 0; i < list->Length(); i++) {
            const auto& p = (*list)[i];
            if (p.GetType() != Value::Type::String) {
              throw Grace::GraceException(
                Grace::GraceException::Type::InvalidType,
                fmt::format("Expected `String` or all elements of `additions` in `std::path::combine(path, additions)` but got `{}` at position {}", p.GetTypeName(), i)
              );
            }

            base /= p.Get<std::string>();
          }

          return Value(base.string());
        }
      }

      throw Grace::GraceException(
        Grace::GraceException::Type::InvalidType,
        fmt::format("Expected `String` or `List` for `std::path::combine(path, additions)` but got `{}`", args[1].GetTypeName())
      );
    }
  }

  throw Grace::GraceException(
    Grace::GraceException::Type::InvalidType,
    fmt::format("Expected `Path` for `std::path::combine(path)` but got `{}`", args[0].GetTypeName())
  );
}

static Value PathExists(Args args)
{
  namespace fs = std::filesystem;

  if (args[0].GetType() == Value::Type::Object) {
    auto instance = args[0].GetObject()->GetAsInstance();
  	if (instance != nullptr && instance->HasMember("data")) {
      const auto& path = instance->LoadMember("data");
      if (path.GetType() != Value::Type::String) {
        throw Grace::GraceException(
          Grace::GraceException::Type::InvalidType,
          fmt::format("Expected type of member `data` of `std::path::Path` to be `String` but got `{}`", path.GetTypeName())
        );
      }

      return Value(fs::exists(path.Get<std::string>()));
    }
  }

  throw Grace::GraceException(
    Grace::GraceException::Type::InvalidType,
    fmt::format("Expected `Path` for `std::path::exists(path)` but got `{}`", args[0].GetTypeName())
  );
}
