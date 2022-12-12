/*
 *  The Grace Programming Language.
 *
 *  This file contains the VM class, which executes compiled Grace bytecode.
 *
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#ifndef GRACE_VM_HPP
#define GRACE_VM_HPP

#include <cstdint>
#include <exception>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <fmt/color.h>
#include <fmt/core.h>

#include "grace.hpp"
#include "native_function.hpp"
#include "objects/grace_exception.hpp"
#include "objects/grace_function.hpp"
#include "value.hpp"
#include "ops.hpp"

namespace Grace::VM
{
  enum class InterpretResult
  {
    RuntimeOk,
    RuntimeError,
  };

  // TODO: there will be user defined objects that can have extension methods...
  enum class ObjectType
  {
    Bool,
    Char,
    Dict,
    Float,
    Int,
    List,
    String,
  };

  class VM
  {
  public:

    static VM& Instance()
    {
      static VM vm;
      return vm;
    }

    GRACE_INLINE void PushOp(Ops op, std::size_t line)
    {
      m_FunctionLookup.at(m_LastFileNameHash).at(m_LastFunctionHash).GetObject()->GetAsFunction()->PushOp(op, line);
    }

    void PrintOps();

    template<BuiltinGraceType T>
    GRACE_INLINE void PushConstant(T value)
    {
      m_FunctionLookup.at(m_LastFileNameHash).at(m_LastFunctionHash).GetObject()->GetAsFunction()->PushConstant(std::move(value));
    }

    GRACE_INLINE void PushConstant(Value value)
    {
      m_FunctionLookup.at(m_LastFileNameHash).at(m_LastFunctionHash).GetObject()->GetAsFunction()->PushConstant(std::move(value));
    }

    GRACE_NODISCARD GRACE_INLINE std::size_t GetNumConstants()
    {
      return m_FunctionLookup.at(m_LastFileNameHash).at(m_LastFunctionHash).GetObject()->GetAsFunction()->GetNumConstants();
    }

    GRACE_NODISCARD GRACE_INLINE std::size_t GetNumOps()
    {
      return m_FunctionLookup.at(m_LastFileNameHash).at(m_LastFunctionHash).GetObject()->GetAsFunction()->GetNumOps();
    }

    template<BuiltinGraceType T>
    GRACE_INLINE void SetConstantAtIndex(std::size_t index, T value)
    {
      m_FunctionLookup.at(m_LastFileNameHash).at(m_LastFunctionHash).GetObject()->GetAsFunction()->SetConstantAtIndex(index, std::move(value));
    }

    GRACE_NODISCARD GRACE_INLINE std::optional<Ops> GetLastOp()
    {
      return m_FunctionLookup.at(m_LastFileNameHash).at(m_LastFunctionHash).GetObject()->GetAsFunction()->GetLastOp();      
    }

    GRACE_NODISCARD const std::string& GetLastFunctionName()
    {
      return m_FunctionLookup.at(m_LastFileNameHash).at(m_LastFunctionHash).GetObject()->GetAsFunction()->GetName();
    }

    GRACE_NODISCARD bool AddFunction(std::string name, std::size_t arity, std::string fileName, bool exported, bool extension, std::size_t objectNameHash = {});
    GRACE_NODISCARD bool AddClass(std::string name, std::string fileName);

    GRACE_NODISCARD std::tuple<bool, std::size_t> HasNativeFunction(const std::string& name)
    {
      auto it = std::find_if(m_NativeFunctions.begin(), m_NativeFunctions.end(), [&name](const Native::NativeFunction& fn) { return fn.GetName() == name; });
      if (it == m_NativeFunctions.end()) {
        return { false, 0 };
      }
      return { true, it - m_NativeFunctions.begin() };
    }

    GRACE_NODISCARD GRACE_INLINE const Native::NativeFunction& GetNativeFunction(std::size_t index)
    {
      return m_NativeFunctions[index];
    }

    GRACE_NODISCARD bool CombineFunctions(const std::string& mainFileName, GRACE_MAYBE_UNUSED bool verbose);
    GRACE_NODISCARD InterpretResult Start(const std::string& mainFileName, bool verbose, const std::vector<std::string>& args);

  protected:

    VM()
    {
      RegisterNatives();
    }

  private:
    
    struct CallStackEntry
    {
      std::int64_t callerHash {}, calleeHash {};
      std::size_t line {};
      std::string fileName, calleeFileName;
      std::int64_t fileNameHash {}, calleeFileNameHash {};
    };

    void RegisterNatives();

    GRACE_NODISCARD InterpretResult Run(std::int64_t mainFileNameHash, GRACE_MAYBE_UNUSED bool verbose, const std::vector<std::string>& clArgs);
    void RuntimeError(const GraceException& exception, std::size_t line, const std::vector<CallStackEntry>& callStack);        

    // { filename { function name, function } }
    std::unordered_map<std::int64_t, std::unordered_map<std::int64_t, Value>> m_FunctionLookup;
    // { hash of object name, list of functions }
    std::unordered_map<std::size_t, std::vector<Value>> m_ExtensionMethodLookup;

    std::vector<Native::NativeFunction> m_NativeFunctions;
    
    // { filename { function name, class name } }
    std::unordered_map<std::int64_t, std::unordered_map<std::int64_t, std::string>> m_ClassLookup;

    std::unordered_map<std::int64_t, std::string> m_FileNameLookup;

    std::vector<OpLine> m_FullOpList;
    std::vector<Value> m_FullConstantList;

    std::int64_t m_LastFileNameHash;
    std::int64_t m_LastFunctionHash;
    std::hash<std::string> m_Hasher;
  };
} // namespace Grace::VM

#endif // ifndef GRACE_VM_HPP
