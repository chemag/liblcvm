// liblcvm_c.h: C interface for liblcvm to support Android code with
// exceptions disabled
// This provides a C-style API that wraps the C++ liblcvm library with
// proper error handling

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Export macros for proper symbol visibility
#ifdef _WIN32
#ifdef LIBLCVM_C_EXPORTS
#define LIBLCVM_C_API __declspec(dllexport)
#else
#define LIBLCVM_C_API __declspec(dllimport)
#endif
#else
#ifdef LIBLCVM_C_EXPORTS
#define LIBLCVM_C_API __attribute__((visibility("default")))
#else
#define LIBLCVM_C_API
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Error codes
typedef enum {
  LIBLCVM_SUCCESS = 0,
  LIBLCVM_ERROR_INVALID_PARAMS = -1,
  LIBLCVM_ERROR_FILE_NOT_FOUND = -2,
  LIBLCVM_ERROR_PARSE_FAILED = -3,
  LIBLCVM_ERROR_EXCEPTION = -4,
  LIBLCVM_ERROR_UNKNOWN = -5,
  LIBLCVM_ERROR_OUT_OF_MEMORY = -6
} liblcvm_error_t;

// Forward declaration of opaque handle
typedef struct liblcvm_file_info* liblcvm_file_info_t;

// Configuration structure
typedef struct {
  bool sort_by_pts;
  int debug;
  char policy[256];
} liblcvm_config_t;

// Timing information structure
typedef struct {
  int num_video_frames;
  double duration_video_sec;
  double duration_audio_sec;
  uint32_t timescale_video_hz;
  uint32_t timescale_audio_hz;
  int num_video_keyframes;
  double key_frame_ratio;
  double audio_video_ratio;
  bool video_freeze;
  double frame_rate_fps_median;
  double frame_rate_fps_average;
  double frame_rate_fps_reverse_average;
  double frame_rate_fps_stddev;
  int frame_drop_count;
  double frame_drop_ratio;
  double normalized_frame_drop_average_length;
  double pts_duration_sec_average;
  double pts_duration_sec_median;
  double pts_duration_sec_stddev;
  double pts_duration_sec_mad;
} liblcvm_timing_info_t;

// Frame information structure
typedef struct {
  int filesize;
  double bitrate_bps;
  double width;
  double height;
  char video_codec_type[8];  // "hvc1", "hev1", "avc1", or "avc3"
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
} liblcvm_frame_info_t;

// Audio information structure
typedef struct {
  char audio_type[8];  // "mp4a" or "mp4s"
  int channel_count;
  int sample_rate;
  int sample_size;
} liblcvm_audio_info_t;

// Combined information structure for convenience
typedef struct {
  liblcvm_timing_info_t timing;
  liblcvm_frame_info_t frame;
  liblcvm_audio_info_t audio;
  char filename[1024];
  char policy[256];
} liblcvm_video_analysis_t;

// Array structures for detailed timing data
typedef struct {
  uint32_t* frame_nums;
  uint32_t* stts_units;
  int32_t* ctts_units;
  double* dts_seconds;
  double* pts_seconds;
  double* pts_durations;
  double* pts_duration_deltas;
  double* framerate_list;
  uint32_t* keyframe_sample_numbers;
  double* frame_drop_lengths;
  size_t count;
} liblcvm_timing_arrays_t;

// ====================
// Configuration API
// ====================

// Initialize configuration with default values
LIBLCVM_C_API void liblcvm_config_init(liblcvm_config_t* config);

// ====================
// Main Analysis API
// ====================

// Parse video file and return opaque handle
// Returns LIBLCVM_SUCCESS on success, error code otherwise
LIBLCVM_C_API liblcvm_error_t liblcvm_parse_file(const char* filename,
                                                 const liblcvm_config_t* config,
                                                 liblcvm_file_info_t* handle);

// Free the file info handle
LIBLCVM_C_API void liblcvm_free_file_info(liblcvm_file_info_t handle);

// Get all video analysis information at once (convenience function)
LIBLCVM_C_API liblcvm_error_t liblcvm_get_video_analysis(
    liblcvm_file_info_t handle, liblcvm_video_analysis_t* analysis);

// ====================
// Individual Info Getters
// ====================

// Get timing information
LIBLCVM_C_API liblcvm_error_t liblcvm_get_timing_info(
    liblcvm_file_info_t handle, liblcvm_timing_info_t* timing);

// Get frame information
LIBLCVM_C_API liblcvm_error_t
liblcvm_get_frame_info(liblcvm_file_info_t handle, liblcvm_frame_info_t* frame);

// Get audio information
LIBLCVM_C_API liblcvm_error_t
liblcvm_get_audio_info(liblcvm_file_info_t handle, liblcvm_audio_info_t* audio);

// ====================
// Detailed Array Data API
// ====================

// Get detailed timing arrays (caller must free with liblcvm_free_timing_arrays)
LIBLCVM_C_API liblcvm_error_t liblcvm_get_timing_arrays(
    liblcvm_file_info_t handle, liblcvm_timing_arrays_t* arrays);

// Free timing arrays
LIBLCVM_C_API void liblcvm_free_timing_arrays(liblcvm_timing_arrays_t* arrays);

// ====================
// Convenience Functions (One-shot analysis)
// ====================

// Simple analysis function that returns basic metrics (matches original
// wrapper)
typedef struct {
  int video_frames_count;
  int video_duration_ms;
  int audio_duration_ms;
  int bit_depth;
} liblcvm_simple_info_t;

LIBLCVM_C_API liblcvm_error_t liblcvm_analyze_video_simple(
    const char* video_file_path, liblcvm_simple_info_t* info);

// Advanced one-shot analysis
LIBLCVM_C_API liblcvm_error_t liblcvm_analyze_video_advanced(
    const char* video_file_path, const liblcvm_config_t* config,
    liblcvm_video_analysis_t* analysis);

// ====================
// Utility Functions
// ====================

// Get library version
LIBLCVM_C_API void liblcvm_get_version(char* version, size_t version_size);

// Get error string for error code
LIBLCVM_C_API const char* liblcvm_get_error_string(liblcvm_error_t error);

// Convert milliseconds to common telemetry fields for convenience
static inline int liblcvm_duration_sec_to_ms(double duration_sec) {
  return (int)(duration_sec * 1000.0);
}

#ifdef __cplusplus
}
#endif
