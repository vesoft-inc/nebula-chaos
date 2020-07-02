/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#include "nebula/NebulaAction.h"
#include "utils/SshHelper.h"
#include "core/CheckProcAction.h"
#include <folly/Random.h>

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
    auto pid = inst_->getPid();
    if (!pid.hasValue()) {
        return ResultCode::ERR_FAILED;
    }
    // Check the start action succeeded or not
    nebula_chaos::core::CheckProcAction action(inst_->getHost(), pid.value(), inst_->owner());
    return action.doRun();
}

ResultCode StopAction::doRun() {
    CHECK_NOTNULL(inst_);
    auto stopCommand = inst_->stopCommand();
    LOG(INFO) << stopCommand << " on " << inst_->toString() << " as " << inst_->owner();

    int32_t tryTimes = 0;
    while (++tryTimes < 10) {
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
        auto pid = inst_->getPid();
        if (!pid.hasValue()) {
            return ResultCode::ERR_FAILED;
        }
        // Check the stop action succeeded or not
        nebula_chaos::core::CheckProcAction action(inst_->getHost(), pid.value(), inst_->owner());
        auto res = action.doRun();
        if (res == ResultCode::OK) {
            sleep(tryTimes);
            LOG(INFO) << "Wait some time, and try again, try times " << tryTimes;
        } else if (res == ResultCode::ERR_NOT_FOUND) {
            return ResultCode::OK;
        }
    }
    return ResultCode::ERR_FAILED;
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

ResultCode WriteCircleAction::sendBatch(const std::vector<std::string>& batchCmds) {
    auto joinStr = folly::join(",", batchCmds);
    auto cmd = folly::stringPrintf("INSERT VERTEX %s (%s) VALUES %s",
                                   tag_.c_str(),
                                   col_.c_str(),
                                   joinStr.c_str());
    VLOG(1) << cmd;
    ExecutionResponse resp;
    uint32_t tryTimes = 0;
    while (tryTimes++ < try_) {
        auto res = client_->execute(cmd, resp);
        if (res == Code::SUCCEEDED) {
            return ResultCode::OK;
        }
        usleep(1000 * 5 * tryTimes);
        LOG(WARNING) << "Failed to send request, tryTimes " << tryTimes
                     << ", error code " << static_cast<int32_t>(res);
    }
    return ResultCode::ERR_FAILED;
}

ResultCode WriteCircleAction::doRun() {
    CHECK_NOTNULL(client_);
    auto insertCmd = folly::stringPrintf("INSERT VERTEX %s (%s) VALUES ",
                                         tag_.c_str(),
                                         col_.c_str());
    std::vector<std::string> batchCmds;
    batchCmds.reserve(1024);
    uint64_t row = 1;
    while (row < totalRows_) {
        if (batchCmds.size() == batchNum_) {
            auto res = sendBatch(batchCmds);
            if (res != ResultCode::OK) {
                LOG(ERROR) << "Send request failed!";
                return res;
            }
            LOG(INFO) << "Send requests successfully, start row " << row
                      << ", batch " << batchNum_;
            batchCmds.clear();
        }
        batchCmds.emplace_back(folly::stringPrintf("%ld:(%ld)", row, row + 1));
        row++;
    }
    batchCmds.emplace_back(folly::stringPrintf("%ld:(%ld)", row, 1L));
    return sendBatch(batchCmds);
}

folly::Expected<uint64_t, ResultCode>
WalkThroughAction::sendCommand(const std::string& cmd) {
    VLOG(1) << cmd;
    ExecutionResponse resp;
    uint32_t tryTimes = 0;
    while (tryTimes++ < try_) {
        auto res = client_->execute(cmd, resp);
        if (res == Code::SUCCEEDED) {
            if (resp.rows.empty() || resp.rows[0].columns.empty()) {
                LOG(WARNING) << "Bad result, resp.rows size " << resp.rows.size();
                break;
            }
            VLOG(1) << resp.rows.size() << ", " << resp.rows[0].columns.size();
            return resp.rows[0].columns[1].get_integer();
        }
        usleep(1000 * 5 * tryTimes);
        LOG(WARNING) << "Failed to send request, tryTimes " << tryTimes
                     << ", error code " << static_cast<int32_t>(res);
    }
    return folly::makeUnexpected(ResultCode::ERR_FAILED);
}

ResultCode WalkThroughAction::doRun() {
    CHECK_NOTNULL(client_);
    start_ = folly::Random::rand64(totalRows_);
    auto id = start_;
    uint64_t count = 0;
    while (++count <= totalRows_) {
        auto cmd = folly::stringPrintf("FETCH PROP ON %s %ld YIELD %s.%s",
                                       tag_.c_str(),
                                       id,
                                       tag_.c_str(),
                                       col_.c_str());
        auto res = sendCommand(cmd);
        if (bool(res)) {
            id = res.value();
        } else {
            LOG(ERROR) << "Send command failed!";
            return ResultCode::ERR_FAILED;
        }
        if (id == start_) {
            LOG(INFO) << "We are back to " << start_
                      << ", total count " << count;
            break;
        }
    }
    if (id != start_) {
        LOG(ERROR) << "Wrong value, id = " << id << ", start = " << start_;
        return ResultCode::ERR_FAILED;
    }
    return count == totalRows_ ? ResultCode::OK : ResultCode::ERR_FAILED;
}

}   // namespace nebula
}   // namespace nebula_chaos
