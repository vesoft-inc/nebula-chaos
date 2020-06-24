/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef ACTIONS_ACTION_H_
#define ACTIONS_ACTION_H_
#include "common/Base.h"

namespace nebula_chaos {
namespace action {

/**
 * This is the basic action class.
 * It is not thread-safe
 * */
class Action {
public:
    Action() = default;

    virtual ~Action() = default;

    bool addDependency(Action* action) {
        CHECK_NOTNULL(action);
        dependers_.emplace_back(action);
        action->dependees_.emplace_back(this);
        return true;
    }

    virtual void run() = 0;

    virtual std::string toString() = 0;

protected:
    // other actions that depend on this action.
    std::vector<Action*>    dependers_;
    // other actions on which this action depends
    std::vector<Action*>    dependees_;
};

}   // namespace action
}   // namespace nebula_chaos

#endif  // ACTIONS_ACTION_H_
