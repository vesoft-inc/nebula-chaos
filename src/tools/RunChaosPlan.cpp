#include "common/Base.h"
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <folly/init/Init.h>
#include "nebula/NebulaChaosPlan.h"
#include "nebula/NebulaUtils.h"

DEFINE_string(conf_file, "", "");

namespace nebula_chaos {
namespace nebula {

int run() {
    auto plan = NebulaChaosPlan::loadFromFile(FLAGS_conf_file);
    LOG(INFO) << "\n============= run the plan =================\n";
    plan->schedule();
    LOG(INFO) << "\n" << plan->toString();
    return 1;
}

}  // namespace nebula
}  // namespace nebula_chaos

int main(int argc, char** argv) {
    folly::init(&argc, &argv, true);
    google::SetStderrLogging(google::INFO);
    return nebula_chaos::nebula::run();
}
