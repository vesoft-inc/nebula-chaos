/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef CORE_WAITACTION_H_
#define CORE_WAITACTION_H_

#include "common/base/Base.h"
#include "core/Action.h"

namespace chaos {
namespace core {

class WaitAction : public Action {
public:
    explicit WaitAction(uint64_t waitTimeMs = 3000)
        : waitTimeMs_(waitTimeMs) {
        CHECK_LT(0, waitTimeMs);
    }

    ~WaitAction() = default;

    ResultCode doRun() override {
        usleep(waitTimeMs_ * 1000);
        return ResultCode::OK;
    }

    std::string toString() override {
        return folly::stringPrintf("wait %ldms", waitTimeMs_);
    }

protected:
    uint64_t waitTimeMs_ = 0;
};

}   // namespace core
}   // namespace chaos
#endif  // CORE_WAITACTION_H_
