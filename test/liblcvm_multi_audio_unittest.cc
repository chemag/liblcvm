// Multi-audio selection and qualified-output regressions.
#include <gtest/gtest.h>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "liblcvm.h"

namespace {
auto Parse(const std::string& name, bool skip = false) {
  LiblcvmConfig config;
  config.set_skip_audio_parsing(skip);
  return IsobmffFileInformation::parse(
      (std::string(TEST_REGRESSION_DIR) + "/" + name).c_str(), config);
}

std::map<std::string, LiblcvmValue> Fields(
    const std::shared_ptr<IsobmffFileInformation>& file, int64_t id = -1,
    bool expanded = true) {
  LiblcvmKeyList keys, tk;
  LiblcvmValList vals;
  LiblcvmTimingList tv;
  EXPECT_EQ(0, IsobmffFileInformation::LiblcvmConfig_to_lists(
                   file, &keys, &vals, false, &tk, &tv, 0, id, expanded));
  EXPECT_EQ(keys.size(), vals.size());
  std::map<std::string, LiblcvmValue> fields;
  for (size_t i = 0; i < keys.size(); ++i) {
    EXPECT_TRUE(fields.emplace(keys[i], vals[i]).second) << keys[i];
  }
  return fields;
}

std::string Text(const LiblcvmValue& value) {
  std::string s;
  EXPECT_EQ(0, liblcvmvalue_to_string(value, &s));
  return s;
}

std::vector<std::string> CsvRow(const std::string& line) {
  std::vector<std::string> cells;
  std::string cell;
  bool quote = false;
  for (size_t i = 0; i < line.size(); ++i) {
    const char c = line[i];
    if (c == '"') {
      if (quote && i + 1 < line.size() && line[i + 1] == '"') {
        cell += '"';
        ++i;
      } else
        quote = !quote;
    } else if (c == ',' && !quote) {
      cells.push_back(cell);
      cell.clear();
    } else if (c != '\r')
      cell += c;
  }
  cells.push_back(cell);
  return cells;
}

std::string Quote(const std::string& s) {
#ifdef _WIN32
  return "\"" + s + "\"";
#else
  std::string result = "'";
  for (char c : s) result += c == '\'' ? "'\\''" : std::string(1, c);
  return result + "'";
#endif
}

struct CliResult {
  int status;
  std::string output;
};
CliResult Cli(const std::vector<std::string>& fixtures,
              const std::string& options = "") {
  std::string cmd = Quote(LCVM_TOOL_PATH) + " " + options;
  for (const auto& f : fixtures) {
    cmd += " " + Quote(std::string(TEST_REGRESSION_DIR) + "/" + f);
  }
#ifdef _WIN32
  cmd += " 2>NUL";
  FILE* pipe = _popen(cmd.c_str(), "r");
#else
  cmd += " 2>/dev/null";
  FILE* pipe = popen(cmd.c_str(), "r");
#endif
  if (!pipe) return {-1, ""};
  std::string text;
  char buffer[4096];
  while (fgets(buffer, sizeof(buffer), pipe)) text += buffer;
#ifdef _WIN32
  const int status = _pclose(pipe);
#else
  const int status = pclose(pipe);
#endif
  return {status, text};
}

std::vector<std::map<std::string, std::string>> Rows(const std::string& csv) {
  std::istringstream in(csv);
  std::string line;
  if (!std::getline(in, line)) return {};
  const auto keys = CsvRow(line);
  EXPECT_EQ(keys.size(),
            std::set<std::string>(keys.begin(), keys.end()).size());
  std::vector<std::map<std::string, std::string>> rows;
  while (std::getline(in, line)) {
    auto vals = CsvRow(line);
    EXPECT_EQ(keys.size(), vals.size());
    if (keys.size() != vals.size()) continue;
    std::map<std::string, std::string> row;
    for (size_t i = 0; i < keys.size(); ++i) row[keys[i]] = vals[i];
    rows.push_back(row);
  }
  return rows;
}
}  // namespace

TEST(LiblcvmMultiAudioTest, FirstMeansContainerOrderNotSmallestId) {
  auto file = Parse("eac3_then_aac.mp4");
  ASSERT_NE(nullptr, file);
  EXPECT_EQ(0xf1234567u, file->get_audio().get_track_id());
  EXPECT_EQ(file->get_audio().get_track_id(),
            file->get_audio(-1).get_track_id());
  EXPECT_EQ("ec-3", file->get_audio().get_audio_type());
  EXPECT_EQ("mp4a", file->get_audio(23).get_audio_type());
  EXPECT_DOUBLE_EQ(4.5, file->get_timing().get_duration_audio_sec());
  EXPECT_DOUBLE_EQ(4.0, file->get_timing(23).get_duration_audio_sec());
  EXPECT_DOUBLE_EQ(1.125, file->get_timing().get_audio_video_ratio());
  EXPECT_DOUBLE_EQ(1.0, file->get_timing(23).get_audio_video_ratio());
  EXPECT_TRUE(file->get_timing().get_video_freeze());
  EXPECT_FALSE(file->get_timing(23).get_video_freeze());
  EXPECT_DOUBLE_EQ(4.5, file->get_timing().get_duration_audio_sec());
}

TEST(LiblcvmMultiAudioTest, MaxUint32IdDoesNotCollideWithMinusOne) {
  auto file = Parse("multi_audio_max_id.mp4");
  ASSERT_NE(nullptr, file);
  EXPECT_EQ(UINT32_MAX, file->get_audio().get_track_id());
  EXPECT_EQ(UINT32_MAX,
            file->get_audio(static_cast<int64_t>(UINT32_MAX)).get_track_id());
  EXPECT_EQ(44100, file->get_audio(23).get_sample_rate());
  EXPECT_EQ(44100u, file->get_timing(23).get_timescale_audio_hz());
  EXPECT_EQ(48000u, file->get_timing().get_timescale_audio_hz());
}

TEST(LiblcvmMultiAudioTest, MissingAndInvalidSelectorsNeverFallBack) {
  auto file = Parse("multi_audio_max_id.mp4");
  ASSERT_NE(nullptr, file);
  for (int64_t id : {int64_t(-2), int64_t(1), int64_t(UINT32_MAX) + 1,
                     std::numeric_limits<int64_t>::min(),
                     std::numeric_limits<int64_t>::max()}) {
    EXPECT_THROW(file->get_audio(id), std::out_of_range);
    EXPECT_THROW(file->get_timing(id), std::out_of_range);
    LiblcvmKeyList keys = {"stale"}, tk;
    LiblcvmValList vals = {1};
    LiblcvmTimingList tv;
    EXPECT_NE(0, IsobmffFileInformation::LiblcvmConfig_to_lists(
                     file, &keys, &vals, false, &tk, &tv, 0, id, true));
    EXPECT_TRUE(keys.empty());
    EXPECT_TRUE(vals.empty());
  }
}

TEST(LiblcvmMultiAudioTest, QualifiedKeysAndAliasesAreConsistent) {
  auto file = Parse("multi_audio_max_id.mp4");
  ASSERT_NE(nullptr, file);
  auto first = Fields(file);
  auto second = Fields(file, 23);
  EXPECT_EQ("ec-3", Text(first.at("audio_type")));
  EXPECT_EQ("mp4a", Text(second.at("audio_type")));
  EXPECT_EQ("4294967295", Text(first.at("audio_track_id")));
  EXPECT_EQ("23", Text(second.at("audio_track_id")));
  const std::vector<std::string> audio_keys = {
      "audio_type",  "channel_count",      "sample_rate",
      "sample_size", "duration_audio_sec", "timescale_audio_hz"};
  for (const auto& key : audio_keys) {
    EXPECT_EQ(Text(first.at(key)), Text(first.at(key + "_a4294967295")));
    EXPECT_EQ(Text(second.at(key)), Text(second.at(key + "_a23")));
    EXPECT_EQ(Text(first.at(key + "_a23")), Text(second.at(key + "_a23")));
    EXPECT_EQ(Text(first.at(key + "_a4294967295")),
              Text(second.at(key + "_a4294967295")));
  }
  for (const auto& key : {"video_freeze", "audio_video_ratio"}) {
    EXPECT_EQ(Text(first.at(key)),
              Text(first.at(std::string(key) + "_v1_a4294967295")));
    EXPECT_EQ(Text(second.at(key)),
              Text(second.at(std::string(key) + "_v1_a23")));
  }
  EXPECT_EQ(Text(first.at("chroma_format")),
            Text(first.at("chroma_format_v1")));
  EXPECT_EQ(0u, first.count("filesize_v1"));
  EXPECT_EQ(0u, first.count("bitrate_bps_v1"));
  EXPECT_EQ(0u, first.count("timescale_movie_hz_v1"));
  auto legacy = Fields(file, -1, false);
  EXPECT_EQ(0u, legacy.count("sample_rate_a23"));
  EXPECT_EQ(0u, legacy.count("audio_track_id"));
}

TEST(LiblcvmMultiAudioTest, SkipRetainsEachMdhdDurationAndIdentity) {
  auto file = Parse("multi_audio_max_id.mp4", true);
  ASSERT_NE(nullptr, file);
  ASSERT_EQ(2u, file->get_audio_tracks().size());
  EXPECT_EQ(UINT32_MAX, file->get_audio().get_track_id());
  EXPECT_EQ(23u, file->get_audio(23).get_track_id());
  EXPECT_EQ("", file->get_audio(23).get_audio_type());
  EXPECT_EQ(0, file->get_audio(23).get_channel_count());
  EXPECT_DOUBLE_EQ(4.5, file->get_timing().get_duration_audio_sec());
  EXPECT_DOUBLE_EQ(5.0, file->get_timing(23).get_duration_audio_sec());
  auto fields = Fields(file);
  EXPECT_EQ("5.000000", Text(fields.at("duration_audio_sec_a23")));
  EXPECT_EQ("0", Text(fields.at("sample_rate_a23")));
}

TEST(LiblcvmMultiAudioTest, NoAudioKeepsDefaultButRejectsExplicitId) {
  auto file = Parse("multi_audio_no_audio.mp4");
  ASSERT_NE(nullptr, file);
  EXPECT_TRUE(file->get_audio_tracks().empty());
  EXPECT_EQ("", file->get_audio().get_audio_type());
  EXPECT_DOUBLE_EQ(-1.0, file->get_timing().get_duration_audio_sec());
  EXPECT_DOUBLE_EQ(-1.0, file->get_timing().get_audio_video_ratio());
  EXPECT_FALSE(file->get_timing().get_video_freeze());
  EXPECT_THROW(file->get_audio(0), std::out_of_range);
  auto fields = Fields(file);
  EXPECT_EQ("-1", Text(fields.at("audio_track_id")));
  EXPECT_NE(0u, fields.count("chroma_format_v1"));
  for (const auto& entry : fields) {
    const auto pos = entry.first.rfind("_a");
    if (pos != std::string::npos && pos + 2 < entry.first.size()) {
      const char next = entry.first[pos + 2];
      EXPECT_FALSE(next >= '0' && next <= '9');
    }
  }
}

TEST(LiblcvmMultiAudioTest, DuplicateIdsRejectedWithAndWithoutSkip) {
  EXPECT_EQ(nullptr, Parse("multi_audio_duplicate_id.mp4"));
  EXPECT_EQ(nullptr, Parse("multi_audio_duplicate_id.mp4", true));
}

TEST(LiblcvmMultiAudioTest, PublicHelperRefreshesParsedFirstTrackConsistently) {
  auto file = Parse("multi_audio_max_id.mp4");
  ASSERT_NE(nullptr, file);
  auto stbl = std::make_shared<ISOBMFF::ContainerBox>("stbl");
  auto stsd = std::make_shared<ISOBMFF::STSD>();
  auto aac = std::make_shared<ISOBMFF::MP4A>();
  aac->SetChannelCount(2);
  aac->SetSampleSize(16);
  aac->SetSampleRateRaw(32000u << 16);
  stsd->AddBox(aac);
  stbl->AddBox(stsd);
  ASSERT_EQ(0, file->get_audio().parse_audio_information(stbl, file, 0));
  EXPECT_EQ(UINT32_MAX, file->get_audio().get_track_id());
  EXPECT_EQ(32000, file->get_audio().get_sample_rate());
  EXPECT_EQ(32000, file->get_audio(int64_t(UINT32_MAX)).get_sample_rate());
  EXPECT_EQ(44100, file->get_audio(23).get_sample_rate());
  EXPECT_EQ(nullptr, file->get_audio().get_eac3_configuration());
  EXPECT_DOUBLE_EQ(4.5, file->get_audio().get_duration_sec());
  EXPECT_DOUBLE_EQ(4.5, file->get_timing().get_duration_audio_sec());
  auto fields = Fields(file);
  EXPECT_EQ(Text(fields.at("sample_rate")),
            Text(fields.at("sample_rate_a4294967295")));
  auto empty = std::make_shared<ISOBMFF::ContainerBox>("stbl");
  EXPECT_NE(0, file->get_audio().parse_audio_information(empty, file, 0));
  EXPECT_EQ(32000, file->get_audio().get_sample_rate());
  EXPECT_EQ(32000, file->get_audio(int64_t(UINT32_MAX)).get_sample_rate());
}

TEST(LiblcvmMultiAudioTest,
     SkippedUnsupportedAudioCanBeSelectedWithoutFakeMetadata) {
  auto file = Parse("unsupported_then_aac.mp4", true);
  ASSERT_NE(nullptr, file);
  EXPECT_EQ(2u, file->get_audio(2).get_track_id());
  EXPECT_EQ("", file->get_audio(2).get_audio_type());
  EXPECT_EQ(0, file->get_audio(2).get_sample_rate());
  EXPECT_DOUBLE_EQ(4.5, file->get_timing(2).get_duration_audio_sec());
  EXPECT_EQ(nullptr, Parse("unsupported_then_aac.mp4", false));
}

TEST(LiblcvmMultiAudioTest, VideoQualificationUsesStoredIdNotPosition) {
  auto file = Parse("multi_audio_video_id7.mp4");
  ASSERT_NE(nullptr, file);
  EXPECT_EQ(7u, file->get_frame().get_track_id());
  auto fields = Fields(file);
  EXPECT_NE(0u, fields.count("chroma_format_v7"));
  EXPECT_NE(0u, fields.count("audio_video_ratio_v7_a23"));
  EXPECT_NE(0u, fields.count("video_freeze_v7_a4294967295"));
  EXPECT_EQ(0u, fields.count("chroma_format_v1"));
}

TEST(LiblcvmMultiAudioTest, MdhdFallbackPreservedForEverySelectedTrack) {
  auto file = Parse("multi_audio_mdhd_fallback.mp4");
  ASSERT_NE(nullptr, file);
  EXPECT_EQ(0u, file->get_timing().get_timescale_movie_hz());
  EXPECT_DOUBLE_EQ(4.5, file->get_audio().get_duration_sec());
  EXPECT_DOUBLE_EQ(5.0, file->get_audio(23).get_duration_sec());
  EXPECT_DOUBLE_EQ(5.0, file->get_timing(23).get_duration_audio_sec());
}

TEST(LiblcvmMultiAudioCliTest, DefaultsAndExplicitSelection) {
  auto result = Cli({"multi_audio_max_id.mp4"});
  ASSERT_EQ(0, result.status);
  auto rows = Rows(result.output);
  ASSERT_EQ(1u, rows.size());
  EXPECT_EQ("ec-3", rows[0]["audio_type"]);
  EXPECT_EQ("mp4a", rows[0]["audio_type_a23"]);
  result = Cli({"multi_audio_max_id.mp4"}, "--audio-track-id 23");
  ASSERT_EQ(0, result.status);
  rows = Rows(result.output);
  ASSERT_EQ(1u, rows.size());
  EXPECT_EQ("mp4a", rows[0]["audio_type"]);
  EXPECT_EQ("44100", rows[0]["sample_rate"]);
  EXPECT_EQ("ec-3", rows[0]["audio_type_a4294967295"]);
  EXPECT_EQ("1", rows[0]["video_freeze_v1_a4294967295"]);
  EXPECT_EQ("0", rows[0]["video_freeze_v1_a23"]);
}

TEST(LiblcvmMultiAudioCliTest, BatchUnionHeaderKeepsRowsAligned) {
  auto result = Cli({"eac3_then_aac.mp4", "multi_audio_no_audio.mp4",
                     "multi_audio_max_id.mp4"});
  ASSERT_EQ(0, result.status);
  auto rows = Rows(result.output);
  ASSERT_EQ(3u, rows.size());
  EXPECT_EQ("ec-3", rows[0]["audio_type_a4045620583"]);
  EXPECT_EQ("", rows[0]["audio_type_a4294967295"]);
  EXPECT_EQ("", rows[1]["audio_type_a4045620583"]);
  EXPECT_EQ("", rows[1]["audio_type_a23"]);
  EXPECT_EQ("ec-3", rows[2]["audio_type_a4294967295"]);
  EXPECT_EQ("", rows[2]["audio_type_a4045620583"]);
}

TEST(LiblcvmMultiAudioCliTest, MissingAndInvalidIdsFailClearly) {
  for (const auto& id :
       {"1", "-2", "4294967296", "9223372036854775808", "junk"}) {
    EXPECT_NE(0, Cli({"multi_audio_max_id.mp4"},
                     std::string("--audio-track-id ") + id)
                     .status);
  }
  EXPECT_EQ(
      0, Cli({"multi_audio_max_id.mp4"}, "--audio-track-id 4294967295").status);
  EXPECT_EQ(0, Cli({"multi_audio_max_id.mp4"}, "--audio-track-id -1").status);
}

TEST(LiblcvmMultiAudioCliTest, FailedLastSelectionPreservesTimestampHeader) {
  const auto dir =
      std::filesystem::temp_directory_path() /
      ("liblcvm-multi-audio-ts-" +
       std::to_string(
           std::chrono::steady_clock::now().time_since_epoch().count()));
  ASSERT_TRUE(std::filesystem::create_directory(dir));
  const auto path = dir / "timestamps.csv";
  auto result =
      Cli({"multi_audio_max_id.mp4", "multi_audio_no_audio.mp4"},
          "--audio-track-id 23 --outfile-timestamps " + Quote(path.string()));
  EXPECT_NE(0, result.status);
  std::ifstream in(path);
  std::ostringstream text;
  text << in.rdbuf();
  auto rows = Rows(text.str());
  ASSERT_EQ(120u, rows.size());
  EXPECT_EQ(10u, rows[0].size());
  EXPECT_EQ("0", rows[0]["frame_num"]);
  EXPECT_NE(0u, rows[0].count("stts"));
  EXPECT_NE(0u, rows[0].count("pts_framerate"));
  in.close();
  std::filesystem::remove(path);
  std::filesystem::remove(dir);
}

TEST(LiblcvmMultiAudioCliTest, PartialFailuresDoNotMisalignSuccessfulRows) {
  auto result = Cli({"multi_audio_max_id.mp4", "unsupported_then_aac.mp4",
                     "multi_audio_no_audio.mp4"});
  EXPECT_NE(0, result.status);
  auto rows = Rows(result.output);
  ASSERT_EQ(2u, rows.size());
  EXPECT_EQ("ec-3", rows[0]["audio_type"]);
  EXPECT_EQ("", rows[1]["audio_type_a4294967295"]);
}
