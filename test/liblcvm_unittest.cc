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

using ::testing::ElementsAreArray;

#if ADD_POLICY
namespace {
int readFileToString(const std::string& filename, std::string* result) {
  std::ifstream file(filename, std::ios::in | std::ios::binary);
  if (!file) {
    return -1;  // Error: Could not open file
  }

  std::ostringstream contents;
  contents << file.rdbuf();
  *result = contents.str();
  return 0;
}
}  // namespace
#endif

namespace {
// Compare two doubles with relative error tolerance
// Returns true if the ratio of their difference vs. the larger value is smaller
// than the accuracy parameter
bool doubles_are_close(double a, double b, double accuracy = 0.0001) {
  // Handle exact equality (including both zero)
  if (a == b) {
    return true;
  }

  // Handle the case where one or both values are zero
  // In this case, use absolute difference instead of relative
  double max_abs = std::max(std::abs(a), std::abs(b));
  if (max_abs == 0.0) {
    return true;  // Both are zero
  }

  // Calculate relative error: |a - b| / max(|a|, |b|)
  double diff = std::abs(a - b);
  double relative_error = diff / max_abs;

  return relative_error < accuracy;
}

// Compare a LiblcvmValue against an expected value with optional tolerance for
// doubles
bool values_are_close(const LiblcvmValue& actual, const LiblcvmValue& expected,
                      double accuracy = 0.0001) {
  // If both are doubles, use double comparison with tolerance
  if (std::holds_alternative<double>(actual) &&
      std::holds_alternative<double>(expected)) {
    return doubles_are_close(std::get<double>(actual),
                             std::get<double>(expected), accuracy);
  }

  // For all other types, convert to string and compare exactly
  std::string actual_str;
  std::string expected_str;
  if (liblcvmvalue_to_string(actual, &actual_str) != 0 ||
      liblcvmvalue_to_string(expected, &expected_str) != 0) {
    return false;
  }

  return actual_str == expected_str;
}
}  // namespace

namespace liblcvm {

class LiblcvmTest : public ::testing::Test {
 public:
  LiblcvmTest() {}
  ~LiblcvmTest() override {}
};

TEST_F(LiblcvmTest, TestParserPolicy) {
  // 1. set input files
  std::string input_filename = "MOV1.MOV";
  std::string infile = std::string(TEST_MEDIA_DIR) + "/" + input_filename;
#if ADD_POLICY
  std::string policy_filename = "example.txt";
  std::string policy_infile =
      std::string(TEST_POLICY_DIR) + "/" + policy_filename;
  std::string policy;
  if (readFileToString(policy_infile, &policy) != 0) {
    policy = "";  // Use empty policy on error
  }
#endif

  // 2. set parsing parameters
  LiblcvmConfig* liblcvm_config = new LiblcvmConfig();
  liblcvm_config->set_sort_by_pts(true);
#if ADD_POLICY
  liblcvm_config->set_policy(policy);
#endif
  liblcvm_config->set_debug(1);

  // 3. parse the input files
  LiblcvmKeyList keys;
  LiblcvmValList vals;
  LiblcvmKeyList keys_timing;
  LiblcvmTimingList vals_timing;
  bool calculate_timestamps = true;
  ASSERT_EQ(0, IsobmffFileInformation::parse_to_lists(
                   infile.c_str(), *liblcvm_config, &keys, &vals,
                   calculate_timestamps, &keys_timing, &vals_timing))
      << "error: IsobmffFileInformation::parse_to_map() in " << infile;

  // 4. check the main parsing values
  LiblcvmKeyList expected_keys = {
      "infile",
      "filesize",
      "bitrate_bps",
      "width",
      "height",
      "video_codec_type",
      "horizresolution",
      "vertresolution",
      "depth",
      "chroma_format",
      "bit_depth_luma",
      "bit_depth_chroma",
      "video_full_range_flag",
      "colour_primaries",
      "transfer_characteristics",
      "matrix_coeffs",
      "profile_idc",
      "level_idc",
      "profile_type_str",
      "num_video_frames",
      "frame_rate_fps_median",
      "frame_rate_fps_average",
      "frame_rate_fps_reverse_average",
      "frame_rate_fps_stddev",
      "video_freeze",
      "audio_video_ratio",
      "duration_video_sec",
      "duration_audio_sec",
      "timescale_video_hz",
      "timescale_audio_hz",
      "pts_duration_sec_average",
      "pts_duration_sec_median",
      "pts_duration_sec_stddev",
      "pts_duration_sec_mad",
      "frame_drop_count",
      "frame_drop_ratio",
      "normalized_frame_drop_average_length",
      "frame_drop_length_percentile_50",
      "frame_drop_length_percentile_90",
      "frame_drop_length_consecutive_2",
      "frame_drop_length_consecutive_5",
      "num_video_keyframes",
      "key_frame_ratio",
      "audio_type",
      "channel_count",
      "sample_rate",
      "sample_size",
#if ADD_POLICY
      "policy_version",
      "warn_list",
      "error_list",
#endif
  };
  EXPECT_EQ(expected_keys, keys) << "output keys incorrect";
  // check the file name
  std::string fullpath = std::get<std::string>(vals[0]);
  std::filesystem::path p(fullpath);
  std::string filename = p.filename().string();
  std::string expected_filename = input_filename;
  EXPECT_EQ(expected_filename, filename) << "incorrect filename";
  // check the remaining values
  LiblcvmValList expected_vals = {
      17784,
      13455.737704918032,
      1920.0,
      1080.0,
      std::string("hvc1"),
      4718592,
      4718592,
      24,
      1,
      8,
      8,
      0,
      1,
      1,
      1,
      1,
      123,
      std::string("Main"),
      634,
      60.0,
      59.965530395507812,
      59.962047518336938,
      0.43256710446899582,
      0,
      1.0058076193907575,
      10.573333333333334,
      10.634739229024943,
      600u,
      44100u,
      0.016677215695381165,
      0.016666666666666666,
      0.000132173283037037,
      0.0,
      0,
      0.0,
      0.0,
      0.0,
      0.0,
      0l,
      0l,
      11,
      57.636363636363633,
      std::string("mp4a"),
      1,
      44100,
      16,
#if ADD_POLICY
      std::string("0.1"),
      std::string("Suspicious bitrate_bps too low (bitrate_bps: "
                  "13455.737705);Suspicious depth (depth: 24)"),
      std::string("Invalid bitrate_bps too low (bitrate_bps: "
                  "13455.737705);Invalid width (width: 1920.000000);Invalid "
                  "height (height: 1080.000000);Invalid "
                  "audio_video_ratio (audio_video_ratio: 1.005808)"),
#endif
  };
  for (unsigned int index = 0; index < expected_vals.size(); index++) {
    auto key = expected_keys[index + 1];
    EXPECT_TRUE(values_are_close(vals[index + 1], expected_vals[index]))
        << "index: " << index << " key: " << key << " expected: " <<
        [&]() {
          std::string s;
          liblcvmvalue_to_string(expected_vals[index], &s);
          return s;
        }()
        << " actual: " << [&]() {
             std::string s;
             liblcvmvalue_to_string(vals[index + 1], &s);
             return s;
           }();
  }
}
}  // namespace liblcvm
