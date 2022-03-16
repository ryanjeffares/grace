#pragma once

#include <cassert>

#if defined (_DEBUG) || !defined(NDEBUG) || defined(DEBUG)
# define GRACE_DEBUG
#endif

#define GRACE_UNREACHABLE()                     \
  do {                                          \
    assert(false &&                             \
        "Unreachable code detected");  \
  } while (false)                               \

