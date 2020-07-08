/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */
#include "nebula/NebulaChaosPlan.h"
#include "core/WaitAction.h"
#include <folly/FileUtil.h>
#include <folly/json.h>
#include "nebula/NebulaUtils.h"

namespace nebula_chaos {
namespace nebula {

// static
std::unique_ptr<NebulaChaosPlan>
NebulaChaosPlan::loadFromFile(const std::string& filename) {
    std::string jsonStr;
    if (!folly::readFile(filename.c_str(), jsonStr)) {
        LOG(ERROR) << "Parse file " << filename << " failed!";
        return nullptr;
    }
    auto jsonObj = folly::parseJson(jsonStr);
    VLOG(1) << folly::toPrettyJson(jsonObj);
    auto instances = jsonObj.at("instances");
    auto rolling = jsonObj.getDefault("rollling_table", true).asBool();
    CHECK(instances.isArray());
    auto it = instances.begin();
    auto ctx = std::make_unique<PlanContext>();
    // Ensure the vector large enough.
    ctx->storageds.reserve(instances.size());
    ctx->metads.reserve(instances.size());
    std::vector<NebulaInstance*> insts;
    while (it != instances.end()) {
        CHECK(it->isObject());
        auto type = it->at("type").asString();
        auto installPath = it->at("install_dir").asString();
        auto confPath = it->at("conf_dir").asString();
        auto host = it->at("host").asString();
        auto user = it->at("user").asString();
        LOG(INFO) << "Load inst type " << type
                  << ", installPath " << installPath
                  << ", confPath " << confPath
                  << ", host " << host
                  << ", user " << user;
        if (type == "storaged") {
            NebulaInstance inst(host,
                                installPath,
                                NebulaInstance::Type::STORAGE,
                                confPath,
                                user);
            CHECK(inst.init());
            ctx->storageds.emplace_back(std::move(inst));
            insts.emplace_back(&ctx->storageds.back());
        } else if (type == "graphd") {
            NebulaInstance inst(host,
                                installPath,
                                NebulaInstance::Type::GRAPH,
                                confPath,
                                user);
            CHECK(inst.init());
            ctx->graphd = std::move(inst);
            insts.emplace_back(&ctx->graphd);
        } else if (type == "metad") {
            NebulaInstance inst(host,
                                installPath,
                                NebulaInstance::Type::META,
                                confPath,
                                user);
            CHECK(inst.init());
            ctx->metads.emplace_back(std::move(inst));
            insts.emplace_back(&ctx->metads.back());
        } else {
            LOG(FATAL) << "Unknown type " << type;
        }
        it++;
    }
    auto concurrency = jsonObj.at("concurrency").asInt();
    auto emailTo = jsonObj.at("email").asString();
    auto plan = std::make_unique<NebulaChaosPlan>(std::move(ctx), concurrency, emailTo);
    auto actionsItem = jsonObj.at("actions");
    std::vector<std::unique_ptr<core::Action>> actions;
    LoadContext loadCtx;
    loadCtx.insts = std::move(insts);
    loadCtx.gClient = plan->getGraphClient();
    loadCtx.rolling = rolling;
    {
        auto actionIt = actionsItem.begin();
        while (actionIt != actionsItem.end()) {
            CHECK(actionIt->isObject());
            auto action = Utils::loadAction(*actionIt, loadCtx);
            action->setId(actions.size());
            actions.emplace_back(std::move(action));
            actionIt++;
        }
    }
    {
        auto actionIt = actionsItem.begin();
        int32_t index = 0;
        while (actionIt != actionsItem.end()) {
            CHECK(actionIt->isObject());
            LOG(INFO) << "Load the " << index << " action's dependees!";
            auto depends = actionIt->at("depends");
            for (auto& dependeeIndex : depends) {
                auto i = dependeeIndex.asInt();
                actions[index]->addDependee(actions[i].get());
            }
            actionIt++;
            index++;
        }
    }
    plan->addActions(std::move(actions));
    return plan;
}

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
    // wait 5s
    {
        auto waitAction = std::make_unique<core::WaitAction>(5000);
        last()->addDependency(waitAction.get());
        addAction(std::move(waitAction));
    }
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

Action* NebulaChaosPlan::createSpaceAndSchema(Action* upstream) {
    auto connectClient = std::make_unique<ClientConnectAction>(client_.get());
    upstream->addDependency(connectClient.get());
    addAction(std::move(connectClient));

    auto createSpace = std::make_unique<CreateSpaceAction>(client_.get(),
                                                           ctx_->space,
                                                           ctx_->replica,
                                                           ctx_->partsNum);
    last()->addDependency(createSpace.get());
    addAction(std::move(createSpace));

    auto useSpace = std::make_unique<UseSpaceAction>(client_.get(),
                                                     ctx_->space);
    last()->addDependency(useSpace.get());
    addAction(std::move(useSpace));

    auto createTag = std::make_unique<CreateSchemaAction>(client_.get(),
                                                          ctx_->tag,
                                                          ctx_->props,
                                                          false);
    last()->addDependency(createTag.get());
    addAction(std::move(createTag));
    // wait 5s
    {
        auto waitAction = std::make_unique<core::WaitAction>(5000);
        last()->addDependency(waitAction.get());
        addAction(std::move(waitAction));
    }
    return last();
}

Action* NebulaChaosPlan::balanceAndCheck(Action* upstream, int32_t expectedLeaders) {
    CHECK(client_ != nullptr);
    // balance leader and check
    {
        auto balanceAction = std::make_unique<BalanceAction>(client_.get(), false);
        upstream->addDependency(balanceAction.get());
        addAction(std::move(balanceAction));
    }
    // wait 15s
    {
        auto waitAction = std::make_unique<core::WaitAction>(15000);
        last()->addDependency(waitAction.get());
        addAction(std::move(waitAction));
    }
    // check leaders
    {
        auto checkLeadersAction = std::make_unique<CheckLeadersAction>(client_.get(),
                                                                       expectedLeaders);
        last()->addDependency(checkLeadersAction.get());
        addAction(std::move(checkLeadersAction));
    }
    return last();
}

}   // namespace nebula
}   // namespace nebula_chaos
