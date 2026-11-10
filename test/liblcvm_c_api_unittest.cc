// Tests for the liblcvm C interface (liblcvm_c). They pin what
// liblcvm_analyze_video_simple() reports for common inputs, including how
// missing values are reported, so a change in the values callers log fails
// here first.
//
// The test media paths are relative to the repository root. CMake passes the
// root as TEST_SOURCE_ROOT; the Android build installs the files next to the
// test binary instead.

#include <gtest/gtest.h>
#include <limits.h>
#include <unistd.h>

#include <string>

#include "config.h"
#include "liblcvm_c.h"

namespace {

std::string test_file_path(const char* rel_path) {
#ifdef TEST_SOURCE_ROOT
  return std::string(TEST_SOURCE_ROOT) + "/" + rel_path;
#else
  char exe_path[PATH_MAX];
  ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
  if (len < 0) {
    return rel_path;
  }
  std::string dir(exe_path, static_cast<size_t>(len));
  return dir.substr(0, dir.rfind('/')) + "/" + rel_path;
#endif
}

struct Result {
  liblcvm_error_t rc;
  liblcvm_simple_info_t info;
};

Result analyze(const char* rel_path) {
  const std::string path = test_file_path(rel_path);
  Result result = {};
  result.rc = liblcvm_analyze_video_simple(path.c_str(), &result.info);
  return result;
}

void expect_info(const Result& result, int video_frames_count,
                 int video_duration_ms, int audio_duration_ms, int bit_depth) {
  ASSERT_EQ(result.rc, LIBLCVM_SUCCESS) << liblcvm_get_error_string(result.rc);
  EXPECT_EQ(result.info.video_frames_count, video_frames_count);
  EXPECT_EQ(result.info.video_duration_ms, video_duration_ms);
  EXPECT_EQ(result.info.audio_duration_ms, audio_duration_ms);
  EXPECT_EQ(result.info.bit_depth, bit_depth);
}

TEST(LiblcvmCApiTest, Version) {
  char version[32] = {};
  liblcvm_get_version(version, sizeof(version));
  EXPECT_STREQ(version, PROJECT_VER);
}

TEST(LiblcvmCApiTest, HevcAac) {
  expect_info(analyze("media/hevc_aac.mp4"), 60, 2000, 2000, 8);
}

TEST(LiblcvmCApiTest, HevcOpus) {
  expect_info(analyze("media/hevc_opus.mp4"), 60, 2000, 2000, 8);
}

// A missing value is -1 in liblcvm's own unit, and liblcvm_c scales durations
// from seconds to ms without checking: a missing track is reported as -1000
// ms with LIBLCVM_SUCCESS. Callers must treat negative values as invalid.
TEST(LiblcvmCApiTest, NoAudioTrackReportsScaledSentinel) {
  expect_info(analyze("test/regression/multivideo_1v_0a.mp4"), 120, 4000, -1000,
              -1);
}

TEST(LiblcvmCApiTest, NoVideoTrackReportsScaledSentinel) {
  expect_info(analyze("test/regression/multivideo_0v_1a.mp4"), 0, -1000, 4500,
              0);
}

// An unknown sample entry is kept as a generic track rather than failing the
// file (strictness is lcvm-CLI only), so the simple API succeeds and reports
// the audio duration as missing.
TEST(LiblcvmCApiTest, UnknownAudioTrackIsNotAnError) {
  expect_info(analyze("test/regression/only_unknown_audio.mp4"), 120, 4000,
              -1000, -1);
}

TEST(LiblcvmCApiTest, MissingFile) {
  EXPECT_EQ(analyze("media/does_not_exist.mp4").rc,
            LIBLCVM_ERROR_FILE_NOT_FOUND);
}

TEST(LiblcvmCApiTest, NullArguments) {
  liblcvm_simple_info_t info = {};
  EXPECT_EQ(liblcvm_analyze_video_simple(nullptr, &info),
            LIBLCVM_ERROR_INVALID_PARAMS);
  EXPECT_EQ(liblcvm_analyze_video_simple("/dev/null", nullptr),
            LIBLCVM_ERROR_INVALID_PARAMS);
}

}  // namespace
