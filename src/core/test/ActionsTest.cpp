#include "common/Base.h"
#include <gtest/gtest.h>
#include <glog/logging.h>
#include <folly/init/Init.h>
#include "core/CheckProcAction.h"
#include "core/SendEmailAction.h"

namespace nebula_chaos {
namespace core {

TEST(ActionsTest, CheckProcActionTest) {
    CheckProcAction action("127.0.0.1", 10);
    action.doRun();
}

TEST(ActionsTest, SendEmailAction) {
    SendEmailAction action("Thanos kill nebula!!!",
                           "Oh my god, nebula dead again!",
                           "heng.chen@vesoft.com");
    action.doRun();
}

}  // namespace utils
}  // namespace nebula_chaos

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    folly::init(&argc, &argv, true);
    google::SetStderrLogging(google::INFO);
    return RUN_ALL_TESTS();
}
