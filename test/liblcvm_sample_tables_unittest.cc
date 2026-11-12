// liblcvm sample table consistency tests.
//
// liblcvm checks the sample tables of every video and audio track before
// expanding them into per-frame lists (see validate_sample_tables() in
// src/liblcvm.cc): exactly one stsz/stz2, stts total == sample count, a
// constant-size video stsz that fits in the file, a ctts no longer than the
// sample count, and valid, strictly increasing stss sample numbers. The
// files come from test/regression/create_sample_table_fixtures.py.

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "liblcvm.h"

namespace {

std::shared_ptr<IsobmffFileInformation> Parse(const std::string& filename,
                                              bool skip_audio_parsing) {
  std::string infile = std::string(TEST_REGRESSION_DIR) + "/" + filename;
  LiblcvmConfig liblcvm_config;
  liblcvm_config.set_skip_audio_parsing(skip_audio_parsing);
  return IsobmffFileInformation::parse(infile.c_str(), liblcvm_config);
}

void ExpectValid(const std::string& filename) {
  for (bool skip_audio_parsing : {true, false}) {
    auto ptr = Parse(filename, skip_audio_parsing);
    ASSERT_NE(nullptr, ptr) << filename;
    const auto timing = ptr->get_timing();
    EXPECT_EQ(4, timing.get_num_frames()) << filename;
    EXPECT_EQ(4u, timing.get_pts_unit_list().size()) << filename;
  }
}

void ExpectInvalid(const std::string& filename) {
  for (bool skip_audio_parsing : {true, false}) {
    EXPECT_EQ(nullptr, Parse(filename, skip_audio_parsing)) << filename;
  }
}

}  // namespace

TEST(LiblcvmSampleTablesTest, ValidSampleSizeBoxes) {
  ExpectValid("sample_tables_valid_stsz.mp4");
  ExpectValid("sample_tables_valid_stz2_4.mp4");
  ExpectValid("sample_tables_valid_stz2_8.mp4");
  ExpectValid("sample_tables_valid_stz2_16.mp4");
  // 4 samples of 1 byte fit in the file
  ExpectValid("sample_tables_valid_constant_size.mp4");
}

// Section 8.7.3.1: exactly one of stsz or stz2.
TEST(LiblcvmSampleTablesTest, SampleSizeBoxCount) {
  ExpectInvalid("sample_tables_no_stsz.mp4");
  ExpectInvalid("sample_tables_stsz_and_stz2.mp4");
  ExpectInvalid("sample_tables_two_stsz.mp4");
}

// Section 8.7.3.1: the stts total is the sample count.
TEST(LiblcvmSampleTablesTest, SttsTotalMatchesSampleCount) {
  ExpectInvalid("sample_tables_stts_more.mp4");
  ExpectInvalid("sample_tables_stts_fewer.mp4");
}

// The stts check also applies to audio tracks, unless audio parsing is
// skipped (then the audio track is not read at all).
TEST(LiblcvmSampleTablesTest, AudioSttsTotalMatchesSampleCount) {
  EXPECT_EQ(nullptr, Parse("sample_tables_audio_stts_more.mp4", false));
  EXPECT_NE(nullptr, Parse("sample_tables_audio_stts_more.mp4", true));
}

// A constant-size stsz must fit in the file for video tracks, but not for
// audio tracks (legacy QuickTime audio uses sample_size = 1 per PCM sample).
TEST(LiblcvmSampleTablesTest, ConstantSampleSizeBound) {
  ExpectInvalid("sample_tables_constant_size_too_large.mp4");
  ExpectValid("sample_tables_valid_audio_constant_size.mp4");
}

// Section 8.6.1.3.1: one composition offset per sample. A shorter ctts is
// accepted (its last offset applies to the remaining samples); a longer one
// is not.
TEST(LiblcvmSampleTablesTest, CttsTotal) {
  ExpectValid("sample_tables_valid_ctts_fewer.mp4");
  auto ptr = Parse("sample_tables_valid_ctts_fewer.mp4", false);
  ASSERT_NE(nullptr, ptr);
  EXPECT_EQ(4u, ptr->get_timing().get_ctts_unit_list().size());
  ExpectInvalid("sample_tables_ctts_more.mp4");
}

// Section 8.6.2.1 and 3.1.49: stss sample numbers are in 1..sample_count,
// in strictly increasing order.
TEST(LiblcvmSampleTablesTest, SyncSampleNumbers) {
  ExpectInvalid("sample_tables_stss_zero.mp4");
  ExpectInvalid("sample_tables_stss_past_end.mp4");
  ExpectInvalid("sample_tables_stss_not_increasing.mp4");
  ExpectInvalid("sample_tables_stss_repeated.mp4");
}
