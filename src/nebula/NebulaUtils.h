/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef NEBULA_NEBULAUTILS_H_
#define NEBULA_NEBULAUTILS_H_

#include "common/Base.h"
#include <ctime>
#include <folly/Random.h>
#include "nebula/NebulaAction.h"
#include "core/WaitAction.h"
#include "core/LoopAction.h"
#include "core/AssignAction.h"
#include "nebula/NebulaChaosPlan.h"

namespace nebula_chaos {
namespace nebula {

class GraphClient;

struct LoadContext {
    std::vector<NebulaInstance*>  insts;
    GraphClient*                  gClient = nullptr;
    // whether to rolling table for this plan
    bool                          rolling;
    PlanContext*                  planCtx;
};

class Utils {
public:
     /**
     * The operating table looks like "prefix_2020_07_06";
     * */
    static std::string getOperatingTable(const std::string& prefix) {
        std::time_t now = std::time(0);
        std::tm* ltm = std::localtime(&now);
        return folly::stringPrintf("%s_%d_%d_%d",
                                   prefix.c_str(),
                                   ltm->tm_year + 1900,
                                   ltm->tm_mon + 1,
                                   ltm->tm_mday);
    }

    static std::unique_ptr<core::Action> loadAction(folly::dynamic& obj, const LoadContext& ctx) {
        auto type = obj.at("type").asString();
        LOG(INFO) << "Load action " << type;
        if (type == "StartAction") {
            auto instIndex = obj.at("inst_index").asInt();
            CHECK_GE(instIndex, 0);
            CHECK_LT(instIndex, ctx.insts.size());
            return std::make_unique<StartAction>(ctx.insts[instIndex]);
        } else if (type == "StopAction") {
            auto instIndex = obj.at("inst_index").asInt();
            CHECK_GE(instIndex, 0);
            CHECK_LT(instIndex, ctx.insts.size());
            return std::make_unique<StopAction>(ctx.insts[instIndex]);
        } else if (type == "WaitAction") {
           auto waitTimeMs = obj.at("wait_time_ms").asInt();
           CHECK_GT(waitTimeMs, 0);
           return std::make_unique<core::WaitAction>(waitTimeMs);
        } else if (type == "CrashAction") {
            auto instIndex = obj.at("inst_index").asInt();
            CHECK_GE(instIndex, 0);
            CHECK_LT(instIndex, ctx.insts.size());
            return std::make_unique<CrashAction>(ctx.insts[instIndex]);
        } else if (type == "ClientConnectAction") {
            return std::make_unique<ClientConnectAction>(ctx.gClient);
        } else if (type == "WriteCircleAction") {
            auto tag = obj.at("tag").asString();
            if (ctx.rolling) {
                tag = Utils::getOperatingTable(tag);
            }
            auto col = obj.at("col").asString();
            auto totalRows = obj.getDefault("total_rows", 100000).asInt();
            auto batchNum = obj.getDefault("batch_num", 1).asInt();
            auto tryNum = obj.getDefault("try_num", 32).asInt();
            auto retryInterval = obj.getDefault("retry_interval_ms", 100).asInt();
            return std::make_unique<WriteCircleAction>(ctx.gClient,
                                                       tag,
                                                       col,
                                                       totalRows,
                                                       batchNum,
                                                       tryNum,
                                                       retryInterval);
        } else if (type == "WriteVerticesAction") {
            auto tag = obj.at("tag").asString();
            if (ctx.rolling) {
                tag = Utils::getOperatingTable(tag);
            }
            auto col = obj.at("col").asString();
            auto totalRows = obj.getDefault("total_rows", 100000).asInt();
            auto batchNum = obj.getDefault("batch_num", 1).asInt();
            auto tryNum = obj.getDefault("try_num", 32).asInt();
            auto retryInterval = obj.getDefault("retry_interval_ms", 100).asInt();
            auto startVid = obj.getDefault("start_vid", 1).asInt();
            auto strValPrefix = obj.getDefault("str_val_prefix", "row_").asString();
            return std::make_unique<WriteVerticesAction>(ctx.gClient,
                                                         tag,
                                                         col,
                                                         totalRows,
                                                         batchNum,
                                                         tryNum,
                                                         retryInterval,
                                                         startVid,
                                                         strValPrefix);
        } else if (type == "WalkThroughAction") {
            auto tag = obj.at("tag").asString();
            if (ctx.rolling) {
                tag = Utils::getOperatingTable(tag);
            }
            auto col = obj.at("col").asString();
            auto totalRows = obj.getDefault("total_rows", 100000).asInt();
            auto tryNum = obj.getDefault("try_num", 32).asInt();
            auto retryInterval = obj.getDefault("retry_interval_ms", 100).asInt();
            return std::make_unique<WalkThroughAction>(ctx.gClient,
                                                       tag,
                                                       col,
                                                       totalRows,
                                                       tryNum,
                                                       retryInterval);
        } else if (type == "CreateSpaceAction") {
            auto spaceName = obj.at("space_name").asString();
            auto replica = obj.getDefault("replica", 3).asInt();
            auto parts = obj.getDefault("parts", 100).asInt();
            return std::make_unique<CreateSpaceAction>(ctx.gClient,
                                                       spaceName,
                                                       replica,
                                                       parts);
        } else if (type == "UseSpaceAction") {
            auto spaceName = obj.at("space_name").asString();
            return std::make_unique<UseSpaceAction>(ctx.gClient,
                                                    spaceName);
        } else if (type == "CreateSchemaAction") {
            auto name = obj.at("name").asString();
            if (ctx.rolling) {
                name = Utils::getOperatingTable(name);
            }
            auto edgeOrTag = obj.at("edge_or_tag").asBool();
            auto propsArray = obj.at("props");
            std::vector<NameType> props;
            auto it = propsArray.begin();
            while (it != propsArray.end()) {
                auto propName = it->at("name").asString();
                auto propType = it->at("type").asString();
                props.emplace_back(std::move(propName), std::move(propType));
                it++;
            }
            return std::make_unique<CreateSchemaAction>(ctx.gClient,
                                                        name,
                                                        props,
                                                        edgeOrTag);
        } else if (type == "BalanceLeaderAction") {
            return std::make_unique<BalanceLeaderAction>(ctx.gClient);
        } else if (type == "BalanceDataAction") {
            auto retry = obj.getDefault("retry", 64).asInt();
            return std::make_unique<BalanceDataAction>(ctx.gClient, retry);
        } else if (type == "CheckLeadersAction") {
            auto expectedNum = obj.at("expected_num").asInt();
            auto spaceName = obj.at("space").asString();
            return std::make_unique<CheckLeadersAction>(ctx.gClient, expectedNum, spaceName);
        } else if (type == "RandomRestartAction") {
            auto insts = obj.at("insts");
            std::vector<NebulaInstance*> targetInsts;
            auto it = insts.begin();
            while (it != insts.end()) {
                auto index = it->asInt();
                targetInsts.emplace_back(ctx.insts[index]);
                it++;
            }
            auto loopTimes = obj.getDefault("loop_times", 20).asInt();
            auto nextDistubInterval = obj.getDefault("next_loop_interval", 30).asInt();
            auto recoverInterval = obj.getDefault("restart_interval", 30).asInt();
            auto graceful = obj.getDefault("graceful", false).asBool();
            auto cleanData = obj.getDefault("clean_data", false).asBool();
            return std::make_unique<RandomRestartAction>(targetInsts,
                                                         loopTimes,
                                                         nextDistubInterval,
                                                         recoverInterval,
                                                         graceful,
                                                         cleanData);
        } else if (type == "EmptyAction") {
            auto name = obj.at("name").asString();
            return std::make_unique<core::EmptyAction>(name);
        } else if (type == "CleanWalAction") {
            auto instIndex = obj.at("inst_index").asInt();
            CHECK_GE(instIndex, 0);
            CHECK_LT(instIndex, ctx.insts.size());
            auto spaceName = obj.at("space_name").asString();
            return std::make_unique<CleanWalAction>(ctx.insts[instIndex], ctx.gClient, spaceName);
        } else if (type == "CleanDataAction") {
            auto instIndex = obj.at("inst_index").asInt();
            CHECK_GE(instIndex, 0);
            CHECK_LT(instIndex, ctx.insts.size());
            // If space_name is specified, only data of specified space is deleted,
            // otherwise, the whole data path is deleted.
            auto spaceName = obj.getDefault("space_name", "").asString();
            return std::make_unique<CleanDataAction>(ctx.insts[instIndex], ctx.gClient, spaceName);
        } else if (type == "LoopAction") {
            auto condition = obj.at("condition").asString();
            auto concurrency = obj.at("concurrency").asInt();
            auto subPlan = obj.at("sub_plan");
            std::vector<std::unique_ptr<core::Action>> actions;
            {
                auto actionIt = subPlan.begin();
                while (actionIt != subPlan.end()) {
                    CHECK(actionIt->isObject());
                    auto action = Utils::loadAction(*actionIt, ctx);
                    CHECK(action != nullptr);
                    action->setId(actions.size());
                    actions.emplace_back(std::move(action));
                    actionIt++;
                }
            }
            {
                auto actionIt = subPlan.begin();
                int32_t index = 0;
                while (actionIt != subPlan.end()) {
                    CHECK(actionIt->isObject());
                    LOG(INFO) << "Load the " << index << " action's in subplan dependees!";
                    auto depends = actionIt->at("depends");
                    for (auto& dependeeIndex : depends) {
                        auto i = dependeeIndex.asInt();
                        actions[index]->addDependee(actions[i].get());
                    }
                    actionIt++;
                    index++;
                }
            }
            return std::make_unique<core::LoopAction>(&ctx.planCtx->actionCtx,
                                                      condition,
                                                      std::move(actions),
                                                      concurrency);
        } else if (type == "RandomPartitionAction") {
            auto metaIdxs = obj.at("metas");
            std::vector<NebulaInstance*> metas;
            for (auto iter = metaIdxs.begin(); iter != metaIdxs.end(); iter++) {
                auto index = iter->asInt();
                metas.emplace_back(ctx.insts[index]);
            }
            auto storageIdxs = obj.at("storages");
            std::vector<NebulaInstance*> storages;
            for (auto iter = storageIdxs.begin(); iter != storageIdxs.end(); iter++) {
                auto index = iter->asInt();
                storages.emplace_back(ctx.insts[index]);
            }
            auto loopTimes = obj.getDefault("loop_times", 20).asInt();
            auto nextDistubInterval = obj.getDefault("next_loop_interval", 30).asInt();
            auto recoverInterval = obj.getDefault("restart_interval", 30).asInt();
            return std::make_unique<RandomPartitionAction>(metas,
                                                           storages,
                                                           loopTimes,
                                                           nextDistubInterval,
                                                           recoverInterval);
        } else if (type == "RandomTrafficControlAction") {
            auto storageIdxs = obj.at("storages");
            std::vector<NebulaInstance*> storages;
            for (auto iter = storageIdxs.begin(); iter != storageIdxs.end(); iter++) {
                auto index = iter->asInt();
                storages.emplace_back(ctx.insts[index]);
            }
            auto loopTimes = obj.getDefault("loop_times", 20).asInt();
            auto nextDistubInterval = obj.getDefault("next_loop_interval", 30).asInt();
            auto recoverInterval = obj.getDefault("restart_interval", 30).asInt();
            auto device = obj.getDefault("device", "eth0").asString();
            auto delay = obj.getDefault("delay", "100ms").asString();
            auto dist = obj.getDefault("delay-distro", "20ms").asString();
            auto loss = obj.getDefault("loss", 0).asInt();
            auto duplicate = obj.getDefault("duplicate", 0).asInt();
            return std::make_unique<RandomTrafficControlAction>(storages,
                                                                loopTimes,
                                                                nextDistubInterval,
                                                                recoverInterval,
                                                                device,
                                                                delay,
                                                                dist,
                                                                loss,
                                                                duplicate);
        } else if (type == "FillDiskAction") {
            auto storageIdxs = obj.at("storages");
            std::vector<NebulaInstance*> storages;
            for (auto iter = storageIdxs.begin(); iter != storageIdxs.end(); iter++) {
                auto index = iter->asInt();
                storages.emplace_back(ctx.insts[index]);
            }
            auto loopTimes = obj.getDefault("loop_times", 1).asInt();
            auto nextDistubInterval = obj.getDefault("next_loop_interval", 30).asInt();
            auto recoverInterval = obj.getDefault("restart_interval", 30).asInt();
            auto count = obj.getDefault("count", 1).asInt();
            return std::make_unique<FillDiskAction>(storages,
                                                    loopTimes,
                                                    nextDistubInterval,
                                                    recoverInterval,
                                                    count);
        } else if (type == "SlowDiskAction") {
            auto storageIdxs = obj.at("storages");
            std::vector<NebulaInstance*> storages;
            for (auto iter = storageIdxs.begin(); iter != storageIdxs.end(); iter++) {
                auto index = iter->asInt();
                storages.emplace_back(ctx.insts[index]);
            }
            auto loopTimes = obj.getDefault("loop_times", 1).asInt();
            auto nextDistubInterval = obj.getDefault("next_loop_interval", 30).asInt();
            auto recoverInterval = obj.getDefault("restart_interval", 30).asInt();
            auto major = obj.at("major").asInt();
            auto minor = obj.at("minor").asInt();
            auto delayMs = obj.at("delay_ms").asInt();
            return std::make_unique<SlowDiskAction>(storages,
                                                    loopTimes,
                                                    nextDistubInterval,
                                                    recoverInterval,
                                                    major,
                                                    minor,
                                                    delayMs);
        } else if (type == "CreateCheckpointAction") {
            return std::make_unique<CreateCheckpointAction>(ctx.gClient);
        } else if (type == "CleanCheckpointAction") {
            auto instIndex = obj.at("inst_index").asInt();
            CHECK_GE(instIndex, 0);
            CHECK_LT(instIndex, ctx.insts.size());
            return std::make_unique<CleanCheckpointAction>(ctx.insts[instIndex]);
        } else if (type == "RestoreFromCheckpointAction") {
            auto instIndex = obj.at("inst_index").asInt();
            CHECK_GE(instIndex, 0);
            CHECK_LT(instIndex, ctx.insts.size());
            return std::make_unique<RestoreFromCheckpointAction>(ctx.insts[instIndex]);
        } else if (type == "RestoreFromDataDirAction") {
            auto sourceDataPaths = obj.getDefault("sourceDataPaths", "").asString();
            auto instIndex = obj.at("inst_index").asInt();
            CHECK_GE(instIndex, 0);
            CHECK_LT(instIndex, ctx.insts.size());
            return std::make_unique<RestoreFromDataDirAction>(ctx.insts[instIndex],
                                                              sourceDataPaths);
        } else if (type == "AssignAction") {
            auto varName = obj.at("var_name").asString();
            auto valExpr = obj.at("value_expr").asString();
            return std::make_unique<core::AssignAction>(&ctx.planCtx->actionCtx,
                                                        varName,
                                                        valExpr);
        } else if (type == "VerifyVerticesAction") {
            auto tag = obj.at("tag").asString();
            if (ctx.rolling) {
                tag = Utils::getOperatingTable(tag);
            }
            auto col = obj.at("col").asString();
            auto totalRows = obj.getDefault("total_rows", 100000).asInt();
            auto tryNum = obj.getDefault("try_num", 32).asInt();
            auto retryInterval = obj.getDefault("retry_interval_ms", 100).asInt();
            auto startVid = obj.getDefault("start_vid", 1).asInt();
            auto strValPrefix = obj.getDefault("str_val_prefix", "row_").asString();
            auto expectedFail = obj.getDefault("expected_fail", false).asBool();
            return std::make_unique<VerifyVerticesAction>(ctx.gClient,
                                                          tag,
                                                          col,
                                                          totalRows,
                                                          tryNum,
                                                          retryInterval,
                                                          startVid,
                                                          strValPrefix,
                                                          expectedFail);
        }
    LOG(FATAL) << "Unknown type " << type;
    return nullptr;
}

static NebulaInstance* randomInstance(const std::vector<NebulaInstance*>& instances,
                                        NebulaInstance::State state) {
    std::vector<NebulaInstance*> candidate;
    for (auto* instance : instances) {
        if (instance->getState() == state) {
            candidate.emplace_back(instance);
        }
    }
    if (candidate.empty()) {
        return nullptr;
    }
    return candidate[folly::Random::rand32(candidate.size())];
}

private:
    Utils() = default;
};

}  // namespace nebula
}  // namespace nebula_chaos

#endif  // NEBULA_NEBULAUTILS_H_

