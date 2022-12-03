/*
 *  The Grace Programming Language.
 *
 *  This file contains the out of line definitions for the GraceRange class, the underlying class for Ranges in Grace.
 *
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#include <fmt/core.h>
#include <fmt/format.h>

#include "grace_range.hpp"
#include "grace_exception.hpp"

namespace Grace
{
  using namespace VM;

  GraceRange::GraceRange(Value&& min, Value&& max, Value&& increment)
    : GraceIterable{8}
    , m_Min(std::move(min))
    , m_Max(std::move(max))
    , m_Increment(std::move(increment))
  {
    if (!m_Min.IsNumber()) {
      throw GraceException(
        GraceException::Type::InvalidType,
        fmt::format("All values in range expression must be numbers, got `{}` for min", m_Min.GetTypeName())
      );
    }

    if (!m_Max.IsNumber()) {
      throw GraceException(
        GraceException::Type::InvalidType,
        fmt::format("All values in range expression must be numbers, got `{}` for max", m_Max.GetTypeName())
      );
    }

    if (!m_Increment.IsNumber()) {
      throw GraceException(
        GraceException::Type::InvalidType,
        fmt::format("All values in range expression must be numbers, got `{}` for increment", increment.GetTypeName())
      );
    }

    bool useDouble = m_Min.GetType() == Value::Type::Double || max.GetType() == Value::Type::Double || m_Increment.GetType() == Value::Type::Double;

    if (useDouble) {
      auto minVal = m_Min.GetType() == Value::Type::Double ? m_Min.Get<double>() : static_cast<double>(m_Min.Get<std::int64_t>());
      auto maxVal = m_Max.GetType() == Value::Type::Double ? m_Max.Get<double>() : static_cast<double>(m_Max.Get<std::int64_t>());
      auto incVal = m_Increment.GetType() == Value::Type::Double ? m_Increment.Get<double>() : static_cast<double>(m_Increment.Get<std::int64_t>());

      auto numElements = static_cast<std::size_t>(maxVal / incVal);
      auto vecSize = numElements < 8 ? numElements : 8;

      m_Data.reserve(vecSize);
      for (auto i = 0; i < vecSize; i++) {
        m_Data.emplace_back(minVal);
        minVal += incVal;
      }
    } else {
      auto minVal = m_Min.Get<std::int64_t>();
      auto maxVal = m_Max.Get<std::int64_t>();
      auto incVal = m_Increment.Get<std::int64_t>();

      auto numElements = static_cast<std::size_t>(maxVal / incVal);
      auto vecSize = numElements < 8 ? numElements : 8;

      m_Data.reserve(vecSize);
      for (auto i = 0; i < vecSize; i++) {
        m_Data.emplace_back(minVal);
        minVal += incVal;
      }
    }
  }

  GraceRange::~GraceRange()
  {
    InvalidateIterators();
  }

  void GraceRange::DebugPrint() const
  {
    fmt::print("Range: {}, current {}\n", ToString(), m_Current);
  }

  void GraceRange::Print(bool err) const
  {
    auto stream = err ? stdout : stderr;
    fmt::print(stream, "{}", ToString());
  }

  void GraceRange::PrintLn(bool err) const
  {
    auto stream = err ? stdout : stderr;
    fmt::print(stream, "{}\n", ToString());
  }

  GRACE_NODISCARD std::string GraceRange::ToString() const
  {
    return fmt::format("[{}..{} by {}]", m_Min, m_Max, m_Increment);
  }
  
  GRACE_NODISCARD bool GraceRange::AsBool() const
  {
    return true;
  }

  void GraceRange::IncrementIterator(GRACE_MAYBE_UNUSED GraceRange::IteratorType& toIncrement) const
  {
    if (m_Current == m_Data.size() - 1) {

    } else {

    }
  }
}