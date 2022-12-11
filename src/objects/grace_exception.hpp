/*
 *  The Grace Programming Language.
 *
 *  This file contains the GraceException class, used for reporting runtime errors in Grace.
 *    
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#ifndef GRACE_EXCEPTION_HPP
#define GRACE_EXCEPTION_HPP

#include <exception>
#include <string>
#include <unordered_map>

#include <fmt/format.h>

#include "grace_object.hpp"

namespace Grace
{
  namespace VM
  {
    class Value;
  }

  class GraceException : public std::exception, public GraceObject
  {
    public:
      enum class Type
      {
        AssertionFailed,
        DuplicateKey,
        Exception,
        FileWriteFailed,
        FileReadFailed,
        FunctionNotExported,
        FunctionNotFound,
        IncorrectArgCount,
        IndexOutOfRange,
        InvalidArgument,
        InvalidCast,
        InvalidCollectionOperation,
        InvalidIterator,
        InvalidOperand,
        InvalidType,
        KeyNotFound,
        LibraryLoadFailure,
        MemberNotFound,
        NamespaceNotFound,
        PathError,
        ThrownException,
      };

      GraceException(Type type, std::string message)
        : m_Type(type), m_Message(std::move(message))
      {

      }

      GraceException(std::string message)
        : m_Type(Type::Exception), m_Message(std::move(message))
      {

      }

      ~GraceException() override = default;

      GRACE_NODISCARD const char* what() const noexcept override
      {
        return s_ExceptionMessages.at(m_Type);
      }

      GRACE_NODISCARD GRACE_INLINE const std::string& Message() const
      {
        return m_Message;
      }

      GRACE_NODISCARD GRACE_INLINE Type GetType() const
      {
        return m_Type;
      }

      void DebugPrint() const override;
      void Print(bool err) const override;
      void PrintLn(bool err) const override;
      GRACE_NODISCARD std::string ToString() const override;
      GRACE_NODISCARD bool AsBool() const override;
      
      GRACE_NODISCARD GRACE_INLINE constexpr std::string_view ObjectName() const override
      {
        return "Exception";
      }

      GRACE_NODISCARD GRACE_INLINE constexpr bool IsIterable() const override
      {
        return false;
      }

      GRACE_NODISCARD GRACE_INLINE constexpr GraceObjectType ObjectType() const override
      {
        return GraceObjectType::Exception;
      }

      GRACE_NODISCARD GRACE_INLINE GraceException* GetAsException() override
      {
        return this;
      }

    private:
      Type m_Type;
      std::string m_Message;
      static std::unordered_map<Type, const char*> s_ExceptionMessages;
  };
} // namespace Grace::VM

template<>
struct fmt::formatter<Grace::GraceException::Type> : fmt::formatter<std::string_view>
{
  template<typename FormatContext>
  auto format(Grace::GraceException::Type type, FormatContext& context) -> decltype(context.out())
  {
    using namespace Grace;

    std::string_view name = "unknown";
    switch (type) {
      case GraceException::Type::AssertionFailed: name = "AssertionFailed"; break;
      case GraceException::Type::DuplicateKey: name = "DuplicateKey"; break;
      case GraceException::Type::Exception: name = "Exception"; break;
      case GraceException::Type::FileWriteFailed: name = "FileWriteFailed"; break;
      case GraceException::Type::FileReadFailed: name = "FileReadFailed"; break;
      case GraceException::Type::FunctionNotExported: name = "FunctionNotExported"; break;
      case GraceException::Type::FunctionNotFound: name = "FunctionNotFound"; break;
      case GraceException::Type::IncorrectArgCount: name = "IncorrectArgCount"; break;
      case GraceException::Type::IndexOutOfRange: name = "IndexOutOfRange"; break;
      case GraceException::Type::InvalidArgument: name = "InvalidArgument"; break;
      case GraceException::Type::InvalidCollectionOperation: name = "InvalidCollectionOperation"; break;
      case GraceException::Type::InvalidCast: name = "InvalidCast"; break;
      case GraceException::Type::InvalidIterator: name = "InvalidIterator"; break;
      case GraceException::Type::InvalidOperand: name = "InvalidOperand"; break;
      case GraceException::Type::InvalidType: name = "InvalidType"; break;
      case GraceException::Type::KeyNotFound: name = "KeyNotFound"; break;
      case GraceException::Type::LibraryLoadFailure: name = "LibraryLoadFailure"; break;
      case GraceException::Type::MemberNotFound: name = "MemberNotFound"; break;
      case GraceException::Type::NamespaceNotFound: name = "NamespaceNotFound"; break;
      case GraceException::Type::PathError: name = "PathError"; break;
      case GraceException::Type::ThrownException: name = "ThrownException"; break;
    }
    return fmt::formatter<std::string_view>::format(name, context);
  }
};

#endif  // ifndef GRACE_EXCEPTION_HPP
