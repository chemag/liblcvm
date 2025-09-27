// liblcvm_c_impl.cpp - Complete C interface implementation for liblcvm
// This provides exception-safe C wrappers for the C++ liblcvm library

#include <liblcvm_c.h>

#include <cstring>
#include <memory>
#include <string>
#include <algorithm>
#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>

// Include the actual liblcvm C++ headers
#include <liblcvm.h>

// Internal wrapper to store C++ objects
struct liblcvm_file_info {
  std::shared_ptr<IsobmffFileInformation> cpp_info;
  std::string last_error;
};

// Utility function to check if file exists
static bool file_exists(const char* filename) {
  struct stat buffer;
  return (stat(filename, &buffer) == 0);
}

// ====================
// Utility Functions
// ====================

static void safe_string_copy(char* dest, const std::string& src, size_t dest_size) {
  size_t copy_size = (src.length() < dest_size - 1) ? src.length() : dest_size - 1;
  strncpy(dest, src.c_str(), copy_size);
  dest[copy_size] = '\0';
}

extern "C" {

const char* liblcvm_get_error_string(liblcvm_error_t error) {
  switch (error) {
    case LIBLCVM_SUCCESS:
      return "Success";
    case LIBLCVM_ERROR_INVALID_PARAMS:
      return "Invalid parameters";
    case LIBLCVM_ERROR_FILE_NOT_FOUND:
      return "File not found";
    case LIBLCVM_ERROR_PARSE_FAILED:
      return "Parse failed";
    case LIBLCVM_ERROR_EXCEPTION:
      return "Exception occurred";
    case LIBLCVM_ERROR_UNKNOWN:
      return "Unknown error";
    case LIBLCVM_ERROR_OUT_OF_MEMORY:
      return "Out of memory";
    default:
      return "Unknown error code";
  }
}

void liblcvm_get_version(char* version, size_t version_size) {
  if (!version || version_size == 0)
    return;

  try {
    std::string version_str;
    IsobmffFileInformation::get_liblcvm_version(version_str);
    safe_string_copy(version, version_str, version_size);
  } catch (...) {
    safe_string_copy(version, "unknown", version_size);
  }
}

// ====================
// Configuration API
// ====================

void liblcvm_config_init(liblcvm_config_t* config) {
  if (!config)
    return;

  config->sort_by_pts = true;
  config->debug = 0;
  config->policy[0] = '\0';
}

// ====================
// Main Analysis API
// ====================

liblcvm_error_t liblcvm_parse_file(
    const char* filename,
    const liblcvm_config_t* config,
    liblcvm_file_info_t* handle) {
  if (!filename || !handle) {
    return LIBLCVM_ERROR_INVALID_PARAMS;
  }

  if (!file_exists(filename)) {
    return LIBLCVM_ERROR_FILE_NOT_FOUND;
  }

  try {
    // Create C++ config
    LiblcvmConfig cpp_config;
    if (config) {
      cpp_config.set_sort_by_pts(config->sort_by_pts);
      cpp_config.set_debug(config->debug);
      if (strlen(config->policy) > 0) {
        cpp_config.set_policy(std::string(config->policy));
      }
    } else {
      // Use defaults
      cpp_config.set_sort_by_pts(true);
      cpp_config.set_debug(0);
    }

    // Parse the file
    auto cpp_info = IsobmffFileInformation::parse(filename, cpp_config);
    if (!cpp_info) {
      return LIBLCVM_ERROR_PARSE_FAILED;
    }

    // Create wrapper
    auto wrapper = new liblcvm_file_info;
    wrapper->cpp_info = cpp_info;
    *handle = wrapper;

    return LIBLCVM_SUCCESS;
  } catch (const std::exception& e) {
    return LIBLCVM_ERROR_EXCEPTION;
  } catch (...) {
    return LIBLCVM_ERROR_UNKNOWN;
  }
}

void liblcvm_free_file_info(liblcvm_file_info_t handle) {
  if (handle) {
    delete handle;
  }
}

// ====================
// Convenience Functions (One-shot analysis)
// ====================

liblcvm_error_t liblcvm_analyze_video_simple(
    const char* video_file_path,
    liblcvm_simple_info_t* info) {
  if (!video_file_path || !info) {
    return LIBLCVM_ERROR_INVALID_PARAMS;
  }

  // Check if file exists
  if (!file_exists(video_file_path)) {
    return LIBLCVM_ERROR_FILE_NOT_FOUND;
  }

  liblcvm_file_info_t handle;
  liblcvm_config_t config;
  liblcvm_config_init(&config);

  liblcvm_error_t result = liblcvm_parse_file(video_file_path, &config, &handle);
  if (result != LIBLCVM_SUCCESS) {
    return result;
  }

  try {
    const auto& cpp_info = handle->cpp_info;
    if (!cpp_info) {
      liblcvm_free_file_info(handle);
      return LIBLCVM_ERROR_PARSE_FAILED;
    }

    // Get timing info for frame count and duration
    const auto& timing = cpp_info->get_timing();
    info->video_frames_count = timing.get_num_video_frames();
    info->video_duration_ms = static_cast<int>(timing.get_duration_video_sec() * 1000.0);
    info->audio_duration_ms = static_cast<int>(timing.get_duration_audio_sec() * 1000.0);

    // Get frame info for bit depth
    const auto& frame = cpp_info->get_frame();
    info->bit_depth = frame.get_bit_depth_luma();

    liblcvm_free_file_info(handle);
    return LIBLCVM_SUCCESS;
  } catch (const std::exception& e) {
    liblcvm_free_file_info(handle);
    return LIBLCVM_ERROR_EXCEPTION;
  } catch (...) {
    liblcvm_free_file_info(handle);
    return LIBLCVM_ERROR_UNKNOWN;
  }
}

// Placeholder implementations for unused functions to avoid linker errors
liblcvm_error_t liblcvm_get_video_analysis(
    liblcvm_file_info_t handle,
    liblcvm_video_analysis_t* analysis) {
  // Not needed for the simple use case but required for linking
  (void)handle;
  (void)analysis;
  return LIBLCVM_ERROR_PARSE_FAILED; // Not implemented
}

liblcvm_error_t liblcvm_get_timing_info(liblcvm_file_info_t handle, liblcvm_timing_info_t* timing) {
  // Not needed for the simple use case but required for linking
  (void)handle;
  (void)timing;
  return LIBLCVM_ERROR_PARSE_FAILED; // Not implemented
}

liblcvm_error_t liblcvm_get_frame_info(liblcvm_file_info_t handle, liblcvm_frame_info_t* frame) {
  // Not needed for the simple use case but required for linking
  (void)handle;
  (void)frame;
  return LIBLCVM_ERROR_PARSE_FAILED; // Not implemented
}

liblcvm_error_t liblcvm_get_audio_info(liblcvm_file_info_t handle, liblcvm_audio_info_t* audio) {
  // Not needed for the simple use case but required for linking
  (void)handle;
  (void)audio;
  return LIBLCVM_ERROR_PARSE_FAILED; // Not implemented
}

liblcvm_error_t liblcvm_get_timing_arrays(
    liblcvm_file_info_t handle,
    liblcvm_timing_arrays_t* arrays) {
  // Not needed for the simple use case but required for linking
  (void)handle;
  (void)arrays;
  return LIBLCVM_ERROR_PARSE_FAILED; // Not implemented
}

void liblcvm_free_timing_arrays(liblcvm_timing_arrays_t* arrays) {
  // Not needed for the simple use case but required for linking
  (void)arrays;
}

liblcvm_error_t liblcvm_analyze_video_advanced(
    const char* video_file_path,
    const liblcvm_config_t* config,
    liblcvm_video_analysis_t* analysis) {
  // Not needed for the simple use case but required for linking
  (void)video_file_path;
  (void)config;
  (void)analysis;
  return LIBLCVM_ERROR_PARSE_FAILED; // Not implemented
}

} // extern "C"
