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

namespace Grace::VM
{
  class GraceException : public std::exception, public GraceObject
  {
    public:
      enum class Type
      {
        AssertionFailed,
        FunctionNotFound,
        IncorrectArgCount,
        InvalidArgument,
        InvalidCast,
        InvalidOperand,
        InvalidType,
      };

      GraceException(Type type, std::string&& message)
        : m_Type(type), m_Message(std::move(message))
      {

      }

      const char* what() const noexcept override
      {
        return s_ExceptionMessages.at(m_Type);
      }

      GRACE_INLINE std::string Message() const
      {
        return m_Message;
      }

      void DebugPrint() const override;
      void Print() const override;
      void PrintLn() const override;
      std::string ToString() const override;
      bool AsBool() const override;

    private:
      Type m_Type;
      std::string m_Message;
      static std::unordered_map<Type, const char*> s_ExceptionMessages;
  };
} // namespace Grace::VM

template<>
struct fmt::formatter<Grace::VM::GraceException::Type> : fmt::formatter<std::string_view>
{
  template<typename FormatContext>
  auto format(Grace::VM::GraceException::Type type, FormatContext& context) -> decltype(context.out())
  {
    using namespace Grace::VM;

    std::string_view name = "unknown";
    switch (type) {
      case GraceException::Type::AssertionFailed: name = "AssertionFailed"; break;
      case GraceException::Type::FunctionNotFound: name = "FunctionNotFound"; break;
      case GraceException::Type::IncorrectArgCount: name = "IncorrectArgCount"; break;
      case GraceException::Type::InvalidArgument: name = "InvalidArgument"; break;
      case GraceException::Type::InvalidCast: name = "InvalidCast"; break;
      case GraceException::Type::InvalidOperand: name = "InvalidOperand"; break;
      case GraceException::Type::InvalidType: name = "InvalidType"; break;
    }
    return fmt::formatter<std::string_view>::format(name, context);
  }
};

#endif  // ifdef GRACE_EXCEPTION_HPP
