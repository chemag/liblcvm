// liblcvm: low-complexity video metric library
// A library to detect frame dups and video freezes.

// A show case of using [ISOBMFF](https://github.com/DigiDNA/ISOBMFF) to
// detect frame dups and video freezes in ISOBMFF files.

#include <stdlib.h>

#include <ISOBMFF.hpp>
#include <ISOBMFF/Parser.hpp>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <numeric>
#include <vector>

#define DECL_GETTER(name, type) \
  type get_##name() const { return this->name; }

class IsobmffFileInformation;

// Declaration of IsobmffFileInforrmation structure.
class TimingInformation {
  int num_video_frames;
  float duration_video_sec;
  float duration_audio_sec;
  uint32_t timescale_video_hz;
  uint32_t timescale_audio_hz;
  std::vector<uint32_t> frame_num_orig_list;
  std::vector<uint32_t> stts_unit_list;
  std::vector<int32_t> ctts_unit_list;
  std::vector<float> dts_sec_list;
  std::vector<int32_t> pts_unit_list;
  std::vector<float> pts_sec_list;
  std::vector<float> pts_duration_sec_list;
  float pts_duration_sec_average;
  float pts_duration_sec_median;
  float pts_duration_sec_stddev;
  float pts_duration_sec_mad;
  std::vector<uint32_t> keyframe_sample_number_list;
  int num_video_keyframes;
  float key_frame_ratio;
  float audio_video_ratio;
  bool video_freeze;
  std::vector<float> frame_rate_fps_list;
  float frame_rate_fps_median;
  float frame_rate_fps_average;
  float frame_rate_fps_stddev;
  std::vector<float> frame_drop_length_sec_list;
  int frame_drop_count;
  float frame_drop_ratio;
  float normalized_frame_drop_average_length;

 public:
  DECL_GETTER(num_video_frames, int)
  DECL_GETTER(duration_video_sec, float)
  DECL_GETTER(duration_audio_sec, float)
  DECL_GETTER(timescale_video_hz, uint32_t)
  DECL_GETTER(timescale_audio_hz, uint32_t)
  DECL_GETTER(frame_num_orig_list, std::vector<uint32_t>)
  DECL_GETTER(stts_unit_list, std::vector<uint32_t>)
  DECL_GETTER(ctts_unit_list, std::vector<int32_t>)
  DECL_GETTER(dts_sec_list, std::vector<float>)
  DECL_GETTER(pts_unit_list, std::vector<int32_t>)
  DECL_GETTER(pts_sec_list, std::vector<float>)
  DECL_GETTER(pts_duration_sec_list, std::vector<float>)
  DECL_GETTER(pts_duration_sec_average, float)
  DECL_GETTER(pts_duration_sec_median, float)
  DECL_GETTER(pts_duration_sec_stddev, float)
  DECL_GETTER(pts_duration_sec_mad, float)
  DECL_GETTER(keyframe_sample_number_list, std::vector<uint32_t>)
  DECL_GETTER(num_video_keyframes, int)
  DECL_GETTER(key_frame_ratio, float)
  DECL_GETTER(audio_video_ratio, float)
  DECL_GETTER(video_freeze, bool)
  DECL_GETTER(frame_rate_fps_list, std::vector<float>)
  DECL_GETTER(frame_rate_fps_median, float)
  DECL_GETTER(frame_rate_fps_average, float)
  DECL_GETTER(frame_rate_fps_stddev, float)
  DECL_GETTER(frame_drop_length_sec_list, std::vector<float>)
  DECL_GETTER(frame_drop_count, int)
  DECL_GETTER(frame_drop_ratio, float)
  DECL_GETTER(normalized_frame_drop_average_length, float)

  static int parse_timing_information(
      std::shared_ptr<ISOBMFF::ContainerBox> stbl, uint32_t timescale_hz,
      std::shared_ptr<IsobmffFileInformation> ptr, int debug);

  static int parse_keyframe_information(
      std::shared_ptr<ISOBMFF::ContainerBox> stbl,
      std::shared_ptr<IsobmffFileInformation> ptr, int debug);

  static int derive_timing_info(std::shared_ptr<IsobmffFileInformation> ptr,
                                bool sort_by_pts, int debug);

  friend class IsobmffFileInformation;
};

class FrameInformation {
 private:
  int filesize;
  float bitrate_bps;
  float width;
  float height;
  std::string type;
  int width2;
  int height2;
  int horizresolution;
  int vertresolution;
  int depth;
  int chroma_format;
  int bit_depth_luma;
  int bit_depth_chroma;
  int video_full_range_flag;
  int colour_primaries;
  int transfer_characteristics;
  int matrix_coeffs;

  void parse_hvcc(std::shared_ptr<ISOBMFF::HVCC> hvcc, int debug);
  void parse_avcc(std::shared_ptr<ISOBMFF::AVCC> avcc, int debug);

 public:
  DECL_GETTER(filesize, int)
  DECL_GETTER(bitrate_bps, float)
  DECL_GETTER(width, float)
  DECL_GETTER(height, float)
  DECL_GETTER(type, std::string)
  DECL_GETTER(width2, int)
  DECL_GETTER(height2, int)
  DECL_GETTER(horizresolution, int)
  DECL_GETTER(vertresolution, int)
  DECL_GETTER(depth, int)
  DECL_GETTER(chroma_format, int)
  DECL_GETTER(bit_depth_luma, int)
  DECL_GETTER(bit_depth_chroma, int)
  DECL_GETTER(video_full_range_flag, int)
  DECL_GETTER(colour_primaries, int)
  DECL_GETTER(transfer_characteristics, int)
  DECL_GETTER(matrix_coeffs, int)

  int parse_frame_information(std::shared_ptr<ISOBMFF::ContainerBox> stbl,
                              std::shared_ptr<IsobmffFileInformation> ptr,
                              int debug);

  static int derive_frame_info(std::shared_ptr<IsobmffFileInformation> ptr,
                               bool sort_by_pts, int debug);

  friend class IsobmffFileInformation;
};

// Main class
class IsobmffFileInformation {
 private:
  std::string filename;
  TimingInformation timing;
  FrameInformation frame;

 public:
  DECL_GETTER(filename, std::string)
  DECL_GETTER(timing, TimingInformation)
  DECL_GETTER(frame, FrameInformation)

  // @brief Gets the library version.
  //
  // @param[out] version: Version string.
  static void get_liblcvm_version(std::string &version);

  // @brief Parse an ISOBMFF file.
  //
  // @param[in] infile: Name of the file to be parsed.
  // @param[in] sort_by_pts: Whether to sort the frames by PTS values.
  // @param[in] debug: Debug level.
  // @return ptr: Full ISOBMFF information.
  static std::shared_ptr<IsobmffFileInformation> parse(const char *infile,
                                                       bool sort_by_pts,
                                                       int debug);

  // Private constructor to prevent direct instantiation
  IsobmffFileInformation() = default;

  friend class TimingInformation;
  friend class FrameInformation;
};

// Old API (do not use).

// @brief Calculates the frame drop info.
//
// @param[in] ptr: ISOBMFF information from IsobmffFileInformation::parse()
// @param[out] num_video_frames: Number of video frames in the file.
// @param[out] frame_rate_fps_median: Frame rate (median, fps).
// @param[out] frame_rate_fps_average: Frame rate (average, fps).
// @param[out] frame_rate_fps_stddev: Frame rate (stddev).
// @param[out] frame_drop_count: Frame drop count.
// @param[out] frame_drop_ratio: Frame drop ratio (unitless).
// @param[out] normalized_frame_drop_average_length: Normalized frame drop
// length.
// @param[in] percentile_list: Percentile list.
// @param[out] frame_drop_length_percentile_list: Frame drop length percentile
// list.
// @param[in] consecutive_list: Consecutive list.
// @param[out] frame_drop_length_consecutive: Frame drop length consecutive.
// @param[in] debug: Debug level.
int get_frame_drop_info(const std::shared_ptr<IsobmffFileInformation> ptr,
                        int *num_video_frames, float *frame_rate_fps_median,
                        float *frame_rate_fps_average,
                        float *frame_rate_fps_stddev, int *frame_drop_count,
                        float *frame_drop_ratio,
                        float *normalized_frame_drop_average_length,
                        const std::vector<float> percentile_list,
                        std::vector<float> &frame_drop_length_percentile_list,
                        const std::vector<int> consecutive_list,
                        std::vector<long int> &frame_drop_length_consecutive,
                        int debug);

// @brief Calculates the video freeze info.
//
// @param[in] ptr: ISOBMFF information from IsobmffFileInformation::parse().
// @param[out] video_freeze: Whether there is a video freeze.
// @param[out] audio_video_ratio: Audio/video length ratio.
// @param[out] duration_video_sec: Video length (seconds).
// @param[out] duration_audio_sec: Audio length (seconds).
// @param[out] timescale_video_hz: Video length (Hz).
// @param[out] timescale_audio_hz: Audio length (Hz).
// @param[out] pts_duration_sec_average: pts duration average (sec).
// @param[out] pts_duration_sec_median: pts duration median (sec).
// @param[out] pts_duration_sec_stddev: pts duration standard deviation (sec).
// @param[out] pts_duration_sec_mad: pts duration MAD (median absolute
//             deviation) (sec).
// @param[in] debug: Debug level.
int get_video_freeze_info(const std::shared_ptr<IsobmffFileInformation> ptr,
                          bool *video_freeze, float *audio_video_ratio,
                          float *duration_video_sec, float *duration_audio_sec,
                          uint32_t *timescale_video_hz,
                          uint32_t *timescale_audio_hz,
                          float *pts_duration_sec_average,
                          float *pts_duration_sec_median,
                          float *pts_duration_sec_stddev,
                          float *pts_duration_sec_mad, int debug);

// @brief Calculates the video GoP structure info.
//
// @param[in] ptr: ISOBMFF information from IsobmffFileInformation::parse()
// @param[out] num_video_frames: Number of video frames in the file.
// @param[out] num_video_keyframes: Number of video key frames in the file.
// @param[in] debug: Debug level.
int get_video_structure_info(const std::shared_ptr<IsobmffFileInformation> ptr,
                             int *num_video_frames, int *num_video_keyframes,
                             int debug);

// @brief Calculates the generic video info.
//
// @param[in] ptr: ISOBMFF information from IsobmffFileInformation::parse()
// @param[out] filesize: Video file size (bytes).
// @param[out] bitrate_bps: Video bitrate (bps).
// @param[out] width: Video width.
// @param[out] height: Video height.
// @param[out] type: Video type ("hvc1" or "avc1").
// @param[out] width2: Video width (from hvc1/avc1).
// @param[out] height2: Video height (from hvc1/avc1).
// @param[out] horizresolution: Video horizresolution (from hvc1/avc1).
// @param[out] vertresolution: Video vertresolution (from hvc1/avc1).
// @param[out] depth: Video depth (from hvc1/avc1).
// @param[out] chroma_format: Video chroma format (from hvcC/avcC).
// @param[out] bit_depth_luma: Video luma bit depth (from hvcC/avcC).
// @param[out] bit_depth_chroma: Video chroma bit depth (from hvcC/avcC).
// @param[out] video_full_range_flag: Video range (from HEVC/AVC SPS).
// @param[out] colour_primaries: Video color primaries (from HEVC/AVC SPS).
// @param[out] transfer_characteristics: Video transfer characteristics (from
// HEVC/AVC SPS).
// @param[out] matrix_coeffs: Video matrix coefficients (from HEVC/AVC SPS).
// @param[in] debug: Debug level.
int get_video_generic_info(const std::shared_ptr<IsobmffFileInformation> ptr,
                           int *filesize, float *bitrate_bps, int *width,
                           int *height, std::string &type, int *width2,
                           int *height2, int *horizresolution,
                           int *vertresolution, int *depth, int *chroma_format,
                           int *bit_depth_luma, int *bit_depth_chroma,
                           int *video_full_range_flag, int *colour_primaries,
                           int *transfer_characteristics, int *matrix_coeffs,
                           int debug);

// debug API

// @brief Calculates full inter-frame time info.
//
// @param[in] ptr: ISOBMFF information from IsobmffFileInformation::parse()
// @param[out] num_video_frames: Number of video frames in the file.
// @param[out] frame_num_orig_list: original frame numbers (unitless).
// @param[out] stts_unit_list: STTS values (units).
// @param[out] ctts_unit_list: CTTS values (units).
// @param[out] dts_sec_list: DTS (decoding timestamp) list (seconds).
// @param[out] pts_sec_list: PTS (presentation timestamp) list (seconds).
// @param[out] pts_duration_sec_list: PTS (presentation timestamp) duration list
// (seconds).
// @param[in] sort_by_pts: Whether to sort the frames by PTS values.
// @param[in] debug: Debug level.
int get_frame_interframe_info(
    const std::shared_ptr<IsobmffFileInformation> ptr, int *num_video_frames,
    std::vector<uint32_t> &frame_num_orig_list,
    std::vector<uint32_t> &stts_unit_list, std::vector<int32_t> &ctts_unit_list,
    std::vector<float> &dts_sec_list, std::vector<float> &pts_sec_list,
    std::vector<float> &pts_duration_list, bool sort_by_pts, int debug);
