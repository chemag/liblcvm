#include <gtest/gtest.h>

#include <cmath>
#include <map>
#include <memory>
#include <string>

#include "liblcvm.h"

namespace {
auto Parse(const char* name, bool skip = false) {
  LiblcvmConfig config;
  config.set_skip_audio_parsing(skip);
  config.set_max_num_frames(120);
  return IsobmffFileInformation::parse(
      (std::string(TEST_REGRESSION_DIR) + "/" + name).c_str(), config);
}
void InvalidAudio(const AudioTrack* track, const char* type) {
  ASSERT_NE(nullptr, track);
  EXPECT_EQ(type, track->get_audio_type());
  EXPECT_FALSE(track->is_supported());
  EXPECT_EQ(-1, track->get_channel_count());
  EXPECT_EQ(-1, track->get_sample_rate());
  EXPECT_EQ(-1, track->get_sample_size());
  EXPECT_EQ(nullptr, track->get_eac3_configuration());
  EXPECT_DOUBLE_EQ(-1, track->get_duration_sec());
  EXPECT_DOUBLE_EQ(-1, track->get_mdhd_duration_sec());
  EXPECT_EQ(0u, track->get_timescale_hz());
  EXPECT_TRUE(track->get_timing().get_stts_entries().empty());
  EXPECT_TRUE(track->get_timing().get_pts_sec_list().empty());
}
std::shared_ptr<ISOBMFF::ContainerBox> Description(
    const std::shared_ptr<ISOBMFF::Box>& entry) {
  auto stbl = std::make_shared<ISOBMFF::ContainerBox>("stbl");
  auto stsd = std::make_shared<ISOBMFF::STSD>();
  stsd->AddBox(entry);
  stbl->AddBox(stsd);
  return stbl;
}
}  // namespace

TEST(LiblcvmGenericTracksTest, AacAndOpusRemainUsableBesideUnknownAudio) {
  for (const auto* name :
       {"unknown_audio_first.mp4", "unknown_audio_middle.mp4",
        "unknown_audio_last.mp4"}) {
    SCOPED_TRACE(name);
    const auto file = Parse(name);
    ASSERT_NE(nullptr, file);
    ASSERT_EQ(3u, file->get_audio_tracks().size());
    InvalidAudio(file->get_audio_track(99), "fLaC");
    EXPECT_EQ(99u, file->get_audio_track(99)->get_track_id());
    EXPECT_TRUE(file->get_audio_track(31)->is_supported());
    EXPECT_TRUE(file->get_audio_track(41)->is_supported());
    EXPECT_EQ("mp4a", file->get_audio_track(31)->get_audio_type());
    EXPECT_EQ("Opus", file->get_audio_track(41)->get_audio_type());
    EXPECT_DOUBLE_EQ(4, file->get_audio_track(31)->get_duration_sec());
    EXPECT_DOUBLE_EQ(5, file->get_audio_track(41)->get_duration_sec());
    EXPECT_EQ(120, file->get_video_track(11)->get_timing().get_num_frames());
    EXPECT_DOUBLE_EQ(-1, file->get_timing(99, 11).get_audio_video_ratio());
    EXPECT_DOUBLE_EQ(1, file->get_timing(31, 11).get_audio_video_ratio());
  }
}

TEST(LiblcvmGenericTracksTest, ContainerOrderDefinesGenericDefaults) {
  const auto file = Parse("unknown_both.mp4");
  ASSERT_NE(nullptr, file);
  EXPECT_EQ((std::vector<uint32_t>{77, 11}), file->get_video_track_ids());
  EXPECT_EQ((std::vector<uint32_t>{99, 31, 41}), file->get_audio_track_ids());
  EXPECT_EQ(77u, file->get_video_track()->get_track_id());
  EXPECT_EQ(99u, file->get_audio_track()->get_track_id());
  EXPECT_FALSE(file->get_video_track()->is_supported());
  EXPECT_EQ("zzzz", file->get_video_track()->get_video_codec_type());
  EXPECT_DOUBLE_EQ(-1, file->get_video_track()->get_width());
  EXPECT_DOUBLE_EQ(-1, file->get_video_track()->get_height());
  EXPECT_EQ(-1, file->get_video_track()->get_chroma_format());
  EXPECT_DOUBLE_EQ(-1,
                   file->get_video_track()->get_timing().get_duration_sec());
  EXPECT_EQ(0u, file->get_video_track()->get_timing().get_timescale_hz());
  EXPECT_EQ(-1, file->get_timing().get_num_frames());
  EXPECT_EQ(120, file->get_timing(31, 11).get_num_frames());
  EXPECT_EQ(nullptr, file->get_video_track(12345));
  EXPECT_EQ(nullptr, file->get_audio_track(12345));
}

TEST(LiblcvmGenericTracksTest, UnknownVideoDoesNotConsumeFrameBudget) {
  for (const auto* name :
       {"unknown_video_first.mp4", "unknown_video_last.mp4"}) {
    const auto file = Parse(name);
    ASSERT_NE(nullptr, file);
    EXPECT_EQ(2u, file->get_video_tracks().size());
    EXPECT_FALSE(file->get_video_track(77)->is_supported());
    EXPECT_TRUE(file->get_video_track(11)->is_supported());
    EXPECT_EQ(120, file->get_video_track(11)->get_timing().get_num_frames());
    EXPECT_TRUE(
        file->get_video_track(77)->get_timing().get_stts_entries().empty());
    EXPECT_TRUE(
        file->get_video_track(77)->get_timing().get_pts_sec_list().empty());
  }
}

TEST(LiblcvmGenericTracksTest, MissingEmptyAndNestedAudioDescriptions) {
  for (const auto* name : {"missing_audio_stsd.mp4", "empty_audio_stsd.mp4",
                           "generic_audio_children.mp4"}) {
    const auto file = Parse(name);
    ASSERT_NE(nullptr, file);
    InvalidAudio(
        file->get_audio_track(99),
        std::string(name) == "generic_audio_children.mp4" ? "fLaC" : "none");
  }
  const auto known = Parse("unknown_then_known_stsd.mp4");
  ASSERT_NE(nullptr, known);
  ASSERT_NE(nullptr, known->get_audio_track(31));
  EXPECT_TRUE(known->get_audio_track(31)->is_supported());
  EXPECT_EQ("mp4a", known->get_audio_track(31)->get_audio_type());
}

TEST(LiblcvmGenericTracksTest, PublicVideoHelperReturnsGenericDescription) {
  VideoTrack video;
  auto empty = std::make_shared<ISOBMFF::ContainerBox>("stbl");
  ASSERT_EQ(0, video.parse_video_information(empty, "probe", 0));
  EXPECT_EQ("none", video.get_video_codec_type());
  EXPECT_FALSE(video.is_supported());
  empty->AddBox(std::make_shared<ISOBMFF::STSD>());
  ASSERT_EQ(0, video.parse_video_information(empty, "probe", 0));
  EXPECT_EQ("none", video.get_video_codec_type());
  auto stbl = Description(std::make_shared<ISOBMFF::ContainerBox>("av01"));
  ASSERT_EQ(0, video.parse_video_information(stbl, "probe", 0));
  EXPECT_EQ("av01", video.get_video_codec_type());
  EXPECT_EQ(-1, video.get_bit_depth_luma());
  EXPECT_FALSE(video.is_supported());
}

TEST(LiblcvmGenericTracksTest, SkipBypassesEveryAudioIncludingMissingMetadata) {
  for (const auto* name : {"unknown_audio_first.mp4", "supported_aac_opus.mp4",
                           "skipped_audio_no_metadata.mp4"}) {
    const auto file = Parse(name, true);
    ASSERT_NE(nullptr, file);
    EXPECT_TRUE(file->get_audio_tracks().empty());
    EXPECT_EQ(nullptr, file->get_audio_track(99));
    EXPECT_EQ(nullptr, file->get_audio_track(31));
    EXPECT_DOUBLE_EQ(-1, file->get_timing().get_duration_audio_sec());
    EXPECT_EQ(120, file->get_timing().get_num_frames());
  }
}

TEST(LiblcvmGenericTracksTest, KnownMalformedConfigurationIsStillFatal) {
  for (const auto* name :
       {"malformed_opus_with_unknown.mp4", "missing_avcc_with_unknown.mp4",
        "eac3_missing_dec3.mp4"})
    EXPECT_EQ(nullptr, Parse(name));
  const auto known = Parse("supported_aac_opus.mp4");
  ASSERT_NE(nullptr, known);
  AudioTrack receiver;
  for (const auto* type : {"mp4a", "Opus", "ec-3"}) {
    auto malformed = Description(std::make_shared<ISOBMFF::ContainerBox>(type));
    EXPECT_NE(0, receiver.parse_audio_information(malformed, known, 0));
    EXPECT_EQ("mp4a", known->get_audio_track()->get_audio_type());
    EXPECT_DOUBLE_EQ(4, known->get_audio_track()->get_duration_sec());
  }
  VideoTrack video;
  for (const auto* type : {"hvc1", "hev1", "avc1", "avc3"})
    EXPECT_NE(0, video.parse_video_information(
                     Description(std::make_shared<ISOBMFF::ContainerBox>(type)),
                     "probe", 0));
}

TEST(LiblcvmGenericTracksTest,
     RefreshToGenericClearsTimingWithoutTouchingSibling) {
  const auto file = Parse("supported_aac_opus.mp4");
  ASSERT_NE(nullptr, file);
  AudioTrack receiver;
  ASSERT_EQ(0, receiver.parse_audio_information(
                   Description(std::make_shared<ISOBMFF::ContainerBox>("fLaC")),
                   file, 0));
  InvalidAudio(file->get_audio_track(31), "fLaC");
  EXPECT_EQ(31u, file->get_audio_track()->get_track_id());
  EXPECT_DOUBLE_EQ(5, file->get_audio_track(41)->get_duration_sec());
  EXPECT_DOUBLE_EQ(-1, file->get_timing().get_audio_video_ratio());
}

TEST(LiblcvmGenericTracksTest, GenericOnlyTracksAreRecordsNotAbsentIds) {
  const auto file = Parse("only_unknown_video.mp4");
  ASSERT_NE(nullptr, file);
  EXPECT_EQ(1u, file->get_num_video_tracks());
  ASSERT_NE(nullptr, file->get_video_track(77));
  EXPECT_FALSE(file->get_video_track(77)->is_supported());
  EXPECT_EQ(-1, file->get_timing().get_num_frames());
  EXPECT_DOUBLE_EQ(-1, file->get_timing().get_duration_video_sec());
}

TEST(LiblcvmGenericTracksTest,
     MalformedKnownSiblingCannotHideBehindValidEntry) {
  const auto file = Parse("supported_aac_opus.mp4");
  ASSERT_NE(nullptr, file);
  for (bool malformed_first : {false, true}) {
    for (const auto* type : {"Opus", "ec-3"}) {
      auto stbl = std::make_shared<ISOBMFF::ContainerBox>("stbl");
      auto stsd = std::make_shared<ISOBMFF::STSD>();
      auto valid = std::make_shared<ISOBMFF::MP4A>();
      auto bad = std::make_shared<ISOBMFF::ContainerBox>(type);
      if (malformed_first) {
        stsd->AddBox(bad);
        stsd->AddBox(valid);
      } else {
        stsd->AddBox(valid);
        stsd->AddBox(bad);
      }
      stbl->AddBox(stsd);
      AudioTrack receiver;
      EXPECT_NE(0, receiver.parse_audio_information(stbl, file, 0));
      EXPECT_EQ("mp4a", file->get_audio_track()->get_audio_type());
      EXPECT_DOUBLE_EQ(4, file->get_audio_track()->get_duration_sec());
    }
    auto valid = std::make_shared<ISOBMFF::AVC1>();
    valid->AddBox(std::make_shared<ISOBMFF::AVCC>());
    for (const auto* type : {"hvc1", "hev1", "avc3"}) {
      auto stbl = std::make_shared<ISOBMFF::ContainerBox>("stbl");
      auto stsd = std::make_shared<ISOBMFF::STSD>();
      auto bad = std::make_shared<ISOBMFF::ContainerBox>(type);
      if (malformed_first) {
        stsd->AddBox(bad);
        stsd->AddBox(valid);
      } else {
        stsd->AddBox(valid);
        stsd->AddBox(bad);
      }
      stbl->AddBox(stsd);
      VideoTrack video;
      EXPECT_NE(0, video.parse_video_information(stbl, "probe", 0));
    }
    auto stbl = Description(std::make_shared<ISOBMFF::MP4A>());
    stbl->GetTypedBox<ISOBMFF::STSD>("stsd")->AddBox(
        std::make_shared<ISOBMFF::EC3>());
    AudioTrack receiver;
    EXPECT_NE(0, receiver.parse_audio_information(stbl, file, 0));
  }
}

TEST(LiblcvmGenericTracksTest,
     UnsupportedTimingCannotLookLikeZeroMeasurements) {
  const auto file = Parse("unknown_both.mp4");
  ASSERT_NE(nullptr, file);
  for (const TimingInformation* timing :
       {&file->get_audio_track(99)->get_timing(),
        &file->get_video_track(77)->get_timing()}) {
    EXPECT_EQ(-1, timing->get_num_frames());
    EXPECT_EQ(-1, timing->get_num_keyframes());
    EXPECT_EQ(-1, timing->get_frame_drop_count());
    EXPECT_TRUE(std::isnan(timing->get_key_frame_ratio()));
    EXPECT_TRUE(std::isnan(timing->get_pts_duration_sec_average()));
    EXPECT_TRUE(std::isnan(timing->get_pts_duration_sec_median()));
    EXPECT_TRUE(std::isnan(timing->get_pts_duration_sec_stddev()));
    EXPECT_TRUE(std::isnan(timing->get_pts_duration_sec_mad()));
    EXPECT_TRUE(std::isnan(timing->get_frame_rate_fps_median()));
    EXPECT_TRUE(std::isnan(timing->get_frame_rate_fps_average()));
    EXPECT_TRUE(std::isnan(timing->get_frame_rate_fps_reverse_average()));
    EXPECT_TRUE(std::isnan(timing->get_frame_rate_fps_stddev()));
    EXPECT_TRUE(std::isnan(timing->get_frame_drop_ratio()));
    EXPECT_TRUE(std::isnan(timing->get_normalized_frame_drop_average_length()));
  }
  auto view = file->get_timing(99, 77);
  std::vector<double> percentiles;
  view.calculate_percentile_list({50, 90}, percentiles, 0);
  ASSERT_EQ(2u, percentiles.size());
  EXPECT_TRUE(std::isnan(percentiles[0]));
  EXPECT_TRUE(std::isnan(percentiles[1]));
  std::vector<long int> consecutive;
  view.calculate_consecutive_list({2, 5}, consecutive, 0);
  EXPECT_EQ((std::vector<long int>{-1, -1}), consecutive);
  EXPECT_EQ(120, file->get_video_track(11)->get_timing().get_num_frames());
  EXPECT_DOUBLE_EQ(
      30, file->get_video_track(11)->get_timing().get_frame_rate_fps_median());
  LiblcvmKeyList keys, timing_keys;
  LiblcvmValList values;
  LiblcvmTimingList timing_values;
  ASSERT_EQ(0, IsobmffFileInformation::LiblcvmConfig_to_lists(
                   file, &keys, &values, false, &timing_keys, &timing_values, 0,
                   -1, true));
  std::map<std::string, LiblcvmValue> fields;
  for (size_t i = 0; i < keys.size(); ++i)
    fields[keys[i]] = values[i];
  EXPECT_EQ(-1, std::get<int>(fields.at("video_freeze_v77_a31")));
  EXPECT_EQ(-1, std::get<int>(fields.at("video_freeze_v11_a99")));
  EXPECT_EQ(0, std::get<int>(fields.at("video_freeze_v11_a31")));
}
