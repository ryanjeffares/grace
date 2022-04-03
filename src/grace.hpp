#pragma once

#include <cassert>

#define GRACE_UNREACHABLE()                     \
  do {                                          \
    assert(false &&                             \
        "Unreachable code detected");           \
  } while (false)                               \

#define GRACE_NOT_IMPLEMENTED()                 \
  do {                                          \
    assert(false && "Not implemented");         \
  } while (false)                               \

#define GRACE_ASSERT(expression, message)       \
  do {                                          \
    assert(expression && message);              \
  } while (false)                               \

