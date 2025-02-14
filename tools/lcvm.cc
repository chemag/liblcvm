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

extern int optind;

/* option values */
typedef struct arg_options {
  int debug;
  int nruns;
  char *outfile;
  char *outfile_timestamps;
  bool outfile_timestamps_sort_pts;
  std::vector<std::string> infile_list;
} arg_options;

/* default option values */
arg_options DEFAULT_OPTIONS{
    .debug = 0,
    .nruns = 1,
    .outfile = nullptr,
    .outfile_timestamps = nullptr,
    .outfile_timestamps_sort_pts = true,
    .infile_list = {},
};

int parse_files(std::vector<std::string> &infile_list, char *outfile,
                char *outfile_timestamps, bool outfile_timestamps_sort_pts,
                int debug) {
  // 0. open outfile
  FILE *outfp;
  if (outfile == nullptr || (strlen(outfile) == 1 && outfile[0] == '-')) {
    // use stdout
    outfp = stdout;
  } else {
    outfp = fopen(outfile, "wb");
    if (outfp == nullptr) {
      // did not work
      fprintf(stderr, "Could not open output file: \"%s\"\n", outfile);
      return -1;
    }
  }

  // 1. write CSV header
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
          "audio_type,channel_count,sample_rate,sample_size\n");

  // 2. write CSV rows
  std::map<std::string, std::vector<uint32_t>> frame_num_orig_list_dict;
  std::map<std::string, std::vector<uint32_t>> stts_unit_list_dict;
  std::map<std::string, std::vector<int32_t>> ctts_unit_list_dict;
  std::map<std::string, std::vector<float>> dts_sec_list_dict;
  std::map<std::string, std::vector<float>> pts_sec_list_dict;
  std::map<std::string, std::vector<float>> pts_duration_sec_list_dict;
  for (const auto &infile : infile_list) {
    // 2.1. analyze file
    auto *liblcvm_config = new LiblcvmConfig();
    liblcvm_config->set_sort_by_pts(outfile_timestamps_sort_pts);
    liblcvm_config->set_debug(debug);
    std::shared_ptr<IsobmffFileInformation> ptr =
        IsobmffFileInformation::parse(infile.c_str(), *liblcvm_config);
    if (ptr == nullptr) {
      fprintf(stderr, "error: IsobmffFileInformation::parse() in %s\n",
              infile.c_str());
      continue;
    }

    // 2.2 get information
    int filesize = ptr->get_frame().get_filesize();
    float bitrate_bps = ptr->get_frame().get_bitrate_bps();
    int width = ptr->get_frame().get_width();
    int height = ptr->get_frame().get_height();
    std::string type = ptr->get_frame().get_type();
    // int width2 = ptr->get_frame().get_width2();
    // int height2 = ptr->get_frame().get_height2();
    int horizresolution = ptr->get_frame().get_horizresolution();
    int vertresolution = ptr->get_frame().get_vertresolution();
    int depth = ptr->get_frame().get_depth();
    int chroma_format = ptr->get_frame().get_chroma_format();
    int bit_depth_luma = ptr->get_frame().get_bit_depth_luma();
    int bit_depth_chroma = ptr->get_frame().get_bit_depth_chroma();
    int video_full_range_flag = ptr->get_frame().get_video_full_range_flag();
    int colour_primaries = ptr->get_frame().get_colour_primaries();
    int transfer_characteristics =
        ptr->get_frame().get_transfer_characteristics();
    int matrix_coeffs = ptr->get_frame().get_matrix_coeffs();

    bool video_freeze = ptr->get_timing().get_video_freeze();
    float audio_video_ratio = ptr->get_timing().get_audio_video_ratio();
    float duration_video_sec = ptr->get_timing().get_duration_video_sec();
    float duration_audio_sec = ptr->get_timing().get_duration_audio_sec();
    uint32_t timescale_video_hz = ptr->get_timing().get_timescale_video_hz();
    uint32_t timescale_audio_hz = ptr->get_timing().get_timescale_audio_hz();
    float pts_duration_sec_average =
        ptr->get_timing().get_pts_duration_sec_average();
    float pts_duration_sec_median =
        ptr->get_timing().get_pts_duration_sec_median();
    float pts_duration_sec_stddev =
        ptr->get_timing().get_pts_duration_sec_stddev();
    float pts_duration_sec_mad = ptr->get_timing().get_pts_duration_sec_mad();
    int num_video_frames = ptr->get_timing().get_num_video_frames();
    float frame_rate_fps_median = ptr->get_timing().get_frame_rate_fps_median();
    float frame_rate_fps_average =
        ptr->get_timing().get_frame_rate_fps_average();
    float frame_rate_fps_reverse_average =
        ptr->get_timing().get_frame_rate_fps_reverse_average();
    float frame_rate_fps_stddev = ptr->get_timing().get_frame_rate_fps_stddev();
    int frame_drop_count = ptr->get_timing().get_frame_drop_count();
    float frame_drop_ratio = ptr->get_timing().get_frame_drop_ratio();
    float normalized_frame_drop_average_length =
        ptr->get_timing().get_normalized_frame_drop_average_length();

    std::vector<float> percentile_list = {50, 90};
    std::vector<float> frame_drop_length_percentile_list;
    ptr->get_timing().calculate_percentile_list(
        percentile_list, frame_drop_length_percentile_list, debug);

    std::vector<int> consecutive_list = {2, 5};
    std::vector<long int> frame_drop_length_consecutive;
    ptr->get_timing().calculate_consecutive_list(
        consecutive_list, frame_drop_length_consecutive, debug);

    // 2.3. get video structure info
    int num_video_keyframes = ptr->get_timing().get_num_video_keyframes();
    float key_frame_ratio = ptr->get_timing().get_key_frame_ratio();

    // 2.3.1 get audio structure info
    std::string audio_type = ptr->get_audio().get_audio_type();
    int channel_count = ptr->get_audio().get_channel_count();
    int sample_rate = ptr->get_audio().get_sample_rate();
    int sample_size = ptr->get_audio().get_sample_size();

    // 2.4. dump all output
    fprintf(outfp, "%s", infile.c_str());
    fprintf(outfp, ",%i", filesize);
    fprintf(outfp, ",%f", bitrate_bps);
    fprintf(outfp, ",%i", width);
    fprintf(outfp, ",%i", height);
    fprintf(outfp, ",%s", type.c_str());
    // fprintf(outfp, ",%i", width2);
    // fprintf(outfp, ",%i", height2);
    fprintf(outfp, ",%u", horizresolution);
    fprintf(outfp, ",%u", vertresolution);
    fprintf(outfp, ",%u", depth);
    fprintf(outfp, ",%u", chroma_format);
    fprintf(outfp, ",%u", bit_depth_luma);
    fprintf(outfp, ",%u", bit_depth_chroma);
    fprintf(outfp, ",%i", video_full_range_flag);
    fprintf(outfp, ",%i", colour_primaries);
    fprintf(outfp, ",%i", transfer_characteristics);
    fprintf(outfp, ",%i", matrix_coeffs);
    fprintf(outfp, ",%i", num_video_frames);
    fprintf(outfp, ",%f", frame_rate_fps_median);
    fprintf(outfp, ",%f", frame_rate_fps_average);
    fprintf(outfp, ",%f", frame_rate_fps_reverse_average);
    fprintf(outfp, ",%f", frame_rate_fps_stddev);
    fprintf(outfp, ",%i", video_freeze ? 1 : 0);
    fprintf(outfp, ",%f", audio_video_ratio);
    fprintf(outfp, ",%f", duration_video_sec);
    fprintf(outfp, ",%f", duration_audio_sec);
    fprintf(outfp, ",%u", timescale_video_hz);
    fprintf(outfp, ",%u", timescale_audio_hz);
    fprintf(outfp, ",%f", pts_duration_sec_average);
    fprintf(outfp, ",%f", pts_duration_sec_median);
    fprintf(outfp, ",%f", pts_duration_sec_stddev);
    fprintf(outfp, ",%f", pts_duration_sec_mad);
    fprintf(outfp, ",%i", frame_drop_count);
    fprintf(outfp, ",%f", frame_drop_ratio);
    fprintf(outfp, ",%f", normalized_frame_drop_average_length);
    fprintf(outfp, ",%f", frame_drop_length_percentile_list[0]);
    fprintf(outfp, ",%f", frame_drop_length_percentile_list[1]);
    fprintf(outfp, ",%ld", frame_drop_length_consecutive[0]);
    fprintf(outfp, ",%ld", frame_drop_length_consecutive[1]);
    fprintf(outfp, ",%i", num_video_keyframes);
    fprintf(outfp, ",%f", key_frame_ratio);
    fprintf(outfp, ",%s", audio_type.c_str());
    fprintf(outfp, ",%i", channel_count);
    fprintf(outfp, ",%i", sample_rate);
    fprintf(outfp, ",%i", sample_size);
    fprintf(outfp, "\n");

    // 2.4. capture outfile timestamps
    if (outfile_timestamps != nullptr) {
      std::vector<uint32_t> frame_num_orig_list =
          ptr->get_timing().get_frame_num_orig_list();
      std::vector<uint32_t> stts_unit_list =
          ptr->get_timing().get_stts_unit_list();
      std::vector<int32_t> ctts_unit_list =
          ptr->get_timing().get_ctts_unit_list();
      std::vector<float> dts_sec_list = ptr->get_timing().get_dts_sec_list();
      std::vector<float> pts_sec_list = ptr->get_timing().get_pts_sec_list();
      std::vector<float> pts_duration_sec_list =
          ptr->get_timing().get_pts_duration_sec_list();
      // store the values
      frame_num_orig_list_dict[infile] = frame_num_orig_list;
      stts_unit_list_dict[infile] = stts_unit_list;
      ctts_unit_list_dict[infile] = ctts_unit_list;
      dts_sec_list_dict[infile] = dts_sec_list;
      pts_sec_list_dict[infile] = pts_sec_list;
      pts_duration_sec_list_dict[infile] = pts_duration_sec_list;
    }
  }

  // 3. dump outfile timestamps
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
    fprintf(outtsfp, ",frame_num_orig");
    for (const auto &entry : stts_unit_list_dict) {
      const std::string &infile = entry.first;
      fprintf(outtsfp, ",stts_%s", infile.c_str());
      fprintf(outtsfp, ",ctts_%s", infile.c_str());
      fprintf(outtsfp, ",dts_%s", infile.c_str());
      fprintf(outtsfp, ",pts_%s", infile.c_str());
      fprintf(outtsfp, ",pts_duration_%s", infile.c_str());
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
      fprintf(outtsfp, "\n");
    }
  }

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
    c = getopt_long(argc, argv, "do:h", longopts, &optindex);
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
  arg_options *options;

  // parse args
  options = parse_args(argc, argv);
  if (options == nullptr) {
    usage(argv[0]);
    exit(-1);
  }

  // print args
  if (options->debug > 1) {
    printf("options->debug = %i\n", options->debug);
    printf("options->outfile = %s\n",
           (options->outfile == nullptr) ? "nullptr" : options->outfile);
    printf("options->outfile_timestamps = %s\n",
           (options->outfile_timestamps == nullptr)
               ? "nullptr"
               : options->outfile_timestamps);
    printf("options->outfile_timestamps_sort_pts = %i\n",
           options->outfile_timestamps_sort_pts);
    printf("options->nruns = %i\n", options->nruns);

    for (const auto &infile : options->infile_list) {
      printf("options->infile = %s\n", infile.c_str());
    }
  }

  for (int i = 0; i < options->nruns; ++i) {
    parse_files(options->infile_list, options->outfile,
                options->outfile_timestamps,
                options->outfile_timestamps_sort_pts, options->debug);
  }

  return 0;
}
