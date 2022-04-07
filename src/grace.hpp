/*
 *  The Grace Programming Language.
 *
 *  This file contains macros and definitions to be used throughout the codebase.
 *
 *  Copyright (c) 2022 - Present, Ryan Jeffares
 *  
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#pragma once

#include <cassert>

#if (__cplusplus < 201703L)
# error "C++17 is required"
#endif

#ifndef GRACE_INLINE
# if defined(__clang__) || defined(__GNUC__)
#  define GRACE_INLINE __attribute__((always_inline))
# else
#  define GRACE_INLINE inline
# endif
#endif

#ifndef GRACE_NODISCARD
# define GRACE_NODISCARD [[nodiscard]]
#endif

#ifndef GRACE_NOEXCEPT
# define GRACE_NOEXCEPT noexcept
#endif

#ifndef FMT_HEADER_ONLY
# define FMT_HEADER_ONLY
#endif

#ifndef GRACE_UNREACHABLE

  #define GRACE_UNREACHABLE()                     \
    do {                                          \
      assert(false &&                             \
          "Unreachable code detected");           \
    } while (false)                               \

#endif

#ifndef GRACE_NOT_IMPLEMENTED

  #define GRACE_NOT_IMPLEMENTED()                 \
    do {                                          \
      assert(false && "Not implemented");         \
    } while (false)                               \

#endif

#ifndef GRACE_ASSERT

  #define GRACE_ASSERT(expression, message)       \
    do {                                          \
      assert(expression && message);              \
    } while (false)                               \

#endif

