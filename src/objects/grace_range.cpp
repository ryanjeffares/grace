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

    m_Current = minVal;
    m_Direction = maxVal > minVal;

    auto delta = static_cast<std::size_t>(std::llabs(maxVal - minVal));
    auto size = delta < s_BaseCapacity ? delta : s_BaseCapacity;

    m_Data.reserve(size);
    for (std::size_t i = 0; i < size; i++) {
      m_Data.emplace_back(minVal);
      minVal += incVal;
    }
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

  void GraceRange::IncrementIterator(IteratorType& toIncrement)
  {
    toIncrement++;

    if (toIncrement == m_Data.end()) {
      if (m_Direction ? m_Data.back() < m_Max : m_Data.back() > m_Max) {
        // not at end yet, refill vector
        auto inc = m_Increment.Get<std::int64_t>();
        auto next = m_Data.back().Get<std::int64_t>() + inc;

        for (std::size_t i = 0; i < m_Data.size(); i++) {
          m_Data[i] = next + inc * static_cast<std::int64_t>(i);
        }

        toIncrement = m_Data.begin();
      }
    }

    if (m_Direction ? *toIncrement >= m_Max : *toIncrement <= m_Max) {
      toIncrement = m_Data.end();
    }
  }
}