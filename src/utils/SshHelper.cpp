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
SshHelper::run(const std::string& command, const std::string& hostName) {
#if defined(SSH_EXEC)
    folly::Subprocess proc({NEBULA_STRINGIFY(SSH_EXEC), hostName, command},
                           folly::Subprocess::Options().pipeStdout());
    auto p = proc.communicate();
    VLOG(1) << "stdout " << p.first;
    VLOG(1) << "stderr " << p.second;
    return proc.wait();
#endif
}

}  // namespace utils
}  // namespace nebula


