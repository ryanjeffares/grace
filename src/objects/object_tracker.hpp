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
    void TrackObject(GraceObject* object);
    void StopTrackingObject(GraceObject* object);
    void Finalise();

    void Collect();
    
    void SetEnabled(bool state);
    bool GetEnabled();

    void SetVerbose(bool state);
    bool GetVerbose();

    void SetGrowFactor(std::size_t state);
    std::size_t GetGrowFactor();

    void SetThreshold(std::size_t state);
    std::size_t GetThreshold();
  } // namespace ObjectTracker
} // namespace Grace

#endif  // ifndef GRACE_OBJECT_TRACKER_HPP
