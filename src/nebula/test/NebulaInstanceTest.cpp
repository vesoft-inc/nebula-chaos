/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#include "common/base/Base.h"
#include <gtest/gtest.h>
#include <glog/logging.h>
#include <folly/init/Init.h>
#include "nebula/NebulaInstance.h"

namespace chaos {
namespace nebula_chaos {

TEST(NebulaInstanceTest, DateTest) {
    auto installPath = folly::stringPrintf("%s/mock/nebula",
                                           NEBULA_STRINGIFY(NEBULA_CHAOS_HOME));
    NebulaInstance instance("127.0.0.1",
                            installPath,
                            NebulaInstance::Type::STORAGE);
    CHECK(instance.init());
    CHECK_EQ(10086, instance.getPid().value());
    CHECK_EQ(44500, instance.getPort().value());
    CHECK_EQ(12000, instance.getHttpPort().value());
    auto dataDirs = instance.dataDirs();
    CHECK(dataDirs.hasValue());
    auto dirs = std::move(dataDirs).value();
    CHECK_EQ(3, dirs.size());
    int i = 0;
    for (auto& d : dirs) {
        CHECK_EQ(folly::stringPrintf("%s/data%d", installPath.c_str(), ++i), d);
    }
}

}  // namespace nebula_chaos
}  // namespace chaos

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    folly::init(&argc, &argv, true);
    google::SetStderrLogging(google::INFO);
    return RUN_ALL_TESTS();
}
