/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */
#include "core/CheckProcAction.h"
#include "utils/SshHelper.h"

namespace nebula_chaos {
namespace core {

ResultCode CheckProcAction::doRun() {
    auto command = folly::stringPrintf("ps -p %d", pid_);
    LOG(INFO) << toString();
    int32_t procsNo = 0;
    utils::SshHelper::run(
                command,
                host_,
                [&procsNo] (const std::string& outMsg) {
                    folly::gen::lines(outMsg) | [&](folly::StringPiece line) {
                        line  = folly::trimWhitespace(line);
                        VLOG(1) << line;
                        if (!line.startsWith("PID")) {
                            procsNo++;
                        }
                    };
                },
                [] (const std::string& errMsg) {
                    LOG(ERROR) << "The error is " << errMsg;
                },
                user_);
    return procsNo > 0 ? ResultCode::OK : ResultCode::ERR_NOT_FOUND;
}

}   // namespace core
}   // namespace nebula_chaos

