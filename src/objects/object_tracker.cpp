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

static void CleanCyclesInternal()
{
  if (s_TrackedObjects.empty()) return;

  // if any two objects have 1 reference left each and they are eachother, that is a cycle that can be deleted
  for (std::size_t i = 0; i < s_TrackedObjects.size(); i++) {
    auto obj = s_TrackedObjects[i];
    if (obj->RefCount() != 1) continue;

    auto type = obj->ObjectType();
    if (type == GraceObjectType::Exception || type == GraceObjectType::Iterator) {
      // these objects don't have members/elements
      continue;
    }

    auto objectMembers = obj->GetObjectMembers();
    for (auto member : objectMembers) {
      if (member->RefCount() == 1 && member->AnyMemberMatches(obj)) {
        // at this point, we know we have two objects that are only referencing eachother
        s_TrackedObjects.erase(s_TrackedObjects.begin() + i);

        // to safely delete these objects, make another Value that holds the pointer to increase their ref counts
        VM::Value memberValue(member), parentValue(obj);

        // remove the objects as members of one another - this will not decrease their refs to 0 and recursively invoke destructors
        obj->RemoveMember(member);

        // but that object's member could be itself...
        // var o = Object();
        // o.member = o;
        if (member != obj) {
          member->RemoveMember(obj);
        }

        // now when the Value instances go out of scope, the objects will be deleted safely
        break;
      }
    }
  }

  // but NOW we need to allow for objects caught in a >=3 way cycle, for example...
  // 
  // var first = Object();
  // var second = Object();
  // var third = Object();
  // 
  // first.child = second;
  // second.child = third;
  // third.child = first;
  //
  // in a cycle like this, if all of their reference counts are only 1 AND they all come from eachother
  // they can be safely deleted, since if they were still being referenced by a local variable in the VM or something on the stack
  // they would simply have more than 1 ref

  std::vector<GraceObject*> objectsWithOneRef, objectsToBeDeleted;

  for (auto object : s_TrackedObjects) {
    if (object->RefCount() == 1) {
      objectsWithOneRef.push_back(object);
    }
  }

  for (auto object : objectsWithOneRef) {
    auto members = object->GetObjectMembers();
    for (auto member : members) {
      if (member->RefCount() != 1) continue;

      if (std::find(objectsWithOneRef.begin(), objectsWithOneRef.end(), member) != objectsWithOneRef.end()) {
        objectsToBeDeleted.push_back(member);
      }
    }
  }

  std::vector<VM::Value> temps;
  for (auto object : objectsToBeDeleted) {
    temps.emplace_back(object);
  }

  for (auto parent : objectsToBeDeleted) {
    for (auto member : objectsToBeDeleted) {
      if (parent->AnyMemberMatches(member)) {
        parent->RemoveMember(member);
      }
      if (member->AnyMemberMatches(parent)) {
        member->RemoveMember(parent);
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
