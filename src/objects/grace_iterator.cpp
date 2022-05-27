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

#include <fmt/format.h>

#include "grace_exception.hpp"
#include "grace_iterator.hpp"
#include "object_tracker.hpp"

using namespace Grace::VM;

GraceIterator::GraceIterator(GraceObject* object)
  : m_IsValid(true)
{
  if (!object->IsIteratable()) {
    throw GraceException(
      GraceException::Type::InvalidType,
      fmt::format("Object {} is not iteratable", object->ObjectName())
    );
  }

  m_Iterable = object;
  m_Iterable->IncreaseRef();

  if (auto list = dynamic_cast<GraceList*>(m_Iterable)) {
    m_Iterator.emplace<0>(list->Begin());
    list->AddIterator(this);
  }
}

GraceIterator::~GraceIterator()
{
  if (auto list = dynamic_cast<GraceList*>(m_Iterable)) {
    m_Iterator.emplace<0>(list->Begin());
    list->RemoveIterator(this);
  }

  if (m_Iterable->DecreaseRef() == 0) {
#ifdef GRACE_DEBUG
    ObjectTracker::StopTracking(m_Iterable);
#endif
    delete m_Iterable;
  }
}

Value GraceIterator::GetValue() const
{
  if (!m_IsValid) {
    throw GraceException(
      GraceException::Type::InvalidIterator,
      "Iterator is no longer valid, due to either being incremented past the end of the collection or the collection being modified"
    );
  }
  switch (m_Iterator.index()) {
    case 0:
      return *std::get<0>(m_Iterator);
    case 1:
      return Value(*(std::get<1>(m_Iterator)));
    default:
      throw std::bad_variant_access();
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
  switch (m_Iterator.index()) {
    case 0:
      std::get<0>(m_Iterator)++;
      break;
    case 1:
      std::get<1>(m_Iterator)++;
      break;
    default:
      throw std::bad_variant_access();
  }
}

void GraceIterator::Invalidate()
{
  m_IsValid = false;
}

bool GraceIterator::IsAtEnd() const
{
  switch (m_Iterator.index()) {
    case 0: {
      auto it = std::get<0>(m_Iterator);
      auto end = dynamic_cast<GraceList*>(m_Iterable)->End();
      return it == end;
    }
    default:
      throw std::bad_variant_access();
  }
}

void GraceIterator::DebugPrint() const
{
  fmt::print("Iterator: {}\n", ToString());
}

void GraceIterator::Print() const
{
  GetValue().Print();
}

void GraceIterator::PrintLn() const
{
  GetValue().PrintLn();
}

GRACE_NODISCARD std::string GraceIterator::ToString() const
{
  return GetValue().AsString();
}

GRACE_NODISCARD bool GraceIterator::AsBool() const
{
  return !IsAtEnd();
}

GRACE_NODISCARD bool GraceIterator::IsIteratable() const
{
  return GetValue().GetType() == Value::Type::Object && GetValue().GetObject()->IsIteratable();
}

GRACE_NODISCARD std::string GraceIterator::ObjectName() const
{
  return "Iterator";
}