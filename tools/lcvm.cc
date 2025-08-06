// lcvm: frame dups and freezes detector

// A show case of using liblcvm to detect frame dups and
// video freezes in ISOBMFF files.

#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>  // for optarg

#include <climits>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>  // for basic_string, string
#include <vector>

#include "liblcvm.h"
#include "policy_protovisitor.h"


extern int optind;

/* option values */
typedef struct arg_options {
  int debug;
  int nruns;
  char *outfile;
  char *outfile_timestamps;
  bool outfile_timestamps_sort_pts;
  std::vector<std::string> infile_list;
  char *policy_file;
} arg_options;

/* default option values */
arg_options DEFAULT_OPTIONS{
    .debug = 0,
    .nruns = 1,
    .outfile = nullptr,
    .outfile_timestamps = nullptr,
    .outfile_timestamps_sort_pts = true,
    .infile_list = {},
    .policy_file = nullptr,
};

std::string join_list(const std::list<std::string>& lst, const char* sep = ";") {
    std::ostringstream oss;
    bool first = true;
    for (const auto& s : lst) {
        if (!first) oss << sep;
        oss << s;
        first = false;
    }
    return oss.str();
}

int policy_runner(const std::string& policy_str,
                  std::shared_ptr<std::map<std::string, LiblcvmValue>> pmap,
                  std::list<std::string>* warn_list,
                  std::list<std::string>* error_list);

int parse_files_with_policy(
    std::vector<std::string> &infile_list,
    char *outfile,
    char *outfile_timestamps,
    bool outfile_timestamps_sort_pts,
    int debug,
    const std::string& policy_str)
{
    // 0. open outfile
    FILE *outfp;
    if (outfile == nullptr || (strlen(outfile) == 1 && outfile[0] == '-')) {
        outfp = stdout;
    } else {
        outfp = fopen(outfile, "wb");
        if (outfp == nullptr) {
            fprintf(stderr, "Could not open output file: \"%s\"\n", outfile);
            return -1;
        }
    }

    // 1. write CSV header (with warn_list and error_list)
    fprintf(outfp,
        "infile,filesize,bitrate_bps,width,height,type,"
        "horizresolution,vertresolution,"
        "depth,chroma_format,bit_depth_luma,bit_depth_chroma,"
        "video_full_range_flag,"
        "colour_primaries,"
        "transfer_characteristics,"
        "matrix_coeffs,"
        "num_video_frames,frame_rate_fps_median,frame_rate_fps_average,"
        "frame_rate_fps_reverse_average,frame_rate_fps_stddev,video_freeze,"
        "audio_video_ratio,duration_video_sec,duration_audio_sec,"
        "timescale_video_hz,timescale_audio_hz,"
        "pts_duration_sec_average,pts_duration_sec_median,"
        "pts_duration_sec_stddev,pts_duration_sec_mad,"
        "frame_drop_count,frame_drop_ratio,"
        "normalized_frame_drop_average_length,"
        "frame_drop_length_percentile_50,frame_drop_length_percentile_90,"
        "frame_drop_length_consecutive_2,frame_drop_length_consecutive_5,"
        "num_video_keyframes,key_frame_ratio,"
        "audio_type,channel_count,sample_rate,sample_size,"
        "warn_list,error_list\n");

    // 2. write CSV rows
    std::map<std::string, std::vector<uint32_t>> frame_num_orig_list_dict;
    std::map<std::string, std::vector<uint32_t>> stts_unit_list_dict;
    std::map<std::string, std::vector<int32_t>> ctts_unit_list_dict;
    std::map<std::string, std::vector<float>> dts_sec_list_dict;
    std::map<std::string, std::vector<float>> pts_sec_list_dict;
    std::map<std::string, std::vector<float>> pts_duration_sec_list_dict;
    std::map<std::string, std::vector<float>> pts_duration_delta_sec_list_dict;

    auto liblcvm_config = std::make_unique<LiblcvmConfig>();
    liblcvm_config->set_sort_by_pts(outfile_timestamps_sort_pts);
    liblcvm_config->set_debug(debug);

    for (const auto &infile : infile_list) {
        std::shared_ptr<IsobmffFileInformation> ptr =
            IsobmffFileInformation::parse(infile.c_str(), *liblcvm_config);
        if (ptr == nullptr) {
            fprintf(stderr, "error: IsobmffFileInformation::parse() in %s\n",
                    infile.c_str());
            continue;
        }

        // --- Extract fields as before ---
        auto filesize = ptr->get_frame().get_filesize();
        auto bitrate_bps = ptr->get_frame().get_bitrate_bps();
        auto width = ptr->get_frame().get_width();
        auto height = ptr->get_frame().get_height();
        auto type = ptr->get_frame().get_type();
        auto horizresolution = ptr->get_frame().get_horizresolution();
        auto vertresolution = ptr->get_frame().get_vertresolution();
        auto depth = ptr->get_frame().get_depth();
        auto chroma_format = ptr->get_frame().get_chroma_format();
        auto bit_depth_luma = ptr->get_frame().get_bit_depth_luma();
        auto bit_depth_chroma = ptr->get_frame().get_bit_depth_chroma();
        auto video_full_range_flag = ptr->get_frame().get_video_full_range_flag();
        auto colour_primaries = ptr->get_frame().get_colour_primaries();
        auto transfer_characteristics = ptr->get_frame().get_transfer_characteristics();
        auto matrix_coeffs = ptr->get_frame().get_matrix_coeffs();

        auto video_freeze = ptr->get_timing().get_video_freeze();
        auto audio_video_ratio = ptr->get_timing().get_audio_video_ratio();
        auto duration_video_sec = ptr->get_timing().get_duration_video_sec();
        auto duration_audio_sec = ptr->get_timing().get_duration_audio_sec();
        auto timescale_video_hz = ptr->get_timing().get_timescale_video_hz();
        auto timescale_audio_hz = ptr->get_timing().get_timescale_audio_hz();
        auto pts_duration_sec_average = ptr->get_timing().get_pts_duration_sec_average();
        auto pts_duration_sec_median = ptr->get_timing().get_pts_duration_sec_median();
        auto pts_duration_sec_stddev = ptr->get_timing().get_pts_duration_sec_stddev();
        auto pts_duration_sec_mad = ptr->get_timing().get_pts_duration_sec_mad();
        auto num_video_frames = ptr->get_timing().get_num_video_frames();
        auto frame_rate_fps_median = ptr->get_timing().get_frame_rate_fps_median();
        auto frame_rate_fps_average = ptr->get_timing().get_frame_rate_fps_average();
        auto frame_rate_fps_reverse_average = ptr->get_timing().get_frame_rate_fps_reverse_average();
        auto frame_rate_fps_stddev = ptr->get_timing().get_frame_rate_fps_stddev();
        auto frame_drop_count = ptr->get_timing().get_frame_drop_count();
        auto frame_drop_ratio = ptr->get_timing().get_frame_drop_ratio();
        auto normalized_frame_drop_average_length = ptr->get_timing().get_normalized_frame_drop_average_length();

        std::vector<float> percentile_list = {50, 90};
        std::vector<float> frame_drop_length_percentile_list;
        ptr->get_timing().calculate_percentile_list(
            percentile_list, frame_drop_length_percentile_list, debug);

        std::vector<int> consecutive_list = {2, 5};
        std::vector<long int> frame_drop_length_consecutive;
        ptr->get_timing().calculate_consecutive_list(
            consecutive_list, frame_drop_length_consecutive, debug);

        auto num_video_keyframes = ptr->get_timing().get_num_video_keyframes();
        auto key_frame_ratio = ptr->get_timing().get_key_frame_ratio();
        auto audio_type = ptr->get_audio().get_audio_type();
        auto channel_count = ptr->get_audio().get_channel_count();
        auto sample_rate = ptr->get_audio().get_sample_rate();
        auto sample_size = ptr->get_audio().get_sample_size();

        // --- Policy evaluation ---
        std::list<std::string> warn_list, error_list;
        auto pmap = std::make_shared<std::map<std::string, LiblcvmValue>>();
        (*pmap)["bitrate_bps"] = bitrate_bps;
        (*pmap)["width"] = width;
        (*pmap)["height"] = height;
        (*pmap)["type"] = type;
        (*pmap)["bit_depth_luma"] = bit_depth_luma;
        (*pmap)["audio_video_ratio"] = audio_video_ratio;
        (*pmap)["depth"] = depth;
        // Add more fields as needed for your policy

        policy_runner(policy_str, pmap, &warn_list, &error_list);

        std::string warn_str = join_list(warn_list);
        std::string error_str = join_list(error_list);

        // --- Write CSV row ---
        fprintf(outfp, "%s,%i,%f,%f,%f,%s,%u,%u,%u,%i,%i,%i,%i,%i,%i,%i,%i,%f,%f,%f,%f,%i,%f,%f,%f,%u,%u,%f,%f,%f,%f,%i,%f,%f,%f,%f,%ld,%li,%d,%f,%s,%i,%d,%d,\"%s\",\"%s\"\n",
            infile.c_str(), filesize, bitrate_bps, width, height, type.c_str(),
            horizresolution, vertresolution, depth, chroma_format, bit_depth_luma, bit_depth_chroma,
            video_full_range_flag, colour_primaries, transfer_characteristics, matrix_coeffs,
            num_video_frames, frame_rate_fps_median, frame_rate_fps_average, frame_rate_fps_reverse_average,
            frame_rate_fps_stddev, video_freeze ? 1 : 0, audio_video_ratio, duration_video_sec, duration_audio_sec,
            timescale_video_hz, timescale_audio_hz, pts_duration_sec_average, pts_duration_sec_median,
            pts_duration_sec_stddev, pts_duration_sec_mad, frame_drop_count, frame_drop_ratio,
            normalized_frame_drop_average_length, frame_drop_length_percentile_list[0], frame_drop_length_percentile_list[1],
            frame_drop_length_consecutive[0], frame_drop_length_consecutive[1], num_video_keyframes, key_frame_ratio,
            audio_type.c_str(), channel_count, sample_rate, sample_size,
            warn_str.c_str(), error_str.c_str());

        // --- Optionally print warnings/errors to stdout ---
        for (const auto& w : warn_list) {
            printf("WARN [%s]: %s\n", infile.c_str(), w.c_str());
        }
        for (const auto& e : error_list) {
            printf("ERROR [%s]: %s\n", infile.c_str(), e.c_str());
        }

        // --- Timestamp collection (unchanged) ---
        if (outfile_timestamps != nullptr) {
            frame_num_orig_list_dict[infile] = ptr->get_timing().get_frame_num_orig_list();
            stts_unit_list_dict[infile] = ptr->get_timing().get_stts_unit_list();
            ctts_unit_list_dict[infile] = ptr->get_timing().get_ctts_unit_list();
            dts_sec_list_dict[infile] = ptr->get_timing().get_dts_sec_list();
            pts_sec_list_dict[infile] = ptr->get_timing().get_pts_sec_list();
            pts_duration_sec_list_dict[infile] = ptr->get_timing().get_pts_duration_sec_list();
            pts_duration_delta_sec_list_dict[infile] = ptr->get_timing().get_pts_duration_delta_sec_list();
        }
    }

    // 3. dump outfile timestamps (unchanged)
    if (outfile_timestamps != nullptr) {
      // 3.1. get the number of frames of the longest file
      size_t max_number_of_frames = 0;
      for (const auto &entry : frame_num_orig_list_dict) {
        const std::vector<uint32_t> &frame_num_orig_list = entry.second;
        max_number_of_frames =
            std::max(max_number_of_frames, frame_num_orig_list.size());
      }
      // 3.2. open outfile_timestamps
      FILE *outtsfp = fopen(outfile_timestamps, "wb");
      if (outtsfp == nullptr) {
        // did not work
        fprintf(stderr, "Could not open output file: \"%s\"\n",
                outfile_timestamps);
        return -1;
      }
      // 3.3. dump the file names
      fprintf(outtsfp, "frame_num");
      long unsigned infile_list_size = infile_list.size();
      for (const auto &entry : stts_unit_list_dict) {
        const std::string &infile = entry.first;
        fprintf(outtsfp, ",frame_num_orig");
        if (infile_list_size > 1) {
          fprintf(outtsfp, "_%s", infile.c_str());
        }
      }
      for (const auto &entry : stts_unit_list_dict) {
        const std::string &infile = entry.first;
        fprintf(outtsfp, ",stts");
        if (infile_list_size > 1) {
          fprintf(outtsfp, "_%s", infile.c_str());
        }
      }
      for (const auto &entry : stts_unit_list_dict) {
        const std::string &infile = entry.first;
        fprintf(outtsfp, ",ctts");
        if (infile_list_size > 1) {
          fprintf(outtsfp, "_%s", infile.c_str());
        }
      }
      for (const auto &entry : stts_unit_list_dict) {
        const std::string &infile = entry.first;
        fprintf(outtsfp, ",dts");
        if (infile_list_size > 1) {
          fprintf(outtsfp, "_%s", infile.c_str());
        }
      }
      for (const auto &entry : stts_unit_list_dict) {
        const std::string &infile = entry.first;
        fprintf(outtsfp, ",pts");
        if (infile_list_size > 1) {
          fprintf(outtsfp, "_%s", infile.c_str());
        }
      }
      for (const auto &entry : stts_unit_list_dict) {
        const std::string &infile = entry.first;
        fprintf(outtsfp, ",pts_duration");
        if (infile_list_size > 1) {
          fprintf(outtsfp, "_%s", infile.c_str());
        }
      }
      for (const auto &entry : stts_unit_list_dict) {
        const std::string &infile = entry.first;
        fprintf(outtsfp, ",pts_duration_delta");
        if (infile_list_size > 1) {
          fprintf(outtsfp, "_%s", infile.c_str());
        }
      }
      fprintf(outtsfp, "\n");
      // 3.4. dump the columns of inter-frame timestamps
      for (size_t frame_num = 0; frame_num < max_number_of_frames; ++frame_num) {
        fprintf(outtsfp, "%li", frame_num);
        // get frame_num_orig_list[frame_num]
        for (const auto &entry : frame_num_orig_list_dict) {
          const std::vector<uint32_t> &frame_num_orig_list = entry.second;
          if (frame_num < frame_num_orig_list.size()) {
            uint32_t frame_num_orig = frame_num_orig_list[frame_num];
            fprintf(outtsfp, ",%u", frame_num_orig);
          } else {
            fprintf(outtsfp, ",");
          }
        }
        // dump stts_unit_list[frame_num]
        for (const auto &entry : stts_unit_list_dict) {
          const std::vector<uint32_t> &stts_unit_list = entry.second;
          if (frame_num < stts_unit_list.size()) {
            uint32_t stts_unit = stts_unit_list[frame_num];
            fprintf(outtsfp, ",%u", stts_unit);
          } else {
            fprintf(outtsfp, ",");
          }
        }
        // dump ctts_sec_list[frame_num]
        for (const auto &entry : ctts_unit_list_dict) {
          const std::vector<int32_t> &ctts_unit_list = entry.second;
          if (frame_num < ctts_unit_list.size()) {
            int32_t ctts_unit = ctts_unit_list[frame_num];
            fprintf(outtsfp, ",%i", ctts_unit);
          } else {
            fprintf(outtsfp, ",");
          }
        }
        // dump dts_sec_list[frame_num]
        for (const auto &entry : dts_sec_list_dict) {
          const std::vector<float> &dts_sec_list = entry.second;
          if (frame_num < dts_sec_list.size()) {
            float dts = dts_sec_list[frame_num];
            fprintf(outtsfp, ",%f", dts);
          } else {
            fprintf(outtsfp, ",");
          }
        }
        // dump pts_sec_list[frame_num]
        for (const auto &entry : pts_sec_list_dict) {
          const std::vector<float> &pts_sec_list = entry.second;
          if (frame_num < pts_sec_list.size()) {
            float pts = pts_sec_list[frame_num];
            fprintf(outtsfp, ",%f", pts);
          } else {
            fprintf(outtsfp, ",");
          }
        }
        // dump pts_duration_sec_list[frame_num]
        for (const auto &entry : pts_duration_sec_list_dict) {
          const std::vector<float> &pts_duration_sec_list = entry.second;
          if (frame_num < pts_duration_sec_list.size()) {
            float pts_duration = pts_duration_sec_list[frame_num];
            fprintf(outtsfp, ",%f", pts_duration);
          } else {
            fprintf(outtsfp, ",");
          }
        }
        // dump pts_duration_delta_sec_list[frame_num]
        for (const auto &entry : pts_duration_delta_sec_list_dict) {
          const std::vector<float> &pts_duration_delta_sec_list = entry.second;
          if (frame_num < pts_duration_delta_sec_list.size()) {
            float pts_duration = pts_duration_delta_sec_list[frame_num];
            fprintf(outtsfp, ",%f", pts_duration);
          } else {
            fprintf(outtsfp, ",");
          }
        }
        fprintf(outtsfp, "\n");
      }
    }
    if (outfp && outfp != stdout) fclose(outfp);
    return 0;
}

int parse_files(std::vector<std::string> &infile_list, char *outfile,
                char *outfile_timestamps, bool outfile_timestamps_sort_pts,
                int debug, const std::string& policy_str) {
    // 0. open outfile
    FILE *outfp;
    if (outfile == nullptr || (strlen(outfile) == 1 && outfile[0] == '-')) {
        outfp = stdout;
    } else {
        outfp = fopen(outfile, "wb");
        if (outfp == nullptr) {
            fprintf(stderr, "Could not open output file: \"%s\"\n", outfile);
            return -1;
        }
    }

    // 1. write CSV header
    const std::vector<std::string> csv_keys = {
        "infile", "filesize", "bitrate_bps", "width", "height", "type",
        "horizresolution", "vertresolution", "depth", "chroma_format",
        "bit_depth_luma", "bit_depth_chroma", "video_full_range_flag",
        "colour_primaries", "transfer_characteristics", "matrix_coeffs",
        "num_video_frames", "frame_rate_fps_median", "frame_rate_fps_average",
        "frame_rate_fps_reverse_average", "frame_rate_fps_stddev", "video_freeze",
        "audio_video_ratio", "duration_video_sec", "duration_audio_sec",
        "timescale_video_hz", "timescale_audio_hz", "pts_duration_sec_average",
        "pts_duration_sec_median", "pts_duration_sec_stddev", "pts_duration_sec_mad",
        "frame_drop_count", "frame_drop_ratio", "normalized_frame_drop_average_length",
        "frame_drop_length_percentile_50", "frame_drop_length_percentile_90",
        "frame_drop_length_consecutive_2", "frame_drop_length_consecutive_5",
        "num_video_keyframes", "key_frame_ratio", "audio_type", "channel_count",
        "sample_rate", "sample_size", "warn_list", "error_list"
    };

    for (size_t i = 0; i < csv_keys.size(); ++i) {
        fprintf(outfp, "%s%s", csv_keys[i].c_str(), (i + 1 < csv_keys.size()) ? "," : "\n");
    }

    // 2. write CSV rows
    std::map<std::string, std::vector<uint32_t>> frame_num_orig_list_dict;
    std::map<std::string, std::vector<uint32_t>> stts_unit_list_dict;
    std::map<std::string, std::vector<int32_t>> ctts_unit_list_dict;
    std::map<std::string, std::vector<float>> dts_sec_list_dict;
    std::map<std::string, std::vector<float>> pts_sec_list_dict;
    std::map<std::string, std::vector<float>> pts_duration_sec_list_dict;
    std::map<std::string, std::vector<float>> pts_duration_delta_sec_list_dict;

    auto liblcvm_config = std::make_unique<LiblcvmConfig>();
    liblcvm_config->set_sort_by_pts(outfile_timestamps_sort_pts);
    liblcvm_config->set_debug(debug);

    for (const auto &infile : infile_list) {
        std::shared_ptr<std::map<std::string, LiblcvmValue>> pmap =
        IsobmffFileInformation::parse_to_map(infile.c_str(), *liblcvm_config, policy_str);
        if (!pmap) {
            fprintf(stderr, "error: IsobmffFileInformation::parse_to_map() in %s\n", infile.c_str());
            continue;
        }
        // Write CSV row in the correct order
        for (size_t i = 0; i < csv_keys.size(); ++i) {
            const std::string &key = csv_keys[i];
            auto it = pmap->find(key);
            if (it == pmap->end()) {
                fprintf(outfp, "%s", (i + 1 < csv_keys.size()) ? "," : "\n");
                continue;
            }
            std::string value = to_csv_string(it->second);
            fprintf(outfp, "%s%s", csv_escape(value).c_str(), (i + 1 < csv_keys.size()) ? "," : "\n");
        }

        // 2.4. capture outfile timestamps
        if (outfile_timestamps != nullptr) {
            std::shared_ptr<IsobmffFileInformation> ptr =
                IsobmffFileInformation::parse(infile.c_str(), *liblcvm_config);
            if (ptr != nullptr) {
                frame_num_orig_list_dict[infile] = ptr->get_timing().get_frame_num_orig_list();
                stts_unit_list_dict[infile] = ptr->get_timing().get_stts_unit_list();
                ctts_unit_list_dict[infile] = ptr->get_timing().get_ctts_unit_list();
                dts_sec_list_dict[infile] = ptr->get_timing().get_dts_sec_list();
                pts_sec_list_dict[infile] = ptr->get_timing().get_pts_sec_list();
                pts_duration_sec_list_dict[infile] = ptr->get_timing().get_pts_duration_sec_list();
                pts_duration_delta_sec_list_dict[infile] = ptr->get_timing().get_pts_duration_delta_sec_list();
            }
        }
    }

    // 3. dump outfile timestamps
    if (outfile_timestamps != nullptr) {
        size_t max_number_of_frames = 0;
        for (const auto &entry : frame_num_orig_list_dict) {
            const std::vector<uint32_t> &frame_num_orig_list = entry.second;
            max_number_of_frames = std::max(max_number_of_frames, frame_num_orig_list.size());
        }
        FILE *outtsfp = fopen(outfile_timestamps, "wb");
        if (outtsfp == nullptr) {
            fprintf(stderr, "Could not open output file: \"%s\"\n", outfile_timestamps);
            if (outfp != stdout) fclose(outfp);
            return -1;
        }
        fprintf(outtsfp, "frame_num");
        long unsigned infile_list_size = infile_list.size();
        for (const auto &entry : stts_unit_list_dict) {
            const std::string &infile = entry.first;
            fprintf(outtsfp, ",frame_num_orig");
            if (infile_list_size > 1) fprintf(outtsfp, "_%s", infile.c_str());
        }
        for (const auto &entry : stts_unit_list_dict) {
            const std::string &infile = entry.first;
            fprintf(outtsfp, ",stts");
            if (infile_list_size > 1) fprintf(outtsfp, "_%s", infile.c_str());
        }
        for (const auto &entry : stts_unit_list_dict) {
            const std::string &infile = entry.first;
            fprintf(outtsfp, ",ctts");
            if (infile_list_size > 1) fprintf(outtsfp, "_%s", infile.c_str());
        }
        for (const auto &entry : stts_unit_list_dict) {
            const std::string &infile = entry.first;
            fprintf(outtsfp, ",dts");
            if (infile_list_size > 1) fprintf(outtsfp, "_%s", infile.c_str());
        }
        for (const auto &entry : stts_unit_list_dict) {
            const std::string &infile = entry.first;
            fprintf(outtsfp, ",pts");
            if (infile_list_size > 1) fprintf(outtsfp, "_%s", infile.c_str());
        }
        for (const auto &entry : stts_unit_list_dict) {
            const std::string &infile = entry.first;
            fprintf(outtsfp, ",pts_duration");
            if (infile_list_size > 1) fprintf(outtsfp, "_%s", infile.c_str());
        }
        for (const auto &entry : stts_unit_list_dict) {
            const std::string &infile = entry.first;
            fprintf(outtsfp, ",pts_duration_delta");
            if (infile_list_size > 1) fprintf(outtsfp, "_%s", infile.c_str());
        }
        fprintf(outtsfp, "\n");
        for (size_t frame_num = 0; frame_num < max_number_of_frames; ++frame_num) {
            fprintf(outtsfp, "%zu", frame_num);
            for (const auto &entry : frame_num_orig_list_dict) {
                const std::vector<uint32_t> &frame_num_orig_list = entry.second;
                if (frame_num < frame_num_orig_list.size()) {
                    fprintf(outtsfp, ",%u", frame_num_orig_list[frame_num]);
                } else {
                    fprintf(outtsfp, ",");
                }
            }
            for (const auto &entry : stts_unit_list_dict) {
                const std::vector<uint32_t> &stts_unit_list = entry.second;
                if (frame_num < stts_unit_list.size()) {
                    fprintf(outtsfp, ",%u", stts_unit_list[frame_num]);
                } else {
                    fprintf(outtsfp, ",");
                }
            }
            for (const auto &entry : ctts_unit_list_dict) {
                const std::vector<int32_t> &ctts_unit_list = entry.second;
                if (frame_num < ctts_unit_list.size()) {
                    fprintf(outtsfp, ",%i", ctts_unit_list[frame_num]);
                } else {
                    fprintf(outtsfp, ",");
                }
            }
            for (const auto &entry : dts_sec_list_dict) {
                const std::vector<float> &dts_sec_list = entry.second;
                if (frame_num < dts_sec_list.size()) {
                    fprintf(outtsfp, ",%f", dts_sec_list[frame_num]);
                } else {
                    fprintf(outtsfp, ",");
                }
            }
            for (const auto &entry : pts_sec_list_dict) {
                const std::vector<float> &pts_sec_list = entry.second;
                if (frame_num < pts_sec_list.size()) {
                    fprintf(outtsfp, ",%f", pts_sec_list[frame_num]);
                } else {
                    fprintf(outtsfp, ",");
                }
            }
            for (const auto &entry : pts_duration_sec_list_dict) {
                const std::vector<float> &pts_duration_sec_list = entry.second;
                if (frame_num < pts_duration_sec_list.size()) {
                    fprintf(outtsfp, ",%f", pts_duration_sec_list[frame_num]);
                } else {
                    fprintf(outtsfp, ",");
                }
            }
            for (const auto &entry : pts_duration_delta_sec_list_dict) {
                const std::vector<float> &pts_duration_delta_sec_list = entry.second;
                if (frame_num < pts_duration_delta_sec_list.size()) {
                    fprintf(outtsfp, ",%f", pts_duration_delta_sec_list[frame_num]);
                } else {
                    fprintf(outtsfp, ",");
                }
            }
            fprintf(outtsfp, "\n");
        }
        fclose(outtsfp);
    }
    if (outfp != stdout) fclose(outfp);
    return 0;
}

void usage(char *name) {
  fprintf(stderr, "usage: %s [options] <infile(s)>\n", name);
  fprintf(stderr, "where options are:\n");
  fprintf(stderr, "\t-d:\t\tIncrease debug verbosity [%i]\n",
          DEFAULT_OPTIONS.debug);
  fprintf(stderr, "\t-q:\t\tZero debug verbosity\n");
  fprintf(stderr, "\t--runs <nruns>:\t\tRun the analysis multiple times [%i]\n",
          DEFAULT_OPTIONS.nruns);
  fprintf(stderr, "\t-p policy file:\t\tSpecify policy file to be parsed\n");
  fprintf(stderr, "\t--policy policy file:\t\tSpecify policy file to be parsed\n");
  fprintf(stderr, "\t-o outfile:\t\tSelect outfile\n");
  fprintf(stderr,
          "\t--outfile-timestamps outfile_timestamps:\t\tSelect outfile to "
          "dump timestamps\n");
  fprintf(stderr, "\t--sort-pts:\t\tSort outfile timestamps by PTS\n");
  fprintf(stderr, "\t--no-sort-pts:\t\tDo not outfile timestamps by PTS\n");
  fprintf(stderr, "\t-h:\t\tHelp\n");
  exit(-1);
}

// long options with no equivalent short option
enum {
  QUIET_OPTION = CHAR_MAX + 1,
  HELP_OPTION,
  OUTFILE_TIMESTAMPS_OPTION,
  SORT_PTS_OPTION,
  NO_SORT_PTS_OPTION,
  RUNS_OPTION,
  VERSION_OPTION,
};

arg_options *parse_args(int argc, char **argv) {
  int c;
  static arg_options options;

  // set default option values
  options = DEFAULT_OPTIONS;

  // getopt_long stores the option index here
  int optindex = 0;

  // long options
  static struct option longopts[] = {
      // matching options to short options
      {"debug", no_argument, nullptr, 'd'},
      {"outfile", required_argument, nullptr, 'o'},
      {"policy", required_argument, nullptr, 'p'},
      {"outfile-timestamps", required_argument, nullptr,
       OUTFILE_TIMESTAMPS_OPTION},
      {"sort-pts", no_argument, nullptr, SORT_PTS_OPTION},
      {"no-sort-pts", no_argument, nullptr, NO_SORT_PTS_OPTION},
      // options without a short option
      {"runs", required_argument, nullptr, RUNS_OPTION},
      {"quiet", no_argument, nullptr, QUIET_OPTION},
      {"version", no_argument, NULL, VERSION_OPTION},
      {"help", no_argument, nullptr, HELP_OPTION},
      {nullptr, 0, nullptr, 0}};

  // parse arguments
  while (true) {
    c = getopt_long(argc, argv, "do:hp:", longopts, &optindex);
    if (c == -1) {
      break;
    }
    switch (c) {
      case 0:
        // long options that define flag
        printf("option %s", longopts[optindex].name);
        if (optarg) {
          printf(" with arg %s", optarg);
        }
        break;

      case 'd':
        options.debug += 1;
        break;

      case 'o':
        options.outfile = optarg;
        break;

      case 'p':
        options.policy_file = optarg;
      break;

      case OUTFILE_TIMESTAMPS_OPTION:
        options.outfile_timestamps = optarg;
        break;

      case SORT_PTS_OPTION:
        options.outfile_timestamps_sort_pts = true;
        break;

      case NO_SORT_PTS_OPTION:
        options.outfile_timestamps_sort_pts = false;
        break;

      case QUIET_OPTION:
        options.debug = 0;
        break;

      case RUNS_OPTION: {
        char *endptr;
        options.nruns = strtol(optarg, &endptr, 0);
        if (*endptr != '\0') {
          fprintf(stderr, "error: invalid --runs parameter: %s\n", optarg);
          exit(-1);
        }
      } break;

      case HELP_OPTION:
      case 'h':
        usage(argv[0]);
        break;

      case VERSION_OPTION: {
        std::string version;
        IsobmffFileInformation::get_liblcvm_version(version);
        fprintf(stdout, "version: %s\n", version.c_str());
        exit(0);
      } break;

      default:
        printf("Unsupported option: %c\n", c);
        usage(argv[0]);
    }
  }

  // any extra parameters are infiles
  options.infile_list.clear();
  if (argc > optind) {
    for (int i = optind; i < argc; ++i) {
      options.infile_list.push_back(argv[i]);
    }
  }

  return &options;
}

int main(int argc, char **argv) {
  arg_options *options = parse_args(argc, argv);
  if (!options) {
    usage(argv[0]);
    exit(-1);
  }

  std::string policy_str;
  bool have_policy = false;
  if (options->policy_file) {
    FILE *pf = fopen(options->policy_file, "r");
    if (!pf) {
      fprintf(stderr, "Could not open policy file: %s\n", options->policy_file);
      exit(-1);
    }
    fseek(pf, 0, SEEK_END);
    long fsize = ftell(pf);
    fseek(pf, 0, SEEK_SET);
    policy_str.resize(fsize);
    fread(&policy_str[0], 1, fsize, pf);
    fclose(pf);
    have_policy = true;
    if (options->debug > 0) {
      printf("Read policy file (%ld bytes)\n", fsize);
    }
  }

  for (int i = 0; i < options->nruns; ++i) {
    if (have_policy) {
        parse_files_with_policy(options->infile_list, options->outfile,
                          options->outfile_timestamps,
                          options->outfile_timestamps_sort_pts,
                          options->debug, policy_str);
    } else {
        parse_files(options->infile_list, options->outfile,
                    options->outfile_timestamps,
                    options->outfile_timestamps_sort_pts,
                    options->debug);
    }
  }
    return 0;
}
