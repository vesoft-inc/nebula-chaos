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

TEST(CommunicateSubprocessTest, TakeOwnershipOfPipes) {
    std::vector<folly::Subprocess::ChildPipe> pipes;
    {
        folly::Subprocess proc(
            std::vector<std::string>{"/bin/sh", "-c", "echo $'oh\\nmy\\ncat' | wc -l &"},
            folly::Subprocess::Options().pipeStdout());
        pipes = proc.takeOwnershipOfPipes();
        proc.waitChecked();
    }
    EXPECT_EQ(1, pipes.size());
    EXPECT_EQ(1, pipes[0].childFd);

    char buf[10];
    EXPECT_EQ(2, folly::readFull(pipes[0].pipe.fd(), buf, 10));
    buf[2] = 0;
    EXPECT_EQ("3\n", std::string(buf));
}

TEST(PythonTest, PythonTest) {
    folly::Subprocess proc(std::vector<std::string>{"/bin/python3", "-c", "print('hello world')"},
                           folly::Subprocess::Options().pipeStdout());
    auto p = proc.communicate();
    EXPECT_EQ("hello world\n", p.first);
    proc.waitChecked();
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    folly::init(&argc, &argv, true);
    google::SetStderrLogging(google::INFO);
    return RUN_ALL_TESTS();
}
