#include "common/Base.h"
#include <gtest/gtest.h>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <folly/init/Init.h>
#include "nebula/NebulaAction.h"
#include "nebula/NebulaChaosPlan.h"
#include "core/WaitAction.h"

DEFINE_string(user, "heng", "");
DEFINE_string(host, "192.168.8.210", "");
DEFINE_string(install_path, "/home/vesoft/prog/heng", "");
DEFINE_int64(total_rows, 10000, "");

namespace nebula_chaos {
namespace nebula {

NebulaInstance instance(NebulaInstance::Type type, const std::string& conf) {
    NebulaInstance inst(FLAGS_host, FLAGS_install_path, type, conf, FLAGS_user);
    inst.init();
    return inst;
}

/**
The plan looks like this:
                             +------------------+
                             | Start Metad      |
            +----------------+-----+------------+---------------+
            |                      |                            |
            |                      |                            |
            |                      |                            |
            |                      |                            |
            |                      |                            |
   +--------v---------+      +-----v------------+      +--------v--------+
   | Start Storaged-1 |      |  Start Storage+2 |      | Start Storage-3 |
   |                  |      |                  |      |                 |
   +------------------+      +-----+------------+      +-----------------+
                                   |
                                   |
                                   |
                                   |
                             +-----v------------+
                             | Start Graphd     |
                             +-----+------------+
                                   |
                                   |
                                   |
                                   |
                                   |
                             +-----v------------+----------------------------------+
        +--------------------+ Wait 10S         +----------------+                 |
        |                    ++-------------+---+                |                 |
        |                     |             |                    |                 |
        |                     |             |                    |                 |
+-------v----+    +-----------v--+   +------v---------+   +------v---------+ +-----v--------+
| Stop metad |    |Stop Storage-1|   | Stop Storaged-2|   | Stop Storaged-3| | Stop Graphd  |
|            |    |              |   |                |   |                | |              |
+------------+    +--------------+   +----------------+   +----------------+ +--------------+
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
    ctx.tag = "circle";
    ctx.props.emplace_back("nextId", "int");

    std::unique_ptr<NebulaChaosPlan> plan(new NebulaChaosPlan(&ctx, 5));
    auto* action = plan->addStartActions();
    LOG(INFO) << "The last action for current plan is " << action->toString();
    // wait 10s
    {
        auto waitAction = std::make_unique<core::WaitAction>(10000);
        action->addDependency(waitAction.get());
        plan->addAction(std::move(waitAction));
    }
    auto* last = plan->createSpaceAndSchema(plan->last());
    // wait 10s
    {
        auto waitAction = std::make_unique<core::WaitAction>(10000);
        last->addDependency(waitAction.get());
        plan->addAction(std::move(waitAction));
    }
    // Begin write data
    {
        auto* client = plan->getGraphClient();
        auto writeAction = std::make_unique<WriteCircleAction>(client,
                                                               ctx.tag,
                                                               ctx.props[0].first,
                                                               FLAGS_total_rows);
        plan->last()->addDependency(writeAction.get());
        plan->addAction(std::move(writeAction));
    }
    // Begin walk through
    {
        auto* client = plan->getGraphClient();
        auto walkAction = std::make_unique<WalkThroughAction>(client,
                                                              ctx.tag,
                                                              ctx.props[0].first,
                                                              FLAGS_total_rows);
        plan->last()->addDependency(walkAction.get());
        plan->addAction(std::move(walkAction));
    }

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
