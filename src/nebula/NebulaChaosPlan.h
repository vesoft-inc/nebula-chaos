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
};

class NebulaChaosPlan : public nebula_chaos::core::ChaosPlan {
public:
    NebulaChaosPlan(PlanContext* ctx, int32_t concurrency)
        : ChaosPlan(concurrency)
        , ctx_(ctx) {
        CHECK_NOTNULL(ctx_);
    }

    GraphClient* getGraphClient() {
        return client_.get();
    }

    /**
     * Return the action which depends on all start actions.
     * */
    Action* addStartActions();

    void addStopActions(Action* upstream);

protected:
    PlanContext* ctx_ = nullptr;
    std::unique_ptr<GraphClient> client_;
};

}   // namespace nebula
}   // namespace nebula_chaos

#endif  // NEBULA_NEBULACHAOSPLAN_H_
