/*
 *  The Grace Programming Language.
 *
 *  This file contains the GraceObject class, the base class for all Grace runtime classes. 
 *  
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#ifndef GRACE_OBJECT_HPP
#define GRACE_OBJECT_HPP

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "../grace.hpp"

namespace Grace 
{
  namespace VM
  {
    class Value;
  }

  enum class GraceObjectType
  {
    Dictionary,
    Exception,
    Instance,
    Iterator,
    KeyValuePair,
    List,
  };

  class GraceObject
  {
    public:

      virtual ~GraceObject() = default;

      virtual void DebugPrint() const = 0;
      virtual void Print(bool err) const = 0;
      virtual void PrintLn(bool err) const = 0;
      GRACE_NODISCARD virtual std::string ToString() const = 0;
      GRACE_NODISCARD virtual bool AsBool() const = 0;
      GRACE_NODISCARD virtual constexpr const char* ObjectName() const = 0;
      GRACE_NODISCARD virtual constexpr bool IsIterable() const = 0;
      GRACE_NODISCARD virtual constexpr GraceObjectType ObjectType() const = 0;

      GRACE_INLINE std::uint32_t IncreaseRef()
      {
        return ++m_RefCount;
      }

      GRACE_INLINE std::uint32_t DecreaseRef()
      {
        return --m_RefCount;
      }

      GRACE_NODISCARD GRACE_INLINE std::uint32_t RefCount() const { return m_RefCount; }

      // it would be nice to give more detailed messages here, but ObjectName() can't be called here
      // and I couldn't be bothered making them pure virtual and implementing them in every class
      GRACE_NODISCARD virtual bool AnyMemberMatches(GRACE_MAYBE_UNUSED const GraceObject* match)
      {
        GRACE_ASSERT(false, "AnyMemberMatches() should only be called on Lists, Dictionaries, and Instances");
        return false;
      }

      GRACE_NODISCARD virtual std::vector<GraceObject*> GetObjectMembers()
      {
        GRACE_ASSERT(false, "GetObjectMembers() should only be called on Lists, Dictionaries, and Instances");
        return {};
      }

      virtual void RemoveMember(GRACE_MAYBE_UNUSED GraceObject* object)
      {
        GRACE_ASSERT(false, "RemoveMember() should only be called on Lists, Dictionaries, and Instances");
      }

    protected:
      std::mutex m_Mutex;

    private:
      std::uint32_t m_RefCount = 0;      
  };
} // namespace Grace

#endif  // ifndef GRACE_OBJECT_HPP
