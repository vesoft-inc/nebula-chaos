/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef CORE_LOOPACTION_H_
#define CORE_LOOPACTION_H_

#include "core/Action.h"
#include <folly/executors/CPUThreadPoolExecutor.h>

namespace nebula_chaos {
namespace core {

class LoopAction : public Action {
public:
    LoopAction(ActionContext* ctx,
               const std::string& conditionExpr,
               std::vector<ActionPtr>&& actions,
               int32_t concurrency)
        : Action(ctx)
        , conditionExpr_(conditionExpr)
        , actions_(std::move(actions))
        , threadsPool_(std::make_unique<folly::CPUThreadPoolExecutor>(concurrency)) {}

    ResultCode doRun() override;

    std::string toString() const override {
        return folly::stringPrintf("Run action, the condition is %s", conditionExpr_.c_str());
    }

private:
    std::string conditionExpr_;
    std::vector<ActionPtr> actions_;
    std::unique_ptr<folly::CPUThreadPoolExecutor> threadsPool_;
};

}   // namespace core
}   // namespace nebula_chaos

#endif  // CORE_LOOPACTION_H_
