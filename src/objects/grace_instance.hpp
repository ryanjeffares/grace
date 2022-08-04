/*
 *  The Grace Programming Language.
 *
 *  This file contains the GraceInstance class, used for representing instances of classes defined in Grace.
 *
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#ifndef GRACE_INSTANCE_HPP
#define GRACE_INSTANCE_HPP

#include <mutex>
#include <tuple>
#include <vector>

#include "../grace.hpp"

#include "grace_object.hpp"

namespace Grace
{
  namespace VM
  {
	class Value;
  }

  // TODO: in future we could provide the ability to override ToString(), AsBool()
  // and maybe even make user defined classes iterable
  class GraceInstance : public GraceObject
  {
  public:

	using MemberList = std::vector<std::pair<std::string, VM::Value>>;

	GraceInstance(std::string&& className, MemberList&&);
	~GraceInstance() override = default;

	void DebugPrint() const override;
	void Print(bool err) const override;
	void PrintLn(bool err) const override;
	GRACE_NODISCARD std::string ToString() const override;	

	GRACE_NODISCARD constexpr bool AsBool() const override
	{
	  return true;
	}

	GRACE_NODISCARD const char* ObjectName() const override
	{
	  return m_ClassName.c_str();
	}

	GRACE_NODISCARD constexpr bool IsIterable() const override
	{
	  return false;
	}

	GRACE_NODISCARD GRACE_INLINE constexpr GraceObjectType ObjectType() const override
	{
	  return GraceObjectType::Instance;
	}

	void AssignMember(const std::string& memberName, VM::Value&& value);
	GRACE_NODISCARD const VM::Value& LoadMember(const std::string& memberName);

	// needs to be thread safe
	GRACE_NODISCARD std::vector<GraceObject*> GetObjectMembers() override;
	// needs to be thread safe
	GRACE_NODISCARD bool AnyMemberMatches(const GraceObject* match) override;
	void RemoveMember(GraceObject* object) override;

  private:
	std::string m_ClassName;
	MemberList m_Members;
  };
} // namespace Grace

#endif	// ifndef GRACE_INSTANCE_HPP
