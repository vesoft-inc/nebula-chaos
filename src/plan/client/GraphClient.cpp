/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#include "common/Base.h"
#include "plan/client/GraphClient.h"

namespace nebula_chaos {
namespace plan {

const int32_t kRetryTimes = 10;

GraphClient::GraphClient(const std::string& addr, uint16_t port)
        : addr_(addr)
        , port_(port) {
    conPool_ = std::make_unique<nebula::ConnectionPool>();
    auto address = folly::stringPrintf("%s:%u", addr.c_str(), port);
    conPool_->init({address}, nebula::Config{});
}

GraphClient::~GraphClient() {
    disconnect();
}

ErrorCode GraphClient::connect(const std::string& username,
                               const std::string& password) {
    session_ = conPool_->getSession(username, password);
    if (session_.valid()) {
        return nebula::ErrorCode::SUCCEEDED;
    }
    return nebula::ErrorCode::E_RPC_FAILURE;
}

void GraphClient::disconnect() {
    session_.release();
}

ErrorCode GraphClient::execute(folly::StringPiece stmt,
                               DataSet* resp) {
    if (!session_.valid()) {
        auto ret = session_.retryConnect();
        if (ret != nebula::ErrorCode::SUCCEEDED ||
            !session_.valid()) {
            return nebula::ErrorCode::E_DISCONNECTED;
        }
    }

    int32_t retry = 0;
    while (++retry < kRetryTimes) {
        auto exeRet = session_.execute(stmt.str());
        if (exeRet.errorCode() != nebula::ErrorCode::SUCCEEDED) {
            LOG(ERROR) << stmt.str() << " execute failed"
                       << static_cast<int>(exeRet.errorCode());
            if (retry + 1 ==  kRetryTimes) {
                return exeRet.errorCode();
            }
            sleep(retry);
            continue;
        }
        resp = exeRet.data();
        return nebula::ErrorCode::SUCCEEDED;
    }
}

}  // namespace plan
}  // namespace nebula_chaos
