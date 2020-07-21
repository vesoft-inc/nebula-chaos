/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */
#include "core/LoopAction.h"

namespace nebula_chaos {
namespace core {

ResultCode LoopAction::doRun() {
    CHECK_NOTNULL(threadsPool_);
    std::vector<Action*> rootActions;
    std::vector<Action*> leafActions;
    for (auto& action : actions_) {
        if (action->dependees_.empty()) {
            rootActions.emplace_back(action.get());
        }
        if (action->dependers_.empty()) {
            leafActions.emplace_back(action.get());
        }
    }
    actions_.emplace_back(std::make_unique<EmptyAction>("End"));
    auto* sinkAction = actions_.back().get();
    for (auto* action : leafActions) {
        action->addDependency(sinkAction);
    }

    actions_.emplace_back(std::make_unique<EmptyAction>("Begin"));
    auto* sourceAction = actions_.back().get();
    for (auto* action : rootActions) {
        sourceAction->addDependency(action);
    }

    for (int32_t i = 0; i < loopTimes_; i++) {
        LOG(INFO) << "Loop the " << i << " times...";
        ResultCode rc = ResultCode::OK;
        for (auto& action : actions_) {
            action->reset();
            std::vector<folly::Future<folly::Unit>> dependees;
            for (auto* dee : action->dependees_) {
                dependees.emplace_back(dee->promise_->getFuture());
            }
            auto actionPtr = action.get();
            folly::collect(dependees)
                        .via(threadsPool_)
                        .thenValue([this, actionPtr](auto&&) {
                            actionPtr->run();
                            LOG(INFO) << "Run " << actionPtr->toString() << " succeeded!";
                        })
                        .thenError([this, actionPtr, &rc](auto ew) {
                            LOG(ERROR) << "Run " << actionPtr->toString() << " failed, msg " << ew.what();
                            actionPtr->markFailed(std::move(ew));
                            if (rc == ResultCode::OK) {
                                rc = ResultCode::ERR_FAILED;
                            }
                        });
        }
        sinkAction->promise_->getFuture().wait();
        if (rc != ResultCode::OK) {
            return ResultCode::ERR_FAILED;
        }
    }
    return ResultCode::OK;
}

}   // namespace core
}   // namespace nebula_chaos

