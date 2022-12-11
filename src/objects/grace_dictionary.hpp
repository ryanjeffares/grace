/*
 *  The Grace Programming Language.
 *
 *  This file contains the GraceDictionary class, the underlying class for Dictionaries in Grace.
 *
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#ifndef GRACE_DICTIONARY_HPP
#define GRACE_DICTIONARY_HPP

#include <functional>
#include <mutex>
#include <vector>

#include "grace_hashable.hpp"
#include "grace_keyvaluepair.hpp"
#include "../value.hpp"

namespace Grace
{
  class GraceDictionary : public GraceHashable
  {
    public:

      GraceDictionary() = default;
      ~GraceDictionary() override = default;

      void DebugPrint() const override;
      void Print(bool err) const override;
      void PrintLn(bool err) const override;
      GRACE_NODISCARD std::string ToString() const override;
      GRACE_NODISCARD bool AsBool() const override;      

      GRACE_NODISCARD GRACE_INLINE constexpr std::string_view ObjectName() const override
      {
        return "Dict";
      }

      GRACE_NODISCARD GRACE_INLINE constexpr bool IsIterable() const override
      {
        return true;
      }

      GRACE_NODISCARD GRACE_INLINE constexpr GraceObjectType ObjectType() const override
      {
        return GraceObjectType::Dictionary;
      }

      GRACE_NODISCARD GRACE_INLINE GraceDictionary* GetAsDictionary() override
      {
        return this;
      }

      GRACE_NODISCARD IteratorType Begin() override;
      GRACE_NODISCARD IteratorType End() override;
      void IncrementIterator(IteratorType& toIncrement) override;

      GRACE_NODISCARD GRACE_INLINE std::size_t Size() const
      {
        return m_Size;
      }

      GRACE_NODISCARD GRACE_INLINE std::size_t Capacity() const
      {
        return m_Capacity;
      }

      void Insert(VM::Value&& key, VM::Value&& value);
      void Update(const VM::Value& key, VM::Value&& value);

      GRACE_NODISCARD const VM::Value& Get(const VM::Value& key);
      GRACE_NODISCARD bool ContainsKey(const VM::Value& key);
      GRACE_NODISCARD bool Remove(const VM::Value& key);      

      GRACE_NODISCARD std::vector<GraceObject*> GetObjectMembers() const override;
      GRACE_NODISCARD bool AnyMemberMatches(const GraceObject* match) const override;
      void RemoveMember(GraceObject* object) override;

    protected:

      void GrowAndRehash() override;
  };
}

#endif // GRACE_DICTIONARY_HPP
