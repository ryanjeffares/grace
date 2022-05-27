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

#ifndef GRACE_LIST_HPP
#define GRACE_LIST_HPP

#include <vector>

#include "grace_object.hpp"
#include "../value.hpp"

namespace Grace
{
  namespace VM
  {
    class GraceIterator;
  }

  class GraceList : public GraceObject
  {
    public:
      using Iterator = std::vector<VM::Value>::iterator;

      GraceList() = default;
      explicit GraceList(const VM::Value&);
      explicit GraceList(std::vector<VM::Value>&& items);
      GraceList(const GraceList& other, std::int64_t multiple);
      GraceList(const VM::Value& min, const VM::Value& max, const VM::Value& increment);

      GRACE_INLINE void Append(const VM::Value& value)
      {
        m_Data.push_back(value);
        InvalidateIterators();
      }
      
      template<typename T>
      constexpr GRACE_INLINE void Append(const T& value)
      {
        static_assert(std::is_same<T, std::int64_t>::value || std::is_same<T, double>::value
            || std::is_same<T, bool>::value || std::is_same<T, char>::value
            || std::is_same<T, std::string>::value || std::is_same<T, VM::Value::NullValue>::value,
            "Invalid type for Value<T>");
        m_Data.emplace_back(value);
        InvalidateIterators();
      }

      void Append(const std::vector<VM::Value>& items);
      void Remove(std::size_t index);

      GRACE_NODISCARD GRACE_INLINE std::size_t Length() const
      {
        return m_Data.size();
      }

      GRACE_NODISCARD GRACE_INLINE Iterator Begin()
      {
        return m_Data.begin();
      }

      GRACE_NODISCARD GRACE_INLINE Iterator End()
      {
        return m_Data.end();
      }

      void DebugPrint() const override;
      void Print() const override;
      void PrintLn() const override;
      GRACE_NODISCARD std::string ToString() const override;
      GRACE_NODISCARD bool AsBool() const override;
      GRACE_NODISCARD bool IsIteratable() const override;
      GRACE_NODISCARD std::string ObjectName() const override;
      
      GRACE_INLINE VM::Value& operator[](std::size_t index)
      {
        return m_Data[index];
      }

      GRACE_INLINE const VM::Value& operator[](std::size_t index) const
      {
        return m_Data[index];
      }

      void AddIterator(VM::GraceIterator* iterator);
      void RemoveIterator(VM::GraceIterator* iterator);
      void InvalidateIterators();

    private:
      std::vector<VM::Value> m_Data;
      std::vector<VM::GraceIterator*> m_ActiveIterators;
  };
} // namespace Grace

#endif  // ifndef GRACE_LIST_HPP
