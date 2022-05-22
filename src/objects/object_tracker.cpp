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

#include <fmt/core.h>

#include "object_tracker.hpp"
#include "grace_object.hpp"

using namespace Grace;

#ifdef GRACE_DEBUG

static bool s_Verbose = false;
static std::vector<GraceObject*> s_TrackedObjects;

void ObjectTracker::EnableVerbose()
{
  s_Verbose = true;
}

void ObjectTracker::TrackObject(GraceObject* object)
{
  GRACE_ASSERT(object != nullptr, "Trying to track an object that is a nullptr");
  auto it = std::find(s_TrackedObjects.begin(), s_TrackedObjects.end(), object);
  GRACE_ASSERT(it == s_TrackedObjects.end(), "Object is already being tracked");
  s_TrackedObjects.push_back(object);

  if (s_Verbose) {
    fmt::print("Starting tracking on object at {}: ", fmt::ptr(object));
    object->DebugPrint();
  }
}

void ObjectTracker::StopTracking(GraceObject* object)
{
  GRACE_ASSERT(object != nullptr, "Trying to stop tracking an object that is a nullptr");
  auto it = std::find(s_TrackedObjects.begin(), s_TrackedObjects.end(), object);
  GRACE_ASSERT(it != s_TrackedObjects.end(), "Could not find object in tracked object list");
  s_TrackedObjects.erase(it);
  
  if (s_Verbose) {
    fmt::print("Stopped tracking on object at {}: ", fmt::ptr(object));
    object->DebugPrint();
  }
}

void ObjectTracker::Finalise()
{
  if (!s_TrackedObjects.empty()) {
    fmt::print(stderr, "Some objects are still being tracked:\n");
    for (const auto obj : s_TrackedObjects) {
      fmt::print("\t{}: ", fmt::ptr(obj));
      obj->DebugPrint();
    }
    GRACE_ASSERT_FALSE();
  }
}
#endif
