/*
 *  The Grace Programming Language.
 *
 *  This file contains the GraceList class, the underlying C++ class for Lists in Grace
 *    
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#pragma once

#include <vector>

#include "grace_object.hpp"
#include "../value.hpp"

namespace Grace
{
  class GraceList : public GraceObject
  {
    public:
      GraceList() = default;
      GraceList(std::vector<VM::Value>&& items);
      GraceList(const GraceList& other, std::int64_t multiple);

      GRACE_INLINE void Append(const VM::Value& value)
      {
        m_Data.push_back(value);
      }
      
      template<typename T>
      constexpr GRACE_INLINE void Append(const T& value)
      {
        static_assert(std::is_same<T, std::int64_t>::value || std::is_same<T, double>::value
            || std::is_same<T, bool>::value || std::is_same<T, char>::value
            || std::is_same<T, std::string>::value || std::is_same<T, VM::Value::NullValue>::value,
            "Invalid type for Value<T>");
        m_Data.emplace_back(value);
      }

      void Append(const std::vector<VM::Value>& items);
      void Remove(std::size_t index);

      void DebugPrint() const override;
      void Print() const override;
      void PrintLn() const override;
      std::string ToString() const override;
      bool AsBool() const override; 

      GRACE_INLINE VM::Value& operator[](std::size_t index)
      {
        return m_Data[index];
      }

      GRACE_INLINE const VM::Value& operator[](std::size_t index) const
      {
        return m_Data[index];
      }

    private:
      std::vector<VM::Value> m_Data;
  };
}