//
// Created by Ryan Jeffares on 03/12/2022.
//

#ifndef GRACE_SET_HPP
#define GRACE_SET_HPP

#include "grace_iterator.hpp"

namespace Grace
{
  class GraceSet : public GraceIterable
  {
    public:
      GraceSet();
      GraceSet(std::vector<VM::Value>&& data);
      GraceSet(VM::Value&& value);

      ~GraceSet() override = default;

      void Add(VM::Value&& value);

      bool Contains(const VM::Value& value) const;

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
      void IncrementIterator(IteratorType& toIncrement) const override;

      GRACE_NODISCARD std::vector<GraceObject*> GetObjectMembers() const override;
      GRACE_NODISCARD bool AnyMemberMatches(const GraceObject* match) const override;
      void RemoveMember(GraceObject* object) override;

    private:

      GRACE_NODISCARD std::vector<VM::Value> ToVector() const;
      void Rehash();

      enum class CellState
      {
        NeverUsed, Tombstone, Occupied,
      };

      std::size_t m_Size, m_Capacity;

      std::vector<CellState> m_CellStates;

      std::hash<VM::Value> m_Hasher;
  };
}

#endif // GRACE_SET_HPP
