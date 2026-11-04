#include <gtest/gtest.h>

#include <algorithm>
#include <type_traits>
#include <utility>

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
                                .get_video_track()),
                   const VideoTrack*>);
static_assert(
    std::is_same_v<decltype(std::declval<const IsobmffFileInformation&>()
                                .get_audio_track()),
                   const AudioTrack*>);
static_assert(
    std::is_same_v<decltype(std::declval<const VideoTrack&>().get_timing()),
                   const TimingInformation&>);
static_assert(
    std::is_same_v<decltype(std::declval<const AudioTrack&>().get_timing()),
                   const TimingInformation&>);
static_assert(
    std::is_same_v<decltype(std::declval<const IsobmffFileInformation&>()
                                .get_video_tracks()),
                   const std::vector<VideoTrack>&>);
static_assert(
    std::is_same_v<decltype(std::declval<const IsobmffFileInformation&>()
                                .get_audio_tracks()),
                   const std::vector<AudioTrack>&>);
}  // namespace

TEST(LiblcvmTrackStorageTest, AllVideosAreFullTrackObjects) {
  const auto file = Parse("multivideo_2v_2a.mp4");
  ASSERT_NE(nullptr, file);
  const auto& tracks = file->get_video_tracks();
  ASSERT_EQ(2u, tracks.size());
  EXPECT_EQ(2u, file->get_num_video_tracks());
  EXPECT_EQ(11u, tracks.front().get_track_id());
  EXPECT_EQ(21u, tracks.back().get_track_id());
  EXPECT_EQ(&tracks.front(), file->get_video_track());
  EXPECT_EQ(&tracks.front(), file->get_video_track(11));
  EXPECT_EQ(&tracks.back(), file->get_video_track(21));
  EXPECT_EQ(file->get_video_track(21), file->get_video_track(21));
  EXPECT_EQ(120, tracks.front().get_timing().get_num_frames());
  EXPECT_EQ(72, tracks.back().get_timing().get_num_frames());
  EXPECT_EQ(tracks.back().get_timing().get_pts_sec_list(),
            file->get_timing(-1, 21).get_pts_sec_list());
}

TEST(LiblcvmTrackStorageTest, AudioTimingIsOwnedWithItsMetadata) {
  const auto file = Parse("multi_audio_max_id.mp4");
  ASSERT_NE(nullptr, file);
  const auto& tracks = file->get_audio_tracks();
  ASSERT_EQ(2u, tracks.size());
  EXPECT_EQ(UINT32_MAX, tracks.front().get_track_id());
  EXPECT_EQ(&tracks.front(), file->get_audio_track());
  EXPECT_EQ(&tracks.front(), file->get_audio_track(int64_t(UINT32_MAX)));
  EXPECT_EQ(23u, tracks.back().get_track_id());
  for (const auto& track : tracks) {
    EXPECT_EQ(&track, file->get_audio_track(track.get_track_id()));
    EXPECT_DOUBLE_EQ(track.get_timing().get_duration_sec(),
                     track.get_duration_sec());
    EXPECT_EQ(track.get_timing().get_timescale_hz(), track.get_timescale_hz());
    EXPECT_TRUE(track.get_timing().get_pts_sec_list().empty());
    EXPECT_TRUE(track.get_timing().get_dts_sec_list().empty());
    EXPECT_TRUE(track.get_timing().get_stts_unit_list().empty());
    EXPECT_TRUE(track.get_timing().get_keyframe_sample_number_list().empty());
  }
}

TEST(LiblcvmTrackStorageTest, SelectedPairsDoNotAlterIntrinsicVideoTiming) {
  const auto file = Parse("multivideo_2v_2a.mp4");
  ASSERT_NE(nullptr, file);
  const auto& stored = file->get_video_track(21)->get_timing();
  const auto before = stored.get_pts_sec_list();
  const auto first = file->get_timing(31, 21);
  const auto second = file->get_timing(41, 21);
  EXPECT_DOUBLE_EQ(
      file->get_audio_track(31)->get_duration_sec() / stored.get_duration_sec(),
      first.get_audio_video_ratio());
  EXPECT_DOUBLE_EQ(
      file->get_audio_track(41)->get_duration_sec() / stored.get_duration_sec(),
      second.get_audio_video_ratio());
  EXPECT_NE(first.get_audio_video_ratio(), second.get_audio_video_ratio());
  EXPECT_EQ(before, stored.get_pts_sec_list());
  EXPECT_EQ(before, first.get_pts_sec_list());
}

TEST(LiblcvmTrackStorageTest, EmptyCollectionsReturnNullPointers) {
  const auto no_audio = Parse("multi_audio_no_audio.mp4");
  ASSERT_NE(nullptr, no_audio);
  EXPECT_TRUE(no_audio->get_audio_tracks().empty());
  EXPECT_EQ(nullptr, no_audio->get_audio_track());
  EXPECT_EQ(nullptr, no_audio->get_audio_track(23));
  EXPECT_DOUBLE_EQ(-1, no_audio->get_timing().get_duration_audio_sec());
  const auto no_video = Parse("multivideo_0v_1a.mp4");
  ASSERT_NE(nullptr, no_video);
  EXPECT_TRUE(no_video->get_video_tracks().empty());
  EXPECT_EQ(nullptr, no_video->get_video_track());
  EXPECT_EQ(nullptr, no_video->get_video_track(11));
  EXPECT_EQ(0, no_video->get_timing().get_num_frames());
  EXPECT_DOUBLE_EQ(-1, no_video->get_timing().get_duration_video_sec());
  EXPECT_GT(no_video->get_filesize(), 0);
}

TEST(LiblcvmTrackStorageTest, InvalidAndAbsentIdsNeverFallBack) {
  const auto file = Parse("multivideo_2v_2a.mp4");
  ASSERT_NE(nullptr, file);
  for (int64_t id : {int64_t(-2), int64_t(0), int64_t(99),
                     int64_t(UINT32_MAX) + 1, INT64_MAX}) {
    EXPECT_EQ(nullptr, file->get_video_track(id));
    EXPECT_EQ(nullptr, file->get_audio_track(id));
    EXPECT_THROW(file->get_timing(id), std::out_of_range);
    EXPECT_THROW(file->get_timing(-1, id), std::out_of_range);
  }
}

TEST(LiblcvmTrackStorageTest, AudioSkipBypassesTracksEntirely) {
  const auto file = Parse("multivideo_2v_2a.mp4", true);
  ASSERT_NE(nullptr, file);
  EXPECT_TRUE(file->get_audio_tracks().empty());
  ASSERT_EQ(2u, file->get_video_tracks().size());
  EXPECT_EQ(nullptr, file->get_audio_track());
  EXPECT_EQ(72, file->get_video_track(21)->get_timing().get_num_frames());
}

TEST(LiblcvmTrackStorageTest, PercentilesDoNotReorderDropHistory) {
  const auto file = Parse("track_borrowed_timing.mp4");
  ASSERT_NE(nullptr, file);
  auto timing = file->get_timing();
  const auto before = timing.get_frame_drop_length_sec_list();
  ASSERT_EQ(2u, before.size());
  ASSERT_FALSE(std::is_sorted(before.begin(), before.end()));
  std::vector<double> percentiles;
  timing.calculate_percentile_list({50, 90}, percentiles, 0);
  ASSERT_EQ(2u, percentiles.size());
  EXPECT_NEAR(3.0, percentiles.back(), 1e-12);
  EXPECT_EQ(before, timing.get_frame_drop_length_sec_list());
  EXPECT_EQ(
      before,
      file->get_video_track()->get_timing().get_frame_drop_length_sec_list());
}

TEST(LiblcvmTrackStorageTest, BorrowedPointersFollowMovedStorage) {
  auto file = Parse("multivideo_2v_2a.mp4");
  ASSERT_NE(nullptr, file);
  const VideoTrack* borrowed = file->get_video_track(21);
  ASSERT_NE(nullptr, borrowed);
  IsobmffFileInformation moved = std::move(*file);
  EXPECT_EQ(borrowed, moved.get_video_track(21));
  EXPECT_EQ(72, borrowed->get_timing().get_num_frames());
  moved = IsobmffFileInformation{};
  EXPECT_EQ(nullptr, moved.get_video_track(21));
}
