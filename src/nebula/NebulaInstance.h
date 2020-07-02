/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef NEBULA_NEBULAINSTANCE_H_
#define NEBULA_NEBULAINSTANCE_H_

#include "common/Base.h"
#include <folly/dynamic.h>

namespace nebula_chaos {
namespace nebula {

class NebulaInstance {
public:
    enum class Type {
        STORAGE,
        META,
        GRAPH,
        UNKNOWN,
    };

    NebulaInstance() = default;
    NebulaInstance(const std::string& host,
                   const std::string& installPath,
                   NebulaInstance::Type type,
                   const std::string& confPath = "",
                   const std::string& owner = "")
        : host_(host)
        , installPath_(installPath)
        , type_(type)
        , confPath_(confPath)
        , owner_(owner) {}

    bool init();

    /**
     * Read the pid inside PID file, so if the instance not started,
     * it will get the last pid.
     * */
    folly::Optional<int32_t> getPid(bool skipCache = true);

    folly::Optional<int32_t> getPort() const;

    folly::Optional<int32_t> getHttpPort() const;

    std::string startCommand() const;

    std::string stopCommand() const;

    std::string killCommand() const;

    std::string toString() const {
        auto port = getPort();
        if (port.hasValue()) {
            return folly::stringPrintf("%s:%d", host_.c_str(), port.value());
        }
        return "";
    }

    const std::string& owner() const {
        return owner_;
    }

    const std::string& getHost() const {
        return host_;
    }

private:
    folly::Optional<int32_t> getIntFromConf(const std::string& key) const;

    bool parseConf(const std::string& confFile);

    std::string command(const std::string& cmd) const;

private:
    folly::dynamic conf_;
    std::string host_;
    std::string installPath_;
    Type type_;
    int32_t     pid_ = -1;
    std::string moduleName_;
    std::string confPath_;
    std::string owner_;
};

}   // namespace nebula
}   // namespace nebula_chaos

#endif  // NEBULA_NEBULAINSTANCE_H_
