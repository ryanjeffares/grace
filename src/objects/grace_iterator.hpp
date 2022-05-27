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

#include "../grace.hpp"
#include "grace_list.hpp"
#include "grace_object.hpp"
#include "../value.hpp"

namespace Grace::VM
{
  class GraceIterator : public GraceObject
  {
    public:
      GraceIterator() = delete;
      GraceIterator(GraceObject*);
      ~GraceIterator();

      GRACE_NODISCARD Value GetValue() const;

      void Increment();
      GRACE_NODISCARD bool IsAtEnd() const;

      void Invalidate();

      void DebugPrint() const override;
      void Print() const override;
      void PrintLn() const override;
      GRACE_NODISCARD std::string ToString() const override;
      GRACE_NODISCARD bool AsBool() const override;
      GRACE_NODISCARD bool IsIteratable() const override;
      GRACE_NODISCARD std::string ObjectName() const override;

    private:
      GraceObject* m_Iterable;
      std::variant<
        GraceList::Iterator,
        std::string::iterator
      > m_Iterator;
      bool m_IsValid{};
  };
} // namespace Grace::VM

#endif  // ifndef GRACE_ITERATOR_HPP