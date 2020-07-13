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
    try {
        auto plan = NebulaChaosPlan::loadFromFile(FLAGS_conf_file);
        if (plan == nullptr) {
            return 0;
        }
        LOG(INFO) << "\n============= run the plan =================\n";
        plan->schedule();
        LOG(INFO) << "\n" << plan->toString();
        return 0;
    } catch (const std::out_of_range& e) {
        LOG(ERROR) << "Load plan failed, err " << e.what();
        return-1;
    }
}

}  // namespace nebula
}  // namespace nebula_chaos

int main(int argc, char** argv) {
    folly::init(&argc, &argv, true);
    google::SetStderrLogging(google::INFO);
    return nebula_chaos::nebula::run();
}
