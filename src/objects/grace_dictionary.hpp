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
#include <vector>

#include "grace_exception.hpp"
#include "grace_iterator.hpp"
#include "grace_keyvaluepair.hpp"
#include "../value.hpp"

namespace Grace
{
  class GraceDictionary : public GraceIterable<std::vector<VM::Value>::iterator>
  {
    public:
      using Iterator = std::vector<VM::Value>::iterator;

      GraceDictionary();
      GraceDictionary(const GraceDictionary&);
      GraceDictionary(GraceDictionary&&);

      ~GraceDictionary() override = default;

      void DebugPrint() const override;
      void Print() const override;
      void PrintLn() const override;
      GRACE_NODISCARD std::string ToString() const override;
      GRACE_NODISCARD bool AsBool() const override;

      GRACE_NODISCARD GRACE_INLINE constexpr const char* ObjectName() const override
      {
        return "Dict";
      }

      GRACE_NODISCARD GRACE_INLINE constexpr bool IsIterable() const override
      {
        return true;
      }

      GRACE_NORETURN const VM::Value& Deref() const override
      {
        throw GraceException(
          GraceException::Type::InvalidType,
          "Dictionary cannot be dereferenced"
        );
      }

      GRACE_NODISCARD Iterator Begin() override;
      GRACE_NODISCARD Iterator End() override;
      void IncrementIterator(Iterator& toIncrement) const override;

      GRACE_NODISCARD GRACE_INLINE std::size_t Size() const
      {
        return m_Size;
      }

      bool Insert(VM::Value&& key, VM::Value&& value);

      std::vector<VM::Value> ToVector() const;

    private:
      enum class CellState
      {
        NeverUsed, Tombstone, Occupied
      };

      std::vector<VM::Value> m_Data;
      std::vector<CellState> m_CellStates;
      std::size_t m_Size;
      std::hash<VM::Value> m_Hasher{};
  };
}

#endif // GRACE_DICTIONARY_HPP
