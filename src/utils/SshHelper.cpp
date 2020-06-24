/* Copyright (c) 2020 nebula inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#include "utils/SshHelper.h"
#include <folly/String.h>

namespace nebula_chaos {
namespace utils {

// static
folly::ProcessReturnCode
SshHelper::run(const std::string& command,
               const std::string& hostName,
               ReadCallback readStdout,
               ReadCallback readStderr,
               const std::string& user) {
    std::string remote;
    if (!user.empty()) {
        remote = folly::stringPrintf("%s@%s", user.c_str(), hostName.c_str());
    } else {
        remote = hostName;
    }
    VLOG(1) << "Remote " << remote  << ", command " << command;
    folly::Subprocess proc({NEBULA_STRINGIFY(SSH_EXEC), remote, command},
                           folly::Subprocess::Options().pipeStdout());
    auto p = proc.communicate();
    readStdout(p.first);
    if (!p.second.empty()) {
        readStderr(p.second);
    }
    return proc.wait();
}

}  // namespace utils
}  // namespace nebula


