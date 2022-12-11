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
#include "../value.hpp"

namespace Grace
{
  // TODO: in future we could provide the ability to override ToString(), AsBool()
  // and maybe even make user defined classes iterable
  class GraceInstance : public GraceObject
  {
  public:

		struct Member
		{
			std::string name;
			VM::Value value;
		};

		GraceInstance(std::string className, std::vector<Member>&& members);
		~GraceInstance() override = default;

		void DebugPrint() const override;
		void Print(bool err) const override;
		void PrintLn(bool err) const override;
		GRACE_NODISCARD std::string ToString() const override;	

		GRACE_NODISCARD constexpr bool AsBool() const override
		{
			return true;
		}

		GRACE_NODISCARD std::string_view ObjectName() const override
		{
			return m_ClassName;
		}

		GRACE_NODISCARD constexpr bool IsIterable() const override
		{
			return false;
		}

		GRACE_NODISCARD GRACE_INLINE constexpr GraceObjectType ObjectType() const override
		{
			return GraceObjectType::Instance;
		}

		GRACE_NODISCARD GRACE_INLINE GraceInstance* GetAsInstance() override
		{
			return this;
		}

		void AssignMember(const std::string& memberName, VM::Value&& value);
		GRACE_NODISCARD const VM::Value& LoadMember(const std::string& memberName);
		GRACE_NODISCARD bool HasMember(const std::string& memberName) const;

		GRACE_NODISCARD std::vector<GraceObject*> GetObjectMembers() const override;
		GRACE_NODISCARD bool AnyMemberMatches(const GraceObject* match) const override;
		void RemoveMember(GraceObject* object) override;
		GRACE_NODISCARD bool OnlyReferenceIsSelf() const override;

		private:
		std::string m_ClassName;
		std::vector<Member> m_Members;
  };
} // namespace Grace

#endif	// ifndef GRACE_INSTANCE_HPP
