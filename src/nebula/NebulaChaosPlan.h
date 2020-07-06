/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef NEBULA_NEBULACHAOSPLAN_H_
#define NEBULA_NEBULACHAOSPLAN_H_

#include "common/Base.h"
#include "nebula/NebulaAction.h"
#include "core/ChaosPlan.h"
#include "core/Action.h"

namespace nebula_chaos {
namespace nebula {
using Action = nebula_chaos::core::Action;
using ActionPtr = nebula_chaos::core::ActionPtr;

struct PlanContext {
    std::vector<NebulaInstance>  storageds;
    std::vector<NebulaInstance>  metads;
    NebulaInstance               graphd;
    // space and schema information
    // Currently, we jsut support one space and one tag with one propery(nextId)
    // I THINK IT IS ENOUGH FOR CHAOS, IF YOU WANT MORE, BUILD YOUR OWN PLAN.
    // We want the data constrct one circle to make the check phase to be more easy.
    std::string                  space;
    int32_t                      replica;
    int32_t                      partsNum;

    std::string                  tag;
    std::vector<NameType>        props;
};

class NebulaChaosPlan : public nebula_chaos::core::ChaosPlan {
public:
    NebulaChaosPlan(std::unique_ptr<PlanContext> ctx,
                    int32_t concurrency,
                    const std::string& emailTo = "")
        : ChaosPlan(concurrency, emailTo)
        , ctx_(std::move(ctx)) {
        CHECK_NOTNULL(ctx_);
        auto port = ctx_->graphd.getPort();
        if (port.hasValue()) {
            client_ = std::make_unique<GraphClient>(ctx_->graphd.getHost(), port.value());
        }
    }

    static std::unique_ptr<NebulaChaosPlan>
    loadFromFile(const std::string& filename);

    void prepare() override {
    }

    GraphClient* getGraphClient() {
        return client_.get();
    }

    /**
     * Return the action which depends on all start actions.
     * */
    Action* addStartActions();

    void addStopActions(Action* upstream);

    /**
     * Create related space and schema on cluster
     * Return the last action currently in the plan.
     * */
    Action* createSpaceAndSchema(Action* upstream);

    Action* balanceAndCheck(Action* upstream, int32_t expectedLeaders);

    PlanContext* getContext() const {
        return ctx_.get();
    }

protected:
    std::unique_ptr<PlanContext> ctx_;
    std::unique_ptr<GraphClient> client_;
};

}   // namespace nebula
}   // namespace nebula_chaos

#endif  // NEBULA_NEBULACHAOSPLAN_H_
