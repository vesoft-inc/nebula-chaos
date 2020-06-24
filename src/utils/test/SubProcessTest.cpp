#include <gtest/gtest.h>
#include <glog/logging.h>
#include <folly/init/Init.h>
#include <folly/Subprocess.h>
#include <folly/gen/Base.h>
#include <folly/gen/File.h>
#include <folly/gen/String.h>

TEST(SimpleSubprocessTest, ExitsSuccessfully) {
    folly::Subprocess proc(std::vector<std::string>{"/bin/true"});
    EXPECT_EQ(0, proc.wait().exitStatus());
}

TEST(PopenSubprocessTest, PopenRead) {
    folly::Subprocess proc({"/bin/sh", "-c", "ls /"},
                           folly::Subprocess::Options().pipeStdout());
    auto p = proc.communicate();
    VLOG(1) << "The output multi lines string is " << p.first;
    int found = 0;
    int lineNo = 0;
    folly::gen::lines(p.first) | [&](folly::StringPiece line) {
        VLOG(1) << lineNo++ << ":" << line;
        if (line == "etc" || line == "bin" || line == "usr") {
            ++found;
        }
    };
    EXPECT_EQ(3, found);
    proc.waitChecked();
}

TEST(CommunicateSubprocessTest, SimpleRead) {
    folly::Subprocess proc(
        std::vector<std::string>{"/bin/echo", "-n", "foo", "bar"},
        folly::Subprocess::Options().pipeStdout());
    auto p = proc.communicate();
    EXPECT_EQ("foo bar", p.first);
    proc.waitChecked();
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    folly::init(&argc, &argv, true);
    google::SetStderrLogging(google::INFO);
    return RUN_ALL_TESTS();
}
