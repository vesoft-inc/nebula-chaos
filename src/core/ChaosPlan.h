/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef PLAN_CHAOSPLAN_H_
#define PLAN_CHAOSPLAN_H_

#include "common/Base.h"
#include <folly/executors/IOThreadPoolExecutor.h>

namespace nebula_chaos {
namespace core {

using ActionStatus = Action::Status;

// The plan is a DAG structure.
class ChaosPlan {
public:
    ChaosPlan(int32_t concurrency = 10) {
        threadsPool_ = std::make_unique<folly::IOThreadPoolExecutor>(concurrency);
    }

    virtual ~ChaosPlan() {
        threadsPool_->stop();
    }

    // Transfer the ownership into the plan.
    void addAction(ActionPtr action) {
        LOG(INFO) << "Add action " << action->toString() << " into plan";
        actions_.emplace_back(std::move(action));
    }

    void addActions(std::vector<ActionPtr>&& actions) {
        for (auto& action : actions) {
            LOG(INFO) << "Add action " << action->toString() << " into plan";
            actions_.emplace_back(std::move(action));
        }
    }

    void schedule() {
        for (auto& action : actions_) {
            if (action->dependees_.empty()) {
                action->run();
                continue;
            }
            std::vector<folly::Future<folly::Unit>> dependees;
            for (auto* dee : action->dependees_) {
                dependees.emplace_back(dee->promise_.getFuture());
            }
            auto actionPtr = action.get();
            folly::collectAll(dependees)
                        .via(threadsPool_.get())
                        .thenValue([this, actionPtr](auto&&) {
                            actionPtr->run();
                        })
                        .thenError([this, actionPtr](auto ew) {
                            LOG(ERROR) << "Run " << actionPtr->toString() << " failed, msg " << ew.what();
                            actionPtr->markFailed(std::move(ew));
                        });
        }
    }

    const std::vector<std::unique_ptr<Action>>& actions() const {
        return actions_;
    }

    Action* last() const {
        return actions_.back().get();
    }

protected:
    std::vector<ActionPtr> actions_;
    std::unique_ptr<folly::IOThreadPoolExecutor> threadsPool_;

};

}  // action
}  // nebula_chaos
#endif  // PLAN_CHAOSPLAN_H_
