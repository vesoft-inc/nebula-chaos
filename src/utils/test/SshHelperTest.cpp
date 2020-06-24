#include "common/Base.h"
#include <gtest/gtest.h>
#include <glog/logging.h>
#include <folly/init/Init.h>
#include <folly/Subprocess.h>
#include <folly/gen/Base.h>
#include <folly/gen/File.h>
#include <folly/gen/String.h>
#include "utils/SshHelper.h"

namespace nebula_chaos {
namespace utils {

TEST(SSHHelperTest, DateTest) {
    auto ret = SshHelper::run("date", "127.0.0.1");
    EXPECT_EQ(0, ret.exitStatus());
}

}  // namespace utils
}  // namespace nebula_chaos

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    folly::init(&argc, &argv, true);
    google::SetStderrLogging(google::INFO);
    return RUN_ALL_TESTS();
}
