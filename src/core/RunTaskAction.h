/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef CORE_RUNTASKACTION_H_
#define CORE_RUNTASKACTION_H_

#include "common/base/Base.h"
#include "core/Action.h"

namespace chaos {
namespace core {

using ActionTask = folly::Function<ResultCode()>;

class RunTaskAction : public Action {
public:
    RunTaskAction(ActionTask&& task, const std::string& name)
        : task_(std::move(task))
        , name_(name) {
    }

    ~RunTaskAction() = default;

    ResultCode doRun() override {
        if (task_) {
            return task_();
        }
        return ResultCode::ERR_BAD_ARGUMENT;
    }

    std::string toString() override {
        return name_;
    }

protected:
    ActionTask task_;
    std::string name_;
};

}   // namespace core
}   // namespace chaos
#endif  // CORE_RUNTASKACTION_H_
