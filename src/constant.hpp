#pragma once

#include <string>

#include "../include/fmt/core.h"
#include "../include/fmt/format.h"

namespace Grace
{
  namespace VM
  {
    struct Constant 
    {
      enum class Type : std::uint8_t 
      {
        Bool,
        Char,
        Double,
        Int,
        String,
      };

      union
      {
        bool m_Bool;
        char m_Char;
        double m_Double;
        std::int64_t m_Int;
        std::string* m_Str;
      } m_Data;

      template<typename T>
      constexpr Constant(const T& value)
      {
        static_assert(std::is_same<T, std::int64_t>::value || std::is_same<T, double>::value
            || std::is_same<T, bool>::value || std::is_same<T, char>::value
            || std::is_same<T, std::string>::value,
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
        }
      }

      Constant(const Constant& other)
      {
        m_Type = other.m_Type;
        if (other.m_Type == Type::String) {
          m_Data.m_Str = new std::string(*other.m_Data.m_Str);
        } else {
          m_Data = other.m_Data;
        }
      }

      ~Constant()
      {
        if (m_Type == Type::String && m_Data.m_Str != nullptr) {
          delete m_Data.m_Str;
        }
      }

      constexpr Constant& operator=(const Constant& other)
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
      constexpr Constant& operator=(const T& value)
      {
        static_assert(std::is_same<T, std::int64_t>::value || std::is_same<T, double>::value
            || std::is_same<T, bool>::value || std::is_same<T, char>::value
            || std::is_same<T, std::string>::value,
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
        }
        return *this;
      }

      void PrintLn() const;
      void Print() const;
      std::string ToString() const;

      template<typename T> 
      constexpr inline T Get() const
      {
        static_assert(std::is_same<T, std::int64_t>::value || std::is_same<T, double>::value
            || std::is_same<T, bool>::value || std::is_same<T, char>::value
            || std::is_same<T, std::string>::value,
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
        }
      }

      constexpr inline Type GetType() const
      {
        return m_Type;
      }

      private:

        Type m_Type;
    };
  }
}

template<>
struct fmt::formatter<Grace::VM::Constant::Type> : fmt::formatter<std::string_view>
{
  template<typename FormatContext>
  auto format(Grace::VM::Constant::Type type, FormatContext& context) -> decltype(context.out())
  {
    using namespace Grace::VM;

    std::string_view name = "unknown";
    switch (type) {
      case Constant::Type::Bool: name = "Type::Bool"; break;
      case Constant::Type::Char: name = "Type::Char"; break;
      case Constant::Type::Double: name = "Type::Float"; break;
      case Constant::Type::Int: name = "Type::Int"; break;
      case Constant::Type::String: name = "Type::String"; break;
    }
    return fmt::formatter<std::string_view>::format(name, context);
  }
};
