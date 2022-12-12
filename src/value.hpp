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

#ifndef GRACE_VALUE_HPP
#define GRACE_VALUE_HPP

#include <functional>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include <fmt/core.h>
#include <fmt/format.h>

#include "grace.hpp"
#include "objects/grace_exception.hpp"
#include "objects/grace_object.hpp"
#include "objects/object_tracker.hpp"

namespace Grace
{
  class GraceList;

  namespace VM
  {
    template<class T>
    concept DerivedGraceObject = std::is_base_of_v<GraceObject, T>;

    template<typename T>
    concept BuiltinGraceType = std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_same_v<T, bool> || std::is_same_v<T, char> || std::is_same_v<T, std::string> || std::is_same_v<T, std::nullptr_t>;

    class Value final
    {
      template<typename T>
      static constexpr bool IsInteger = std::is_same_v<T, short> || std::is_same_v<T, unsigned short> || std::is_same_v<T, int> || std::is_same_v<T, unsigned int> || std::is_same_v<T, long> || std::is_same_v<T, unsigned long> || std::is_same_v<T, long long> || std::is_same_v<T, unsigned long long>;

    public:
      using NullValue = std::nullptr_t;

      enum class Type : std::uint8_t
      {
        Bool,
        Char,
        Double,
        Int,
        Null,
        String,
        Object,
      };

      template<BuiltinGraceType T>
      GRACE_INLINE constexpr Value(T value)
      {
        if constexpr (IsInteger<T>) {
          m_Type = Type::Int;
          m_Data.m_Int = static_cast<std::int64_t>(value);
        } else if constexpr (std::is_floating_point_v<T>) {
          m_Type = Type::Double;
          m_Data.m_Double = static_cast<double>(value);
        } else if constexpr (std::is_same_v<T, bool>) {
          m_Type = Type::Bool;
          m_Data.m_Bool = value;          
        } else if constexpr (std::is_same_v<T, char>) {
          m_Type = Type::Char;
          m_Data.m_Char = value;
        } else if constexpr (std::is_same_v<T, std::string>) {
          m_Type = Type::String;
          m_Data.m_Str = new std::string { std::move(value) };
        } else if constexpr (std::is_same_v<T, NullValue>) {
          m_Type = Type::Null;
          m_Data.m_Null = nullptr;
        }
      }

      Value();
      Value(const Value& other);
      Value(Value&& other) noexcept;

      // ONLY call with an object that already exists and has refs elsewhere, this is really only for use in the cycle cleaner
      explicit Value(GraceObject* object);

      ~Value();

      // Template function that allocates any GraceObject with any of its constructors
      // Use this and only this function to allocate GraceObjects
      template<DerivedGraceObject T, typename... Args>
      GRACE_NODISCARD static Value CreateObject(Args&&... args)
      {
        Value res;
        res.m_Type = Type::Object;
        res.m_Data.m_Object = new T(std::forward<Args>(args)...);
        res.m_Data.m_Object->IncreaseRef();
        ObjectTracker::TrackObject(res.m_Data.m_Object);
        return res;
      }

      constexpr Value& operator=(const Value& other)
      {
        if (this != &other) {
          if (m_Type == Type::String) {
            delete m_Data.m_Str;
          }

          if (m_Type == Type::Object) {
            if (m_Data.m_Object->DecreaseRef() == 0) {
              ObjectTracker::StopTrackingObject(m_Data.m_Object);
              delete m_Data.m_Object;
            }
          }

          m_Type = other.m_Type;
          if (other.m_Type == Type::String) {
            m_Data.m_Str = new std::string(*other.m_Data.m_Str);
          } else if (other.m_Type == Type::Object) {
            m_Data.m_Object = other.m_Data.m_Object;
            m_Data.m_Object->IncreaseRef();
          } else {
            m_Data = other.m_Data;
          }
        }
        return *this;
      }

      constexpr Value& operator=(Value&& other) noexcept
      {
        if (this != &other) {
          if (m_Type == Type::String) {
            delete m_Data.m_Str;
          }

          if (m_Type == Type::Object) {
            if (m_Data.m_Object->DecreaseRef() == 0) {
              ObjectTracker::StopTrackingObject(m_Data.m_Object);
              delete m_Data.m_Object;
            }
          }

          m_Type = other.m_Type;
          m_Data = other.m_Data;

          if (other.m_Type == Type::String || other.m_Type == Type::Object) {
            other.m_Data.m_Null = nullptr;
            other.m_Type = Type::Null;
          }
        }
        return *this;
      }

      template<BuiltinGraceType T>
      constexpr Value& operator=(T value)
      {
        if (m_Type == Type::String) {
          delete m_Data.m_Str;
        }

        if (m_Type == Type::Object) {
          if (m_Data.m_Object->DecreaseRef() == 0) {
            ObjectTracker::StopTrackingObject(m_Data.m_Object);
            delete m_Data.m_Object;
          }
        }

        if constexpr (IsInteger<T>) {
          m_Type = Type::Int;
          m_Data.m_Int = static_cast<std::int64_t>(value);
        } else if constexpr (std::is_floating_point_v<T>) {
          m_Type = Type::Double;
          m_Data.m_Double = static_cast<double>(value);
        } else if constexpr (std::is_same_v<T, bool>) {
          m_Type = Type::Bool;
          m_Data.m_Bool = value;
        } else if constexpr (std::is_same_v<T, char>) {
          m_Type = Type::Char;
          m_Data.m_Char = value;
        } else if constexpr (std::is_same_v<T, std::string>) {
          m_Type = Type::String;
          m_Data.m_Str = new std::string { std::move(value) };
        } else if constexpr (std::is_same_v<T, NullValue>) {
          m_Type = Type::Null;
          m_Data.m_Null = nullptr;
        }

        return *this;
      }

      GRACE_NODISCARD Value operator+(const Value&) const;
      GRACE_NODISCARD Value operator-(const Value&) const;
      GRACE_NODISCARD Value operator*(const Value&) const;
      GRACE_NODISCARD Value operator/(const Value&) const;
      GRACE_NODISCARD Value operator%(const Value&) const;
      GRACE_NODISCARD Value operator<<(const Value&) const;
      GRACE_NODISCARD Value operator>>(const Value&) const;
      GRACE_NODISCARD Value operator|(const Value&) const;
      GRACE_NODISCARD Value operator^(const Value&) const;
      GRACE_NODISCARD Value operator&(const Value&) const;
      GRACE_NODISCARD bool operator==(const Value&) const;
      GRACE_NODISCARD bool operator!=(const Value&) const;
      GRACE_NODISCARD bool operator<(const Value&) const;
      GRACE_NODISCARD bool operator<=(const Value&) const;
      GRACE_NODISCARD bool operator>(const Value&) const;
      GRACE_NODISCARD bool operator>=(const Value&) const;
      GRACE_NODISCARD Value operator!() const;
      GRACE_NODISCARD Value operator-() const;
      GRACE_NODISCARD Value operator~() const;

      Value& operator+=(const Value&);
      Value& operator-=(const Value&);
      Value& operator*=(const Value&);
      Value& operator/=(const Value&);
      Value& operator&=(const Value&);
      Value& operator|=(const Value&);
      Value& operator^=(const Value&);
      Value& operator%=(const Value&);
      Value& operator<<=(const Value&);
      Value& operator>>=(const Value&);

      GRACE_NODISCARD Value Pow(const Value&) const;

      void PrintLn(bool err) const;
      void Print(bool err) const;
      void DebugPrint() const;

      GRACE_NODISCARD std::string AsString() const;
      GRACE_NODISCARD bool AsBool() const;
      GRACE_NODISCARD std::tuple<bool, std::optional<std::string>> AsInt(std::int64_t& result) const;
      GRACE_NODISCARD std::tuple<bool, std::optional<std::string>> AsDouble(double& result) const;
      GRACE_NODISCARD std::tuple<bool, std::optional<std::string>> AsChar(char& result) const;

      template<BuiltinGraceType T>
      GRACE_NODISCARD constexpr GRACE_INLINE T Get() const
      {
        static_assert(!std::is_same_v<T, std::string> && "Use GetString() instead");

        if constexpr (IsInteger<T>) {
          return static_cast<T>(m_Data.m_Int);
        } else if constexpr (std::is_floating_point_v<T>) {
          return static_cast<T>(m_Data.m_Double);
        } else if constexpr (std::is_same_v<T, bool>) {
          return m_Data.m_Bool;
        } else if constexpr (std::is_same_v<T, char>) {
          return m_Data.m_Char;
        } else if constexpr (std::is_same_v<T, NullValue>) {
          return nullptr;
        }
      }

      GRACE_NODISCARD GRACE_INLINE const std::string& GetString() const
      {
        return *m_Data.m_Str;
      }

      GRACE_NODISCARD constexpr GRACE_INLINE GraceObject* GetObject() const
      {
        if (m_Type == Type::Object) {
          return m_Data.m_Object;
        }
        return nullptr;
      }

      GRACE_NODISCARD constexpr GRACE_INLINE Type GetType() const
      {
        return m_Type;
      }

      GRACE_NODISCARD std::string GetTypeName() const;

    private:
      Type m_Type { Type::Null };

      union
      {
        bool m_Bool;
        char m_Char;
        double m_Double;
        std::int64_t m_Int;
        NullValue m_Null;
        GraceObject* m_Object;
        std::string* m_Str;
      } m_Data {};
    };
  } // namespace VM
} // namespace Grace

template<>
struct fmt::formatter<Grace::VM::Value::Type> : fmt::formatter<std::string_view>
{
  template<typename FormatContext>
  constexpr auto format(Grace::VM::Value::Type type, FormatContext& context) -> decltype(context.out())
  {
    using namespace Grace::VM;

    std::string_view name = "unknown";
    switch (type) {
      case Value::Type::Bool: name = "Bool"; break;
      case Value::Type::Char: name = "Char"; break;
      case Value::Type::Double: name = "Float"; break;
      case Value::Type::Int: name = "Int"; break;
      case Value::Type::Null: name = "Null"; break;
      case Value::Type::Object: name = "Object"; break;
      case Value::Type::String: name = "String"; break;
    }
    return fmt::formatter<std::string_view>::format(name, context);
  }
};

template<>
struct fmt::formatter<Grace::VM::Value> : fmt::formatter<std::string_view>
{
  template<typename FormatContext>
  auto format(const Grace::VM::Value& value, FormatContext& context) const -> decltype(context.out())
  {
    std::string res = value.AsString();
    return fmt::formatter<std::string_view>::format(res, context);
  }
};

namespace std
{
  template<>
  struct hash<Grace::VM::Value>
  {
    std::size_t operator()(const Grace::VM::Value&) const;
  };
} // namespace std

#endif // ifndef GRACE_VALUE_HPP
