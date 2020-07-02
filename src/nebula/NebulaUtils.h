/* Copyright (c) 2020 nebula inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef NEBULA_NEBULAUTILS_H_
#define NEBULA_NEBULAUTILS_H_

#include "common/Base.h"
#include <ctime>

namespace nebula_chaos {
namespace nebula {


class Utils {
public:
     /**
     * The operating table looks like "prefix_2020_07_06";
     * */
    static std::string getOperatingTable(const std::string& prefix) {
        std::time_t now = std::time(0);
        std::tm* ltm = std::localtime(&now);
        return folly::stringPrintf("%s_%d_%d_%d",
                                   prefix.c_str(),
                                   ltm->tm_year + 1900,
                                   ltm->tm_mon + 1,
                                   ltm->tm_mday);
    }

private:
    Utils() = default;
};

}  // namespace nebula
}  // namespace nebula_chaos

#endif  // NEBULA_NEBULAUTILS_H_

