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
std::string readFileToString(const std::string& filename) {
  std::ifstream file(filename, std::ios::in | std::ios::binary);
  if (!file) {
    throw std::runtime_error("Could not open file: " + filename);
  }

  std::ostringstream contents;
  contents << file.rdbuf();
  return contents.str();
}
}  // namespace
#endif

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
  std::string policy = readFileToString(policy_infile);
#endif

  // 2. set parsing parameters
  auto liblcvm_config = std::make_unique<LiblcvmConfig>();
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
      "type",
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
      std::string("Suspicious bitrate_bps;Suspicious depth"),
      std::string("Invalid bitrate_bps;Invalid width;Invalid height;Invalid "
                  "type;Invalid audio_video_ratio"),
#endif
  };
  EXPECT_THAT(expected_vals, ElementsAreArray(vals.begin() + 1, vals.end()))
      << "output vals incorrect";
}
}  // namespace liblcvm
