#include "common/Base.h"
#include <gtest/gtest.h>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <folly/init/Init.h>
#include "nebula/NebulaAction.h"
#include "nebula/NebulaChaosPlan.h"
#include "core/WaitAction.h"
#include "nebula/NebulaUtils.h"

DEFINE_string(user, "heng", "");
DEFINE_string(host, "192.168.8.210", "");
DEFINE_string(install_path, "/home/vesoft/prog/heng", "");
DEFINE_int64(total_rows, 10000, "");
DEFINE_int32(loop_times, 10, "");

namespace nebula_chaos {
namespace nebula {

NebulaInstance instance(NebulaInstance::Type type, const std::string& conf) {
    NebulaInstance inst(FLAGS_host, FLAGS_install_path, type, conf, FLAGS_user);
    inst.init();
    return inst;
}

/**
The plan looks like this:

*/
TEST(NebulaChaosTest, PlanTest) {
    PlanContext ctx;
    ctx.metads.emplace_back(instance(NebulaInstance::Type::META,
                                     folly::stringPrintf("%s/conf/nebula-metad.conf",
                                                         FLAGS_install_path.c_str())));

    for (int i = 1; i <= 3; i++) {
        ctx.storageds.emplace_back(instance(NebulaInstance::Type::STORAGE,
                                            folly::stringPrintf("%s/conf/nebula-storaged-%d.conf",
                                                                FLAGS_install_path.c_str(),
                                                                i)));
    }
    ctx.graphd = instance(NebulaInstance::Type::GRAPH,
                          folly::stringPrintf("%s/conf/nebula-graphd.conf",
                                              FLAGS_install_path.c_str()));

    ctx.space = "chaos";
    ctx.replica = 3;
    ctx.partsNum = 100;
    ctx.tag = Utils::getOperatingTable("circle");
    ctx.props.emplace_back("nextId", "int");

    std::unique_ptr<NebulaChaosPlan> plan(new NebulaChaosPlan(&ctx, 5));
    LOG(INFO) << "Start the cluster...";
    auto* action = plan->addStartActions();
    LOG(INFO) << "Create space and the schema...";
    plan->createSpaceAndSchema(plan->last());
    LOG(INFO) << "Balance and check the leaders...";
    auto* last = plan->balanceAndCheck(plan->last(), 100);
    LOG(INFO) << "The last action for current plan is " << action->toString();
    auto* client = plan->getGraphClient();
    /**
     * write - read flow and random flow will join on this node.
     * */
    auto joinNode = std::make_unique<core::EmptyAction>("Join Node");
    // Add randomRestartAction
    {
        std::vector<NebulaInstance*> storageds;
        for (auto& inst : ctx.storageds) {
            storageds.emplace_back(&inst);
        }
        auto randomRestartAction = std::make_unique<RandomRestartAction>(storageds,
                                                                         FLAGS_loop_times,
                                                                         30,
                                                                         30,
                                                                         false);
        last->addDependency(randomRestartAction.get());
        plan->addAction(std::move(randomRestartAction));
        auto* bcNode = plan->balanceAndCheck(plan->last(), 100);
        bcNode->addDependency(joinNode.get());
    }
    // Begin write data
    {
        auto writeAction = std::make_unique<WriteCircleAction>(client,
                                                               ctx.tag,
                                                               ctx.props[0].first,
                                                               FLAGS_total_rows);
        last->addDependency(writeAction.get());
        plan->addAction(std::move(writeAction));
    }
    // Begin walk through
    {
        auto walkAction = std::make_unique<WalkThroughAction>(client,
                                                              ctx.tag,
                                                              ctx.props[0].first,
                                                              FLAGS_total_rows);
        plan->last()->addDependency(walkAction.get());
        plan->addAction(std::move(walkAction));
        plan->last()->addDependency(joinNode.get());
    }
    plan->addAction(std::move(joinNode));

    // Stop all services
    plan->addStopActions(plan->last());

    LOG(INFO) << "=================== Now let's schedule the plan =========================";
    plan->schedule();

    LOG(INFO) << "=================== Finish the plan =========================";
    LOG(INFO) << "\n" << plan->toString();
}

}  // namespace utils
}  // namespace nebula_chaos

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    folly::init(&argc, &argv, true);
    google::SetStderrLogging(google::INFO);
    return RUN_ALL_TESTS();
}
