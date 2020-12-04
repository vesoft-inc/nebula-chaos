/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */
#include "nebula/NebulaInstance.h"
#include "utils/SshHelper.h"
#include <boost/algorithm/string.hpp>

namespace nebula_chaos {
namespace nebula {

bool NebulaInstance::parseConf(const std::string& confFile) {
    LOG(INFO) << "Parse conf file " << confFile
              << " on " << host_;
    this->conf_ = folly::dynamic::object;
    auto ret = utils::SshHelper::run(
                    folly::stringPrintf("cat %s", confFile.c_str()),
                    host_,
                    [this] (const std::string& outMsg) {
                        VLOG(1) << "The output is " << outMsg;
                        int lineNo = 0;
                        folly::gen::lines(outMsg) | [&](folly::StringPiece line) {
                            lineNo++;
                            VLOG(1) << lineNo << ":" << line;
                            if (line.startsWith("--")) {
                                std::vector<folly::StringPiece> kv;
                                folly::split("=", line, kv, true);
                                if (kv.size() == 2) {
                                    this->conf_[kv[0]] = kv[1];
                                } else {
                                    LOG(ERROR) << "Bad format line, " << line;
                                }
                            }
                        };
                        CHECK_LT(0, lineNo);
                    },
                    [] (const std::string& errMsg) {
                        LOG(ERROR) << "The error is " << errMsg;
                    },
                    owner_);
    CHECK_EQ(0, ret.exitStatus());
    VLOG(1) << "The conf is " << this->conf_;
    return true;
}

bool NebulaInstance::init() {
    // This is the default conf file name inside nebula, to keep it simple, we
    // just use it. If we can't find it, dump err message inside parseConf.
    switch (type_) {
        case NebulaInstance::Type::STORAGE: {
            moduleName_ = "storaged";
            break;
        }
        case NebulaInstance::Type::META: {
            moduleName_ = "metad";
            break;
        }
        case NebulaInstance::Type::GRAPH: {
            moduleName_ = "graphd";
            break;
        }
        default:
            LOG(FATAL) << "Unknown type, type " << static_cast<int32_t>(type_);
            break;
    }
    if (confPath_.empty()) {
        confPath_ = folly::stringPrintf("%s/etc/nebula-%s.conf",
                                        installPath_.c_str(),
                                        moduleName_.c_str());
    }
    return parseConf(confPath_);
}

folly::Optional<int32_t> NebulaInstance::getPid(bool skipCache) {
    if (pid_ != -1 && !skipCache) {
        return pid_;
    }
    auto it = conf_.find("--pid_file");
    if (it == conf_.items().end()) {
        LOG(ERROR) << "Can't find the pid file in conf";
        return folly::none;
    }
    VLOG(1) << "Found the pid file " << it->second;
    std::string pidFile = it->second.asString();
    // It is relative path.
    if (!boost::starts_with(pidFile, "/")) {
        pidFile = folly::stringPrintf("%s/%s", installPath_.c_str(), pidFile.c_str());
    }
    auto ret = utils::SshHelper::run(
                    folly::stringPrintf("cat %s", pidFile.c_str()),
                    host_,
                    [this] (const std::string& outMsg) {
                        VLOG(1) << "The output is " << outMsg;
                        try {
                            this->pid_ = folly::to<int32_t>(outMsg);
                        } catch (const folly::ConversionError& e) {
                            LOG(ERROR) << "Parse pid file failed, error " << e.what();
                        }
                    },
                    [] (const std::string& errMsg) {
                        LOG(ERROR) << "The error is " << errMsg;
                    },
                    owner_);
    if (ret.exitStatus() != 0) {
        return folly::none;
    }
    LOG(INFO) << "The pid for current instance is " << pid_;
    return pid_;
}

folly::Optional<int32_t> NebulaInstance::getIntFromConf(const std::string& key) const {
    try {
        CHECK(conf_.isObject());
        auto it = conf_.find(folly::stringPrintf("--%s", key.c_str()));
        if (it == conf_.items().end()) {
            LOG(ERROR) << "Can't find the " << key << " in conf";
            return folly::none;
        }
        VLOG(1) << "Found the " <<  key << ":" << it->second;
        return folly::to<int32_t>(it->second.asString());
    } catch (const folly::ConversionError& e) {
        LOG(ERROR) << "Parse failed, error " << e.what();
        return folly::none;
    } catch (const folly::TypeError& e) {
        LOG(ERROR) << "Parse failed, error " << e.what();
        return folly::none;
    }
}

folly::Optional<std::string> NebulaInstance::getStringFromConf(const std::string& key) const {
    try {
        CHECK(conf_.isObject());
        auto it = conf_.find(folly::stringPrintf("--%s", key.c_str()));
        if (it == conf_.items().end()) {
            LOG(ERROR) << "Can't find the " << key << " in conf";
            return folly::none;
        }
        VLOG(1) << "Found the " <<  key << ":" << it->second;
        return it->second.asString();
    } catch (const folly::ConversionError& e) {
        LOG(ERROR) << "Parse failed, error " << e.what();
        return folly::none;
    } catch (const folly::TypeError& e) {
        LOG(ERROR) << "Parse failed, error " << e.what();
        return folly::none;
    }
}

folly::Optional<int32_t> NebulaInstance::getPort() const {
    return getIntFromConf("port");
}

folly::Optional<int32_t> NebulaInstance::getHttpPort() const {
    return getIntFromConf("ws_http_port");
}

folly::Optional<std::vector<std::string>>
NebulaInstance::dataDirs() const {
    try {
        CHECK(conf_.isObject());
        auto it = conf_.find("--data_path");
        if (it == conf_.items().end()) {
            LOG(ERROR) << "Can't find data_path in conf";
            return folly::none;
        }
        auto pathStr = it->second.asString();
        std::vector<std::string> strArray;
        folly::split(",", pathStr, strArray);
        for (auto& dataPath : strArray) {
            if (!boost::starts_with(dataPath, "/")) {
                dataPath = folly::stringPrintf("%s/%s", installPath_.c_str(), dataPath.c_str());
            }
        }
        return strArray;
    } catch (const folly::ConversionError& e) {
        LOG(ERROR) << "Parse failed, error " << e.what();
        return folly::none;
    } catch (const folly::TypeError& e) {
        LOG(ERROR) << "Parse failed, error " << e.what();
        return folly::none;
    }
}

folly::Optional<std::vector<std::string>>
NebulaInstance::walDirs(int64_t spaceId) const {
    auto data = dataDirs();
    if (!data.hasValue()) {
        LOG(ERROR) << "Get data path failed!";
        return folly::none;
    }
    std::vector<std::string> walPaths;
    walPaths.reserve(8);
    for (auto& dataPath : data.value()) {
        walPaths.emplace_back(folly::stringPrintf("%s/nebula/%ld/wal", dataPath.c_str(), spaceId));
    }
    return walPaths;
}

std::string NebulaInstance::command(const std::string& cmd) const {
    return folly::stringPrintf(
            "%s/scripts/nebula.service -c %s %s %s > /dev/null 2>&1 &",
            installPath_.c_str(),
            confPath_.c_str(),
            cmd.c_str(),
            moduleName_.c_str());
}

std::string NebulaInstance::startCommand() const {
    return command("start");
}

std::string NebulaInstance::stopCommand() const {
    return command("stop");
}

std::string NebulaInstance::killCommand() const {
    return command("kill");
}

}   // namespace nebula
}   // namespace nebula_chaos

