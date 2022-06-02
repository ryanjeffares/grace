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
    concept DerivedGraceObject = std::is_base_of<GraceObject, T>::value;

    template<typename T>
    concept BuiltinGraceType = std::is_same<T, std::int64_t>::value || std::is_same<T, double>::value
            || std::is_same<T, bool>::value || std::is_same<T, char>::value
            || std::is_same<T, std::string>::value || std::is_same<T, std::nullptr_t>::value;

    class Value final 
    {
    public:

      using NullValue = std::nullptr_t; 

      enum class Type : std::uint8_t 
      {
        Bool,
        Char,
        Double,
        Int,
        Null,
        Object,
        String,
      };

      template<BuiltinGraceType T>
      GRACE_INLINE explicit constexpr Value(const T& value)
      {
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

      Value();
      Value(const Value& other);
      Value(Value&& other);
      ~Value();

      template<DerivedGraceObject T, typename... Args>
      GRACE_NODISCARD static Value CreateObject(Args&&... args)
      {
        Value res;
        res.m_Type = Type::Object;
        res.m_Data.m_Object = new T(std::forward<Args>(args)...);
        res.m_Data.m_Object->IncreaseRef();
      #ifdef GRACE_DEBUG 
        ObjectTracker::TrackObject(res.m_Data.m_Object);
      #endif
        return res;
      }

      constexpr Value& operator=(const Value& other)
      {
        if (this != &other) {
          if (m_Type == Type::String && m_Data.m_Str != nullptr) {
            delete m_Data.m_Str;
          }

          if (m_Type == Type::Object) {
            if (m_Data.m_Object->DecreaseRef() == 0) {
#ifdef GRACE_DEBUG
              ObjectTracker::StopTracking(m_Data.m_Object); 
#endif
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

      constexpr Value& operator=(Value&& other)
      {
        if (this != &other) {
          if (m_Type == Type::String && m_Data.m_Str != nullptr) {
            delete m_Data.m_Str;
          }
          
          if (m_Type == Type::Object) {
            if (m_Data.m_Object->DecreaseRef() == 0) {
#ifdef GRACE_DEBUG
              ObjectTracker::StopTracking(m_Data.m_Object); 
#endif
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
      constexpr Value& operator=(const T& value)
      {
        if (m_Type == Type::String && m_Data.m_Str != nullptr) {
          delete m_Data.m_Str;
        }

        if (m_Type == Type::Object) {
          if (m_Data.m_Object->DecreaseRef() == 0) {
#ifdef GRACE_DEBUG
            ObjectTracker::StopTracking(m_Data.m_Object); 
#endif
            delete m_Data.m_Object;
          }
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

      GRACE_NODISCARD Value operator+(const Value&) const;
      GRACE_NODISCARD Value operator-(const Value&) const;
      GRACE_NODISCARD Value operator*(const Value&) const;
      GRACE_NODISCARD Value operator/(const Value&) const;
      GRACE_NODISCARD Value operator%(const Value&) const;
      GRACE_NODISCARD bool operator==(const Value&) const;
      GRACE_NODISCARD bool operator!=(const Value&) const;
      GRACE_NODISCARD bool operator<(const Value&) const;
      GRACE_NODISCARD bool operator<=(const Value&) const;
      GRACE_NODISCARD bool operator>(const Value&) const;
      GRACE_NODISCARD bool operator>=(const Value&) const;
      GRACE_NODISCARD Value operator!() const;
      GRACE_NODISCARD Value operator-() const;

      GRACE_NODISCARD Value Pow(const Value&) const;

      void PrintLn() const;
      void Print() const;
      void DebugPrint() const;

      GRACE_NODISCARD GRACE_INLINE bool IsNumber() const
      {
        return m_Type == Type::Int || m_Type == Type::Double;
      }

      GRACE_NODISCARD std::string AsString() const;
      GRACE_NODISCARD bool AsBool() const;
      GRACE_NODISCARD std::tuple<bool, std::optional<std::string>> AsInt(std::int64_t& result) const;
      GRACE_NODISCARD std::tuple<bool, std::optional<std::string>> AsDouble(double& result) const;
      GRACE_NODISCARD std::tuple<bool, std::optional<std::string>> AsChar(char& result) const;

      template<BuiltinGraceType T> 
      GRACE_NODISCARD constexpr GRACE_INLINE T Get() const
      {
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

      GRACE_NODISCARD GRACE_INLINE std::string GetTypeName() const
      {
        if (m_Type == Type::Object) {
          return m_Data.m_Object->ObjectName();
        }
        return fmt::format("{}", m_Type);
      }

    private:

      Type m_Type{ Type::Null }; 

      union
      {
        bool m_Bool;
        char m_Char;
        double m_Double;
        std::int64_t m_Int;
        NullValue m_Null;
        GraceObject* m_Object;
        std::string* m_Str;
      } m_Data{};
    };
  } // namespace VM
} // namespace Grace

template<>
struct fmt::formatter<Grace::VM::Value::Type> : fmt::formatter<std::string_view>
{
  template<typename FormatContext>
  auto format(Grace::VM::Value::Type type, FormatContext& context) -> decltype(context.out())
  {
    using namespace Grace::VM;

    std::string_view name = "unknown";
    switch (type) {
#ifdef GRACE_DEBUG
      case Value::Type::Bool: name = "Type::Bool"; break;
      case Value::Type::Char: name = "Type::Char"; break;
      case Value::Type::Double: name = "Type::Float"; break;
      case Value::Type::Int: name = "Type::Int"; break;
      case Value::Type::Null: name = "Type::Null"; break;
      case Value::Type::Object: name = "Type::Object"; break;
      case Value::Type::String: name = "Type::String"; break;
#else
      case Value::Type::Bool: name = "Bool"; break;
      case Value::Type::Char: name = "Char"; break;
      case Value::Type::Double: name = "Float"; break;
      case Value::Type::Int: name = "Int"; break;
      case Value::Type::Null: name = "Null"; break;
      case Value::Type::Object: name = "Object"; break;
      case Value::Type::String: name = "String"; break;
#endif
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
}

#endif  // ifndef GRACE_VALUE_HPP
