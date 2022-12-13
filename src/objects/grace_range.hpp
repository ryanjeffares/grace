/*
 *  The Grace Programming Language.
 *
 *  This file contains the GraceRange class, the underlying class for Ranges in Grace.
 *
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#ifndef GRACE_RANGE_HPP
#define GRACE_RANGE_HPP

#include "grace_iterator.hpp"
#include "../vm/value.hpp"

namespace Grace
{
  class GraceRange : public GraceIterable
  {
  public:
    GraceRange(VM::Value&& min, VM::Value&& max, VM::Value&& increment);
    ~GraceRange() override = default;

    void DebugPrint() const override;
    void Print(bool err) const override;
    void PrintLn(bool err) const override;
    GRACE_NODISCARD std::string ToString() const override;
    GRACE_NODISCARD bool AsBool() const override;

    GRACE_NODISCARD GRACE_INLINE constexpr std::string_view ObjectName() const override
    {
      return "Range";
    }

    GRACE_NODISCARD GRACE_INLINE constexpr bool IsIterable() const override
    {
      return true;
    }

    GRACE_NODISCARD GRACE_INLINE constexpr GraceObjectType ObjectType() const override
    {
      return GraceObjectType::Range;
    }

    GRACE_NODISCARD GRACE_INLINE GraceRange* GetAsRange() override
    {
      return this;
    }

    GRACE_NODISCARD GRACE_INLINE IteratorType Begin() override
    {
      return m_Data.begin();
    }

    GRACE_NODISCARD GRACE_INLINE IteratorType End() override
    {
      return m_Data.end();
    }

    void IncrementIterator(IteratorType& toIncrement) override;

  private:
    bool m_Direction;
    VM::Value m_Current;
    VM::Value m_Min, m_Max, m_Increment;
  };
} // namespace Grace

#endif // ifndef GRACE_RANGE_HPP