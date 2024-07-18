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
  std::vector<std::string> infile_list;
} arg_options;

/* default option values */
arg_options DEFAULT_OPTIONS{
    .debug = 0,
    .outfile = nullptr,
};

int parse_files(std::vector<std::string> &infile_list, char *outfile,
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
  fprintf(
      outfp,
      "infile,num_video_frames,frame_rate_fps,video_freeze,video_freeze_ratio,"
      "frame_drop_ratio,normalized_frame_drop_average_length\n");

  // 2. write CSV rows
  for (const auto &infile : infile_list) {
    // 2.1. get video freeze info
    bool video_freeze;
    float audio_video_ratio;
    int ret = get_video_freeze_info(infile.c_str(), &video_freeze,
                                    &audio_video_ratio, debug);
    if (ret < 0) {
      fprintf(stderr, "error: get_video_freeze_info() in %s\n", infile.c_str());
      return -1;
    }

    // 2.2. get frame drop info
    int num_video_frames;
    float frame_rate_fps;
    float frame_drop_ratio;
    float normalized_frame_drop_average_length;
    ret = get_frame_drop_info(infile.c_str(), &num_video_frames,
                              &frame_rate_fps, &frame_drop_ratio,
                              &normalized_frame_drop_average_length, debug);
    if (ret < 0) {
      fprintf(stderr, "error: get_frame_drop_info() in %s\n", infile.c_str());
      return -1;
    }

    // 2.3. dump all output
    fprintf(outfp, "%s", infile.c_str());
    fprintf(outfp, ",%i", num_video_frames);
    fprintf(outfp, ",%f", frame_rate_fps);
    fprintf(outfp, ",%i", video_freeze ? 1 : 0);
    fprintf(outfp, ",%f", audio_video_ratio);
    fprintf(outfp, ",%f", frame_drop_ratio);
    fprintf(outfp, ",%f", normalized_frame_drop_average_length);
    fprintf(outfp, "\n");
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
  fprintf(stderr, "\t-h:\t\tHelp\n");
  exit(-1);
}

// long options with no equivalent short option
enum {
  QUIET_OPTION = CHAR_MAX + 1,
  HELP_OPTION,
};

arg_options *parse_args(int argc, char **argv) {
  int c;
  char *endptr;
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

      case QUIET_OPTION:
        options.debug = 0;
        break;

      case HELP_OPTION:
      case 'h':
        usage(argv[0]);

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

    for (const auto &infile : options->infile_list) {
      printf("options->infile = %s\n", infile.c_str());
    }
  }

  parse_files(options->infile_list, options->outfile, options->debug);

  return 0;
}
