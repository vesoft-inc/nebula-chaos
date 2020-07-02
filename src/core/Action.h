/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef ACTIONS_ACTION_H_
#define ACTIONS_ACTION_H_

#include "common/Base.h"
#include <chrono>
#include <folly/Unit.h>
#include <folly/Try.h>
#include <folly/Executor.h>
#include <folly/futures/Future.h>
#include <folly/executors/GlobalExecutor.h>
#include <folly/futures/SharedPromise.h>
#include <folly/String.h>


namespace nebula_chaos {
namespace core {
using Clock = std::chrono::high_resolution_clock;
using TimePoint = Clock::time_point;
using Duration = Clock::duration;

enum class ResultCode {
    OK,
    ERR_TIMEOUT,
    ERR_NOT_FOUND,
    ERR_BAD_ARGUMENT,
    ERR_FAILED,
};

/**
 * This is the basic action class.
 * It is not thread-safe
 * */
class Action {
    friend class ChaosPlan;
public:
    enum class Status {
        INIT,
        RUNNING,
        SUCCEEDED,
        FAILED,
    };
    Action() = default;

    virtual ~Action() = default;

    bool addDependency(Action* action) {
        CHECK_NOTNULL(action);
        dependers_.emplace_back(action);
        action->dependees_.emplace_back(this);
        return true;
    }

    void setId(int32_t id) {
        id_ = id;
    }

    int32_t id() const {
        return id_;
    }

    Status status() const {
        return status_;
    }

    std::string statusStr() const {
        switch (status_) {
            case Status::INIT:
                return "init";
            case Status::RUNNING:
                return "running";
            case Status::SUCCEEDED:
                return "succceeded";
            case Status::FAILED:
                return "failed";
            default:
                break;
        }
        return "unknown";
    }

    void run() {
        CHECK(Status::INIT == status_);
        status_ = Status::RUNNING;
        TimePoint start = Clock::now();
        LOG(INFO) << "Begin the action " << id_ << ":" << toString();
        auto rc = this->doRun();
        auto end = Clock::now();
        timeSpent_ = end - start;
        CHECK(Status::RUNNING == status_);
        if (rc == ResultCode::OK) {
            status_ = Status::SUCCEEDED;
            LOG(INFO) << "Then action " << id_ << ":" << toString() << " finished!";
            promise_.setTry(folly::Try<folly::Unit>(folly::Unit()));
        } else {
            status_ = Status::FAILED;
            LOG(ERROR) << "The action " << id_ << ":" << toString() << " failed!";
            promise_.setException(
                    std::runtime_error(
                        folly::stringPrintf("run failed, rc %d",
                                            static_cast<int32_t>(rc))));
        }
    }

    void markFailed(folly::exception_wrapper&& ew) {
        status_ = Status::FAILED;
        promise_.setException(std::move(ew));
    }

    virtual std::string toString() const = 0;

protected:
    virtual ResultCode doRun() = 0;

protected:
    // other actions that depend on this action.
    std::vector<Action*>    dependers_;
    // other actions on which this action depends
    std::vector<Action*>    dependees_;
    Duration                timeSpent_;

private:
    Status status_{Status::INIT};
    folly::SharedPromise<folly::Unit> promise_;
    int32_t     id_;
};

class EmptyAction : public Action {
public:
    EmptyAction(const std::string& name = "EmptyAction")
        : name_(name) {}

    ~EmptyAction() = default;

    ResultCode doRun() override {
        LOG(INFO) << toString();
        return ResultCode::OK;
    }

    std::string toString() const override {
        return name_;
    }

private:
    std::string name_;
};

using ActionPtr = std::unique_ptr<Action>;

}   // namespace core
}   // namespace nebula_chaos

#endif  // ACTIONS_ACTION_H_
