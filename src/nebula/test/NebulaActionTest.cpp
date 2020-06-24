#include "common/Base.h"
#include <gtest/gtest.h>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <folly/init/Init.h>
#include "nebula/NebulaAction.h"

DEFINE_string(user, "heng", "");
DEFINE_string(host, "192.168.8.210", "");
DEFINE_string(install_path, "/home/vesoft/prog/heng", "");
DEFINE_string(conf_path, "/home/vesoft/prog/heng/conf/nebula-storaged-1.conf", "");

namespace nebula_chaos {
namespace nebula {

TEST(NebulaActionTest, ActionsTest) {
    NebulaInstance instance(FLAGS_host,
                            FLAGS_install_path,
                            NebulaInstance::Type::STORAGE,
                            FLAGS_conf_path,
                            FLAGS_user);
    CHECK(instance.init());

    {
        StartAction start(&instance);
        LOG(INFO) << start.toString();
        start.run();
    }
    sleep(3);
    {
        StopAction stop(&instance);
        LOG(INFO) << stop.toString();
        stop.run();
    }
    sleep(3);
    {
        StartAction start(&instance);
        LOG(INFO) << start.toString();
        start.run();
    }
    sleep(3);
    {
        CrashAction crash(&instance);
        LOG(INFO) << crash.toString();
        crash.run();
    }
}

}  // namespace utils
}  // namespace nebula_chaos

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    folly::init(&argc, &argv, true);
    google::SetStderrLogging(google::INFO);
    return RUN_ALL_TESTS();
}
