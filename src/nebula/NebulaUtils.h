/* Copyright (c) 2020 nebula inc. All rights reserved.
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

namespace nebula_chaos {
namespace nebula {

class GraphClient;

struct LoadContext {
    std::vector<NebulaInstance*> insts;
    GraphClient*                 gClient;
    // whether to rolling table for this plan
    bool                         rolling;
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
            auto retryInterval = obj.getDefault("retry_interval_ms", 1).asInt();
            return std::make_unique<WriteCircleAction>(ctx.gClient,
                                                       tag,
                                                       col,
                                                       totalRows,
                                                       batchNum,
                                                       tryNum,
                                                       retryInterval);
        } else if (type == "WalkThroughAction") {
            auto tag = obj.at("tag").asString();
            if (ctx.rolling) {
                tag = Utils::getOperatingTable(tag);
            }
            auto col = obj.at("col").asString();
            auto totalRows = obj.getDefault("total_rows", 100000).asInt();
            auto tryNum = obj.getDefault("try_num", 32).asInt();
            auto retryInterval = obj.getDefault("retry_interval_ms", 1).asInt();
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
        } else if (type == "BalanceAction") {
            auto dataOrLeader = obj.getDefault("data_or_leader", false).asBool();
            return std::make_unique<BalanceAction>(ctx.gClient, dataOrLeader);
        } else if (type == "CheckLeadersAction") {
            auto expectedNum = obj.at("expected_num").asInt();
            return std::make_unique<CheckLeadersAction>(ctx.gClient, expectedNum);
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
            auto restartInterval = obj.getDefault("restart_interval", 30).asInt();
            auto nextLoopInterval = obj.getDefault("next_loop_interval", 30).asInt();
            auto graceful = obj.getDefault("graceful", false).asBool();
            auto cleanData = obj.getDefault("clean_data", false).asBool();
            return std::make_unique<RandomRestartAction>(targetInsts,
                                                         loopTimes,
                                                         restartInterval,
                                                         nextLoopInterval,
                                                         graceful,
                                                         cleanData);
        } else if (type == "EmptyAction") {
            auto name = obj.at("name").asString();
            return std::make_unique<core::EmptyAction>(name);
        } else if (type == "CleanWalAction") {
            auto instIndex = obj.at("inst_index").asInt();
            CHECK_GE(instIndex, 0);
            CHECK_LT(instIndex, ctx.insts.size());
            auto spaceId = obj.at("space_id").asInt();
            return std::make_unique<CleanWalAction>(ctx.insts[instIndex], spaceId);
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

