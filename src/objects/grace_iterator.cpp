/*
 *  The Grace Programming Language.
 *
 *  This file contains the out of line definitions for the GraceIterator class, which represents a value used to iterate over a collection in Grace.
 *
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#include "grace_iterator.hpp"

namespace Grace
{
  GraceIterator::GraceIterator(GraceIterable* iterable, IterableType type)
    : m_Iterable(iterable),
    m_IterableType(type)
  {
    m_IsValid = true;
    m_Iterable->IncreaseRef();
    m_Iterable->AddIterator(this);
    m_Iterator = m_Iterable->Begin();
  }

  GraceIterator::~GraceIterator()
  {
    m_Iterable->RemoveIterator(this);

    if (m_Iterable->DecreaseRef() == 0) {
      ObjectTracker::StopTrackingObject(m_Iterable);
      delete m_Iterable;
    }
  }

  void GraceIterator::Increment()
  {
    if (!m_IsValid) {
      throw GraceException(
        GraceException::Type::InvalidIterator,
        "Iterator is no longer valid, due to either being incremented past the end of the collection or the collection being modified"
      );
    }
    m_Iterable->IncrementIterator(m_Iterator);
  }

  bool GraceIterator::IsAtEnd() const
  {
    return m_Iterable->IsAtEnd(m_Iterator);
  }

  void GraceIterator::Print(bool err) const
  {
    if (m_Iterator == m_Iterable->End()) {
      fmt::print(err ? stderr : stdout, "null");
    } else {
      Value().Print(err);
    }
  }

  void GraceIterator::PrintLn(bool err) const
  {
    if (m_Iterator == m_Iterable->End()) {
      fmt::print(err ? stderr : stdout, "null\n");
    } else {
      Value().PrintLn(err);
    }
  }

  std::string GraceIterator::ToString() const
  {
    if (m_Iterator == m_Iterable->End()) {
      return "null";
    }
    return Value().AsString();
  }
} // namespace Grace
