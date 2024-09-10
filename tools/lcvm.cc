// lcvm: frame dups and freezes detector

// A show case of using liblcvm to detect frame dups and
// video freezes in ISOBMFF files.

#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>  // for optarg

#include <climits>
#include <cstdio>
#include <cstring>
#include <string>  // for basic_string, string
#include <vector>

#include "liblcvm.h"

extern int optind;

/* option values */
typedef struct arg_options {
  int debug;
  char *outfile;
  char *outfile_timestamps;
  std::vector<std::string> infile_list;
} arg_options;

/* default option values */
arg_options DEFAULT_OPTIONS{
    .debug = 0,
    .outfile = nullptr,
    .outfile_timestamps = nullptr,
    .infile_list = {},
};

int parse_files(std::vector<std::string> &infile_list, char *outfile,
                char *outfile_timestamps, int debug) {
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
          "infile,num_video_frames,frame_rate_fps_median,"
          "frame_rate_fps_average,frame_rate_fps_stddev,video_freeze,"
          "video_freeze_ratio,duration_video_sec,duration_audio_sec,"
          "frame_drop_count,frame_drop_ratio,"
          "normalized_frame_drop_average_length,"
          "frame_drop_length_percentile_50,frame_drop_length_percentile_90\n");

  // 2. write CSV rows
  std::vector<std::vector<float>> delta_timestamp_sec_list_list;
  for (const auto &infile : infile_list) {
    // 2.1. get video freeze info
    bool video_freeze;
    float audio_video_ratio;
    float duration_video_sec;
    float duration_audio_sec;
    int ret =
        get_video_freeze_info(infile.c_str(), &video_freeze, &audio_video_ratio,
                              &duration_video_sec, &duration_audio_sec, debug);
    if (ret < 0) {
      fprintf(stderr, "error: get_video_freeze_info() in %s\n", infile.c_str());
    }

    // 2.2. get frame drop info
    int num_video_frames;
    float frame_rate_fps_median;
    float frame_rate_fps_average;
    float frame_rate_fps_stddev;
    int frame_drop_count;
    float frame_drop_ratio;
    std::vector<float> percentile_list = {50, 90};
    std::vector<float> frame_drop_length_percentile_list;
    float normalized_frame_drop_average_length;
    ret = get_frame_drop_info(
        infile.c_str(), &num_video_frames, &frame_rate_fps_median,
        &frame_rate_fps_average, &frame_rate_fps_stddev, &frame_drop_count,
        &frame_drop_ratio, &normalized_frame_drop_average_length,
        percentile_list, frame_drop_length_percentile_list, debug);
    if (ret < 0) {
      fprintf(stderr, "error: get_frame_drop_info() in %s\n", infile.c_str());
      return -1;
    }

    // 2.3. dump all output
    fprintf(outfp, "%s", infile.c_str());
    fprintf(outfp, ",%i", num_video_frames);
    fprintf(outfp, ",%f", frame_rate_fps_median);
    fprintf(outfp, ",%f", frame_rate_fps_average);
    fprintf(outfp, ",%f", frame_rate_fps_stddev);
    fprintf(outfp, ",%i", video_freeze ? 1 : 0);
    fprintf(outfp, ",%f", audio_video_ratio);
    fprintf(outfp, ",%f", duration_video_sec);
    fprintf(outfp, ",%f", duration_audio_sec);
    fprintf(outfp, ",%i", frame_drop_count);
    fprintf(outfp, ",%f", frame_drop_ratio);
    fprintf(outfp, ",%f", normalized_frame_drop_average_length);
    fprintf(outfp, ",%f", frame_drop_length_percentile_list[0]);
    fprintf(outfp, ",%f", frame_drop_length_percentile_list[1]);
    fprintf(outfp, "\n");

    // 2.4. capture outfile timestamps
    if (outfile_timestamps != nullptr) {
      std::vector<float> delta_timestamp_sec_list;
      ret = get_frame_interframe_info(infile.c_str(), &num_video_frames,
                                      delta_timestamp_sec_list, debug);
      if (ret < 0) {
        fprintf(stderr, "error: get_frame_interframe_info() in %s\n",
                infile.c_str());
      }
      delta_timestamp_sec_list_list.push_back(delta_timestamp_sec_list);
    }
  }

  // 3. dump outfile timestamps
  if (outfile_timestamps != nullptr) {
    // 3.1. get the maximum number of frames
    size_t max_number_of_frames = 0;
    for (const auto &delta_timestamp_sec_list : delta_timestamp_sec_list_list) {
      max_number_of_frames =
          std::max(max_number_of_frames, delta_timestamp_sec_list.size());
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
    for (const auto &infile : infile_list) {
      fprintf(outtsfp, ",%s", infile.c_str());
    }
    fprintf(outtsfp, "\n");
    // 3.4. dump the columns of inter-frame timestamps
    for (size_t i = 0; i < max_number_of_frames; ++i) {
      fprintf(outtsfp, "%li", i);
      for (const auto &delta_timestamp_sec_list :
           delta_timestamp_sec_list_list) {
        if (i < delta_timestamp_sec_list.size()) {
          float delta_timestamp_sec = delta_timestamp_sec_list[i];
          fprintf(outtsfp, ",%f", delta_timestamp_sec);
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
  fprintf(stderr, "\t-o outfile:\t\tSelect outfile\n");
  fprintf(stderr,
          "\t--outfile_timestamps outfile_timestamps:\t\tSelect outfile to "
          "dump timestamps\n");
  fprintf(stderr, "\t-h:\t\tHelp\n");
  exit(-1);
}

// long options with no equivalent short option
enum {
  QUIET_OPTION = CHAR_MAX + 1,
  HELP_OPTION,
  OUTFILE_TIMESTAMPS_OPTION,
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
      // options without a short option
      {"quiet", no_argument, nullptr, QUIET_OPTION},
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

      case QUIET_OPTION:
        options.debug = 0;
        break;

      case HELP_OPTION:
      case 'h':
        usage(argv[0]);
        break;

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

    for (const auto &infile : options->infile_list) {
      printf("options->infile = %s\n", infile.c_str());
    }
  }

  parse_files(options->infile_list, options->outfile,
              options->outfile_timestamps, options->debug);

  return 0;
}
