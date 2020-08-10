#include "common/Base.h"
#include <gtest/gtest.h>
#include <glog/logging.h>
#include <folly/init/Init.h>
#include "core/CheckProcAction.h"
#include "core/SendEmailAction.h"
#include "core/LoopAction.h"
#include "core/AssignAction.h"

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
TEST(ActionsTest, LoopAction) {
    ActionContext ctx;

    auto action1 = std::make_unique<EmptyAction>("action1");
    action1->setId(1);
    auto action2 = std::make_unique<EmptyAction>("action2");
    action2->setId(2);
    auto action3 = std::make_unique<EmptyAction>("action3");
    action3->setId(3);
    auto action4 = std::make_unique<AssignAction>(&ctx, "i", "$i + 1");
    action4->setId(4);

    action4->addDependee(action3.get());
    action4->addDependee(action2.get());

    action2->addDependee(action1.get());
    action3->addDependee(action1.get());

    std::vector<ActionPtr> actions;
    actions.emplace_back(std::move(action1));
    actions.emplace_back(std::move(action2));
    actions.emplace_back(std::move(action3));
    actions.emplace_back(std::move(action4));


    AssignAction action(&ctx, "i", "0");
    auto ret = action.doRun();
    CHECK(ResultCode::OK == ret);

    LoopAction loopAction(&ctx, "$i < 10", std::move(actions), 2);
    loopAction.doRun();
}
TEST(ActionsTest, AssignActionTest) {
    ActionContext ctx;
    AssignAction action(&ctx, "a", "1 + 2 * 3");
    auto ret = action.doRun();
    CHECK(ResultCode::OK == ret);
    auto valOrErr = ctx.exprCtx.getVar("a");
    CHECK(valOrErr);
    CHECK_EQ(7, asInt(valOrErr.value()));
}

}  // namespace utils
}  // namespace nebula_chaos

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    folly::init(&argc, &argv, true);
    google::SetStderrLogging(google::INFO);
    return RUN_ALL_TESTS();
}
