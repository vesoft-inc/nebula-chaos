/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef CORE_ACTION_H_
#define CORE_ACTION_H_

#include "common/base/Base.h"
#include <chrono>
#include <folly/Unit.h>
#include <folly/Try.h>
#include <folly/Executor.h>
#include <folly/futures/Future.h>
#include <folly/executors/GlobalExecutor.h>
#include <folly/futures/SharedPromise.h>
#include <folly/String.h>
#include "expression/Expressions.h"

namespace chaos {
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
    ERR_FAILED_NO_RETRY,
    ERR_NOT_FINISHED,
};

// all actions share the same context
struct ActionContext {
    ExprContext exprCtx;
};

/**
 * This is the basic action class.
 * It is not thread-safe
 * */
class Action {
    friend class ChaosPlan;
    friend class LoopAction;

public:
    enum class Status {
        INIT,
        RUNNING,
        SUCCEEDED,
        FAILED,
    };

    explicit Action(ActionContext* ctx = nullptr)
        : ctx_(ctx)
        , promise_(std::make_unique<folly::SharedPromise<folly::Unit>>()) {}

    virtual ~Action() = default;

    bool addDependency(Action* action) {
        CHECK_NOTNULL(action);
        dependers_.emplace_back(action);
        action->dependees_.emplace_back(this);
        return true;
    }

    void addDependee(Action* action) {
        CHECK_NOTNULL(action);
        this->dependees_.emplace_back(action);
        action->dependers_.emplace_back(this);
    }

    virtual void reset() {
        if (status_ == Status::INIT
                || status_ == Status::RUNNING) {
            LOG(INFO) << "Can't reset action with status " << statusStr();
            return;
        }
        status_ = Status::INIT;
        promise_.reset(new folly::SharedPromise<folly::Unit>());
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
        CHECK(promise_ != nullptr);
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
            promise_->setTry(folly::Try<folly::Unit>(folly::Unit()));
        } else {
            status_ = Status::FAILED;
            LOG(ERROR) << "The action " << id_ << ":" << toString() << " failed!";
            promise_->setException(
                    std::runtime_error(
                        folly::stringPrintf("run failed, rc %d",
                                            static_cast<int32_t>(rc))));
        }
    }

    void markFailed(folly::exception_wrapper&& ew) {
        status_ = Status::FAILED;
        promise_->setException(std::move(ew));
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
    ActionContext*          ctx_ = nullptr;

private:
    Status status_{Status::INIT};
    std::unique_ptr<folly::SharedPromise<folly::Unit>> promise_;
    int32_t     id_ = -1;
};

using ActionPtr = std::unique_ptr<Action>;

class EmptyAction : public Action {
public:
    explicit EmptyAction(const std::string& name = "EmptyAction")
        : name_(name) {}

    ~EmptyAction() = default;

    ResultCode doRun() override {
        return ResultCode::OK;
    }

    std::string toString() const override {
        return name_;
    }

private:
    std::string name_;
};

class DisturbAction : public Action {
public:
    DisturbAction(int32_t loopTimes,
                  int32_t timeToDisurb,
                  int32_t timeToRecover)
        : loopTimes_(loopTimes)
        , timeToDisurb_(timeToDisurb)
        , timeToRecover_(timeToRecover) {}

    ResultCode doRun() override {
        int32_t i = 0;
        while (i++ < loopTimes_) {
            sleep(timeToDisurb_);
            auto rc = disturb();
            if (rc != ResultCode::OK) {
                LOG(ERROR) << "Disturb failed!";
                return rc;
            }
            sleep(timeToRecover_);
            rc = recover();
            if (rc != ResultCode::OK) {
                LOG(ERROR) << "Recover failed!";
                return rc;
            }
        }
        return ResultCode::OK;
    }

protected:
    virtual ResultCode disturb() = 0;

    virtual ResultCode recover() = 0;

    int32_t loopTimes_;
    int32_t timeToDisurb_;    // seconds
    int32_t timeToRecover_;   // seconds
};

}   // namespace core
}   // namespace chaos

#endif  // CORE_ACTION_H_
