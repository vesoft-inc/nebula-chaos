/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef NEBULA_CRASHACTION_H_
#define NEBULA_CRASHACTION_H_

#include "actions/Action.h"
#include "nebula/NebulaInstance.h"

namespace nebula_chaos {
namespace nebula {

class CrashAction : public action::Action {
public:
    CrashAction(NebulaInstance* inst)
        : inst_(inst) {}

    ~CrashAction() = default;

    void run() override;

    std::string toString() override {
        CHECK_NOTNULL(inst_);
        auto hostAddr = inst_->toString();
        return folly::stringPrintf("kill -9 %s", hostAddr.c_str());
    }

private:
    NebulaInstance* inst_ = nullptr;
};

class StartAction : public action::Action {
public:
    StartAction(NebulaInstance* inst)
        : inst_(inst) {}

    ~StartAction() = default;

    void run() override;

    std::string toString() override {
        CHECK_NOTNULL(inst_);
        auto hostAddr = inst_->toString();
        return folly::stringPrintf("start %s", hostAddr.c_str());
    }

private:
    NebulaInstance* inst_ = nullptr;
};

class StopAction : public action::Action {
public:
    StopAction(NebulaInstance* inst)
        : inst_(inst) {}

    ~StopAction() = default;

    void run() override;

    std::string toString() override {
        CHECK_NOTNULL(inst_);
        auto hostAddr = inst_->toString();
        return folly::stringPrintf("stop %s", hostAddr.c_str());
    }

private:
    NebulaInstance* inst_ = nullptr;
};

}   // namespace nebula
}   // namespace nebula_chaos

#endif  // NEBULA_CRASHACTION_H_
