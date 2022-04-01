#pragma once

#include <cassert>

#define GRACE_UNREACHABLE()                     \
  do {                                          \
    assert(false &&                             \
        "Unreachable code detected");           \
  } while (false)                               \

