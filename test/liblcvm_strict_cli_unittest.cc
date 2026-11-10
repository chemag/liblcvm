#include <gtest/gtest.h>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace {
std::string Quote(const std::string& value) {
#ifdef _WIN32
  return "\"" + value + "\"";
#else
  std::string result = "'";
  for (const char c : value)
    result += c == '\'' ? "'\\''" : std::string(1, c);
  return result + "'";
#endif
}

struct CliResult {
  int status;
  std::string output;
};

CliResult Cli(const std::string& name, const std::string& options = "") {
  std::string cmd = Quote(LCVM_TOOL_PATH) + " " + options + " " +
                    Quote(std::string(TEST_REGRESSION_DIR) + "/" + name);
#ifdef _WIN32
  cmd += " 2>NUL";
  FILE* pipe = _popen(cmd.c_str(), "r");
#else
  cmd += " 2>/dev/null";
  FILE* pipe = popen(cmd.c_str(), "r");
#endif
  if (!pipe)
    return {-1, ""};
  std::string text;
  char buffer[4096];
  while (fgets(buffer, sizeof(buffer), pipe))
    text += buffer;
#ifdef _WIN32
  return {_pclose(pipe), text};
#else
  return {pclose(pipe), text};
#endif
}
}  // namespace

TEST(LiblcvmStrictCliTest, StrictIsDefaultAndOnlyToolPolicy) {
  for (const auto& name : {"unknown_audio_last.mp4", "unknown_video_first.mp4",
                           "unknown_both.mp4"}) {
    SCOPED_TRACE(name);
    auto original = Cli(name);
    EXPECT_NE(0, original.status);
    EXPECT_TRUE(original.output.empty());
    EXPECT_NE(0, Cli(name, "--strict").status);
    const auto skipped = Cli(name, "--strict --skip-audio-parsing");
    if (std::string(name) == "unknown_audio_last.mp4") {
      EXPECT_EQ(0, skipped.status);
    } else {
      EXPECT_NE(0, skipped.status);
    }
    auto result = Cli(name, "--no-strict");
    ASSERT_EQ(0, result.status);
    EXPECT_NE(std::string::npos, result.output.find("audio_type_a31"));
    EXPECT_NE(std::string::npos, result.output.find("audio_type_a41"));
    EXPECT_NE(std::string::npos, result.output.find("mp4a"));
    EXPECT_NE(std::string::npos, result.output.find("Opus"));
    if (std::string(name) != "unknown_video_first.mp4") {
      EXPECT_NE(std::string::npos, result.output.find("_a99"));
    }
    if (std::string(name) != "unknown_audio_last.mp4") {
      EXPECT_NE(std::string::npos, result.output.find("_v77"));
    }
    if (std::string(name) != "unknown_video_first.mp4") {
      EXPECT_NE(std::string::npos, result.output.find("fLaC"));
    }
  }
}

TEST(LiblcvmStrictCliTest, ModeOrderingAndExplicitSelectorErrors) {
  EXPECT_EQ(0, Cli("unknown_audio_last.mp4", "--strict --no-strict").status);
  EXPECT_NE(0, Cli("unknown_audio_last.mp4", "--no-strict --strict").status);
  EXPECT_EQ(
      0,
      Cli("unknown_audio_last.mp4", "--no-strict --audio-track-id 41").status);
  EXPECT_EQ(
      0,
      Cli("unknown_audio_last.mp4", "--no-strict --audio-track-id 99").status);
  EXPECT_EQ(0, Cli("only_unknown_video.mp4", "--no-strict").status);
  EXPECT_EQ(
      0,
      Cli("unknown_video_first.mp4", "--no-strict --video-track-id 77").status);
  EXPECT_EQ(0, Cli("only_unknown_audio.mp4", "--no-strict").status);
  EXPECT_NE(
      0, Cli("unknown_both.mp4", "--no-strict --audio-track-id 12345").status);
  EXPECT_NE(
      0, Cli("unknown_both.mp4", "--no-strict --video-track-id 12345").status);
  EXPECT_EQ(0, Cli("unknown_both.mp4",
                   "--no-strict --audio-track-id 31 --video-track-id 11")
                   .status);
}

TEST(LiblcvmStrictCliTest, MalformedSupportedFormatsFailInBothModes) {
  for (const auto& name :
       {"malformed_opus_with_unknown.mp4", "missing_avcc_with_unknown.mp4"}) {
    SCOPED_TRACE(name);
    EXPECT_NE(0, Cli(name, "--strict").status);
    EXPECT_NE(0, Cli(name, "--no-strict").status);
  }
}

TEST(LiblcvmStrictCliTest, SupportedFileOutputIsModeIndependent) {
  const auto baseline = Cli("supported_aac_opus.mp4");
  ASSERT_EQ(0, baseline.status);
  for (const auto& mode : {"--strict", "--no-strict"}) {
    const auto result = Cli("supported_aac_opus.mp4", mode);
    ASSERT_EQ(0, result.status);
    EXPECT_EQ(baseline.output, result.output);
  }
}

TEST(LiblcvmStrictCliTest, GenericDescriptorsRemainInAudioCardinality) {
  for (const auto& name : {"missing_audio_stsd.mp4", "empty_audio_stsd.mp4",
                           "generic_audio_children.mp4"}) {
    SCOPED_TRACE(name);
    EXPECT_NE(0, Cli(name, "--strict").status);
    const auto result = Cli(name, "--no-strict");
    ASSERT_EQ(0, result.status);
    const auto header = result.output.substr(0, result.output.find('\n'));
    EXPECT_NE(std::string::npos, header.find("_a99"));
    EXPECT_NE(std::string::npos, header.find("_a31"));
    EXPECT_NE(std::string::npos, header.find("audio_type"));
    EXPECT_NE(std::string::npos, result.output.find("mp4a"));
    if (std::string(name) == "generic_audio_children.mp4")
      EXPECT_NE(std::string::npos, result.output.find("fLaC"));
    else
      EXPECT_NE(std::string::npos, result.output.find("none"));
    EXPECT_EQ(0, Cli(name, "--no-strict --audio-track-id 99").status);
  }
}

TEST(LiblcvmStrictCliTest, SkipOmitsAllAudioInEitherToolMode) {
  for (const auto& mode : {"--strict", "--no-strict"}) {
    for (const auto& name :
         {"unknown_audio_first.mp4", "missing_audio_stsd.mp4",
          "skipped_audio_no_metadata.mp4", "supported_aac_opus.mp4"}) {
      SCOPED_TRACE(mode);
      SCOPED_TRACE(name);
      const auto result =
          Cli(name, std::string(mode) + " --skip-audio-parsing");
      ASSERT_EQ(0, result.status);
      EXPECT_EQ(std::string::npos, result.output.find("duration_audio_sec"));
      EXPECT_EQ(std::string::npos, result.output.find("audio_video_ratio"));
      EXPECT_EQ(std::string::npos, result.output.find("audio_type"));
      EXPECT_EQ(std::string::npos, result.output.find("fLaC"));
      EXPECT_NE(std::string::npos, result.output.find("num_video_frames"));
      EXPECT_NE(0, Cli(name, std::string(mode) +
                                 " --skip-audio-parsing --audio-track-id 31")
                       .status);
    }
  }
}

TEST(LiblcvmStrictCliTest,
     GenericTimestampSelectionIsEmptyUnlessSupportedIdIsExplicit) {
  const auto dir =
      std::filesystem::temp_directory_path() /
      ("lcvm-strict-ts-" +
       std::to_string(
           std::chrono::steady_clock::now().time_since_epoch().count()));
  ASSERT_TRUE(std::filesystem::create_directory(dir));
  const auto path = dir / "timestamps.csv";
  const auto result =
      Cli("unknown_video_first.mp4",
          "--no-strict --outfile-timestamps " + Quote(path.string()));
  ASSERT_EQ(0, result.status);
  std::ifstream in(path);
  std::string line;
  size_t lines = 0;
  while (std::getline(in, line))
    ++lines;
  EXPECT_EQ(1u, lines);
  in.close();
  const auto supported =
      Cli("unknown_video_first.mp4",
          "--no-strict --video-track-id 11 --outfile-timestamps " +
              Quote(path.string()));
  ASSERT_EQ(0, supported.status);
  std::ifstream selected(path);
  lines = 0;
  while (std::getline(selected, line))
    ++lines;
  EXPECT_EQ(121u, lines);
  selected.close();
  std::filesystem::remove_all(dir);
}

#if ADD_POLICY
TEST(LiblcvmStrictCliTest, PolicySeesReportedGenericRecordsWithInvalidValues) {
  const auto dir =
      std::filesystem::temp_directory_path() /
      ("lcvm-strict-policy-" +
       std::to_string(
           std::chrono::steady_clock::now().time_since_epoch().count()));
  ASSERT_TRUE(std::filesystem::create_directory(dir));
  const auto path = dir / "rules.policy";
  {
    std::ofstream out(path);
    out << "error \"IGNORED_GENERIC_VIDEO\" width_v77 < 0\nwarn "
           "\"KNOWN_VIDEO\" width_v11 > 0\n";
  }
  auto result =
      Cli("unknown_video_first.mp4", "--no-strict -p " + Quote(path.string()));
  ASSERT_EQ(0, result.status);
  EXPECT_NE(std::string::npos, result.output.find("IGNORED_GENERIC_VIDEO"));
  EXPECT_NE(std::string::npos, result.output.find("KNOWN_VIDEO"));
  {
    std::ofstream out(path);
    out << "error \"GENERIC_AUDIO_ALIAS\" channel_count < 0\nerror "
           "\"GENERIC_AUDIO_ID\" sample_rate_a99 < 0\nwarn \"ABSENT_AUDIO\" "
           "audio_track_id < 0\n";
  }
  result =
      Cli("only_unknown_audio.mp4", "--no-strict -p " + Quote(path.string()));
  ASSERT_EQ(0, result.status);
  EXPECT_NE(std::string::npos, result.output.find("GENERIC_AUDIO_ALIAS"));
  EXPECT_NE(std::string::npos, result.output.find("GENERIC_AUDIO_ID"));
  EXPECT_EQ(std::string::npos, result.output.find("ABSENT_AUDIO"));
  std::filesystem::remove_all(dir);
}
#endif

TEST(LiblcvmStrictCliTest, UnknownOnlyVideoProducesARecordNotAnError) {
  const auto result = Cli("only_unknown_video.mp4", "--no-strict");
  ASSERT_EQ(0, result.status);
  EXPECT_NE(std::string::npos, result.output.find("video_codec_type"));
  EXPECT_NE(std::string::npos, result.output.find("zzzz"));
  EXPECT_NE(std::string::npos, result.output.find("duration_video_sec"));
  EXPECT_EQ(std::string::npos, result.output.find("_v77"));
  EXPECT_NE(0, Cli("only_unknown_video.mp4", "--strict").status);
  EXPECT_NE(0,
            Cli("only_unknown_video.mp4", "--no-strict --video-track-id 12345")
                .status);
}
