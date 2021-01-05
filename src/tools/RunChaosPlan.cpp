/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#include "common/base/Base.h"
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <folly/init/Init.h>
#include "nebula/NebulaChaosPlan.h"
#include "nebula/NebulaUtils.h"

DEFINE_string(instance_conf_file, "", "The json path of the instance conf file");
DEFINE_string(action_conf_file, "", "The json path of the action conf file");
DEFINE_string(flow_chart_script, "", "python script to generate flow chart of json");

namespace chaos {
namespace nebula_chaos {

int run() {
    std::string flowChart;
    if (!FLAGS_flow_chart_script.empty()) {
        folly::Subprocess proc(
            std::vector<std::string>{"/bin/python3", FLAGS_flow_chart_script,
                                     FLAGS_action_conf_file},
            folly::Subprocess::Options().pipeStdout());
        auto p = proc.communicate();
        if (proc.wait().exitStatus() == 0) {
            flowChart = folly::trimWhitespace(p.first).str();
            LOG(INFO) << "Flow chart is " << flowChart;
        }
    }
    try {
        auto plan = NebulaChaosPlan::loadFromFile(FLAGS_instance_conf_file, FLAGS_action_conf_file);
        if (plan == nullptr) {
            return 0;
        }
        plan->setAttachment(flowChart);
        LOG(INFO) << "\n============= run the plan =================\n";
        plan->schedule();
        LOG(INFO) << "\n" << plan->toString();
        plan->getGraphClient()->disconnect();
        return 0;
    } catch (const std::out_of_range& e) {
        LOG(ERROR) << "Load plan failed, err " << e.what();
        return -1;
    }
}

}  // namespace nebula_chaos
}  // namespace chaos

int main(int argc, char** argv) {
    folly::init(&argc, &argv, true);
    google::SetStderrLogging(google::INFO);
    return chaos::nebula_chaos::run();
}
