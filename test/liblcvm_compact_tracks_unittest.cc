// CLI-only regressions for cardinality-aware multi-video/audio CSV output.
#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {
using Row = std::map<std::string, std::string>;

std::string Quote(const std::string& text) {
#ifdef _WIN32
  return "\"" + text + "\"";
#else
  std::string result = "'";
  for (char c : text)
    result += c == '\'' ? "'\\''" : std::string(1, c);
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
  for (const auto& fixture : fixtures) {
    cmd += " " + Quote(std::string(TEST_REGRESSION_DIR) + "/" + fixture);
  }
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
  const int status = _pclose(pipe);
#else
  const int status = pclose(pipe);
#endif
  return {status, text};
}

std::vector<std::string> CsvRow(const std::string& line) {
  std::vector<std::string> cells;
  std::string cell;
  bool quoted = false;
  for (size_t i = 0; i < line.size(); ++i) {
    const char c = line[i];
    if (c == '"') {
      if (quoted && i + 1 < line.size() && line[i + 1] == '"') {
        cell += '"';
        ++i;
      } else {
        quoted = !quoted;
      }
    } else if (c == ',' && !quoted) {
      cells.push_back(cell);
      cell.clear();
    } else if (c != '\r') {
      cell += c;
    }
  }
  EXPECT_FALSE(quoted);
  cells.push_back(cell);
  return cells;
}

struct Csv {
  std::vector<std::string> header;
  std::vector<Row> rows;
};

Csv ReadCsv(const std::string& text) {
  std::istringstream in(text);
  std::string line;
  Csv csv;
  if (!std::getline(in, line))
    return csv;
  csv.header = CsvRow(line);
  EXPECT_EQ(csv.header.size(),
            std::set<std::string>(csv.header.begin(), csv.header.end()).size());
  while (std::getline(in, line)) {
    const auto values = CsvRow(line);
    EXPECT_EQ(csv.header.size(), values.size());
    if (csv.header.size() != values.size())
      continue;
    Row row;
    for (size_t i = 0; i < values.size(); ++i) {
      row.emplace(csv.header[i], values[i]);
    }
    csv.rows.push_back(std::move(row));
  }
  return csv;
}

void ExpectText(const Row& row, const std::string& key,
                const std::string& value) {
  const auto entry = row.find(key);
  ASSERT_NE(row.end(), entry) << key;
  EXPECT_EQ(value, entry->second) << key;
}

void ExpectNumber(const Row& row, const std::string& key, double value) {
  const auto entry = row.find(key);
  ASSERT_NE(row.end(), entry) << key;
  EXPECT_NEAR(value, std::stod(entry->second), 0.000002) << key;
}

// Existing export fields: only file-wide fields remain unqualified regardless
// of cardinality. All remaining fields belong to video, audio, or a VxA pair.
const std::vector<std::string> kVideoKeys = {
    "width",
    "height",
    "video_codec_type",
    "horizresolution",
    "vertresolution",
    "depth",
    "chroma_format",
    "bit_depth_luma",
    "bit_depth_chroma",
    "video_full_range_flag",
    "colour_primaries",
    "transfer_characteristics",
    "matrix_coeffs",
    "profile_idc",
    "level_idc",
    "profile_type_str",
    "num_video_frames",
    "frame_rate_fps_median",
    "frame_rate_fps_average",
    "frame_rate_fps_reverse_average",
    "frame_rate_fps_stddev",
    "duration_video_sec",
    "timescale_video_hz",
    "pts_duration_sec_average",
    "pts_duration_sec_median",
    "pts_duration_sec_stddev",
    "pts_duration_sec_mad",
    "frame_drop_count",
    "frame_drop_ratio",
    "normalized_frame_drop_average_length",
    "frame_drop_length_percentile_50",
    "frame_drop_length_percentile_90",
    "frame_drop_length_consecutive_2",
    "frame_drop_length_consecutive_5",
    "num_video_keyframes",
    "key_frame_ratio"};
const std::vector<std::string> kAudioKeys = {
    "audio_type",  "channel_count",      "sample_rate",
    "sample_size", "duration_audio_sec", "timescale_audio_hz"};
const std::vector<std::string> kPairKeys = {"audio_video_ratio",
                                            "video_freeze"};
const std::set<std::string> kPolicyKeys = {"policy_version", "warn_list",
                                           "error_list"};

struct Fixture {
  const char* name;
  size_t videos;
  size_t audios;
};
const std::vector<Fixture> kFixtures = {{"multivideo_1v_1a.mp4", 1, 1},
                                        {"multivideo_1v_2a.mp4", 1, 2},
                                        {"multivideo_2v_1a.mp4", 2, 1},
                                        {"multivideo_2v_2a.mp4", 2, 2},
                                        {"multivideo_2v_0a.mp4", 2, 0}};
const std::vector<uint32_t> kVideoIds = {11, 21};
const std::vector<uint32_t> kAudioIds = {31, 41};

std::string VideoSuffix(const Fixture& fixture, size_t index) {
  return fixture.videos == 1 ? "" : "_v" + std::to_string(kVideoIds[index]);
}
std::string AudioSuffix(const Fixture& fixture, size_t index) {
  return fixture.audios == 1 ? "" : "_a" + std::to_string(kAudioIds[index]);
}

void ExpectSchema(const Fixture& fixture, const Row& row) {
  std::set<std::string> expected = {"infile", "filesize", "bitrate_bps",
                                    "timescale_movie_hz"};
  for (size_t v = 0; v < fixture.videos; ++v) {
    const auto vs = VideoSuffix(fixture, v);
    for (const auto& key : kVideoKeys)
      expected.insert(key + vs);
    for (size_t a = 0; a < fixture.audios; ++a) {
      const auto suffix = vs + AudioSuffix(fixture, a);
      for (const auto& key : kPairKeys)
        expected.insert(key + suffix);
    }
  }
  for (size_t a = 0; a < fixture.audios; ++a) {
    const auto suffix = AudioSuffix(fixture, a);
    for (const auto& key : kAudioKeys)
      expected.insert(key + suffix);
  }
  std::set<std::string> actual;
  for (const auto& entry : row) {
    if (kPolicyKeys.count(entry.first) == 0)
      actual.insert(entry.first);
  }
  EXPECT_EQ(expected, actual);
  EXPECT_EQ(0u, row.count("audio_track_id"));
  EXPECT_EQ(0u, row.count("video_track_id"));
}

void ExpectFixtureValues(const Fixture& fixture, const Row& row) {
  ExpectText(row, "infile",
             std::string(TEST_REGRESSION_DIR) + "/" + fixture.name);
  for (size_t v = 0; v < fixture.videos; ++v) {
    const auto vs = VideoSuffix(fixture, v);
    ExpectNumber(row, "width" + vs, v == 0 ? 320 : 640);
    ExpectNumber(row, "height" + vs, v == 0 ? 240 : 360);
    ExpectNumber(row, "num_video_frames" + vs, v == 0 ? 120 : 72);
    ExpectNumber(row, "duration_video_sec" + vs, v == 0 ? 4 : 3);
    ExpectNumber(row, "frame_rate_fps_average" + vs, v == 0 ? 30 : 24);
    for (size_t a = 0; a < fixture.audios; ++a) {
      const auto suffix = vs + AudioSuffix(fixture, a);
      const double ratio =
          a == 0 ? (v == 0 ? 1.125 : 1.5) : (v == 0 ? 0.75 : 1.0);
      ExpectNumber(row, "audio_video_ratio" + suffix, ratio);
      ExpectText(row, "video_freeze" + suffix, a == 0 ? "1" : "0");
    }
  }
  for (size_t a = 0; a < fixture.audios; ++a) {
    const auto suffix = AudioSuffix(fixture, a);
    ExpectText(row, "audio_type" + suffix, a == 0 ? "ec-3" : "mp4a");
    ExpectNumber(row, "channel_count" + suffix, a == 0 ? 6 : 2);
    ExpectNumber(row, "sample_rate" + suffix, a == 0 ? 48000 : 44100);
    // Normal parsing retains tkhd duration rather than the second mdhd's 3.2s.
    ExpectNumber(row, "duration_audio_sec" + suffix, a == 0 ? 4.5 : 3.0);
  }
}

struct TempDirectory {
  std::filesystem::path path =
      std::filesystem::temp_directory_path() /
      ("liblcvm-compact-tracks-" +
       std::to_string(
           std::chrono::steady_clock::now().time_since_epoch().count()));
  TempDirectory() { EXPECT_TRUE(std::filesystem::create_directory(path)); }
  ~TempDirectory() {
    std::error_code error;
    std::filesystem::remove(path / "timestamps.csv", error);
    std::filesystem::remove(path, error);
  }
};

Csv ReadTimestampFile(const std::filesystem::path& path) {
  std::ifstream in(path);
  EXPECT_TRUE(in.is_open());
  std::ostringstream text;
  text << in.rdbuf();
  return ReadCsv(text.str());
}

const std::vector<std::string> kTimestampHeader = {
    "filename",     "frame_num",    "frame_num_orig",
    "stts",         "ctts",         "dts",
    "pts",          "pts_duration", "pts_duration_delta",
    "pts_framerate"};
}  // namespace

TEST(LiblcvmCompactTracksCliTest, AllCardinalitiesHaveExactlyOneCanonicalView) {
  for (const auto& fixture : kFixtures) {
    SCOPED_TRACE(fixture.name);
    const auto result = Cli({fixture.name});
    ASSERT_EQ(0, result.status);
    const auto csv = ReadCsv(result.output);
    ASSERT_EQ(1u, csv.rows.size());
    ExpectSchema(fixture, csv.rows[0]);
    ExpectFixtureValues(fixture, csv.rows[0]);
  }
}

TEST(LiblcvmCompactTracksCliTest, SelectionKeepsEveryTrackAndPair) {
  const auto& fixture = kFixtures[3];
  const auto first = Cli({fixture.name});
  ASSERT_EQ(0, first.status);
  const auto first_csv = ReadCsv(first.output);
  ASSERT_EQ(1u, first_csv.rows.size());
  for (const auto& options :
       {"--video-track-id -1 --audio-track-id -1",
        "--video-track-id 11 --audio-track-id 31", "--audio-track-id 41",
        "--video-track-id 21 --audio-track-id 41"}) {
    SCOPED_TRACE(options);
    const auto result = Cli({fixture.name}, options);
    ASSERT_EQ(0, result.status);
    const auto csv = ReadCsv(result.output);
    ASSERT_EQ(1u, csv.rows.size());
    EXPECT_EQ(first_csv.header, csv.header);
    ExpectSchema(fixture, csv.rows[0]);
    ExpectFixtureValues(fixture, csv.rows[0]);
    auto expected = first_csv.rows[0];
    auto actual = csv.rows[0];
    expected.erase("bitrate_bps");
    actual.erase("bitrate_bps");
    EXPECT_EQ(expected, actual);
    // Keep the existing total-filesize / selected-video-duration estimator.
    const double duration =
        std::string(options).find("video-track-id 21") != std::string::npos
            ? 3.0
            : 4.0;
    ExpectNumber(csv.rows[0], "bitrate_bps",
                 std::stod(csv.rows[0].at("filesize")) * 8.0 / duration);
  }
}

TEST(LiblcvmCompactTracksCliTest, OrderedUnionHeaderHasBlankAbsentCells) {
  std::vector<std::string> names;
  std::vector<Csv> singles;
  std::vector<std::string> expected_header;
  std::set<std::string> seen;
  for (const auto& fixture : kFixtures) {
    names.push_back(fixture.name);
    const auto result = Cli({fixture.name});
    ASSERT_EQ(0, result.status);
    singles.push_back(ReadCsv(result.output));
    ASSERT_EQ(1u, singles.back().rows.size());
    for (const auto& key : singles.back().header) {
      if (seen.insert(key).second)
        expected_header.push_back(key);
    }
  }
  const auto result = Cli(names);
  ASSERT_EQ(0, result.status);
  const auto csv = ReadCsv(result.output);
  EXPECT_EQ(expected_header, csv.header);
  ASSERT_EQ(names.size(), csv.rows.size());
  for (size_t i = 0; i < names.size(); ++i) {
    SCOPED_TRACE(names[i]);
    for (const auto& key : expected_header) {
      const auto value = singles[i].rows[0].find(key);
      ExpectText(csv.rows[i], key,
                 value == singles[i].rows[0].end() ? "" : value->second);
    }
  }
}

TEST(LiblcvmCompactTracksCliTest, SkippedAudioPreservesCountsAndMdhdTiming) {
  const auto& fixture = kFixtures[3];
  const auto result = Cli({fixture.name}, "--skip-audio-parsing");
  ASSERT_EQ(0, result.status);
  const auto csv = ReadCsv(result.output);
  ASSERT_EQ(1u, csv.rows.size());
  const auto& row = csv.rows[0];
  ExpectSchema(fixture, row);
  ExpectText(row, "audio_type_a31", "");
  ExpectText(row, "audio_type_a41", "");
  ExpectNumber(row, "sample_rate_a31", 0);
  ExpectNumber(row, "sample_rate_a41", 0);
  ExpectNumber(row, "duration_audio_sec_a31", 4.5);
  ExpectNumber(row, "duration_audio_sec_a41", 3.2);
  ExpectNumber(row, "audio_video_ratio_v11_a41", 0.8);
  ExpectNumber(row, "audio_video_ratio_v21_a41", 3.2 / 3.0);
  ExpectText(row, "video_freeze_v11_a41", "0");
  ExpectText(row, "video_freeze_v21_a41", "1");
}

TEST(LiblcvmCompactTracksCliTest, InvalidOrMissingVideoIdsNeverFallBack) {
  for (const auto& id :
       {"0", "1", "31", "4294967295", "-2", "4294967296", "9223372036854775808",
        "-9223372036854775809", "junk", "21junk", "1.5", ""}) {
    SCOPED_TRACE(id);
    const auto result =
        Cli({"multivideo_2v_2a.mp4"}, "--video-track-id " + Quote(id));
    EXPECT_NE(0, result.status);
    EXPECT_TRUE(result.output.empty());
  }
  for (const auto& options : {"", "--video-track-id -1", "--video-track-id 11",
                              "--video-track-id 31"}) {
    SCOPED_TRACE(options);
    const auto result = Cli({"multivideo_0v_1a.mp4"}, options);
    EXPECT_NE(0, result.status);
    EXPECT_TRUE(result.output.empty());
  }
  EXPECT_EQ(0, Cli({"multivideo_2v_2a.mp4"}, "--video-track-id 0x15").status);
}

TEST(LiblcvmCompactTracksCliTest,
     TimestampsUseOnlySelectedVideoAndLegacySchema) {
  TempDirectory dir;
  const auto path = dir.path / "timestamps.csv";
  for (const auto& selector : {"", "--video-track-id -1", "--video-track-id 11",
                               "--video-track-id 21"}) {
    SCOPED_TRACE(selector);
    const bool second = std::string(selector) == "--video-track-id 21";
    const auto result = Cli({"multivideo_2v_2a.mp4"},
                            std::string(selector) +
                                " --audio-track-id 41 --outfile-timestamps " +
                                Quote(path.string()));
    ASSERT_EQ(0, result.status);
    const auto csv = ReadTimestampFile(path);
    EXPECT_EQ(kTimestampHeader, csv.header);
    ASSERT_EQ(second ? 72u : 120u, csv.rows.size());
    ExpectText(csv.rows.front(), "frame_num", "0");
    ExpectText(csv.rows.back(), "frame_num", second ? "71" : "119");
    ExpectNumber(csv.rows.front(), "pts", 0.0);
    ExpectNumber(csv.rows[1], "pts", 1.0 / (second ? 24.0 : 30.0));
    ExpectText(csv.rows.front(), "filename",
               std::string(TEST_REGRESSION_DIR) + "/multivideo_2v_2a.mp4");
  }
}

TEST(LiblcvmCompactTracksCliTest,
     FailedLaterInputPreservesSuccessfulTimestamps) {
  TempDirectory dir;
  const auto path = dir.path / "timestamps.csv";
  for (const auto& last : {"multivideo_1v_1a.mp4", "multivideo_0v_1a.mp4",
                           "nonexistent/compact_tracks_missing.mp4"}) {
    SCOPED_TRACE(last);
    const auto result =
        Cli({"multivideo_2v_2a.mp4", last},
            "--video-track-id 21 --outfile-timestamps " + Quote(path.string()));
    EXPECT_NE(0, result.status);
    const auto metrics = ReadCsv(result.output);
    ASSERT_EQ(1u, metrics.rows.size());
    ExpectSchema(kFixtures[3], metrics.rows[0]);
    const auto csv = ReadTimestampFile(path);
    EXPECT_EQ(kTimestampHeader, csv.header);
    ASSERT_EQ(72u, csv.rows.size());
    ExpectText(csv.rows.back(), "frame_num", "71");
  }
}

TEST(LiblcvmCompactTracksCliTest,
     NoVideoBetweenSuccessesKeepsMetricRowsAligned) {
  const auto result = Cli(
      {"multivideo_1v_1a.mp4", "multivideo_0v_1a.mp4", "multivideo_2v_0a.mp4"});
  EXPECT_NE(0, result.status);
  const auto csv = ReadCsv(result.output);
  ASSERT_EQ(2u, csv.rows.size());
  ExpectNumber(csv.rows[0], "width", 320);
  ExpectText(csv.rows[0], "width_v21", "");
  ExpectNumber(csv.rows[1], "width_v21", 640);
  ExpectText(csv.rows[1], "width", "");
  ExpectText(csv.rows[1], "audio_type", "");
  ExpectText(csv.rows[1], "audio_video_ratio", "");
}
