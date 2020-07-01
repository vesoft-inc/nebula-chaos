/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef CORE_SENDEMAILACTION_H_
#define CORE_SENDEMAILACTION_H_

#include "common/Base.h"
#include "core/Action.h"
#include <folly/Subprocess.h>

namespace nebula_chaos {
namespace core {

class SendEmailAction : public Action {
public:
    SendEmailAction(const std::string& subject,
                    const std::string& content,
                    const std::string& to)
        : subject_(subject)
        , content_(content)
        , to_(to) {
    }

    ~SendEmailAction() = default;

    ResultCode doRun() override {
        auto mail = folly::stringPrintf("%s '%s' | %s -s '%s' %s",
                                        NEBULA_STRINGIFY(ECHO_EXEC),
                                        content_.c_str(),
                                        NEBULA_STRINGIFY(MAIL_EXEC),
                                        subject_.c_str(),
                                        to_.c_str());
        folly::Subprocess proc(std::vector<std::string>{"/bin/bash", "-c", mail},
                               folly::Subprocess::Options().pipeStdout());
        proc.waitChecked();
        return ResultCode::OK;
    }

    std::string toString() const override {
        return folly::stringPrintf("Send e-mail to %s", to_.c_str());
    }

protected:
    std::string subject_;
    std::string content_;
    std::string to_;
};

}   // namespace core
}   // namespace nebula_chaos

#endif  // CORE_SENDEMAILACTION_H_
