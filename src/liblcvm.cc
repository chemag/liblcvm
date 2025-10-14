// liblcvm: Low-Complexity Video Metrics Library.
// A library to detect frame dups and video freezes.

// A show case of using [ISOBMFF](https://github.com/DigiDNA/ISOBMFF) to
// detect frame dups and video freezes in ISOBMFF files.

#include "config.h"
#include "liblcvm.h"

#include <h264_bitstream_parser.h>
#include <h264_common.h>
#include <h264_nal_unit_parser.h>
#include <h265_common.h>
#include <h265_nal_unit_parser.h>
#include <h265_bitstream_parser.h>
#include <inttypes.h>
#include <stdint.h>  // for uint32_t, uint64_t
#include <sys/stat.h>

#include <ISOBMFF.hpp>  // for various
#include <Parser.hpp>   // for isobmff Parser
#include <algorithm>    // for sort
#include <cmath>        // for sqrt
#include <cstdio>       // for fprintf, stderr, stdout
#include <list>         // for list
#include <map>          // for map
#include <memory>       // for shared_ptr, operator==, __shared...
#include <numeric>      // for accumulate
#include <sstream>      // for ostringstream
#include <string>       // for basic_string, string
#include <vector>       // for vector

#if ADD_POLICY
#include "policy_protovisitor.h"
#endif

#define MAX_AUDIO_VIDEO_RATIO 1.05

void IsobmffFileInformation::get_liblcvm_version(std::string &version) {
  version = PROJECT_VER;
}

// variant operation
int liblcvmvalue_to_double(const LiblcvmValue &value, double *result) {
  if (std::holds_alternative<int>(value)) {
    *result = static_cast<double>(std::get<int>(value));
    return 0;
  } else if (std::holds_alternative<double>(value)) {
    *result = std::get<double>(value);
    return 0;
  } else {
    return -1;  // Error: LiblcvmValue is not numeric
  }
}

int liblcvmvalue_to_string(const LiblcvmValue &value, std::string *result) {
  if (std::holds_alternative<std::string>(value)) {
    *result = std::get<std::string>(value);
    return 0;
  } else if (std::holds_alternative<int>(value)) {
    *result = std::to_string(std::get<int>(value));
    return 0;
  } else if (std::holds_alternative<unsigned int>(value)) {
    *result = std::to_string(std::get<unsigned int>(value));
    return 0;
  } else if (std::holds_alternative<long int>(value)) {
    *result = std::to_string(std::get<long int>(value));
    return 0;
  } else if (std::holds_alternative<double>(value)) {
    *result = std::to_string(std::get<double>(value));
    return 0;
  } else {
    return -1;  // Error: LiblcvmValue type is unsupported
  }
}

std::string join_list(const std::list<std::string> &lst,
                      const char *sep = ";") {
  std::ostringstream oss;
  bool first = true;
  for (const auto &s : lst) {
    if (!first) {
      oss << sep;
    }
    oss << s;
    first = false;
  }
  return oss.str();
}

int IsobmffFileInformation::parse_to_lists(const char *infile,
                                           const LiblcvmConfig &liblcvm_config,
                                           std::vector<std::string> *pkeys,
                                           LiblcvmValList *pvals,
                                           bool calculate_timestamps,
                                           LiblcvmKeyList *pkeys_timing,
                                           LiblcvmTimingList *pvals_timing) {
  // Default parsing logic
  std::shared_ptr<IsobmffFileInformation> pobj =
      IsobmffFileInformation::parse(infile, liblcvm_config);
  if (!pobj) {
    fprintf(stderr, "Failed to parse file: %s\n", infile);
    return -1;
  }

  // convert IsobmffFileInformation to list
  int debug = liblcvm_config.get_debug();
  return IsobmffFileInformation::LiblcvmConfig_to_lists(
      pobj, pkeys, pvals, calculate_timestamps, pkeys_timing, pvals_timing,
      debug);
}

int IsobmffFileInformation::LiblcvmConfig_to_lists(
    std::shared_ptr<IsobmffFileInformation> pobj, LiblcvmKeyList *pkeys,
    LiblcvmValList *pvals, bool calculate_timestamps,
    LiblcvmKeyList *pkeys_timing, LiblcvmTimingList *pvals_timing, int debug) {
  // 0. reset all vectors
  pkeys->clear();
  pvals->clear();
  pkeys_timing->clear();
  pvals_timing->clear();

  // 1. fill up the main keys/vals
  pkeys->push_back("infile");
  pvals->push_back(pobj->get_filename());
  pkeys->push_back("filesize");
  pvals->push_back(pobj->get_frame().get_filesize());
  pkeys->push_back("bitrate_bps");
  pvals->push_back(pobj->get_frame().get_bitrate_bps());
  pkeys->push_back("width");
  pvals->push_back(pobj->get_frame().get_width());
  pkeys->push_back("height");
  pvals->push_back(pobj->get_frame().get_height());
  pkeys->push_back("type");
  pvals->push_back(std::string(pobj->get_frame().get_type()));
  pkeys->push_back("horizresolution");
  pvals->push_back(pobj->get_frame().get_horizresolution());
  pkeys->push_back("vertresolution");
  pvals->push_back(pobj->get_frame().get_vertresolution());
  pkeys->push_back("depth");
  pvals->push_back(pobj->get_frame().get_depth());
  pkeys->push_back("chroma_format");
  pvals->push_back(pobj->get_frame().get_chroma_format());
  pkeys->push_back("bit_depth_luma");
  pvals->push_back(pobj->get_frame().get_bit_depth_luma());
  pkeys->push_back("bit_depth_chroma");
  pvals->push_back(pobj->get_frame().get_bit_depth_chroma());
  pkeys->push_back("video_full_range_flag");
  pvals->push_back(pobj->get_frame().get_video_full_range_flag());
  pkeys->push_back("colour_primaries");
  pvals->push_back(pobj->get_frame().get_colour_primaries());
  pkeys->push_back("transfer_characteristics");
  pvals->push_back(pobj->get_frame().get_transfer_characteristics());
  pkeys->push_back("matrix_coeffs");
  pvals->push_back(pobj->get_frame().get_matrix_coeffs());
  pkeys->push_back("profile_idc");
  pvals->push_back(pobj->get_frame().get_profile_idc());
  pkeys->push_back("level_idc");
  pvals->push_back(pobj->get_frame().get_level_idc());
  pkeys->push_back("profile_type_str");
  pvals->push_back(pobj->get_frame().get_profile_type_str());
  pkeys->push_back("num_video_frames");
  pvals->push_back(pobj->get_timing().get_num_video_frames());
  pkeys->push_back("frame_rate_fps_median");
  pvals->push_back(pobj->get_timing().get_frame_rate_fps_median());
  pkeys->push_back("frame_rate_fps_average");
  pvals->push_back(pobj->get_timing().get_frame_rate_fps_average());
  pkeys->push_back("frame_rate_fps_reverse_average");
  pvals->push_back(pobj->get_timing().get_frame_rate_fps_reverse_average());
  pkeys->push_back("frame_rate_fps_stddev");
  pvals->push_back(pobj->get_timing().get_frame_rate_fps_stddev());
  pkeys->push_back("video_freeze");
  pvals->push_back(pobj->get_timing().get_video_freeze() ? 1 : 0);
  pkeys->push_back("audio_video_ratio");
  pvals->push_back(pobj->get_timing().get_audio_video_ratio());
  pkeys->push_back("duration_video_sec");
  pvals->push_back(pobj->get_timing().get_duration_video_sec());
  pkeys->push_back("duration_audio_sec");
  pvals->push_back(pobj->get_timing().get_duration_audio_sec());
  pkeys->push_back("timescale_video_hz");
  pvals->push_back(pobj->get_timing().get_timescale_video_hz());
  pkeys->push_back("timescale_audio_hz");
  pvals->push_back(pobj->get_timing().get_timescale_audio_hz());
  pkeys->push_back("pts_duration_sec_average");
  pvals->push_back(pobj->get_timing().get_pts_duration_sec_average());
  pkeys->push_back("pts_duration_sec_median");
  pvals->push_back(pobj->get_timing().get_pts_duration_sec_median());
  pkeys->push_back("pts_duration_sec_stddev");
  pvals->push_back(pobj->get_timing().get_pts_duration_sec_stddev());
  pkeys->push_back("pts_duration_sec_mad");
  pvals->push_back(pobj->get_timing().get_pts_duration_sec_mad());
  pkeys->push_back("frame_drop_count");
  pvals->push_back(pobj->get_timing().get_frame_drop_count());
  pkeys->push_back("frame_drop_ratio");
  pvals->push_back(pobj->get_timing().get_frame_drop_ratio());
  pkeys->push_back("normalized_frame_drop_average_length");
  pvals->push_back(
      pobj->get_timing().get_normalized_frame_drop_average_length());

  // Percentiles
  std::vector<double> percentile_list = {50, 90};
  std::vector<double> frame_drop_length_percentile_list;
  pobj->get_timing().calculate_percentile_list(
      percentile_list, frame_drop_length_percentile_list, debug);
  pkeys->push_back("frame_drop_length_percentile_50");
  pvals->push_back(frame_drop_length_percentile_list.size() > 0
                       ? frame_drop_length_percentile_list[0]
                       : 0.0);
  pkeys->push_back("frame_drop_length_percentile_90");
  pvals->push_back(frame_drop_length_percentile_list.size() > 1
                       ? frame_drop_length_percentile_list[1]
                       : 0.0);

  // Consecutive frame drop lists
  std::vector<int> consecutive_list = {2, 5};
  std::vector<long int> frame_drop_length_consecutive;
  pobj->get_timing().calculate_consecutive_list(
      consecutive_list, frame_drop_length_consecutive, debug);
  pkeys->push_back("frame_drop_length_consecutive_2");
  pvals->push_back(frame_drop_length_consecutive.size() > 0
                       ? frame_drop_length_consecutive[0]
                       : 0L);
  pkeys->push_back("frame_drop_length_consecutive_5");
  pvals->push_back(frame_drop_length_consecutive.size() > 1
                       ? frame_drop_length_consecutive[1]
                       : 0L);
  pkeys->push_back("num_video_keyframes");
  pvals->push_back(pobj->get_timing().get_num_video_keyframes());
  pkeys->push_back("key_frame_ratio");
  pvals->push_back(pobj->get_timing().get_key_frame_ratio());
  // audio values
  pkeys->push_back("audio_type");
  pvals->push_back(std::string(pobj->get_audio().get_audio_type()));
  pkeys->push_back("channel_count");
  pvals->push_back(pobj->get_audio().get_channel_count());
  pkeys->push_back("sample_rate");
  pvals->push_back(pobj->get_audio().get_sample_rate());
  pkeys->push_back("sample_size");
  pvals->push_back(pobj->get_audio().get_sample_size());

  // 2. run the policy
#if ADD_POLICY
  std::list<std::string> warn_list;
  std::list<std::string> error_list;
  std::string version_str;
  if (!pobj->get_policy().empty()) {
    // Policy string provided, run policy logic
    int policy_status = policy_runner(pobj->get_policy(), pkeys, pvals,
                                      &warn_list, &error_list, &version_str);
    if (policy_status != 0 || !pvals || pvals->empty()) {
      fprintf(stderr, "Policy evaluation failed for file: %s\n",
              pobj->get_filename().c_str());
      if (policy_status != 0) {
        fprintf(stderr, "policy_runner returned error status: %d\n",
                policy_status);
      }
      if (!pvals) {
        fprintf(stderr, "pvals is null!\n");
      } else if (pvals->empty()) {
        fprintf(stderr, "pvals is empty!\n");
      }
      return -1;
    }
  }
  pkeys->push_back("policy_version");
  pvals->push_back(version_str); // Populated by policy parser
  pkeys->push_back("warn_list");
  pvals->push_back(join_list(warn_list));
  pkeys->push_back("error_list");
  pvals->push_back(join_list(error_list));
#endif

  // 3. run the per-file timings
  if (calculate_timestamps) {
    pkeys_timing->insert(
        pkeys_timing->end(),
        {"frame_num_orig", "stts", "ctts", "dts", "pts", "pts_duration",
         "pts_duration_delta", "pts_framerate"});

    std::vector<uint32_t> frame_num_orig_list =
        pobj->get_timing().get_frame_num_orig_list();
    std::vector<uint32_t> stts_unit_list =
        pobj->get_timing().get_stts_unit_list();
    std::vector<int32_t> ctts_unit_list =
        pobj->get_timing().get_ctts_unit_list();
    std::vector<double> dts_sec_list = pobj->get_timing().get_dts_sec_list();
    std::vector<double> pts_sec_list = pobj->get_timing().get_pts_sec_list();
    std::vector<double> pts_duration_sec_list =
        pobj->get_timing().get_pts_duration_sec_list();
    std::vector<double> pts_duration_delta_sec_list =
        pobj->get_timing().get_pts_duration_delta_sec_list();
    std::vector<double> pts_framerate_list =
        pobj->get_timing().get_pts_framerate_list();
    // zip them
    size_t n = frame_num_orig_list.size();
    pvals_timing->reserve(n);
    for (size_t i = 0; i < n; ++i) {
      pvals_timing->emplace_back(
          frame_num_orig_list[i], stts_unit_list[i], ctts_unit_list[i],
          dts_sec_list[i], pts_sec_list[i],
          // we typically have 1 less value
          (i < pts_duration_sec_list.size())
              ? pts_duration_sec_list[i]
              : std::numeric_limits<double>::quiet_NaN(),
          (i < pts_duration_delta_sec_list.size())
              ? pts_duration_delta_sec_list[i]
              : std::numeric_limits<double>::quiet_NaN(),
          (i < pts_framerate_list.size())
              ? pts_framerate_list[i]
              : std::numeric_limits<double>::quiet_NaN());
    }
  }

  return 0;
}

std::shared_ptr<IsobmffFileInformation> IsobmffFileInformation::parse(
    const char *infile, const LiblcvmConfig &liblcvm_config) {
  // 0. create an ISOBMFF configuration object
  std::shared_ptr<IsobmffFileInformation> ptr =
      std::make_shared<IsobmffFileInformation>();
  ptr->filename = infile;
  ptr->policy = liblcvm_config.get_policy();

  // 1. parse the input file
  ISOBMFF::Parser parser;
  ISOBMFF::Error err = parser.Parse(ptr->filename.c_str());
  if (err) {
    fprintf(stderr, "error: %s\n", err.GetMessage().c_str());
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
    double duration_sec = ((double)duration) / timescale_hz;
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
      double dts_sec = ((double)dts_unit) / timescale_hz;
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
            ((double)ptr->timing.pts_unit_list[cur_video_frame]) / timescale_hz;
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
          ((double)ptr->timing.pts_unit_list[cur_video_frame]) / timescale_hz;
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

double calculate_median(const std::vector<double> &vec) {
  if (vec.empty()) {
    fprintf(stderr, "error: calculate_median empty input vector\n");
    return 0.0f;
  }
  std::vector<double> vec2 = vec;
  std::sort(vec2.begin(), vec2.end());
  size_t n = vec2.size();
  if (n % 2 == 0) {
    return (vec2[n / 2 - 1] + vec2[n / 2]) / 2.0f;
  } else {
    return vec2[n / 2];
  }
}

double calculate_average(const std::vector<double> &vec) {
  return std::accumulate(vec.begin(), vec.end(), 0.0f) / vec.size();
}

double calculate_standard_deviation(const std::vector<double> &vec) {
  if (vec.size() < 2) {
    fprintf(stderr,
            "error: calculate_standard_deviation needs at least 2 "
            "elements\n");
    return 0.0f;
  }

  double mean = calculate_average(vec);
  double sum_squares = 0.0f;
  for (const double &x : vec) {
    sum_squares += (x - mean) * (x - mean);
  }

  return std::sqrt(sum_squares / (vec.size() - 1));
}

// https://en.wikipedia.org/wiki/Median_absolute_deviation
double calculate_median_absolute_deviation(const std::vector<double> &vec) {
  // \tilde(X): median(vec)
  double median = calculate_median(vec);
  // |Xi - \tilde(X)|: vector of absolute differences to the median
  std::vector<double> vec_abs_differences(vec.size());
  for (size_t i = 0; i < vec.size(); i++) {
    vec_abs_differences[i] = abs(vec[i] - median);
  }
  // MAD = median(|Xi - \tilde(X)|)
  double mad = calculate_median(vec_abs_differences);
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
    std::vector<double> dts_sec_list_alt(ptr->timing.dts_sec_list.size());
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
    std::vector<double> pts_sec_list_alt(ptr->timing.pts_sec_list.size());
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
        ((double)pts_duration_unit_list[i]) / ptr->timing.timescale_video_hz;
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
  // 3.5 derive the pts_framerate_list (instantaneous framerate)
  ptr->timing.pts_framerate_list.resize(
      ptr->timing.pts_duration_sec_list.size());
  for (uint32_t i = 0; i < ptr->timing.pts_duration_sec_list.size(); ++i) {
    ptr->timing.pts_framerate_list[i] =
        (ptr->timing.pts_duration_sec_list[i] == 0.0)
            ? std::nan("")
            : 1.0 / ptr->timing.pts_duration_sec_list[i];
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
                 ptr->timing.frame_rate_fps_list.begin(), [](double val) {
                   // Handle division by zero
                   return val != 0.0f ? 1.0f / static_cast<double>(val) : 0.0f;
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
  double pts_duration_sec_threshold =
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
  double frame_drop_length_sec_list = 0.0;
  for (const auto &drop_length_sec : ptr->timing.frame_drop_length_sec_list) {
    frame_drop_length_sec_list += drop_length_sec;
  }
  double drop_length_duration_sec =
      frame_drop_length_sec_list -
      ptr->timing.pts_duration_sec_median *
          ptr->timing.frame_drop_length_sec_list.size();
  // drop_length_duration_sec: sum({33.35900000000022, 66.92600000000168, ...})

  // 10. get the total duration as the sum of all the inter-frame distances
  double total_duration_sec = 0.0;
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
    double frame_drop_average_length =
        frame_drop_length_sec_list /
        ptr->timing.frame_drop_length_sec_list.size();
    ptr->timing.normalized_frame_drop_average_length =
        (frame_drop_average_length / ptr->timing.pts_duration_sec_median);
  }

  return 0;
}

void TimingInformation::calculate_percentile_list(
    const std::vector<double> percentile_list,
    std::vector<double> &frame_drop_length_percentile_list, int debug) {
  // calculate percentile list
  frame_drop_length_percentile_list.clear();
  if (frame_drop_length_sec_list.size() > 0) {
    std::sort(frame_drop_length_sec_list.begin(),
              frame_drop_length_sec_list.end());
    for (const double &percentile : percentile_list) {
      int position = (percentile / 100.0) * frame_drop_length_sec_list.size();
      double frame_drop_length_percentile =
          frame_drop_length_sec_list[position] /
          this->get_pts_duration_sec_median();
      frame_drop_length_percentile_list.push_back(frame_drop_length_percentile);
    }
  } else {
    frame_drop_length_percentile_list.resize(percentile_list.size(), 0.0);
  }
}

void TimingInformation::calculate_consecutive_list(
    std::vector<int> consecutive_list,
    std::vector<long int> &frame_drop_length_consecutive, int debug) {
  // calculate consecutive list
  frame_drop_length_consecutive.clear();
  frame_drop_length_consecutive.resize(consecutive_list.size(), 0);
  if (frame_drop_length_sec_list.size() > 0) {
    for (const auto &frame_drop_length_sec : frame_drop_length_sec_list) {
      double drop_length =
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
  ptr->frame.bitrate_bps = 8.0 * ((double)(ptr->frame.filesize)) /
                           ((double)ptr->get_timing().get_duration_video_sec());

  return 0;
}

void FrameInformation::parse_avcc(std::shared_ptr<ISOBMFF::AVCC> avcc,
                                  int debug) {
  // define an avcc parser state
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
  this->profile_idc = -1;
  this->level_idc = -1;
  this->profile_type_str = "";

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
        (nal_unit->nal_unit_payload->sps->sps_data != nullptr)) {
      if ((nal_unit->nal_unit_payload->sps->sps_data->vui_parameters !=
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
        this->video_full_range_flag =
            nal_unit->nal_unit_payload->sps->sps_data->vui_parameters
                ->video_full_range_flag;
      }
      this->profile_idc =
          nal_unit->nal_unit_payload->sps->sps_data->profile_idc;
      this->level_idc = nal_unit->nal_unit_payload->sps->sps_data->level_idc;
      h264nal::profileTypeToString(
          nal_unit->nal_unit_payload->sps->sps_data->profile_type,
          this->profile_type_str);
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
  this->profile_idc = -1;
  this->level_idc = -1;
  this->profile_type_str = "";

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
          (nal_unit->nal_unit_payload->sps != nullptr)) {
        if ((nal_unit->nal_unit_payload->sps->vui_parameters != nullptr) &&
            (nal_unit->nal_unit_payload->sps->vui_parameters_present_flag ==
             1) &&
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
        if ((nal_unit->nal_unit_payload->sps->profile_tier_level != nullptr) &&
            (nal_unit->nal_unit_payload->sps->profile_tier_level->general !=
             nullptr)) {
          this->profile_idc = nal_unit->nal_unit_payload->sps
                                  ->profile_tier_level->general->profile_idc;
          this->level_idc = nal_unit->nal_unit_payload->sps->profile_tier_level
                                ->general_level_idc;
          h265nal::profileTypeToString(
              nal_unit->nal_unit_payload->sps->profile_tier_level->general
                  ->profile_type,
              this->profile_type_str);
        }
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

  // 2. look for hvc1/hev1 or avc1/avc3 (container) boxes
  std::shared_ptr<ISOBMFF::HVC1> hvc1 =
      stsd->GetTypedBox<ISOBMFF::HVC1>("hvc1");
  std::shared_ptr<ISOBMFF::HEV1> hev1 =
      stsd->GetTypedBox<ISOBMFF::HEV1>("hev1");
  std::shared_ptr<ISOBMFF::AVC1> avc1 =
      stsd->GetTypedBox<ISOBMFF::AVC1>("avc1");
  std::shared_ptr<ISOBMFF::AVC3> avc3 =
      stsd->GetTypedBox<ISOBMFF::AVC3>("avc3");

  if (hvc1 != nullptr || hev1 != nullptr) {
    auto hevc_box = (hvc1 != nullptr) ? hvc1 : hev1;
    // 3. look for an hvcC box
    std::shared_ptr<ISOBMFF::HVCC> hvcc =
        hevc_box->GetTypedBox<ISOBMFF::HVCC>("hvcC");
    if (hvcc == nullptr) {
      if (debug > 0) {
        fprintf(stderr,
                "error: no /moov/trak/mdia/minf/stbl/stsd/%s/hvcC in %s\n",
                (hvc1 != nullptr) ? "hvc1" : "hev1", ptr->filename.c_str());
      }
      return -1;
    }
    ptr->frame.type = (hvc1 != nullptr) ? "hvc1" : "hev1";
    ptr->frame.width2 = hevc_box->GetWidth();
    ptr->frame.height2 = hevc_box->GetHeight();
    ptr->frame.horizresolution = hevc_box->GetHorizResolution();
    ptr->frame.vertresolution = hevc_box->GetVertResolution();
    ptr->frame.depth = hevc_box->GetDepth();
    ptr->frame.chroma_format = hvcc->GetChromaFormat();
    ptr->frame.bit_depth_luma = 8 + hvcc->GetBitDepthLumaMinus8();
    ptr->frame.bit_depth_chroma = 8 + hvcc->GetBitDepthChromaMinus8();
    ptr->frame.colour_primaries = -1;
    ptr->frame.transfer_characteristics = -1;
    ptr->frame.matrix_coeffs = -1;
    ptr->frame.video_full_range_flag = -1;
    ptr->frame.parse_hvcc(hvcc, debug);

  } else if (avc1 != nullptr || avc3 != nullptr) {
    auto avc_box = (avc1 != nullptr) ? avc1 : avc3;
    // 4. look for an avcC box
    std::shared_ptr<ISOBMFF::AVCC> avcc =
        avc_box->GetTypedBox<ISOBMFF::AVCC>("avcC");
    if (avcc == nullptr) {
      if (debug > 0) {
        fprintf(stderr,
                "error: no /moov/trak/mdia/minf/stbl/stsd/%s/avcC in %s\n",
                (avc1 != nullptr) ? "avc1" : "avc3", ptr->filename.c_str());
      }
      return -1;
    }
    ptr->frame.type = (avc1 != nullptr) ? "avc1" : "avc3";
    ptr->frame.width2 = avc_box->GetWidth();
    ptr->frame.height2 = avc_box->GetHeight();
    ptr->frame.horizresolution = avc_box->GetHorizResolution();
    ptr->frame.vertresolution = avc_box->GetVertResolution();
    ptr->frame.depth = avc_box->GetDepth();
    ptr->frame.chroma_format = -1;
    ptr->frame.bit_depth_luma = -1;
    ptr->frame.bit_depth_chroma = -1;
    ptr->frame.parse_avcc(avcc, debug);

  } else {
    if (debug > 0) {
      fprintf(stderr,
              "error: cannot find a supported video trak in %s. Searched "
              "/moov/trak/mdia/minf/stbl/stsd/{hvc1|hev1|avc1|avc3}\n",
              ptr->filename.c_str());
    }
    return -1;
  }

  return 0;
}
