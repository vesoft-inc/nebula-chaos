/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef NEBULA_CLIENT_CPP_GRAPHCLIENT_H_
#define NEBULA_CLIENT_CPP_GRAPHCLIENT_H_

#include "common/Base.h"
#include "common/graph/Response.h"
#include <folly/String.h>
#include "nebula/client/Config.h"
#include "nebula/client/ConnectionPool.h"
#include "nebula/client/Session.h"

namespace nebula_chaos {
namespace plan {

using ErrorCode = nebula::ErrorCode;
using DataSet = nebula::DataSet;

class GraphClient {
public:
    GraphClient(const std::string& addr, uint16_t port);

    virtual ~GraphClient();

    // Authenticate the user
    ErrorCode connect(const std::string& username,
                      const std::string& password);

    void disconnect();

    ErrorCode execute(folly::StringPiece stmt,
                      nebula::DataSet& resp);

    std::string serverAddress() const {
        return folly::stringPrintf("%s:%d", addr_.c_str(), port_);
    }

private:
    std::unique_ptr<nebula::ConnectionPool> conPool_{nullptr};
    const std::string                       addr_;
    const uint16_t                          port_;
    std::unique_ptr<nebula::Session>        session_{nullptr};
};

}  // namespace plan
}  // namespace nebula_chaos

#endif  // NEBULA_CLIENT_CPP_GRAPHCLIENT_H_
