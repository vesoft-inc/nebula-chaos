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
                      uint64_t totalRows = 10000,
                      uint32_t batchNum = 1,
                      uint32_t tryNum = 32,
                      uint32_t retryIntervalMs = 1)
        : client_(client)
        , tag_(tag)
        , col_(col)
        , totalRows_(totalRows)
        , batchNum_(batchNum)
        , try_(tryNum)
        , retryIntervalMs_(retryIntervalMs) {}

    virtual ~WriteCircleAction() = default;

    ResultCode doRun() override;

    std::string toString() const override {
        return folly::stringPrintf("Write data to %s", client_->serverAddress().c_str());
    }

private:
   ResultCode sendBatch(const std::vector<std::string>& batchCmds);

private:
    GraphClient* client_ = nullptr;
    std::string tag_;
    std::string col_;
    uint64_t    totalRows_;
    uint32_t    batchNum_;
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
    MetaAction(GraphClient* client)
        : client_(client) {}

    virtual ~MetaAction() = default;

    ResultCode doRun() override;

    virtual std::string command() const = 0;

    std::string toString() const override {
        return command();
    }

    virtual ResultCode checkResp(const ExecutionResponse& resp) const;

protected:
    GraphClient* client_ = nullptr;
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

class BalanceAction : public MetaAction {
public:
    /**
     * Balance data if dataOrLeader is true, otherwide balance leader
     * This action must be run after UseSpaceAction
     */
    BalanceAction(GraphClient* client, bool dataOrLeader)
        : MetaAction(client)
        , dataOrLeader_(dataOrLeader) {}

    std::string command() const override {
        return dataOrLeader_ ? "balance data" : "balance leader";
    }

private:
    bool dataOrLeader_;
};

class CheckLeadersAction : public MetaAction {
public:
    CheckLeadersAction(GraphClient* client, int32_t expectedNum)
        : MetaAction(client)
        , expectedNum_(expectedNum) {}

    std::string command() const override {
        return "show hosts";
    }

    ResultCode checkResp(const ExecutionResponse& resp) const override;

private:
    int32_t expectedNum_;
};

/**
 * Random kill the instance and restart it.
 * We could set the loop times.
 * */
class RandomRestartAction : public core::Action {
public:
    RandomRestartAction(const std::vector<NebulaInstance*>& instances,
                        int32_t loopTimes,
                        int32_t restartInterval,
                        int32_t nextLoopInterval,
                        bool graceful = false,
                        bool cleanData = false)
        : instances_(instances)
        , loopTimes_(loopTimes)
        , restartInterval_(restartInterval)
        , nextLoopInterval_(nextLoopInterval)
        , graceful_(graceful)
        , cleanData_(cleanData) {}

    ~RandomRestartAction() = default;

    ResultCode doRun() override;

    std::string toString() const override {
        return folly::stringPrintf("Random kill: loop %d", loopTimes_);
    }

private:
    ResultCode start(NebulaInstance* inst);

    ResultCode stop(NebulaInstance* inst);

private:
    std::vector<NebulaInstance*> instances_;
    int32_t loopTimes_;
    int32_t restartInterval_;  // seconds
    int32_t nextLoopInterval_;  // seconds
    bool graceful_;
    bool cleanData_;
};

// Clean wal of specified space
class CleanWalAction : public core::Action {
public:
    CleanWalAction(NebulaInstance* inst, int64_t spaceId)
        : inst_(inst)
        , spaceId_(spaceId) {}

    ~CleanWalAction() = default;

    ResultCode doRun() override;

    std::string toString() const override {
        return folly::stringPrintf("Clean space %ld wal on instance %s",
                                   spaceId_,
                                   inst_->toString().c_str());
    }

private:
    NebulaInstance* inst_;
    int64_t spaceId_;
};

// Clean all data path
class CleanDataAction : public core::Action {
public:
    CleanDataAction(NebulaInstance* inst)
        : inst_(inst) {}

    ~CleanDataAction() = default;

    ResultCode doRun() override;

    std::string toString() const override {
        CHECK_NOTNULL(inst_);
        auto hostAddr = inst_->toString();
        return folly::stringPrintf("clean data %s ", hostAddr.c_str());
    }

private:
    NebulaInstance* inst_ = nullptr;
};

}   // namespace nebula
}   // namespace nebula_chaos

#endif  // NEBULA_CRASHACTION_H_
