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
// @param[in] debug: Debug level.
int get_video_freeze_info(const char *infile, bool *video_freeze,
                          float *audio_video_ratio, float *duration_video_sec,
                          float *duration_audio_sec, int debug);

// debug API

// @brief Calculates full inter-frame time info.
//
// @param[in] infile: Name of the file to be parsed.
// @param[out] num_video_frames: Number of video frames in the file.
// @param[out] delta_timestamp_sec_list: Inter-frame distances (seconds).
// @param[in] debug: Debug level.
int get_frame_interframe_info(const char *infile, int *num_video_frames,
                              std::vector<float> &delta_timestamp_sec_list,
                              int debug);
