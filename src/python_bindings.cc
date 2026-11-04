#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "liblcvm.h"

#if ADD_POLICY
#include <list>
#endif

namespace py = pybind11;

// Define the macro to bind frame getters
#define VIDEO_GETTERS(class_name)                                     \
  .def("get_track_id", &class_name::get_track_id)                     \
      .def("get_filesize", &class_name::get_filesize)                 \
      .def("get_bitrate_bps", &class_name::get_bitrate_bps)           \
      .def("get_width", &class_name::get_width)                       \
      .def("get_height", &class_name::get_height)                     \
      .def("get_video_codec_type", &class_name::get_video_codec_type) \
      .def("get_horizresolution", &class_name::get_horizresolution)   \
      .def("get_vertresolution", &class_name::get_vertresolution)     \
      .def("get_depth", &class_name::get_depth)                       \
      .def("get_chroma_format", &class_name::get_chroma_format)       \
      .def("get_bit_depth_luma", &class_name::get_bit_depth_luma)     \
      .def("get_bit_depth_chroma", &class_name::get_bit_depth_chroma) \
      .def("get_video_full_range_flag",                               \
           &class_name::get_video_full_range_flag)                    \
      .def("get_colour_primaries", &class_name::get_colour_primaries) \
      .def("get_transfer_characteristics",                            \
           &class_name::get_transfer_characteristics)                 \
      .def("get_matrix_coeffs", &class_name::get_matrix_coeffs)

// Define the macro to bind timing getters
#define TIMING_GETTERS(class_name)                                            \
  .def("get_timescale_movie_hz", &class_name::get_timescale_movie_hz)         \
      .def("get_pts_duration_sec_average",                                    \
           &class_name::get_pts_duration_sec_average)                         \
      .def("get_pts_duration_sec_median",                                     \
           &class_name::get_pts_duration_sec_median)                          \
      .def("get_pts_duration_sec_stddev",                                     \
           &class_name::get_pts_duration_sec_stddev)                          \
      .def("get_pts_duration_sec_mad", &class_name::get_pts_duration_sec_mad) \
      .def("get_num_frames", &class_name::get_num_frames)                     \
      .def("get_frame_rate_fps_median",                                       \
           &class_name::get_frame_rate_fps_median)                            \
      .def("get_frame_rate_fps_average",                                      \
           &class_name::get_frame_rate_fps_average)                           \
      .def("get_frame_rate_fps_reverse_average",                              \
           &class_name::get_frame_rate_fps_reverse_average)                   \
      .def("get_frame_rate_fps_stddev",                                       \
           &class_name::get_frame_rate_fps_stddev)                            \
      .def("get_frame_drop_count", &class_name::get_frame_drop_count)         \
      .def("get_frame_drop_ratio", &class_name::get_frame_drop_ratio)         \
      .def("get_frame_drop_length_sec_list",                                  \
           &class_name::get_frame_drop_length_sec_list)                       \
      .def("get_normalized_frame_drop_average_length",                        \
           &class_name::get_normalized_frame_drop_average_length)             \
      .def(                                                                   \
          "calculate_percentile_list",                                        \
          [](class_name& self, const std::vector<double>& percentile_list,    \
             int debug) {                                                     \
            std::vector<double> frame_drop_length_percentile_list;            \
            self.calculate_percentile_list(                                   \
                percentile_list, frame_drop_length_percentile_list, debug);   \
            return frame_drop_length_percentile_list;                         \
          },                                                                  \
          py::arg("percentile_list"), py::arg("debug"))                       \
      .def(                                                                   \
          "calculate_consecutive_list",                                       \
          [](class_name& self, const std::vector<int>& consecutive_list,      \
             int debug) {                                                     \
            std::vector<long int> frame_drop_length_consecutive;              \
            self.calculate_consecutive_list(                                  \
                consecutive_list, frame_drop_length_consecutive, debug);      \
            return frame_drop_length_consecutive;                             \
          },                                                                  \
          py::arg("consecutive_list"), py::arg("debug"))                      \
      .def("get_num_keyframes", &class_name::get_num_keyframes)               \
      .def("get_key_frame_ratio", &class_name::get_key_frame_ratio)           \
      .def("get_frame_num_orig_list", &class_name::get_frame_num_orig_list)   \
      .def("get_stts_unit_list", &class_name::get_stts_unit_list)             \
      .def("get_ctts_unit_list", &class_name::get_ctts_unit_list)             \
      .def("get_dts_sec_list", &class_name::get_dts_sec_list)                 \
      .def("get_pts_sec_list", &class_name::get_pts_sec_list)                 \
      .def("get_pts_duration_sec_list",                                       \
           &class_name::get_pts_duration_sec_list)                            \
      .def("get_pts_duration_delta_sec_list",                                 \
           &class_name::get_pts_duration_delta_sec_list)                      \
      .def("get_pts_framerate_list", &class_name::get_pts_framerate_list)

// Define the macro to bind audio getters
#define AUDIO_GETTERS(class_name)                                       \
  .def("get_audio_type", &class_name::get_audio_type)                   \
      .def("get_channel_count", &class_name::get_channel_count)         \
      .def("get_sample_rate", &class_name::get_sample_rate)             \
      .def("get_sample_size", &class_name::get_sample_size)             \
      .def("get_track_id", &class_name::get_track_id)                   \
      .def("get_duration_sec", &class_name::get_duration_sec)           \
      .def("get_mdhd_duration_sec", &class_name::get_mdhd_duration_sec) \
      .def("get_timescale_hz", &class_name::get_timescale_hz)

PYBIND11_MODULE(liblcvm, m) {
  m.doc() = "Pybind11 interface for liblcvm shared library";

  // Expose the IsobmffFileInformation class
  py::class_<IsobmffFileInformation, std::shared_ptr<IsobmffFileInformation>>(
      m, "IsobmffFileInformation")
      .def(py::init<>())
      // Expose all getter methods
      .def("get_filename", &IsobmffFileInformation::get_filename)
      .def("get_filesize", &IsobmffFileInformation::get_filesize)
      .def("get_timing", &IsobmffFileInformation::get_timing,
           py::arg("audio_track_id") = -1, py::arg("video_track_id") = -1)
      .def("get_video_track", &IsobmffFileInformation::get_video_track,
           py::arg("video_track_id") = -1,
           py::return_value_policy::reference_internal)
      .def("get_audio_track", &IsobmffFileInformation::get_audio_track,
           py::arg("audio_track_id") = -1,
           py::return_value_policy::reference_internal)
      .def("get_audio_tracks", &IsobmffFileInformation::get_audio_tracks)
      .def("get_video_tracks", &IsobmffFileInformation::get_video_tracks)
      .def("get_video_track_ids", &IsobmffFileInformation::get_video_track_ids)
      .def("get_audio_track_ids", &IsobmffFileInformation::get_audio_track_ids)
      .def("get_num_video_tracks",
           &IsobmffFileInformation::get_num_video_tracks)
      .def("get_num_audio_tracks",
           &IsobmffFileInformation::get_num_audio_tracks);

  // Expose the parse method as a standalone function
  m.def("parse", &IsobmffFileInformation::parse, py::arg("infile"),
        py::arg("liblcvm_config"),
        "Parse an ISOBMFF file and return file information.");

  // Expose the VideoTrack class
  py::class_<VideoTrack>(m, "VideoTrack")
      .def(py::init<>())
      // Automatically bind all VideoTrack class getters
      VIDEO_GETTERS(VideoTrack)
      .def("get_timing", &VideoTrack::get_timing,
           py::return_value_policy::reference_internal);

  py::class_<TimingInformation::SampleTimingEntry>(m, "SampleTimingEntry")
      .def_readonly("sample_count",
                    &TimingInformation::SampleTimingEntry::sample_count)
      .def_readonly("sample_delta",
                    &TimingInformation::SampleTimingEntry::sample_delta);
  py::class_<TimingInformation::CompositionTimingEntry>(
      m, "CompositionTimingEntry")
      .def_readonly("sample_count",
                    &TimingInformation::CompositionTimingEntry::sample_count)
      .def_readonly("sample_offset",
                    &TimingInformation::CompositionTimingEntry::sample_offset);

  // Expose the TimingInformation class
  py::class_<TimingInformation>(m, "TimingInformation")
      .def(py::init<>())
      .def("get_stts_entries", &TimingInformation::get_stts_entries)
      .def("get_ctts_entries", &TimingInformation::get_ctts_entries)
      .def("get_per_frame_info", &TimingInformation::get_per_frame_info)
      .def("get_has_stts", &TimingInformation::get_has_stts)
      .def("get_num_samples", &TimingInformation::get_num_samples)
      .def("get_timescale_hz", &TimingInformation::get_timescale_hz)
      .def("get_stts_duration_units",
           &TimingInformation::get_stts_duration_units)
      .def("get_stts_duration_sec", &TimingInformation::get_stts_duration_sec)
      .def("get_mdhd_duration_sec", &TimingInformation::get_mdhd_duration_sec)
      .def("get_duration_sec", &TimingInformation::get_duration_sec)
      .def("get_mdhd_duration_units",
           &TimingInformation::get_mdhd_duration_units)
      .def("get_tkhd_duration_units",
           &TimingInformation::get_tkhd_duration_units)
      // Automatically bind all TimingInformation class getters
      TIMING_GETTERS(TimingInformation);

  py::class_<AudioVideoTimingInformation, TimingInformation>(
      m, "AudioVideoTimingInformation")
      .def("get_duration_video_sec",
           &AudioVideoTimingInformation::get_duration_video_sec)
      .def("get_duration_audio_sec",
           &AudioVideoTimingInformation::get_duration_audio_sec)
      .def("get_timescale_video_hz",
           &AudioVideoTimingInformation::get_timescale_video_hz)
      .def("get_timescale_audio_hz",
           &AudioVideoTimingInformation::get_timescale_audio_hz)
      .def("get_num_video_frames",
           &AudioVideoTimingInformation::get_num_video_frames)
      .def("get_num_video_keyframes",
           &AudioVideoTimingInformation::get_num_video_keyframes)
      .def("get_audio_video_ratio",
           &AudioVideoTimingInformation::get_audio_video_ratio)
      .def("get_video_freeze", &AudioVideoTimingInformation::get_video_freeze);

  // Expose the AudioTrack class
  py::class_<AudioTrack>(m, "AudioTrack")
      .def(py::init<>())
      // Automatically bind all AudioTrack class getters
      AUDIO_GETTERS(AudioTrack)
      .def("get_timing", &AudioTrack::get_timing,
           py::return_value_policy::reference_internal);

  // Expose the LiblcvmConfig class
  py::class_<LiblcvmConfig>(m, "LiblcvmConfig").def(py::init<>());

#if ADD_POLICY
  // Expose the policy_runner function
  m.def(
      "policy_runner",
      [](const std::string& policy_str, const std::vector<std::string>& keys,
         const std::vector<py::object>& vals) {
        // Convert Python objects to LiblcvmValue
        LiblcvmKeyList cpp_keys = keys;
        LiblcvmValList cpp_vals;

        for (const auto& val : vals) {
          if (py::isinstance<py::int_>(val)) {
            cpp_vals.push_back(val.cast<int>());
          } else if (py::isinstance<py::float_>(val)) {
            cpp_vals.push_back(val.cast<double>());
          } else if (py::isinstance<py::str>(val)) {
            cpp_vals.push_back(val.cast<std::string>());
          } else {
            throw std::runtime_error("Unsupported value type in policy_runner");
          }
        }

        std::list<std::string> warn_list;
        std::list<std::string> error_list;
        std::string version;

        int result = policy_runner(policy_str, &cpp_keys, &cpp_vals, &warn_list,
                                   &error_list, &version);

        // Convert std::list to std::vector for Python
        std::vector<std::string> warn_vec(warn_list.begin(), warn_list.end());
        std::vector<std::string> error_vec(error_list.begin(),
                                           error_list.end());

        return py::make_tuple(result, warn_vec, error_vec, version);
      },
      py::arg("policy_str"), py::arg("keys"), py::arg("vals"));
#endif
}
