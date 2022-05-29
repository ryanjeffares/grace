/*
 *  The Grace Programming Language.
 *
 *  This file contains the GraceIterator class, which represents a value used to iterate over a collection in Grace.
 *
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#ifndef GRACE_ITERATOR_HPP
#define GRACE_ITERATOR_HPP

#include <variant>
#include <vector>

#include <fmt/format.h>

#include "../grace.hpp"
#include "grace_exception.hpp"
#include "grace_object.hpp"
#include "object_tracker.hpp"
#include "../value.hpp"

namespace Grace
{
  template<class IteratorType>
  class GraceIterable;

  template<class IteratorType>
  class GraceIterator : public GraceObject
  {
    public:
      GraceIterator() = delete;
      GraceIterator(GraceIterable<IteratorType>* iterable)
        : m_Iterable(iterable)
      {
        m_IsValid = true;
        m_Iterable->IncreaseRef();
        m_Iterable->AddIterator(this);
        m_Iterator = m_Iterable->Begin();
      }

      ~GraceIterator()
      {
        m_Iterable->RemoveIterator(this);

        if (m_Iterable->DecreaseRef() == 0) {
#ifdef GRACE_DEBUG
          ObjectTracker::StopTracking(m_Iterable);
#endif
          delete m_Iterable;
        }
      }

      void Increment()
      {
        if (!m_IsValid) {
          throw GraceException(
            GraceException::Type::InvalidIterator,
            "Iterator is no longer valid, due to either being incremented past the end of the collection or the collection being modified"
          );
        }
        m_Iterator++;
      }

      GRACE_NODISCARD bool IsAtEnd() const
      {
        return m_Iterator == m_Iterable->End();
      }

      void Invalidate()
      {
        m_IsValid = false;
      }

      void DebugPrint() const override
      {
        fmt::print("Iterator: {}\n", ToString());
      }

      void Print() const override
      {
        Deref().Print();
      }

      void PrintLn() const override
      {
        Deref().PrintLn();
      }

      GRACE_NODISCARD std::string ToString() const override
      {
        return Deref().AsString();
      }

      GRACE_NODISCARD bool AsBool() const override
      {
        return !IsAtEnd();
      }

      GRACE_NODISCARD std::string ObjectName() const override
      {
        return "Iterator";
      }

      VM::Value Deref() const override
      {
        if (!m_IsValid) {
          throw GraceException(
            GraceException::Type::InvalidIterator,
            "Iterator is no longer valid, due to either being incremented past the end of the collection or the collection being modified"
          );
        }
        return *m_Iterator;
      }

    private:
      GraceIterable<IteratorType>* m_Iterable;
      IteratorType m_Iterator;
      bool m_IsValid{};
  };

  template<class IteratorType>
  class GraceIterable : public GraceObject
  {
    public:
      virtual ~GraceIterable() = default;
      virtual IteratorType Begin() = 0;
      virtual IteratorType End() = 0;
      virtual void AddIterator(GraceIterator<IteratorType>*) = 0;
      virtual void RemoveIterator(GraceIterator<IteratorType>*) = 0;
      virtual void InvalidateIterators() = 0;
      
    protected:
      std::vector<GraceIterator<IteratorType>*> m_ActiveIterators;
  };
} // namespace Grace::VM

#endif  // ifndef GRACE_ITERATOR_HPP