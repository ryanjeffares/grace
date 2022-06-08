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

#include "grace_exception.hpp"
#include "grace_iterator.hpp"
#include "../value.hpp"

namespace Grace
{
  class GraceList : public GraceIterable<std::vector<VM::Value>::iterator>
  {
    public:
      using Iterator = std::vector<VM::Value>::iterator;

      GraceList() = default;
      explicit GraceList(const VM::Value&);
      explicit GraceList(std::vector<VM::Value>&& items);
      GraceList(const GraceList& other, std::int64_t multiple);
      GraceList(const VM::Value& min, const VM::Value& max, const VM::Value& increment);

      ~GraceList() override;

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

      GRACE_NODISCARD GRACE_INLINE Iterator Begin() override
      {
        return m_Data.begin();
      }

      GRACE_NODISCARD GRACE_INLINE Iterator End() override
      {
        return m_Data.end();
      }

      GRACE_INLINE void IncrementIterator(Iterator& toIncrement) const override
      {
        toIncrement++;
      }

      void DebugPrint() const override;
      void Print() const override;
      void PrintLn() const override;
      GRACE_NODISCARD std::string ToString() const override;
      GRACE_NODISCARD bool AsBool() const override;

      GRACE_NODISCARD GRACE_INLINE constexpr const char* ObjectName() const override
      {
        return "List";
      }

      GRACE_NODISCARD GRACE_INLINE constexpr bool IsIterable() const override
      {
        return true;
      }
      
      GRACE_INLINE VM::Value& operator[](std::size_t index)
      {
        if (index >= m_Data.size()) {
          throw GraceException(
            GraceException::Type::IndexOutOfRange,
            fmt::format("Given index is {} but the length of the List is {}", index, m_Data.size())
          );
        }
        return m_Data[index];
      }

      GRACE_INLINE const VM::Value& operator[](std::size_t index) const
      {
        if (index >= m_Data.size()) {
          throw GraceException(
            GraceException::Type::IndexOutOfRange,
            fmt::format("Given index is {} but the length of the List is {}", index, m_Data.size())
          );
        }
        return m_Data[index];
      }

    private:
      std::vector<VM::Value> m_Data;
  };
} // namespace Grace

#endif  // ifndef GRACE_LIST_HPP
