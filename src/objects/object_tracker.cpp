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

static std::vector<GraceObject*> s_TrackedObjects;

void ObjectTracker::TrackObject(GraceObject* object, bool verbose)
{
  GRACE_ASSERT(object != nullptr, "Trying to track an object that is a nullptr");
  auto it = std::find(s_TrackedObjects.begin(), s_TrackedObjects.end(), object);
  GRACE_ASSERT(it == s_TrackedObjects.end(), "Object is already being tracked");
  s_TrackedObjects.push_back(object);

  if (verbose) {
    fmt::print("Starting tracking on object at {}: {}\n", fmt::ptr(object), object->ToString());  
  }
}

void ObjectTracker::StopTracking(GraceObject* object, bool verbose)
{
  GRACE_ASSERT(object != nullptr, "Trying to stop tracking an object that is a nullptr");
  auto it = std::find(s_TrackedObjects.begin(), s_TrackedObjects.end(), object);
  GRACE_ASSERT(it != s_TrackedObjects.end(), "Could not find object in tracked object list");
  s_TrackedObjects.erase(it);
  
  if (verbose) {
    fmt::print("Stopped tracking on object at {}: {}\n", fmt::ptr(object), object->ToString());  
  }
}

void ObjectTracker::Finalise()
{
  if (!s_TrackedObjects.empty()) {
    fmt::print(stderr, "Some objects are still being tracked:\n");
    for (const auto obj : s_TrackedObjects) {
      fmt::print("\t{}: {}\n", fmt::ptr(obj), obj->ToString());
    }
    GRACE_ASSERT_FALSE();
  }
}
