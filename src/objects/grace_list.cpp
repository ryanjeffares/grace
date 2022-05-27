/*
 *  The Grace Programming Language.
 *
 *  This file contains the out of line definitions for the GraceList class, the underlying C++ class for Lists in Grace
 *    
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#include <fmt/core.h>
#include <fmt/format.h>

#include "grace_iterator.hpp"
#include "grace_list.hpp"

using namespace Grace;
using namespace Grace::VM;

GraceList::GraceList(std::vector<Value>&& items)
  : m_Data(std::move(items))
{
}

GraceList::GraceList(const Value& value)
{
  if (value.GetType() == Value::Type::String) {
    for (const auto c : value.Get<std::string>()) {
      m_Data.emplace_back(c);
    }
  } else {
    m_Data.push_back(value);
  }
}

GraceList::GraceList(const GraceList& other, std::int64_t multiple)
{
  m_Data.reserve(other.m_Data.size() * multiple);
  for (std::size_t i = 0; i < static_cast<std::size_t>(multiple); i++) {
    m_Data.insert(m_Data.begin() + (i * other.m_Data.size()), other.m_Data.begin(), other.m_Data.end());
  }
}

GraceList::GraceList(const Value& min, const Value& max, const Value& increment)
{
  auto useDouble = false;
  if (min.GetType() == Value::Type::Double || max.GetType() == Value::Type::Double
    || increment.GetType() == Value::Type::Double) {
    useDouble = true;
  }

  if (useDouble) {
    auto minVal = min.GetType() == Value::Type::Double ? min.Get<double>() : static_cast<double>(min.Get<std::int64_t>());
    auto maxVal = max.GetType() == Value::Type::Double ? max.Get<double>() : static_cast<double>(max.Get<std::int64_t>());
    auto incVal = increment.GetType() == Value::Type::Double ? increment.Get<double>() : static_cast<double>(increment.Get<std::int64_t>());

    for (auto i = minVal; i < maxVal; i += incVal) {
      m_Data.emplace_back(i);
    }
  } else {
    for (auto i = min.Get<std::int64_t>(); i < max.Get<std::int64_t>(); i += increment.Get<std::int64_t>()) {
      m_Data.emplace_back(i);
    }
  }
}

GraceList::~GraceList()
{
  for (auto it : m_ActiveIterators) {
    it->Invalidate();
  }
}

void GraceList::Append(const std::vector<Value>& items)
{
  m_Data.reserve(items.size());
  m_Data.insert(m_Data.end(), items.begin(), items.end());
  InvalidateIterators();
}

void GraceList::Remove(std::size_t index)
{
  m_Data.erase(m_Data.begin() + index);
}

void GraceList::DebugPrint() const
{
  fmt::print("GraceList: {}\n", ToString());
}

void GraceList::Print() const
{
  fmt::print("{}", ToString());
}

void GraceList::PrintLn() const
{
  fmt::print("{}\n", ToString());
}

std::string GraceList::ToString() const
{
  std::string res = "[";
  for (std::size_t i = 0; i < m_Data.size(); i++) {
    const auto& el = m_Data[i];
    switch (el.GetType()) {
      case Value::Type::Char:
        res.push_back('\'');
        res.push_back(el.Get<char>());
        res.push_back('\'');
        break;
      case Value::Type::String:
        res.push_back('"');
        res.append(el.Get<std::string>());
        res.push_back('"');
        break;
      default:
        res.append(el.AsString());
        break;
    }
    if (i < m_Data.size() - 1) {
      res.append(", ");
    }
  }
  res.push_back(']');
  return res;
}

bool GraceList::AsBool() const
{
  return !m_Data.empty();
}

std::string GraceList::ObjectName() const
{
  return "List";
}

bool GraceList::IsIteratable() const
{
  return true;
}

void GraceList::AddIterator(GraceIterator* iterator)
{
  m_ActiveIterators.push_back(iterator);
}

void GraceList::RemoveIterator(GraceIterator* iterator)
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

void GraceList::InvalidateIterators()
{
  for (auto it : m_ActiveIterators) {
    it->Invalidate();
  }
}