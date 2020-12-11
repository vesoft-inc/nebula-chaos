/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#include "core/ChaosPlan.h"

namespace chaos {
namespace core {

// Transfer the ownership into the plan.
void ChaosPlan::addAction(ActionPtr action) {
    if (action->id() < 0) {
        action->setId(actions_.size());
    }
    LOG(INFO) << "Add action " << action->id()
              << ":" << action->toString() << " into plan";
    actions_.emplace_back(std::move(action));
}

void ChaosPlan::addActions(std::vector<ActionPtr>&& actions) {
    for (auto& action : actions) {
        addAction(std::move(action));
    }
}

void ChaosPlan::schedule() {
    prepare();
    auto start = Clock::now();
    std::vector<Action*> rootActions;
    std::vector<Action*> leafActions;

    for (auto& action : actions_) {
        if (action->dependees_.empty()) {
            rootActions.emplace_back(action.get());
        }
        if (action->dependers_.empty()) {
            leafActions.emplace_back(action.get());
        }
    }

    addAction(std::make_unique<RunTaskAction>([this, &start]() {
        timeSpent_ = Clock::now() - start;
        if (emailTo_.empty()) {
            return ResultCode::OK;
        }
        std::string subject;
        if (status_ == Status::SUCCEEDED) {
            subject = folly::stringPrintf("%s passed!", planName_.c_str());
        } else {
            subject = folly::stringPrintf("%s failed!", planName_.c_str());
        }
        auto content = this->toString();
        SendEmailAction action(subject, content, emailTo_, attachment_);
        return action.doRun();
    }, "Send Email"));

    auto* sinkAction = last();
    for (auto* action : leafActions) {
        action->addDependency(sinkAction);
    }

    addAction(std::make_unique<EmptyAction>("Begin"));
    auto* sourceAction = last();
    for (auto* action : rootActions) {
        sourceAction->addDependency(action);
    }

    for (auto& action : actions_) {
        std::vector<folly::Future<folly::Unit>> dependees;
        for (auto* dee : action->dependees_) {
            LOG(INFO) << "Add depender " << action->id() << " for " << dee->id();
            dependees.emplace_back(dee->promise_->getFuture());
        }
        auto actionPtr = action.get();
        folly::collect(dependees)
                    .via(threadsPool_.get())
                    .thenValue([actionPtr](auto&&) {
                        actionPtr->run();
                    })
                    .thenError([this, actionPtr](auto ew) {
                        LOG(ERROR) << "Run " << actionPtr->toString()
                                   << " failed, msg " << ew.what();
                        actionPtr->markFailed(std::move(ew));
                        if (status_ == Status::SUCCEEDED) {
                            status_ = Status::FAILED;
                        }
                    });
    }
    sinkAction->promise_->getFuture().wait();
    if (sinkAction->status() == ActionStatus::FAILED) {
        LOG(INFO) << "The plan failed, rerun the last action "
                  << "to ensure the email has been send out!";
        sinkAction->doRun();
    }
    return;
}

std::string ChaosPlan::toString() const {
    std::stringstream str;
    str << "STATUS: " << (status_ == Status::SUCCEEDED ? "SUCCEEDED" : "FAILED") << "\n";
    for (auto& action : actions_) {
        if (action->dependees_.empty() || action->dependers_.empty()) {
            continue;
        }
        str << "Action " << action->id() << ", " << action->toString()
            << ", Status: " << action->statusStr()
            << ", Cost "
            << std::chrono::duration_cast<std::chrono::milliseconds>(action->timeSpent_).count()
            << "ms, Depends on Action ";
        for (auto& der : action->dependees_) {
            str << der->id() << ",";
        }
        str.seekp(-1, std::ios_base::end);
        str << std::endl;
    }
    str << "TIME COSTS: "
        << std::chrono::duration_cast<std::chrono::milliseconds>(timeSpent_).count() << "ms\n";
    return str.str();
}

}  // namespace core
}  // namespace chaos
