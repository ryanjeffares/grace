/*
 *  The Grace Programming Language.
 *
 *  This file contains the VM class, which executes compiled Grace bytecode.
 *
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#ifndef GRACE_VM_HPP
#define GRACE_VM_HPP

#include <cstdint>
#include <exception>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <type_traits>

#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/format.h>

#include "grace.hpp"
#include "native_function.hpp"
#include "objects/grace_exception.hpp"
#include "value.hpp"

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
      Assert,
      AssertWithMessage,
      AssignLocal,
      Call,
      Cast,
      CheckType,
      CreateList,
      CreateRepeatingList,
      CreateEmptyList,
      DeclareLocal,
      Divide,
      Dup,
      EnterTry,
      Equal,
      Exit,
      ExitTry,
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
      NativeCall,
      Negate,
      Not,
      NotEqual,
      Or,
      Pop,
      PopLocal,
      PopLocals,
      Pow,
      Print,
      PrintEmptyLine,
      PrintLn,
      PrintTab,
      Return,
      Subtract,
      Throw,
    };

    enum class InterpretResult
    {
      RuntimeOk,
      RuntimeError,
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
          std::string m_Name;
          std::int64_t m_NameHash;
          int m_Line, m_Arity;

          std::vector<OpLine> m_OpList;
          std::vector<Value> m_ConstantList;

          std::size_t m_OpIndexStart = 0, m_ConstantIndexStart = 0;

          Function(std::string&& name, std::int64_t nameHash, int arity, int line)
            : m_Name(std::move(name)), m_NameHash(nameHash), m_Line(line), m_Arity(arity)
          {

          }
        };
        
        VM();
        VM(const VM&) = delete;
        VM(VM&&) = delete;

        GRACE_INLINE void PushOp(Ops op, int line)
        {
          m_FunctionList.at(m_LastFunctionHash).m_OpList.emplace_back(op, line);
        }

        void PrintOps() const
        {
          for (const auto& [name, func] : m_FunctionList) {
            fmt::print("<function `{}`>\n", func.m_Name);
            for (const auto [op, line] : func.m_OpList) {
              fmt::print("{:>5} | {}\n", line, op);
            }
          }
        }

        template<typename T>
        constexpr GRACE_INLINE void PushConstant(const T& value)
        {
          m_FunctionList.at(m_LastFunctionHash).m_ConstantList.emplace_back(value);
        }

        GRACE_NODISCARD GRACE_INLINE std::size_t GetNumConstants() const
        {
          return m_FunctionList.at(m_LastFunctionHash).m_ConstantList.size();
        }

        GRACE_NODISCARD GRACE_INLINE std::size_t GetNumOps() const
        {
          return m_FunctionList.at(m_LastFunctionHash).m_OpList.size();
        }

        template<typename T>
        constexpr GRACE_INLINE void SetConstantAtIndex(std::size_t index, const T& value)
        {
          m_FunctionList.at(m_LastFunctionHash).m_ConstantList[index] = value;
        }

        GRACE_NODISCARD
        GRACE_INLINE const std::string& GetLastFunctionName() const 
        {
          return m_FunctionList.at(m_LastFunctionHash).m_Name;
        }

        bool AddFunction(std::string&& name, int line, int arity);

        GRACE_INLINE std::tuple<bool, std::size_t> HasNativeFunction(const std::string& name)
        {
          auto it = std::find_if(m_NativeFunctions.begin(), m_NativeFunctions.end(), 
              [name](const Native::NativeFunction& fn) { return fn.GetName() == name;});
          if (it == m_NativeFunctions.end()) {
            return {false, 0};
          }
          return {true, it - m_NativeFunctions.begin()};
        }

        bool CombineFunctions(bool verbose);
        InterpretResult Start(bool verbose);

      private:

        void RegisterNatives();
        InterpretResult Run(bool verbose);
        void RuntimeError(const GraceException& exception, int line, const CallStack& callStack);

      private:

        std::unordered_map<std::int64_t, Function> m_FunctionList;
        std::vector<Native::NativeFunction> m_NativeFunctions;

        std::vector<OpLine> m_FullOpList;
        std::vector<Value> m_FullConstantList;

        std::int64_t m_LastFunctionHash{};
        std::hash<std::string> m_Hasher;
    };
  } // namespace VM
} // namespace Grace

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
      case Ops::Assert: name = "Ops::Assert"; break;
      case Ops::AssertWithMessage: name = "Ops::AssertWithMessage"; break;
      case Ops::AssignLocal: name = "Ops::AssignLocal"; break;
      case Ops::Call: name = "Ops::Call"; break;
      case Ops::Cast: name = "Ops::Cast"; break;
      case Ops::CheckType: name = "Ops::CheckType"; break;
      case Ops::CreateList: name = "Ops::CreateList"; break;
      case Ops::CreateRepeatingList: name = "Ops::CreateRepeatingList"; break;
      case Ops::CreateEmptyList: name = "Ops::CreateEmptyList"; break;
      case Ops::DeclareLocal: name = "Ops::DeclareLocal"; break;
      case Ops::Divide: name = "Ops::Divide"; break;
      case Ops::Dup: name = "Ops::Dup"; break;
      case Ops::EnterTry: name = "Ops::EnterTry"; break;
      case Ops::Equal: name = "Ops::Equal"; break;
      case Ops::Exit: name = "Ops::Exit"; break;
      case Ops::ExitTry: name = "Ops::ExitTry"; break;
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
      case Ops::NativeCall: name = "Ops::NativeCall"; break;
      case Ops::Negate: name = "Ops::Negate"; break;
      case Ops::Not: name = "Ops::Not"; break;
      case Ops::NotEqual: name = "Ops::NotEqual"; break;
      case Ops::Or: name = "Ops::Or"; break;
      case Ops::Pop: name = "Ops::Pop"; break;
      case Ops::PopLocal: name = "Ops::PopLocal"; break;
      case Ops::PopLocals: name = "Ops::PopLocals"; break;
      case Ops::Pow: name = "Ops::Pow"; break;
      case Ops::Print: name = "Ops::Print"; break;
      case Ops::PrintEmptyLine: name = "Ops::PrintEmptyLine"; break;
      case Ops::PrintLn: name = "Ops::PrintLn"; break;
      case Ops::PrintTab: name = "Ops::PrintTab"; break;
      case Ops::Return: name = "Ops::Return"; break;
      case Ops::Subtract: name = "Ops::Subtract"; break;
      case Ops::Throw: name = "Ops::Throw"; break;
    }
    return fmt::formatter<std::string_view>::format(name, context);
  }
};

#endif  // ifndef GRACE_VM_HPP
