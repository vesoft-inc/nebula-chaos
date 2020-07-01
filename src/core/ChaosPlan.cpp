/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */
#include "core/ChaosPlan.h"

DEFINE_string(email_to, "", "");

namespace nebula_chaos {
namespace core {

// Transfer the ownership into the plan.
void ChaosPlan::addAction(ActionPtr action) {
    LOG(INFO) << "Add action " << action->toString() << " into plan";
    actions_.emplace_back(std::move(action));
}

void ChaosPlan::addActions(std::vector<ActionPtr>&& actions) {
    for (auto& action : actions) {
        LOG(INFO) << "Add action " << action->toString() << " into plan";
        actions_.emplace_back(std::move(action));
    }
}

void ChaosPlan::schedule() {
    prepare();

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

    addAction(std::make_unique<RunTaskAction>([this]() {
        std::string subject;
        if (status_ == Status::SUCCEEDED) {
            subject = "Nebula Survived!";
        } else {
            subject = "Thanos killed nebula!";
        }
        auto content = this->toString();
        SendEmailAction action(subject, content, FLAGS_email_to);
        return action.doRun();
    }));

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
            LOG(INFO) << "Add dependee " << dee->toString() << " for " << action->toString();
            dependees.emplace_back(dee->promise_.getFuture());
        }
        auto actionPtr = action.get();
        folly::collect(dependees)
                    .via(threadsPool_.get())
                    .thenValue([this, actionPtr](auto&&) {
                        actionPtr->run();
                    })
                    .thenError([this, actionPtr](auto ew) {
                        LOG(ERROR) << "Run " << actionPtr->toString() << " failed, msg " << ew.what();
                        actionPtr->markFailed(std::move(ew));
                        if (status_ == Status::SUCCEEDED) {
                            status_ = Status::FAILED;
                        }
                    });
    }
    sinkAction->promise_.getFuture().wait();
    if (sinkAction->status() == ActionStatus::FAILED) {
        LOG(INFO) << "The plan failed, rerun the last action to ensure the email has been send out!";
        sinkAction->doRun();
    }
    return;
}

std::string ChaosPlan::toString() const {
    std::stringstream str;
    str << "STATUS: " << (status_ == Status::SUCCEEDED ? "SUCCEEDED" : "FAILED") << "\n";
    for (auto& action : actions_) {
        str << "Action " << action->toString() << ", Status: " << action->statusStr() << "\n";
    }
    return str.str();
}

}  // action
}  // nebula_chaos
