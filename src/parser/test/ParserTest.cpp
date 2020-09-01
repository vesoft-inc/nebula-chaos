#include "common/Base.h"
#include <gtest/gtest.h>
#include <glog/logging.h>
#include <folly/init/Init.h>
#include "parser/ParserHelper.h"

namespace nebula_chaos {

TEST(ParserTest, ExpressionTest) {
    ExprContext ctx;
    ctx.setVar("a", 1L);

    {
        auto expr = ParserHelper::parse("1 + 1");
        CHECK(expr != nullptr);
        LOG(INFO) << "The expression is " << expr->toString();
        auto valOrErr = expr->eval(&ctx);
        CHECK(valOrErr);
        CHECK_EQ(2, asInt(valOrErr.value()));
    }
    {
        auto expr = ParserHelper::parse("$a + 3");
        CHECK(expr != nullptr);
        LOG(INFO) << "The expression is " << expr->toString();
        auto valOrErr = expr->eval(&ctx);
        CHECK(valOrErr);
        CHECK_EQ(4, asInt(valOrErr.value()));
    }
}

}  // namespace nebula_chaos

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    folly::init(&argc, &argv, true);
    google::SetStderrLogging(google::INFO);
    return RUN_ALL_TESTS();
}
