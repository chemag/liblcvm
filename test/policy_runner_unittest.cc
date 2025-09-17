/*
 *  Copyright (c) Meta Platforms, Inc. and its affiliates.
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <liblcvm.h>  // for various

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::StrEq;

namespace liblcvm {

class PolicyRunnerTest : public ::testing::Test {
 public:
  PolicyRunnerTest() {}
  ~PolicyRunnerTest() override {}
};

TEST_F(PolicyRunnerTest, TestComparisons) {
  // 1. set input jeys/vals
  LiblcvmKeyList keys = {
      "int",
      "string",
      "double",
  };
  LiblcvmValList vals = {
      1,
      std::string("hello"),
      1.0,
  };
  std::list<std::string> warn_list;
  std::list<std::string> error_list;
  std::string version;

  // 2. int comparisons
  std::string policy_str_int_1 = "error \"Invalid int\" int > 5";
  ASSERT_EQ(0, policy_runner(policy_str_int_1, &keys, &vals, &warn_list,
                             &error_list, &version));
  EXPECT_TRUE(warn_list.empty());
  EXPECT_TRUE(error_list.empty());

  std::string policy_str_int_2 = "error \"Invalid int\" int < 5";
  ASSERT_EQ(0, policy_runner(policy_str_int_2, &keys, &vals, &warn_list,
                             &error_list, &version));
  EXPECT_TRUE(warn_list.empty());
  EXPECT_THAT(error_list, ElementsAre(StrEq("Invalid int")));

  // 3. string comparisons
  std::string policy_str_string_1 = "warn \"Invalid str\" string == \"hello\"";
  ASSERT_EQ(0, policy_runner(policy_str_string_1, &keys, &vals, &warn_list,
                             &error_list, &version));
  EXPECT_THAT(warn_list, ElementsAre(StrEq("Invalid str")));
  EXPECT_TRUE(error_list.empty());

  std::string policy_str_string_2 = "warn \"Invalid str\" string != \"hello\"";
  ASSERT_EQ(0, policy_runner(policy_str_string_2, &keys, &vals, &warn_list,
                             &error_list, &version));
  EXPECT_TRUE(warn_list.empty());
  EXPECT_TRUE(error_list.empty());

  std::string policy_str_string_3 = "error \"Invalid string\" string > \"5\"";
  ASSERT_EQ(1, policy_runner(policy_str_string_3, &keys, &vals, &warn_list,
                             &error_list, &version));
  EXPECT_TRUE(warn_list.empty());
  EXPECT_TRUE(error_list.empty());

  // 3. double comparisons
  std::string policy_str_double_1 = "error \"Invalid double\" double > 5.0";
  ASSERT_EQ(0, policy_runner(policy_str_double_1, &keys, &vals, &warn_list,
                             &error_list, &version));
  EXPECT_TRUE(warn_list.empty());
  EXPECT_TRUE(error_list.empty());

  std::string policy_str_double_2 = "error \"Invalid double\" double < 5.0";
  ASSERT_EQ(0, policy_runner(policy_str_double_2, &keys, &vals, &warn_list,
                             &error_list, &version));
  EXPECT_TRUE(warn_list.empty());
  EXPECT_THAT(error_list, ElementsAre(StrEq("Invalid double")));
}
}  // namespace liblcvm
