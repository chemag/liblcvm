// liblcvm: low-complexity video metric library
// A library to detect frame dups and video freezes.

// A show case of using [ISOBMFF](https://github.com/DigiDNA/ISOBMFF) to
// detect frame dups and video freezes in ISOBMFF files.

#include <stdlib.h>

#include <ISOBMFF.hpp>
#include <Parser.hpp>  // for isobmff Parser
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <numeric>
#include <variant>
#include <vector>

#define DECL_GETTER(name, type) \
  type get_##name() const { return this->name; }

#define DECL_SETTER(name, type) \
  void set_##name(type val) { this->name = val; }

class IsobmffFileInformation;

// Declaration of IsobmffFileInforrmation structure.
class TimingInformation {
  // num_video_frames: Number of video frames in the file.
  int num_video_frames;
  // duration_video_sec: Video length (seconds).
  float duration_video_sec;
  // duration_audio_sec: Audio length (seconds).
  float duration_audio_sec;
  // timescale_video_hz: Video length (Hz).
  uint32_t timescale_video_hz;
  // timescale_audio_hz: Audio length (Hz).
  uint32_t timescale_audio_hz;
  // frame_num_orig_list: original frame numbers (unitless).
  std::vector<uint32_t> frame_num_orig_list;
  // stts_unit_list: STTS values (units).
  std::vector<uint32_t> stts_unit_list;
  // ctts_unit_list: CTTS values (units).
  std::vector<int32_t> ctts_unit_list;
  // dts_sec_list: DTS (decoding timestamp) list (seconds).
  std::vector<float> dts_sec_list;
  // pts_sec_list: PTS (presentation timestamp) list (units).
  std::vector<int32_t> pts_unit_list;
  // pts_sec_list: PTS (presentation timestamp) list (seconds).
  std::vector<float> pts_sec_list;
  // pts_duration_sec_list: PTS (presentation timestamp) duration list
  // (seconds).
  std::vector<float> pts_duration_sec_list;
  // pts_duration_delta_sec_list: PTS (presentation timestamp) duration
  // delta to the average list (seconds).
  std::vector<float> pts_duration_delta_sec_list;
  // pts_duration_sec_average: pts duration average (sec).
  float pts_duration_sec_average;
  // pts_duration_sec_median: pts duration median (sec).
  float pts_duration_sec_median;
  // pts_duration_sec_stddev: pts duration standard deviation (sec).
  float pts_duration_sec_stddev;
  // pts_duration_sec_mad: pts duration MAD (median absolute deviation) (sec).
  float pts_duration_sec_mad;
  std::vector<uint32_t> keyframe_sample_number_list;
  // num_video_keyframes: Number of video key frames in the file.
  int num_video_keyframes;
  float key_frame_ratio;
  // audio_video_ratio: Audio/video length ratio.
  float audio_video_ratio;
  // video_freeze: Whether there is a video freeze.
  bool video_freeze;
  // frame_rate_fps_list: Vector of per-frame frame rates (fps).
  std::vector<float> frame_rate_fps_list;
  // frame_rate_fps_median: Frame rate (median, fps).
  float frame_rate_fps_median;
  // frame_rate_fps_average: Frame rate (average, fps).
  float frame_rate_fps_average;
  // frame_rate_fps_reverse_average: Average frame rate calculated
  // as the reverse of average PTS duration (sec)
  float frame_rate_fps_reverse_average;
  // frame_rate_fps_stddev: Frame rate (stddev).
  float frame_rate_fps_stddev;
  // frame_drop_length_sec_list: Vector of lengths between frame drops.
  std::vector<float> frame_drop_length_sec_list;
  // frame_drop_count: Frame drop count.
  int frame_drop_count;
  // frame_drop_ratio: Frame drop ratio (unitless).
  float frame_drop_ratio;
  // normalized_frame_drop_average_length: Normalized frame drop length.
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
  DECL_GETTER(pts_duration_delta_sec_list, std::vector<float>)
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
  DECL_GETTER(frame_rate_fps_reverse_average, float)
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

  // @param[in] percentile_list: Percentile list.
  // @param[out] frame_drop_length_percentile_list: Frame drop length percentile
  // list.
  void calculate_percentile_list(
      const std::vector<float> percentile_list,
      std::vector<float> &frame_drop_length_percentile_list, int debug);

  // @param[in] consecutive_list: Consecutive list.
  // @param[out] frame_drop_length_consecutive: Frame drop length consecutive.
  void calculate_consecutive_list(
      std::vector<int> consecutive_list,
      std::vector<long int> &frame_drop_length_consecutive, int debug);

  friend class IsobmffFileInformation;
};

class AudioInformation {
 private:
  // audio_type: Audio type ("mp4a" or "mp4s").
  std::string audio_type;
  // audio_object_type: Audio object type (from mp4a/mp4s).
  int channel_count;
  // audio_sample_rate: Audio sample rate (Hz).
  int sample_rate;
  // audio_sample_size:
  int sample_size;

 public:
  DECL_GETTER(audio_type, std::string)
  DECL_GETTER(channel_count, int)
  DECL_GETTER(sample_rate, int)
  DECL_GETTER(sample_size, int)

  int parse_mp4a(std::shared_ptr<ISOBMFF::ContainerBox> stbl,
                 std::shared_ptr<IsobmffFileInformation> ptr, int debug);
  friend class IsobmffFileInformation;
};

class FrameInformation {
 private:
  // filesize: Video file size (bytes).
  int filesize;
  // bitrate_bps: Video bitrate (bps).
  float bitrate_bps;
  // width: Video width.
  float width;
  // height: Video height.
  float height;
  // type: Video type ("hvc1" or "avc1").
  std::string type;
  // width2: Video width (from hvc1/avc1).
  int width2;
  // height2: Video height (from hvc1/avc1).
  int height2;
  // horizresolution: Video horizresolution (from hvc1/avc1).
  int horizresolution;
  // vertresolution: Video vertresolution (from hvc1/avc1).
  int vertresolution;
  // depth: Video depth (from hvc1/avc1).
  int depth;
  // chroma_format: Video chroma format (from hvcC/avcC).
  int chroma_format;
  // bit_depth_luma: Video luma bit depth (from hvcC/avcC).
  int bit_depth_luma;
  // bit_depth_chroma: Video chroma bit depth (from hvcC/avcC).
  int bit_depth_chroma;
  // video_full_range_flag: Video range (from HEVC/AVC SPS).
  int video_full_range_flag;
  // colour_primaries: Video color primaries (from HEVC/AVC SPS).
  int colour_primaries;
  // transfer_characteristics: Video transfer characteristics (from
  // HEVC/AVC SPS).
  int transfer_characteristics;
  // matrix_coeffs: Video matrix coefficients (from HEVC/AVC SPS).
  int matrix_coeffs;

  // internal functions
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

using LiblcvmValue = std::variant<int, double, std::string>;
double to_double(const LiblcvmValue &value);
std::string to_string_value(const LiblcvmValue &value);

class LiblcvmConfig {
 private:
  // sort_by_pts: Whether to sort the frames by PTS values.
  bool sort_by_pts;
  // debug: Debug level.
  int debug;

 public:
  LiblcvmConfig() {
    sort_by_pts = true;
    debug = 0;
  };

  DECL_GETTER(sort_by_pts, bool)
  DECL_SETTER(sort_by_pts, bool)
  DECL_GETTER(debug, int)
  DECL_SETTER(debug, int)
};

// Main class
class IsobmffFileInformation {
 private:
  std::string filename;
  TimingInformation timing;
  FrameInformation frame;
  AudioInformation audio;

 public:
  DECL_GETTER(filename, std::string)
  DECL_GETTER(timing, TimingInformation)
  DECL_GETTER(frame, FrameInformation)
  DECL_GETTER(audio, AudioInformation)

  // @brief Gets the library version.
  //
  // @param[out] version: Version string.
  static void get_liblcvm_version(std::string &version);

  // @brief Parse an ISOBMFF file.
  //
  // @param[in] infile: Name of the file to be parsed.
  // @param[in] liblcvm_config: Parsing configuration.
  // @return ptr: Full ISOBMFF information.
  static std::shared_ptr<IsobmffFileInformation> parse(
      const char *infile, const LiblcvmConfig &liblcvm_config);

  // @brief Parse an ISOBMFF file into a map (dictionary).
  //
  // @param[in] infile: Name of the file to be parsed.
  // @param[in] liblcvm_config: Parsing configuration.
  // @return ptr: Full ISOBMFF information, as a map.
  static std::shared_ptr<std::map<std::string, LiblcvmValue>> parse_to_map(
      const char *infile, const LiblcvmConfig &liblcvm_config,
      const std::string &policy_str);

  // Private constructor to prevent direct instantiation
  IsobmffFileInformation() = default;

  friend class TimingInformation;
  friend class FrameInformation;
  friend class AudioInformation;
};
