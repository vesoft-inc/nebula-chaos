/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef NEBULA_CLIENT_CPP_GRAPHCLIENT_H_
#define NEBULA_CLIENT_CPP_GRAPHCLIENT_H_

#include "common/Base.h"
#include "nebula/interface/gen-cpp2/GraphServiceAsyncClient.h"
#include <folly/executors/IOThreadPoolExecutor.h>

namespace nebula_chaos {
namespace nebula {

using Code = ::nebula::graph::cpp2::ErrorCode;
using ExecutionResponse = ::nebula::graph::cpp2::ExecutionResponse;
using GraphServiceAsyncClient = ::nebula::graph::cpp2::GraphServiceAsyncClient;

class GraphClient {
public:
    GraphClient(const std::string& addr, uint16_t port);
    virtual ~GraphClient();

    // Authenticate the user
    Code connect(const std::string& username,
                 const std::string& password);

    void disconnect();

    Code execute(folly::StringPiece stmt,
                 ExecutionResponse& resp);

    std::string serverAddress() const {
        return folly::stringPrintf("%s:%d", addr_.c_str(), port_);
    }

private:
    std::unique_ptr<GraphServiceAsyncClient> client_;
    const std::string addr_;
    const uint16_t port_;
    int64_t sessionId_;
    std::unique_ptr<folly::IOThreadPoolExecutor> ioPool_;
    folly::EventBase* evb_;
};

}  // namespace nebula
}  // namespace nebula_chaos

#endif  // NEBULA_CLIENT_CPP_GRAPHCLIENT_H_
