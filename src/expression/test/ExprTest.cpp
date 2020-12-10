/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#include "common/base/Base.h"
#include <gtest/gtest.h>
#include <glog/logging.h>
#include <folly/init/Init.h>
#include "expression/Expressions.h"

namespace nebula_chaos {

TEST(ExprTest, ExprTest) {
    ExprContext ctx;
    ctx.setVar("a", 1L);
    {
        auto* left = new ConstantExpression(1L);
        auto* right = new ConstantExpression(2L);
        ArithmeticExpression expr(left, ArithmeticExpression::Operator::ADD, right);
        auto valOrErr = expr.eval(&ctx);
        CHECK(valOrErr);
        CHECK_EQ(3, ExprUtils::asInt(valOrErr.value()));
    }
    {
        auto* left = new ConstantExpression(1L);
        auto* right = new VariableExpression("a");
        ArithmeticExpression expr(left, ArithmeticExpression::Operator::ADD, right);
        auto valOrErr = expr.eval(&ctx);
        CHECK(valOrErr);
        CHECK_EQ(2, ExprUtils::asInt(valOrErr.value()));
    }
}

}  // namespace nebula_chaos

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    folly::init(&argc, &argv, true);
    google::SetStderrLogging(google::INFO);
    return RUN_ALL_TESTS();
}
