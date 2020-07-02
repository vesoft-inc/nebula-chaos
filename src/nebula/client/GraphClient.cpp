/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#include "common/Base.h"
#include "nebula/client/GraphClient.h"
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

DEFINE_int32(server_conn_timeout_ms, 1000,
             "Connection timeout in milliseconds");


namespace nebula_chaos {
namespace nebula {
const int32_t kRetryTimes = 10;
using apache::thrift::async::TAsyncSocket;
using apache::thrift::HeaderClientChannel;

GraphClient::GraphClient(const std::string& addr, uint16_t port)
        : addr_(addr)
        , port_(port)
        , sessionId_(0) {
    ioPool_ = std::make_unique<folly::IOThreadPoolExecutor>(1);
    evb_ = ioPool_->getEventBase();
}


GraphClient::~GraphClient() {
    disconnect();
}


Code GraphClient::connect(const std::string& username,
                          const std::string& password) {
    evb_->runImmediatelyOrRunInEventBaseThreadAndWait([this]() {
        auto socket = TAsyncSocket::newSocket(
            evb_,
            addr_,
            port_,
            FLAGS_server_conn_timeout_ms);

        client_ = std::make_unique<GraphServiceAsyncClient>(
            HeaderClientChannel::newChannel(socket));
    });

    try {
        auto f = folly::via(evb_, [this, &username, &password]() -> auto {
            return client_->future_authenticate(username, password);
        });
        auto resp = std::move(f).get();
        if (resp.get_error_code() != Code::SUCCEEDED) {
            LOG(ERROR) << "Failed to authenticate \"" << username << "\": "
                       << ((!resp.get_error_msg()) ? *(resp.get_error_msg()) : "");
            return resp.get_error_code();
        }
        sessionId_ = *(resp.get_session_id());
        return Code::SUCCEEDED;
    } catch (const std::exception& ex) {
        LOG(ERROR) << "Thrift rpc call failed: " << ex.what();
        return Code::E_RPC_FAILURE;
    }

}


void GraphClient::disconnect() {
    if (!client_) {
        return;
    }

    auto f = folly::via(evb_, [this]() -> auto {
        client_->future_signout(sessionId_);
    });
    f.wait();
    client_.reset();
}


Code GraphClient::execute(folly::StringPiece stmt,
                          ExecutionResponse& resp) {
    if (!client_) {
        LOG(ERROR) << "Disconnected from the server";
        return Code::E_DISCONNECTED;
    }
    int32_t retry = 0;
    while (++retry < kRetryTimes) {
        try {
            auto f = folly::via(evb_, [this, &stmt]() -> auto {
                return client_->future_execute(sessionId_, stmt.toString());
            });
            resp = std::move(f).get();
            auto* msg = resp.get_error_msg();
            if (msg != nullptr) {
                LOG(WARNING) << *msg;
            }
            return resp.get_error_code();
        } catch (const std::exception& ex) {
            LOG(ERROR) << "Thrift rpc call failed: " << ex.what();
        }
        sleep(retry);
    }
    // Reconnect to server with a new socket
    evb_->runImmediatelyOrRunInEventBaseThreadAndWait([this]() {
        auto socket = TAsyncSocket::newSocket(
            evb_,
            addr_,
            port_,
            FLAGS_server_conn_timeout_ms);

        client_ = std::make_unique<GraphServiceAsyncClient>(
            HeaderClientChannel::newChannel(socket));
    });
    return Code::E_RPC_FAILURE;
}

}  // namespace nebula
}  // namespace nebula_chaos
