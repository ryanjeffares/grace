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

namespace Grace
{
  namespace Native
  {
    class NativeFunction
    {
      public:

        using FuncDecl = std::function<VM::Value(const std::vector<VM::Value>&)>;

        NativeFunction(std::string&& name, std::uint32_t arity, FuncDecl&& func)
          : m_Name(std::move(name)), m_Arity(arity), m_Function(std::move(func))
        {

        }
        
        GRACE_INLINE const std::string& GetName() const
        {
          return m_Name;
        }

        GRACE_INLINE std::uint32_t GetArity() const
        {
          return m_Arity;
        }

        /*
         *  Call the inner function. The length of the args list is expected to match the function's arity
         */
        VM::Value operator()(const std::vector<VM::Value>& args) 
        {
          return m_Function(args); 
        }

      private:

        std::string m_Name;
        std::uint32_t m_Arity;
        FuncDecl m_Function;
    };
  }
}

#endif  // ifndef GRACE_NATIVE_FUNCTION_HPP
