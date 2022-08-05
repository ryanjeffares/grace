/*
 *  The Grace Programming Language.
 *
 *  This file contains the out of line definitions for the ObjectTracker class, used for tracking leaks in debug mode
 *  
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#include "object_tracker.hpp"

#ifdef GRACE_CLEAN_CYCLES_ASYNC
# include <thread>
#endif

#include <fmt/core.h>

#include "grace_object.hpp"
#include "grace_list.hpp"
#include "grace_dictionary.hpp"
#include "grace_instance.hpp"

using namespace Grace;

static bool s_Verbose = false;
static std::vector<GraceObject*> s_TrackedObjects;

static void CleanCyclesInternal();

void ObjectTracker::EnableVerbose()
{
  s_Verbose = true;
}

void ObjectTracker::TrackObject(GraceObject* object)
{
  GRACE_ASSERT(object != nullptr, "Trying to track an object that is a nullptr");
  GRACE_ASSERT(std::find(s_TrackedObjects.begin(), s_TrackedObjects.end(), object) == s_TrackedObjects.end(), 
    "Object is already being tracked");
  s_TrackedObjects.push_back(object);

#ifdef GRACE_DEBUG
  if (s_Verbose) {
    fmt::print("Starting tracking on object at {}: ", fmt::ptr(object));
    object->DebugPrint();
  }
#endif
}

void ObjectTracker::StopTrackingObject(GraceObject* object)
{
  GRACE_ASSERT(object != nullptr, "Trying to stop tracking an object that is a nullptr");
  auto it = std::find(s_TrackedObjects.begin(), s_TrackedObjects.end(), object);
  if (it != s_TrackedObjects.end()) {
    // this function could get called from the Value destructor after the object has already been removed by CleanCycles()
    s_TrackedObjects.erase(it);
#ifdef GRACE_DEBUG
    if (s_Verbose) {
      fmt::print("Stopped tracking on object at {}: ", fmt::ptr(object));
      object->DebugPrint();
    }
#endif
  }
}

void ObjectTracker::Finalise()
{
  CleanCyclesInternal();

  if (!s_TrackedObjects.empty()) {
    fmt::print(stderr, "Some objects are still being tracked:\n");
    for (const auto obj : s_TrackedObjects) {
      fmt::print(stderr, "\t{}: ", fmt::ptr(obj));
      obj->DebugPrint();
    }
  }
}

// utility function to check if the root can be reached by traversing the "graph" of objects
// i.e. it is caught in a cycle
static bool FindObject(GraceObject* toFind, GraceObject* root, std::vector<GraceObject*>& visitedObjects)
{
  for (auto object : root->GetObjectMembers()) {
    if (object == toFind) {
      return true;
    } else {
      if (std::find(visitedObjects.begin(), visitedObjects.end(), object) == visitedObjects.end()) {
        visitedObjects.push_back(object);
        return FindObject(toFind, object, visitedObjects);
      }
    }
  }

  return false;
}

static void CleanCyclesInternal()
{
  if (s_TrackedObjects.empty()) return;

  // First deal with objects caught in a weird complicated cycle, for example
  // 
  // ```
  // var first = Object();
  // var second = Object();
  // var third = Object();
  // 
  // first.child = second;
  // second.child = third;
  // third.child = first;
  // ```
  // 
  // in a cycle like this, if all of their reference counts are only 1 AND they all come from eachother
  // they can be safely deleted, since if they were still also being referenced by a local variable in the VM etc
  // they would have more than 1 ref
  // 
  // an even more complicated situation could come from something like this:
  // 
  // ```
  // var object = Object();
  // var dict = {};
  // dict.insert(dict, object);
  // ```
  // 
  // in this nightmare situation (which is code you should never write, but is technically allowed)
  // the dict's reference is coming from a KeyValuePair, which the dict itself is giving a ref to
  // 
  // any object in the vector can be treated as a "root" in our "graph" of objects
  // if that root has a single ref, and traversing the graph can lead back itself, it can be deleted

  std::vector<GraceObject*> objectsToBeDeleted;

  for (std::size_t i = 0; i < s_TrackedObjects.size(); i++) {
    auto root = s_TrackedObjects[i];

    if (root->RefCount() > 1) continue;

    auto type = root->ObjectType();
    if (type == GraceObjectType::Exception || type == GraceObjectType::Iterator) {
      // these objects don't have members/elements
      continue;
    }

    std::vector<GraceObject*> visitedObjects;
    if (FindObject(root, root, visitedObjects)) {
      objectsToBeDeleted.push_back(root);
    }
  }

  for (std::size_t i = 0; i < objectsToBeDeleted.size(); i++) {
    auto object = objectsToBeDeleted[i];

    auto members = object->GetObjectMembers();
    for (std::size_t j = 0; j < members.size(); j++) {
      auto member = members[j];

      VM::Value tempObj(object), tempMember(member);
        
      objectsToBeDeleted.erase(objectsToBeDeleted.begin() + i);
      members.erase(members.begin() + j);

      object->RemoveMember(member);

      if (object != member) {
        member->RemoveMember(object);
      }
    }
  }

  // now we can deal with simple "one way" cycles, like:
  // 
  // ```
  // var o1 = Object();
  // var o2 = Object();
  // o1.member = o2;
  // o2.member = o1;
  // ```
  // 
  // if any two objects have 1 reference left each and they are eachother, that is a cycle that can be deleted
  for (std::size_t i = 0; i < s_TrackedObjects.size(); i++) {
    auto object = s_TrackedObjects[i];
    if (object->RefCount() != 1) continue;

    auto type = object->ObjectType();
    if (type == GraceObjectType::Exception || type == GraceObjectType::Iterator) {
      // these objects don't have members/elements
      continue;
    }

    auto objectMembers = object->GetObjectMembers();
    for (auto member : objectMembers) {
      if (member->RefCount() == 1 && member->AnyMemberMatches(object)) {
        // at this point, we know we have two objects that are only referencing eachother
        s_TrackedObjects.erase(s_TrackedObjects.begin() + i);

        // to safely delete these objects, make another Value that holds the pointer to increase their ref counts
        VM::Value memberValue(member), parentValue(object);

        // remove the objects as members of one another - this will not decrease their refs to 0 and recursively invoke destructors
        object->RemoveMember(member);

        // but that object's member could be itself...
        // var o = Object();
        // o.member = o;
        if (member != object) {
          member->RemoveMember(object);
        }

        // now when the Value instances go out of scope, the objects will be deleted safely
        break;
      }
    }
  }  
}

void ObjectTracker::CleanCycles()
{
#ifdef GRACE_CLEAN_CYCLES_ASYNC
  std::thread(CleanCyclesInternal).detach();
#else
  CleanCyclesInternal();
#endif
}
