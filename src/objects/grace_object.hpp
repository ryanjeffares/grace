/*
 *  The Grace Programming Language.
 *
 *  This file contains the GraceObject class, the base class for all Grace runtime classes. 
 *  
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#pragma once

#include <cstdint>
#include <string>

#include "../grace.hpp"

namespace Grace 
{
  class GraceObject
  {
    public:
      virtual ~GraceObject() = default;

      virtual void DebugPrint() const = 0;
      virtual void Print() const = 0;
      virtual void PrintLn() const = 0;
      virtual std::string ToString() const = 0;
      virtual bool AsBool() const = 0;

      GRACE_INLINE std::uint32_t IncreaseRef()
      {
        m_RefCount++;
        return m_RefCount;
      }

      GRACE_INLINE std::uint32_t DecreaseRef()
      {
        m_RefCount--;
        return m_RefCount;
      }

      GRACE_NODISCARD GRACE_INLINE std::uint32_t RefCount() const { return m_RefCount; }

    private:
      std::uint32_t m_RefCount = 0;
  };
}
