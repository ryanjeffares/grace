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

namespace Grace
{
  GraceInstance::GraceInstance(std::string&& className, std::vector<Member> && members)
		: m_ClassName(std::move(className)), m_Members(std::move(members))
  {

  }

  void GraceInstance::DebugPrint() const
  {
		std::string res = ToString() + " { ";
		for (std::size_t i = 0; i < m_Members.size(); i++) {
			const auto& [name, value] = m_Members[i];
			res.append(fmt::format("{}: ", name));
			if (value.GetType() == VM::Value::Type::String) {
				res.append(fmt::format("\"{}\"", value));
			} else if (value.GetType() == VM::Value::Type::Char) {
				res.append(fmt::format("'{}'", value));
			} else {
				res.append(value.AsString());
			}

			if (i < m_Members.size() - 1) {
				res.append(", ");
			}
		}

		res.append(" }\n");
		fmt::print("{}", res);
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
		return fmt::format("<{} instance at {}>", m_ClassName, fmt::ptr(this));
  }  

  void GraceInstance::AssignMember(const std::string& memberName, VM::Value&& value)
  {
		for (auto& [name, val] : m_Members) {
			if (name == memberName) {
				val = value;
				return;
			}
		}

		throw GraceException(
			GraceException::Type::MemberNotFound,
			fmt::format("`{}` has no member name '{}'", ObjectName(), memberName)
		);
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

  GRACE_NODISCARD std::vector<GraceObject*> GraceInstance::GetObjectMembers() const
  {
		std::vector<GraceObject*> res;
		for (const auto& [name, value] : m_Members) {
			if (auto obj = value.GetObject()) {
				res.push_back(obj);
			}
		}

		return res;
  }

  GRACE_NODISCARD bool GraceInstance::AnyMemberMatches(const GraceObject* match) const
  {
		return std::any_of(m_Members.begin(), m_Members.end(), [match] (const Member& member) {
			return member.value.GetObject() == match;
		});
  }

  void GraceInstance::RemoveMember(GraceObject* object)
  {
		auto it = std::find_if(m_Members.begin(), m_Members.end(), [object] (const Member& member) {
			return member.value.GetObject() == object;
		});

		if (it != m_Members.end()) {
			m_Members.erase(it);
		}
  }

  GRACE_NODISCARD bool GraceInstance::OnlyReferenceIsSelf() const
  {
		return static_cast<std::uint32_t>(std::count_if(m_Members.begin(), m_Members.end(), [this] (const Member& member) {
			return member.value.GetObject() == this;
		})) == m_RefCount;
  }
} // namespace Grace
