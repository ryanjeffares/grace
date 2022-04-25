/*
 *  The Grace Programming Language.
 *
 *  This file contains the ObjectTracker class, used for tracking leaks in debug mode
 *  
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#ifndef GRACE_OBJECT_TRACKER_HPP
#define GRACE_OBJECT_TRACKER_HPP

#include <vector>

#include "../grace.hpp"

namespace Grace
{
  class GraceObject;

  namespace ObjectTracker
  {
#ifdef GRACE_DEBUG  // these functions should not be called in Debug
    void TrackObject(GraceObject* object, bool verbose = false);
    void StopTracking(GraceObject* object, bool verbose = false);
    void Finalise();
#endif
  }
} // namespace Grace

#endif  // ifndef GRACE_OBJECT_TRACKER_HPP
