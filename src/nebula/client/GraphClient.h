/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef NEBULA_CLIENT_CPP_GRAPHCLIENT_H_
#define NEBULA_CLIENT_CPP_GRAPHCLIENT_H_

#include "common/Base.h"
#include "nebula/client/Config.h"
#include "nebula/client/ConnectionPool.h"
#include "nebula/client/Session.h"

namespace nebula_chaos {
namespace nebula {

using Code = ::nebula::graph::cpp2::ErrorCode;

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
    std::unique_ptr<ConnectionPool> pool_;
    const std::string addr_;
    const uint16_t port_;
    Session session_;
};

}  // namespace nebula
}  // namespace nebula_chaos

#endif  // NEBULA_CLIENT_CPP_GRAPHCLIENT_H_
