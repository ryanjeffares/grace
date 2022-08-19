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
#include "objects/grace_exception.hpp"
#include "objects/grace_instance.hpp"
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
static Value DictionaryRemove(std::vector<Value>& args);

static Value KeyValuePairKey(std::vector<Value>& args);
static Value KeyValuePairValue(std::vector<Value>& args);

static Value FileWrite(std::vector<Value>& args);

static Value FlushStdout(GRACE_MAYBE_UNUSED std::vector<Value>& args);
static Value FlushStderr(GRACE_MAYBE_UNUSED std::vector<Value>& args);

static Value SystemExit(std::vector<Value>& args);
static Value SystemRun(std::vector<Value>& args);
static Value SystemPlatform(GRACE_MAYBE_UNUSED std::vector<Value>& args);

static Value DirectoryExists(std::vector<Value>& args);
static Value DirectoryCreate(std::vector<Value>& args);

static Value InteropLoadLibrary(std::vector<Value>& args);
static Value InteropDoCall(std::vector<Value>& args);

static Value StringLength(std::vector<Value>& args);
static Value StringSplit(std::vector<Value>& args);

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
  m_NativeFunctions.emplace_back("__NATIVE_DICTIONARY_REMOVE", 2, &DictionaryRemove);

  m_NativeFunctions.emplace_back("__NATIVE_KEYVALUEPAIR_KEY", 1, &KeyValuePairKey);
  m_NativeFunctions.emplace_back("__NATIVE_KEYVALUEPAIR_VALUE", 1, &KeyValuePairValue);

  // File functions
  m_NativeFunctions.emplace_back("__NATIVE_FILE_WRITE", 2, &FileWrite);

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

  // Interop functions
  m_NativeFunctions.emplace_back("__NATIVE_INTEROP_LOAD_LIBRARY", 1, &InteropLoadLibrary);
  m_NativeFunctions.emplace_back("__NATIVE_INTEROP_DO_CALL", 4, &InteropDoCall);

  // String functions
  m_NativeFunctions.emplace_back("__NATIVE_STRING_LENGTH", 1, &StringLength);
  m_NativeFunctions.emplace_back("__NATIVE_STRING_SPLIT", 2, &StringSplit);
}

static Value SqrtFloat(std::vector<Value>& args)
{
  if (args[0].GetType() != Value::Type::Double) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `Float` for `std::float::sqrt(f)` but got `{}`", args[0].GetTypeName())
    );
  }

  return Value(std::sqrt(args[0].Get<double>()));
}

static Value SqrtInt(std::vector<Value>& args)
{
  if (args[0].GetType() != Value::Type::Int) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `Int` for `std::int::sqrt(i)` but got `{}`", args[0].GetTypeName())
    );
  }

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
  if (args[0].GetType() != Value::Type::Int) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `Int` for `std::time::sleep(time_ns)` but got `{}`", args[0].GetTypeName())
    );
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(args[0].Get<std::int64_t>()));
  return Value();
}

static Value ListAppend(std::vector<Value>& args)
{
  if (args[0].GetObject()->GetAsList() == nullptr) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `List` for `std::list::append(list, value)` but got `{}`", args[0].GetTypeName())
    );
  }

  args[0].GetObject()->GetAsList()->Append(std::move(args[1]));
  return Value();
}

static Value ListSetAtIndex(std::vector<Value>& args)
{
  if (args[0].GetObject()->GetAsList() == nullptr) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `List` for `std::list::set(list, index, value)` but got `{}`", args[0].GetTypeName())
    );
  }

  if (args[1].GetType() != Value::Type::Int) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `Int` for `std::list::set(list, index, value)` but got `{}`", args[1].GetTypeName())
    );
  }

  auto l = args[0].GetObject()->GetAsList();
  (*l)[args[1].Get<std::int64_t>()] = std::move(args[2]);
  return Value();
}

static Value ListGetAtIndex(std::vector<Value>& args)
{
  if (args[0].GetObject()->GetAsList() == nullptr) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `List` for `std::list::get(list, index)` but got `{}`", args[0].GetTypeName())
    );
  }

  if (args[1].GetType() != Value::Type::Int) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `Int` for `std::list::get(list, index)` but got `{}`", args[1].GetTypeName())
    );
  }

  auto l = args[0].GetObject()->GetAsList();
  return (*l)[args[1].Get<std::int64_t>()];
}

static Value ListLength(std::vector<Value>& args)
{
  if (args[0].GetObject()->GetAsList() == nullptr) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `List` for `std::list::get(list, index)` but got `{}`", args[0].GetTypeName())
    );
  }

  return Value(static_cast<std::int64_t>(args[0].GetObject()->GetAsList()->Length()));
}

static Value DictionaryInsert(std::vector<Value>& args)
{
  if (args[0].GetObject()->GetAsDictionary() == nullptr) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `Dict` for `std::dict::insert(dict, key, value)` but got `{}`", args[0].GetTypeName())
    );
  }

  auto dict = args[0].GetObject()->GetAsDictionary();
  dict->Insert(std::move(args[1]), std::move(args[2]));
  return Value();
}

static Value DictionaryGet(std::vector<Value>& args)
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

static Value DictionaryContainsKey(std::vector<Value>& args)
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

static Value DictionaryRemove(std::vector<Value>& args)
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

static Value KeyValuePairKey(std::vector<Value>& args)
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

static Value KeyValuePairValue(std::vector<Value>& args)
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

static Value FileWrite(std::vector<Value>& args)
{
  if (args[0].GetType() == Value::Type::String) {
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
  if (args[0].GetType() == Value::Type::Int) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `Int` for `std::system::exit(exit_code)` but got `{}`", args[0].GetTypeName())
    );
  }

  std::exit(static_cast<int>(args[0].Get<std::int64_t>()));
}

static Value SystemRun(std::vector<Value>& args)
{
  if (args[0].GetType() == Value::Type::String) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `String` for `std::system::run(command)` but got `{}`", args[0].GetTypeName())
    );
  }

  auto res = std::system(args[0].Get<std::string>().c_str());
  return Value(std::int64_t(res));
}

static Value SystemPlatform(GRACE_MAYBE_UNUSED std::vector<Value>& args)
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

static Value DirectoryExists(std::vector<Value>& args)
{
  if (args[0].GetType() == Value::Type::String) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `String` for `std::directory::exists(path)` but got `{}`", args[0].GetTypeName())
    );
  }

  const auto& path = args[0].Get<std::string>();
  auto exists = std::filesystem::is_directory(path);
  return Value(exists);
}

static Value DirectoryCreate(std::vector<Value>& args)
{
  if (args[0].GetType() == Value::Type::String) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `String` for `std::directory::create(path)` but got `{}`", args[0].GetTypeName())
    );
  }

  const auto& path = args[0].Get<std::string>();
  auto result = std::filesystem::create_directories(path);
  return Value(result);
}

static Value InteropLoadLibrary(std::vector<Value>& args)
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

static Value InteropDoCall(std::vector<Value>& args)
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

static Value StringLength(std::vector<Value>& args)
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

static Value StringSplit(std::vector<Value>& args)
{
  if (args[0].GetType() != Value::Type::String) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `String` for `std::string::split(s, separator)` but got `{}`", args[0].GetTypeName())
    );
  }

  if (args[1].GetType() != Value::Type::String) {
    throw Grace::GraceException(
      Grace::GraceException::Type::InvalidType,
      fmt::format("Expected `String` for `std::string::split(s, separator)` but got `{}`", args[1].GetTypeName())
    );
  }

  const auto& s = args[0].Get<std::string>();
  const auto& separator = args[1].Get<std::string>();

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
  for (std::size_t i = 0; i < s.length(); i++) {
    if (s.substr(i, separator.length()) == separator) {
      list->Append(s.substr(lastSplit, i));
      lastSplit = i + separator.length();
    }
  }

  list->Append(s.substr(lastSplit));

  return res;
}
