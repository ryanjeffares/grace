/*
 *  The Grace Programming Language.
 *
 *  This file contains the Ops enum, representing bytecode opcodes the VM can perform.
 *
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#ifndef GRACE_OPS_HPP
#define GRACE_OPS_HPP

#include "../grace.hpp"

#include <fmt/format.h>

#include <cstdint>

namespace Grace::VM
{
  enum class Ops : std::uint8_t
  {
    Add,
    AddAssign,
    And,
    AppendNamespace,
    Assert,
    AssertWithMessage,
    AssignIteratorBegin,
    AssignLocal,
    AssignMember,
    AssignSubscript,
    BitwiseAnd,
    BitwiseAndAssign,
    BitwiseNot,
    BitwiseOr,
    BitwiseOrAssign,
    BitwiseXOr,
    BitwiseXOrAssign,
    Call,
    Cast,
    CheckIteratorEnd,
    CheckType,
    CreateDictionary,
    CreateInstance,
    CreateList,
    CreateListFromCast,
    CreateRange,
    CreateSet,
    DeclareLocal,
    DestroyHeldIterator,
    Divide,
    DivideAssign,
    Dup,
    EnterTry,
    Equal,
    Exit,
    ExitTry,
    GetSubscript,
    Greater,
    GreaterEqual,
    IncrementIterator,
    IsObject,
    Jump,
    JumpIfFalse,
    JumpIfTrue,
    Less,
    LessEqual,
    LoadConstant,
    LoadLocal,
    LoadMember,
    MemberCall,
    Mod,
    ModAssign,
    Multiply,
    MultiplyAssign,
    NativeCall,
    Negate,
    Not,
    NotEqual,
    Or,
    Pop,
    PopLocal,
    PopLocals,
    Pow,
    PowAssign,
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
    ShiftLeftAssign,
    ShiftRight,
    ShiftRightAssign,
    StartNewNamespace,
    Subtract,
    SubtractAssign,
    Throw,
    Typename
  };

  struct OpLine
  {
    Ops op;
    std::size_t line;
  };
} // namespace Grace::VM

template<>
struct fmt::formatter<Grace::VM::Ops> : fmt::formatter<std::string_view>
{
  template<typename FormatContext>
  auto format(Grace::VM::Ops type, FormatContext& context) -> decltype(context.out())
  {
    using namespace Grace::VM;

    std::string_view name = "unknown";
    switch (type) {
      case Ops::Add:
        name = "Ops::Add";
        break;
      case Ops::And:
        name = "Ops::And";
        break;
      case Ops::AppendNamespace:
        name = "Ops::AppendNamespace";
        break;
      case Ops::StartNewNamespace:
        name = "Ops::StartNewNamespace";
        break;
      case Ops::Assert:
        name = "Ops::Assert";
        break;
      case Ops::AssertWithMessage:
        name = "Ops::AssertWithMessage";
        break;
      case Ops::AssignIteratorBegin:
        name = "Ops::AssignIteratorBegin";
        break;
      case Ops::AssignLocal:
        name = "Ops::AssignLocal";
        break;
      case Ops::AssignMember:
        name = "Ops::AssignMember";
        break;
      case Ops::Call:
        name = "Ops::Call";
        break;
      case Ops::Cast:
        name = "Ops::Cast";
        break;
      case Ops::CheckIteratorEnd:
        name = "Ops::CheckIteratorEnd";
        break;
      case Ops::CheckType:
        name = "Ops::CheckType";
        break;
      case Ops::CreateDictionary:
        name = "Ops::CreateDictionary";
        break;
      case Ops::CreateInstance:
        name = "Ops::CreateInstance";
        break;
      case Ops::CreateList:
        name = "Ops::CreateList";
        break;
      case Ops::CreateListFromCast:
        name = "Ops::CreateListFromCast";
        break;
      case Ops::CreateRange:
        name = "Ops::CreateRange";
        break;
      case Ops::CreateSet:
        name = "Ops::CreateSet";
        break;
      case Ops::DeclareLocal:
        name = "Ops::DeclareLocal";
        break;
      case Ops::DestroyHeldIterator:
        name = "Ops::DestroyHeldIterator";
        break;
      case Ops::Divide:
        name = "Ops::Divide";
        break;
      case Ops::Dup:
        name = "Ops::Dup";
        break;
      case Ops::EnterTry:
        name = "Ops::EnterTry";
        break;
      case Ops::Equal:
        name = "Ops::Equal";
        break;
      case Ops::Exit:
        name = "Ops::Exit";
        break;
      case Ops::ExitTry:
        name = "Ops::ExitTry";
        break;
      case Ops::Greater:
        name = "Ops::Greater";
        break;
      case Ops::GreaterEqual:
        name = "Ops::GreaterEqual";
        break;
      case Ops::IncrementIterator:
        name = "Ops::IncrementIterator";
        break;
      case Ops::IsObject:
        name = "Ops::IsObject";
        break;
      case Ops::Jump:
        name = "Ops::Jump";
        break;
      case Ops::JumpIfFalse:
        name = "Ops::JumpIfFalse";
        break;
      case Ops::JumpIfTrue:
        name = "Ops::JumpIfTrue";
        break;
      case Ops::Less:
        name = "Ops::Less";
        break;
      case Ops::LessEqual:
        name = "Ops::LessEqual";
        break;
      case Ops::LoadConstant:
        name = "Ops::LoadConstant";
        break;
      case Ops::LoadLocal:
        name = "Ops::LoadLocal";
        break;
      case Ops::LoadMember:
        name = "Ops::LoadMember";
        break;
      case Ops::MemberCall:
        name = "Ops::MemberCall";
        break;
      case Ops::Mod:
        name = "Ops::Mod";
        break;
      case Ops::Multiply:
        name = "Ops::Multiply";
        break;
      case Ops::NativeCall:
        name = "Ops::NativeCall";
        break;
      case Ops::Negate:
        name = "Ops::Negate";
        break;
      case Ops::Not:
        name = "Ops::Not";
        break;
      case Ops::NotEqual:
        name = "Ops::NotEqual";
        break;
      case Ops::Or:
        name = "Ops::Or";
        break;
      case Ops::Pop:
        name = "Ops::Pop";
        break;
      case Ops::PopLocal:
        name = "Ops::PopLocal";
        break;
      case Ops::PopLocals:
        name = "Ops::PopLocals";
        break;
      case Ops::Pow:
        name = "Ops::Pow";
        break;
      case Ops::Print:
        name = "Ops::Print";
        break;
      case Ops::PrintEmptyLine:
        name = "Ops::PrintEmptyLine";
        break;
      case Ops::PrintLn:
        name = "Ops::PrintLn";
        break;
      case Ops::PrintTab:
        name = "Ops::PrintTab";
        break;
      case Ops::EPrint:
        name = "Ops::EPrint";
        break;
      case Ops::EPrintEmptyLine:
        name = "Ops::EPrintEmptyLine";
        break;
      case Ops::EPrintLn:
        name = "Ops::EPrintLn";
        break;
      case Ops::EPrintTab:
        name = "Ops::EPrintTab";
        break;
      case Ops::Return:
        name = "Ops::Return";
        break;
      case Ops::AssignSubscript:
        name = "Ops::AssignSubscript";
        break;
      case Ops::GetSubscript:
        name = "Ops::GetSubscript";
        break;
      case Ops::Subtract:
        name = "Ops::Subtract";
        break;
      case Ops::Throw:
        name = "Ops::Throw";
        break;
      case Ops::Typename:
        name = "Ops::Typename";
        break;
      case Ops::BitwiseAnd:
        name = "Ops::BitwiseAnd";
        break;
      case Ops::BitwiseNot:
        name = "Ops::BitwiseNot";
        break;
      case Ops::BitwiseOr:
        name = "Ops::BitwiseOr";
        break;
      case Ops::BitwiseXOr:
        name = "Ops::BitwiseXOr";
        break;
      case Ops::ShiftLeft:
        name = "Ops::ShiftLeft";
        break;
      case Ops::ShiftRight:
        name = "Ops::ShiftRight";
        break;
      case Ops::AddAssign:
        name = "Ops::AddAssign";
        break;
      case Ops::SubtractAssign:
        name = "Ops::SubtractAssign";
        break;
      case Ops::DivideAssign:
        name = "Ops::DivideAssign";
        break;
      case Ops::MultiplyAssign:
        name = "Ops::MultiplyAssign";
        break;
      case Ops::ModAssign:
        name = "Ops::ModAssign";
        break;
      case Ops::BitwiseAndAssign:
        name = "Ops::BitwiseAndAssign";
        break;
      case Ops::BitwiseOrAssign:
        name = "Ops::BitwiseOrAssign";
        break;
      case Ops::BitwiseXOrAssign:
        name = "Ops::BitwiseXOrAssign";
        break;
      case Ops::ShiftLeftAssign:
        name = "Ops::ShiftLeftAssign";
        break;
      case Ops::ShiftRightAssign:
        name = "Ops::ShiftRightAssign";
        break;
      case Ops::PowAssign:
        name = "Ops::PowAssign";
        break;
    }
    return fmt::formatter<std::string_view>::format(name, context);
  }
};

#endif // ifndef GRACE_OPS_HPP