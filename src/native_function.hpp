/*
 *  The Grace Programming Language.
 *
 *  This file contains the NativeFunction class, which represents a function that can be called from Grace and executed within C++.
 *    
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#ifndef GRACE_NATIVE_FUNCTION_HPP
#define GRACE_NATIVE_FUNCTION_HPP

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "grace.hpp"
#include "value.hpp"

namespace Grace::Native
{
  class NativeFunction
  {
  public:
    // the arg vector is not const so that we can efficiently std::move the arguments to inner calls to Lists, Dicts etc
    using FuncDecl = VM::Value (*)(std::vector<VM::Value>&);

    NativeFunction(std::string&& name, std::uint32_t arity, FuncDecl func)
        : m_Name(std::move(name))
        , m_Arity(arity)
        , m_Function(func)
    {
    }

    GRACE_NODISCARD GRACE_INLINE const std::string& GetName() const
    {
      return m_Name;
    }

    GRACE_NODISCARD GRACE_INLINE std::uint32_t GetArity() const
    {
      return m_Arity;
    }

    /*
       *  Call the inner function. The length of the args list is expected to match the function's arity,
       *  and the types are expected to be correct
       */
    VM::Value operator()(std::vector<VM::Value>& args)
    {
      return m_Function(args);
    }

  private:
    std::string m_Name;
    std::uint32_t m_Arity;
    FuncDecl m_Function;
  };
}// namespace Grace::Native

#endif// ifndef GRACE_NATIVE_FUNCTION_HPP
