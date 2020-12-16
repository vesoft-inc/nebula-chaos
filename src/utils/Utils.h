/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef UTILS_UTILS_H_
#define UTILS_UTILS_H_

#include "common/base/Base.h"
#include <folly/Range.h>

namespace chaos {
namespace utils {

class Utils final {
public:
    /**
     * Returns a subpiece with all characters the provided @toTrim returns true
     * for removed from the front of @sp.
    */
    template <typename ToTrim>
    static folly::StringPiece ltrim(folly::StringPiece sp, ToTrim toTrim) {
        while (!sp.empty() && toTrim(sp.front())) {
            sp.pop_front();
        }
        return sp;
    }

    /**
     * Returns a subpiece with all characters the provided @toTrim returns true
    * for removed from the back of @sp.
    */
    template <typename ToTrim>
    static folly::StringPiece rtrim(folly::StringPiece sp, ToTrim toTrim) {
        while (!sp.empty() && toTrim(sp.back())) {
            sp.pop_back();
        }
        return sp;
    }

    /**
     * Returns a subpiece with all characters the provided @toTrim returns true
     * for removed from the back and front of @sp.
     */
    template <typename ToTrim>
    static folly::StringPiece trim(folly::StringPiece sp, ToTrim toTrim) {
        return ltrim(rtrim(sp, std::ref(toTrim)), std::ref(toTrim));
    }
};

}  // namespace utils
}  // namespace chaos

#endif  // UTILS_UTILS_H_
