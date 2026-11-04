// Independent-video parsing, selection, and exhaustive AV export regressions.
#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "liblcvm.h"

namespace {
auto ParseVideo(const std::string& name, bool skip = false,
                uint32_t max_frames = LiblcvmConfig::kDefaultMaxNumFrames,
                bool sort = true) {
  LiblcvmConfig config;
  config.set_skip_audio_parsing(skip);
  config.set_max_num_frames(max_frames);
  config.set_sort_by_pts(sort);
  return IsobmffFileInformation::parse(
      (std::string(TEST_REGRESSION_DIR) + "/" + name).c_str(), config);
}

std::map<std::string, LiblcvmValue> VideoFields(
    const std::shared_ptr<IsobmffFileInformation>& file, int64_t audio = -1,
    int64_t video = -1, bool expanded = true) {
  LiblcvmKeyList keys, tk;
  LiblcvmValList vals;
  LiblcvmTimingList tv;
  EXPECT_EQ(
      0, IsobmffFileInformation::LiblcvmConfig_to_lists(
             file, &keys, &vals, false, &tk, &tv, 0, audio, expanded, video));
  EXPECT_EQ(keys.size(), vals.size());
  std::map<std::string, LiblcvmValue> result;
  for (size_t i = 0; i < keys.size(); ++i)
    EXPECT_TRUE(result.emplace(keys[i], vals[i]).second);
  return result;
}

std::string ValueText(const LiblcvmValue& value) {
  std::string result;
  EXPECT_EQ(0, liblcvmvalue_to_string(value, &result));
  return result;
}
}  // namespace

TEST(LiblcvmMultivideoTest, EveryVideoRetainsIndependentDimensionsAndTimeline) {
  auto file = ParseVideo("multivideo_2v_2a.mp4");
  ASSERT_NE(nullptr, file);
  EXPECT_EQ((std::vector<uint32_t>{11, 21}), file->get_video_track_ids());
  EXPECT_EQ((std::vector<uint32_t>{31, 41}), file->get_audio_track_ids());
  EXPECT_EQ(2u, file->get_num_video_tracks());
  EXPECT_EQ(2u, file->get_num_audio_tracks());
  EXPECT_EQ(11u, file->get_video_info().get_track_id());
  EXPECT_DOUBLE_EQ(320, file->get_video_info().get_width());
  EXPECT_DOUBLE_EQ(640, file->get_video_info(21).get_width());
  EXPECT_DOUBLE_EQ(240, file->get_video_info(11).get_height());
  EXPECT_DOUBLE_EQ(360, file->get_video_info(21).get_height());
  auto first = file->get_timing(-1, 11), second = file->get_timing(-1, 21);
  EXPECT_EQ(120, first.get_num_video_frames());
  EXPECT_EQ(72, second.get_num_video_frames());
  EXPECT_EQ(30000u, first.get_timescale_video_hz());
  EXPECT_EQ(24000u, second.get_timescale_video_hz());
  EXPECT_DOUBLE_EQ(30, first.get_frame_rate_fps_median());
  EXPECT_DOUBLE_EQ(24, second.get_frame_rate_fps_median());
  EXPECT_DOUBLE_EQ(4, first.get_duration_video_sec());
  EXPECT_DOUBLE_EQ(3, second.get_duration_video_sec());
  EXPECT_EQ(2, first.get_num_video_keyframes());
  EXPECT_EQ(1, second.get_num_video_keyframes());
  EXPECT_EQ(120u, first.get_pts_sec_list().size());
  EXPECT_EQ(72u, second.get_pts_sec_list().size());
  EXPECT_EQ(120, file->get_timing().get_num_video_frames());
}

TEST(LiblcvmMultivideoTest, FirstVideoMeansContainerOrderNotLowestId) {
  auto file = ParseVideo("multivideo_reversed.mp4");
  ASSERT_NE(nullptr, file);
  EXPECT_EQ((std::vector<uint32_t>{21, 11}), file->get_video_track_ids());
  EXPECT_EQ(21u, file->get_video_info().get_track_id());
  EXPECT_EQ(72, file->get_timing().get_num_video_frames());
  EXPECT_DOUBLE_EQ(3, file->get_timing().get_duration_video_sec());
  EXPECT_DOUBLE_EQ(4.5, file->get_timing().get_duration_audio_sec());
  EXPECT_DOUBLE_EQ(1.5, file->get_timing().get_audio_video_ratio());
  EXPECT_EQ(120, file->get_timing(-1, 11).get_num_video_frames());
}

TEST(LiblcvmMultivideoTest, AudioFirstTimingArgumentsAndAllPairs) {
  auto file = ParseVideo("multivideo_2v_2a.mp4");
  ASSERT_NE(nullptr, file);
  // The old one-argument get_timing(audio_id) call keeps its audio meaning.
  EXPECT_DOUBLE_EQ(.75, file->get_timing(41).get_audio_video_ratio());
  EXPECT_DOUBLE_EQ(1.125, file->get_timing(31, 11).get_audio_video_ratio());
  EXPECT_DOUBLE_EQ(.75, file->get_timing(41, 11).get_audio_video_ratio());
  EXPECT_DOUBLE_EQ(1.5, file->get_timing(31, 21).get_audio_video_ratio());
  EXPECT_DOUBLE_EQ(1, file->get_timing(41, 21).get_audio_video_ratio());
  EXPECT_TRUE(file->get_timing(31, 11).get_video_freeze());
  EXPECT_FALSE(file->get_timing(41, 11).get_video_freeze());
  EXPECT_TRUE(file->get_timing(31, 21).get_video_freeze());
  EXPECT_FALSE(file->get_timing(41, 21).get_video_freeze());
  EXPECT_DOUBLE_EQ(1.125, file->get_timing().get_audio_video_ratio());
}

TEST(LiblcvmMultivideoTest,
     LibraryKeysStayFullyQualifiedAndAliasesSelectOnlyViews) {
  auto file = ParseVideo("multivideo_2v_2a.mp4");
  ASSERT_NE(nullptr, file);
  auto first = VideoFields(file), selected = VideoFields(file, 41, 21);
  EXPECT_EQ("320.000000", ValueText(first.at("width")));
  EXPECT_EQ("640.000000", ValueText(selected.at("width")));
  EXPECT_EQ("mp4a", ValueText(selected.at("audio_type")));
  for (const auto& key :
       {"width_v11", "width_v21", "num_video_frames_v11",
        "num_video_frames_v21", "audio_type_a31", "audio_type_a41",
        "audio_video_ratio_v11_a31", "audio_video_ratio_v11_a41",
        "audio_video_ratio_v21_a31", "audio_video_ratio_v21_a41"}) {
    ASSERT_NE(0u, first.count(key));
    EXPECT_EQ(ValueText(first.at(key)), ValueText(selected.at(key)));
  }
  EXPECT_EQ("1.125000", ValueText(first.at("audio_video_ratio_v11_a31")));
  EXPECT_EQ("0.750000", ValueText(first.at("audio_video_ratio_v11_a41")));
  EXPECT_EQ("1.500000", ValueText(first.at("audio_video_ratio_v21_a31")));
  EXPECT_EQ("1.000000", ValueText(first.at("audio_video_ratio_v21_a41")));
  auto singleton = VideoFields(ParseVideo("multivideo_1v_1a.mp4"));
  EXPECT_NE(0u, singleton.count("width_v11"));
  EXPECT_NE(0u, singleton.count("audio_type_a31"));
  EXPECT_NE(0u, singleton.count("video_freeze_v11_a31"));
  auto legacy = VideoFields(file, -1, -1, false);
  EXPECT_EQ(0u, legacy.count("width_v11"));
  EXPECT_EQ("320.000000", ValueText(legacy.at("width")));
}

TEST(LiblcvmMultivideoTest,
     ExplicitVideoSelectorWorksInParseToListsAndTimestamps) {
  const auto path = std::string(TEST_REGRESSION_DIR) + "/multivideo_2v_2a.mp4";
  LiblcvmConfig config;
  config.set_video_track_id(21);
  config.set_audio_track_id(41);
  config.set_include_track_keys(true);
  LiblcvmKeyList keys, tk;
  LiblcvmValList values;
  LiblcvmTimingList tv;
  ASSERT_EQ(0, IsobmffFileInformation::parse_to_lists(
                   path.c_str(), config, &keys, &values, true, &tk, &tv));
  EXPECT_EQ(72u, tv.size());
  EXPECT_EQ(8u, tk.size());
  auto file = IsobmffFileInformation::parse(path.c_str(), config);
  ASSERT_NE(nullptr, file);
  // Export selectors do not alter the object's first-track default.
  EXPECT_EQ(11u, file->get_video_info().get_track_id());
  EXPECT_EQ(31u, file->get_audio().get_track_id());
}

TEST(LiblcvmMultivideoTest, MaxUint32VideoIdAndInvalidSelectors) {
  auto file = ParseVideo("multivideo_max_id.mp4");
  ASSERT_NE(nullptr, file);
  EXPECT_EQ(UINT32_MAX, file->get_video_info().get_track_id());
  EXPECT_EQ(UINT32_MAX,
            file->get_video_info(int64_t(UINT32_MAX)).get_track_id());
  EXPECT_EQ(120,
            file->get_timing(-1, int64_t(UINT32_MAX)).get_num_video_frames());
  for (int64_t id : {int64_t(-2), int64_t(99), int64_t(UINT32_MAX) + 1,
                     std::numeric_limits<int64_t>::min(),
                     std::numeric_limits<int64_t>::max()}) {
    EXPECT_THROW(file->get_video_info(id), std::out_of_range);
    EXPECT_THROW(file->get_timing(-1, id), std::out_of_range);
  }
}

TEST(LiblcvmMultivideoTest, FrameBudgetAppliesToTheWholeFile) {
  EXPECT_EQ(nullptr, ParseVideo("multivideo_2v_2a.mp4", false, 191));
  EXPECT_NE(nullptr, ParseVideo("multivideo_2v_2a.mp4", false, 192));
  EXPECT_NE(nullptr, ParseVideo("multivideo_1v_1a.mp4", false, 120));
  EXPECT_EQ(nullptr, ParseVideo("multivideo_1v_1a.mp4", false, 119));
}

TEST(LiblcvmMultivideoTest, EveryVideoHasIndependentPtsOrderingAndDropState) {
  auto file = ParseVideo("multivideo_ctts.mp4");
  ASSERT_NE(nullptr, file);
  EXPECT_EQ((std::vector<uint32_t>{0, 2, 1, 3}),
            file->get_timing(-1, 11).get_frame_num_orig_list());
  EXPECT_EQ((std::vector<uint32_t>{2, 1, 0}),
            file->get_timing(-1, 21).get_frame_num_orig_list());
  EXPECT_EQ(1u,
            file->get_timing(-1, 11).get_frame_drop_length_sec_list().size());
  EXPECT_TRUE(
      file->get_timing(-1, 21).get_frame_drop_length_sec_list().empty());
  auto unsorted = ParseVideo("multivideo_ctts.mp4", false, 100, false);
  ASSERT_NE(nullptr, unsorted);
  EXPECT_EQ((std::vector<uint32_t>{0, 1, 2, 3}),
            unsorted->get_timing(-1, 11).get_frame_num_orig_list());
  EXPECT_EQ((std::vector<uint32_t>{0, 1, 2}),
            unsorted->get_timing(-1, 21).get_frame_num_orig_list());
}

TEST(LiblcvmMultivideoTest, SkippedAudioPreservesEveryVideoAndMdhdPairing) {
  auto file = ParseVideo("multivideo_2v_2a.mp4", true);
  ASSERT_NE(nullptr, file);
  EXPECT_EQ(2u, file->get_num_video_tracks());
  EXPECT_EQ(2u, file->get_num_audio_tracks());
  EXPECT_EQ("", file->get_audio(41).get_audio_type());
  EXPECT_DOUBLE_EQ(3.2, file->get_timing(41, 21).get_duration_audio_sec());
  EXPECT_DOUBLE_EQ(3, file->get_timing(41, 21).get_duration_video_sec());
  EXPECT_TRUE(file->get_timing(41, 21).get_video_freeze());
}

TEST(LiblcvmMultivideoTest,
     MissingAndUnsupportedVideoNeverHiddenByValidSibling) {
  for (const auto& name :
       {"multivideo_duplicate.mp4", "multivideo_unsupported.mp4",
        "multivideo_missing_config.mp4"}) {
    EXPECT_EQ(nullptr, ParseVideo(name));
    EXPECT_EQ(nullptr, ParseVideo(name, true));
  }
}

TEST(LiblcvmMultivideoTest, AbsentVideoOrAudioHasNoInventedPairs) {
  auto no_audio = ParseVideo("multivideo_2v_0a.mp4");
  ASSERT_NE(nullptr, no_audio);
  EXPECT_EQ(2u, no_audio->get_num_video_tracks());
  EXPECT_EQ(0u, no_audio->get_num_audio_tracks());
  EXPECT_DOUBLE_EQ(-1, no_audio->get_timing(-1, 21).get_audio_video_ratio());
  auto fields = VideoFields(no_audio);
  EXPECT_NE(0u, fields.count("width_v11"));
  EXPECT_NE(0u, fields.count("width_v21"));
  EXPECT_EQ(0u, fields.count("video_freeze_v11_a0"));
  auto no_video = ParseVideo("multivideo_0v_1a.mp4");
  ASSERT_NE(nullptr, no_video);
  EXPECT_TRUE(no_video->get_video_track_ids().empty());
  EXPECT_EQ(1u, no_video->get_num_audio_tracks());
  EXPECT_THROW(no_video->get_video_info(11), std::out_of_range);
  fields = VideoFields(no_video);
  EXPECT_EQ(0u, fields.count("width_v0"));
  EXPECT_EQ(0u, fields.count("video_freeze_v0_a31"));
  EXPECT_NE(0u, fields.count("audio_type_a31"));
}

TEST(LiblcvmMultivideoTest,
     EmptyAndVideoOnlyInputsReturnDefaultsWithoutCrashing) {
  auto empty = ParseVideo("multivideo_0v_0a.mp4");
  ASSERT_NE(nullptr, empty);
  EXPECT_EQ(0u, empty->get_num_video_tracks());
  EXPECT_EQ(0u, empty->get_num_audio_tracks());
  EXPECT_DOUBLE_EQ(-1, empty->get_timing().get_duration_video_sec());
  EXPECT_DOUBLE_EQ(-1, empty->get_timing().get_duration_audio_sec());
  EXPECT_DOUBLE_EQ(-1, empty->get_timing().get_audio_video_ratio());
  EXPECT_FALSE(empty->get_timing().get_video_freeze());
  auto one_video = ParseVideo("multivideo_1v_0a.mp4");
  ASSERT_NE(nullptr, one_video);
  EXPECT_EQ(1u, one_video->get_num_video_tracks());
  EXPECT_EQ(0u, one_video->get_num_audio_tracks());
  EXPECT_DOUBLE_EQ(-1, one_video->get_timing().get_audio_video_ratio());
}
