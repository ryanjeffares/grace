/*
 *  The Grace Programming Language.
 *
 *  This file contains the GraceHashable class, an interface for hashable collections in Grace
 *
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#ifndef GRACE_HASHABLE_HPP
#define GRACE_HASHABLE_HPP

#include "grace_iterator.hpp"

namespace Grace
{
  class GraceHashable : public GraceIterable
  {
  public:

    GRACE_NODISCARD std::vector<VM::Value> ToVector() const
    {
      std::vector<VM::Value> res;
      res.reserve(m_Size);

      for (auto& value : m_Data) {
        if (value.GetType() != VM::Value::Type::Null) {
          res.push_back(value);
        }
      }

      return res;
    }

  protected:
    static constexpr std::size_t s_InitialCapacity = 8;
    static constexpr float s_GrowFactor = 0.75f;

    GraceHashable(const VM::Value defaultValue = VM::Value())
      : GraceIterable{s_InitialCapacity, defaultValue}
      , m_CellStates{s_InitialCapacity, CellState::NeverUsed}
    {

    }

    virtual void Rehash() = 0;

    enum class CellState
    {
      NeverUsed, Tombstone, Occupied
    };

    std::size_t m_Size{0};
    std::size_t m_Capacity = s_InitialCapacity;

    std::vector<CellState> m_CellStates;
    std::hash<VM::Value> m_Hasher;
  };
} // namespace Grace


#endif  // ifndef GRACE_HASHABLE_HPP