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

#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/format.h>

#include "grace.hpp"
#include "native_function.hpp"
#include "objects/grace_exception.hpp"
#include "value.hpp"

namespace Grace::VM
{
  enum class Ops : std::uint8_t
  {
    Add,
    And,
    Assert,
    AssertWithMessage,
    AssignIteratorBegin,
    AssignLocal,
    BitwiseAnd,
    BitwiseNot,
    BitwiseOr,
    BitwiseXOr,
    Call,
    Cast,
    CheckIteratorEnd,
    CheckType,
    CreateDictionary,
    CreateList,
    CreateRangeList,
    DeclareLocal,
    DestroyHeldIterator,
    Divide,
    Dup,
    EnterTry,
    Equal,
    Exit,
    ExitTry,
    Greater,
    GreaterEqual,
    IncrementIterator,
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
    EPrint,
    EPrintEmptyLine,
    EPrintLn,
    EPrintTab,
    Return,
    ShiftLeft,
    ShiftRight,
    Subtract,
    Throw,
    Typename
  };

  enum class InterpretResult
  {
    RuntimeOk,
    RuntimeError,
  };

  class VM
  {
    public:
      
      VM(const VM&) = delete;
      VM(VM&&) = delete;
      VM& operator=(const VM&) = delete;
      VM& operator=(VM&&) = delete;
    
    protected:

      VM();

    public:

      GRACE_NODISCARD GRACE_INLINE static VM& GetInstance()
      {
        static VM vm;
        return vm;
      }

      GRACE_INLINE void PushOp(Ops op, std::size_t line)
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

      template<BuiltinGraceType T>
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

      template<BuiltinGraceType T>
      constexpr GRACE_INLINE void SetConstantAtIndex(std::size_t index, const T& value)
      {
        m_FunctionList.at(m_LastFunctionHash).m_ConstantList[index] = value;
      }

      GRACE_NODISCARD
      GRACE_INLINE const std::string& GetLastFunctionName() const 
      {
        return m_FunctionList.at(m_LastFunctionHash).m_Name;
      }

      GRACE_NODISCARD bool AddFunction(std::string&& name, std::size_t line, std::size_t arity, const std::string& fileName);

      std::tuple<bool, std::size_t> HasNativeFunction(const std::string& name)
      {
        auto it = std::find_if(m_NativeFunctions.begin(), m_NativeFunctions.end(), 
            [&name](const Native::NativeFunction& fn) { return fn.GetName() == name;});
        if (it == m_NativeFunctions.end()) {
          return {false, 0};
        }
        return {true, it - m_NativeFunctions.begin()};
      }

      GRACE_NODISCARD GRACE_INLINE const Native::NativeFunction& GetNativeFunction(std::size_t index) const
      {
        return m_NativeFunctions[index];
      }

      GRACE_NODISCARD bool CombineFunctions(bool verbose);
      GRACE_NODISCARD InterpretResult Start(bool verbose);

    private:

      // hash of caller, hash of callee, line
      using CallStack = std::vector<std::tuple<std::int64_t, std::int64_t, std::size_t>>;

      void RegisterNatives();
      GRACE_NODISCARD InterpretResult Run(bool verbose);
      void RuntimeError(const GraceException& exception, std::size_t line, const CallStack& callStack);

    private:

      struct OpLine
      {
        Ops m_Op;
        std::size_t m_Line;

        OpLine(Ops op, std::size_t line) 
          : m_Op(op), m_Line(line)
        {

        }
      };

      struct Function 
      {
        std::string m_Name;
        std::int64_t m_NameHash;
        std::size_t m_Line, m_Arity;

        std::string m_FileName;

        std::vector<OpLine> m_OpList;
        std::vector<Value> m_ConstantList;

        std::size_t m_OpIndexStart = 0, m_ConstantIndexStart = 0;

        Function(std::string&& name, std::int64_t nameHash, std::size_t arity, std::size_t line, const std::string& fileName)
          : m_Name(std::move(name)), m_NameHash(nameHash), m_Line(line), m_Arity(arity), m_FileName(fileName)
        {

        }
      };

      std::unordered_map<std::int64_t, Function> m_FunctionList;
      std::vector<Native::NativeFunction> m_NativeFunctions;

      std::vector<OpLine> m_FullOpList;
      std::vector<Value> m_FullConstantList;

      std::int64_t m_LastFunctionHash{};
      std::hash<std::string> m_Hasher;
  };
} // namespace Grace::VM

template<>
struct fmt::formatter<Grace::VM::Ops> : fmt::formatter<std::string_view>
{
  template<typename FormatContext>
  constexpr auto format(Grace::VM::Ops type, FormatContext& context) -> decltype(context.out())
  {
    using namespace Grace::VM;

    std::string_view name = "unknown";
    switch (type) {
      case Ops::Add: name = "Ops::Add"; break;
      case Ops::And: name = "Ops::And"; break;
      case Ops::Assert: name = "Ops::Assert"; break;
      case Ops::AssertWithMessage: name = "Ops::AssertWithMessage"; break;
      case Ops::AssignIteratorBegin: name = "Ops::AssignIteratorBegin"; break;
      case Ops::AssignLocal: name = "Ops::AssignLocal"; break;
      case Ops::Call: name = "Ops::Call"; break;
      case Ops::Cast: name = "Ops::Cast"; break;
      case Ops::CheckIteratorEnd: name = "Ops::CheckIteratorEnd"; break;
      case Ops::CheckType: name = "Ops::CheckType"; break;
      case Ops::CreateDictionary: name = "Ops::CreateDictionary"; break;
      case Ops::CreateList: name = "Ops::CreateList"; break;
      case Ops::CreateRangeList: name = "Ops::CreateRangeList"; break;
      case Ops::DeclareLocal: name = "Ops::DeclareLocal"; break;
      case Ops::DestroyHeldIterator: name = "Ops::DestroyHeldIterator"; break;
      case Ops::Divide: name = "Ops::Divide"; break;
      case Ops::Dup: name = "Ops::Dup"; break;
      case Ops::EnterTry: name = "Ops::EnterTry"; break;
      case Ops::Equal: name = "Ops::Equal"; break;
      case Ops::Exit: name = "Ops::Exit"; break;
      case Ops::ExitTry: name = "Ops::ExitTry"; break;
      case Ops::Greater: name = "Ops::Greater"; break;
      case Ops::GreaterEqual: name = "Ops::GreaterEqual"; break;
      case Ops::IncrementIterator: name = "Ops::IncrementIterator"; break;
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
      case Ops::EPrint: name = "Ops::EPrint"; break;
      case Ops::EPrintEmptyLine: name = "Ops::EPrintEmptyLine"; break;
      case Ops::EPrintLn: name = "Ops::EPrintLn"; break;
      case Ops::EPrintTab: name = "Ops::EPrintTab"; break;
      case Ops::Return: name = "Ops::Return"; break;
      case Ops::Subtract: name = "Ops::Subtract"; break;
      case Ops::Throw: name = "Ops::Throw"; break;
      case Ops::Typename: name = "Ops::Typename"; break;
      case Ops::BitwiseAnd: name = "Ops:BitwiseAnd"; break;
      case Ops::BitwiseNot: name = "Ops:BitwiseNot"; break;
      case Ops::BitwiseOr: name = "Ops:BitwiseOr"; break;
      case Ops::BitwiseXOr: name = "Ops:BitwiseXOr"; break;
      case Ops::ShiftLeft: name = "Ops:ShiftLeft"; break;
      case Ops::ShiftRight: name = "Ops:ShiftRight"; break;
    }
    return fmt::formatter<std::string_view>::format(name, context);
  }
};

#endif  // ifndef GRACE_VM_HPP
