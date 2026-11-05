#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <type_traits>

#include "liblcvm.h"
#include "liblcvm_c.h"
#include "timestamp_math.h"

static_assert(std::is_same_v<
              decltype(std::declval<TimingInformation>().get_pts_unit_list()),
              std::vector<int32_t>>);
static_assert(std::is_same_v<
              decltype(std::declval<TimingInformation>().get_ctts_unit_list()),
              std::vector<int32_t>>);
static_assert(
    std::is_same_v<
        decltype(TimingInformation::CompositionTimingEntry{}.sample_offset),
        int32_t>);
static_assert(std::is_same_v<std::tuple_element_t<2, LiblcvmTiming>, int32_t>);

static_assert(std::is_same_v<
              decltype(TimingInformation::SampleTimingEntry{}.sample_count),
              uint32_t>);
static_assert(std::is_same_v<
              decltype(TimingInformation::SampleTimingEntry{}.sample_delta),
              uint32_t>);
static_assert(std::is_same_v<decltype(std::declval<const TimingInformation&>()
                                          .get_stts_unit_list()),
                             std::vector<uint32_t>>);
static_assert(std::is_same_v<decltype(std::declval<const TimingInformation&>()
                                          .get_dts_sec_list()),
                             std::vector<double>>);
static_assert(std::is_same_v<decltype(std::declval<const TimingInformation&>()
                                          .get_pts_sec_list()),
                             std::vector<double>>);
static_assert(std::is_same_v<decltype(std::declval<const TimingInformation&>()
                                          .get_pts_duration_sec_list()),
                             std::vector<double>>);
static_assert(std::is_same_v<LiblcvmTiming,
                             std::tuple<uint32_t, uint32_t, int32_t, double,
                                        double, double, double, double>>);
static_assert(
    std::is_same_v<decltype(liblcvm_timing_arrays_t{}.ctts_units), int32_t*>);
static_assert(
    std::is_same_v<decltype(liblcvm_timing_arrays_t{}.stts_units), uint32_t*>);

namespace {
TimingInformation Read(const char* name, bool sort = false) {
  ISOBMFF::Parser parser;
  const auto error =
      parser.Parse(std::string(TEST_REGRESSION_DIR) + "/" + name);
  EXPECT_FALSE(static_cast<bool>(error));
  const auto mdia = parser.GetFile()
                        ->GetTypedBox<ISOBMFF::ContainerBox>("moov")
                        ->GetTypedBox<ISOBMFF::ContainerBox>("trak")
                        ->GetTypedBox<ISOBMFF::ContainerBox>("mdia");
  const auto stbl = mdia->GetTypedBox<ISOBMFF::ContainerBox>("minf")
                        ->GetTypedBox<ISOBMFF::ContainerBox>("stbl");
  const auto hz = mdia->GetTypedBox<ISOBMFF::MDHD>("mdhd")->GetTimescale();
  TimingInformation result;
  EXPECT_EQ(0, result.parse_timing_information(stbl, hz, 32, true, name, 0));
  EXPECT_EQ(0, result.derive_timing_info(sort, name, 0));
  return result;
}
}  // namespace

TEST(LiblcvmArithmeticTest, MultipleWrapsPreserveStoredBitsAndRealTime) {
  const auto timing = Read("arithmetic_multiple_wraps.mp4");
  EXPECT_EQ(
      (std::vector<int32_t>{0, -1294967296, 1705032704, 410065408, -884901888}),
      timing.get_pts_unit_list());
  EXPECT_EQ((std::vector<double>{0, 3000000, 6000000, 9000000, 12000000}),
            timing.get_dts_sec_list());
  EXPECT_EQ(timing.get_dts_sec_list(), timing.get_pts_sec_list());
  for (double gap : timing.get_pts_duration_sec_list())
    EXPECT_DOUBLE_EQ(3000000, gap);
  EXPECT_DOUBLE_EQ(3000000, timing.get_pts_duration_sec_average());
  EXPECT_DOUBLE_EQ(3000000, timing.get_pts_duration_sec_median());
  EXPECT_DOUBLE_EQ(1.0 / 3000000, timing.get_frame_rate_fps_median());
  EXPECT_EQ(0, timing.get_frame_drop_count());
}

TEST(LiblcvmArithmeticTest, SignedOffsetsAndWideDifferencesAreNotNarrowed) {
  const auto timing = Read("arithmetic_signed_pts.mp4");
  EXPECT_EQ((std::vector<int32_t>{-100, -1294967296, -1294967296, 1705032704}),
            timing.get_pts_unit_list());
  EXPECT_EQ((std::vector<int32_t>{-100, 1000000000, -1000000000, 0}),
            timing.get_ctts_unit_list());
  EXPECT_DOUBLE_EQ(-0.1, timing.get_pts_sec_list()[0]);
  EXPECT_EQ((std::vector<double>{3000000.1, 0, 3000000}),
            timing.get_pts_duration_sec_list());
  EXPECT_DOUBLE_EQ(2000000.0333333334, timing.get_pts_duration_sec_average());
}

TEST(LiblcvmArithmeticTest, SortingAndNegativeGapsUseTheUnwrappedContext) {
  const auto unsorted = Read("arithmetic_reordered_pts.mp4");
  EXPECT_DOUBLE_EQ(-1100000, unsorted.get_pts_duration_sec_list()[1]);
  const auto sorted = Read("arithmetic_reordered_pts.mp4", true);
  EXPECT_EQ((std::vector<uint32_t>{0, 2, 1, 3}),
            sorted.get_frame_num_orig_list());
  EXPECT_EQ((std::vector<double>{0, 3000000, 4100000, 6000000}),
            sorted.get_pts_sec_list());
  EXPECT_EQ((std::vector<double>{3000000, 1100000, 1900000}),
            sorted.get_pts_duration_sec_list());
  EXPECT_DOUBLE_EQ(2000000, sorted.get_pts_duration_sec_average());
  EXPECT_DOUBLE_EQ(1900000, sorted.get_pts_duration_sec_median());
  EXPECT_EQ((std::vector<int32_t>{0, -1294967296, -194967296, 1705032704}),
            sorted.get_pts_unit_list());
}

TEST(LiblcvmArithmeticTest, VersionZeroKeepsAppleSignedInterpretation) {
  const auto v0 = Read("arithmetic_signed_ctts_v0.mp4");
  const auto v1 = Read("arithmetic_signed_ctts_v1.mp4");
  EXPECT_EQ(-100, v0.get_ctts_entries()[0].sample_offset);
  EXPECT_EQ(v1.get_ctts_unit_list(), v0.get_ctts_unit_list());
  EXPECT_EQ(v1.get_pts_unit_list(), v0.get_pts_unit_list());
  EXPECT_EQ((std::vector<int32_t>{-100, 1020, 1980, 3000}),
            v0.get_pts_unit_list());
  EXPECT_EQ(v1.get_pts_sec_list(), v0.get_pts_sec_list());
  EXPECT_EQ(v1.get_pts_duration_sec_list(), v0.get_pts_duration_sec_list());
}

TEST(LiblcvmArithmeticTest, CheckedOperationsAreDefinedAtIntegerLimits) {
  using liblcvm_detail::add_timestamp;
  using liblcvm_detail::diff_timestamp;
  using liblcvm_detail::wrap_timestamp;
  EXPECT_EQ(INT32_MIN, wrap_timestamp(INT64_C(1) << 31));
  EXPECT_EQ(INT32_MAX, wrap_timestamp((INT64_C(1) << 31) - 1));
  EXPECT_EQ(5, wrap_timestamp((INT64_C(1) << 32) + 5));
  EXPECT_EQ(-1, wrap_timestamp(-1));
  EXPECT_EQ(0, wrap_timestamp(INT64_MIN));
  EXPECT_EQ(-1, wrap_timestamp(INT64_MAX));
  int64_t result = 123;
  EXPECT_TRUE(diff_timestamp(4294967286LL, 4294967301LL, result));
  EXPECT_EQ(15, result);
  EXPECT_TRUE(diff_timestamp(4294967301LL, 4294967286LL, result));
  EXPECT_EQ(-15, result);
  EXPECT_TRUE(diff_timestamp(INT64_MIN, -1, result));
  EXPECT_EQ(INT64_MAX, result);
  EXPECT_TRUE(diff_timestamp(INT64_MAX, -1, result));
  EXPECT_EQ(INT64_MIN, result);
  result = 123;
  EXPECT_FALSE(diff_timestamp(INT64_MIN, 0, result));
  EXPECT_FALSE(add_timestamp(INT64_MAX, 1, result));
  EXPECT_FALSE(add_timestamp(INT64_MIN, -1, result));
  EXPECT_EQ(123, result);
  EXPECT_TRUE(add_timestamp(INT64_MAX, -100, result));
  EXPECT_EQ(INT64_MAX - 100, result);
}

TEST(LiblcvmArithmeticTest, RepeatedDerivationAndNarrowExportsRemainCoherent) {
  auto timing = Read("arithmetic_multiple_wraps.mp4");
  const auto pts = timing.get_pts_unit_list();
  ASSERT_EQ(0, timing.derive_timing_info(true, "probe", 0));
  EXPECT_EQ(pts, timing.get_pts_unit_list());
  LiblcvmConfig config;
  LiblcvmKeyList keys, time_keys;
  LiblcvmValList values;
  LiblcvmTimingList times;
  const auto path =
      std::string(TEST_REGRESSION_DIR) + "/arithmetic_signed_ctts_v0.mp4";
  ASSERT_EQ(
      0, IsobmffFileInformation::parse_to_lists(
             path.c_str(), config, &keys, &values, true, &time_keys, &times));
  ASSERT_EQ(4u, times.size());
  EXPECT_EQ(-100, std::get<2>(times[0]));
  EXPECT_DOUBLE_EQ(-0.1, std::get<4>(times[0]));
}

TEST(LiblcvmArithmeticTest, AverageAccumulationDoesNotOverflow32Bits) {
  const auto timing = Read("arithmetic_large_average.mp4");
  const auto gaps = timing.get_pts_duration_sec_list();
  ASSERT_EQ(4u, gaps.size());
  for (double gap : gaps)
    EXPECT_DOUBLE_EQ(3000000000LL, gap);
  EXPECT_DOUBLE_EQ(3000000000LL, timing.get_pts_duration_sec_average());
  EXPECT_DOUBLE_EQ(3000000000LL, timing.get_pts_duration_sec_median());
  EXPECT_DOUBLE_EQ(1.0 / 3000000000LL,
                   timing.get_frame_rate_fps_reverse_average());
  EXPECT_DOUBLE_EQ(0, timing.get_pts_duration_sec_stddev());
}

TEST(LiblcvmArithmeticTest, ExactSignedAndUnsignedBoundariesKeepTheirEpochs) {
  const auto timing = Read("arithmetic_exact_boundaries.mp4");
  EXPECT_EQ((std::vector<int32_t>{0, INT32_MIN, 0, INT32_MIN}),
            timing.get_pts_unit_list());
  EXPECT_EQ((std::vector<double>{0, 2147483648.0, 4294967296.0, 6442450944.0}),
            timing.get_pts_sec_list());
  for (double gap : timing.get_pts_duration_sec_list())
    EXPECT_DOUBLE_EQ(2147483648.0, gap);
  EXPECT_DOUBLE_EQ(2147483648.0, timing.get_pts_duration_sec_average());
}

TEST(LiblcvmArithmeticTest, RepeatedExportsOfWrappedTimelineAreStable) {
  LiblcvmConfig config;
  const auto path =
      std::string(TEST_REGRESSION_DIR) + "/arithmetic_reordered_pts.mp4";
  const auto file = IsobmffFileInformation::parse(path.c_str(), config);
  ASSERT_NE(nullptr, file);
  const auto& original = file->get_video_track()->get_timing();
  const auto pts = original.get_pts_unit_list();
  const auto seconds = original.get_pts_sec_list();
  LiblcvmKeyList k1, k2, t1, t2;
  LiblcvmValList v1, v2;
  LiblcvmTimingList rows1, rows2;
  ASSERT_EQ(0, IsobmffFileInformation::LiblcvmConfig_to_lists(
                   file, &k1, &v1, true, &t1, &rows1, 0, -1, true));
  ASSERT_EQ(0, IsobmffFileInformation::LiblcvmConfig_to_lists(
                   file, &k2, &v2, true, &t2, &rows2, 0, -1, true));
  EXPECT_EQ(k1, k2);
  EXPECT_EQ(t1, t2);
  ASSERT_EQ(v1.size(), v2.size());
  for (size_t i = 0; i < v1.size(); ++i) {
    ASSERT_EQ(v1[i].index(), v2[i].index());
    std::visit(
        [&](const auto& first) {
          using T = std::decay_t<decltype(first)>;
          const auto& second = std::get<T>(v2[i]);
          if constexpr (std::is_same_v<T, double>) {
            if (std::isnan(first)) {
              EXPECT_TRUE(std::isnan(second));
            } else {
              EXPECT_DOUBLE_EQ(first, second);
            }
          } else {
            EXPECT_EQ(first, second);
          }
        },
        v1[i]);
  }
  ASSERT_EQ(rows1.size(), rows2.size());
  for (size_t i = 0; i < rows1.size(); ++i) {
    EXPECT_EQ(std::get<0>(rows1[i]), std::get<0>(rows2[i]));
    EXPECT_EQ(std::get<1>(rows1[i]), std::get<1>(rows2[i]));
    EXPECT_EQ(std::get<2>(rows1[i]), std::get<2>(rows2[i]));
    EXPECT_DOUBLE_EQ(std::get<3>(rows1[i]), std::get<3>(rows2[i]));
    EXPECT_DOUBLE_EQ(std::get<4>(rows1[i]), std::get<4>(rows2[i]));
    if (i + 1 < rows1.size()) {
      EXPECT_DOUBLE_EQ(std::get<5>(rows1[i]), std::get<5>(rows2[i]));
      EXPECT_DOUBLE_EQ(std::get<6>(rows1[i]), std::get<6>(rows2[i]));
      EXPECT_DOUBLE_EQ(std::get<7>(rows1[i]), std::get<7>(rows2[i]));
    } else {
      EXPECT_TRUE(std::isnan(std::get<5>(rows1[i])));
      EXPECT_TRUE(std::isnan(std::get<5>(rows2[i])));
      EXPECT_TRUE(std::isnan(std::get<6>(rows1[i])));
      EXPECT_TRUE(std::isnan(std::get<6>(rows2[i])));
      EXPECT_TRUE(std::isnan(std::get<7>(rows1[i])));
      EXPECT_TRUE(std::isnan(std::get<7>(rows2[i])));
    }
  }
  EXPECT_EQ(pts, original.get_pts_unit_list());
  EXPECT_EQ(seconds, original.get_pts_sec_list());
}
