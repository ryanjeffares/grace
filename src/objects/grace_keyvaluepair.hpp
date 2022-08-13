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
      ~GraceKeyValuePair() override = default;

      void DebugPrint() const override;
      void Print(bool err) const override;
      void PrintLn(bool err) const override;
      GRACE_NODISCARD std::string ToString() const override;
      GRACE_NODISCARD bool AsBool() const override;
      
      GRACE_NODISCARD GRACE_INLINE constexpr const char* ObjectName() const override
      {
        return "KeyValuePair";
      }
      
      GRACE_NODISCARD GRACE_INLINE constexpr bool IsIterable() const override
      {
        return false;
      }

      GRACE_NODISCARD GRACE_INLINE constexpr GraceObjectType ObjectType() const override
      {
        return GraceObjectType::KeyValuePair;
      }

      GRACE_NODISCARD GRACE_INLINE GraceKeyValuePair* GetAsKeyValuePair() override
      {
        return this;
      }

      GRACE_NODISCARD GRACE_INLINE VM::Value& Key()
      {
        return m_Key;
      }

      GRACE_NODISCARD GRACE_INLINE VM::Value& Value()
      {
        return m_Value;
      }

      GRACE_NODISCARD bool AnyMemberMatches(const GraceObject* match) override;
      GRACE_NODISCARD std::vector<GraceObject*> GetObjectMembers() override;
      void RemoveMember(GraceObject* object) override;

    private:
      VM::Value m_Key, m_Value;
  };
} // namespace Grace


#endif  // ifndef GRACE_KEYVALUEPAIR_HPP