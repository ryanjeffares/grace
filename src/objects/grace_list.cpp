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
    m_Data.push_back(value);
  }
}

GraceList::GraceList(const Value& value, std::int64_t repeats)
{
  m_Data.insert(m_Data.begin(), repeats, value);
}

GraceList::GraceList(const GraceList& other, std::int64_t multiple)
{
  m_Data.reserve(other.m_Data.size() * multiple);
  for (std::size_t i = 0; i < static_cast<std::size_t>(multiple); i++) {
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
