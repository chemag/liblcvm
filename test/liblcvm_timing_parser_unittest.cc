#include <gtest/gtest.h>

#include <cmath>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include "liblcvm.h"

namespace {
template <typename T, typename = void>
struct HasAudioDuration : std::false_type {};
template <typename T>
struct HasAudioDuration<
    T, std::void_t<decltype(std::declval<T>().get_duration_audio_sec())>>
    : std::true_type {};
template <typename T, typename = void>
struct HasVideoDuration : std::false_type {};
template <typename T>
struct HasVideoDuration<
    T, std::void_t<decltype(std::declval<T>().get_duration_video_sec())>>
    : std::true_type {};
template <typename T, typename = void>
struct CanDeriveTiming : std::false_type {};
template <typename T>
struct CanDeriveTiming<
    T, std::void_t<decltype(std::declval<T&>().derive_timing_info(
           false, std::string{}, 0))>> : std::true_type {};
template <typename T, typename = void>
struct CanReadTiming : std::false_type {};
template <typename T>
struct CanReadTiming<
    T, std::void_t<decltype(std::declval<T&>().parse_timing_information(
           std::shared_ptr<ISOBMFF::ContainerBox>{}, 1000, 32, true,
           std::string{}, 0))>> : std::true_type {};
static_assert(CanReadTiming<TimingInformation>::value);
static_assert(CanDeriveTiming<TimingInformation>::value);
static_assert(!CanReadTiming<AudioVideoTimingInformation>::value);
static_assert(!CanDeriveTiming<AudioVideoTimingInformation>::value);
static_assert(!HasAudioDuration<TimingInformation>::value);
static_assert(!HasVideoDuration<TimingInformation>::value);
static_assert(HasAudioDuration<AudioVideoTimingInformation>::value);
static_assert(std::is_same_v<decltype(std::declval<TimingInformation>()
                                          .get_timescale_movie_hz()),
                             uint32_t>);
std::shared_ptr<ISOBMFF::ContainerBox> Stbl(const char* name, uint32_t* hz) {
  ISOBMFF::Parser parser;
  const auto error =
      parser.Parse(std::string(TEST_REGRESSION_DIR) + "/" + name);
  if (error)
    return nullptr;
  const auto file = parser.GetFile();
  const auto moov = file->GetTypedBox<ISOBMFF::ContainerBox>("moov");
  const auto trak = moov->GetTypedBox<ISOBMFF::ContainerBox>("trak");
  const auto mdia = trak->GetTypedBox<ISOBMFF::ContainerBox>("mdia");
  *hz = mdia->GetTypedBox<ISOBMFF::MDHD>("mdhd")->GetTimescale();
  return mdia->GetTypedBox<ISOBMFF::ContainerBox>("minf")
      ->GetTypedBox<ISOBMFF::ContainerBox>("stbl");
}
}  // namespace

TEST(LiblcvmTimingParserTest, ReaderOnlyStoresCompactRawValues) {
  uint32_t hz = 0;
  const auto stbl = Stbl("track_borrowed_timing.mp4", &hz);
  ASSERT_NE(nullptr, stbl);
  TimingInformation timing;
  ASSERT_EQ(0, timing.parse_timing_information(stbl, hz, 32, true, "probe", 0));
  ASSERT_EQ(5u, timing.get_stts_entries().size());
  EXPECT_EQ(2u, timing.get_stts_entries().front().sample_count);
  EXPECT_EQ(40u, timing.get_stts_entries().front().sample_delta);
  EXPECT_EQ(hz, timing.get_timescale_hz());
  EXPECT_TRUE(timing.get_per_frame_info());
  EXPECT_TRUE(timing.get_pts_sec_list().empty());
  EXPECT_TRUE(timing.get_dts_sec_list().empty());
  EXPECT_TRUE(timing.get_stts_unit_list().empty());
  EXPECT_TRUE(timing.get_keyframe_sample_number_list().empty());
  EXPECT_TRUE(timing.get_frame_drop_length_sec_list().empty());
  EXPECT_EQ(0, timing.get_num_frames());
  EXPECT_EQ(0u, timing.get_num_samples());
}

TEST(LiblcvmTimingParserTest, SameReaderSupportsScalarAndPerFrameModes) {
  uint32_t hz = 0;
  const auto stbl = Stbl("track_borrowed_timing.mp4", &hz);
  ASSERT_NE(nullptr, stbl);
  TimingInformation frame, scalar;
  ASSERT_EQ(0, frame.parse_timing_information(stbl, hz, 32, true, "probe", 0));
  ASSERT_EQ(0, scalar.parse_timing_information(stbl, hz, 1, false, "probe", 0));
  ASSERT_EQ(0, frame.derive_timing_info(true, "probe", 0));
  ASSERT_EQ(0, scalar.derive_timing_info(true, "probe", 0));
  EXPECT_EQ(8u, frame.get_num_samples());
  EXPECT_EQ(8u, scalar.get_num_samples());
  EXPECT_EQ(440u, scalar.get_stts_duration_units());
  EXPECT_DOUBLE_EQ(0.44, scalar.get_stts_duration_sec());
  EXPECT_DOUBLE_EQ(frame.get_stts_duration_sec(),
                   scalar.get_stts_duration_sec());
  EXPECT_EQ(8, frame.get_num_frames());
  EXPECT_EQ(8u, frame.get_pts_sec_list().size());
  EXPECT_EQ(2u, frame.get_frame_drop_length_sec_list().size());
  EXPECT_TRUE(std::isfinite(frame.get_frame_rate_fps_median()));
  EXPECT_EQ(hz, frame.get_timescale_hz());
  EXPECT_EQ(0, scalar.get_num_frames());
  EXPECT_TRUE(scalar.get_pts_sec_list().empty());
  EXPECT_TRUE(scalar.get_stts_unit_list().empty());
  EXPECT_TRUE(scalar.get_frame_drop_length_sec_list().empty());
  EXPECT_TRUE(scalar.get_keyframe_sample_number_list().empty());
  EXPECT_EQ(0, scalar.get_frame_drop_count());
}

TEST(LiblcvmTimingParserTest, DerivationIsRepeatableWithoutAccumulatingDrops) {
  uint32_t hz = 0;
  const auto stbl = Stbl("track_borrowed_timing.mp4", &hz);
  ASSERT_NE(nullptr, stbl);
  TimingInformation timing;
  ASSERT_EQ(0, timing.parse_timing_information(stbl, hz, 32, true, "probe", 0));
  ASSERT_EQ(0, timing.derive_timing_info(true, "probe", 0));
  const auto pts = timing.get_pts_sec_list();
  const auto drops = timing.get_frame_drop_length_sec_list();
  ASSERT_EQ(0, timing.derive_timing_info(true, "probe", 0));
  EXPECT_EQ(pts, timing.get_pts_sec_list());
  EXPECT_EQ(drops, timing.get_frame_drop_length_sec_list());
  EXPECT_EQ(8u, timing.get_num_samples());
  ASSERT_EQ(0,
            timing.parse_timing_information(stbl, hz, 32, false, "probe", 0));
  EXPECT_TRUE(timing.get_pts_sec_list().empty());
  EXPECT_TRUE(timing.get_frame_drop_length_sec_list().empty());
  ASSERT_EQ(0, timing.derive_timing_info(true, "probe", 0));
  EXPECT_TRUE(timing.get_pts_sec_list().empty());
}

TEST(LiblcvmTimingParserTest, FailedReadDoesNotReplaceEarlierResult) {
  uint32_t hz = 0;
  const auto stbl = Stbl("track_borrowed_timing.mp4", &hz);
  ASSERT_NE(nullptr, stbl);
  TimingInformation timing;
  ASSERT_EQ(0, timing.parse_timing_information(stbl, hz, 32, true, "probe", 0));
  ASSERT_EQ(0, timing.derive_timing_info(true, "probe", 0));
  const auto pts = timing.get_pts_sec_list();
  const auto empty = std::make_shared<ISOBMFF::ContainerBox>("stbl");
  EXPECT_EQ(-1,
            timing.parse_timing_information(empty, hz, 32, true, "probe", 0));
  EXPECT_EQ(pts, timing.get_pts_sec_list());
  EXPECT_EQ(8u, timing.get_num_samples());
}

TEST(LiblcvmTimingParserTest, ScalarReadAllowsMissingFrameTables) {
  const auto empty = std::make_shared<ISOBMFF::ContainerBox>("stbl");
  TimingInformation timing;
  ASSERT_EQ(
      0, timing.parse_timing_information(empty, 48000, 0, false, "probe", 0));
  ASSERT_EQ(0, timing.derive_timing_info(false, "probe", 0));
  EXPECT_FALSE(timing.get_has_stts());
  EXPECT_DOUBLE_EQ(-1, timing.get_stts_duration_sec());
  EXPECT_TRUE(timing.get_pts_sec_list().empty());
  EXPECT_EQ(48000u, timing.get_timescale_hz());
}

TEST(LiblcvmTimingParserTest,
     HugeSampleCountDoesNotAllocateFrameArraysInScalarMode) {
  uint32_t hz = 0;
  const auto stbl = Stbl("stts_huge_sample_count.mov", &hz);
  ASSERT_NE(nullptr, stbl);
  TimingInformation frames, scalar;
  EXPECT_EQ(-1,
            frames.parse_timing_information(stbl, hz, 16, true, "probe", 0));
  ASSERT_EQ(0,
            scalar.parse_timing_information(stbl, hz, 16, false, "probe", 0));
  ASSERT_EQ(0, scalar.derive_timing_info(false, "probe", 0));
  EXPECT_EQ(134218362u, scalar.get_num_samples());
  EXPECT_EQ(1476401352u, scalar.get_stts_duration_units());
  EXPECT_TRUE(scalar.get_pts_sec_list().empty());
  EXPECT_TRUE(scalar.get_dts_sec_list().empty());
  EXPECT_TRUE(scalar.get_stts_unit_list().empty());
}

TEST(LiblcvmTimingParserTest, FileUsesFrameModeForVideoAndScalarModeForAudio) {
  LiblcvmConfig config;
  const auto file = IsobmffFileInformation::parse(
      (std::string(TEST_REGRESSION_DIR) + "/multivideo_2v_2a.mp4").c_str(),
      config);
  ASSERT_NE(nullptr, file);
  for (const auto& video : file->get_video_tracks()) {
    EXPECT_TRUE(video.get_timing().get_per_frame_info());
    EXPECT_EQ(video.get_timing().get_num_samples(),
              video.get_timing().get_pts_sec_list().size());
  }
  for (const auto& audio : file->get_audio_tracks()) {
    EXPECT_FALSE(audio.get_timing().get_per_frame_info());
    EXPECT_TRUE(audio.get_timing().get_pts_sec_list().empty());
    EXPECT_TRUE(audio.get_timing().get_frame_drop_length_sec_list().empty());
    EXPECT_DOUBLE_EQ(audio.get_timing().get_mdhd_duration_sec(),
                     audio.get_mdhd_duration_sec());
  }
}

TEST(LiblcvmTimingParserTest, ReadingNewSourceClearsPreviousHeaderContext) {
  LiblcvmConfig config;
  const auto file = IsobmffFileInformation::parse(
      (std::string(TEST_REGRESSION_DIR) + "/multivideo_2v_2a.mp4").c_str(),
      config);
  ASSERT_NE(nullptr, file);
  auto timing = file->get_video_track(21)->get_timing();
  EXPECT_DOUBLE_EQ(3, timing.get_duration_sec());
  uint32_t hz = 0;
  const auto stbl = Stbl("track_borrowed_timing.mp4", &hz);
  ASSERT_NE(nullptr, stbl);
  ASSERT_EQ(0,
            timing.parse_timing_information(stbl, hz, 32, false, "probe", 0));
  EXPECT_EQ(0u, timing.get_mdhd_duration_units());
  ASSERT_EQ(0, timing.derive_timing_info(false, "probe", 0));
  EXPECT_DOUBLE_EQ(0.44, timing.get_duration_sec());
  ASSERT_EQ(0, timing.parse_timing_information(stbl, hz, 32, true, "probe", 0));
  ASSERT_EQ(0, timing.derive_timing_info(true, "probe", 0));
  EXPECT_EQ(8, timing.get_num_frames());
  EXPECT_DOUBLE_EQ(0.44, timing.get_duration_sec());
}

TEST(LiblcvmTimingParserTest, FileAudioUsesScalarTableSummaryAndRawHeaders) {
  LiblcvmConfig config;
  const auto file = IsobmffFileInformation::parse(
      (std::string(TEST_REGRESSION_DIR) + "/timing_audio_stts.mp4").c_str(),
      config);
  ASSERT_NE(nullptr, file);
  const auto* audio = file->get_audio_track(31);
  ASSERT_NE(nullptr, audio);
  const auto& timing = audio->get_timing();
  EXPECT_FALSE(timing.get_per_frame_info());
  EXPECT_EQ(12u, timing.get_num_samples());
  EXPECT_EQ(12288u, timing.get_stts_duration_units());
  EXPECT_DOUBLE_EQ(0.256, timing.get_stts_duration_sec());
  EXPECT_EQ(12288u, timing.get_mdhd_duration_units());
  EXPECT_EQ(4000u, timing.get_tkhd_duration_units());
  EXPECT_DOUBLE_EQ(0.256, timing.get_mdhd_duration_sec());
  EXPECT_DOUBLE_EQ(4, audio->get_duration_sec());
  EXPECT_TRUE(timing.get_pts_sec_list().empty());
  EXPECT_TRUE(timing.get_frame_drop_length_sec_list().empty());
}

TEST(LiblcvmTimingParserTest,
     EmptyCttsBoxExpandsZeroOffsetsOnlyDuringDerivation) {
  uint32_t hz = 0;
  const auto stbl = Stbl("timing_empty_ctts.mp4", &hz);
  ASSERT_NE(nullptr, stbl);
  TimingInformation timing;
  ASSERT_EQ(0, timing.parse_timing_information(stbl, hz, 16, true, "probe", 0));
  EXPECT_TRUE(timing.get_ctts_entries().empty());
  EXPECT_TRUE(timing.get_ctts_unit_list().empty());
  ASSERT_EQ(0, timing.derive_timing_info(false, "probe", 0));
  EXPECT_EQ((std::vector<int32_t>{0, 0, 0, 0}), timing.get_ctts_unit_list());
}

TEST(LiblcvmTimingParserTest,
     ScalarTickOverflowIsRejectedWithoutFrameAllocation) {
  uint32_t hz = 0;
  const auto stbl = Stbl("timing_scalar_tick_overflow.mp4", &hz);
  ASSERT_NE(nullptr, stbl);
  TimingInformation timing;
  ASSERT_EQ(0, timing.parse_timing_information(stbl, hz, 1, false, "probe", 0));
  EXPECT_EQ(-1, timing.derive_timing_info(false, "probe", 0));
  EXPECT_TRUE(timing.get_pts_sec_list().empty());
  EXPECT_EQ(0u, timing.get_num_samples());
}

TEST(LiblcvmTimingParserTest,
     SelectedAvViewIsAnIndependentNonReparseableSnapshot) {
  LiblcvmConfig config;
  const auto file = IsobmffFileInformation::parse(
      (std::string(TEST_REGRESSION_DIR) + "/multivideo_2v_2a.mp4").c_str(),
      config);
  ASSERT_NE(nullptr, file);
  const auto view = file->get_timing(31, 21);
  auto intrinsic_copy = static_cast<TimingInformation>(view);
  uint32_t hz = 0;
  const auto stbl = Stbl("track_borrowed_timing.mp4", &hz);
  ASSERT_NE(nullptr, stbl);
  ASSERT_EQ(0, intrinsic_copy.parse_timing_information(stbl, hz, 32, true,
                                                       "probe", 0));
  ASSERT_EQ(0, intrinsic_copy.derive_timing_info(true, "probe", 0));
  EXPECT_DOUBLE_EQ(0.44, intrinsic_copy.get_duration_sec());
  EXPECT_DOUBLE_EQ(3, view.get_duration_video_sec());
  EXPECT_DOUBLE_EQ(4.5, view.get_duration_audio_sec());
  EXPECT_DOUBLE_EQ(1.5, view.get_audio_video_ratio());
  EXPECT_TRUE(view.get_video_freeze());
}
