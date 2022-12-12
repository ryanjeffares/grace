/*
 *  The Grace Programming Language.
 *
 *  This file contains the out of line definitions for the GraceFunction class, the underlying class for runtime functions in Grace.
 *
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#include "grace_function.hpp"

namespace Grace
{
  static std::hash<std::string> s_Hasher;

  GraceFunction::GraceFunction(std::string name, std::size_t arity, std::string fileName, bool exported)
      : m_Name {std::move(name)}
      , m_Arity {arity}
      , m_FileName {std::move(fileName)}
      , m_FileNameHash {static_cast<std::int64_t>(s_Hasher(m_FileName))}
      , m_Exported {exported}
  {

  }

  void GraceFunction::DebugPrint() const
  {
    fmt::print("Function: {}\n", ToString());
  }

  void GraceFunction::Print(bool err) const
  {
    auto stream = err ? stdout : stderr;
    fmt::print(stream, "{}", ToString());
  }

  void GraceFunction::PrintLn(bool err) const
  {
    auto stream = err ? stdout : stderr;
    fmt::print(stream, "{}\n", ToString());
  }

  GRACE_NODISCARD std::string GraceFunction::ToString() const
  {
    return fmt::format("<Function {} defined in {}>", m_Name, m_FileName);
  }

  void GraceFunction::CombineOps(std::vector<VM::OpLine>& toFill)
  {
    m_OpIndexStart = toFill.size();
    toFill.reserve(m_OpList.size());
    toFill.insert(toFill.end(), m_OpList.begin(), m_OpList.end());
  }

  void GraceFunction::CombineConstants(std::vector<VM::Value>& toFill)
  {
    m_ConstantIndexStart = toFill.size();
    toFill.reserve(m_ConstantList.size());
    toFill.insert(toFill.end(), m_ConstantList.begin(), m_ConstantList.end());
  }

  void GraceFunction::PrintOps() const
  {
    for (auto [op, line] : m_OpList) {
      fmt::print("{:>5} | {}\n", line, op);
    }
  }
} // namespace Grace