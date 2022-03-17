#pragma once

#include <cstdint>
#include <exception>
#include <vector>
#include <type_traits>

#include "../include/fmt/color.h"
#include "../include/fmt/core.h"
#include "../include/fmt/format.h"

#include "constant.hpp"

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
      Pow,
      Print,
      PrintEmptyLine,
      PrintLn,
      PrintTab,
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
      case Ops::Pow: name = "Ops::Pow"; break;
      case Ops::Print: name = "Ops::Print"; break;
      case Ops::PrintEmptyLine: name = "Ops::PrintEmptyLine"; break;
      case Ops::PrintLn: name = "Ops::PrintLn"; break;
      case Ops::PrintTab: name = "Ops::PrintTab"; break;
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


