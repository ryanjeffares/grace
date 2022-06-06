/*
 *  The Grace Programming Language.
 *
 *  This file contains the GraceKeyValuePair class, the object held by a GraceDictionary
 *
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#ifndef GRACE_KEYVALUEPAIR_HPP
#define GRACE_KEYVALUEPAIR_HPP

#include <utility>

#include "../grace.hpp"
#include "grace_object.hpp"
#include "grace_exception.hpp"
#include "../value.hpp"

namespace Grace
{
  class GraceKeyValuePair : public GraceObject
  {
    public:

      GraceKeyValuePair(VM::Value&& key, VM::Value&& value);
      GraceKeyValuePair(std::pair<VM::Value, VM::Value>&& pair);

      void DebugPrint() const override;
      void Print() const override;
      void PrintLn() const override;
      GRACE_NODISCARD std::string ToString() const override;
      GRACE_NODISCARD bool AsBool() const override;
      
      GRACE_NODISCARD constexpr const char* ObjectName() const override
      {
        return "KeyValuePair";
      }
      
      GRACE_NODISCARD constexpr bool IsIterable() const override
      {
        return false;
      }

      GRACE_NORETURN const VM::Value& Deref() const override
      {
        throw GraceException(
          GraceException::Type::InvalidType,
          "KeyValuePair is not dereferenceable"
        );
      }

      GRACE_NODISCARD GRACE_INLINE const VM::Value& Key() const
      {
        return m_Key;
      }

      GRACE_NODISCARD GRACE_INLINE const VM::Value& Value() const
      {
        return m_Value;
      }

    private:
      VM::Value m_Key, m_Value;
  };
} // namespace Grace


#endif  // ifndef GRACE_KEYVALUEPAIR_HPP