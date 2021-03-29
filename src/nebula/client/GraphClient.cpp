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
    std::lock_guard<std::mutex> lk(sessionLk_);
    auto session = conPool_->getSession(username, password);
    if (session.valid()) {
        session_.reset(new nebula::Session(std::move(session)));
        return nebula::ErrorCode::SUCCEEDED;
    }
    return nebula::ErrorCode::E_RPC_FAILURE;
}

void GraphClient::disconnect() {
    std::lock_guard<std::mutex> lk(sessionLk_);
    if (session_ != nullptr) {
        session_ = nullptr;
    }
    conPool_ = nullptr;
}

ErrorCode GraphClient::execute(folly::StringPiece stmt,
                               nebula::DataSet& resp,
                               std::string* errMSg) {
    std::lock_guard<std::mutex> lk(sessionLk_);
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
        auto errCode = exeRet.errorCode;

        if (errCode == nebula::ErrorCode::E_RPC_FAILURE) {
            LOG(ERROR) << "Thrift rpc call failed, retry times " << retry;
            if (retry + 1 <= kRetryTimes) {
                sleep(retry);
            }
            continue;
        } else if (errCode != nebula::ErrorCode::SUCCEEDED) {
            auto* msg = exeRet.errorMsg.get();
            if (msg != nullptr) {
                LOG(ERROR) << *msg;
                if (errMSg != nullptr) {
                    *errMSg = *msg;
                }
            }
            LOG(ERROR) << stmt.str() << " execute failed, error code : "
                       << static_cast<int>(errCode);
            return errCode;
        } else {
            // Not every ResultSet returned by Session::execute contains a DataSet
            auto* dataSet = exeRet.data.get();
            if (dataSet != nullptr) {
                resp = *(const_cast<nebula::DataSet*>(dataSet));
            }

            // Save the current spacename when the execution is successful
            auto* spaceName = exeRet.spaceName.get();
            if (spaceName != nullptr) {
                spaceName_ = *spaceName;
            }
            return nebula::ErrorCode::SUCCEEDED;
        }
    }

    // Reconnect to server, then restore space
    {
        auto ret = session_->retryConnect();
        if (ret != nebula::ErrorCode::SUCCEEDED || !session_->valid()) {
            return nebula::ErrorCode::E_DISCONNECTED;
        }

        // Restore to the current space
        if (!spaceName_.empty()) {
            auto restoreSpace = folly::stringPrintf("USE %s", spaceName_.c_str());
            auto exeRet = session_->execute(restoreSpace);
            auto errCode = exeRet.errorCode;

            if (errCode != nebula::ErrorCode::SUCCEEDED) {
                LOG(ERROR) << "Restore space failed when thrift rpc call failed!";
            } else {
                LOG(INFO) << "Restore space successed when thrift rpc call failed!";
            }
        }
    }
    return nebula::ErrorCode::E_RPC_FAILURE;
}

}  // namespace nebula_chaos
}  // namespace chaos
