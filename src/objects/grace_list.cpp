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

GraceList::GraceList()
{
}

GraceList::GraceList(std::vector<Value>&& items)
  : m_Data(std::move(items))
{
}

void GraceList::Insert(const std::vector<Value>& items)
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
  fmt::print("GraceList: [{}]", ToString());
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
