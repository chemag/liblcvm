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
#include <list>
#include <map>
#include <numeric>
#include <tuple>
#include <utility>
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
  double duration_video_sec;
  // duration_audio_sec: Audio length (seconds).
  double duration_audio_sec;
  // timescale_video_hz: Video length (Hz).
  uint32_t timescale_video_hz;
  // timescale_audio_hz: Audio length (Hz).
  uint32_t timescale_audio_hz;
  // timescale_movie_hz: Movie timescale (Hz) from mvhd box.
  uint32_t timescale_movie_hz;
  // frame_num_orig_list: original frame numbers (unitless).
  std::vector<uint32_t> frame_num_orig_list;
  // stts_unit_list: STTS values (units).
  std::vector<uint32_t> stts_unit_list;
  // ctts_unit_list: CTTS values (units).
  std::vector<int32_t> ctts_unit_list;
  // dts_sec_list: DTS (decoding timestamp) list (seconds).
  std::vector<double> dts_sec_list;
  // pts_sec_list: PTS (presentation timestamp) list (units).
  std::vector<int32_t> pts_unit_list;
  // pts_sec_list: PTS (presentation timestamp) list (seconds).
  std::vector<double> pts_sec_list;
  // pts_duration_sec_list: PTS (presentation timestamp) duration list
  // (seconds).
  std::vector<double> pts_duration_sec_list;
  // pts_duration_delta_sec_list: PTS (presentation timestamp) duration
  // delta to the average list (seconds).
  std::vector<double> pts_duration_delta_sec_list;
  // pts_framerate_list: Instantaneous framerate (fps).
  std::vector<double> pts_framerate_list;
  // pts_duration_sec_average: pts duration average (sec).
  double pts_duration_sec_average;
  // pts_duration_sec_median: pts duration median (sec).
  double pts_duration_sec_median;
  // pts_duration_sec_stddev: pts duration standard deviation (sec).
  double pts_duration_sec_stddev;
  // pts_duration_sec_mad: pts duration MAD (median absolute deviation) (sec).
  double pts_duration_sec_mad;
  std::vector<uint32_t> keyframe_sample_number_list;
  // num_video_keyframes: Number of video key frames in the file.
  int num_video_keyframes;
  double key_frame_ratio;
  // audio_video_ratio: Audio/video length ratio.
  double audio_video_ratio;
  // video_freeze: Whether there is a video freeze.
  bool video_freeze;
  // frame_rate_fps_list: Vector of per-frame frame rates (fps).
  std::vector<double> frame_rate_fps_list;
  // frame_rate_fps_median: Frame rate (median, fps).
  double frame_rate_fps_median;
  // frame_rate_fps_average: Frame rate (average, fps).
  double frame_rate_fps_average;
  // frame_rate_fps_reverse_average: Average frame rate calculated
  // as the reverse of average PTS duration (sec)
  double frame_rate_fps_reverse_average;
  // frame_rate_fps_stddev: Frame rate (stddev).
  double frame_rate_fps_stddev;
  // frame_drop_length_sec_list: Vector of lengths between frame drops.
  std::vector<double> frame_drop_length_sec_list;
  // frame_drop_count: Frame drop count.
  int frame_drop_count;
  // frame_drop_ratio: Frame drop ratio (unitless).
  double frame_drop_ratio;
  // normalized_frame_drop_average_length: Normalized frame drop length.
  double normalized_frame_drop_average_length;

 public:
  DECL_GETTER(num_video_frames, int)
  DECL_GETTER(duration_video_sec, double)
  DECL_GETTER(duration_audio_sec, double)
  DECL_GETTER(timescale_video_hz, uint32_t)
  DECL_GETTER(timescale_audio_hz, uint32_t)
  DECL_GETTER(timescale_movie_hz, uint32_t)
  DECL_GETTER(frame_num_orig_list, std::vector<uint32_t>)
  DECL_GETTER(stts_unit_list, std::vector<uint32_t>)
  DECL_GETTER(ctts_unit_list, std::vector<int32_t>)
  DECL_GETTER(dts_sec_list, std::vector<double>)
  DECL_GETTER(pts_unit_list, std::vector<int32_t>)
  DECL_GETTER(pts_sec_list, std::vector<double>)
  DECL_GETTER(pts_duration_sec_list, std::vector<double>)
  DECL_GETTER(pts_duration_delta_sec_list, std::vector<double>)
  DECL_GETTER(pts_framerate_list, std::vector<double>)
  DECL_GETTER(pts_duration_sec_average, double)
  DECL_GETTER(pts_duration_sec_median, double)
  DECL_GETTER(pts_duration_sec_stddev, double)
  DECL_GETTER(pts_duration_sec_mad, double)
  DECL_GETTER(keyframe_sample_number_list, std::vector<uint32_t>)
  DECL_GETTER(num_video_keyframes, int)
  DECL_GETTER(key_frame_ratio, double)
  DECL_GETTER(audio_video_ratio, double)
  DECL_GETTER(video_freeze, bool)
  DECL_GETTER(frame_rate_fps_list, std::vector<double>)
  DECL_GETTER(frame_rate_fps_median, double)
  DECL_GETTER(frame_rate_fps_average, double)
  DECL_GETTER(frame_rate_fps_reverse_average, double)
  DECL_GETTER(frame_rate_fps_stddev, double)
  DECL_GETTER(frame_drop_length_sec_list, std::vector<double>)
  DECL_GETTER(frame_drop_count, int)
  DECL_GETTER(frame_drop_ratio, double)
  DECL_GETTER(normalized_frame_drop_average_length, double)

  static int parse_timing_information(
      std::shared_ptr<ISOBMFF::ContainerBox> stbl, uint32_t timescale_track_hz,
      uint32_t max_num_frames, std::shared_ptr<IsobmffFileInformation> ptr,
      int debug);

  static int parse_keyframe_information(
      std::shared_ptr<ISOBMFF::ContainerBox> stbl,
      std::shared_ptr<IsobmffFileInformation> ptr, int debug);

  static int derive_timing_info(std::shared_ptr<IsobmffFileInformation> ptr,
                                bool sort_by_pts, int debug);

  // @param[in] percentile_list: Percentile list.
  // @param[out] frame_drop_length_percentile_list: Frame drop length percentile
  // list.
  void calculate_percentile_list(
      const std::vector<double> percentile_list,
      std::vector<double>& frame_drop_length_percentile_list, int debug);

  // @param[in] consecutive_list: Consecutive list.
  // @param[out] frame_drop_length_consecutive: Frame drop length consecutive.
  void calculate_consecutive_list(
      std::vector<int> consecutive_list,
      std::vector<long int>& frame_drop_length_consecutive, int debug);

  friend class IsobmffFileInformation;
};

class AudioTrack {
 private:
  TimingInformation timing{};
  // audio_type: Exact audio sample entry type ("mp4a", "Opus", or "ec-3").
  std::string audio_type;
  // E-AC-3 values that cannot be resolved from metadata are -1. Keep the
  // legacy zero defaults for skipped audio and files without audio.
  int channel_count = 0;
  int sample_rate = 0;
  int sample_size = 0;
  // All independent E-AC-3 services and optional container JOC signalling.
  // This is not elementary-stream validation or decoder configuration.
  std::shared_ptr<const ISOBMFF::DEC3> eac3_configuration;
  uint32_t track_id = 0;
  double mdhd_duration_sec = -1.0;

  int parse_sample_entry(std::shared_ptr<ISOBMFF::ContainerBox> stbl,
                         const std::string& filename, int debug);

 public:
  AudioTrack();
  const TimingInformation& get_timing() const { return timing; }
  DECL_GETTER(audio_type, std::string)
  DECL_GETTER(channel_count, int)
  DECL_GETTER(sample_rate, int)
  DECL_GETTER(sample_size, int)
  // Track identity and durations are retained even when codec parsing is
  // skipped.
  DECL_GETTER(eac3_configuration, std::shared_ptr<const ISOBMFF::DEC3>)
  DECL_GETTER(track_id, uint32_t)
  uint32_t get_timescale_hz() const { return timing.get_timescale_audio_hz(); }
  DECL_GETTER(mdhd_duration_sec, double)
  double get_duration_sec() const { return timing.get_duration_audio_sec(); }

  // Refresh the owner's first/default audio metadata. Registered track IDs
  // and durations are preserved, and the per-track/default views stay aligned.
  int parse_audio_information(std::shared_ptr<ISOBMFF::ContainerBox> stbl,
                              std::shared_ptr<IsobmffFileInformation> ptr,
                              int debug);
  friend class IsobmffFileInformation;
  friend class TimingInformation;
};

class VideoTrack {
 private:
  TimingInformation timing{};
  // track_id: Stored tkhd ID of the analyzed video track, not its index.
  uint32_t track_id = 0;
  // filesize: Video file size (bytes).
  int filesize = 0;
  // bitrate_bps: Video bitrate (bps).
  double bitrate_bps = 0.0;
  // width: Video width.
  double width = 0.0;
  // height: Video height.
  double height = 0.0;
  // video_codec_type: Video type ("hvc1", "hev1", "avc1", or "avc3").
  std::string video_codec_type;
  // width2: Video width (from hvc1/hev1/avc1/avc3).
  int width2 = 0;
  // height2: Video height (from hvc1/hev1/avc1/avc3).
  int height2 = 0;
  // horizresolution: Video horizresolution (from hvc1/hev1/avc1/avc3).
  int horizresolution = 0;
  // vertresolution: Video vertresolution (from hvc1/hev1/avc1/avc3).
  int vertresolution = 0;
  // depth: Video depth (from hvc1/hev1/avc1/avc3).
  int depth = 0;
  // chroma_format: Video chroma format (from hvcC/avcC).
  int chroma_format = 0;
  // bit_depth_luma: Video luma bit depth (from hvcC/avcC).
  int bit_depth_luma = 0;
  // bit_depth_chroma: Video chroma bit depth (from hvcC/avcC).
  int bit_depth_chroma = 0;
  // video_full_range_flag: Video range (from HEVC/AVC SPS).
  int video_full_range_flag = 0;
  // colour_primaries: Video color primaries (from HEVC/AVC SPS).
  int colour_primaries = 0;
  // transfer_characteristics: Video transfer characteristics (from
  // HEVC/AVC SPS).
  int transfer_characteristics = 0;
  // matrix_coeffs: Video matrix coefficients (from HEVC/AVC SPS).
  int matrix_coeffs = 0;
  // profile_idc: Video profile IDC (from HEVC/AVC SPS).
  int profile_idc = 0;
  // level_idc: Video level IDC (from HEVC/AVC SPS).
  int level_idc = 0;
  // profile_type_str: Video profile type string (from HEVC/AVC SPS).
  std::string profile_type_str;

  // internal functions
  void parse_hvcc(std::shared_ptr<ISOBMFF::HVCC> hvcc, int debug);
  void parse_avcc(std::shared_ptr<ISOBMFF::AVCC> avcc, int debug);

 public:
  VideoTrack();
  const TimingInformation& get_timing() const { return timing; }
  DECL_GETTER(track_id, uint32_t)
  DECL_GETTER(filesize, int)
  DECL_GETTER(bitrate_bps, double)
  DECL_GETTER(width, double)
  DECL_GETTER(height, double)
  DECL_GETTER(video_codec_type, std::string)
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
  DECL_GETTER(profile_idc, int)
  DECL_GETTER(level_idc, int)
  DECL_GETTER(profile_type_str, std::string)

  int parse_video_information(std::shared_ptr<ISOBMFF::ContainerBox> stbl,
                              std::shared_ptr<IsobmffFileInformation> ptr,
                              int debug);

  static int derive_video_info(std::shared_ptr<IsobmffFileInformation> ptr,
                               bool sort_by_pts, int debug);

  friend class IsobmffFileInformation;
  friend class TimingInformation;
};

using LiblcvmValue =
    std::variant<int, unsigned int, long int, double, std::string>;
using LiblcvmValList = std::vector<LiblcvmValue>;
using LiblcvmKeyList = std::vector<std::string>;

// frame_num, stts, ctts, dts, pts, pts_duration, pts_duration_delta,
// pts_framerate
using LiblcvmTiming = std::tuple<uint32_t, uint32_t, int32_t, double, double,
                                 double, double, double>;
using LiblcvmTimingList = std::vector<LiblcvmTiming>;

int liblcvmvalue_to_double(const LiblcvmValue& value, double* result);
int liblcvmvalue_to_string(const LiblcvmValue& value, std::string* result);

class LiblcvmConfig {
 private:
  // sort_by_pts: Whether to sort the frames by PTS values.
  bool sort_by_pts;
  // skip_audio_parsing: Whether to skip parsing the audio sample entry
  // (codec, channel count, sample size, sample rate). The audio duration
  // and timescale are still read from the track's mdhd box.
  bool skip_audio_parsing;
  // max_num_frames: Maximum total number of video frames (stts samples)
  // across all video tracks to accept. liblcvm keeps several per-frame lists,
  // and the sample counts come straight from the file (32 bits each), so a
  // small malformed file can otherwise declare billions of frames and exhaust
  // memory. Files declaring more frames fail to parse.
  uint32_t max_num_frames;
  // Export video selector: -1 selects the first video in container order.
  int64_t video_track_id = -1;
  // Export selection: -1 means the first audio track in container order.
  // Explicit IDs are stored uint32 tkhd values, represented without narrowing.
  int64_t audio_track_id = -1;
  // Expanded exports add typed per-track keys alongside the unsuffixed aliases.
  // Default OFF preserves the existing library export schema; the CLI enables
  // it.
  bool include_track_keys = false;
  // policy: Warn/Error policy.
  std::string policy;
  // debug: Debug level.
  int debug;

 public:
  // Default max_num_frames: 2^20 frames (about 9.7 hours at 30 fps, or 73
  // minutes at 240 fps). Each frame costs about 130 bytes and 1.5 us of
  // parsing (x86-64, release build), so the default bounds a malformed
  // file to about 130 MB and 1.5 seconds.
  static constexpr uint32_t kDefaultMaxNumFrames = 1 << 20;

  LiblcvmConfig() {
    sort_by_pts = true;
    skip_audio_parsing = false;
    max_num_frames = kDefaultMaxNumFrames;
    policy = "";
    debug = 0;
  }

  DECL_GETTER(sort_by_pts, bool)
  DECL_SETTER(sort_by_pts, bool)
  DECL_GETTER(skip_audio_parsing, bool)
  DECL_SETTER(skip_audio_parsing, bool)
  DECL_GETTER(max_num_frames, uint32_t)
  DECL_SETTER(max_num_frames, uint32_t)
  DECL_GETTER(video_track_id, int64_t)
  DECL_SETTER(video_track_id, int64_t)
  DECL_GETTER(audio_track_id, int64_t)
  DECL_SETTER(audio_track_id, int64_t)
  DECL_GETTER(include_track_keys, bool)
  DECL_SETTER(include_track_keys, bool)
  DECL_GETTER(policy, std::string)
  DECL_SETTER(policy, std::string)
  DECL_GETTER(debug, int)
  DECL_SETTER(debug, int)
};

// Main class

class IsobmffFileInformation {
 private:
  std::string filename;
  std::string policy;
  uint32_t timescale_movie_hz = 0;
  int filesize = 0;
  // Container order defines the default view, independently of stored IDs.
  std::vector<VideoTrack> video_tracks;
  std::vector<AudioTrack> audio_tracks;
  static TimingInformation empty_timing();

 public:
  DECL_GETTER(filename, std::string)
  DECL_GETTER(policy, std::string)
  DECL_GETTER(filesize, int)
  // -1 selects the first record. Missing or invalid IDs return nullptr.
  // Borrowed pointers require the owning storage to remain alive and unchanged.
  // Owner assignment invalidates them; moves transfer their storage ownership.
  const AudioTrack* get_audio_track(int64_t audio_track_id = -1) const;
  // -1 selects the first video. Audio remains the first argument so existing
  // get_timing(audio_id) calls keep their meaning.
  TimingInformation get_timing(int64_t audio_track_id = -1,
                               int64_t video_track_id = -1) const;
  const VideoTrack* get_video_track(int64_t video_track_id = -1) const;
  std::vector<uint32_t> get_video_track_ids() const;
  std::vector<uint32_t> get_audio_track_ids() const;
  size_t get_num_video_tracks() const { return video_tracks.size(); }
  size_t get_num_audio_tracks() const { return audio_tracks.size(); }
  const std::vector<AudioTrack>& get_audio_tracks() const {
    return audio_tracks;
  }
  const std::vector<VideoTrack>& get_video_tracks() const {
    return video_tracks;
  }

  // @brief Get the library version.
  //
  // @param[out] version: Version string.
  static void get_liblcvm_version(std::string& version);

  // @brief Parse an ISOBMFF file.
  //
  // @param[in] infile: Name of the file to be parsed.
  // @param[in] liblcvm_config: Parsing configuration.
  // @return ptr: Full ISOBMFF information.
  static std::shared_ptr<IsobmffFileInformation> parse(
      const char* infile, const LiblcvmConfig& liblcvm_config);

  // @brief Converts IsobmffFileInformation to 2 generic lists.
  //
  // @param[in] pobj: IsobmffFileInformation object.
  // @param[out] pkeys: List of keys (in-order).
  // @param[out] pvals: List of values (in-order)
  // @param[in] calculate_timestamps: Whether to calculate the timing lists.
  // @param[out] pkeys_timing: List of timing keys (in-order).
  // @param[out] pvals_timing: List of timing values (in-order).
  // @param[in] debug: Debug level.
  // @return int: Error code (0 if ok, !=0 otherwise).
  static int LiblcvmConfig_to_lists(
      std::shared_ptr<IsobmffFileInformation> pobj, LiblcvmKeyList* pkeys,
      LiblcvmValList* pvals, bool calculate_timestamps,
      LiblcvmKeyList* pkeys_timing, LiblcvmTimingList* pvals_timing, int debug,
      int64_t audio_track_id = -1, bool include_track_keys = false,
      int64_t video_track_id = -1);

  // @brief Parse an ISOBMFF file into 2 lists.
  //
  // @param[in] infile: Name of the file to be parsed.
  // @param[in] liblcvm_config: Parsing configuration.
  // @param[out] pkeys: List of keys (in-order).
  // @param[out] pvals: List of values (in-order)
  // @param[in] calculate_timestamps: Whether to calculate the timing lists.
  // @param[out] pkeys_timing: List of timing keys (in-order).
  // @param[out] pvals_timing: List of timing values (in-order).
  // @return int: Error code (0 if ok, !=0 otherwise).
  static int parse_to_lists(const char* infile,
                            const LiblcvmConfig& liblcvm_config,
                            std::vector<std::string>* pkeys,
                            LiblcvmValList* pvals, bool calculate_timestamps,
                            LiblcvmKeyList* pkeys_timing,
                            LiblcvmTimingList* pvals_timing);

  // Private constructor to prevent direct instantiation
  IsobmffFileInformation() = default;

  friend class TimingInformation;
  friend class VideoTrack;
  friend class AudioTrack;
};

#if ADD_POLICY
// policy runner code
int policy_runner(const std::string& policy_str, LiblcvmKeyList* pkeys,
                  LiblcvmValList* pvals, std::list<std::string>* warn_list,
                  std::list<std::string>* error_list, std::string* version);
#endif
