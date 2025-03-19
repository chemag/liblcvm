#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "liblcvm.h"

namespace py = pybind11;

// Define the macro to bind frame getters
#define FRAME_GETTERS(class_name) \
    .def("get_filesize", &class_name::get_filesize) \
    .def("get_bitrate_bps", &class_name::get_bitrate_bps) \
    .def("get_width", &class_name::get_width) \
    .def("get_height", &class_name::get_height) \
    .def("get_type", &class_name::get_type) \
    .def("get_horizresolution", &class_name::get_horizresolution) \
    .def("get_vertresolution", &class_name::get_vertresolution) \
    .def("get_depth", &class_name::get_depth) \
    .def("get_chroma_format", &class_name::get_chroma_format) \
    .def("get_bit_depth_luma", &class_name::get_bit_depth_luma) \
    .def("get_bit_depth_chroma", &class_name::get_bit_depth_chroma) \
    .def("get_video_full_range_flag", &class_name::get_video_full_range_flag) \
    .def("get_colour_primaries", &class_name::get_colour_primaries) \
    .def("get_transfer_characteristics", &class_name::get_transfer_characteristics) \
    .def("get_matrix_coeffs", &class_name::get_matrix_coeffs)


// Define the macro to bind timing getters
#define TIMING_GETTERS(class_name) \
    .def("get_video_freeze", &class_name::get_video_freeze) \
    .def("get_audio_video_ratio", &class_name::get_audio_video_ratio) \
    .def("get_duration_video_sec", &class_name::get_duration_video_sec) \
    .def("get_duration_audio_sec", &class_name::get_duration_audio_sec) \
    .def("get_timescale_video_hz", &class_name::get_timescale_video_hz) \
    .def("get_timescale_audio_hz", &class_name::get_timescale_audio_hz) \
    .def("get_pts_duration_sec_average", &class_name::get_pts_duration_sec_average) \
    .def("get_pts_duration_sec_median", &class_name::get_pts_duration_sec_median) \
    .def("get_pts_duration_sec_stddev", &class_name::get_pts_duration_sec_stddev) \
    .def("get_pts_duration_sec_mad", &class_name::get_pts_duration_sec_mad) \
    .def("get_num_video_frames", &class_name::get_num_video_frames) \
    .def("get_frame_rate_fps_median", &class_name::get_frame_rate_fps_median) \
    .def("get_frame_rate_fps_average", &class_name::get_frame_rate_fps_average) \
    .def("get_frame_rate_fps_reverse_average", &class_name::get_frame_rate_fps_reverse_average) \
    .def("get_frame_rate_fps_stddev", &class_name::get_frame_rate_fps_stddev) \
    .def("get_frame_drop_count", &class_name::get_frame_drop_count) \
    .def("get_frame_drop_ratio", &class_name::get_frame_drop_ratio) \
    .def("get_normalized_frame_drop_average_length", &class_name::get_normalized_frame_drop_average_length) \
    .def("calculate_percentile_list", [](class_name& self, const std::vector<float>& percentile_list, int debug) { \
        std::vector<float> frame_drop_length_percentile_list; \
        self.calculate_percentile_list(percentile_list, frame_drop_length_percentile_list, debug); \
        return frame_drop_length_percentile_list; \
    }, py::arg("percentile_list"), py::arg("debug")) \
    .def("calculate_consecutive_list", [](class_name& self, const std::vector<int>& consecutive_list, int debug) { \
        std::vector<long int> frame_drop_length_consecutive; \
        self.calculate_consecutive_list(consecutive_list, frame_drop_length_consecutive, debug); \
        return frame_drop_length_consecutive; \
    }, py::arg("consecutive_list"), py::arg("debug")) \
    .def("get_num_video_keyframes", &class_name::get_num_video_keyframes) \
    .def("get_key_frame_ratio", &class_name::get_key_frame_ratio) \
    .def("get_frame_num_orig_list", &class_name::get_frame_num_orig_list) \
    .def("get_stts_unit_list", &class_name::get_stts_unit_list) \
    .def("get_ctts_unit_list", &class_name::get_ctts_unit_list) \
    .def("get_dts_sec_list", &class_name::get_dts_sec_list) \
    .def("get_pts_sec_list", &class_name::get_pts_sec_list) \
    .def("get_pts_duration_sec_list", &class_name::get_pts_duration_sec_list)


// Define the macro to bind audio getters
#define AUDIO_GETTERS(class_name) \
    .def("get_audio_type", &class_name::get_audio_type) \
    .def("get_channel_count", &class_name::get_channel_count) \
    .def("get_sample_rate", &class_name::get_sample_rate) \
    .def("get_sample_size", &class_name::get_sample_size)


PYBIND11_MODULE(liblcvm, m) {
    m.doc() = "Pybind11 interface for liblcvm shared library";

    // Expose the IsobmffFileInformation class
    py::class_<IsobmffFileInformation, std::shared_ptr<IsobmffFileInformation>>(m, "IsobmffFileInformation")
        .def(py::init<>())
        // Expose all getter methods
        .def("get_filename", &IsobmffFileInformation::get_filename)
        .def("get_timing", &IsobmffFileInformation::get_timing)
        .def("get_frame", &IsobmffFileInformation::get_frame)
        .def("get_audio", &IsobmffFileInformation::get_audio);

    // Expose the parse method as a standalone function
    m.def("parse",
          &IsobmffFileInformation::parse,
          py::arg("infile"),
          py::arg("liblcvm_config"),
          "Parse an ISOBMFF file and return file information.");

    // Expose the FrameInformation class
    py::class_<FrameInformation>(m, "FrameInformation")
        .def(py::init<>())
        // Automatically bind all FrameInformation class getters
        FRAME_GETTERS(FrameInformation);

    // Expose the TimingInformation class
    py::class_<TimingInformation>(m, "TimingInformation")
        .def(py::init<>())
        // Automatically bind all TimingInformation class getters
        TIMING_GETTERS(TimingInformation);

    // Expose the AudioInformation class
    py::class_<AudioInformation>(m, "AudioInformation")
        .def(py::init<>())
        // Automatically bind all AudioInformation class getters
        AUDIO_GETTERS(AudioInformation);

    // Expose the LiblcvmConfig class
    py::class_<LiblcvmConfig>(m, "LiblcvmConfig")
        .def(py::init<>());
}
