/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#include "utils/HttpClient.h"
#include <folly/Subprocess.h>

namespace chaos {
namespace utils {

folly::Optional<std::string>
HttpClient::get(const std::string& path, const std::string& options) {
    folly::Subprocess proc({NEBULA_STRINGIFY(CURL_EXEC), options, path},
                           folly::Subprocess::Options().pipeStdout());
    auto p = proc.communicate();
    try {
        proc.waitChecked();
        return p.first;
    } catch (const folly::CalledProcessError& e) {
        LOG(ERROR) << "Http get failed:" << path;
        return folly::none;
    }
}

}   // namespace utils
}   // namespace chaos
