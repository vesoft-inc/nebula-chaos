/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef PLAN_CHAOSPLAN_H_
#define PLAN_CHAOSPLAN_H_

#include "common/Base.h"
#include "core/RunTaskAction.h"
#include "core/SendEmailAction.h"
#include <folly/executors/CPUThreadPoolExecutor.h>

namespace nebula_chaos {
namespace core {

using ActionStatus = Action::Status;

// The plan is a DAG structure.
class ChaosPlan {
public:
    enum class Status {
        SUCCEEDED,
        FAILED,
    };

    ChaosPlan(int32_t concurrency = 10,
              const std::string& emailTo = "",
              const std::string& planName = "") {
        emailTo_ = emailTo;
        planName_ = planName;
        threadsPool_ = std::make_unique<folly::CPUThreadPoolExecutor>(concurrency);
    }

    virtual ~ChaosPlan() {
        threadsPool_->stop();
    }

    // Transfer the ownership into the plan.
    void addAction(ActionPtr action);

    void addActions(std::vector<ActionPtr>&& actions);

    virtual void prepare() {};

    void schedule();

    const std::vector<std::unique_ptr<Action>>& actions() const {
        return actions_;
    }

    Action* last() const {
        return actions_.back().get();
    }

    std::string toString() const;

    void setAnnex(const std::string& annex) {
        annex_ = annex;
    }

protected:
    std::vector<ActionPtr> actions_;
    std::unique_ptr<folly::CPUThreadPoolExecutor> threadsPool_;
    Status status_{Status::SUCCEEDED};
    Duration  timeSpent_;
    std::string  emailTo_;
    std::string  planName_;
    std::string  annex_;
};

}  // action
}  // nebula_chaos
#endif  // PLAN_CHAOSPLAN_H_
