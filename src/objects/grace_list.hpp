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

#include <mutex>
#include <vector>

#include "grace_exception.hpp"
#include "grace_iterator.hpp"
#include "../value.hpp"

namespace Grace
{
  class GraceList : public GraceIterable
  {
    public:

      GraceList() = default;
      explicit GraceList(const VM::Value&);
      explicit GraceList(std::vector<VM::Value>&& items);
      GraceList(const GraceList& other, std::int64_t multiple);
      GraceList(const VM::Value& min, const VM::Value& max, const VM::Value& increment);

      ~GraceList() override;

      
      template<VM::BuiltinGraceType T>
      GRACE_INLINE void Append(const T& value)
      {
        m_Data.emplace_back(value);
        InvalidateIterators();
      }

      void Append(const VM::Value& value);
      void Append(const std::vector<VM::Value>& items);
      void Remove(std::size_t index);

      GRACE_NODISCARD GRACE_INLINE std::size_t Length() const
      {
        return m_Data.size();
      }

      GRACE_NODISCARD GRACE_INLINE IteratorType Begin() override
      {
        return m_Data.begin();
      }

      GRACE_NODISCARD GRACE_INLINE IteratorType End() override
      {
        return m_Data.end();
      }

      GRACE_INLINE void IncrementIterator(IteratorType& toIncrement) const override
      {
        toIncrement++;
      }

      void DebugPrint() const override;
      void Print(bool err) const override;
      void PrintLn(bool err) const override;
      GRACE_NODISCARD std::string ToString() const override;
      GRACE_NODISCARD bool AsBool() const override;      

      GRACE_NODISCARD GRACE_INLINE constexpr std::string_view ObjectName() const override
      {
        return "List";
      }

      GRACE_NODISCARD GRACE_INLINE constexpr bool IsIterable() const override
      {
        return true;
      }

      GRACE_NODISCARD GRACE_INLINE constexpr GraceObjectType ObjectType() const override
      {
        return GraceObjectType::List;
      }

      GRACE_NODISCARD GRACE_INLINE GraceList* GetAsList() override
      {
        return this;
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

      GRACE_NODISCARD bool AnyMemberMatches(const GraceObject* match) const override;
      GRACE_NODISCARD std::vector<GraceObject*> GetObjectMembers() const override;
      void RemoveMember(GraceObject* object) override;
      GRACE_NODISCARD bool OnlyReferenceIsSelf() const override;

    private:
      std::vector<VM::Value> m_Data;
  };
} // namespace Grace

#endif  // ifndef GRACE_LIST_HPP
