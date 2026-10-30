// E-AC-3 / Atmos metadata regressions, not audio decoding tests.

#include <gtest/gtest.h>

#include <map>
#include <memory>
#include <string>

#include "liblcvm.h"

namespace {
std::shared_ptr<IsobmffFileInformation> ParseEac3(const std::string& name,
                                                  bool skip = false) {
  LiblcvmConfig config;
  config.set_skip_audio_parsing(skip);
  return IsobmffFileInformation::parse(
      (std::string(TEST_REGRESSION_DIR) + "/" + name).c_str(), config);
}
}  // namespace

TEST(LiblcvmEac3Test, ConfigurationNotSampleEntryPlaceholder) {
  for (const auto& name : {"eac3_joc.mp4", "eac3_plain.mp4"}) {
    auto file = ParseEac3(name);
    ASSERT_NE(nullptr, file);
    auto audio = file->get_audio();
    EXPECT_EQ("ec-3", audio.get_audio_type());
    EXPECT_EQ(6, audio.get_channel_count());
    EXPECT_EQ(48000, audio.get_sample_rate());
    EXPECT_EQ(-1, audio.get_sample_size());
    auto dec3 = audio.get_eac3_configuration();
    ASSERT_NE(nullptr, dec3);
    EXPECT_EQ(768, dec3->GetDataRate());
    EXPECT_EQ(std::string(name) == "eac3_joc.mp4",
              dec3->GetFlagEC3ExtensionTypeA());
    EXPECT_DOUBLE_EQ(4.5, audio.get_duration_sec());
    EXPECT_DOUBLE_EQ(4.5, file->get_timing().get_duration_audio_sec());
    EXPECT_DOUBLE_EQ(1.125, file->get_timing().get_audio_video_ratio());
    EXPECT_TRUE(file->get_timing().get_video_freeze());
  }
}

TEST(LiblcvmEac3Test, RealTrackIdsAndAllAudioRetainedInBothOrders) {
  for (bool ec3_first : {true, false}) {
    const std::string name =
        ec3_first ? "eac3_then_aac.mp4" : "aac_then_eac3.mp4";
    auto file = ParseEac3(name);
    ASSERT_NE(nullptr, file);
    auto tracks = file->get_audio_tracks();
    ASSERT_EQ(2u, tracks.size());
    const auto& ec3 = tracks[ec3_first ? 0 : 1];
    const auto& aac = tracks[ec3_first ? 1 : 0];
    EXPECT_EQ(0xf1234567u, ec3.get_track_id());
    EXPECT_EQ(23u, aac.get_track_id());
    EXPECT_EQ("ec-3", ec3.get_audio_type());
    EXPECT_EQ(6, ec3.get_channel_count());
    ASSERT_NE(nullptr, ec3.get_eac3_configuration());
    EXPECT_TRUE(ec3.get_eac3_configuration()->GetFlagEC3ExtensionTypeA());
    EXPECT_EQ(12, ec3.get_eac3_configuration()->GetComplexityIndexTypeA());
    EXPECT_EQ("mp4a", aac.get_audio_type());
    EXPECT_EQ(2, aac.get_channel_count());
    EXPECT_EQ(16, aac.get_sample_size());
    EXPECT_EQ(nullptr, aac.get_eac3_configuration());
    EXPECT_EQ(48000u, ec3.get_timescale_hz());
    EXPECT_EQ(48000u, aac.get_timescale_hz());
    EXPECT_DOUBLE_EQ(4.5, ec3.get_duration_sec());
    EXPECT_DOUBLE_EQ(4.5, ec3.get_mdhd_duration_sec());
    EXPECT_DOUBLE_EQ(4.0, aac.get_duration_sec());
    EXPECT_DOUBLE_EQ(4.125, aac.get_mdhd_duration_sec());
    // Document and preserve the existing last-audio-track selection limitation.
    // This patch neither chooses AAC nor normalizes differing track durations.
    EXPECT_EQ(tracks.back().get_track_id(), file->get_audio().get_track_id());
    EXPECT_EQ(ec3_first ? "mp4a" : "ec-3", file->get_audio().get_audio_type());
    EXPECT_DOUBLE_EQ(ec3_first ? 4.0 : 4.5,
                     file->get_timing().get_duration_audio_sec());
    EXPECT_DOUBLE_EQ(ec3_first ? 1.0 : 1.125,
                     file->get_timing().get_audio_video_ratio());
    EXPECT_EQ(!ec3_first, file->get_timing().get_video_freeze());
    // Legacy flat output remains last-track based too, with its existing
    // schema.
    LiblcvmKeyList keys, timing_keys;
    LiblcvmValList values;
    LiblcvmTimingList timing_values;
    ASSERT_EQ(
        0, IsobmffFileInformation::LiblcvmConfig_to_lists(
               file, &keys, &values, false, &timing_keys, &timing_values, 0));
    std::map<std::string, LiblcvmValue> fields;
    for (size_t i = 0; i < keys.size(); ++i) fields[keys[i]] = values[i];
    EXPECT_EQ(tracks.back().get_audio_type(),
              std::get<std::string>(fields["audio_type"]));
    EXPECT_EQ(tracks.back().get_channel_count(),
              std::get<int>(fields["channel_count"]));
  }
}

TEST(LiblcvmEac3Test, DependentChannelsAndIndependentServicesNotSummed) {
  auto dependent = ParseEac3("eac3_dependent.mp4");
  ASSERT_NE(nullptr, dependent);
  EXPECT_EQ(8, dependent->get_audio().get_channel_count());
  auto multiple = ParseEac3("eac3_multi_service.mp4");
  ASSERT_NE(nullptr, multiple);
  EXPECT_EQ(6, multiple->get_audio().get_channel_count());
  auto subs = multiple->get_audio()
                  .get_eac3_configuration()
                  ->GetIndependentSubstreams();
  ASSERT_EQ(2u, subs.size());
  EXPECT_TRUE(subs[1].asvc);
  EXPECT_EQ(3, subs[1].bsmod);
  EXPECT_EQ(1, subs[1].GetChannelCount());
}

TEST(LiblcvmEac3Test, HalfRateFallbackAndUnknownRate) {
  auto half = ParseEac3("eac3_half_rate.mp4");
  ASSERT_NE(nullptr, half);
  EXPECT_EQ(24000, half->get_audio().get_sample_rate());
  EXPECT_EQ(0u, half->get_audio()
                    .get_eac3_configuration()
                    ->GetIndependentSubstreams()[0]
                    .GetSampleRate());
  auto unknown = ParseEac3("eac3_unknown_rate.mp4");
  ASSERT_NE(nullptr, unknown);
  EXPECT_EQ(-1, unknown->get_audio().get_sample_rate());
}

TEST(LiblcvmEac3Test, SkipSemanticsAndDefaultAudioAreUnchanged) {
  auto file = ParseEac3("eac3_then_aac.mp4", true);
  ASSERT_NE(nullptr, file);
  EXPECT_TRUE(file->get_audio_tracks().empty());
  EXPECT_EQ("", file->get_audio().get_audio_type());
  EXPECT_EQ(0, file->get_audio().get_channel_count());
  EXPECT_EQ(0, file->get_audio().get_sample_rate());
  EXPECT_EQ(0, file->get_audio().get_sample_size());
  EXPECT_EQ(nullptr, file->get_audio().get_eac3_configuration());
  // Skip still uses last mdhd duration; full parsing still gives tkhd priority.
  EXPECT_DOUBLE_EQ(4.125, file->get_timing().get_duration_audio_sec());
  EXPECT_DOUBLE_EQ(4.125 / 4.0, file->get_timing().get_audio_video_ratio());
}

TEST(LiblcvmEac3Test, MalformedConfigurationNeverHiddenByAacSibling) {
  for (const auto& name :
       {"eac3_truncated_header.mp4", "eac3_truncated_substream.mp4",
        "eac3_truncated_joc.mp4", "eac3_truncated_dependent.mp4",
        "eac3_missing_dec3.mp4", "eac3_duplicate_dec3.mp4"}) {
    SCOPED_TRACE(name);
    EXPECT_EQ(nullptr, ParseEac3(name));
    // Box syntax is always validated by isobmff, even with audio analysis
    // skipped.
    EXPECT_EQ(nullptr, ParseEac3(name, true));
  }
}

TEST(LiblcvmEac3Test, PublicHelperStillUpdatesOwnerNotReceiver) {
  auto owner = std::make_shared<IsobmffFileInformation>();
  auto stbl = std::make_shared<ISOBMFF::ContainerBox>("stbl");
  auto stsd = std::make_shared<ISOBMFF::STSD>();
  auto mp4a = std::make_shared<ISOBMFF::MP4A>();
  mp4a->SetChannelCount(2);
  mp4a->SetSampleSize(16);
  mp4a->SetSampleRateRaw(48000u << 16);
  stsd->AddBox(mp4a);
  stbl->AddBox(stsd);
  AudioInformation receiver;
  ASSERT_EQ(0, receiver.parse_audio_information(stbl, owner, 0));
  EXPECT_EQ("mp4a", owner->get_audio().get_audio_type());
  EXPECT_EQ(48000, owner->get_audio().get_sample_rate());
  EXPECT_EQ("", receiver.get_audio_type());

  auto opus_stbl = std::make_shared<ISOBMFF::ContainerBox>("stbl");
  auto opus_stsd = std::make_shared<ISOBMFF::STSD>();
  auto opus = std::make_shared<ISOBMFF::OPUS>();
  opus->SetChannelCount(5);
  opus->SetSampleSize(16);
  opus->SetSampleRateRaw(48000u << 16);
  opus_stsd->AddBox(opus);
  opus_stbl->AddBox(opus_stsd);
  ASSERT_EQ(0, owner->get_audio().parse_audio_information(opus_stbl, owner, 0));
  EXPECT_EQ("Opus", owner->get_audio().get_audio_type());
  EXPECT_EQ(5, owner->get_audio().get_channel_count());
}

TEST(LiblcvmEac3Test, OtherUnsupportedAudioStillFailsInEitherOrder) {
  for (const auto& name :
       {"unsupported_then_aac.mp4", "aac_then_unsupported.mp4"}) {
    SCOPED_TRACE(name);
    EXPECT_EQ(nullptr, ParseEac3(name));
    EXPECT_NE(nullptr, ParseEac3(name, true));
  }
}
