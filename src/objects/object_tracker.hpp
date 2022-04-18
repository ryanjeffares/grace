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

#pragma once

#include <vector>

#include "../grace.hpp"

namespace Grace
{
  class GraceObject;

  class ObjectTracker
  {
    public:
      
      static void TrackObject(GraceObject* object, bool verbose = false);
      static void StopTracking(GraceObject* object, bool verbose = false);
      static void Finalise();

    private:
      static std::vector<GraceObject*> m_TrackedObjects;
  };
}
