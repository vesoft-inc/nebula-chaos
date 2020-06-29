/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#include "nebula/NebulaAction.h"
#include "utils/SshHelper.h"

namespace nebula_chaos {
namespace nebula {

ResultCode CrashAction::doRun() {
    CHECK_NOTNULL(inst_);
    auto killCommand = inst_->killCommand();
    LOG(INFO) << killCommand << " on " << inst_->toString() << " as " << inst_->owner();
    auto ret = utils::SshHelper::run(
                killCommand,
                inst_->getHost(),
                [this] (const std::string& outMsg) {
                    VLOG(1) << "The output is " << outMsg;
                },
                [] (const std::string& errMsg) {
                    LOG(ERROR) << "The error is " << errMsg;
                },
                inst_->owner());
    CHECK_EQ(0, ret.exitStatus());
    return ResultCode::OK;
}

ResultCode StartAction::doRun() {
    CHECK_NOTNULL(inst_);
    auto startCommand = inst_->startCommand();
    LOG(INFO) << startCommand << " on " << inst_->toString() << " as " << inst_->owner();
    auto ret = utils::SshHelper::run(
                startCommand,
                inst_->getHost(),
                [this] (const std::string& outMsg) {
                    VLOG(1) << "The output is " << outMsg;
                },
                [] (const std::string& errMsg) {
                    LOG(ERROR) << "The error is " << errMsg;
                },
                inst_->owner());
    CHECK_EQ(0, ret.exitStatus());
    return ResultCode::OK;
}

ResultCode StopAction::doRun() {
    CHECK_NOTNULL(inst_);
    auto stopCommand = inst_->stopCommand();
    LOG(INFO) << stopCommand << " on " << inst_->toString() << " as " << inst_->owner();
    auto ret = utils::SshHelper::run(
                stopCommand,
                inst_->getHost(),
                [this] (const std::string& outMsg) {
                    VLOG(1) << "The output is " << outMsg;
                },
                [] (const std::string& errMsg) {
                    LOG(ERROR) << "The error is " << errMsg;
                },
                inst_->owner());
    CHECK_EQ(0, ret.exitStatus());
    return ResultCode::OK;
}

ResultCode MetaAction::doRun() {
    CHECK_NOTNULL(client_);
    auto cmd = command();
    LOG(INFO) << "Send " << cmd << " to graphd";

    ExecutionResponse resp;
    auto res = client_->execute(cmd, resp);
    if (res == Code::SUCCEEDED) {
        LOG(INFO) << "Execute " << cmd << " successfully!";
        return ResultCode::OK;
    } else {
        LOG(ERROR) << "Execute " << cmd << " failed!";
        return ResultCode::ERR_FAILED;
    }
}

}   // namespace nebula
}   // namespace nebula_chaos
