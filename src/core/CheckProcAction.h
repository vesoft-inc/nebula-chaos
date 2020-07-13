/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef CORE_CHECKPROCACTION_H_
#define CORE_CHECKPROCACTION_H_

#include "common/Base.h"
#include "core/Action.h"

namespace nebula_chaos {
namespace core {

class CheckProcAction : public Action {
public:
    CheckProcAction(const std::string& host, uint32_t pid, const std::string& user = "")
        : host_(host)
        , pid_(pid)
        , user_(user) {}

    ~CheckProcAction() = default;

    ResultCode doRun() override;

    std::string toString() const override {
        return folly::stringPrintf("check process %d on %s", pid_, host_.c_str());
    }

protected:
    std::string host_;
    uint32_t    pid_;
    std::string user_;
};

}   // namespace core
}   // namespace nebula_chaos

#endif  // CORE_CHECKPROCACTION_H_
