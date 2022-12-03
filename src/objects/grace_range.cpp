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

#include "grace_range.hpp"

#include "grace_exception.hpp"

#include <fmt/core.h>
#include <fmt/format.h>

#include <cinttypes>

static constexpr std::size_t s_BaseCapacity = 8;

namespace Grace
{
  using namespace VM;

  GraceRange::GraceRange(Value&& min, Value&& max, Value&& increment)
    : GraceIterable{0}
    , m_Min{std::move(min)}
    , m_Max{std::move(max)}
    , m_Increment{std::move(increment)}
  {
    if (m_Min.GetType() != Value::Type::Int) {
      throw GraceException(
        GraceException::Type::InvalidType,
        fmt::format("All values in range expression must be `Ints`, got `{}` for min", m_Min.GetTypeName())
      );
    }

    if (m_Max.GetType() != Value::Type::Int) {
      throw GraceException(
        GraceException::Type::InvalidType,
        fmt::format("All values in range expression must be `Ints`, got `{}` for max", m_Max.GetTypeName())
      );
    }

    if (m_Increment.GetType() != Value::Type::Int) {
      throw GraceException(
        GraceException::Type::InvalidType,
        fmt::format("All values in range expression must be `Ints`, got `{}` for increment", m_Increment.GetTypeName())
      );
    }

    auto minVal = m_Min.Get<std::int64_t>();
    auto maxVal = m_Max.Get<std::int64_t>();
    auto incVal = m_Increment.Get<std::int64_t>();

    m_Direction = maxVal > minVal;

    auto delta = static_cast<std::size_t>(std::llabs(maxVal - minVal));
    auto size = delta < 8 ? delta : 8;

    m_Data.reserve(size);
    for (std::size_t i = 0; i < size; i++) {
      m_Data.emplace_back(minVal);
      minVal += incVal;
    }
  }

  GraceRange::~GraceRange()
  {
    InvalidateIterators();
  }

  void GraceRange::DebugPrint() const
  {
    fmt::print("Range: {}, current {}\n", ToString(), m_Data[m_Current]);
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

  void GraceRange::IncrementIterator(IteratorType& toIncrement)
  {
    if (m_Current == m_Data.size() - 1) {
      m_Current = 0;
      auto lastValue = toIncrement->Get<std::int64_t>();
      for (std::size_t i = 0; i < m_Data.size(); i++) {
        auto curr = lastValue + m_Increment.Get<std::int64_t>() * static_cast<std::int64_t>(i);
        m_Data[i] = curr;
      }
      toIncrement = m_Data.begin();
    } else {
      m_Current++;
      toIncrement++;
    }
  }

  bool GraceRange::IsAtEnd(GRACE_MAYBE_UNUSED const IteratorType& iterator) const
  {
    return m_Direction ? m_Data[m_Current] >= m_Max : m_Data[m_Current] <= m_Max;
  }
}