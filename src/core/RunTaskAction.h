/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef ACTIONS_RUNTASKACTION_H_
#define ACTIONS_RUNTASKACTION_H_

#include "common/Base.h"
#include "core/Action.h"

namespace nebula_chaos {
namespace core {

using ActionTask = folly::Function<ResultCode()>;

class RunTaskAction : public Action {
public:
    RunTaskAction(ActionTask&& task)
        : task_(std::move(task)) {
    }

    ~RunTaskAction() = default;

    ResultCode doRun() override {
        if (task_) {
            return task_();
        }
        return ResultCode::ERR_BAD_ARGUMENT;
    }

    std::string toString() const override {
        return "Run task";
    }

protected:
    ActionTask task_;
};

}   // namespace core
}   // namespace nebula_chaos

#endif  // ACTIONS_RUNTASKACTION_H_
