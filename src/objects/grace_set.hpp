/*
 *  The Grace Programming Language.
 *
 *  This file contains the GraceSet class, the underlying class for Sets in Grace.
 *
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#ifndef GRACE_SET_HPP
#define GRACE_SET_HPP

#include "grace_hashable.hpp"

namespace Grace
{
  class GraceSet : public GraceHashable
  {
    public:
      GraceSet() = default;
      GraceSet(std::vector<VM::Value>&& data);
      GraceSet(VM::Value&& value);

      ~GraceSet() override = default;

      void Add(VM::Value&& value);

      GRACE_NODISCARD bool Contains(const VM::Value& value) const;
      GRACE_NODISCARD GRACE_INLINE std::size_t Size() const
      {
        return m_Size;
      }

      void DebugPrint() const override;
      void Print(bool err) const override;
      void PrintLn(bool err) const override;
      GRACE_NODISCARD std::string ToString() const override;
      GRACE_NODISCARD bool AsBool() const override;

      GRACE_NODISCARD GRACE_INLINE constexpr std::string_view ObjectName() const override
      {
        return "Set";
      }

      GRACE_NODISCARD GRACE_INLINE constexpr bool IsIterable() const override
      {
        return true;
      }

      GRACE_NODISCARD GRACE_INLINE constexpr GraceObjectType ObjectType() const override
      {
        return GraceObjectType::Set;
      }

      GRACE_NODISCARD GRACE_INLINE GraceSet* GetAsSet() override
      {
        return this;
      }

      GRACE_NODISCARD IteratorType Begin() override;
      GRACE_NODISCARD IteratorType End() override;
      void IncrementIterator(IteratorType& toIncrement) override;

      GRACE_NODISCARD std::vector<GraceObject*> GetObjectMembers() const override;
      GRACE_NODISCARD bool AnyMemberMatches(const GraceObject* match) const override;
      GRACE_NODISCARD bool OnlyReferenceIsSelf() const override;
      void RemoveMember(GraceObject* object) override;

    protected:
      
      void GrowAndRehash() override;
  };
} // namespace Grace

#endif // GRACE_SET_HPP
