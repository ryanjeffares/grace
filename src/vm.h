#pragma once

#include <cstdint>
#include <vector>

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

        inline void PushConstant(int value)
        {
          Constant c(Constant::Type::Int);
          c.as.m_Int = value;
          m_ConstantList.push_back(c);
        }

        inline void PushConstant(float value)
        {
          Constant c(Constant::Type::Float);
          c.as.m_Float = value;
          m_ConstantList.push_back(c);
        }

        inline void PushConstant(bool value)
        {
          Constant c(Constant::Type::Bool);
          c.as.m_Bool = value;
          m_ConstantList.push_back(c);
        }

        InterpretResult Run();

        struct Constant 
        {
          enum class Type 
          {
            Int,
            Float,
            Bool,
          };

          union Data
          {
            int m_Int;
            float m_Float;
            bool m_Bool;
          } as;

          Type m_Type;

          explicit Constant(Type type) : m_Type(type)
          {

          }

          void Print() const
          {
            switch (m_Type)
            {
              case Type::Int:
                fmt::print("{}\n", as.m_Int);
                break;
              case Type::Float:
                fmt::print("{}\n", as.m_Float);
                break;
              case Type::Bool:
                fmt::print("{}\n", as.m_Bool);
                break;
            }
          }

          std::string ToString() const
          {
            switch (m_Type)
            {
              case Type::Int:
                return fmt::format("{}", as.m_Int);
              case Type::Float:
                return fmt::format("{}", as.m_Float);
              case Type::Bool:
                return fmt::format("{}", as.m_Bool);
              default:
                return "Invalid type";
            }
          }
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
      case VM::Constant::Type::Float: name = "Type::Float"; break;
      case VM::Constant::Type::Int: name = "Type::Int"; break;
    }
    return fmt::formatter<std::string_view>::format(name, context);
  }
};
