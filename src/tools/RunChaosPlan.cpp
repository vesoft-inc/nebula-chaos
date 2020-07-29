#include "common/Base.h"
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <folly/init/Init.h>
#include "nebula/NebulaChaosPlan.h"
#include "nebula/NebulaUtils.h"
#include "boost/filesystem/operations.hpp"


DEFINE_string(conf_path, "", "plan conf path");
DEFINE_string(flow_chart_script, "", "python script to generate flow chart of json");

namespace nebula_chaos {
namespace nebula {

int run() {
    for (auto& entry : boost::filesystem::directory_iterator(FLAGS_conf_path)) {
        auto fileName = folly::stringPrintf("%s/%s",
                                            FLAGS_conf_path.c_str(),
                                            entry.path().filename().c_str());
        std::string flowChart;
        if (!FLAGS_flow_chart_script.empty()) {
            folly::Subprocess proc(
                std::vector<std::string>{"/bin/python3", FLAGS_flow_chart_script, fileName},
                folly::Subprocess::Options().pipeStdout());
            auto p = proc.communicate();
            if (proc.wait().exitStatus() == 0) {
                flowChart = folly::trimWhitespace(p.first).str();
                LOG(INFO) << "Flow chart is " << flowChart;
            }
        }
        try {
            LOG(INFO) << "\n============= run the plan " << fileName
                      << " =================\n";
            auto plan = NebulaChaosPlan::loadFromFile(fileName);
            if (plan == nullptr) {
                return 0;
            }
            plan->setAttachment(flowChart);
            plan->schedule();
            LOG(INFO) << "\n" << plan->toString();
        } catch (const std::out_of_range& e) {
            LOG(ERROR) << "Load plan failed, err " << e.what();
            return -1;
        }
    }
    return 0;
}

}  // namespace nebula
}  // namespace nebula_chaos

int main(int argc, char** argv) {
    folly::init(&argc, &argv, true);
    google::SetStderrLogging(google::INFO);
    return nebula_chaos::nebula::run();
}
