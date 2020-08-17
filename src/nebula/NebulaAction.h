/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef NEBULA_CRASHACTION_H_
#define NEBULA_CRASHACTION_H_

#include "core/Action.h"
#include "nebula/NebulaInstance.h"
#include "nebula/client/GraphClient.h"
#include <folly/Expected.h>

namespace nebula_chaos {
namespace nebula {
using ResultCode = nebula_chaos::core::ResultCode;

class CrashAction : public core::Action {
public:
    CrashAction(NebulaInstance* inst)
        : inst_(inst) {}

    ~CrashAction() = default;

    ResultCode doRun() override;

    std::string toString() const override {
        CHECK_NOTNULL(inst_);
        auto hostAddr = inst_->toString();
        return folly::stringPrintf("kill -9 %s", hostAddr.c_str());
    }

private:
    NebulaInstance* inst_ = nullptr;
};

class StartAction : public core::Action {
public:
    StartAction(NebulaInstance* inst)
        : inst_(inst) {
        VLOG(1) << "Construct StartAction for " << inst_->toString();
    }

    ~StartAction() = default;

    ResultCode doRun() override;

    std::string toString() const override {
        CHECK_NOTNULL(inst_);
        auto hostAddr = inst_->toString();
        return folly::stringPrintf("start %s", hostAddr.c_str());
    }

private:
    NebulaInstance* inst_ = nullptr;
};

class StopAction : public core::Action {
public:
    StopAction(NebulaInstance* inst)
        : inst_(inst) {}

    ~StopAction() = default;

    ResultCode doRun() override;

    std::string toString() const override {
        CHECK_NOTNULL(inst_);
        auto hostAddr = inst_->toString();
        return folly::stringPrintf("stop %s", hostAddr.c_str());
    }

private:
    NebulaInstance* inst_ = nullptr;
};

class ClientConnectAction : public core::Action {
public:
    ClientConnectAction(GraphClient* client)
        : client_(client) {}

    virtual ~ClientConnectAction() = default;

    ResultCode doRun() override {
        CHECK_NOTNULL(client_);
        int retry = 0;
        while (++retry < 32) {
            auto code = client_->connect("user", "password");
            if (Code::SUCCEEDED == code) {
                return ResultCode::OK;
            }
            LOG(ERROR) << "connect failed, retry " << retry;
            sleep(retry);
        }
        return ResultCode::ERR_FAILED;
    }

    std::string toString() const override {
        return folly::stringPrintf("Connect to %s", client_->serverAddress().c_str());
    }

private:
    GraphClient* client_ = nullptr;
};

class WriteCircleAction : public core::Action {
public:
    WriteCircleAction(GraphClient* client,
                      const std::string& tag,
                      const std::string& col,
                      uint64_t totalRows = 100000,
                      uint32_t batchNum = 1,
                      uint32_t rowSize = 10,
                      uint64_t startId = 1,
                      uint32_t tryNum = 32,
                      uint32_t retryIntervalMs = 100)
        : client_(client)
        , tag_(tag)
        , col_(col)
        , totalRows_(totalRows)
        , batchNum_(batchNum)
        , rowSize_(rowSize)
        , startId_(startId)
        , try_(tryNum)
        , retryIntervalMs_(retryIntervalMs) {}

    virtual ~WriteCircleAction() = default;

    ResultCode doRun() override;

    std::string toString() const override {
        return folly::stringPrintf("Write data to %s", client_->serverAddress().c_str());
    }

private:
   ResultCode sendBatch(const std::vector<std::string>& batchCmds);

   std::string genData();

private:
    GraphClient* client_ = nullptr;
    std::string tag_;
    std::string col_;
    uint64_t    totalRows_;
    uint32_t    batchNum_;
    uint32_t    rowSize_;
    uint64_t    startId_;
    uint32_t    try_;
    uint32_t    retryIntervalMs_;
};

class WalkThroughAction : public core::Action {
public:
    WalkThroughAction(GraphClient* client,
                      const std::string& tag,
                      const std::string& col,
                      uint64_t totalRows,
                      uint32_t tryNum = 32,
                      uint32_t retryIntervalMs = 1)
        : client_(client)
        , tag_(tag)
        , col_(col)
        , totalRows_(totalRows)
        , try_(tryNum)
        , retryIntervalMs_(retryIntervalMs) {}

    ~WalkThroughAction() = default;

    ResultCode doRun() override;

    std::string toString() const override {
        return folly::stringPrintf("Wark through the circle, from %ld, total %ld",
                                   start_,
                                   totalRows_);
    }

private:
    folly::Expected<uint64_t, ResultCode> sendCommand(const std::string& cmd);

private:
    GraphClient* client_ = nullptr;
    std::string  tag_;
    std::string  col_;
    uint64_t     totalRows_;
    uint64_t     start_ = 0;
    uint32_t     try_;
    uint32_t     retryIntervalMs_;
};

/**
 * The action will change the meta on the cluster.
 * */
class MetaAction : public core::Action {
public:
    MetaAction(GraphClient* client, int32_t retryTimes = 32)
        : client_(client)
        , retryTimes_(retryTimes) {}

    virtual ~MetaAction() = default;

    ResultCode doRun() override;

    virtual std::string command() const = 0;

    std::string toString() const override {
        return command();
    }

    virtual ResultCode checkResp(const ExecutionResponse& resp) const;

protected:
    GraphClient* client_ = nullptr;
    int32_t      retryTimes_ = 0;
};

class CreateSpaceAction : public MetaAction {
public:
    CreateSpaceAction(GraphClient* client,
                      const std::string& spaceName = "test",
                      int32_t replica = 3,
                      int32_t parts = 100)
        : MetaAction(client)
        , spaceName_(spaceName)
        , replica_(replica)
        , parts_(parts) {}

    ~CreateSpaceAction() = default;

    std::string command() const override {
        return folly::stringPrintf(
                       "CREATE SPACE IF NOT EXISTS %s (replica_factor=%d, partition_num=%d)",
                       spaceName_.c_str(),
                       replica_,
                       parts_);
    }

protected:
    std::string spaceName_;
    int32_t replica_;
    int32_t parts_;
};

class UseSpaceAction : public MetaAction {
public:
    UseSpaceAction(GraphClient* client, const std::string& spaceName)
        : MetaAction(client)
        , spaceName_(spaceName) {}

    std::string command() const override {
        return folly::stringPrintf("USE %s", spaceName_.c_str());
    }

private:
    std::string spaceName_;
};

using NameType = std::pair<std::string, std::string>;
class CreateSchemaAction : public MetaAction {
public:
    /**
     * Create edge or tag with props
     * create edge if edgeOrTag is true, otherwise create tag.
     * */
    CreateSchemaAction(GraphClient* client,
                       const std::string& name,
                       const std::vector<NameType>& props,
                       bool edgeOrTag)
        : MetaAction(client)
        , name_(name)
        , props_(props)
        , edgeOrTag_(edgeOrTag) {}

    ~CreateSchemaAction() = default;

    std::string command() const override {
        std::string str;
        str.reserve(256);
        str += "CREATE ";
        str += edgeOrTag_ ? "EDGE " : "TAG ";
        str += "IF NOT EXISTS ";
        str += name_;
        if (props_.empty()) {
            return str;
        }
        str += "(";
        for (auto& prop : props_) {
            str += prop.first;
            str += " ";
            str += prop.second;
            str += ",";
        }
        str[str.size() - 1] = ')';
        return str;
    }

protected:
    std::string name_;
    std::vector<NameType> props_;
    bool edgeOrTag_;
};

class BalanceLeaderAction : public MetaAction {
public:
    /**
     * Balance leader
     * This action must be run after UseSpaceAction
     */
    explicit BalanceLeaderAction(GraphClient* client)
        : MetaAction(client) {}

    std::string command() const override {
        return "balance leader";
    }
};

class BalanceDataAction : public MetaAction {
public:
    /**
     * Balance data
     * This action must be run after UseSpaceAction
     */
    BalanceDataAction(GraphClient* client, int32_t retry)
        : MetaAction(client, retry) {}

    ResultCode checkResp(const ExecutionResponse& resp) const override;

    std::string command() const override {
        return "balance data";
    }
};

class DescSpaceAction : public MetaAction {
public:
    /**
     * Get space info
     */
    DescSpaceAction(GraphClient* client, const std::string& spaceName)
        : MetaAction(client)
        , spaceName_(spaceName) {}

    ResultCode checkResp(const ExecutionResponse& resp) const override;

    std::string command() const override {
        return folly::stringPrintf("desc space %s", spaceName_.c_str());
    }

    int64_t spaceId() {
        return spaceId_;
    }

private:
    std::string spaceName_;
    mutable int64_t spaceId_ = -1;
};

class CheckLeadersAction : public MetaAction {
public:
    CheckLeadersAction(GraphClient* client,
                       core::ActionContext* ctx,
                       int32_t expectedNum,
                       const std::string& spaceName,
                       const std::string& resultVarName)
        : MetaAction(client)
        , ctx_(ctx)
        , expectedNum_(expectedNum)
        , spaceName_(spaceName)
        , resultVarName_(resultVarName) {}

    ~CheckLeadersAction() = default;

    ResultCode doRun() override;

    std::string command() const override {
        return "show hosts";
    }

    ResultCode checkResp(const ExecutionResponse& resp) const override;

    ResultCode checkLeaderDis(const ExecutionResponse& resp);

private:
    core::ActionContext*    ctx_{nullptr};
    int32_t                 expectedNum_;
    std::string             spaceName_;

    // If resultVarName_ is empty, not check leader distribution
    std::string             resultVarName_;
    std::string             restult_;
};

class UpdateConfigsAction : public MetaAction {
public:
    UpdateConfigsAction(GraphClient* client,
                        const std::string& layer,
                        const std::string& name,
                        const std::string& value)
        : MetaAction(client)
        , layer_(layer)
        , name_(name)
        , value_(value) {}

    ~UpdateConfigsAction() = default;

    ResultCode doRun() override;

    ResultCode buildCmd();

    std::string command() const override {
        return cmd_;
    }

    std::string toString() const override {
        return "update configs";
    }

private:
    std::string             layer_;
    std::string             name_;
    std::string             value_;
    std::string             cmd_;
};

class CompactionAction : public MetaAction {
public:
    CompactionAction(GraphClient* client)
        : MetaAction(client){}

    ~CompactionAction() = default;

    ResultCode checkResp(const ExecutionResponse& resp) const override;

    std::string command() const override {
        return "submit job compact";
    }
};

class ExecutionExpressionAction : public core::Action {
public:
    ExecutionExpressionAction(core::ActionContext* ctx,
                              const std::string& condition)
        : Action(ctx)
        , condition_(condition) {}

    ~ExecutionExpressionAction() = default;

    ResultCode doRun() override;

    std::string toString() const override {
        return folly::stringPrintf("Execution expression %s",
                                   condition_.c_str());
    }

private:
    std::string             condition_;
};

/**
 * Random kill the instance and restart it.
 * We could set the loop times.
 * */
class RandomRestartAction : public core::DisturbAction {
public:
    RandomRestartAction(const std::vector<NebulaInstance*>& instances,
                        int32_t loopTimes,
                        int32_t timeToDisurb,
                        int32_t timeToRecover,
                        bool graceful = false,
                        bool cleanData = false)
        : DisturbAction(loopTimes, timeToDisurb, timeToRecover)
        , instances_(instances)
        , graceful_(graceful)
        , cleanData_(cleanData) {}

    ~RandomRestartAction() = default;

    std::string toString() const override {
        return folly::stringPrintf("Random kill: loop %d", loopTimes_);
    }

private:
    ResultCode disturb() override;
    ResultCode recover() override;
    ResultCode start(NebulaInstance* inst);
    ResultCode stop(NebulaInstance* inst);

private:
    std::vector<NebulaInstance*> instances_;
    NebulaInstance* picked_;
    bool graceful_;
    bool cleanData_;
};

// Clean wal of specified space
class CleanWalAction : public core::Action {
public:
    CleanWalAction(NebulaInstance* inst, GraphClient* client, const std::string& spaceName)
        : inst_(inst)
        , client_(client)
        , spaceName_(spaceName) {}

    ~CleanWalAction() = default;

    ResultCode doRun() override;

    std::string toString() const override {
        return folly::stringPrintf("Clean space %s wal on instance %s",
                                   spaceName_.c_str(),
                                   inst_->toString().c_str());
    }

private:
    NebulaInstance* inst_;
    GraphClient* client_;
    std::string spaceName_;
};

// If space name is not empty, only clean data path of specified space,
// otherwise clean the whole data path
class CleanDataAction : public core::Action {
public:
    CleanDataAction(NebulaInstance* inst,
                    GraphClient* client = nullptr,
                    const std::string& spaceName = "")
        : inst_(inst)
        , client_(client)
        , spaceName_(spaceName) {}

    ~CleanDataAction() = default;

    ResultCode doRun() override;

    std::string toString() const override {
        CHECK_NOTNULL(inst_);
        auto hostAddr = inst_->toString();
        return folly::stringPrintf("clean data %s ", hostAddr.c_str());
    }

private:
    NebulaInstance* inst_ = nullptr;
    GraphClient* client_;
    std::string spaceName_;
};

/**
 * Random network partition one storagge instance from the other storage instances
 * and meta instances using iptables.
 * */
class RandomPartitionAction : public core::DisturbAction {
public:
    RandomPartitionAction(const std::vector<NebulaInstance*>& metas,
                          const std::vector<NebulaInstance*>& storages,
                          int32_t loopTimes,
                          int32_t timeToDisurb,
                          int32_t timeToRecover)
        : DisturbAction(loopTimes, timeToDisurb, timeToRecover)
        , metas_(metas)
        , storages_(storages) {}

    ~RandomPartitionAction() = default;

    std::string toString() const override {
        return folly::stringPrintf("Random partition: loop %d", loopTimes_);
    }

private:
    ResultCode disturb() override;
    ResultCode recover() override;

private:
    std::vector<NebulaInstance*> metas_;
    std::vector<NebulaInstance*> storages_;
    NebulaInstance* picked_;
    std::vector<std::string> paras_;
};

/**
 * Random traffic control one storagge instance from the other storage instances using tcconfig.
 * tcconfig is a wrapper of linux tc.
 * */
class RandomTrafficControlAction : public core::DisturbAction {
public:
    RandomTrafficControlAction(const std::vector<NebulaInstance*>& storages,
                               int32_t loopTimes,
                               int32_t timeToDisurb,
                               int32_t timeToRecover,
                               const std::string& device,
                               const std::string& delay,
                               const std::string& dist,
                               int32_t loss,
                               int32_t duplicate)
        : DisturbAction(loopTimes, timeToDisurb, timeToRecover)
        , storages_(storages)
        , device_(device)
        , delay_(delay)
        , dist_(dist)
        , loss_(loss)
        , duplicate_(duplicate) {}

    ~RandomTrafficControlAction() = default;

    std::string toString() const override {
        return folly::stringPrintf("Random traffic control: loop %d delay %s +/- %s",
                                   loopTimes_, delay_.c_str(), dist_.c_str());
    }

private:
    ResultCode disturb() override;
    ResultCode recover() override;

private:
    std::vector<NebulaInstance*> storages_;
    std::string device_;
    std::string delay_;
    std::string dist_;
    int32_t loss_;
    int32_t duplicate_;
    NebulaInstance* picked_;
    std::vector<std::string> paras_;
};

class FillDiskAction : public core::DisturbAction {
public:
    FillDiskAction(const std::vector<NebulaInstance*>& storages,
                   int32_t loopTimes,
                   int32_t timeToDisurb,
                   int32_t timeToRecover,
                   int32_t count)
        : DisturbAction(loopTimes, timeToDisurb, timeToRecover)
        , storages_(storages)
        , count_(count) {}

    ~FillDiskAction() = default;

    std::string toString() const override {
        return folly::stringPrintf("Fill disk: loop %d", loopTimes_);
    }

private:
    ResultCode disturb() override;
    ResultCode recover() override;

    ResultCode reboot(NebulaInstance* inst);

private:
    std::vector<NebulaInstance*> storages_;
    int32_t count_;
};

class SlowDiskAction : public core::DisturbAction {
public:
    SlowDiskAction(const std::vector<NebulaInstance*>& storages,
                   int32_t loopTimes,
                   int32_t timeToDisurb,
                   int32_t timeToRecover,
                   int32_t major,
                   int32_t minor,
                   int32_t delayMs)
        : DisturbAction(loopTimes, timeToDisurb, timeToRecover)
        , storages_(storages)
        , major_(major)
        , minor_(minor)
        , delayMs_(delayMs) {}

    ~SlowDiskAction() = default;

    std::string toString() const override {
        return folly::stringPrintf("Slow disk: loop %d", loopTimes_);
    }

private:
    ResultCode disturb() override;
    ResultCode recover() override;

private:
    std::vector<NebulaInstance*> storages_;
    int32_t major_;
    int32_t minor_;
    int32_t delayMs_;

    NebulaInstance* picked_;
    folly::Optional<int32_t> stapPid_;
};

class CreateCheckpointAction : public MetaAction {
public:
    CreateCheckpointAction(GraphClient* client)
        : MetaAction(client) {}

    ~CreateCheckpointAction() = default;

    std::string command() const override {
        return "CREATE SNAPSHOT";
    }
};

// Clean all snapshot
class CleanCheckpointAction : public core::Action {
public:
    CleanCheckpointAction(NebulaInstance* inst)
        : inst_(inst) {}

    ~CleanCheckpointAction() = default;

    ResultCode doRun() override;

    std::string toString() const override {
        return folly::stringPrintf("Clean snapshot on instance %s",
                                   inst_->toString().c_str());
    }

private:
    NebulaInstance* inst_;
};

// Clean all snapshot
class RestoreFromCheckpointAction : public core::Action {
public:
    RestoreFromCheckpointAction(NebulaInstance* inst)
        : inst_(inst) {}

    ~RestoreFromCheckpointAction() = default;

    ResultCode doRun() override;

    std::string toString() const override {
        return folly::stringPrintf("Restore db from snapshot on instance %s",
                                   inst_->toString().c_str());
    }

private:
    NebulaInstance* inst_;
};

class RestoreFromDataDirAction : public core::Action {
public:
    RestoreFromDataDirAction(NebulaInstance* inst, const std::string& srcDataPaths)
        : inst_(inst)
        , srcDataPaths_(srcDataPaths) {}

    ~RestoreFromDataDirAction() = default;

    ResultCode doRun() override;

    std::string toString() const override {
        return folly::stringPrintf("Restore db from data folder on instance %s",
                                   inst_->toString().c_str());
    }

private:
    NebulaInstance* inst_;
    std::string     srcDataPaths_;
};

class TruncateWalAction : public core::Action {
public:
    TruncateWalAction(const std::vector<NebulaInstance*>& storages,
                      GraphClient* client,
                      const std::string& spaceName,
                      int32_t partId,
                      int32_t count,
                      int32_t bytes)
        : storages_(storages)
        , client_(client)
        , spaceName_(spaceName)
        , partId_(partId)
        , count_(count)
        , bytes_(bytes) {}

    ~TruncateWalAction() = default;

    ResultCode doRun() override;

    std::string toString() const override {
        return folly::stringPrintf("Truncate space %s wal", spaceName_.c_str());
    }

private:
    std::vector<NebulaInstance*> storages_;
    GraphClient* client_;
    std::string spaceName_;
    int32_t partId_;
    int32_t count_;                     // how many storage to truncate
    int32_t bytes_;                     // how many bytes to truncate wal
};

}   // namespace nebula
}   // namespace nebula_chaos

#endif  // NEBULA_CRASHACTION_H_
