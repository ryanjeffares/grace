/*
 *  The Grace Programming Language.
 *
 *  This file contains the out of line definitions for the GraceInstance class, used for representing instances of classes defined in Grace.
 *
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#include <fmt/core.h>

#include "grace_exception.hpp"
#include "grace_instance.hpp"
#include "../value.hpp"

namespace Grace
{
  GraceInstance::GraceInstance(std::string&& className, MemberList&& members)
	: m_ClassName(std::move(className)), m_Members(std::move(members))
  {

  }

  void GraceInstance::DebugPrint() const
  {
	fmt::print("GraceInstance<{}>: {}\n", m_ClassName, ToString());
  }

  void GraceInstance::Print(bool err) const
  {
	auto stream = err ? stderr : stdout;
	fmt::print(stream, "{}", ToString());
  }

  void GraceInstance::PrintLn(bool err) const
  {
	auto stream = err ? stderr : stdout;
	fmt::print(stream, "{}\n", ToString());
  }

  std::string GraceInstance::ToString() const
  {
	std::string res = m_ClassName + " [ ";
	for (std::size_t i = 0; i < m_Members.size(); i++) {
	  auto& [name, value] = m_Members[i];
	  if (value.GetType() == VM::Value::Type::String) {
		res += fmt::format("{}: \"{}\"", name, value);
	  } else if (value.GetType() == VM::Value::Type::Char) {
		res += fmt::format("{}: '{}'", name, value);
	  } else {
		res += fmt::format("{}: {}", name, value);
	  }
	  if (i < m_Members.size() - 1) {
		res += ", ";
	  }
	}
	return res + " ]";
  }

  GRACE_NODISCARD bool GraceInstance::AssignMember(const std::string& memberName, VM::Value&& value)
  {
	for (auto& [name, val] : m_Members) {
	  if (name == memberName) {
		val = value;
		return true;
	  }
	}

	return false;
  }

  GRACE_NODISCARD const VM::Value& GraceInstance::LoadMember(const std::string& memberName)
  {
	for (const auto& [name, val] : m_Members) {
	  if (name == memberName) {
		return val;
	  }
	}

	throw GraceException(
	  GraceException::Type::MemberNotFound,
	  fmt::format("`{}` has no member name '{}'", ObjectName(), memberName)
	);
  }
} // namespace Grace