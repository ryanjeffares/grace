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

#include <unordered_map>

#include "grace_exception.hpp"
#include "grace_iterator.hpp"
#include "../value.hpp"

namespace Grace
{
  class GraceDictionary : public GraceIterable<std::unordered_map<VM::Value, VM::Value>::iterator>
  {
    public:
      using Dictionary = std::unordered_map<VM::Value, VM::Value>;
      using Iterator = Dictionary::iterator;

      GraceDictionary() = default;
      explicit GraceDictionary(Dictionary&& dict);

      ~GraceDictionary() override = default;

      void DebugPrint() const override;
      void Print() const override;
      void PrintLn() const override;
      GRACE_NODISCARD std::string ToString() const override;
      GRACE_NODISCARD bool AsBool() const override;

      GRACE_NODISCARD GRACE_INLINE constexpr const char* ObjectName() const override
      {
        return "Dictionary";
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

      GRACE_NODISCARD Iterator Begin() override
      {
        return m_Dict.begin();
      }

      GRACE_NODISCARD Iterator End() override
      {
        return m_Dict.end();
      }

    private:
      Dictionary m_Dict;
  };
}

#endif // GRACE_DICTIONARY_HPP
