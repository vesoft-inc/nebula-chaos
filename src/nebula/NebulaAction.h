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
        auto code = client_->connect("user", "password");
        if (Code::SUCCEEDED == code) {
            return ResultCode::OK;
        }
        return ResultCode::ERR_FAILED;
    }

    std::string toString() const override {
        return folly::stringPrintf("Connect to %s", client_->serverAddress().c_str());
    }

private:
    GraphClient* client_ = nullptr;
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

}   // namespace nebula
}   // namespace nebula_chaos

#endif  // NEBULA_CRASHACTION_H_
