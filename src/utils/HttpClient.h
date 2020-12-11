/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef UTILS_HTTPCLIENT_H_
#define UTILS_HTTPCLIENT_H_

#include "common/base/Base.h"
#include <folly/Optional.h>

namespace chaos {
namespace utils {

class HttpClient {
public:
    HttpClient() = delete;

    ~HttpClient() = default;

    static folly::Optional<std::string> get(const std::string& path,
                                            const std::string& options = "-G");
};

}   // namespace utils
}   // namespace chaos
#endif  // UTILS_HTTPCLIENT_H_
