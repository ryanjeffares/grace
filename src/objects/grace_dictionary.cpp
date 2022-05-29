/*
 *  The Grace Programming Language.
 *
 *  This file contains the out of line definitions for the GraceDictionary class, the underlying class for Dictionaries in Grace.
 *
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#include <fmt/core.h>

#include "grace_dictionary.hpp"

namespace Grace
{
  GraceDictionary::GraceDictionary(Dictionary&& dict)
    : m_Dict(std::move(dict))
  {

  }

  void GraceDictionary::DebugPrint() const
  {
    fmt::print("Dictionary: {}\n", ToString());
  }

  void GraceDictionary::Print() const
  {
    fmt::print("{}", ToString());
  }

  void GraceDictionary::PrintLn() const
  {
    fmt::print("{}\n", ToString());
  }

  std::string GraceDictionary::ToString() const
  {
    std::string res = "{";
    std::size_t index = 0;
    for (const auto& [key, value] : m_Dict) {
      res.append("{");
      res.append(key.AsString());
      res.append(": ");
      res.append(value.AsString());
      res.append("}");
      if (index++ < m_Dict.size() - 1) {
        res.append(", ");
      }
    }
    res.append("}");
    return res;
  }

  bool GraceDictionary::AsBool() const
  {
    return !m_Dict.empty();
  }
} // namespace Grace