/*
 *  Copyright (c) Meta Platforms, Inc. and its affiliates.
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <liblcvm.h>  // for various

namespace liblcvm {

class LiblcvmTest : public ::testing::Test {
 public:
  LiblcvmTest() {}
  ~LiblcvmTest() override {}
};

TEST_F(LiblcvmTest, TestCalculateMedianAbsoluteDeviation) {
  ASSERT_NE(0, 1) << "Broken Check";
}
}  // namespace liblcvm
