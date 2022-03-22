#pragma once

#include <cstdint>
#include <exception>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <type_traits>

#include "../include/fmt/color.h"
#include "../include/fmt/core.h"
#include "../include/fmt/format.h"

#include "constant.hpp"

namespace Grace
{
  namespace Compiler 
  {
    class Compiler;
  }

  namespace VM
  {
    enum class Ops : std::uint8_t
    {
      Add,
      And,
      AssignLocal,
      Call,
      DeclareLocal,
      Divide,
      Equal,
      Greater,
      GreaterEqual,
      Less,
      LessEqual,
      LoadConstant,
      LoadLocal,
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
      FunctionNotFound,
      InvalidOperand,
    };

    class VM
    {
      public:

        // Caller, Callee, Line
        typedef std::vector<std::tuple<String, String, int>> CallStack;

        struct OpLine
        {
          Ops m_Op;
          int m_Line;

          OpLine(Ops op, int line) 
            : m_Op(op), m_Line(line)
          {

          }
        };

        struct Function 
        {
          String m_Name;
          std::vector<OpLine> m_OpList; 
          std::vector<Value> m_ConstantList;
          std::unordered_map<String, Value> m_Locals;
          int m_Line;

          Function(const String& name, int line)
            : m_Name(name), m_Line(line)
          {

          }
        };
        
        VM(Compiler::Compiler& compiler) : m_Compiler(compiler)
        {

        }

        VM(const VM&) = delete;
        VM(VM&&) = delete;

        inline void PushOp(Ops op, int line)
        {
          m_FunctionList.at(m_LastFunction).m_OpList.emplace_back(op, line);
        }

        void PrintOps() const
        {
          for (const auto& [name, func] : m_FunctionList) {
            fmt::print("<function `{}`>\n", name);
            for (const auto o : func.m_OpList) {
              fmt::print("\t{}\n", o.m_Op);
            }
          }
        }

        template<typename T>
        constexpr inline void PushConstant(T value)
        {
          m_FunctionList.at(m_LastFunction).m_ConstantList.emplace_back(value);
        }

        bool AddFunction(const String& name, int line);
        void Start(bool verbose);

      private:

        InterpretResult Run(const String& funcName, int startLine, bool verbose, CallStack& cs);
        void RuntimeError(const std::string& message, InterpretError errorType, int line, const CallStack& callStack);

        std::unordered_map<String, Function> m_FunctionList;
        String m_LastFunction;
        Compiler::Compiler& m_Compiler;
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
      case Ops::AssignLocal: name = "Ops::AssignLocal"; break;
      case Ops::Call: name = "Ops::Call"; break;
      case Ops::DeclareLocal: name = "Ops::DeclareLocal"; break;
      case Ops::Divide: name = "Ops::Divide"; break;
      case Ops::Equal: name = "Ops::Equal"; break;
      case Ops::Greater: name = "Ops::Greater"; break;
      case Ops::GreaterEqual: name = "Ops::GreaterEqual"; break;
      case Ops::Less: name = "Ops::Less"; break;
      case Ops::LessEqual: name = "Ops::LessEqual"; break;
      case Ops::LoadConstant: name = "Ops::LoadConstant"; break;
      case Ops::LoadLocal: name = "Ops::LoadLocal"; break;
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
      case InterpretError::FunctionNotFound: name = "FunctionNotFound"; break;
      case InterpretError::InvalidOperand: name = "InvalidOperand"; break;
    }
    return fmt::formatter<std::string_view>::format(name, context);
  }
};


