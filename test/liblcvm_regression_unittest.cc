// liblcvm regression tests.
//
// Each test parses a (malformed) file that used to trigger a memory-safety
// bug or undefined behavior. The files in test/regression/ come from
// fuzzing liblcvm (libFuzzer, ASan/UBSan). Build with
// -fsanitize=address,undefined to catch regressions that the expectations
// below cannot observe directly.

#include <gtest/gtest.h>

#include <chrono>
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

// No usable inter-frame duration: frame_drop_ratio is 0/0 (NaN), and
// frame_drop_count converted it to int (undefined behavior; INT_MIN on
// x86). frame_drop_count is now 0 when the ratio is not finite.
TEST_F(LiblcvmRegressionTest, TestFrameDropRatioNan) {
  for (bool sort_by_pts : {true, false}) {
    for (bool skip_audio_parsing : {true, false}) {
      auto ptr =
          parse("frame_drop_ratio_nan.mov", sort_by_pts, skip_audio_parsing);
      ASSERT_NE(nullptr, ptr);
      const auto timing = ptr->get_timing();
      EXPECT_FALSE(std::isfinite(timing.get_frame_drop_ratio()));
      EXPECT_EQ(timing.get_frame_drop_count(), 0);
      parse_to_lists("frame_drop_ratio_nan.mov", sort_by_pts,
                     skip_audio_parsing);
    }
  }
}

// Consecutive pts values far apart in the int32_t range: the pts delta
// overflowed (signed integer overflow, undefined behavior). Only
// observable with UBSan.
TEST_F(LiblcvmRegressionTest, TestPtsDeltaInt32Overflow) {
  for (bool sort_by_pts : {true, false}) {
    for (bool skip_audio_parsing : {true, false}) {
      auto ptr = parse("pts_delta_int32_overflow.mov", sort_by_pts,
                       skip_audio_parsing);
      ASSERT_NE(nullptr, ptr);
      const auto timing = ptr->get_timing();
      // one inter-frame duration per pair of consecutive frames
      ASSERT_GT(timing.get_pts_unit_list().size(), 0u);
      EXPECT_EQ(timing.get_pts_duration_sec_list().size(),
                timing.get_pts_unit_list().size() - 1);
      parse_to_lists("pts_delta_int32_overflow.mov", sort_by_pts,
                     skip_audio_parsing);
    }
  }
}

// The stts box declares billions of frames in a 17 KB file. Every frame
// adds an entry to several per-frame lists, so parsing used to allocate
// until it ran out of memory. Files declaring more than max_num_frames
// frames (default 2^20) are now rejected before any allocation.
TEST_F(LiblcvmRegressionTest, TestSttsHugeSampleCount) {
  for (bool sort_by_pts : {true, false}) {
    for (bool skip_audio_parsing : {true, false}) {
      auto start = std::chrono::steady_clock::now();
      auto ptr =
          parse("stts_huge_sample_count.mov", sort_by_pts, skip_audio_parsing);
      EXPECT_EQ(nullptr, ptr);
      EXPECT_NE(0, parse_to_lists("stts_huge_sample_count.mov", sort_by_pts,
                                  skip_audio_parsing));
      auto elapsed = std::chrono::steady_clock::now() - start;
      EXPECT_LT(
          std::chrono::duration_cast<std::chrono::seconds>(elapsed).count(), 5);
    }
  }
}
