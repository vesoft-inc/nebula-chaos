/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#include "common/base/Base.h"
#include <gtest/gtest.h>
#include <glog/logging.h>
#include <folly/init/Init.h>
#include <folly/Subprocess.h>
#include <folly/gen/Base.h>
#include <folly/gen/File.h>
#include <folly/gen/String.h>
#include "utils/SshHelper.h"

namespace chaos {
namespace utils {

TEST(SSHHelperTest, DateTest) {
    auto ret = SshHelper::run("date",
                              "127.0.0.1",
                              [] (const std::string& out) {
                                 LOG(INFO) << out;
                              },
                              [] (const std::string& err) {
                                 LOG(ERROR) << err;
                              });
    EXPECT_EQ(0, ret.exitStatus());
}

}  // namespace utils
}  // namespace chaos

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    folly::init(&argc, &argv, true);
    google::SetStderrLogging(google::INFO);
    return RUN_ALL_TESTS();
}
