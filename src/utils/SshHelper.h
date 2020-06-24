/* Copyright (c) 2020 nebula inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef UTILS_SSHHELPER_H_
#define UTILS_SSHHELPER_H_

#include "common/Base.h"
#include <folly/Subprocess.h>

namespace nebula_chaos {
namespace utils {

using ReadCallback = std::function<void(const std::string&)>;

class SshHelper {
public:
    /**
     * ssh to the host as current user, and run command
     * */
    static folly::ProcessReturnCode run(const std::string& command,
                                        const std::string& hostName,
                                        ReadCallback readStdout,
                                        ReadCallback readStderr,
                                        const std::string& user = "");

private:
    SshHelper() = default;
};

}  // namespace utils
}  // namespace nebula

#endif  // UTILS_SSHHELPER_H_

