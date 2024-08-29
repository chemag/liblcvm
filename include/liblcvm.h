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
// @param[out] frame_rate_fps: Frame rate (fps).
// @param[out] frame_drop_count: Frame drop count.
// @param[out] frame_drop_ratio: Frame drop ratio (unitless).
// @param[out] normalized_frame_drop_average_length: Normalized frame drop
// length.
// @param[in] debug: Debug level.
int get_frame_drop_info(const char *infile, int *num_video_frames,
                        float *frame_rate_fps, int *frame_drop_count,
                        float *frame_drop_ratio,
                        float *normalized_frame_drop_average_length, int debug);

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
