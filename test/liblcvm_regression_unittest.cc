// liblcvm regression tests.
//
// Each test parses a (malformed) file that used to trigger a memory-safety
// bug or undefined behavior. The files in test/regression/ come from
// fuzzing liblcvm (libFuzzer, ASan/UBSan). Build with
// -fsanitize=address,undefined to catch regressions that the expectations
// below cannot observe directly.

#include <gtest/gtest.h>

#include <cmath>
#include <memory>
#include <string>

#include "liblcvm.h"

class LiblcvmRegressionTest : public ::testing::Test {
 public:
  LiblcvmRegressionTest() {}
  ~LiblcvmRegressionTest() override {}

  std::shared_ptr<IsobmffFileInformation> parse(const std::string& filename,
                                                bool sort_by_pts,
                                                bool skip_audio_parsing) {
    std::string infile = std::string(TEST_REGRESSION_DIR) + "/" + filename;
    LiblcvmConfig liblcvm_config;
    liblcvm_config.set_sort_by_pts(sort_by_pts);
    liblcvm_config.set_skip_audio_parsing(skip_audio_parsing);
    return IsobmffFileInformation::parse(infile.c_str(), liblcvm_config);
  }

  int parse_to_lists(const std::string& filename, bool sort_by_pts,
                     bool skip_audio_parsing) {
    std::string infile = std::string(TEST_REGRESSION_DIR) + "/" + filename;
    LiblcvmConfig liblcvm_config;
    liblcvm_config.set_sort_by_pts(sort_by_pts);
    liblcvm_config.set_skip_audio_parsing(skip_audio_parsing);
    LiblcvmKeyList keys;
    LiblcvmValList vals;
    LiblcvmKeyList keys_timing;
    LiblcvmTimingList vals_timing;
    return IsobmffFileInformation::parse_to_lists(
        infile.c_str(), liblcvm_config, &keys, &vals, true, &keys_timing,
        &vals_timing);
  }
};

// The ctts box describes more samples than the stts box. The pts update
// loop used to write past the end of pts_unit_list/pts_sec_list
// (heap-buffer-overflow). The extra ctts samples are now ignored.
TEST_F(LiblcvmRegressionTest, TestCttsMoreSamplesThanStts) {
  for (bool sort_by_pts : {true, false}) {
    for (bool skip_audio_parsing : {true, false}) {
      auto ptr = parse("ctts_more_samples_than_stts.mov", sort_by_pts,
                       skip_audio_parsing);
      ASSERT_NE(nullptr, ptr);
      const auto timing = ptr->get_timing();
      EXPECT_EQ(timing.get_pts_unit_list().size(),
                static_cast<size_t>(timing.get_num_video_frames()));
      EXPECT_EQ(timing.get_pts_sec_list().size(),
                static_cast<size_t>(timing.get_num_video_frames()));
      // no ctts value beyond the last frame
      EXPECT_LE(timing.get_ctts_unit_list().size(),
                timing.get_pts_unit_list().size());
      parse_to_lists("ctts_more_samples_than_stts.mov", sort_by_pts,
                     skip_audio_parsing);
    }
  }
}

// A large dts plus a ctts offset exceeded the int32_t range of the pts
// value (signed integer overflow, undefined behavior). The addition now
// wraps around. Only observable with UBSan.
TEST_F(LiblcvmRegressionTest, TestCttsPtsInt32Overflow) {
  for (bool sort_by_pts : {true, false}) {
    for (bool skip_audio_parsing : {true, false}) {
      parse("ctts_pts_int32_overflow.mov", sort_by_pts, skip_audio_parsing);
      parse_to_lists("ctts_pts_int32_overflow.mov", sort_by_pts,
                     skip_audio_parsing);
    }
  }
}

// The ctts box describes fewer samples than the stts box (the last ctts
// offset applies to the remaining frames). ctts_unit_list only got the
// explicit entries, and sorting by pts indexed it with the (larger) frame
// numbers, reading past its end (heap-buffer-overflow). ctts_unit_list now
// has one entry per frame.
TEST_F(LiblcvmRegressionTest, TestCttsFewerSamplesThanStts) {
  for (bool sort_by_pts : {true, false}) {
    for (bool skip_audio_parsing : {true, false}) {
      auto ptr = parse("ctts_fewer_samples_than_stts.mov", sort_by_pts,
                       skip_audio_parsing);
      ASSERT_NE(nullptr, ptr);
      const auto timing = ptr->get_timing();
      const size_t num_frames = timing.get_pts_unit_list().size();
      EXPECT_EQ(num_frames, static_cast<size_t>(timing.get_num_video_frames()));
      EXPECT_EQ(timing.get_ctts_unit_list().size(), num_frames);
      EXPECT_EQ(timing.get_stts_unit_list().size(), num_frames);
      EXPECT_EQ(timing.get_dts_sec_list().size(), num_frames);
      EXPECT_EQ(timing.get_pts_sec_list().size(), num_frames);
      EXPECT_EQ(timing.get_frame_num_orig_list().size(), num_frames);
      parse_to_lists("ctts_fewer_samples_than_stts.mov", sort_by_pts,
                     skip_audio_parsing);
    }
  }
}
