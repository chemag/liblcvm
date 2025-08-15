// liblcvm: Low-Complexity Video Metrics Library.
// A library to detect frame dups and video freezes.

// A show case of using [ISOBMFF](https://github.com/DigiDNA/ISOBMFF) to
// detect frame dups and video freezes in ISOBMFF files.

#include "liblcvm.h"

#include <h264_bitstream_parser.h>
#include <h264_common.h>
#include <h264_nal_unit_parser.h>
#include <h265_bitstream_parser.h>
#include <h265_common.h>
#include <h265_nal_unit_parser.h>
#include <inttypes.h>
#include <stdint.h>  // for uint32_t, uint64_t
#include <sys/stat.h>

#include <ISOBMFF.hpp>  // for various
#include <Parser.hpp>   // for isobmff Parser
#include <algorithm>    // for sort
#include <cmath>        // for sqrt
#include <cstdio>       // for fprintf, stderr, stdout
#include <list>
#include <map>      // for map
#include <memory>   // for shared_ptr, operator==, __shared...
#include <numeric>  // for accumulate
#include <string>   // for basic_string, string
#include <vector>   // for vector

#include "config.h"
#include "policy_protovisitor.h"

#define MAX_AUDIO_VIDEO_RATIO 1.05

void IsobmffFileInformation::get_liblcvm_version(std::string &version) {
  version = PROJECT_VER;
}

// variant operation
double to_double(const LiblcvmValue &value) {
  if (std::holds_alternative<int>(value)) {
    return static_cast<double>(std::get<int>(value));
  } else if (std::holds_alternative<double>(value)) {
    return std::get<double>(value);
  } else {
    throw std::runtime_error("LiblcvmValue is not numeric");
  }
}

std::string to_string_value(const LiblcvmValue &value) {
  if (std::holds_alternative<std::string>(value)) {
    return std::get<std::string>(value);
  } else if (std::holds_alternative<int>(value)) {
    return std::to_string(std::get<int>(value));
  } else if (std::holds_alternative<double>(value)) {
    return std::to_string(std::get<double>(value));
  } else {
    throw std::runtime_error("LiblcvmValue type is unsupported");
  }
}

int policy_runner(const std::string &policy_str,
                  std::shared_ptr<std::map<std::string, LiblcvmValue>> pmap,
                  std::list<std::string> *warn_list,
                  std::list<std::string> *error_list);

std::string join_list(const std::list<std::string> &lst,
                      const char *sep = ", ") {
  std::ostringstream oss;
  bool first = true;
  for (const auto &s : lst) {
    if (!first) oss << sep;
    oss << s;
    first = false;
  }

  return oss.str();
}

std::shared_ptr<std::map<std::string, LiblcvmValue>>
IsobmffFileInformation::parse_to_map(const char *infile,
                                     const LiblcvmConfig &liblcvm_config,
                                     const std::string &policy_str) {
  // Helper lambdas
  auto to_double = [](auto v) { return static_cast<double>(v); };
  auto to_int = [](auto v) { return static_cast<int>(v); };

  std::shared_ptr<std::map<std::string, LiblcvmValue>> pmap =
      std::make_shared<std::map<std::string, LiblcvmValue>>();
  // Default parsing logic
  std::shared_ptr<IsobmffFileInformation> pobj =
      IsobmffFileInformation::parse(infile, liblcvm_config);
  if (!pobj) {
    fprintf(stderr, "Failed to parse file: %s\n", infile);
    return nullptr;
  }

  pmap->emplace("infile", std::string(infile));
  // TODO(marko): why to_int() here? filesize should already be an int.
  // Same for all the other to_int() cases.
  pmap->emplace("filesize", to_int(pobj->get_frame().get_filesize()));
  // TODO(marko): move all the floats to double to avoid the conversion
  pmap->emplace("bitrate_bps", to_double(pobj->get_frame().get_bitrate_bps()));
  pmap->emplace("width", to_double(pobj->get_frame().get_width()));
  pmap->emplace("height", to_double(pobj->get_frame().get_height()));
  pmap->emplace("type", std::string(pobj->get_frame().get_type()));
  pmap->emplace("horizresolution",
                to_int(pobj->get_frame().get_horizresolution()));
  pmap->emplace("vertresolution",
                to_int(pobj->get_frame().get_vertresolution()));
  pmap->emplace("depth", to_int(pobj->get_frame().get_depth()));
  pmap->emplace("chroma_format", to_int(pobj->get_frame().get_chroma_format()));
  pmap->emplace("bit_depth_luma",
                to_int(pobj->get_frame().get_bit_depth_luma()));
  pmap->emplace("bit_depth_chroma",
                to_int(pobj->get_frame().get_bit_depth_chroma()));
  pmap->emplace("video_full_range_flag",
                to_int(pobj->get_frame().get_video_full_range_flag()));
  pmap->emplace("colour_primaries",
                to_int(pobj->get_frame().get_colour_primaries()));
  pmap->emplace("transfer_characteristics",
                to_int(pobj->get_frame().get_transfer_characteristics()));
  pmap->emplace("matrix_coeffs", to_int(pobj->get_frame().get_matrix_coeffs()));
  pmap->emplace("num_video_frames",
                to_int(pobj->get_timing().get_num_video_frames()));
  pmap->emplace("frame_rate_fps_median",
                to_double(pobj->get_timing().get_frame_rate_fps_median()));
  pmap->emplace("frame_rate_fps_average",
                to_double(pobj->get_timing().get_frame_rate_fps_average()));
  pmap->emplace(
      "frame_rate_fps_reverse_average",
      to_double(pobj->get_timing().get_frame_rate_fps_reverse_average()));
  pmap->emplace("frame_rate_fps_stddev",
                to_double(pobj->get_timing().get_frame_rate_fps_stddev()));
  pmap->emplace("video_freeze", pobj->get_timing().get_video_freeze() ? 1 : 0);
  pmap->emplace("audio_video_ratio",
                to_double(pobj->get_timing().get_audio_video_ratio()));
  pmap->emplace("duration_video_sec",
                to_double(pobj->get_timing().get_duration_video_sec()));
  pmap->emplace("duration_audio_sec",
                to_double(pobj->get_timing().get_duration_audio_sec()));
  pmap->emplace("timescale_video_hz",
                to_int(pobj->get_timing().get_timescale_video_hz()));
  pmap->emplace("timescale_audio_hz",
                to_int(pobj->get_timing().get_timescale_audio_hz()));
  pmap->emplace("pts_duration_sec_average",
                to_double(pobj->get_timing().get_pts_duration_sec_average()));
  pmap->emplace("pts_duration_sec_median",
                to_double(pobj->get_timing().get_pts_duration_sec_median()));
  pmap->emplace("pts_duration_sec_stddev",
                to_double(pobj->get_timing().get_pts_duration_sec_stddev()));
  pmap->emplace("pts_duration_sec_mad",
                to_double(pobj->get_timing().get_pts_duration_sec_mad()));
  pmap->emplace("frame_drop_count",
                to_int(pobj->get_timing().get_frame_drop_count()));
  pmap->emplace("frame_drop_ratio",
                to_double(pobj->get_timing().get_frame_drop_ratio()));
  pmap->emplace(
      "normalized_frame_drop_average_length",
      to_double(pobj->get_timing().get_normalized_frame_drop_average_length()));

  // Percentiles
  std::vector<float> percentile_list = {50, 90};
  std::vector<float> frame_drop_length_percentile_list;
  pobj->get_timing().calculate_percentile_list(
      percentile_list, frame_drop_length_percentile_list,
      liblcvm_config.get_debug());
  pmap->emplace("frame_drop_length_percentile_50",
                frame_drop_length_percentile_list.size() > 0
                    ? to_double(frame_drop_length_percentile_list[0])
                    : 0.0);
  pmap->emplace("frame_drop_length_percentile_90",
                frame_drop_length_percentile_list.size() > 1
                    ? to_double(frame_drop_length_percentile_list[1])
                    : 0.0);

  // Consecutive frame drop lists
  std::vector<int> consecutive_list = {2, 5};
  std::vector<long int> frame_drop_length_consecutive;
  pobj->get_timing().calculate_consecutive_list(consecutive_list,
                                                frame_drop_length_consecutive,
                                                liblcvm_config.get_debug());
  pmap->emplace("frame_drop_length_consecutive_2",
                frame_drop_length_consecutive.size() > 0
                    ? to_int(frame_drop_length_consecutive[0])
                    : 0);
  pmap->emplace("frame_drop_length_consecutive_5",
                frame_drop_length_consecutive.size() > 1
                    ? to_int(frame_drop_length_consecutive[1])
                    : 0);
  pmap->emplace("num_video_keyframes",
                to_int(pobj->get_timing().get_num_video_keyframes()));
  pmap->emplace("key_frame_ratio",
                to_double(pobj->get_timing().get_key_frame_ratio()));
  pmap->emplace("audio_type", std::string(pobj->get_audio().get_audio_type()));
  pmap->emplace("channel_count", to_int(pobj->get_audio().get_channel_count()));
  pmap->emplace("sample_rate", to_int(pobj->get_audio().get_sample_rate()));
  pmap->emplace("sample_size", to_int(pobj->get_audio().get_sample_size()));

  if (policy_str.empty()) {
    return pmap;
  }

  // Policy string provided, run policy logic
  std::list<std::string> warn_list, error_list;
  int policy_status = policy_runner(policy_str, pmap, &warn_list, &error_list);
  if (policy_status != 0 || !pmap || pmap->empty()) {
    fprintf(stderr, "Policy evaluation failed for file: %s\n", infile);
    if (policy_status != 0) {
      fprintf(stderr, "policy_runner returned error status: %d\n",
              policy_status);
    }
    if (!pmap) {
      fprintf(stderr, "pmap is null!\n");
    } else if (pmap->empty()) {
      fprintf(stderr, "pmap is empty!\n");
    }
    return nullptr;
  }

  pmap->emplace("warn_list", join_list(warn_list));
  pmap->emplace("error_list", join_list(error_list));

  return pmap;
}

std::shared_ptr<IsobmffFileInformation> IsobmffFileInformation::parse(
    const char *infile, const LiblcvmConfig &liblcvm_config) {
  // 0. create a new object
  std::shared_ptr<IsobmffFileInformation> ptr = nullptr;
  try {
    ptr = std::make_shared<IsobmffFileInformation>();
  } catch (...) {
    return nullptr;
  }

  // 0. store the filename
  ptr->filename = infile;

  // 1. parse the input file
  ISOBMFF::Parser parser;
  try {
    parser.Parse(ptr->filename.c_str());
  } catch (std::runtime_error &e) {
    fprintf(stderr, "error: %s\n", e.what());
    return nullptr;
  }
  std::shared_ptr<ISOBMFF::File> file = parser.GetFile();
  if (file == nullptr) {
    if (liblcvm_config.get_debug() > 0) {
      fprintf(stderr, "error: no file in %s\n", ptr->filename.c_str());
    }
    return nullptr;
  }

  // 2. look for a moov container box
  std::shared_ptr<ISOBMFF::ContainerBox> moov =
      file->GetTypedBox<ISOBMFF::ContainerBox>("moov");
  if (moov == nullptr) {
    if (liblcvm_config.get_debug() > 0) {
      fprintf(stderr, "error: no /moov in %s\n", ptr->filename.c_str());
    }
    return nullptr;
  }

  // 3. look for trak container boxes
  ptr->timing.duration_video_sec = -1.0;
  ptr->timing.duration_audio_sec = -1.0;
  for (auto &box : moov->GetBoxes()) {
    std::string name = box->GetName();
    if (name.compare("trak") != 0) {
      continue;
    }
    auto trak = std::dynamic_pointer_cast<ISOBMFF::ContainerBox>(box);

    // 4. look for a mdia container box
    std::shared_ptr<ISOBMFF::ContainerBox> mdia =
        trak->GetTypedBox<ISOBMFF::ContainerBox>("mdia");
    if (mdia == nullptr) {
      if (liblcvm_config.get_debug() > 0) {
        fprintf(stderr, "error: no /moov/trak/mdia in %s\n",
                ptr->filename.c_str());
      }
      return nullptr;
    }

    // 5. look for a hdlr box
    std::shared_ptr<ISOBMFF::HDLR> hdlr =
        mdia->GetTypedBox<ISOBMFF::HDLR>("hdlr");
    if (hdlr == nullptr) {
      if (liblcvm_config.get_debug() > 0) {
        fprintf(stderr, "error: no /moov/trak/mdia/hdlr in %s\n",
                ptr->filename.c_str());
      }
      return nullptr;
    }
    std::string handler_type = hdlr->GetHandlerType();

    // 6. look for a mdhd box
    std::shared_ptr<ISOBMFF::MDHD> mdhd =
        mdia->GetTypedBox<ISOBMFF::MDHD>("mdhd");
    if (mdhd == nullptr) {
      if (liblcvm_config.get_debug() > 0) {
        fprintf(stderr, "error: no /moov/trak/mdia/mdhd in %s\n",
                ptr->filename.c_str());
      }
      return nullptr;
    }
    uint32_t timescale_hz = mdhd->GetTimescale();
    uint64_t duration = mdhd->GetDuration();
    float duration_sec = ((float)duration) / timescale_hz;
    if (liblcvm_config.get_debug() > 1) {
      fprintf(stdout, "-> handler_type: %s ", handler_type.c_str());
      fprintf(stdout, "timescale: %u ", timescale_hz);
      fprintf(stdout, "duration: %" PRIu64 " ", duration);
      fprintf(stdout, "duration_sec: %f\n", duration_sec);
    }
    if (handler_type.compare("soun") == 0) {
      ptr->timing.duration_audio_sec = duration_sec;
      ptr->timing.timescale_audio_hz = timescale_hz;
    } else if (handler_type.compare("vide") == 0) {
      ptr->timing.duration_video_sec = duration_sec;
      ptr->timing.timescale_video_hz = timescale_hz;
    }

    if (handler_type.compare("vide") != 0 &&
        handler_type.compare("soun") != 0) {
      continue;
    }

    // 7. look for a minf container box
    std::shared_ptr<ISOBMFF::ContainerBox> minf =
        mdia->GetTypedBox<ISOBMFF::ContainerBox>("minf");
    if (minf == nullptr) {
      if (liblcvm_config.get_debug() > 0) {
        fprintf(stderr, "error: no /moov/trak/mdia/minf in %s\n",
                ptr->filename.c_str());
      }
      return nullptr;
    }

    // 8. look for a stbl container box
    std::shared_ptr<ISOBMFF::ContainerBox> stbl =
        minf->GetTypedBox<ISOBMFF::ContainerBox>("stbl");
    if (stbl == nullptr) {
      if (liblcvm_config.get_debug() > 0) {
        fprintf(stderr, "error: no /moov/trak/mdia/minf/stbl in %s\n",
                ptr->filename.c_str());
      }
      return nullptr;
    }

    // 8.1 Audio processing
    if (handler_type.compare("soun") == 0) {
      if (ptr->audio.parse_mp4a(stbl, ptr, liblcvm_config.get_debug()) < 0) {
        if (liblcvm_config.get_debug() > 0) {
          fprintf(stderr, "error: in getting audio information in %s\n",
                  ptr->filename.c_str());
        }
        return nullptr;
      }
      continue;
    }

    // 9. look for a tkhd box
    std::shared_ptr<ISOBMFF::TKHD> tkhd =
        trak->GetTypedBox<ISOBMFF::TKHD>("tkhd");
    if (tkhd == nullptr) {
      if (liblcvm_config.get_debug() > 0) {
        fprintf(stderr, "error: no /moov/trak/tkhd in %s\n",
                ptr->filename.c_str());
      }
      return nullptr;
    }
    ptr->frame.width = tkhd->GetWidth();
    ptr->frame.height = tkhd->GetHeight();

    // 10. get video timing information
    // init timing info
    ptr->timing.num_video_frames = 0;
    ptr->timing.dts_sec_list.clear();
    ptr->timing.pts_unit_list.clear();
    ptr->timing.pts_sec_list.clear();
    ptr->timing.stts_unit_list.clear();
    ptr->timing.ctts_unit_list.clear();
    // first frame starts at 0.0
    ptr->timing.dts_sec_list.push_back(0.0);
    ptr->timing.pts_unit_list.push_back(0);
    ptr->timing.pts_sec_list.push_back(0.0);
    if (ptr->timing.parse_timing_information(stbl, timescale_hz, ptr,
                                             liblcvm_config.get_debug()) < 0) {
      if (liblcvm_config.get_debug() > 0) {
        fprintf(stderr, "error: no timing information in %s\n",
                ptr->filename.c_str());
      }
      return nullptr;
    }

    // 11. get video keyframe information
    if (ptr->timing.parse_keyframe_information(
            stbl, ptr, liblcvm_config.get_debug()) < 0) {
      if (liblcvm_config.get_debug() > 0) {
        fprintf(stderr, "error: no keyframe information in %s\n",
                ptr->filename.c_str());
      }
      return nullptr;
    }

    // 12. get video frame information
    if (ptr->frame.parse_frame_information(stbl, ptr,
                                           liblcvm_config.get_debug()) < 0) {
      if (liblcvm_config.get_debug() > 0) {
        fprintf(stderr, "error: no frame information in %s\n",
                ptr->filename.c_str());
      }
      return nullptr;
    }
  }

  // 13. derive timing info
  if (ptr->timing.derive_timing_info(ptr, liblcvm_config.get_sort_by_pts(),
                                     liblcvm_config.get_debug()) < 0) {
    if (liblcvm_config.get_debug() > 0) {
      fprintf(stderr, "error: cannot derive timing information in %s\n",
              ptr->filename.c_str());
    }
    return nullptr;
  }

  // 14. derive frame info
  if (ptr->frame.derive_frame_info(ptr, liblcvm_config.get_sort_by_pts(),
                                   liblcvm_config.get_debug()) < 0) {
    if (liblcvm_config.get_debug() > 0) {
      fprintf(stderr, "error: cannot derive frame information in %s\n",
              ptr->filename.c_str());
    }
    return nullptr;
  }

  return ptr;
}

int TimingInformation::parse_timing_information(
    std::shared_ptr<ISOBMFF::ContainerBox> stbl, uint32_t timescale_hz,
    std::shared_ptr<IsobmffFileInformation> ptr, int debug) {
  // 1. look for a stts box
  std::shared_ptr<ISOBMFF::STTS> stts =
      stbl->GetTypedBox<ISOBMFF::STTS>("stts");
  if (stts == nullptr) {
    if (debug > 0) {
      fprintf(stderr, "error: no /moov/trak/mdia/minf/stbl/stts in %s\n",
              ptr->filename.c_str());
    }
    return -1;
  }

  // 2. gather the stts timestamp durations
  // run all through the stts table
  uint32_t stts_sample_count = 0;
  uint32_t last_dts_unit = 0.0;
  for (unsigned int i = 0; i < stts->GetEntryCount(); i++) {
    uint32_t sample_count = stts->GetSampleCount(i);
    stts_sample_count += sample_count;
    ptr->timing.num_video_frames += sample_count;
    uint32_t sample_offset = stts->GetSampleOffset(i);
    for (uint32_t sample = 0; sample < sample_count; sample++) {
      // store the new stts value
      ptr->timing.stts_unit_list.push_back(sample_offset);
      // set the dts value of the next frame
      uint32_t dts_unit = last_dts_unit + sample_offset;
      float dts_sec = ((float)dts_unit) / timescale_hz;
      ptr->timing.dts_sec_list.push_back(dts_sec);
      // init the pts value of the next frame
      ptr->timing.pts_unit_list.push_back(dts_unit);
      ptr->timing.pts_sec_list.push_back(dts_sec);
      last_dts_unit = dts_unit;
    }
    if (debug > 2) {
      fprintf(stdout, "stts::sample_count: %u ", sample_count);
      fprintf(stdout, "stts::sample_offset: %u ", sample_offset);
    }
  }
  // we need to remove the last element of the dts and pts lists, as we
  // set them pointing at the start of the next frame (inexistent)
  ptr->timing.dts_sec_list.pop_back();
  ptr->timing.pts_unit_list.pop_back();
  ptr->timing.pts_sec_list.pop_back();

  // 3. look for a ctts box
  std::shared_ptr<ISOBMFF::CTTS> ctts =
      stbl->GetTypedBox<ISOBMFF::CTTS>("ctts");
  if (ctts != nullptr) {
    int32_t last_ctts_sample_offset_unit = 0;
    // 10.1. adjust pts list using ctts timestamp durations
    uint32_t ctts_sample_count = 0;
    uint32_t cur_video_frame = 0;
    for (unsigned int i = 0; i < ctts->GetEntryCount(); i++) {
      uint32_t sample_count = ctts->GetSampleCount(i);
      ctts_sample_count += sample_count;
      // update pts_sec_list
      int32_t sample_offset = ctts->GetSampleOffset(i);
      last_ctts_sample_offset_unit = sample_offset;
      for (uint32_t sample = 0; sample < sample_count; sample++) {
        // store the new ctts value
        ptr->timing.ctts_unit_list.push_back(sample_offset);
        // update the pts value
        ptr->timing.pts_unit_list[cur_video_frame] += sample_offset;
        ptr->timing.pts_sec_list[cur_video_frame] =
            ((float)ptr->timing.pts_unit_list[cur_video_frame]) / timescale_hz;
        ++cur_video_frame;
      }
      if (debug > 2) {
        fprintf(stdout, "ctts::sample_count: %u ", sample_count);
        fprintf(stdout, "ctts::sample_offset: %i ", sample_offset);
      }
    }
    // standard suggests that, if there are less ctts than actual samples,
    // the decoder reuses the latest ctts sample offset again and again
    while (cur_video_frame < stts_sample_count) {
      // update the pts value
      ptr->timing.pts_unit_list[cur_video_frame] +=
          last_ctts_sample_offset_unit;
      ptr->timing.pts_sec_list[cur_video_frame] =
          ((float)ptr->timing.pts_unit_list[cur_video_frame]) / timescale_hz;
      ++cur_video_frame;
    }
    if (debug > 2) {
      printf("cur_video_frame: %u\n", cur_video_frame);
      printf("stts_sample_count: %u\n", stts_sample_count);
      printf("ctts_sample_count: %u\n", ctts_sample_count);
    }
  }

  return 0;
}

int TimingInformation::parse_keyframe_information(
    std::shared_ptr<ISOBMFF::ContainerBox> stbl,
    std::shared_ptr<IsobmffFileInformation> ptr, int debug) {
  // look for a stss box in the video track for key frames
  std::shared_ptr<ISOBMFF::STSS> stss =
      stbl->GetTypedBox<ISOBMFF::STSS>("stss");
  ptr->timing.keyframe_sample_number_list.clear();
  if (stss == nullptr) {
    if (debug > 0) {
      fprintf(stderr, "warning: no /moov/trak/mdia/minf/stbl/stss in %s\n",
              ptr->get_filename().c_str());
    }
  } else {
    for (unsigned int i = 0; i < stss->GetEntryCount(); i++) {
      uint32_t sample_count = stss->GetSampleNumber(i);
      ptr->timing.keyframe_sample_number_list.push_back(sample_count);
    }
  }

  return 0;
}

// Function derives an (N-1)-element vector from an N-element vector by
// setting element i-th as:
//   out[i] = in[i] - in[i-1]
//
// The function will allocate all the N-1 elements.
void calculate_vector_deltas_int32_t(const std::vector<int32_t> in,
                                     std::vector<int32_t> &out) {
  out.clear();
  int32_t last_val = -1;
  for (const auto &val : in) {
    if (last_val != -1) {
      out.push_back(val - last_val);
    }
    last_val = val;
  }
}

float calculate_median(const std::vector<float> &vec) {
  if (vec.empty()) {
    fprintf(stderr, "error: calculate_median empty input vector\n");
    return 0.0f;
  }
  std::vector<float> vec2 = vec;
  std::sort(vec2.begin(), vec2.end());
  size_t n = vec2.size();
  if (n % 2 == 0) {
    return (vec2[n / 2 - 1] + vec2[n / 2]) / 2.0f;
  } else {
    return vec2[n / 2];
  }
}

float calculate_average(const std::vector<float> &vec) {
  return std::accumulate(vec.begin(), vec.end(), 0.0f) / vec.size();
}

float calculate_standard_deviation(const std::vector<float> &vec) {
  if (vec.size() < 2) {
    fprintf(stderr,
            "error: calculate_standard_deviation needs at least 2 "
            "elements\n");
    return 0.0f;
  }

  float mean = calculate_average(vec);
  float sum_squares = 0.0f;
  for (const float &x : vec) {
    sum_squares += (x - mean) * (x - mean);
  }

  return std::sqrt(sum_squares / (vec.size() - 1));
}

// https://en.wikipedia.org/wiki/Median_absolute_deviation
float calculate_median_absolute_deviation(const std::vector<float> &vec) {
  // \tilde(X): median(vec)
  float median = calculate_median(vec);
  // |Xi - \tilde(X)|: vector of absolute differences to the median
  std::vector<float> vec_abs_differences(vec.size());
  for (size_t i = 0; i < vec.size(); i++) {
    vec_abs_differences[i] = abs(vec[i] - median);
  }
  // MAD = median(|Xi - \tilde(X)|)
  float mad = calculate_median(vec_abs_differences);
  return mad;
}

int TimingInformation::derive_timing_info(
    std::shared_ptr<IsobmffFileInformation> ptr, bool sort_by_pts, int debug) {
  // 1. set the frame_num_orig_list vector
  ptr->timing.frame_num_orig_list.resize(ptr->timing.pts_sec_list.size());
  for (uint32_t i = 0; i < ptr->timing.pts_sec_list.size(); ++i) {
    ptr->timing.frame_num_orig_list[i] = i;
  }

  // 2. sort the frames by pts value
  if (sort_by_pts) {
    // sort frame_num_orig_list elements based on the values in pts_sec_list
    // TODO(chema): there should be a clear way to access the struct element
    const auto &pts_sec_list = ptr->timing.pts_sec_list;
    std::stable_sort(ptr->timing.frame_num_orig_list.begin(),
                     ptr->timing.frame_num_orig_list.end(),
                     [&pts_sec_list](int a, int b) {
                       return pts_sec_list[a] < pts_sec_list[b];
                     });
    // sort all the others based in the new order
    // 2.1. stts_unit_list
    std::vector<uint32_t> stts_unit_list_alt(ptr->timing.stts_unit_list.size());
    for (uint32_t i = 0; i < ptr->timing.stts_unit_list.size(); ++i) {
      stts_unit_list_alt[i] =
          ptr->timing.stts_unit_list[ptr->timing.frame_num_orig_list[i]];
    }
    ptr->timing.stts_unit_list = stts_unit_list_alt;
    // 2.2. ctts_unit_list
    std::vector<int32_t> ctts_unit_list_alt(ptr->timing.ctts_unit_list.size());
    for (uint32_t i = 0; i < ptr->timing.ctts_unit_list.size(); ++i) {
      ctts_unit_list_alt[i] =
          ptr->timing.ctts_unit_list[ptr->timing.frame_num_orig_list[i]];
    }
    ptr->timing.ctts_unit_list = ctts_unit_list_alt;
    // 2.3. dts_sec_list
    std::vector<float> dts_sec_list_alt(ptr->timing.dts_sec_list.size());
    for (uint32_t i = 0; i < ptr->timing.dts_sec_list.size(); ++i) {
      dts_sec_list_alt[i] =
          ptr->timing.dts_sec_list[ptr->timing.frame_num_orig_list[i]];
    }
    ptr->timing.dts_sec_list = dts_sec_list_alt;
    // 2.4. pts_unit_list
    std::vector<int32_t> pts_unit_list_alt(ptr->timing.pts_unit_list.size());
    for (uint32_t i = 0; i < ptr->timing.pts_unit_list.size(); ++i) {
      pts_unit_list_alt[i] =
          ptr->timing.pts_unit_list[ptr->timing.frame_num_orig_list[i]];
    }
    ptr->timing.pts_unit_list = pts_unit_list_alt;
    // 2.5. pts_sec_list
    std::vector<float> pts_sec_list_alt(ptr->timing.pts_sec_list.size());
    for (uint32_t i = 0; i < ptr->timing.pts_sec_list.size(); ++i) {
      pts_sec_list_alt[i] =
          ptr->timing.pts_sec_list[ptr->timing.frame_num_orig_list[i]];
    }
    ptr->timing.pts_sec_list = pts_sec_list_alt;
  }

  // 3. derived timing values
  // 3.1. calculate the duration (inter-frame distance)
  std::vector<int32_t> pts_duration_unit_list;
  calculate_vector_deltas_int32_t(ptr->timing.pts_unit_list,
                                  pts_duration_unit_list);
  ptr->timing.pts_duration_sec_list.resize(pts_duration_unit_list.size());
  for (uint32_t i = 0; i < pts_duration_unit_list.size(); ++i) {
    ptr->timing.pts_duration_sec_list[i] =
        ((float)pts_duration_unit_list[i]) / ptr->timing.timescale_video_hz;
  }
  // 3.2. calculate the duration average/median
  ptr->timing.pts_duration_sec_average =
      calculate_average(ptr->timing.pts_duration_sec_list);
  ptr->timing.pts_duration_sec_median =
      calculate_median(ptr->timing.pts_duration_sec_list);
  // 3.3. calculate the duration stddev and median absolute difference (MAD)
  ptr->timing.pts_duration_sec_stddev =
      calculate_standard_deviation(ptr->timing.pts_duration_sec_list);
  ptr->timing.pts_duration_sec_mad =
      calculate_median_absolute_deviation(ptr->timing.pts_duration_sec_list);
  // 3.4. derive the pts_duration_delta_sec_list
  ptr->timing.pts_duration_delta_sec_list.resize(
      ptr->timing.pts_duration_sec_list.size());
  for (uint32_t i = 0; i < ptr->timing.pts_duration_sec_list.size(); ++i) {
    ptr->timing.pts_duration_delta_sec_list[i] =
        ptr->timing.pts_duration_sec_list[i] -
        ptr->timing.pts_duration_sec_average;
  }

  // 4. derive keyframe-related values
  ptr->timing.num_video_keyframes =
      ptr->timing.get_keyframe_sample_number_list().size();
  ptr->timing.key_frame_ratio = (ptr->timing.num_video_keyframes > 0)
                                    ? (1.0 * ptr->timing.num_video_frames) /
                                          ptr->timing.num_video_keyframes
                                    : 0.0;

  // 5. audio/video ratio and video freeze info
  // use a default invalid value for audio video ratio
  ptr->timing.audio_video_ratio = -1.0;
  ptr->timing.video_freeze = false;
  if ((ptr->timing.duration_video_sec != -1.0) &&
      (ptr->timing.duration_audio_sec != -1.0) &&
      (ptr->timing.duration_video_sec >= 2.0)) {
    ptr->timing.audio_video_ratio =
        ptr->timing.duration_audio_sec / ptr->timing.duration_video_sec;
    ptr->timing.video_freeze =
        ptr->timing.audio_video_ratio > MAX_AUDIO_VIDEO_RATIO;
  }

  // 6. calculate framerate statistics
  // 6.1. get the framerate series
  ptr->timing.frame_rate_fps_list.resize(
      ptr->timing.pts_duration_sec_list.size());
  std::transform(ptr->timing.pts_duration_sec_list.begin(),
                 ptr->timing.pts_duration_sec_list.end(),
                 ptr->timing.frame_rate_fps_list.begin(), [](float val) {
                   // Handle division by zero
                   return val != 0.0f ? 1.0f / static_cast<float>(val) : 0.0f;
                 });
  // 6.2. median
  ptr->timing.frame_rate_fps_median =
      calculate_median(ptr->timing.frame_rate_fps_list);
  // 6.3. average
  ptr->timing.frame_rate_fps_average =
      calculate_average(ptr->timing.frame_rate_fps_list);
  // 6.4. reverse average
  // Considering the sample_duration of different frames inside boxes
  // as a series X: {x1, x2, ..., xn}, for calculating average FPS from this,
  // consider the reciprocal series Y = 1/X = {1/x1, 1/x2, ..., 1/xn}
  // The average of X is $\hat{X}$.
  // The average of Y is $\hat{Y}$.
  // A single, very small number $xi$ can cause this average ($\hat{Y}$) to be
  // extreme, leading to extreme values in frame_rate_fps_average calculation.
  // In contrast, the reverse of the average of X ($1/\hat{X}$) should not
  // have this biased to extreme value.
  ptr->timing.frame_rate_fps_reverse_average =
      1.0 / ptr->timing.pts_duration_sec_average;
  // 6.5. stddev
  ptr->timing.frame_rate_fps_stddev =
      calculate_standard_deviation(ptr->timing.frame_rate_fps_list);

  // 7. calculate the threshold to consider frame drop: This should be 2
  // times the median, minus a factor
  double FACTOR = 0.75;
  float pts_duration_sec_threshold =
      ptr->timing.pts_duration_sec_median * FACTOR * 2;

  // 8. get the list of all the drops (absolute inter-frame values)
  for (const auto &pts_duration_sec : ptr->timing.pts_duration_sec_list) {
    if (pts_duration_sec > pts_duration_sec_threshold) {
      ptr->timing.frame_drop_length_sec_list.push_back(pts_duration_sec);
    }
  }
  // ptr->timing.frame_drop_length_sec_list: {0.6668900000000022,
  // 0.10025600000000168,
  // ...}

  // 9. sum all the drops, but adding only the length over 1x frame time
  float frame_drop_length_sec_list = 0.0;
  for (const auto &drop_length_sec : ptr->timing.frame_drop_length_sec_list) {
    frame_drop_length_sec_list += drop_length_sec;
  }
  float drop_length_duration_sec =
      frame_drop_length_sec_list -
      ptr->timing.pts_duration_sec_median *
          ptr->timing.frame_drop_length_sec_list.size();
  // drop_length_duration_sec: sum({33.35900000000022, 66.92600000000168, ...})

  // 10. get the total duration as the sum of all the inter-frame distances
  float total_duration_sec = 0.0;
  for (const auto &pts_duration_sec : ptr->timing.pts_duration_sec_list) {
    total_duration_sec += pts_duration_sec;
  }

  // 11. calculate frame drop ratio as extra drop length over total duration
  ptr->timing.frame_drop_ratio = drop_length_duration_sec / total_duration_sec;
  ptr->timing.frame_drop_count =
      int((ptr->timing.frame_drop_ratio) * (ptr->timing.num_video_frames));

  // 12. calculate average drop length, normalized to framerate. Note that
  // a single frame drop is a normalized frame drop length of 2. When
  // frame drops are uncorrelated, the normalized average drop length
  // should be close to 2
  ptr->timing.normalized_frame_drop_average_length = 0.0;
  if (ptr->timing.frame_drop_length_sec_list.size() > 0) {
    float frame_drop_average_length =
        frame_drop_length_sec_list /
        ptr->timing.frame_drop_length_sec_list.size();
    ptr->timing.normalized_frame_drop_average_length =
        (frame_drop_average_length / ptr->timing.pts_duration_sec_median);
  }

  return 0;
}

void TimingInformation::calculate_percentile_list(
    const std::vector<float> percentile_list,
    std::vector<float> &frame_drop_length_percentile_list, int debug) {
  // calculate percentile list
  frame_drop_length_percentile_list.clear();
  if (frame_drop_length_sec_list.size() > 0) {
    std::sort(frame_drop_length_sec_list.begin(),
              frame_drop_length_sec_list.end());
    for (const float &percentile : percentile_list) {
      int position = (percentile / 100.0) * frame_drop_length_sec_list.size();
      float frame_drop_length_percentile =
          frame_drop_length_sec_list[position] /
          this->get_pts_duration_sec_median();
      frame_drop_length_percentile_list.push_back(frame_drop_length_percentile);
    }
  } else {
    for (const float &_ : percentile_list) {
      frame_drop_length_percentile_list.push_back(0.0);
    }
  }
}

void TimingInformation::calculate_consecutive_list(
    std::vector<int> consecutive_list,
    std::vector<long int> &frame_drop_length_consecutive, int debug) {
  // calculate consecutive list
  frame_drop_length_consecutive.clear();
  for (int _ : consecutive_list) {
    frame_drop_length_consecutive.push_back(0);
  }
  if (frame_drop_length_sec_list.size() > 0) {
    for (const auto &frame_drop_length_sec : frame_drop_length_sec_list) {
      float drop_length =
          frame_drop_length_sec / this->get_pts_duration_sec_median();
      for (unsigned int i = 0; i < consecutive_list.size(); i++) {
        if (drop_length >= consecutive_list[i]) {
          frame_drop_length_consecutive[i]++;
        }
      }
    }
  }
}

int FrameInformation::derive_frame_info(
    std::shared_ptr<IsobmffFileInformation> ptr, bool sort_by_pts, int debug) {
  // 1. get basic file info
  struct stat stat_buf;
  int rc = stat(ptr->get_filename().c_str(), &stat_buf);
  if (rc < 0) {
    fprintf(stderr, "error: cannot access %s\n", ptr->filename.c_str());
    return -1;
  }
  ptr->frame.filesize = stat_buf.st_size;
  ptr->frame.bitrate_bps = 8.0 * ((float)(ptr->frame.filesize)) /
                           ((float)ptr->get_timing().get_duration_video_sec());

  return 0;
}

void FrameInformation::parse_avcc(std::shared_ptr<ISOBMFF::AVCC> avcc,
                                  int debug) {
  // define an hevc parser state
  h264nal::H264BitstreamParserState bitstream_parser_state;
  std::unique_ptr<h264nal::H264BitstreamParser::BitstreamState> bitstream;
  h264nal::ParsingOptions parsing_options;
  parsing_options.add_offset = false;
  parsing_options.add_length = false;
  parsing_options.add_parsed_length = false;
  parsing_options.add_checksum = false;
  parsing_options.add_resolution = false;

  // set default values
  this->colour_primaries = -1;
  this->transfer_characteristics = -1;
  this->matrix_coeffs = -1;
  this->video_full_range_flag = -1;

  // extract the SPS NAL Units
  for (const auto &sps : avcc->GetSequenceParameterSetNALUnits()) {
    std::vector<uint8_t> buffer = sps->GetData();
    auto nal_unit = h264nal::H264NalUnitParser::ParseNalUnit(
        buffer.data(), buffer.size(), &bitstream_parser_state, parsing_options);
    if (nal_unit == nullptr) {
      // cannot parse the NalUnit
      continue;
    }

    // Look for valid SPS NAL units
    if ((nal_unit->nal_unit_payload != nullptr) &&
        (nal_unit->nal_unit_payload->sps != nullptr) &&
        (nal_unit->nal_unit_payload->sps->sps_data != nullptr) &&
        (nal_unit->nal_unit_payload->sps->sps_data->vui_parameters !=
         nullptr) &&
        (nal_unit->nal_unit_payload->sps->sps_data->vui_parameters
             ->colour_description_present_flag == 1) &&
        (nal_unit->nal_unit_payload->sps->sps_data
             ->vui_parameters_present_flag == 1)) {
      this->colour_primaries = nal_unit->nal_unit_payload->sps->sps_data
                                   ->vui_parameters->colour_primaries;
      this->transfer_characteristics =
          nal_unit->nal_unit_payload->sps->sps_data->vui_parameters
              ->transfer_characteristics;
      this->matrix_coeffs = nal_unit->nal_unit_payload->sps->sps_data
                                ->vui_parameters->matrix_coefficients;
      this->video_full_range_flag = nal_unit->nal_unit_payload->sps->sps_data
                                        ->vui_parameters->video_full_range_flag;
    }
  }
}

void FrameInformation::parse_hvcc(std::shared_ptr<ISOBMFF::HVCC> hvcc,
                                  int debug) {
  // define an hevc parser state
  h265nal::H265BitstreamParserState bitstream_parser_state;
  std::unique_ptr<h265nal::H265BitstreamParser::BitstreamState> bitstream;
  h265nal::ParsingOptions parsing_options;
  parsing_options.add_offset = false;
  parsing_options.add_length = false;
  parsing_options.add_parsed_length = false;
  parsing_options.add_checksum = false;
  parsing_options.add_resolution = false;

  // set default values
  this->colour_primaries = -1;
  this->transfer_characteristics = -1;
  this->matrix_coeffs = -1;
  this->video_full_range_flag = -1;

  // extract the NAL Units
  for (const auto &array : hvcc->GetArrays()) {
    // bool array_completeness = array->GetArrayCompleteness();
    uint8_t nal_unit_type = array->GetNALUnitType();
    for (const auto &data : array->GetNALUnits()) {
      std::vector<uint8_t> buffer = data->GetData();
      auto nal_unit = h265nal::H265NalUnitParser::ParseNalUnit(
          buffer.data(), buffer.size(), &bitstream_parser_state,
          parsing_options);
      if (nal_unit == nullptr) {
        // cannot parse the NalUnit
        continue;
      }

      // Look for SPS NAL units
      if ((nal_unit_type == h265nal::NalUnitType::SPS_NUT) &&
          (nal_unit->nal_unit_payload != nullptr) &&
          (nal_unit->nal_unit_payload->sps != nullptr) &&
          (nal_unit->nal_unit_payload->sps->vui_parameters != nullptr) &&
          (nal_unit->nal_unit_payload->sps->vui_parameters_present_flag == 1) &&
          (nal_unit->nal_unit_payload->sps->vui_parameters
               ->colour_description_present_flag == 1)) {
        this->colour_primaries =
            nal_unit->nal_unit_payload->sps->vui_parameters->colour_primaries;
        this->transfer_characteristics =
            nal_unit->nal_unit_payload->sps->vui_parameters
                ->transfer_characteristics;
        this->matrix_coeffs =
            nal_unit->nal_unit_payload->sps->vui_parameters->matrix_coeffs;
        this->video_full_range_flag =
            nal_unit->nal_unit_payload->sps->vui_parameters
                ->video_full_range_flag;
      }
    }
  }
}

int AudioInformation::parse_mp4a(std::shared_ptr<ISOBMFF::ContainerBox> stbl,
                                 std::shared_ptr<IsobmffFileInformation> ptr,
                                 int debug) {
  // 1. look for a stsd container box
  std::shared_ptr<ISOBMFF::STSD> stsd =
      stbl->GetTypedBox<ISOBMFF::STSD>("stsd");
  if (stsd == nullptr) {
    if (debug > 0) {
      fprintf(stderr, "error: no /moov/trak/mdia/minf/stbl/stsd in %s\n",
              ptr->filename.c_str());
    }
    return -1;
  }

  // 2. look for a MP4A container box
  std::shared_ptr<ISOBMFF::MP4A> mp4a =
      stsd->GetTypedBox<ISOBMFF::MP4A>("mp4a");
  if (mp4a == nullptr) {
    if (debug > 0) {
      fprintf(stderr, "error: no /moov/trak2/mdia/minf/stbl/stsd/mp4a in %s\n",
              ptr->filename.c_str());
    }
    return -1;
  }
  ptr->audio.audio_type = "mp4a";
  ptr->audio.channel_count = mp4a->GetChannelCount();
  ptr->audio.sample_size = mp4a->GetSampleSize();
  ptr->audio.sample_rate = mp4a->GetSampleRate();
  return 0;
}

int FrameInformation::parse_frame_information(
    std::shared_ptr<ISOBMFF::ContainerBox> stbl,
    std::shared_ptr<IsobmffFileInformation> ptr, int debug) {
  // 1. look for a stsd container box
  std::shared_ptr<ISOBMFF::STSD> stsd =
      stbl->GetTypedBox<ISOBMFF::STSD>("stsd");
  if (stsd == nullptr) {
    if (debug > 0) {
      fprintf(stderr, "error: no /moov/trak/mdia/minf/stbl/stsd in %s\n",
              ptr->filename.c_str());
    }
    return -1;
  }

  // 2. look for hvc1 or avc1 (container) boxes
  std::shared_ptr<ISOBMFF::HVC1> hvc1 =
      stsd->GetTypedBox<ISOBMFF::HVC1>("hvc1");
  std::shared_ptr<ISOBMFF::AVC1> avc1 =
      stsd->GetTypedBox<ISOBMFF::AVC1>("avc1");

  if (hvc1 != nullptr) {
    // 3. look for an hvcC box
    std::shared_ptr<ISOBMFF::HVCC> hvcc =
        hvc1->GetTypedBox<ISOBMFF::HVCC>("hvcC");
    if (hvcc == nullptr) {
      if (debug > 0) {
        fprintf(stderr,
                "error: no /moov/trak/mdia/minf/stbl/stsd/hvc1/hvcC in %s\n",
                ptr->filename.c_str());
      }
      return -1;
    }
    ptr->frame.type = "hvc1";
    ptr->frame.width2 = hvc1->GetWidth();
    ptr->frame.height2 = hvc1->GetHeight();
    ptr->frame.horizresolution = hvc1->GetHorizResolution();
    ptr->frame.vertresolution = hvc1->GetVertResolution();
    ptr->frame.depth = hvc1->GetDepth();
    ptr->frame.chroma_format = hvcc->GetChromaFormat();
    ptr->frame.bit_depth_luma = 8 + hvcc->GetBitDepthLumaMinus8();
    ptr->frame.bit_depth_chroma = 8 + hvcc->GetBitDepthChromaMinus8();
    ptr->frame.colour_primaries = -1;
    ptr->frame.transfer_characteristics = -1;
    ptr->frame.matrix_coeffs = -1;
    ptr->frame.video_full_range_flag = -1;
    ptr->frame.parse_hvcc(hvcc, debug);

  } else if (avc1 != nullptr) {
    // 4. look for an avcC box
    std::shared_ptr<ISOBMFF::AVCC> avcc =
        avc1->GetTypedBox<ISOBMFF::AVCC>("avcC");
    if (avcc == nullptr) {
      if (debug > 0) {
        fprintf(stderr,
                "error: no /moov/trak/mdia/minf/stbl/stsd/avc1/avcC in %s\n",
                ptr->filename.c_str());
      }
      return -1;
    }
    ptr->frame.type = "avc1";
    ptr->frame.width2 = avc1->GetWidth();
    ptr->frame.height2 = avc1->GetHeight();
    ptr->frame.horizresolution = avc1->GetHorizResolution();
    ptr->frame.vertresolution = avc1->GetVertResolution();
    ptr->frame.depth = avc1->GetDepth();
    ptr->frame.chroma_format = -1;
    ptr->frame.bit_depth_luma = -1;
    ptr->frame.bit_depth_chroma = -1;
    ptr->frame.parse_avcc(avcc, debug);

  } else {
    if (debug > 0) {
      fprintf(stderr,
              "error: no /moov/trak/mdia/minf/stbl/stsd/hvc1 or "
              "/moov/trak/mdia/minf/stbl/stsd/avc1 in %s\n",
              ptr->filename.c_str());
    }
    return -1;
  }

  return 0;
}
