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
#include <string_view>
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
    List,
    Dictionary,
    Exception,
    KeyValuePair,
    Instance,
    Iterator,
  };

  class GraceList;
  class GraceDictionary;
  class GraceException;
  class GraceKeyValuePair;
  class GraceInstance;
  class GraceIterator;

  class GraceObject
  {
    public:

      virtual ~GraceObject() = default;

      virtual void DebugPrint() const = 0;
      virtual void Print(bool err) const = 0;
      virtual void PrintLn(bool err) const = 0;
      GRACE_NODISCARD virtual std::string ToString() const = 0;
      GRACE_NODISCARD virtual bool AsBool() const = 0;
      GRACE_NODISCARD virtual constexpr std::string_view ObjectName() const = 0;
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
      GRACE_NODISCARD virtual bool AnyMemberMatches(GRACE_MAYBE_UNUSED const GraceObject* match) const
      {
        GRACE_ASSERT(false, "AnyMemberMatches() should only be called on Lists, Dictionaries, and Instances");
        return false;
      }

      GRACE_NODISCARD virtual std::vector<GraceObject*> GetObjectMembers() const
      {
        GRACE_ASSERT(false, "GetObjectMembers() should only be called on Lists, Dictionaries, and Instances");
        return {};
      }

      virtual void RemoveMember(GRACE_MAYBE_UNUSED GraceObject* object)
      {
        GRACE_ASSERT(false, "RemoveMember() should only be called on Lists, Dictionaries, and Instances");
      }

      GRACE_NODISCARD virtual bool OnlyReferenceIsSelf() const
      {
        GRACE_ASSERT(false, "OnlyReferenceIsSelf() should only be called on Lists and Instances");
        return false;
      }

      // the derived classes can overload the respective function to avoid dynamic_casts 
      GRACE_NODISCARD GRACE_INLINE virtual GraceList* GetAsList() { return nullptr; }
      GRACE_NODISCARD GRACE_INLINE virtual GraceDictionary* GetAsDictionary() { return nullptr; }
      GRACE_NODISCARD GRACE_INLINE virtual GraceException* GetAsException() { return nullptr; }
      GRACE_NODISCARD GRACE_INLINE virtual GraceKeyValuePair* GetAsKeyValuePair() { return nullptr; }
      GRACE_NODISCARD GRACE_INLINE virtual GraceInstance* GetAsInstance() { return nullptr; }
      GRACE_NODISCARD GRACE_INLINE virtual GraceIterator* GetAsIterator() { return nullptr; }


      GRACE_NODISCARD static bool AnyMemberMatchesRecursive(const GraceObject* toFind, GraceObject* root, std::vector<GraceObject*>& visitedObjects)
      {
        for (auto object : root->GetObjectMembers()) {
          if (object == toFind) {
            return true;
          } else {
            if (std::find(visitedObjects.begin(), visitedObjects.end(), object) == visitedObjects.end()) {
              visitedObjects.push_back(object);
              if (AnyMemberMatchesRecursive(toFind, object, visitedObjects)) {
                return true;
              }
            }
          }
        }

        return false;
      }

    protected:
      std::uint32_t m_RefCount = 0;      
  };
} // namespace Grace

#endif  // ifndef GRACE_OBJECT_HPP
