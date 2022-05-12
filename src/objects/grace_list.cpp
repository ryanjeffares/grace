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
    m_Data.push_back(std::move(value));
  }
}

GraceList::GraceList(const Value& value, std::int64_t repeats)
{
  m_Data.insert(m_Data.begin(), repeats, value);
}

GraceList::GraceList(const GraceList& other, std::int64_t multiple)
{
  m_Data.reserve(other.m_Data.size() * multiple);
  for (auto i = 0; i < multiple; i++) {
    m_Data.insert(m_Data.begin() + (i * other.m_Data.size()), other.m_Data.begin(), other.m_Data.end());
  }
}

void GraceList::Append(const std::vector<Value>& items)
{
  m_Data.reserve(items.size());
  m_Data.insert(m_Data.end(), items.begin(), items.end());
}

void GraceList::Remove(std::size_t index)
{
  m_Data.erase(m_Data.begin() + index);
}

void GraceList::DebugPrint() const
{
  fmt::print("GraceList: [{}]\n", ToString());
}

void GraceList::Print() const
{
  fmt::print("[{}]", fmt::join(m_Data, ", "));
}

void GraceList::PrintLn() const
{
  fmt::print("[{}]\n", fmt::join(m_Data, ", "));
}

std::string GraceList::ToString() const
{
  return fmt::format("[{}]", fmt::join(m_Data, ", "));
}

bool GraceList::AsBool() const
{
  return !m_Data.empty();
}
