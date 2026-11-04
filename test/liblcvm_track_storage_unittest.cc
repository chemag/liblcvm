#include <gtest/gtest.h>

#include <type_traits>

#include "liblcvm.h"

namespace {
auto Parse(const char* name, bool skip = false) {
  LiblcvmConfig config;
  config.set_skip_audio_parsing(skip);
  return IsobmffFileInformation::parse(
      (std::string(TEST_REGRESSION_DIR) + "/" + name).c_str(), config);
}
static_assert(
    std::is_same_v<decltype(std::declval<const IsobmffFileInformation&>()
                                .get_video_tracks()),
                   const std::vector<IsobmffFileInformation::VideoTrack>&>);
static_assert(
    std::is_same_v<decltype(std::declval<const IsobmffFileInformation&>()
                                .get_audio_tracks()),
                   const std::vector<IsobmffFileInformation::AudioTrack>&>);
}  // namespace

TEST(LiblcvmTrackStorageTest, AllVideosUseTheSameCollection) {
  const auto file = Parse("multivideo_2v_2a.mp4");
  ASSERT_NE(nullptr, file);
  const auto& tracks = file->get_video_tracks();
  ASSERT_EQ(2u, tracks.size());
  EXPECT_EQ(2u, file->get_num_video_tracks());
  EXPECT_EQ(11u, tracks.front().video.get_track_id());
  EXPECT_EQ(21u, tracks.back().video.get_track_id());
  EXPECT_EQ(tracks.front().video.get_track_id(),
            file->get_video_info().get_track_id());
  EXPECT_EQ(120, tracks.front().timing.get_num_video_frames());
  EXPECT_EQ(72, tracks.back().timing.get_num_video_frames());
  EXPECT_EQ(tracks.front().timing.get_pts_sec_list(),
            file->get_timing(-1, 11).get_pts_sec_list());
  EXPECT_EQ(tracks.back().timing.get_pts_sec_list(),
            file->get_timing(-1, 21).get_pts_sec_list());
  EXPECT_EQ(&tracks, &file->get_video_tracks());
}

TEST(LiblcvmTrackStorageTest, AudioTimingIsCanonicalAndHasNoFrameArrays) {
  const auto file = Parse("multi_audio_max_id.mp4");
  ASSERT_NE(nullptr, file);
  const auto& tracks = file->get_audio_tracks();
  ASSERT_EQ(2u, tracks.size());
  EXPECT_EQ(UINT32_MAX, tracks.front().audio.get_track_id());
  EXPECT_EQ(UINT32_MAX, file->get_audio_info().get_track_id());
  EXPECT_EQ(23u, tracks.back().audio.get_track_id());
  for (const auto& track : tracks) {
    const auto info = file->get_audio_info(track.audio.get_track_id());
    EXPECT_DOUBLE_EQ(track.timing.get_duration_audio_sec(),
                     info.get_duration_sec());
    EXPECT_EQ(track.timing.get_timescale_audio_hz(), info.get_timescale_hz());
    EXPECT_DOUBLE_EQ(-1, track.audio.get_duration_sec());
    EXPECT_EQ(0u, track.audio.get_timescale_hz());
    EXPECT_TRUE(track.timing.get_pts_sec_list().empty());
    EXPECT_TRUE(track.timing.get_dts_sec_list().empty());
    EXPECT_TRUE(track.timing.get_stts_unit_list().empty());
    EXPECT_TRUE(track.timing.get_keyframe_sample_number_list().empty());
    EXPECT_TRUE(track.timing.get_frame_drop_length_sec_list().empty());
  }
}

TEST(LiblcvmTrackStorageTest, SelectedPairsDoNotAlterStoredVideoTiming) {
  const auto file = Parse("multivideo_2v_2a.mp4");
  ASSERT_NE(nullptr, file);
  const auto& stored = file->get_video_tracks().back().timing;
  const auto before = stored.get_pts_sec_list();
  const auto first = file->get_timing(31, 21);
  const auto second = file->get_timing(41, 21);
  EXPECT_DOUBLE_EQ(file->get_audio_info(31).get_duration_sec() /
                       stored.get_duration_video_sec(),
                   first.get_audio_video_ratio());
  EXPECT_DOUBLE_EQ(file->get_audio_info(41).get_duration_sec() /
                       stored.get_duration_video_sec(),
                   second.get_audio_video_ratio());
  EXPECT_NE(first.get_audio_video_ratio(), second.get_audio_video_ratio());
  EXPECT_DOUBLE_EQ(-1, stored.get_duration_audio_sec());
  EXPECT_EQ(0u, stored.get_timescale_audio_hz());
  EXPECT_DOUBLE_EQ(-1, stored.get_audio_video_ratio());
  EXPECT_FALSE(stored.get_video_freeze());
  EXPECT_EQ(before, stored.get_pts_sec_list());
  EXPECT_EQ(before, first.get_pts_sec_list());
  EXPECT_EQ(before, second.get_pts_sec_list());
}

TEST(LiblcvmTrackStorageTest, EmptyCollectionsProvideDefaultViews) {
  const auto no_audio = Parse("multi_audio_no_audio.mp4");
  ASSERT_NE(nullptr, no_audio);
  EXPECT_TRUE(no_audio->get_audio_tracks().empty());
  EXPECT_EQ(0u, no_audio->get_num_audio_tracks());
  EXPECT_DOUBLE_EQ(-1, no_audio->get_audio_info().get_duration_sec());
  EXPECT_DOUBLE_EQ(-1, no_audio->get_timing().get_audio_video_ratio());
  EXPECT_THROW(no_audio->get_audio_info(23), std::out_of_range);
  const auto no_video = Parse("multivideo_0v_1a.mp4");
  ASSERT_NE(nullptr, no_video);
  EXPECT_TRUE(no_video->get_video_tracks().empty());
  EXPECT_EQ(0u, no_video->get_num_video_tracks());
  EXPECT_EQ(0, no_video->get_timing().get_num_video_frames());
  EXPECT_DOUBLE_EQ(-1, no_video->get_timing().get_duration_video_sec());
  EXPECT_DOUBLE_EQ(-1, no_video->get_timing().get_audio_video_ratio());
  EXPECT_THROW(no_video->get_video_info(11), std::out_of_range);
  EXPECT_GT(no_video->get_video_info().get_filesize(), 0);
}

TEST(LiblcvmTrackStorageTest, LegacyCodecSkipBehaviorIsUnchanged) {
  const auto file = Parse("multivideo_2v_2a.mp4", true);
  ASSERT_NE(nullptr, file);
  ASSERT_EQ(2u, file->get_audio_tracks().size());
  ASSERT_EQ(2u, file->get_video_tracks().size());
  for (const auto& track : file->get_audio_tracks()) {
    EXPECT_EQ("", track.audio.get_audio_type());
    EXPECT_DOUBLE_EQ(track.audio.get_mdhd_duration_sec(),
                     track.timing.get_duration_audio_sec());
    EXPECT_TRUE(track.timing.get_pts_sec_list().empty());
  }
  EXPECT_EQ(72, file->get_video_tracks().back().timing.get_num_video_frames());
}
