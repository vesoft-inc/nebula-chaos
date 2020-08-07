/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#include "nebula/NebulaAction.h"
#include "nebula/NebulaUtils.h"
#include "utils/SshHelper.h"
#include "core/CheckProcAction.h"
#include <folly/Random.h>
#include <folly/GLog.h>

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
    auto pid = inst_->getPid();
    if (!pid.hasValue()) {
        return ResultCode::ERR_FAILED;
    }
    nebula_chaos::core::CheckProcAction action(inst_->getHost(), pid.value(), inst_->owner());
    auto res = action.doRun();
    if (res == ResultCode::ERR_NOT_FOUND) {
        inst_->setState(NebulaInstance::State::STOPPED);
        return ResultCode::OK;
    } else {
        return ResultCode::ERR_FAILED;
    }
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
    auto res = action.doRun();
    if (res == ResultCode::OK) {
        inst_->setState(NebulaInstance::State::RUNNING);
    }
    return res;
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
            inst_->setState(NebulaInstance::State::STOPPED);
            return ResultCode::OK;
        }
    }
    return ResultCode::ERR_FAILED;
}

ResultCode CleanDataAction::doRun() {
    CHECK_NOTNULL(inst_);
    if (inst_->getState() == NebulaInstance::State::RUNNING) {
        LOG(ERROR) << "Can't clean data while instance " << inst_->toString()
                   << " is stil running";
        return ResultCode::OK;
    }
    auto dataPaths = inst_->dataDirs();
    if (!dataPaths.hasValue()) {
        LOG(ERROR) << "Can't find data path on " << inst_->toString();
        return ResultCode::ERR_FAILED;
    }
    std::string cleanCommand = "rm -rf ";
    if (!spaceName_.empty()) {
        DescSpaceAction desc(client_, spaceName_);
        auto rc = desc.doRun();
        if (rc != ResultCode::OK) {
            LOG(ERROR) << "Failed to desc space " << spaceName_;
        }
        auto spaceId = desc.spaceId();
        for (const auto& dataPath : dataPaths.value()) {
            cleanCommand.append(folly::stringPrintf("%s/nebula/%ld ", dataPath.c_str(), spaceId));
        }
    } else {
        for (const auto& dataPath : dataPaths.value()) {
            cleanCommand.append(folly::stringPrintf("%s ", dataPath.c_str()));
        }
    }
    LOG(INFO) << cleanCommand << " on " << inst_->toString() << " as " << inst_->owner();
    auto ret = utils::SshHelper::run(
                cleanCommand,
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

ResultCode MetaAction::checkResp(const ExecutionResponse&) const {
    return ResultCode::OK;
}

ResultCode MetaAction::doRun() {
    CHECK_NOTNULL(client_);
    auto cmd = command();
    LOG(INFO) << "Send " << cmd << " to graphd";
    ExecutionResponse resp;
    int32_t retry = 0;
    while (++retry < retryTimes_) {
        client_->execute(cmd, resp);
        LOG(INFO) << "Execute " << cmd << " successfully!";
        auto ret = checkResp(resp);
        if (ret == ResultCode::OK
                || ret == ResultCode::ERR_FAILED_NO_RETRY) {
            return ret;
        }
        LOG(ERROR) << "Execute " << cmd << " failed, retry " << retry
                   << " after " << retry << " seconds...";
        sleep(retry);
    }
    return ResultCode::ERR_FAILED;
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
    uint32_t retryInterval = retryIntervalMs_;
    while (++tryTimes < try_) {
        auto res = client_->execute(cmd, resp);
        if (res == Code::SUCCEEDED) {
            return ResultCode::OK;
        }
        usleep(retryInterval * 1000 * tryTimes);
        LOG(WARNING) << "Failed to send request, tryTimes " << tryTimes
                     << ", error code " << static_cast<int32_t>(res);
    }
    return ResultCode::ERR_FAILED;
}

ResultCode WriteCircleAction::doRun() {
    CHECK_NOTNULL(client_);
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
            FB_LOG_EVERY_MS(INFO, 3000) << "Send requests successfully, row "
                                        << row;
            batchCmds.clear();
        }
        batchCmds.emplace_back(folly::stringPrintf("%ld:(%ld)", row, row + 1));
        row++;
    }
    batchCmds.emplace_back(folly::stringPrintf("%ld:(%ld)", row, 1L));
    auto res = sendBatch(batchCmds);
    LOG(INFO) << "Send all requests successfully, row " << row;
    return res;
}

folly::Expected<uint64_t, ResultCode>
WalkThroughAction::sendCommand(const std::string& cmd) {
    VLOG(1) << cmd;
    ExecutionResponse resp;
    uint32_t tryTimes = 0;
    uint32_t retryInterval = retryIntervalMs_;
    while (++tryTimes < try_) {
        auto res = client_->execute(cmd, resp);
        if (res == Code::SUCCEEDED) {
            if (resp.rows.empty() || resp.rows[0].columns.empty()) {
                LOG(WARNING) << "Bad result, resp.rows size " << resp.rows.size();
                break;
            }
            VLOG(1) << resp.rows.size() << ", " << resp.rows[0].columns.size();
            return resp.rows[0].columns[1].get_integer();
        }
        usleep(retryInterval * 1000 * tryTimes);
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
        FB_LOG_EVERY_MS(INFO, 3000) << cmd;
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

ResultCode BalanceDataAction::checkResp(const ExecutionResponse& resp) const {
    auto* msg = resp.get_error_msg();
    if (msg != nullptr && *msg == "The cluster is balanced!") {
        return ResultCode::OK;
    }
    return ResultCode::ERR_NOT_FINISHED;
}

ResultCode DescSpaceAction::checkResp(const ExecutionResponse& resp) const {
    if (resp.rows.empty()) {
        LOG(ERROR) << "Result should not be empty!";
        return ResultCode::ERR_FAILED;
    }
    if (resp.rows.size() != 1) {
        LOG(ERROR) << "Row number is wrong!";
        return ResultCode::ERR_FAILED;
    }
    auto& row = resp.rows[0];
    if (row.columns.size() != 6) {
        LOG(ERROR) << "Column number is wrong!";
        return ResultCode::ERR_FAILED;
    }
    if (row.columns[1].get_str() != spaceName_) {
        LOG(ERROR) << "Desc a wrong space! Should be " << spaceName_;
        return ResultCode::ERR_FAILED;
    }
    spaceId_ = row.columns[0].get_integer();
    return ResultCode::OK;
}

ResultCode CheckLeadersAction::checkResp(const ExecutionResponse& resp) const {
    if (resp.rows.empty()) {
        LOG(ERROR) << "Result should not be empty!";
        return ResultCode::ERR_FAILED;
    }
    auto& row = resp.rows[resp.rows.size() - 1];
    if (row.columns.size() != 6) {
        LOG(ERROR) << "Column number is wrong!";
        return ResultCode::ERR_FAILED;
    }
    if (row.columns[0].get_str() != "Total") {
        LOG(ERROR) << "Bad format for the response!";
        return ResultCode::ERR_FAILED;
    }
    auto& leaderStr = row.columns[4].get_str();
    if (leaderStr.empty()) {
        return ResultCode::ERR_FAILED;
    }
    try {
        std::vector<folly::StringPiece> spaceLeaders;
        folly::split(",", leaderStr, spaceLeaders);
        for (auto& sl : spaceLeaders) {
            std::vector<folly::StringPiece> oneSpaceLeaderPair;
            folly::split(":", sl, oneSpaceLeaderPair);
            CHECK_EQ(2, oneSpaceLeaderPair.size());
            auto spaceName = folly::trimWhitespace(oneSpaceLeaderPair[0]);
            auto leaderNum = folly::to<int32_t>(folly::trimWhitespace(oneSpaceLeaderPair[1]));
            if (spaceName == spaceName_ && leaderNum == expectedNum_) {
                return ResultCode::OK;
            }
        }
    } catch (const std::exception& e) {
        LOG(ERROR) << "col4 is " <<  leaderStr << ", exception: " << e.what();
    }
    return ResultCode::ERR_FAILED;
}

ResultCode RandomRestartAction::stop(NebulaInstance* inst) {
    if (graceful_) {
        StopAction stop(inst);
        return stop.doRun();
    } else {
        CrashAction crash(inst);
        return crash.doRun();
    }
}

ResultCode RandomRestartAction::start(NebulaInstance* inst) {
    for (int retry = 0; retry != 32; retry++) {
        StartAction start(inst);
        auto ret = start.doRun();
        if (ret == ResultCode::OK) {
            return ret;
        }
        LOG(ERROR) << "Start failed, retry " << retry;
        sleep(retry);
    }
    return ResultCode::ERR_FAILED;
}

ResultCode RandomRestartAction::disturb() {
    picked_ = Utils::randomInstance(instances_, NebulaInstance::State::RUNNING);
    CHECK_NOTNULL(picked_);
    {
        LOG(INFO) << "Begin to kill " << picked_->toString() << ", graceful " << graceful_;
        auto rc = stop(picked_);
        if (rc != ResultCode::OK) {
            LOG(ERROR) << "Stop instance " << picked_->toString() << " failed!";
            return rc;
        }
        LOG(INFO) << "Finish to kill " << picked_->toString();
    }

    if (cleanData_) {
        // plan must make sure that snapshot would be sent within nextLoopInterval_,
        // clean data of all spaces
        CleanDataAction cleanData(picked_);
        auto rc = cleanData.doRun();
        if (rc != ResultCode::OK) {
            LOG(ERROR) << "Clean instance data " << picked_->toString() << " failed!";
            return rc;
        }
    }
    return ResultCode::OK;
}

ResultCode RandomRestartAction::recover() {
    CHECK_NOTNULL(picked_);
    LOG(INFO) << "Begin to start " << picked_->toString();
    auto rc = start(picked_);
    if (rc != ResultCode::OK) {
        LOG(ERROR) << "Start instance " << picked_->toString() << " failed!";
        return rc;
    }
    LOG(INFO) << "Finish to start " << picked_->toString();
    return ResultCode::OK;
}

ResultCode CleanWalAction::doRun() {
    CHECK_NOTNULL(inst_);
    DescSpaceAction desc(client_, spaceName_);
    auto rc = desc.doRun();
    if (rc != ResultCode::OK) {
        LOG(ERROR) << "Failed to desc space " << spaceName_;
    }
    auto spaceId = desc.spaceId();
    auto wals = inst_->walDirs(spaceId);
    if (!wals.hasValue()) {
        LOG(ERROR) << "Can't find wals for space " << spaceName_ << " on " << inst_->toString();
        return ResultCode::ERR_FAILED;
    }
    for (auto& walDir : wals.value()) {
        auto cmd = folly::stringPrintf("rm -rf %s", walDir.c_str());
        LOG(INFO) << cmd << " on " << inst_->toString() << " as " << inst_->owner();
        auto ret = utils::SshHelper::run(
                    cmd,
                    inst_->getHost(),
                    [this] (const std::string& outMsg) {
                        VLOG(1) << "The output is " << outMsg;
                    },
                    [] (const std::string& errMsg) {
                        LOG(ERROR) << "The error is " << errMsg;
                    },
                    inst_->owner());
        CHECK_EQ(0, ret.exitStatus());
    }
    return ResultCode::OK;
}

ResultCode RandomPartitionAction::disturb() {
    picked_ = Utils::randomInstance(storages_, NebulaInstance::State::RUNNING);
    CHECK_NOTNULL(picked_);
    auto pickedHost = picked_->getHost();
    auto pickedPort = picked_->getPort().value();
    paras_.clear();
    for (auto* storage : storages_) {
        auto host = storage->getHost();
        auto port = storage->getPort().value();
        if (host == pickedHost && port == pickedPort) {
            continue;
        }
        // forbid input packets from other storage hosts, both data port and raft port
        paras_.emplace_back(folly::stringPrintf("INPUT -p tcp -m tcp -s %s --dport %d:%d -j DROP",
                            host.c_str(), pickedPort, pickedPort + 1));
        // forbid output packets to other storage hosts, both data port and raft port
        paras_.emplace_back(folly::stringPrintf("OUTPUT -p tcp -m tcp -d %s --dport %d:%d -j DROP",
                            host.c_str(), port, port + 1));
    }
    for (auto* meta : metas_) {
        auto host = meta->getHost();
        auto port = meta->getPort().value();
        // forbid input packets from meta hosts
        paras_.emplace_back(folly::stringPrintf("INPUT -p tcp -m tcp -s %s --dport %d -j DROP",
                            host.c_str(), pickedPort));
        // forbid output packets to meta hosts
        paras_.emplace_back(folly::stringPrintf("OUTPUT -p tcp -m tcp -d %s --dport %d -j DROP",
                            host.c_str(), port));
    }
    std::string iptable;
    for (const auto& para : paras_) {
        iptable.append(folly::stringPrintf("sudo iptables -I %s; ", para.c_str()));
    }
    LOG(INFO) << "Begin network partition of " << picked_->toString();
    VLOG(1) << iptable << " on " << picked_->toString();
    auto ret = utils::SshHelper::run(
                iptable,
                picked_->getHost(),
                [this] (const std::string& outMsg) {
                    VLOG(1) << "The output is " << outMsg;
                },
                [] (const std::string& errMsg) {
                    LOG(ERROR) << "The error is " << errMsg;
                },
                picked_->owner());
    CHECK_EQ(0, ret.exitStatus());
    return ResultCode::OK;
}

ResultCode RandomPartitionAction::recover() {
    std::string iptable;
    for (const auto& para : paras_) {
        iptable.append(folly::stringPrintf("sudo iptables -D %s;", para.c_str()));
    }
    LOG(INFO) << "Recover network partition of " << picked_->toString();
    VLOG(1) << iptable << " on " << picked_->toString();
    auto ret = utils::SshHelper::run(
                iptable,
                picked_->getHost(),
                [this] (const std::string& outMsg) {
                    VLOG(1) << "The output is " << outMsg;
                },
                [] (const std::string& errMsg) {
                    LOG(ERROR) << "The error is " << errMsg;
                },
                picked_->owner());
    CHECK_EQ(0, ret.exitStatus());
    return ResultCode::OK;
}

ResultCode RandomTrafficControlAction::disturb() {
    picked_ = Utils::randomInstance(storages_, NebulaInstance::State::RUNNING);
    CHECK_NOTNULL(picked_);
    auto pickedHost = picked_->getHost();
    auto pickedPort = picked_->getPort().value();
    paras_.clear();
    for (auto* storage : storages_) {
        auto host = storage->getHost();
        auto port = storage->getPort().value();
        if (host == pickedHost && port == pickedPort) {
            continue;
        }
        // traffic control packets from other storage hosts, both data port and raft port
        paras_.emplace_back(folly::stringPrintf(
            "--direction incoming --src-network %s --dst-port %d", host.c_str(), pickedPort));
        paras_.emplace_back(folly::stringPrintf(
            "--direction incoming --src-network %s --dst-port %d", host.c_str(), pickedPort + 1));
        // traffic contrl packets to other storage hosts, both data port and raft port
        paras_.emplace_back(folly::stringPrintf(
            "--direction outgoing --dst-network %s --dst-port %d", host.c_str(), port));
        paras_.emplace_back(folly::stringPrintf(
            "--direction outgoing --dst-network %s --dst-port %d", host.c_str(), port + 1));
    }
    std::string tcset;
    for (const auto& para : paras_) {
        tcset.append(folly::stringPrintf("tcset %s %s --delay %s --delay-distro %s --loss %d --duplicate %d --add; ",
                     device_.c_str(), para.c_str(),  delay_.c_str(), dist_.c_str(), loss_, duplicate_));
    }
    LOG(INFO) << "Begin traffic control of " << picked_->toString();
    VLOG(1) << tcset << " on " << picked_->toString();
    auto ret = utils::SshHelper::run(
                tcset,
                picked_->getHost(),
                [this] (const std::string& outMsg) {
                    VLOG(1) << "The output is " << outMsg;
                },
                [] (const std::string& errMsg) {
                    LOG(ERROR) << "The error is " << errMsg;
                },
                picked_->owner());
    CHECK_EQ(0, ret.exitStatus());
    return ResultCode::OK;
}

ResultCode RandomTrafficControlAction::recover() {
    std::string tcdel;
    for (const auto& para : paras_) {
        tcdel.append(folly::stringPrintf("tcdel %s %s; ", device_.c_str(), para.c_str()));
    }
    LOG(INFO) << "Recover traffic control of " << picked_->toString();
    VLOG(1) << tcdel << " on " << picked_->toString();
    auto ret = utils::SshHelper::run(
                tcdel,
                picked_->getHost(),
                [this] (const std::string& outMsg) {
                    VLOG(1) << "The output is " << outMsg;
                },
                [] (const std::string& errMsg) {
                    LOG(ERROR) << "The error is " << errMsg;
                },
                picked_->owner());
    CHECK_EQ(0, ret.exitStatus());
    return ResultCode::OK;
}

ResultCode FillDiskAction::disturb() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(storages_.begin(), storages_.end(), gen);

    for (int32_t i = 0; i < count_; i++) {
        auto* picked = storages_[i];
        auto dirs = picked->dataDirs();
        if (!dirs.hasValue()) {
            LOG(ERROR) << "Failed to get data_path of " << picked->toString();
            return ResultCode::ERR_FAILED;
        }
        LOG(INFO) << "Begin to fill disk of " << picked->toString();
        // just use the first data path
        auto fill = folly::stringPrintf("cat /dev/zero > %s/full", dirs.value().front().c_str());
        auto ret = utils::SshHelper::run(
                    fill,
                    picked->getHost(),
                    [this] (const std::string& outMsg) {
                        VLOG(1) << "The output is " << outMsg;
                    },
                    [] (const std::string& errMsg) {
                        LOG(ERROR) << "The error is " << errMsg;
                    },
                    picked->owner());
        CHECK_EQ(1, ret.exitStatus());
    }
    return ResultCode::OK;
}

ResultCode FillDiskAction::recover() {
    /*
    1. remove the mock file
    2. all storage may crashed because of disk is full when data path is under same directory,
       so try to reboot all of them
    */
    for (int32_t i = 0; i < count_; i++) {
        auto* storage = storages_[i];
        auto dirs = storage->dataDirs();
        if (!dirs.hasValue()) {
            LOG(ERROR) << "Failed to get data_path of " << storage->toString();
            return ResultCode::ERR_FAILED;
        }
        LOG(INFO) << "Clean disk on " << storage->toString();
        auto clean = folly::stringPrintf("rm -f %s/full", dirs.value().front().c_str());
        auto rc = utils::SshHelper::run(
                    clean,
                    storage->getHost(),
                    [this] (const std::string& outMsg) {
                        VLOG(1) << "The output is " << outMsg;
                    },
                    [] (const std::string& errMsg) {
                        LOG(ERROR) << "The error is " << errMsg;
                    },
                    storage->owner());
        CHECK_EQ(0, rc.exitStatus());
    }

    for (auto* storage : storages_) {
        LOG(INFO) << "Begin to reboot " << storage->toString();
        auto rc = reboot(storage);
        if (rc != ResultCode::OK) {
            return rc;
        }
    }
    return ResultCode::OK;
}

ResultCode FillDiskAction::reboot(NebulaInstance* inst) {
    StartAction start(inst);
    for (int32_t retry = 0; retry != 32; retry++) {
        auto ret = start.doRun();
        if (ret == ResultCode::OK) {
            return ret;
        }
        LOG(ERROR) << "Reboot failed, retry " << retry;
        sleep(retry);
    }
    return ResultCode::ERR_FAILED;
}

ResultCode SlowDiskAction::disturb() {
    static std::string script = R"(
        global cnt, bytes;
        probe vfs.write.return {
            if (pid() == target() && dev == MKDEV($1, $2) && $return) {
                cnt++;
                bytes += $return;
                mdelay($3);
            }
        }
        probe begin {
            printf("Begin slow disk of %d ms\n", $3);
        }
        probe timer.s(5), end {
            printf("(%d) count = %d, bytes = %d\n", target(), cnt, bytes);
        }
    )";
    picked_ = Utils::randomInstance(storages_, NebulaInstance::State::RUNNING);
    CHECK_NOTNULL(picked_);
    auto pid = picked_->getPid();
    if (!pid.hasValue()) {
        LOG(ERROR) << "Failed to get pid of " << picked_->toString();
        return ResultCode::ERR_FAILED;
    }
    LOG(INFO) << "Begin to slow disk of " << picked_->toString();
    auto stap = folly::stringPrintf("stap -e \'%s\' -DMAXSKIPPED=1000000 -F -o /tmp/stap_log_%d -g -x %d %d %d %d",
                                    script.c_str(), pid.value(), pid.value(), major_, minor_, delayMs_);
    VLOG(1) << "SystemTap script: " << stap;
    auto ret = utils::SshHelper::run(
                stap,
                picked_->getHost(),
                [this] (const std::string& outMsg) {
                    try {
                        stapPid_ = folly::to<int32_t>(outMsg);
                        LOG(INFO) << "SystemTap has been running as daemon of pid " << stapPid_.value();
                    } catch (const folly::ConversionError& e) {
                        LOG(ERROR) << "Failed to get SystemTap pid";
                    }
                },
                [] (const std::string& errMsg) {
                    LOG(ERROR) << "The error is " << errMsg;
                },
                picked_->owner());
    CHECK_EQ(0, ret.exitStatus());
    return stapPid_.hasValue() ? ResultCode::OK : ResultCode::ERR_FAILED;
}

ResultCode SlowDiskAction::recover() {
    nebula_chaos::core::CheckProcAction action(picked_->getHost(), stapPid_.value(), picked_->owner());
    auto res = action.doRun();
    if (res == ResultCode::ERR_NOT_FOUND) {
        LOG(WARNING) << "SystemTap has quit before we kill it, please check the log";
        return ResultCode::OK;
    }

    auto kill = folly::stringPrintf("kill %d", stapPid_.value());
    LOG(INFO) << "Stop slow disk of " << picked_->toString();
    auto ret = utils::SshHelper::run(
                kill,
                picked_->getHost(),
                [this] (const std::string& outMsg) {
                    VLOG(1) << "The output is " << outMsg;
                },
                [] (const std::string& errMsg) {
                    LOG(ERROR) << "The error is " << errMsg;
                },
                picked_->owner());
    CHECK_EQ(0, ret.exitStatus());
    stapPid_.clear();
    return ResultCode::OK;
}

}   // namespace nebula
}   // namespace nebula_chaos
