/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#include "nebula/client/GraphClient.h"

namespace chaos {
namespace nebula_chaos {

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
    auto session = conPool_->getSession(username, password);
    if (session.valid()) {
        session_.reset(new nebula::Session(std::move(session)));
        return nebula::ErrorCode::SUCCEEDED;
    }
    return nebula::ErrorCode::E_RPC_FAILURE;
}

void GraphClient::disconnect() {
    if (session_ != nullptr) {
        session_->release();
        session_ = nullptr;
    }
    conPool_ = nullptr;
}

ErrorCode GraphClient::execute(folly::StringPiece stmt,
                               nebula::DataSet& resp,
                               std::string* errMSg) {
    if (!session_->valid()) {
        auto ret = session_->retryConnect();
        if (ret != nebula::ErrorCode::SUCCEEDED ||
            !session_->valid()) {
            return nebula::ErrorCode::E_DISCONNECTED;
        }
    }

    // If an exception is thrown in Connection::execute,
    // the error E_RPC_FAILURE will be returned and need to retry
    int32_t retry = 0;
    while (++retry <= kRetryTimes) {
        auto exeRet = session_->execute(stmt.str());
        auto errCode = exeRet.errorCode();

        if (errCode == nebula::ErrorCode::E_RPC_FAILURE) {
            LOG(ERROR) << "Thrift rpc call failed, retry times " << retry;
            if (retry + 1 <= kRetryTimes) {
                sleep(retry);
            }
            continue;
        } else if (errCode != nebula::ErrorCode::SUCCEEDED) {
            auto* msg = exeRet.errorMsg();
            if (msg != nullptr) {
                LOG(ERROR) << *msg;
                if (errMSg != nullptr) {
                    *errMSg = *msg;
                }
            }
            LOG(ERROR) << stmt.str() << " execute failed, error code : "
                       << static_cast<int>(exeRet.errorCode());
            return errCode;
        } else {
            // Not every ResultSet returned by Session::execute contains a DataSet
            auto* dataSet = exeRet.data();
            if (dataSet != nullptr) {
                resp = *(const_cast<nebula::DataSet*>(dataSet));
            }
            return nebula::ErrorCode::SUCCEEDED;
        }
    }
    return nebula::ErrorCode::E_RPC_FAILURE;
}

}  // namespace nebula_chaos
}  // namespace chaos
