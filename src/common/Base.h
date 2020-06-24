/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef COMMON_BASE_H_
#define COMMON_BASE_H_

#define MUST_USE_RESULT                 __attribute__((warn_unused_result))
#define DONT_OPTIMIZE                   __attribute__((optimize("O0")))

#define ALWAYS_INLINE                   __attribute__((always_inline))
#define ALWAYS_NO_INLINE                __attribute__((noinline))

#define BEGIN_NO_OPTIMIZATION           _Pragma("GCC push_options") \
                                        _Pragma("GCC optimize(\"O0\")")
#define END_NO_OPTIMIZATION             _Pragma("GCC pop_options")

#define NEBULA_STRINGIFY(STR)           NEBULA_STRINGIFY_X(STR)
#define NEBULA_STRINGIFY_X(STR)         #STR

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif  // UNUSED

#ifndef MAYBE_UNUSED
#if (__cplusplus == 201703L)   // c++17
#include <folly/CppAttributes.h>
#define MAYBE_UNUSED FOLLY_MAYBE_UNUSED
#else
#define MAYBE_UNUSED __attribute__((unused))
#endif
#endif

#ifndef COMPILER_BARRIER
#define COMPILER_BARRIER()              asm volatile ("":::"memory")
#endif  // COMPILER_BARRIER

#include <string>
#include <vector>
#include <tuple>
#include <glog/logging.h>

namespace nebula_chaos {
// hostname:port
using HostAddr = std::pair<std::string, int32_t>;

}
#endif  // COMMON_BASE_H_
