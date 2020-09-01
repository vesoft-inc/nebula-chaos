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

DEFINE_string(email_to, "", "");

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
    auto planName = jsonObj.at("name").asString();
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
    auto emailTo = jsonObj.getDefault("email", FLAGS_email_to).asString();
    auto plan = std::make_unique<NebulaChaosPlan>(std::move(ctx), concurrency, emailTo, planName);
    auto actionsItem = jsonObj.at("actions");
    std::vector<std::unique_ptr<core::Action>> actions;
    LoadContext loadCtx;
    loadCtx.insts = std::move(insts);
    loadCtx.gClient = plan->getGraphClient();
    loadCtx.rolling = rolling;
    loadCtx.planCtx = plan->getContext();
    {
        auto actionIt = actionsItem.begin();
        while (actionIt != actionsItem.end()) {
            CHECK(actionIt->isObject());
            auto action = Utils::loadAction(*actionIt, loadCtx);
            CHECK(action != nullptr);
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

}   // namespace nebula
}   // namespace nebula_chaos
