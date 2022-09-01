/*
 *  The Grace Programming Language.
 *
 *  This file contains the GraceIterator class, which represents a value used to iterate over a collection in Grace, and the GraceIterable class, the base class for iterable objects in Grace.
 *
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#ifndef GRACE_ITERATOR_HPP
#define GRACE_ITERATOR_HPP

#include <vector>

#include <fmt/format.h>

#include "../grace.hpp"
#include "grace_exception.hpp"
#include "grace_object.hpp"
#include "object_tracker.hpp"
#include "../value.hpp"

namespace Grace
{
  class GraceIterable;

  class GraceIterator : public GraceObject
  {
    public:

      enum class IterableType
      {
        List,
        Dictionary,
      };

      using IteratorType = std::vector<VM::Value>::iterator;

      GraceIterator() = delete;
      GraceIterator(GraceIterable* iterable, IterableType type);

      ~GraceIterator() override;

      void Increment();

      GRACE_NODISCARD bool IsAtEnd() const;

      void Invalidate()
      {
        m_IsValid = false;
      }

      GRACE_INLINE void DebugPrint() const override
      {
        fmt::print("Iterator: {}\n", ToString());
      }

      void Print(bool err) const override;
      void PrintLn(bool err) const override;

      GRACE_NODISCARD std::string ToString() const override;

      GRACE_NODISCARD bool AsBool() const override
      {
        return !IsAtEnd();
      }

      GRACE_NODISCARD GRACE_INLINE constexpr std::string_view ObjectName() const override
      {
        return "Iterator";
      }

      GRACE_NODISCARD GRACE_INLINE constexpr bool IsIterable() const override
      {
        return false;
      }

      GRACE_NODISCARD GRACE_INLINE constexpr GraceObjectType ObjectType() const override
      {
        return GraceObjectType::Iterator;
      }

      GRACE_NODISCARD GRACE_INLINE GraceIterator* GetAsIterator() override
      {
        return this;
      }

      GRACE_NODISCARD const VM::Value& Value() const
      {
        if (!m_IsValid) {
          throw GraceException(
            GraceException::Type::InvalidIterator,
            "Iterator is no longer valid, due to either being incremented past the end of the collection or the collection being modified"
          );
        }
        return *m_Iterator;
      }

      GRACE_NODISCARD GRACE_INLINE IterableType GetType() const
      {
        return m_IterableType;
      }

    private:
      GraceIterable* m_Iterable;
      IteratorType m_Iterator;
      bool m_IsValid{};
      IterableType m_IterableType;
  };

  class GraceIterable : public GraceObject
  {
    public:

      using IteratorType = std::vector<VM::Value>::iterator;

      virtual IteratorType Begin() = 0;
      virtual IteratorType End() = 0;
      virtual void IncrementIterator(IteratorType&) const = 0;

      void AddIterator(GraceIterator* iterator)
      {
        m_ActiveIterators.push_back(iterator);
      }

      void RemoveIterator(GraceIterator* iterator)
      {
        auto it = std::find(m_ActiveIterators.begin(), m_ActiveIterators.end(), iterator);
        if (it == m_ActiveIterators.end()) {
#ifdef GRACE_DEBUG
          GRACE_ASSERT(false, "Trying to remove an iterator that wasn't added");
#endif
          return;
        }
        m_ActiveIterators.erase(it);
      }
      
    protected:
      void InvalidateIterators()
      {
        for (auto it : m_ActiveIterators) {
          it->Invalidate();
        }
      }

      std::vector<GraceIterator*> m_ActiveIterators;
  };
} // namespace Grace::VM

#endif  // ifndef GRACE_ITERATOR_HPP