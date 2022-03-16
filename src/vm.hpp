#pragma once

#include <cstdint>
#include <exception>
#include <vector>
#include <typeinfo>
#include <type_traits>

#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/format.h>

namespace Grace
{
  namespace VM
  {
    enum class Ops : std::uint8_t
    {
      Add,
      And,
      Divide,
      Equal,
      Greater,
      GreaterEqual,
      Less,
      LessEqual,
      LoadConstant,
      Multiply,
      NotEqual,
      Or,
      Pop,
      Print,
      Subtract,
    };

    enum class InterpretResult
    {
      RuntimeError,
      RuntimeOk,
    };

    enum class InterpretError
    {
      InvalidOperand,
    };

    class VM
    {
      public:

        VM() = default;
        ~VM() = default;

        VM(const VM&) = delete;
        VM(VM&&) = delete;

        inline void PushOp(Ops op, int line)
        {
          m_OpList.emplace_back(op, line);
        }

        void PrintOps() const
        {
          for (const auto o : m_OpList)
          {
            fmt::print("{}\n", o.m_Op);
          }
        }

        template<typename T>
        constexpr inline void PushConstant(T value)
        {
          m_ConstantList.emplace_back(value);
        }

        InterpretResult Run();

        struct Constant 
        {
          enum class Type 
          {
            Bool,
            Char,
            Float,
            Int,
          };

          union 
          {
            bool m_Bool;
            char m_Char;
            float m_Float;
            int m_Int;
          } m_Data;


          template<typename T>
          explicit constexpr Constant(T value)
          {
            static_assert(std::is_same<T, int>::value || std::is_same<T, float>::value
                || std::is_same<T, bool>::value || std::is_same<T, char>::value,
                "Invalid type for Constant<T>");
            if constexpr (std::is_same<T, int>::value) {
              m_Type = Type::Int;
              m_Data.m_Int = value;
            } else if constexpr (std::is_same<T, float>::value) {
              m_Type = Type::Float;
              m_Data.m_Float = value;
            } else if constexpr (std::is_same<T, bool>::value) {
              m_Type = Type::Bool;
              m_Data.m_Bool = value;
            } else if constexpr (std::is_same<T, char>::value) {
              m_Type = Type::Char;
              m_Data.m_Char = value;
            }
          }
          
          template<typename T>
          constexpr Constant& operator=(const T& value)
          {
            static_assert(std::is_same<T, int>::value || std::is_same<T, float>::value
                || std::is_same<T, bool>::value || std::is_same<T, char>::value,
                "Invalid type for Constant<T>::operator=");
            if constexpr (std::is_same<T, int>::value) {
              m_Type = Type::Int;
              m_Data.m_Int = value;
            } else if constexpr (std::is_same<T, float>::value) {
              m_Type = Type::Float;
              m_Data.m_Float = value;
            } else if constexpr (std::is_same<T, bool>::value) {
              m_Type = Type::Bool;
              m_Data.m_Bool = value;
            } else if constexpr (std::is_same<T, char>::value) {
              m_Type = Type::Char;
              m_Data.m_Char = value;
            }
            return *this;
          }

          void Print() const
          {
            switch (m_Type) {
              case Type::Bool:
                fmt::print("{}\n", m_Data.m_Bool);
                break;
              case Type::Char:
                fmt::print("{}\n", m_Data.m_Char);
                break;
              case Type::Float:
                fmt::print("{}\n", m_Data.m_Float);
                break;
              case Type::Int:
                fmt::print("{}\n", m_Data.m_Int);
                break;
            }
          }

          std::string ToString() const
          {
            switch (m_Type)
            {
              case Type::Bool:
                return fmt::format("{}", m_Data.m_Bool);
              case Type::Char:
                return fmt::format("{}", m_Data.m_Char);
              case Type::Float:
                return fmt::format("{}", m_Data.m_Float);
              case Type::Int:
                return fmt::format("{}", m_Data.m_Int);
              default:
                return "Invalid type";
            }
          }

          template<typename T> 
          constexpr inline T Get() const
          {
            static_assert(std::is_same<T, int>::value || std::is_same<T, float>::value
                || std::is_same<T, bool>::value || std::is_same<T, char>::value,
                "Invalid type for Constant<T>::Get<T>()");
            if constexpr (std::is_same<T, int>::value) {
              return m_Data.m_Int;
            } else if constexpr (std::is_same<T, float>::value) {
              return m_Data.m_Float;
            } else if constexpr (std::is_same<T, bool>::value) {
              return m_Data.m_Bool;
            } else if constexpr (std::is_same<T, char>::value) {
              return m_Data.m_Char;
            }
          }

          constexpr inline Type GetType() const
          {
            return m_Type;
          }

          private:

            Type m_Type;
        };

      private:

        struct OpLine
        {
          Ops m_Op;
          int m_Line;

          OpLine(Ops op, int line) 
            : m_Op(op), m_Line(line)
          {

          }
        };
        
        void RuntimeError(const std::string& message, InterpretError errorType, int line);

        std::vector<OpLine> m_OpList;
        std::vector<Constant> m_ConstantList;

        std::size_t m_OpCurrent = 0, m_ConstantCurrent = 0;
    };
  }
}

template<>
struct fmt::formatter<Grace::VM::Ops> : fmt::formatter<std::string_view>
{
  template<typename FormatContext>
  auto format(Grace::VM::Ops type, FormatContext& context) -> decltype(context.out())
  {
    using namespace Grace::VM;

    std::string_view name = "unknown";
    switch (type) {
      case Ops::Add: name = "Ops::Add"; break;
      case Ops::And: name = "Ops::And"; break;
      case Ops::Divide: name = "Ops::Divide"; break;
      case Ops::Equal: name = "Ops::Equal"; break;
      case Ops::Greater: name = "Ops::Greater"; break;
      case Ops::GreaterEqual: name = "Ops::GreaterEqual"; break;
      case Ops::Less: name = "Ops::Less"; break;
      case Ops::LessEqual: name = "Ops::LessEqual"; break;
      case Ops::LoadConstant: name = "Ops::LoadConstant"; break;
      case Ops::Multiply: name = "Ops::Multiply"; break;
      case Ops::NotEqual: name = "Ops::NotEqual"; break;
      case Ops::Or: name = "Ops::Or"; break;
      case Ops::Pop: name = "Ops::Pop"; break;
      case Ops::Print: name = "Ops::Print"; break;
      case Ops::Subtract: name = "Ops::Subtract"; break;
    }
    return fmt::formatter<std::string_view>::format(name, context);
  }
};

template<>
struct fmt::formatter<Grace::VM::InterpretError> : fmt::formatter<std::string_view>
{
  template<typename FormatContext>
  auto format(Grace::VM::InterpretError type, FormatContext& context) -> decltype(context.out())
  {
    using namespace Grace::VM;

    std::string_view name = "unknown";
    switch (type) {
      case InterpretError::InvalidOperand: name = "InvalidOperand"; break;
    }
    return fmt::formatter<std::string_view>::format(name, context);
  }
};

template<>
struct fmt::formatter<Grace::VM::VM::Constant::Type> : fmt::formatter<std::string_view>
{
  template<typename FormatContext>
  auto format(Grace::VM::VM::Constant::Type type, FormatContext& context) -> decltype(context.out())
  {
    using namespace Grace::VM;

    std::string_view name = "unknown";
    switch (type) {
      case VM::Constant::Type::Bool: name = "Type::Bool"; break;
      case VM::Constant::Type::Char: name = "Type::Char"; break;
      case VM::Constant::Type::Float: name = "Type::Float"; break;
      case VM::Constant::Type::Int: name = "Type::Int"; break;
    }
    return fmt::formatter<std::string_view>::format(name, context);
  }
};
