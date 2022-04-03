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

#include "value.hpp"

namespace Grace
{
  namespace Compiler 
  {
    class Compiler;
  }

  namespace VM
  {

    static constexpr inline int s_CallstackLimit = 1000;
    
    enum class Ops : std::uint8_t
    {
      Add,
      And,
      AssignLocal,
      Call,
      CastAsInt,
      CastAsFloat,
      CastAsBool,
      CastAsString,
      CastAsChar,
      CheckType,
      DeclareLocal,
      Divide,
      Equal,
      Greater,
      GreaterEqual,
      Jump,
      JumpIfFalse,
      Less,
      LessEqual,
      LoadConstant,
      LoadLocal,
      Mod,
      Multiply,
      Negate,
      Not,
      NotEqual,
      Or,
      Pop,
      Pow,
      Print,
      PrintEmptyLine,
      PrintLn,
      PrintTab,
      Return,
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
      IncorrectArgCount,
      InvalidCast,
      InvalidOperand,
      InvalidType,
      CallstackDepthExceeded,
    };

    class VM
    {
      public:

        // Caller, Callee, Line
        typedef std::vector<std::tuple<std::int64_t, std::int64_t, int>> CallStack;

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
          std::int64_t m_NameHash;
          std::vector<OpLine> m_OpList; 
          std::vector<Value> m_ConstantList, m_ValueStack;
          std::vector<std::string> m_Parameters;
          int m_Line, m_Arity;

          Function(std::int64_t nameHash, std::vector<std::string>&& params, int line)
            : m_NameHash(nameHash), m_Parameters(std::move(params)), 
            m_Line(line), m_Arity(m_Parameters.size())
          {
            m_ValueStack.reserve(8);
          }
        };
        
        VM(Compiler::Compiler& compiler) : m_Compiler(compiler)
        {
    
        }

        VM(const VM&) = delete;
        VM(VM&&) = delete;

        inline void PushOp(Ops op, int line)
        {
          m_FunctionList.at(m_LastFunctionHash).m_OpList.emplace_back(op, line);
        }

        void PrintOps() const
        {
          for (const auto& [name, func] : m_FunctionList) {
            fmt::print("<function `{}`>\n", m_FunctionNames.at(name));
            for (const auto o : func.m_OpList) {
              fmt::print("\t{}\n", o.m_Op);
            }
          }
        }

        template<typename T>
        constexpr inline void PushConstant(T value)
        {
          m_FunctionList.at(m_LastFunctionHash).m_ConstantList.emplace_back(value);
        }

        inline std::size_t GetNumConstants() const
        {
          return m_FunctionList.at(m_LastFunctionHash).m_ConstantList.size();
        }

        inline std::size_t GetNumOps() const 
        {
          return m_FunctionList.at(m_LastFunctionHash).m_OpList.size();
        }

        template<typename T>
        constexpr inline void SetConstantAtIndex(std::size_t index, T value)
        {
          m_FunctionList.at(m_LastFunctionHash).m_ConstantList[index] = value;
        }

        [[nodiscard]]
        inline const std::string& GetLastFunctionName() const 
        {
          return m_FunctionNames.at(m_LastFunctionHash);
        }

        bool AddFunction(const std::string& name, int line, std::vector<std::string>&& parmeters);
        void Start(bool verbose);

      private:

        [[nodiscard]]
        static bool HandleAddition(const Value& c1, const Value& c2, std::vector<Value>& stack);

        [[nodiscard]]
        static bool HandleSubtraction(const Value& c1, const Value& c2, std::vector<Value>& stack);

        [[nodiscard]]
        static bool HandleDivision(const Value& c1, const Value& c2, std::vector<Value>& stack);

        [[nodiscard]]
        static bool HandleMultiplication(const Value& c1, const Value& c2, std::vector<Value>& stack);

        [[nodiscard]]
        static bool HandleMod(const Value& c1, const Value& c2, std::vector<Value>& stack);

        static void HandleEquality(const Value& c1, const Value& c2, std::vector<Value>& stack, bool equal);

        [[nodiscard]]
        static bool HandleLessThan(const Value& c1, const Value& c2, std::vector<Value>& stack);
        
        [[nodiscard]]
        static bool HandleLessEqual(const Value& c1, const Value& c2, std::vector<Value>& stack);

        [[nodiscard]]
        static bool HandleGreaterThan(const Value& c1, const Value& c2, std::vector<Value>& stack);

        [[nodiscard]]
        static bool HandleGreaterEqual(const Value& c1, const Value& c2, std::vector<Value>& stack);

        [[nodiscard]]
        static bool HandlePower(const Value& c1, const Value& c2, std::vector<Value>& stack);

        [[nodiscard]]
        static bool HandleNegate(const Value& c, std::vector<Value>& stack);

      private:

        InterpretResult Run(std::int64_t funcNameHash, int startLine, bool verbose);
        void RuntimeError(const std::string& message, InterpretError errorType, int line, const CallStack& callStack);

        std::unordered_map<std::int64_t, Function> m_FunctionList;
        std::unordered_map<std::int64_t, std::string> m_FunctionNames;
        std::int64_t m_LastFunctionHash;
        Compiler::Compiler& m_Compiler;
        std::hash<std::string> m_Hasher;
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
      case Ops::CastAsInt: name = "Ops::CastAsInt"; break;
      case Ops::CastAsFloat: name = "Ops::CastAsFloat"; break;
      case Ops::CastAsBool: name = "Ops::CastAsBool"; break;
      case Ops::CastAsString: name = "Ops::CastAsString"; break;
      case Ops::CastAsChar: name = "Ops::CastAsChar"; break;
      case Ops::CheckType: name = "Ops::CheckType"; break;
      case Ops::DeclareLocal: name = "Ops::DeclareLocal"; break;
      case Ops::Divide: name = "Ops::Divide"; break;
      case Ops::Equal: name = "Ops::Equal"; break;
      case Ops::Greater: name = "Ops::Greater"; break;
      case Ops::GreaterEqual: name = "Ops::GreaterEqual"; break;
      case Ops::Jump: name = "Ops::Jump"; break;
      case Ops::JumpIfFalse: name = "Ops::JumpIfFalse"; break;
      case Ops::Less: name = "Ops::Less"; break;
      case Ops::LessEqual: name = "Ops::LessEqual"; break;
      case Ops::LoadConstant: name = "Ops::LoadConstant"; break;
      case Ops::LoadLocal: name = "Ops::LoadLocal"; break;
      case Ops::Mod: name = "Ops::Mod"; break;
      case Ops::Multiply: name = "Ops::Multiply"; break;
      case Ops::Negate: name = "Ops::Negate"; break;
      case Ops::Not: name = "Ops::Not"; break;
      case Ops::NotEqual: name = "Ops::NotEqual"; break;
      case Ops::Or: name = "Ops::Or"; break;
      case Ops::Pop: name = "Ops::Pop"; break;
      case Ops::Pow: name = "Ops::Pow"; break;
      case Ops::Print: name = "Ops::Print"; break;
      case Ops::PrintEmptyLine: name = "Ops::PrintEmptyLine"; break;
      case Ops::PrintLn: name = "Ops::PrintLn"; break;
      case Ops::PrintTab: name = "Ops::PrintTab"; break;
      case Ops::Return: name = "Ops::Return"; break;
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
      case InterpretError::IncorrectArgCount: name = "IncorrectArgCount"; break;
      case InterpretError::InvalidCast: name = "InvalidCast"; break;
      case InterpretError::InvalidOperand: name = "InvalidOperand"; break;
      case InterpretError::InvalidType: name = "InvalidType"; break;
      case InterpretError::CallstackDepthExceeded: name = "CallstackDepthExceeded"; break;
    }
    return fmt::formatter<std::string_view>::format(name, context);
  }
};


