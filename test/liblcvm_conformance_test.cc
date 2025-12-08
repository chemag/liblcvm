/*
 *  Copyright (c) Meta Platforms, Inc. and its affiliates.
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <liblcvm.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace liblcvm {

class ConformanceTest : public ::testing::Test {
 public:
  ConformanceTest() {}
  ~ConformanceTest() override {}

  struct ExpectedMetrics {
    std::string infile;
    int num_video_frames;
    double frame_rate_fps_median;
    double frame_rate_fps_average;
    int video_freeze;
    double frame_drop_ratio;
    int num_video_keyframes;
    double key_frame_ratio;
    int width;
    int height;
    std::string video_codec_type;
  };

  bool compareDouble(double a, double b, double tolerance = 0.01) {
    return std::abs(a - b) < tolerance;
  }

  void runConformanceTest(const std::string& video_filename,
                          const ExpectedMetrics& expected) {
    std::string corpus_dir = std::string(TEST_CONFORMANCE_DIR) + "/corpus";
    std::string infile = corpus_dir + "/" + video_filename;

    if (!std::filesystem::exists(infile)) {
      GTEST_SKIP() << "Test video not found: " << infile
                   << ". Please add videos to the conformance corpus.";
    }

    auto liblcvm_config = std::make_unique<LiblcvmConfig>();
    liblcvm_config->set_sort_by_pts(true);
    liblcvm_config->set_debug(0);

    LiblcvmKeyList keys;
    LiblcvmValList vals;
    LiblcvmKeyList keys_timing;
    LiblcvmTimingList vals_timing;
    bool calculate_timestamps = true;

    int result = IsobmffFileInformation::parse_to_lists(
        infile.c_str(), *liblcvm_config, &keys, &vals, calculate_timestamps,
        &keys_timing, &vals_timing);

    ASSERT_EQ(0, result) << "Failed to parse video: " << infile;

    std::map<std::string, LiblcvmValue> metrics;
    for (size_t i = 0; i < keys.size(); i++) {
      metrics[keys[i]] = vals[i];
    }

    EXPECT_EQ(expected.num_video_frames,
              std::get<int>(metrics["num_video_frames"]))
        << "num_video_frames mismatch for " << video_filename;

    EXPECT_TRUE(
        compareDouble(expected.frame_rate_fps_median,
                      std::get<double>(metrics["frame_rate_fps_median"])))
        << "frame_rate_fps_median mismatch for " << video_filename
        << " expected: " << expected.frame_rate_fps_median
        << " got: " << std::get<double>(metrics["frame_rate_fps_median"]);

    EXPECT_TRUE(
        compareDouble(expected.frame_rate_fps_average,
                      std::get<double>(metrics["frame_rate_fps_average"])))
        << "frame_rate_fps_average mismatch for " << video_filename
        << " expected: " << expected.frame_rate_fps_average
        << " got: " << std::get<double>(metrics["frame_rate_fps_average"]);

    EXPECT_EQ(expected.video_freeze, std::get<int>(metrics["video_freeze"]))
        << "video_freeze mismatch for " << video_filename;

    EXPECT_TRUE(compareDouble(expected.frame_drop_ratio,
                              std::get<double>(metrics["frame_drop_ratio"])))
        << "frame_drop_ratio mismatch for " << video_filename
        << " expected: " << expected.frame_drop_ratio
        << " got: " << std::get<double>(metrics["frame_drop_ratio"]);

    EXPECT_EQ(expected.num_video_keyframes,
              std::get<int>(metrics["num_video_keyframes"]))
        << "num_video_keyframes mismatch for " << video_filename;

    EXPECT_TRUE(compareDouble(expected.key_frame_ratio,
                              std::get<double>(metrics["key_frame_ratio"])))
        << "key_frame_ratio mismatch for " << video_filename
        << " expected: " << expected.key_frame_ratio
        << " got: " << std::get<double>(metrics["key_frame_ratio"]);

    EXPECT_EQ(expected.width,
              static_cast<int>(std::get<double>(metrics["width"])))
        << "width mismatch for " << video_filename;

    EXPECT_EQ(expected.height,
              static_cast<int>(std::get<double>(metrics["height"])))
        << "height mismatch for " << video_filename;

    EXPECT_EQ(expected.video_codec_type,
              std::get<std::string>(metrics["video_codec_type"]))
        << "video_codec_type mismatch for " << video_filename;
  }
};

TEST_F(ConformanceTest, MOV1_Conformance) {
  ExpectedMetrics expected;
  expected.infile = "MOV1.MOV";
  expected.num_video_frames = 634;
  expected.frame_rate_fps_median = 60.0;
  expected.frame_rate_fps_average = 59.965530395507812;
  expected.video_freeze = 0;
  expected.frame_drop_ratio = 0.0;
  expected.num_video_keyframes = 11;
  expected.key_frame_ratio = 57.636363636363633;
  expected.width = 1920;
  expected.height = 1080;
  expected.video_codec_type = "hvc1";

  runConformanceTest("MOV1.MOV", expected);
}

}  // namespace liblcvm
