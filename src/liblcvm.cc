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
#include <list>         // for list
#include <map>          // for map
#include <memory>       // for shared_ptr, operator==, __shared...
#include <numeric>      // for accumulate
#include <sstream>      // for ostringstream
#include <stdexcept>    // for out_of_range
#include <string>       // for basic_string, string
#include <vector>       // for vector

#include "config.h"

#if ADD_POLICY
#include "policy_protovisitor.h"
#endif

#define MAX_AUDIO_VIDEO_RATIO 1.05

// The existing duration-mismatch heuristic, shared by default and selected
// audio views. This is not pixel-based freeze detection.
static void derive_audio_video_metrics(double video_sec, double audio_sec,
                                       double& ratio, bool& freeze) {
  ratio = -1.0;
  freeze = false;
  if (video_sec != -1.0 && audio_sec != -1.0 && video_sec >= 2.0) {
    ratio = audio_sec / video_sec;
    freeze = ratio > MAX_AUDIO_VIDEO_RATIO;
  }
}

AudioTrack::AudioTrack() = default;
VideoTrack::VideoTrack() = default;

const AudioTrack* IsobmffFileInformation::get_audio_track(int64_t id) const {
  if (id == -1)
    return audio_tracks.empty() ? nullptr : &audio_tracks.front();
  if (id < 0 || static_cast<uint64_t>(id) > UINT32_MAX)
    return nullptr;
  for (const auto& track : audio_tracks)
    if (track.get_track_id() == static_cast<uint32_t>(id))
      return &track;
  return nullptr;
}
const VideoTrack* IsobmffFileInformation::get_video_track(int64_t id) const {
  if (id == -1)
    return video_tracks.empty() ? nullptr : &video_tracks.front();
  if (id < 0 || static_cast<uint64_t>(id) > UINT32_MAX)
    return nullptr;
  for (const auto& track : video_tracks)
    if (track.get_track_id() == static_cast<uint32_t>(id))
      return &track;
  return nullptr;
}
std::vector<uint32_t> IsobmffFileInformation::get_video_track_ids() const {
  std::vector<uint32_t> ids;
  for (const auto& track : video_tracks)
    ids.push_back(track.get_track_id());
  return ids;
}
std::vector<uint32_t> IsobmffFileInformation::get_audio_track_ids() const {
  std::vector<uint32_t> ids;
  for (const auto& track : audio_tracks)
    ids.push_back(track.get_track_id());
  return ids;
}
AudioVideoTimingInformation IsobmffFileInformation::get_timing(
    int64_t audio_id, int64_t video_id) const {
  const auto* audio = get_audio_track(audio_id);
  if (!audio && audio_id != -1) {
    const bool invalid =
        audio_id < 0 || static_cast<uint64_t>(audio_id) > UINT32_MAX;
    throw std::out_of_range(std::string(invalid
                                            ? "invalid audio track selector: "
                                            : "audio track ID not found: ") +
                            std::to_string(audio_id));
  }
  const auto* video = get_video_track(video_id);
  if (!video && video_id != -1) {
    const bool invalid =
        video_id < 0 || static_cast<uint64_t>(video_id) > UINT32_MAX;
    throw std::out_of_range(std::string(invalid
                                            ? "invalid video track selector: "
                                            : "video track ID not found: ") +
                            std::to_string(video_id));
  }
  AudioVideoTimingInformation result;
  static_cast<TimingInformation&>(result) =
      video ? video->get_timing() : empty_timing();
  result.timescale_movie_hz = timescale_movie_hz;
  result.duration_audio_sec =
      audio ? audio->get_timing().get_duration_sec() : -1.0;
  result.timescale_audio_hz =
      audio ? audio->get_timing().get_timescale_hz() : 0;
  derive_audio_video_metrics(result.get_duration_video_sec(),
                             result.duration_audio_sec,
                             result.audio_video_ratio, result.video_freeze);
  return result;
}

void IsobmffFileInformation::get_liblcvm_version(std::string& version) {
  version = PROJECT_VER;
}

// variant operation
int liblcvmvalue_to_double(const LiblcvmValue& value, double* result) {
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

int liblcvmvalue_to_string(const LiblcvmValue& value, std::string* result) {
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

std::string join_list(const std::list<std::string>& lst,
                      const char* sep = ";") {
  std::ostringstream oss;
  bool first = true;
  for (const auto& s : lst) {
    if (!first) {
      oss << sep;
    }
    oss << s;
    first = false;
  }
  return oss.str();
}

int IsobmffFileInformation::parse_to_lists(const char* infile,
                                           const LiblcvmConfig& liblcvm_config,
                                           std::vector<std::string>* pkeys,
                                           LiblcvmValList* pvals,
                                           bool calculate_timestamps,
                                           LiblcvmKeyList* pkeys_timing,
                                           LiblcvmTimingList* pvals_timing) {
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
      debug, liblcvm_config.get_audio_track_id(),
      liblcvm_config.get_include_track_keys(),
      liblcvm_config.get_video_track_id());
}

static void fill_base_fields(
    const std::shared_ptr<IsobmffFileInformation>& pobj,
    const VideoTrack* selected_video,
    AudioVideoTimingInformation& selected_timing,
    const AudioTrack* selected_audio, LiblcvmKeyList* pkeys,
    LiblcvmValList* pvals, int debug) {
  const VideoTrack empty_video;
  const AudioTrack empty_audio;
  const auto& video = selected_video ? *selected_video : empty_video;
  const auto& audio = selected_audio ? *selected_audio : empty_audio;
  pkeys->push_back("infile");
  pvals->push_back(pobj->get_filename());
  pkeys->push_back("filesize");
  pvals->push_back(pobj->get_filesize());
  pkeys->push_back("bitrate_bps");
  pvals->push_back(selected_video ? video.get_bitrate_bps()
                                  : -8.0 * pobj->get_filesize());
  pkeys->push_back("width");
  pvals->push_back(video.get_width());
  pkeys->push_back("height");
  pvals->push_back(video.get_height());
  pkeys->push_back("video_codec_type");
  pvals->push_back(video.get_video_codec_type());
  pkeys->push_back("horizresolution");
  pvals->push_back(video.get_horizresolution());
  pkeys->push_back("vertresolution");
  pvals->push_back(video.get_vertresolution());
  pkeys->push_back("depth");
  pvals->push_back(video.get_depth());
  pkeys->push_back("chroma_format");
  pvals->push_back(video.get_chroma_format());
  pkeys->push_back("bit_depth_luma");
  pvals->push_back(video.get_bit_depth_luma());
  pkeys->push_back("bit_depth_chroma");
  pvals->push_back(video.get_bit_depth_chroma());
  pkeys->push_back("video_full_range_flag");
  pvals->push_back(video.get_video_full_range_flag());
  pkeys->push_back("colour_primaries");
  pvals->push_back(video.get_colour_primaries());
  pkeys->push_back("transfer_characteristics");
  pvals->push_back(video.get_transfer_characteristics());
  pkeys->push_back("matrix_coeffs");
  pvals->push_back(video.get_matrix_coeffs());
  pkeys->push_back("profile_idc");
  pvals->push_back(video.get_profile_idc());
  pkeys->push_back("level_idc");
  pvals->push_back(video.get_level_idc());
  pkeys->push_back("profile_type_str");
  pvals->push_back(video.get_profile_type_str());
  pkeys->push_back("num_video_frames");
  pvals->push_back(selected_timing.get_num_video_frames());
  pkeys->push_back("frame_rate_fps_median");
  pvals->push_back(selected_timing.get_frame_rate_fps_median());
  pkeys->push_back("frame_rate_fps_average");
  pvals->push_back(selected_timing.get_frame_rate_fps_average());
  pkeys->push_back("frame_rate_fps_reverse_average");
  pvals->push_back(selected_timing.get_frame_rate_fps_reverse_average());
  pkeys->push_back("frame_rate_fps_stddev");
  pvals->push_back(selected_timing.get_frame_rate_fps_stddev());
  pkeys->push_back("video_freeze");
  const bool unsupported_pair =
      (selected_video && !selected_video->is_supported()) ||
      (selected_audio && !selected_audio->is_supported());
  pvals->push_back(
      unsupported_pair ? -1 : (selected_timing.get_video_freeze() ? 1 : 0));
  pkeys->push_back("audio_video_ratio");
  pvals->push_back(selected_timing.get_audio_video_ratio());
  pkeys->push_back("duration_video_sec");
  pvals->push_back(selected_timing.get_duration_video_sec());
  pkeys->push_back("duration_audio_sec");
  pvals->push_back(selected_timing.get_duration_audio_sec());
  pkeys->push_back("timescale_movie_hz");
  pvals->push_back(selected_timing.get_timescale_movie_hz());
  pkeys->push_back("timescale_video_hz");
  pvals->push_back(selected_timing.get_timescale_video_hz());
  pkeys->push_back("timescale_audio_hz");
  pvals->push_back(selected_timing.get_timescale_audio_hz());
  pkeys->push_back("pts_duration_sec_average");
  pvals->push_back(selected_timing.get_pts_duration_sec_average());
  pkeys->push_back("pts_duration_sec_median");
  pvals->push_back(selected_timing.get_pts_duration_sec_median());
  pkeys->push_back("pts_duration_sec_stddev");
  pvals->push_back(selected_timing.get_pts_duration_sec_stddev());
  pkeys->push_back("pts_duration_sec_mad");
  pvals->push_back(selected_timing.get_pts_duration_sec_mad());
  pkeys->push_back("frame_drop_count");
  pvals->push_back(selected_timing.get_frame_drop_count());
  pkeys->push_back("frame_drop_ratio");
  pvals->push_back(selected_timing.get_frame_drop_ratio());
  pkeys->push_back("normalized_frame_drop_average_length");
  pvals->push_back(selected_timing.get_normalized_frame_drop_average_length());
  std::vector<double> percentiles;
  selected_timing.calculate_percentile_list({50, 90}, percentiles, debug);
  pkeys->push_back("frame_drop_length_percentile_50");
  pvals->push_back(percentiles.size() > 0 ? percentiles[0] : 0.0);
  pkeys->push_back("frame_drop_length_percentile_90");
  pvals->push_back(percentiles.size() > 1 ? percentiles[1] : 0.0);
  std::vector<long int> consecutive;
  selected_timing.calculate_consecutive_list({2, 5}, consecutive, debug);
  pkeys->push_back("frame_drop_length_consecutive_2");
  pvals->push_back(consecutive.size() > 0 ? consecutive[0] : 0L);
  pkeys->push_back("frame_drop_length_consecutive_5");
  pvals->push_back(consecutive.size() > 1 ? consecutive[1] : 0L);
  pkeys->push_back("num_video_keyframes");
  pvals->push_back(selected_timing.get_num_video_keyframes());
  pkeys->push_back("key_frame_ratio");
  pvals->push_back(selected_timing.get_key_frame_ratio());
  pkeys->push_back("audio_type");
  pvals->push_back(audio.get_audio_type());
  pkeys->push_back("channel_count");
  pvals->push_back(audio.get_channel_count());
  pkeys->push_back("sample_rate");
  pvals->push_back(audio.get_sample_rate());
  pkeys->push_back("sample_size");
  pvals->push_back(audio.get_sample_size());
}

int IsobmffFileInformation::LiblcvmConfig_to_lists(
    std::shared_ptr<IsobmffFileInformation> pobj, LiblcvmKeyList* pkeys,
    LiblcvmValList* pvals, bool calculate_timestamps,
    LiblcvmKeyList* pkeys_timing, LiblcvmTimingList* pvals_timing, int debug,
    int64_t audio_track_id, bool include_track_keys, int64_t video_track_id) {
  // 0. reset outputs, including on a failed explicit selection.
  pkeys->clear();
  pvals->clear();
  pkeys_timing->clear();
  pvals_timing->clear();
  const AudioTrack* selected_audio = pobj->get_audio_track(audio_track_id);
  const VideoTrack* selected_video = pobj->get_video_track(video_track_id);
  AudioVideoTimingInformation selected_timing;
  try {
    selected_timing = pobj->get_timing(audio_track_id, video_track_id);
  } catch (const std::out_of_range& error) {
    fprintf(stderr, "error: %s\n", error.what());
    return -1;
  }

  // Preserve the existing flat key order for the selected/default view.
  fill_base_fields(pobj, selected_video, selected_timing, selected_audio, pkeys,
                   pvals, debug);

  if (include_track_keys) {
    // Fully qualified library keys remain stable regardless of cardinality.
    // Singleton elision belongs only to the CLI presentation layer.
    const size_t base_size = pkeys->size();
    const std::vector<std::string> file_keys = {
        "infile", "filesize", "bitrate_bps", "timescale_movie_hz"};
    const std::vector<std::string> audio_keys = {
        "audio_type",  "channel_count",      "sample_rate",
        "sample_size", "duration_audio_sec", "timescale_audio_hz"};
    const auto video_ids = pobj->get_video_track_ids();
    for (const auto id : video_ids) {
      LiblcvmKeyList video_keys;
      LiblcvmValList video_values;
      if (selected_video != nullptr && id == selected_video->get_track_id()) {
        // Reuse the original selected view rather than copy its frame arrays
        // again.
        video_keys.assign(pkeys->begin(),
                          pkeys->begin() + static_cast<ptrdiff_t>(base_size));
        video_values.assign(pvals->begin(),
                            pvals->begin() + static_cast<ptrdiff_t>(base_size));
      } else {
        const auto* video_info = pobj->get_video_track(id);
        auto video_timing = pobj->get_timing(audio_track_id, id);
        fill_base_fields(pobj, video_info, video_timing, selected_audio,
                         &video_keys, &video_values, debug);
      }
      const auto suffix = "_v" + std::to_string(id);
      for (size_t i = 0; i < video_keys.size(); ++i) {
        const auto& key = video_keys[i];
        if (std::find(file_keys.begin(), file_keys.end(), key) !=
                file_keys.end() ||
            std::find(audio_keys.begin(), audio_keys.end(), key) !=
                audio_keys.end() ||
            key == "audio_video_ratio" || key == "video_freeze")
          continue;
        pkeys->push_back(key + suffix);
        pvals->push_back(video_values[i]);
      }
    }
    const auto& tracks = pobj->get_audio_tracks();
    pkeys->push_back("audio_track_id");
    pvals->push_back(tracks.empty()
                         ? LiblcvmValue(-1)
                         : LiblcvmValue(selected_audio->get_track_id()));
    pkeys->push_back("video_track_id");
    pvals->push_back(video_ids.empty()
                         ? LiblcvmValue(-1)
                         : LiblcvmValue(selected_video->get_track_id()));
    for (const auto& audio : tracks) {
      const auto suffix = "_a" + std::to_string(audio.get_track_id());
      const LiblcvmValList values = {
          audio.get_audio_type(),          audio.get_channel_count(),
          audio.get_sample_rate(),         audio.get_sample_size(),
          audio.timing.get_duration_sec(), audio.timing.get_timescale_hz()};
      for (size_t i = 0; i < audio_keys.size(); ++i) {
        pkeys->push_back(audio_keys[i] + suffix);
        pvals->push_back(values[i]);
      }
      for (const auto video_id : video_ids) {
        const auto* video = pobj->get_video_track(video_id);
        const auto& pair_timing = video->get_timing();
        double ratio;
        bool freeze;
        derive_audio_video_metrics(pair_timing.get_duration_sec(),
                                   audio.timing.get_duration_sec(), ratio,
                                   freeze);
        const auto pair = "_v" + std::to_string(video_id) + suffix;
        pkeys->push_back("audio_video_ratio" + pair);
        pvals->push_back(ratio);
        pkeys->push_back("video_freeze" + pair);
        pvals->push_back(!video->is_supported() || !audio.is_supported()
                             ? -1
                             : (freeze ? 1 : 0));
      }
    }
  }

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
  pvals->push_back(version_str);  // Populated by policy parser
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
        selected_timing.get_frame_num_orig_list();
    std::vector<uint32_t> stts_unit_list = selected_timing.get_stts_unit_list();
    std::vector<int32_t> ctts_unit_list = selected_timing.get_ctts_unit_list();
    std::vector<double> dts_sec_list = selected_timing.get_dts_sec_list();
    std::vector<double> pts_sec_list = selected_timing.get_pts_sec_list();
    std::vector<double> pts_duration_sec_list =
        selected_timing.get_pts_duration_sec_list();
    std::vector<double> pts_duration_delta_sec_list =
        selected_timing.get_pts_duration_delta_sec_list();
    std::vector<double> pts_framerate_list =
        selected_timing.get_pts_framerate_list();
    // zip them
    size_t n = frame_num_orig_list.size();
    pvals_timing->reserve(n);
    for (size_t i = 0; i < n; ++i) {
      pvals_timing->emplace_back(
          frame_num_orig_list[i],
          (i < stts_unit_list.size()) ? stts_unit_list[i] : 0,
          // ctts may not exist
          (i < ctts_unit_list.size()) ? ctts_unit_list[i] : 0,
          (i < dts_sec_list.size()) ? dts_sec_list[i]
                                    : std::numeric_limits<double>::quiet_NaN(),
          (i < pts_sec_list.size()) ? pts_sec_list[i]
                                    : std::numeric_limits<double>::quiet_NaN(),
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
    const char* infile, const LiblcvmConfig& liblcvm_config) {
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

  // 3. look for a mvhd box
  std::shared_ptr<ISOBMFF::MVHD> mvhd =
      moov->GetTypedBox<ISOBMFF::MVHD>("mvhd");
  if (mvhd == nullptr) {
    if (liblcvm_config.get_debug() > 0) {
      fprintf(stderr, "error: no /moov/mvhd in %s\n", ptr->filename.c_str());
    }
    return nullptr;
  }
  uint32_t timescale_movie_hz = mvhd->GetTimescale();
  ptr->timescale_movie_hz = timescale_movie_hz;

  // 4. look for trak container boxes
  uint64_t total_video_frames = 0;
  for (auto& box : moov->GetBoxes()) {
    std::string name = box->GetName();
    if (name.compare("trak") != 0) {
      continue;
    }
    auto trak = std::dynamic_pointer_cast<ISOBMFF::ContainerBox>(box);

    // 5. look for a mdia container box
    std::shared_ptr<ISOBMFF::ContainerBox> mdia =
        trak->GetTypedBox<ISOBMFF::ContainerBox>("mdia");
    if (mdia == nullptr) {
      if (liblcvm_config.get_debug() > 0) {
        fprintf(stderr, "error: no /moov/trak/mdia in %s\n",
                ptr->filename.c_str());
      }
      return nullptr;
    }

    // 6. look for a hdlr box
    std::shared_ptr<ISOBMFF::HDLR> hdlr =
        mdia->GetTypedBox<ISOBMFF::HDLR>("hdlr");
    if (hdlr == nullptr) {
      if (liblcvm_config.get_debug() > 0) {
        fprintf(stderr, "error: no /moov/trak/mdia/hdlr in %s\n",
                ptr->filename.c_str());
      }
      return nullptr;
    }
    const auto handler_type = hdlr->GetHandlerType();
    if (handler_type != "vide" && handler_type != "soun")
      continue;
    if (handler_type == "soun" && liblcvm_config.get_skip_audio_parsing())
      continue;
    const auto mdhd = mdia->GetTypedBox<ISOBMFF::MDHD>("mdhd");
    const auto tkhd = trak->GetTypedBox<ISOBMFF::TKHD>("tkhd");
    const auto minf = mdia->GetTypedBox<ISOBMFF::ContainerBox>("minf");
    if (!mdhd || !tkhd || !minf)
      return nullptr;
    const auto stbl = minf->GetTypedBox<ISOBMFF::ContainerBox>("stbl");
    if (!stbl)
      return nullptr;
    const auto timescale_track_hz = mdhd->GetTimescale();
    const auto id = tkhd->GetTrackID();
    if (handler_type == "soun") {
      for (const auto& track : ptr->audio_tracks)
        if (track.get_track_id() == id)
          return nullptr;
      AudioTrack track;
      const int metadata_result = track.parse_sample_entry(
          stbl, ptr->filename, liblcvm_config.get_debug());
      if (metadata_result < 0)
        return nullptr;
      track.track_id = id;
      if (!track.is_supported()) {
        ptr->audio_tracks.push_back(std::move(track));
        continue;
      }

      const int timing_result = track.timing.parse_timing_information(
          stbl, timescale_track_hz, liblcvm_config.get_max_num_frames(), false,
          ptr->filename, liblcvm_config.get_debug());
      if (timing_result < 0)
        return nullptr;
      track.timing.timescale_movie_hz = timescale_movie_hz;
      track.timing.has_header_timing = true;
      track.timing.mdhd_duration_units = mdhd->GetDuration();
      track.timing.tkhd_duration_units = tkhd->GetDuration();
      if (track.timing.derive_timing_info(false, ptr->filename,
                                          liblcvm_config.get_debug()) < 0)
        return nullptr;
      ptr->audio_tracks.push_back(std::move(track));
      continue;
    }
    for (const auto& track : ptr->video_tracks)
      if (track.get_track_id() == id)
        return nullptr;
    VideoTrack track;
    const int metadata_result = track.parse_video_information(
        stbl, ptr->filename, liblcvm_config.get_debug());
    if (metadata_result < 0)
      return nullptr;
    track.track_id = id;
    if (!track.is_supported()) {
      ptr->video_tracks.push_back(std::move(track));
      continue;
    }
    track.width = tkhd->GetWidth();
    track.height = tkhd->GetHeight();

    const auto remaining = static_cast<uint32_t>(
        liblcvm_config.get_max_num_frames() - total_video_frames);
    const int timing_result = track.timing.parse_timing_information(
        stbl, timescale_track_hz, remaining, true, ptr->filename,
        liblcvm_config.get_debug());
    if (timing_result < 0)
      return nullptr;
    track.timing.timescale_movie_hz = timescale_movie_hz;
    track.timing.has_header_timing = true;
    track.timing.mdhd_duration_units = mdhd->GetDuration();
    track.timing.tkhd_duration_units = tkhd->GetDuration();
    if (track.timing.derive_timing_info(liblcvm_config.get_sort_by_pts(),
                                        ptr->filename,
                                        liblcvm_config.get_debug()) < 0 ||
        track.derive_video_info(ptr->filename, liblcvm_config.get_debug()) < 0)
      return nullptr;
    total_video_frames += static_cast<uint64_t>(track.timing.get_num_frames());
    ptr->video_tracks.push_back(std::move(track));
  }
  struct stat stat_buf;
  if (stat(ptr->filename.c_str(), &stat_buf) < 0)
    return nullptr;
  ptr->filesize = stat_buf.st_size;

  return ptr;
}

// Add two int32_t timestamps (units) with two's-complement wraparound.
// pts values are stored as int32_t, starting from the uint32_t dts, so a
// large dts plus a ctts offset can exceed the int32_t range. Signed
// overflow is undefined behavior, so add as uint32_t instead.
static int32_t add_int32_wraparound(int32_t a, int32_t b) {
  return static_cast<int32_t>(static_cast<uint32_t>(a) +
                              static_cast<uint32_t>(b));
}

int TimingInformation::parse_timing_information(
    std::shared_ptr<ISOBMFF::ContainerBox> stbl, uint32_t track_hz,
    uint32_t max_num_frames, bool want_per_frame_info,
    const std::string& filename, int debug) {
  if (!stbl)
    return -1;
  TimingInformation parsed;
  parsed.timescale_hz = track_hz;
  parsed.per_frame_info = want_per_frame_info;
  const auto stts = stbl->GetTypedBox<ISOBMFF::STTS>("stts");
  if (!stts && want_per_frame_info) {
    if (debug > 0)
      fprintf(stderr, "error: no stts timing table in %s\n", filename.c_str());
    return -1;
  }
  if (stts) {
    uint64_t count = 0;
    parsed.has_stts = true;
    for (size_t i = 0; i < stts->GetEntryCount(); ++i) {
      const auto samples = stts->GetSampleCount(i);
      if (want_per_frame_info &&
          (count + samples > max_num_frames ||
           count + samples > static_cast<uint64_t>(INT_MAX))) {
        if (debug > 0)
          fprintf(stderr, "error: stts frame budget exceeded in %s\n",
                  filename.c_str());
        return -1;
      }
      count += samples;
      parsed.stts_entries.push_back({samples, stts->GetSampleOffset(i)});
    }
  }
  const auto ctts = stbl->GetTypedBox<ISOBMFF::CTTS>("ctts");
  if (ctts) {
    parsed.has_ctts = true;
    for (size_t i = 0; i < ctts->GetEntryCount(); ++i)
      parsed.ctts_entries.push_back(
          {ctts->GetSampleCount(i), ctts->GetSampleOffset(i)});
  }
  if (want_per_frame_info) {
    const auto stss = stbl->GetTypedBox<ISOBMFF::STSS>("stss");
    if (stss) {
      for (size_t i = 0; i < stss->GetEntryCount(); ++i)
        parsed.raw_keyframe_sample_number_list.push_back(
            stss->GetSampleNumber(i));
    }
  }
  *this = std::move(parsed);
  return 0;
}

// Function derives an (N-1)-element vector from an N-element vector by
// setting element i-th as:
//   out[i] = in[i] - in[i-1]
//
// The function will allocate all the N-1 elements.
void calculate_vector_deltas_int32_t(const std::vector<int32_t> in,
                                     std::vector<int32_t>& out) {
  out.clear();
  // Use the index (not a -1 "no previous value" sentinel, which is also a
  // valid pts), and subtract with two's-complement wraparound, as the
  // int32_t pts values can be far apart (signed overflow is undefined
  // behavior). See add_int32_wraparound().
  for (size_t i = 1; i < in.size(); ++i) {
    out.push_back(static_cast<int32_t>(static_cast<uint32_t>(in[i]) -
                                       static_cast<uint32_t>(in[i - 1])));
  }
}

double calculate_median(const std::vector<double>& vec) {
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

double calculate_average(const std::vector<double>& vec) {
  return std::accumulate(vec.begin(), vec.end(), 0.0) /
         static_cast<double>(vec.size());
}

double calculate_standard_deviation(const std::vector<double>& vec) {
  if (vec.size() < 2) {
    fprintf(stderr,
            "error: calculate_standard_deviation needs at least 2 "
            "elements\n");
    return 0.0f;
  }

  double mean = calculate_average(vec);
  double sum_squares = 0.0f;
  for (const double& x : vec) {
    sum_squares += (x - mean) * (x - mean);
  }

  return std::sqrt(sum_squares / (vec.size() - 1));
}

// https://en.wikipedia.org/wiki/Median_absolute_deviation
double calculate_median_absolute_deviation(const std::vector<double>& vec) {
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

// Reorder a per-frame list so that list[i] becomes list[order[i]]. Returns
// -1 (and leaves the list as is) when the list does not have one entry per
// frame. An empty list (e.g. ctts_unit_list without a ctts box) is valid.
template <typename T>
static int reorder_by_index(std::vector<T>& list,
                            const std::vector<uint32_t>& order) {
  if (list.empty()) {
    return 0;
  }
  if (list.size() != order.size()) {
    return -1;
  }
  std::vector<T> list_alt(list.size());
  for (size_t i = 0; i < order.size(); ++i) {
    list_alt[i] = list[order[i]];
  }
  list = std::move(list_alt);
  return 0;
}

int TimingInformation::derive_timing_info(bool sort_by_pts,
                                          const std::string& filename,
                                          int debug) {
  TimingInformation derived;
  derived.timescale_movie_hz = timescale_movie_hz;
  derived.has_header_timing = has_header_timing;
  derived.mdhd_duration_units = mdhd_duration_units;
  derived.tkhd_duration_units = tkhd_duration_units;
  derived.timescale_hz = timescale_hz;
  derived.per_frame_info = per_frame_info;
  derived.has_stts = has_stts;
  derived.has_ctts = has_ctts;
  derived.stts_entries = stts_entries;
  derived.ctts_entries = ctts_entries;
  derived.raw_keyframe_sample_number_list = raw_keyframe_sample_number_list;
  for (const auto& entry : stts_entries) {
    const uint64_t ticks =
        static_cast<uint64_t>(entry.sample_count) * entry.sample_delta;
    if (UINT64_MAX - derived.num_samples < entry.sample_count ||
        UINT64_MAX - derived.stts_duration_units < ticks)
      return -1;
    derived.num_samples += entry.sample_count;
    derived.stts_duration_units += ticks;
  }
  if (has_stts)
    derived.stts_duration_sec =
        static_cast<double>(derived.stts_duration_units) / timescale_hz;
  derived.mdhd_duration_sec =
      has_header_timing
          ? static_cast<double>(mdhd_duration_units) / timescale_hz
          : -1.0;
  derived.duration_sec =
      has_header_timing
          ? (timescale_movie_hz > 0
                 ? static_cast<double>(tkhd_duration_units) / timescale_movie_hz
                 : derived.mdhd_duration_sec)
          : derived.stts_duration_sec;
  if (!per_frame_info) {
    *this = std::move(derived);
    return 0;
  }
  derived.num_frames = static_cast<int>(derived.num_samples);
  derived.dts_sec_list.push_back(0.0);
  derived.pts_unit_list.push_back(0);
  derived.pts_sec_list.push_back(0.0);
  uint32_t last_dts_unit = 0;
  for (const auto& entry : stts_entries) {
    for (uint32_t sample = 0; sample < entry.sample_count; ++sample) {
      derived.stts_unit_list.push_back(entry.sample_delta);
      last_dts_unit += entry.sample_delta;
      derived.dts_sec_list.push_back(static_cast<double>(last_dts_unit) /
                                     timescale_hz);
      derived.pts_unit_list.push_back(last_dts_unit);
      derived.pts_sec_list.push_back(static_cast<double>(last_dts_unit) /
                                     timescale_hz);
    }
  }
  derived.dts_sec_list.pop_back();
  derived.pts_unit_list.pop_back();
  derived.pts_sec_list.pop_back();
  if (has_ctts) {
    size_t current = 0;
    int32_t last_offset = 0;
    for (const auto& entry : ctts_entries) {
      last_offset = entry.sample_offset;
      for (uint32_t sample = 0; sample < entry.sample_count &&
                                current < derived.pts_unit_list.size();
           ++sample) {
        derived.ctts_unit_list.push_back(last_offset);
        derived.pts_unit_list[current] =
            add_int32_wraparound(derived.pts_unit_list[current], last_offset);
        derived.pts_sec_list[current] =
            static_cast<double>(derived.pts_unit_list[current]) / timescale_hz;
        ++current;
      }
    }
    while (current < derived.pts_unit_list.size()) {
      derived.ctts_unit_list.push_back(last_offset);
      derived.pts_unit_list[current] =
          add_int32_wraparound(derived.pts_unit_list[current], last_offset);
      derived.pts_sec_list[current] =
          static_cast<double>(derived.pts_unit_list[current]) / timescale_hz;
      ++current;
    }
  }
  derived.keyframe_sample_number_list = raw_keyframe_sample_number_list;
  *this = std::move(derived);
  // 1. set the frame_num_orig_list vector
  this->frame_num_orig_list.resize(this->pts_sec_list.size());
  for (uint32_t i = 0; i < this->pts_sec_list.size(); ++i) {
    this->frame_num_orig_list[i] = i;
  }

  // 2. sort the frames by pts value
  if (sort_by_pts) {
    // sort frame_num_orig_list elements based on the values in pts_sec_list
    // TODO(chema): there should be a clear way to access the struct element
    const auto& pts_sec_list = this->pts_sec_list;
    std::stable_sort(this->frame_num_orig_list.begin(),
                     this->frame_num_orig_list.end(),
                     [&pts_sec_list](int a, int b) {
                       return pts_sec_list[a] < pts_sec_list[b];
                     });
    // sort all the others based in the new order
    // All the per-frame lists must have one entry per frame (ctts_unit_list
    // may also be empty, when there is no ctts box): indexing a shorter list
    // with frame_num_orig_list would read past its end.
    if (reorder_by_index(this->stts_unit_list, this->frame_num_orig_list) < 0 ||
        reorder_by_index(this->ctts_unit_list, this->frame_num_orig_list) < 0 ||
        reorder_by_index(this->dts_sec_list, this->frame_num_orig_list) < 0 ||
        reorder_by_index(this->pts_unit_list, this->frame_num_orig_list) < 0 ||
        reorder_by_index(this->pts_sec_list, this->frame_num_orig_list) < 0) {
      if (debug > 0) {
        fprintf(stderr,
                "error: inconsistent per-frame timing list sizes in %s\n",
                filename.c_str());
      }
      return -1;
    }
  }

  // 3. derived timing values
  // 3.1. calculate the duration (inter-frame distance)
  std::vector<int32_t> pts_duration_unit_list;
  calculate_vector_deltas_int32_t(this->pts_unit_list, pts_duration_unit_list);
  this->pts_duration_sec_list.resize(pts_duration_unit_list.size());
  for (uint32_t i = 0; i < pts_duration_unit_list.size(); ++i) {
    this->pts_duration_sec_list[i] =
        ((double)pts_duration_unit_list[i]) / this->timescale_hz;
  }
  // 3.2. calculate the duration average/median
  this->pts_duration_sec_average =
      calculate_average(this->pts_duration_sec_list);
  this->pts_duration_sec_median = calculate_median(this->pts_duration_sec_list);
  // 3.3. calculate the duration stddev and median absolute difference (MAD)
  this->pts_duration_sec_stddev =
      calculate_standard_deviation(this->pts_duration_sec_list);
  this->pts_duration_sec_mad =
      calculate_median_absolute_deviation(this->pts_duration_sec_list);
  // 3.4. derive the pts_duration_delta_sec_list
  this->pts_duration_delta_sec_list.resize(this->pts_duration_sec_list.size());
  for (uint32_t i = 0; i < this->pts_duration_sec_list.size(); ++i) {
    this->pts_duration_delta_sec_list[i] =
        this->pts_duration_sec_list[i] - this->pts_duration_sec_average;
  }
  // 3.5 derive the pts_framerate_list (instantaneous framerate)
  this->pts_framerate_list.resize(this->pts_duration_sec_list.size());
  for (uint32_t i = 0; i < this->pts_duration_sec_list.size(); ++i) {
    this->pts_framerate_list[i] = (this->pts_duration_sec_list[i] == 0.0)
                                      ? std::nan("")
                                      : 1.0 / this->pts_duration_sec_list[i];
  }

  // 4. derive keyframe-related values
  this->num_keyframes = this->get_keyframe_sample_number_list().size();
  this->key_frame_ratio = (this->num_keyframes > 0)
                              ? (1.0 * this->num_frames) / this->num_keyframes
                              : 0.0;

  // 6. calculate framerate statistics
  // 6.1. get the framerate series
  this->frame_rate_fps_list.resize(this->pts_duration_sec_list.size());
  std::transform(this->pts_duration_sec_list.begin(),
                 this->pts_duration_sec_list.end(),
                 this->frame_rate_fps_list.begin(), [](double val) {
                   // Handle division by zero
                   return val != 0.0f ? 1.0f / static_cast<double>(val) : 0.0f;
                 });
  // 6.2. median
  this->frame_rate_fps_median = calculate_median(this->frame_rate_fps_list);
  // 6.3. average
  this->frame_rate_fps_average = calculate_average(this->frame_rate_fps_list);
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
  this->frame_rate_fps_reverse_average = 1.0 / this->pts_duration_sec_average;
  // 6.5. stddev
  this->frame_rate_fps_stddev =
      calculate_standard_deviation(this->frame_rate_fps_list);

  // 7. calculate the threshold to consider frame drop: This should be 2
  // times the median, minus a factor
  double FACTOR = 0.75;
  double pts_duration_sec_threshold =
      this->pts_duration_sec_median * FACTOR * 2;

  // 8. get the list of all the drops (absolute inter-frame values)
  for (const auto& pts_duration_sec : this->pts_duration_sec_list) {
    if (pts_duration_sec > pts_duration_sec_threshold) {
      this->frame_drop_length_sec_list.push_back(pts_duration_sec);
    }
  }
  // this->frame_drop_length_sec_list:
  // {0.6668900000000022, 0.10025600000000168,
  // ...}

  // 9. sum all the drops, but adding only the length over 1x frame time
  double frame_drop_length_sec_list = 0.0;
  for (const auto& drop_length_sec : this->frame_drop_length_sec_list) {
    frame_drop_length_sec_list += drop_length_sec;
  }
  double drop_length_duration_sec =
      frame_drop_length_sec_list -
      this->pts_duration_sec_median * this->frame_drop_length_sec_list.size();
  // drop_length_duration_sec: sum({33.35900000000022, 66.92600000000168, ...})

  // 10. get the total duration as the sum of all the inter-frame distances
  double total_duration_sec = 0.0;
  for (const auto& pts_duration_sec : this->pts_duration_sec_list) {
    total_duration_sec += pts_duration_sec;
  }

  // 11. calculate frame drop ratio as extra drop length over total duration
  this->frame_drop_ratio = drop_length_duration_sec / total_duration_sec;
  // The ratio is NaN or infinite when there is no usable inter-frame
  // duration (e.g. a single frame, all frames with the same pts, or a zero
  // timescale), and converting NaN, infinity, or an out-of-range value to
  // int is undefined behavior. Report 0 frame drops in that case.
  double frame_drop_count = this->frame_drop_ratio * this->num_frames;
  this->frame_drop_count =
      (std::isfinite(frame_drop_count) && frame_drop_count > INT_MIN &&
       frame_drop_count < INT_MAX)
          ? static_cast<int>(frame_drop_count)
          : 0;

  // 12. calculate average drop length, normalized to framerate. Note that
  // a single frame drop is a normalized frame drop length of 2. When
  // frame drops are uncorrelated, the normalized average drop length
  // should be close to 2
  this->normalized_frame_drop_average_length = 0.0;
  if (this->frame_drop_length_sec_list.size() > 0) {
    double frame_drop_average_length =
        frame_drop_length_sec_list / this->frame_drop_length_sec_list.size();
    this->normalized_frame_drop_average_length =
        (frame_drop_average_length / this->pts_duration_sec_median);
  }

  return 0;
}

void TimingInformation::invalidate_analysis() {
  *this = TimingInformation{};
  num_frames = num_keyframes = frame_drop_count = -1;
  const double invalid = std::numeric_limits<double>::quiet_NaN();
  pts_duration_sec_average = pts_duration_sec_median = pts_duration_sec_stddev =
      pts_duration_sec_mad = invalid;
  frame_rate_fps_median = frame_rate_fps_average =
      frame_rate_fps_reverse_average = frame_rate_fps_stddev = invalid;
  key_frame_ratio = frame_drop_ratio = normalized_frame_drop_average_length =
      invalid;
}

TimingInformation IsobmffFileInformation::empty_timing() { return {}; }

void TimingInformation::calculate_percentile_list(
    const std::vector<double> percentile_list,
    std::vector<double>& frame_drop_length_percentile_list, int debug) {
  // calculate percentile list
  frame_drop_length_percentile_list.clear();
  if (num_frames < 0) {
    frame_drop_length_percentile_list.assign(
        percentile_list.size(), std::numeric_limits<double>::quiet_NaN());
    return;
  }
  if (frame_drop_length_sec_list.size() > 0) {
    auto sorted_lengths = frame_drop_length_sec_list;
    std::sort(sorted_lengths.begin(), sorted_lengths.end());
    for (const double& percentile : percentile_list) {
      int position = (percentile / 100.0) * sorted_lengths.size();
      double frame_drop_length_percentile =
          sorted_lengths[position] / this->get_pts_duration_sec_median();
      frame_drop_length_percentile_list.push_back(frame_drop_length_percentile);
    }
  } else {
    frame_drop_length_percentile_list.resize(percentile_list.size(), 0.0);
  }
}

void TimingInformation::calculate_consecutive_list(
    std::vector<int> consecutive_list,
    std::vector<long int>& frame_drop_length_consecutive, int debug) {
  // calculate consecutive list
  frame_drop_length_consecutive.clear();
  if (num_frames < 0) {
    frame_drop_length_consecutive.assign(consecutive_list.size(), -1);
    return;
  }
  frame_drop_length_consecutive.resize(consecutive_list.size(), 0);
  if (frame_drop_length_sec_list.size() > 0) {
    for (const auto& frame_drop_length_sec : frame_drop_length_sec_list) {
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

int VideoTrack::derive_video_info(const std::string& filename, int debug) {
  struct stat stat_buf;
  if (stat(filename.c_str(), &stat_buf) < 0)
    return -1;
  filesize = stat_buf.st_size;
  bitrate_bps = 8.0 * static_cast<double>(filesize) / timing.get_duration_sec();
  return 0;
}

void VideoTrack::parse_avcc(std::shared_ptr<ISOBMFF::AVCC> avcc, int debug) {
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
  for (const auto& sps : avcc->GetSequenceParameterSetNALUnits()) {
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

void VideoTrack::parse_hvcc(std::shared_ptr<ISOBMFF::HVCC> hvcc, int debug) {
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
  for (const auto& array : hvcc->GetArrays()) {
    // bool array_completeness = array->GetArrayCompleteness();
    uint8_t nal_unit_type = array->GetNALUnitType();
    for (const auto& data : array->GetNALUnits()) {
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

int AudioTrack::parse_audio_information(
    std::shared_ptr<ISOBMFF::ContainerBox> stbl,
    std::shared_ptr<IsobmffFileInformation> ptr, int debug) {
  if (!ptr || !stbl)
    return -1;
  AudioTrack parsed;
  const int result = parsed.parse_sample_entry(stbl, ptr->filename, debug);
  if (result < 0)
    return -1;
  if (ptr->audio_tracks.empty()) {
    ptr->audio_tracks.push_back(std::move(parsed));
  } else {
    const auto& first = ptr->audio_tracks.front();
    parsed.track_id = first.track_id;
    if (parsed.is_supported())
      parsed.timing = first.timing;
    ptr->audio_tracks.front() = std::move(parsed);
  }
  return 0;
}

int AudioTrack::parse_sample_entry(std::shared_ptr<ISOBMFF::ContainerBox> stbl,
                                   const std::string& filename, int debug) {
  if (!stbl)
    return -1;
  auto saved_timing = std::move(timing);
  *this = AudioTrack();
  timing = std::move(saved_timing);
  auto stsd = stbl->GetTypedBox<ISOBMFF::STSD>("stsd");
  const auto generic = [&](const std::string& type) {
    audio_type = type;
    supported = false;
    channel_count = sample_rate = sample_size = -1;
    timing.invalidate_analysis();
    eac3_configuration.reset();
    return 0;
  };
  if (!stsd || stsd->GetBoxes().empty())
    return generic("none");
  for (const auto& box : stsd->GetBoxes()) {
    const auto name = box->GetName();
    if ((name == "mp4a" && !std::dynamic_pointer_cast<ISOBMFF::MP4A>(box)) ||
        (name == "Opus" && !std::dynamic_pointer_cast<ISOBMFF::OPUS>(box)))
      return -1;
    if (name == "ec-3") {
      const auto entry = std::dynamic_pointer_cast<ISOBMFF::EC3>(box);
      if (!entry)
        return -1;
      const auto config = entry->GetTypedBox<ISOBMFF::DEC3>("dec3");
      if (!config || config->GetIndependentSubstreams().empty())
        return -1;
    }
  }
  auto mp4a = stsd->GetTypedBox<ISOBMFF::MP4A>("mp4a");
  if (mp4a) {
    supported = true;
    audio_type = "mp4a";
    channel_count = mp4a->GetChannelCount();
    sample_size = mp4a->GetSampleSize();
    sample_rate = static_cast<int>(mp4a->GetSampleRateRaw() >> 16);
    return 0;
  }
  auto opus = stsd->GetTypedBox<ISOBMFF::OPUS>("Opus");
  if (opus) {
    supported = true;
    audio_type = "Opus";
    channel_count = opus->GetChannelCount();
    sample_size = opus->GetSampleSize();
    sample_rate = static_cast<int>(opus->GetSampleRateRaw() >> 16);
    return 0;
  }
  auto ec3 = stsd->GetTypedBox<ISOBMFF::EC3>("ec-3");
  if (ec3) {
    auto dec3 = ec3->GetTypedBox<ISOBMFF::DEC3>("dec3");
    if (!dec3 || dec3->GetIndependentSubstreams().empty())
      return -1;
    supported = true;
    audio_type = "ec-3";
    eac3_configuration = dec3;
    // Annex F.5.2 requires ignoring sample-entry ChannelCount and SampleSize.
    // Describe I0, including its unique dependent channels, not a sum of
    // services.
    const auto service = dec3->GetIndependentSubstreams().front();
    channel_count = service.GetChannelCount();
    uint32_t rate = service.GetSampleRate();
    // fscod2 is bitstream-only when fscod == 3: use the declared entry rate
    // if available; otherwise report unknown rather than inventing a rate.
    if (rate == 0)
      rate = ec3->GetSampleRateRaw() >> 16;
    sample_rate = rate == 0 ? -1 : static_cast<int>(rate);
    sample_size = -1;  // No compressed PCM precision is declared by dec3.
    return 0;
  }
  return generic(stsd->GetBoxes().front()->GetName());
}

int VideoTrack::parse_video_information(
    std::shared_ptr<ISOBMFF::ContainerBox> stbl, const std::string& filename,
    int debug) {
  if (!stbl)
    return -1;
  // 1. look for a stsd container box
  std::shared_ptr<ISOBMFF::STSD> stsd =
      stbl->GetTypedBox<ISOBMFF::STSD>("stsd");
  const auto generic = [&](const std::string& type) {
    const auto id = track_id;
    *this = VideoTrack{};
    timing.invalidate_analysis();
    track_id = id;
    video_codec_type = type;
    supported = false;
    filesize = width2 = height2 = horizresolution = vertresolution = depth =
        chroma_format = bit_depth_luma = bit_depth_chroma =
            video_full_range_flag = colour_primaries =
                transfer_characteristics = matrix_coeffs = profile_idc =
                    level_idc = -1;
    width = height = bitrate_bps = -1.0;
    return 0;
  };
  if (!stsd || stsd->GetBoxes().empty())
    return generic("none");
  // 2. look for hvc1/hev1 or avc1/avc3 (container) boxes
  for (const auto& box : stsd->GetBoxes()) {
    const auto name = box->GetName();
    const bool hevc = name == "hvc1" || name == "hev1";
    const bool avc = name == "avc1" || name == "avc3";
    if (!hevc && !avc)
      continue;
    if ((name == "hvc1" && !std::dynamic_pointer_cast<ISOBMFF::HVC1>(box)) ||
        (name == "hev1" && !std::dynamic_pointer_cast<ISOBMFF::HEV1>(box)) ||
        (name == "avc1" && !std::dynamic_pointer_cast<ISOBMFF::AVC1>(box)) ||
        (name == "avc3" && !std::dynamic_pointer_cast<ISOBMFF::AVC3>(box)))
      return -1;
    if (hevc) {
      const auto entry = std::dynamic_pointer_cast<ISOBMFF::HVC1>(box);
      if (!entry || !entry->GetTypedBox<ISOBMFF::HVCC>("hvcC"))
        return -1;
    } else {
      const auto entry = std::dynamic_pointer_cast<ISOBMFF::AVC1>(box);
      if (!entry || !entry->GetTypedBox<ISOBMFF::AVCC>("avcC"))
        return -1;
    }
  }
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
                (hvc1 != nullptr) ? "hvc1" : "hev1", filename.c_str());
      }
      return -1;
    }
    this->video_codec_type = (hvc1 != nullptr) ? "hvc1" : "hev1";
    this->width2 = hevc_box->GetWidth();
    this->height2 = hevc_box->GetHeight();
    this->horizresolution = hevc_box->GetHorizResolution();
    this->vertresolution = hevc_box->GetVertResolution();
    this->depth = hevc_box->GetDepth();
    this->chroma_format = hvcc->GetChromaFormat();
    this->bit_depth_luma = 8 + hvcc->GetBitDepthLumaMinus8();
    this->bit_depth_chroma = 8 + hvcc->GetBitDepthChromaMinus8();
    this->colour_primaries = -1;
    this->transfer_characteristics = -1;
    this->matrix_coeffs = -1;
    this->video_full_range_flag = -1;
    this->parse_hvcc(hvcc, debug);

  } else if (avc1 != nullptr || avc3 != nullptr) {
    auto avc_box = (avc1 != nullptr) ? avc1 : avc3;
    // 4. look for an avcC box
    std::shared_ptr<ISOBMFF::AVCC> avcc =
        avc_box->GetTypedBox<ISOBMFF::AVCC>("avcC");
    if (avcc == nullptr) {
      if (debug > 0) {
        fprintf(stderr,
                "error: no /moov/trak/mdia/minf/stbl/stsd/%s/avcC in %s\n",
                (avc1 != nullptr) ? "avc1" : "avc3", filename.c_str());
      }
      return -1;
    }
    this->video_codec_type = (avc1 != nullptr) ? "avc1" : "avc3";
    this->width2 = avc_box->GetWidth();
    this->height2 = avc_box->GetHeight();
    this->horizresolution = avc_box->GetHorizResolution();
    this->vertresolution = avc_box->GetVertResolution();
    this->depth = avc_box->GetDepth();
    this->chroma_format = -1;
    this->bit_depth_luma = -1;
    this->bit_depth_chroma = -1;
    this->parse_avcc(avcc, debug);

  } else {
    return generic(stsd->GetBoxes().front()->GetName());
  }
  supported = true;
  return 0;
}
