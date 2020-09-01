/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef CORE_ASSIGNACTION_H_
#define CORE_ASSIGNACTION_H_

#include "common/Base.h"
#include "core/Action.h"
#include "parser/ParserHelper.h"

namespace nebula_chaos {
namespace core {

class AssignAction : public Action {
public:
    AssignAction(ActionContext* ctx,
                 const std::string& var,
                 const std::string& expr)
        : Action(ctx)
        , var_(var)
        , exprStr_(expr) {}

    ~AssignAction() = default;

    ResultCode doRun() override {
        auto expr = ParserHelper::parse(exprStr_);
        if (expr == nullptr) {
            return ResultCode::ERR_FAILED;
        }
        auto valOrErr = expr->eval(&ctx_->exprCtx);
        if (!valOrErr) {
            LOG(ERROR) << "Eval " << exprStr_ << " failed!";
            return ResultCode::ERR_FAILED;
        }
        auto val = std::move(valOrErr).value();
        ctx_->exprCtx.setVar(var_, std::move(val));
        return ResultCode::OK;
    }

    std::string toString() const override {
        return folly::stringPrintf("$%s=%s", var_.c_str(), exprStr_.c_str());
    }

private:
    std::string var_;
    std::string exprStr_;
};

}   // namespace core
}   // namespace nebula_chaos

#endif  // CORE_ASSIGNACTION_H_
