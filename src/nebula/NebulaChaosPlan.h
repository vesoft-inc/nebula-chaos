/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef NEBULA_NEBULACHAOSPLAN_H_
#define NEBULA_NEBULACHAOSPLAN_H_

#include "common/base/Base.h"
#include "nebula/NebulaAction.h"
#include "core/ChaosPlan.h"
#include "core/Action.h"

namespace chaos {
namespace nebula_chaos {

using Action = chaos::core::Action;
using ActionPtr = chaos::core::ActionPtr;

struct PlanContext {
    std::vector<NebulaInstance>  storageds;
    std::vector<NebulaInstance>  metads;
    NebulaInstance               graphd;
    core::ActionContext          actionCtx;
};

class NebulaChaosPlan : public chaos::core::ChaosPlan {
public:
    NebulaChaosPlan(std::unique_ptr<PlanContext> ctx,
                    int32_t concurrency,
                    const std::string& emailTo = "",
                    const std::string& planName = "")
        : ChaosPlan(concurrency, emailTo, planName)
        , ctx_(std::move(ctx)) {
        CHECK_NOTNULL(ctx_);
        auto port = ctx_->graphd.getPort();
        if (port.hasValue()) {
            client_ = std::make_unique<GraphClient>(ctx_->graphd.getHost(), port.value());
        }
    }

    static std::unique_ptr<PlanContext>
    loadInstanceFromFile(const std::string& instanceFilename, std::vector<NebulaInstance*>& insts);

    static std::unique_ptr<NebulaChaosPlan>
    loadActionFromFile(const std::string& actionFilename,
                       std::unique_ptr<PlanContext> ctx,
                       const std::vector<NebulaInstance*>& insts);

    static std::unique_ptr<NebulaChaosPlan>
    loadFromFile(const std::string& instanceFilename, const std::string& actionFilename);

    void prepare() override {
    }

    GraphClient* getGraphClient() {
        return client_.get();
    }

    PlanContext* getContext() const {
        return ctx_.get();
    }

protected:
    std::unique_ptr<PlanContext> ctx_;
    std::unique_ptr<GraphClient> client_;
};

}   // namespace nebula_chaos
}   // namespace chaos

#endif  // NEBULA_NEBULACHAOSPLAN_H_
