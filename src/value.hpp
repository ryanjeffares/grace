/*
 *  The Grace Programming Language.
 *
 *  This file contains the Value class, which represents all values in Grace. 
 *
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#pragma once

#include <optional>
#include <string>

#include "../include/fmt/core.h"
#include "../include/fmt/format.h"

#include "grace.hpp"

namespace Grace
{
  namespace VM
  {
    class Value final 
    {
    public:

      using NullValue = void*;

      enum class Type : std::uint8_t 
      {
        Bool,
        Char,
        Double,
        Int,
        Null,
        String,
      };

      template<typename T>
      explicit constexpr Value(const T& value)
      {
        static_assert(std::is_same<T, std::int64_t>::value || std::is_same<T, double>::value
            || std::is_same<T, bool>::value || std::is_same<T, char>::value
            || std::is_same<T, std::string>::value || std::is_same<T, NullValue>::value,
            "Invalid type for Constant<T>");
        if constexpr (std::is_same<T, std::int64_t>::value) {
          m_Type = Type::Int;
          m_Data.m_Int = value;
        } else if constexpr (std::is_same<T, double>::value) {
          m_Type = Type::Double;
          m_Data.m_Double = value;
        } else if constexpr (std::is_same<T, bool>::value) {
          m_Type = Type::Bool;
          m_Data.m_Bool = value;
        } else if constexpr (std::is_same<T, char>::value) {
          m_Type = Type::Char;
          m_Data.m_Char = value;
        } else if constexpr (std::is_same<T, std::string>::value) {
          m_Type = Type::String;
          m_Data.m_Str = new std::string(value);
        } else if constexpr (std::is_same<T, NullValue>::value) {
          m_Type = Type::Null;
          m_Data.m_Null = nullptr;
        }
      }

      Value()
      {
        m_Type = Type::Null;
        m_Data.m_Null = nullptr;
      }

      Value(const Value& other)
      {
        m_Type = other.m_Type;
        if (other.m_Type == Type::String) {
          m_Data.m_Str = new std::string(*other.m_Data.m_Str);
        } else {
          m_Data = other.m_Data;
        }
      }

      Value(Value&& other)
      {
        m_Type = other.m_Type;
        m_Data = other.m_Data;
        if (other.m_Type == Type::String) {
          other.m_Data.m_Str = nullptr;
        }
      }

      ~Value()
      {
        if (m_Type == Type::String && m_Data.m_Str != nullptr) {
          delete m_Data.m_Str;
        }
      }

      constexpr Value& operator=(const Value& other)
      {
        if (this != &other)
        {
          if (m_Type == Type::String && m_Data.m_Str != nullptr) {
            delete m_Data.m_Str;
          }
          m_Type = other.m_Type;
          if (other.m_Type == Type::String) {
            m_Data.m_Str = new std::string(*other.m_Data.m_Str);
          } else {
            m_Data = other.m_Data;
          }
        }
        return *this;
      }

      template<typename T>
      constexpr Value& operator=(const T& value)
      {
        static_assert(std::is_same<T, std::int64_t>::value || std::is_same<T, double>::value
            || std::is_same<T, bool>::value || std::is_same<T, char>::value
            || std::is_same<T, std::string>::value || std::is_same<T, NullValue>::value,
            "Invalid type for Constant<T>::operator=");

        if (m_Type == Type::String && m_Data.m_Str != nullptr) {
          delete m_Data.m_Str;
        }

        if constexpr (std::is_same<T, std::int64_t>::value) {
          m_Type = Type::Int;
          m_Data.m_Int = value;
        } else if constexpr (std::is_same<T, double>::value) {
          m_Type = Type::Double;
          m_Data.m_Double = value;
        } else if constexpr (std::is_same<T, bool>::value) {
          m_Type = Type::Bool;
          m_Data.m_Bool = value;
        } else if constexpr (std::is_same<T, char>::value) {
          m_Type = Type::Char;
          m_Data.m_Char = value;
        } else if constexpr (std::is_same<T, std::string>::value) {
          m_Type = Type::String;
          m_Data.m_Str = new std::string(value);
        } else if constexpr (std::is_same<T, NullValue>::value) {
          m_Type = Type::Null;
          m_Data.m_Null = nullptr;
        }
        return *this;
      }

      void PrintLn() const;
      void Print() const;
      void DebugPrint() const;

      GRACE_NODISCARD std::string AsString() const;
      GRACE_NODISCARD bool AsBool() const;
      GRACE_NODISCARD std::tuple<bool, std::optional<std::string>> AsInt(std::int64_t& result) const;
      GRACE_NODISCARD std::tuple<bool, std::optional<std::string>> AsDouble(double& result) const;
      GRACE_NODISCARD std::tuple<bool, std::optional<std::string>> AsChar(char& result) const;

      template<typename T> 
      constexpr GRACE_INLINE T Get() const
      {
        static_assert(std::is_same<T, std::int64_t>::value || std::is_same<T, double>::value
            || std::is_same<T, bool>::value || std::is_same<T, char>::value
            || std::is_same<T, std::string>::value || std::is_same<T, NullValue>::value,
            "Invalid type for Constant<T>::Get<T>()");
        if constexpr (std::is_same<T, std::int64_t>::value) {
          return m_Data.m_Int;
        } else if constexpr (std::is_same<T, double>::value) {
          return m_Data.m_Double;
        } else if constexpr (std::is_same<T, bool>::value) {
          return m_Data.m_Bool;
        } else if constexpr (std::is_same<T, char>::value) {
          return m_Data.m_Char;
        } else if constexpr (std::is_same<T, std::string>::value) {
          return *m_Data.m_Str;
        } else if constexpr (std::is_same<T, NullValue>::value) {
          return nullptr;
        }
      }

      GRACE_NODISCARD
      constexpr GRACE_INLINE Type GetType() const
      {
        return m_Type;
      }

    private:

      Type m_Type;

      union
      {
        bool m_Bool;
        char m_Char;
        double m_Double;
        std::int64_t m_Int;
        std::string* m_Str;
        NullValue m_Null;
      } m_Data;

    };
  }
}

template<>
struct fmt::formatter<Grace::VM::Value::Type> : fmt::formatter<std::string_view>
{
  template<typename FormatContext>
  auto format(Grace::VM::Value::Type type, FormatContext& context) -> decltype(context.out())
  {
    using namespace Grace::VM;

    std::string_view name = "unknown";
    switch (type) {
      case Value::Type::Bool: name = "Type::Bool"; break;
      case Value::Type::Char: name = "Type::Char"; break;
      case Value::Type::Double: name = "Type::Float"; break;
      case Value::Type::Int: name = "Type::Int"; break;
      case Value::Type::Null: name = "Type::Null"; break;
      case Value::Type::String: name = "Type::String"; break;
    }
    return fmt::formatter<std::string_view>::format(name, context);
  }
};
