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
#include <memory>
#include <sstream>
#include <string>
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

  enum class InterpretResult
  {
    RuntimeOk,
    RuntimeError,
  };

  // TODO: there will be user defined objects that can have extension methods...
  enum class ObjectType
  {
    Bool,
    Char,
    Dict,
    Float,
    Int,
    List,
    String,
  };

  class VM
  {
    public:

      static void RegisterNatives();

      GRACE_INLINE static void PushOp(Ops op, std::size_t line)
      {
        m_FunctionLookup.at(m_LastFileNameHash).at(m_LastFunctionHash)->opList.push_back({ op, line });
      }

      static void PrintOps();

      template<BuiltinGraceType T>
      GRACE_INLINE static void PushConstant(const T& value)
      {
        m_FunctionLookup.at(m_LastFileNameHash).at(m_LastFunctionHash)->constantList.emplace_back(value);
      }

      GRACE_INLINE static void PushConstant(const Value& value)
      {
        m_FunctionLookup.at(m_LastFileNameHash).at(m_LastFunctionHash)->constantList.push_back(value);
      }

      GRACE_NODISCARD GRACE_INLINE static std::size_t GetNumConstants()
      {
        return m_FunctionLookup.at(m_LastFileNameHash).at(m_LastFunctionHash)->constantList.size();
      }

      GRACE_NODISCARD GRACE_INLINE static std::size_t GetNumOps()
      {
        return m_FunctionLookup.at(m_LastFileNameHash).at(m_LastFunctionHash)->opList.size();
      }

      template<BuiltinGraceType T>
      GRACE_INLINE static void SetConstantAtIndex(std::size_t index, const T& value)
      {
        m_FunctionLookup.at(m_LastFileNameHash).at(m_LastFunctionHash)->constantList[index] = value;
      }

      GRACE_NODISCARD GRACE_INLINE static std::optional<Ops> GetLastOp()
      {
        const auto& opList = m_FunctionLookup.at(m_LastFileNameHash).at(m_LastFunctionHash)->opList;
        return opList.empty() ? std::optional<Ops>{} : opList.back().op;
      }

      GRACE_NODISCARD GRACE_INLINE static const std::string& GetLastFunctionName()
      {
        return m_FunctionLookup.at(m_LastFileNameHash).at(m_LastFunctionHash)->name;
      }

      GRACE_NODISCARD static bool AddFunction(std::string&& name, std::size_t arity, const std::string& fileName, bool exported, bool extension, std::size_t objectNameHash = {});
      GRACE_NODISCARD static bool AddClass(std::string&& name, const std::vector<std::string>& members, const std::string& fileName, bool exported);

      GRACE_NODISCARD static std::tuple<bool, std::size_t> HasNativeFunction(const std::string& name)
      {
        auto it = std::find_if(m_NativeFunctions.begin(), m_NativeFunctions.end(), 
            [&name](const Native::NativeFunction& fn) { return fn.GetName() == name;});
        if (it == m_NativeFunctions.end()) {
          return {false, 0};
        }
        return {true, it - m_NativeFunctions.begin()};
      }

      GRACE_NODISCARD GRACE_INLINE static const Native::NativeFunction& GetNativeFunction(std::size_t index)
      {
        return m_NativeFunctions[index];
      }

      GRACE_NODISCARD static bool CombineFunctions(const std::string& mainFileName, GRACE_MAYBE_UNUSED bool verbose);
      GRACE_NODISCARD static InterpretResult Start(const std::string& mainFileName, bool verbose, const std::vector<std::string>& args);

    private:

      struct CallStackEntry
      {
        std::int64_t callerHash{}, calleeHash{};
        std::size_t line{};
        std::string fileName, calleeFileName;
        std::int64_t fileNameHash{}, calleeFileNameHash{};
      };
      
      GRACE_NODISCARD static InterpretResult Run(std::int64_t mainFileNameHash, GRACE_MAYBE_UNUSED bool verbose, const std::vector<std::string>& clArgs);
      static void RuntimeError(const GraceException& exception, std::size_t line, const std::vector<CallStackEntry>& callStack);

      struct OpLine
      {
        Ops op;
        std::size_t line;
      };

      struct Function 
      {
        std::string name;
        std::int64_t nameHash;
        std::size_t arity;

        std::string fileName;
        std::int64_t fileNameHash;
        std::vector<std::string> namespaceVec;
        std::vector<std::int64_t> namespaceHashVec;

        std::vector<OpLine> opList;
        std::vector<Value> constantList;

        std::size_t opIndexStart{}, constantIndexStart{};

        // TODO: it's possible that the Function won't need to know if it's an extension or not
        bool exported{}, extensionMethod{};

        Function(std::string&& name_, std::int64_t nameHash_, std::size_t arity_, const std::string& fileName_, bool exported_, bool extension)
          : name(std::move(name_)), nameHash(nameHash_), arity(arity_), fileName(fileName_), exported(exported_), extensionMethod(extension)
        {
          static std::hash<std::string> hasher;
          fileNameHash = static_cast<std::int64_t>(hasher(fileName_));
          std::stringstream ss(fileName.substr(0, fileName.find_last_of('.')));
          std::string part;
          while (std::getline(ss, part, '/')) {
            namespaceVec.push_back(part);
            namespaceHashVec.push_back(static_cast<std::int64_t>(hasher(part)));
          }
        }

        GRACE_INLINE bool CompareNamespace(const std::vector<std::string>& nameSpace) const
        {
          return namespaceVec == nameSpace;
        }
      };

      struct Class
      {       
        std::string name;
        std::vector<std::string> members;

        std::string fileName;
        std::int64_t fileNameHash;

        bool exported;
      };

      // { filename { function name, function } }
      static std::unordered_map<std::int64_t, std::unordered_map<std::int64_t, std::shared_ptr<Function>>> m_FunctionLookup;
      // { hash of object name, list of functions }
      static std::unordered_map<std::size_t, std::vector<std::shared_ptr<Function>>> m_ExtensionMethodLookup;
      static std::unordered_map<std::int64_t, std::string> m_FileNameLookup;

      static std::vector<Native::NativeFunction> m_NativeFunctions;

      // { filename { function name, class } }
      static std::unordered_map<std::int64_t, std::unordered_map<std::int64_t, Class>> m_ClassLookup;

      static std::vector<OpLine> m_FullOpList;
      static std::vector<Value> m_FullConstantList;

      static std::int64_t m_LastFileNameHash;
      static std::int64_t m_LastFunctionHash;
      static std::hash<std::string> m_Hasher;
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
      case Ops::Add: name = "Ops::Add"; break;
      case Ops::And: name = "Ops::And"; break;
      case Ops::AppendNamespace: name = "Ops::AppendNamespace"; break;
      case Ops::StartNewNamespace: name = "Ops::StartNewNamespace"; break;
      case Ops::Assert: name = "Ops::Assert"; break;
      case Ops::AssertWithMessage: name = "Ops::AssertWithMessage"; break;
      case Ops::AssignIteratorBegin: name = "Ops::AssignIteratorBegin"; break;
      case Ops::AssignLocal: name = "Ops::AssignLocal"; break;
      case Ops::AssignMember: name = "Ops::AssignMember"; break;
      case Ops::Call: name = "Ops::Call"; break;
      case Ops::Cast: name = "Ops::Cast"; break;
      case Ops::CheckIteratorEnd: name = "Ops::CheckIteratorEnd"; break;
      case Ops::CheckType: name = "Ops::CheckType"; break;
      case Ops::CreateDictionary: name = "Ops::CreateDictionary"; break;
      case Ops::CreateInstance: name = "Ops::CreateInstance"; break;
      case Ops::CreateList: name = "Ops::CreateList"; break;
      case Ops::CreateListFromCast: name = "Ops::CreateListFromCast"; break;
      case Ops::CreateRange: name = "Ops::CreateRange"; break;
      case Ops::CreateSet: name = "Ops::CreateSet"; break;
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
      case Ops::IsObject: name = "Ops::IsObject"; break;
      case Ops::Jump: name = "Ops::Jump"; break;
      case Ops::JumpIfFalse: name = "Ops::JumpIfFalse"; break;
      case Ops::Less: name = "Ops::Less"; break;
      case Ops::LessEqual: name = "Ops::LessEqual"; break;
      case Ops::LoadConstant: name = "Ops::LoadConstant"; break;
      case Ops::LoadLocal: name = "Ops::LoadLocal"; break;
      case Ops::LoadMember: name = "Ops::LoadMember"; break;
      case Ops::MemberCall: name = "Ops::MemberCall"; break;
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
      case Ops::AssignSubscript: name = "Ops::AssignSubscript"; break;
      case Ops::GetSubscript: name = "Ops::GetSubscript"; break;
      case Ops::Subtract: name = "Ops::Subtract"; break;
      case Ops::Throw: name = "Ops::Throw"; break;
      case Ops::Typename: name = "Ops::Typename"; break;
      case Ops::BitwiseAnd: name = "Ops::BitwiseAnd"; break;
      case Ops::BitwiseNot: name = "Ops::BitwiseNot"; break;
      case Ops::BitwiseOr: name = "Ops::BitwiseOr"; break;
      case Ops::BitwiseXOr: name = "Ops::BitwiseXOr"; break;
      case Ops::ShiftLeft: name = "Ops::ShiftLeft"; break;
      case Ops::ShiftRight: name = "Ops::ShiftRight"; break;
      case Ops::AddAssign: name = "Ops::AddAssign"; break;
      case Ops::SubtractAssign: name = "Ops::SubtractAssign"; break;
      case Ops::DivideAssign: name = "Ops::DivideAssign"; break;
      case Ops::MultiplyAssign: name = "Ops::MultiplyAssign"; break;
      case Ops::ModAssign: name = "Ops::ModAssign"; break;
      case Ops::BitwiseAndAssign: name = "Ops::BitwiseAndAssign"; break;
      case Ops::BitwiseOrAssign: name = "Ops::BitwiseOrAssign"; break;
      case Ops::BitwiseXOrAssign: name = "Ops::BitwiseXOrAssign"; break;
      case Ops::ShiftLeftAssign: name = "Ops::ShiftLeftAssign"; break;
      case Ops::ShiftRightAssign: name = "Ops::ShiftRightAssign"; break;
      case Ops::PowAssign: name = "Ops::PowAssign"; break;
    }
    return fmt::formatter<std::string_view>::format(name, context);
  }
};

#endif  // ifndef GRACE_VM_HPP
