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

// @brief Gets the library version.
//
// @param[out] version: Version string.
int get_liblcvm_version(std::string &version);

// @brief Calculates the frame drop info.
//
// @param[in] infile: Name of the file to be parsed.
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
int get_frame_drop_info(const char *infile, int *num_video_frames,
                        float *frame_rate_fps_median,
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
// @param[in] infile: Name of the file to be parsed.
// @param[out] video_freeze: Whether there is a video freeze.
// @param[out] audio_video_ratio: Audio/video length ratio.
// @param[out] duration_video_sec: Video length (seconds).
// @param[out] duration_audio_sec: Audio length (seconds).
// @param[out] timescale_video_hz: Video length (Hz).
// @param[out] timescale_audio_hz: Audio length (Hz).
// @param[in] debug: Debug level.
int get_video_freeze_info(const char *infile, bool *video_freeze,
                          float *audio_video_ratio, float *duration_video_sec,
                          float *duration_audio_sec,
                          uint32_t *timescale_video_hz,
                          uint32_t *timescale_audio_hz, int debug);

// @brief Calculates the video GoP structure info.
//
// @param[in] infile: Name of the file to be parsed.
// @param[out] num_video_frames: Number of video frames in the file.
// @param[out] num_video_keyframes: Number of video key frames in the file.
// @param[in] debug: Debug level.
int get_video_structure_info(const char *infile, int *num_video_frames,
                             int *num_video_keyframes, int debug);

// @brief Calculates the generic video info.
//
// @param[in] infile: Name of the file to be parsed.
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
int get_video_generic_info(const char *infile, int *width, int *height,
                           std::string &type, int *width2, int *height2,
                           int *horizresolution, int *vertresolution,
                           int *depth, int *chroma_format, int *bit_depth_luma,
                           int *bit_depth_chroma, int *video_full_range_flag,
                           int *colour_primaries, int *transfer_characteristics,
                           int *matrix_coeffs, int debug);

// debug API

// @brief Calculates full inter-frame time info.
//
// @param[in] infile: Name of the file to be parsed.
// @param[out] num_video_frames: Number of video frames in the file.
// @param[out] stts_unit_list: STTS values (units).
// @param[out] ctts_unit_list: CTTS values (units).
// @param[out] dts_list: DTS (decoding timestamp) list (seconds).
// @param[out] pts_list: PTS (presentation timestamp) list (seconds).
// @param[in] debug: Debug level.
int get_frame_interframe_info(const char *infile, int *num_video_frames,
                              std::vector<uint32_t> &stts_unit_list,
                              std::vector<uint32_t> &ctts_unit_list,
                              std::vector<float> &dts_list,
                              std::vector<float> &pts_list, int debug);
