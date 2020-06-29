/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */
#include "nebula/NebulaChaosPlan.h"

namespace nebula_chaos {
namespace nebula {

Action* NebulaChaosPlan::addStartActions() {
    auto graphd = std::make_unique<StartAction>(&ctx_->graphd);
    // start storaged
    std::vector<ActionPtr> storageds;
    for (auto& inst : ctx_->storageds) {
        auto storaged = std::make_unique<StartAction>(&inst);
        storaged->addDependency(graphd.get());
        storageds.emplace_back(std::move(storaged));
    }

    // Start metad
    std::vector<ActionPtr> metads;
    for (auto& inst : ctx_->metads) {
        auto metad = std::make_unique<StartAction>(&inst);
        for (auto& storaged : storageds) {
            metad->addDependency(storaged.get());
        }
        metads.emplace_back(std::move(metad));
    }

    VLOG(1) << "Begin add start actions...";
    addActions(std::move(metads));
    addActions(std::move(storageds));
    addAction(std::move(graphd));
    return last();
}

void NebulaChaosPlan::addStopActions(Action* upstream) {
    VLOG(1) << "Begin add stop actions....";
    auto graphd = std::make_unique<StopAction>(&ctx_->graphd);
    upstream->addDependency(graphd.get());
    addAction(std::move(graphd));
    for (auto& inst : ctx_->storageds) {
        auto storaged = std::make_unique<StopAction>(&inst);
        upstream->addDependency(storaged.get());
        addAction(std::move(storaged));
    }

    for (auto& inst : ctx_->metads) {
        auto metad = std::make_unique<StopAction>(&inst);
        upstream->addDependency(metad.get());
        addAction(std::move(metad));
    }
}

}   // namespace nebula
}   // namespace nebula_chaos
