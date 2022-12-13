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

#include "grace_dictionary.hpp"
#include "grace_instance.hpp"
#include "grace_list.hpp"

#include <fmt/core.h>

using namespace Grace;

static bool s_Enabled = true;
static bool s_Verbose = false;
static std::vector<GraceObject*> s_TrackedObjects;

static std::size_t s_NextSweepThreshold = 8;
static std::size_t s_GrowFactor = 2;

#ifdef GRACE_DEBUG
// track every single object that gets allocated but never remove any so we can
// set a breakpoint and make sure they're all garbage at the end of the program
static std::vector<GraceObject*> s_AllObjects;
#endif

static void CleanCycles();
static void CleanCyclesInternal();

void ObjectTracker::SetVerbose(bool state)
{
  s_Verbose = state;
}

bool ObjectTracker::GetVerbose()
{
  return s_Verbose;
}

void ObjectTracker::Collect()
{
  CleanCycles();
}

void ObjectTracker::SetEnabled(bool state)
{
  s_Enabled = state;
}

bool ObjectTracker::GetEnabled()
{
  return s_Enabled;
}

void ObjectTracker::SetGrowFactor(std::size_t state)
{
  s_GrowFactor = state;
}

std::size_t ObjectTracker::GetGrowFactor()
{
  return s_GrowFactor;
}

void ObjectTracker::SetThreshold(std::size_t threshold)
{
  s_NextSweepThreshold = threshold;
  CleanCycles();
}

std::size_t ObjectTracker::GetThreshold()
{
  return s_NextSweepThreshold;
}

void ObjectTracker::TrackObject(GraceObject* object)
{
  GRACE_ASSERT(object != nullptr, "Trying to track an object that is a nullptr");
  GRACE_ASSERT(std::find(s_TrackedObjects.begin(), s_TrackedObjects.end(), object) == s_TrackedObjects.end(),
    "Object is already being tracked");
  s_TrackedObjects.push_back(object);

#ifdef GRACE_DEBUG
  s_AllObjects.push_back(object);
#endif

  if (s_Verbose) {
    fmt::print(stderr, "Starting tracking on object at {}: ", fmt::ptr(object));
    object->DebugPrint();
  }

  if (s_Enabled) {
    CleanCycles();
  }
}

void ObjectTracker::StopTrackingObject(GraceObject* object)
{
  GRACE_ASSERT(object != nullptr, "Trying to stop tracking an object that is a nullptr");
  auto it = std::find(s_TrackedObjects.begin(), s_TrackedObjects.end(), object);

  if (it != s_TrackedObjects.end()) {
    // this function could get called from the Value destructor after the object has already been removed by CleanCycles()
    s_TrackedObjects.erase(it);

    if (s_Verbose) {
      fmt::print(stderr, "Stopped tracking on object at {}: ", fmt::ptr(object));
      object->DebugPrint();
    }
  }
}

void ObjectTracker::Finalise()
{
  CleanCyclesInternal();

  if (!s_TrackedObjects.empty()) {
#ifndef GRACE_DEBUG
    if (!s_Verbose)
      return;
#endif

    fmt::print(stderr, "Some objects are still being tracked:\n");
    for (const auto obj : s_TrackedObjects) {
      fmt::print(stderr, "\t{}: ", fmt::ptr(obj));
      obj->DebugPrint();
    }
  }
}

static void CleanObjects(const std::vector<VM::Value>& objectsToBeDeleted)
{
  for (auto& value : objectsToBeDeleted) {
    auto object = value.GetObject();
    auto it = std::find(s_TrackedObjects.begin(), s_TrackedObjects.end(), object);
    if (it != s_TrackedObjects.end()) {
      s_TrackedObjects.erase(it);
    }

    for (auto member : object->GetObjectMembers()) {
      VM::Value memberValue(member);
      object->RemoveMember(member);
      member->RemoveMember(object);
    }
  }
}

static void CleanCyclesInternal()
{
  if (s_TrackedObjects.empty())
    return;

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
  // object.member = dict;
  // ```
  //
  // in this nightmare situation (which is code you should never write, but is technically allowed)
  // the dict's reference is coming from a KeyValuePair, which the dict itself is giving a ref to
  // but the KeyValuePair is giving a ref to the Object instance which is giving a ref to the dict
  //
  // any object in the vector can be treated as a "root" in our "graph" of objects
  // if that root has a single ref, and traversing the graph can lead back itself, it can be deleted

  std::vector<VM::Value> objectsToBeDeleted;

  for (auto root : s_TrackedObjects) {
    auto type = root->ObjectType();    
    if (type == GraceObjectType::Exception || type == GraceObjectType::Iterator || type == GraceObjectType::Range || type == GraceObjectType::Function) {
      // these objects don't have members/elements
      continue;
    }

    if (root->RefCount() > 1) {
      if (type != GraceObjectType::Dictionary && root->OnlyReferenceIsSelf()) {
        objectsToBeDeleted.emplace_back(root);
        if (s_Verbose) {
          fmt::print(stderr, "Preparing to delete object at {}: ", fmt::ptr(root));
          root->DebugPrint();
        }
      }
    } else {
      std::vector<GraceObject*> visitedObjects;
      if (GraceObject::AnyMemberMatchesRecursive(root, root, visitedObjects)) {
        objectsToBeDeleted.emplace_back(root);
        if (s_Verbose) {
          fmt::print(stderr, "Preparing to delete object at {}: ", fmt::ptr(root));
          root->DebugPrint();
        }
      }
    }
  }

  // now we have a list of objects that we know need to be deleted
  // safely remove their members to stop weird recursive destructor calls
  // then clearing the vector will delete them

  CleanObjects(objectsToBeDeleted);
  objectsToBeDeleted.clear();

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

  for (auto object : s_TrackedObjects) {
    if (object->RefCount() > 1)
      continue;

    auto type = object->ObjectType();
    if (type == GraceObjectType::Exception || type == GraceObjectType::Iterator || type == GraceObjectType::Range || type == GraceObjectType::Function) {
      // these objects don't have members/elements
      continue;
    }

    auto members = object->GetObjectMembers();
    for (auto member : members) {
      if (member->RefCount() > 1)
        continue;

      auto memberType = member->ObjectType();
      if (memberType == GraceObjectType::Exception || memberType == GraceObjectType::Iterator || type == GraceObjectType::Range || type == GraceObjectType::Function) {
        // these objects don't have members/elements
        continue;
      }

      if (member->AnyMemberMatches(object)) {
        objectsToBeDeleted.emplace_back(object);
        objectsToBeDeleted.emplace_back(member);

        if (s_Verbose) {
          fmt::print(stderr, "Preparing to delete object at {}: ", fmt::ptr(object));
          object->DebugPrint();
        }

        if (s_Verbose) {
          fmt::print(stderr, "Preparing to delete object at {}: ", fmt::ptr(member));
          member->DebugPrint();
        }
      }
    }
  }

  CleanObjects(objectsToBeDeleted);
  objectsToBeDeleted.clear();
}

static void CleanCycles()
{
  if (s_TrackedObjects.size() > s_NextSweepThreshold) {
    if (s_Verbose) {
      fmt::print("PERFORMING GC SWEEP\n");
      fmt::print("\t{} Tracked Objects\n", s_TrackedObjects.size());
      fmt::print("\t{} Threshold\n", s_NextSweepThreshold);
    }

    CleanCyclesInternal();

    s_NextSweepThreshold *= s_GrowFactor;
  }
}
