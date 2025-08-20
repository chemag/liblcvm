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

std::string csv_escape(const std::string &value) {
  bool must_quote = value.find_first_of(",\"\n") != std::string::npos;
  std::string escaped = value;
  size_t pos = 0;
  while ((pos = escaped.find('"', pos)) != std::string::npos) {
    escaped.insert(pos, "\"");
    pos += 2;
  }
  if (must_quote) {
    escaped = "\"" + escaped + "\"";
  }
  return escaped;
}

int parse_files(std::vector<std::string> &infile_list, char *outfile,
                char *outfile_timestamps, bool outfile_timestamps_sort_pts,
                int debug, const std::string &policy_str) {
  // 1. open outfile
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

  // 2. set parsing parameters
  auto liblcvm_config = std::make_unique<LiblcvmConfig>();
  liblcvm_config->set_sort_by_pts(outfile_timestamps_sort_pts);
  liblcvm_config->set_policy(policy_str);
  liblcvm_config->set_debug(debug);

  // 3. parse the input files
  bool printed_csv_header = false;
  LiblcvmKeyList keys_timing;
  std::map<std::string, LiblcvmTimingList> vals_timing_map;
  bool calculate_timestamps = outfile_timestamps != nullptr;
  for (const auto &infile : infile_list) {
    LiblcvmKeyList keys;
    LiblcvmValList vals;
    LiblcvmKeyList pkeys_timing;
    LiblcvmTimingList vals_timing;
    if (IsobmffFileInformation::parse_to_lists(
            infile.c_str(), *liblcvm_config, &keys, &vals, calculate_timestamps,
            &keys_timing, &vals_timing)) {
      fprintf(stderr, "error: IsobmffFileInformation::parse_to_map() in %s\n",
              infile.c_str());
      continue;
    }
    // write CSV header
    if (!printed_csv_header) {
      for (size_t i = 0; i < keys.size(); ++i) {
        fprintf(outfp, "%s%s", keys[i].c_str(),
                (i + 1 < keys.size()) ? "," : "\n");
      }
      printed_csv_header = true;
    }

    // write CSV rows
    for (size_t i = 0; i < vals.size(); ++i) {
      std::string value = liblcvmvalue_to_string(vals[i]);
      fprintf(outfp, "%s%s", csv_escape(value).c_str(),
              (i + 1 < vals.size()) ? "," : "\n");
    }

    // capture outfile timestamps
    if (calculate_timestamps) {
      vals_timing_map.emplace(infile, vals_timing);
    }
  }

  // 4. dump outfile timestamps
  if (calculate_timestamps) {
    // 3.1. open outfile_timestamps
    FILE *outtsfp = fopen(outfile_timestamps, "wb");
    if (outtsfp == nullptr) {
      // did not work
      fprintf(stderr, "Could not open output file: \"%s\"\n",
              outfile_timestamps);
      if (outfp != stdout) {
        fclose(outfp);
      }
      return -1;
    }

    // 3.2. write CSV header
    fprintf(outtsfp, "%s,", "filename");
    fprintf(outtsfp, "%s,", "frame_num");
    for (size_t i = 0; i < keys_timing.size(); ++i) {
      fprintf(outtsfp, "%s%s", keys_timing[i].c_str(),
              (i + 1 < keys_timing.size()) ? "," : "\n");
    }

    // 3.3. write CSV rows
    for (const auto &entry : vals_timing_map) {
      const auto &filename = entry.first;
      const auto &vals_timing = entry.second;
      for (size_t frame_num = 0; frame_num < vals_timing.size(); ++frame_num) {
        fprintf(outtsfp, "%s", filename.c_str());
        fprintf(outtsfp, ",%zu", frame_num);
        const LiblcvmTiming &timing = vals_timing[frame_num];
        // frame_num
        std::string value0 = std::to_string(std::get<0>(timing));
        fprintf(outtsfp, ",%s", csv_escape(value0).c_str());
        // stts
        std::string value1 = std::to_string(std::get<1>(timing));
        fprintf(outtsfp, ",%s", csv_escape(value1).c_str());
        // ctts
        std::string value2 = std::to_string(std::get<2>(timing));
        fprintf(outtsfp, ",%s", csv_escape(value2).c_str());
        // dts
        std::string value3 = std::to_string(std::get<3>(timing));
        fprintf(outtsfp, ",%s", csv_escape(value3).c_str());
        // pts
        std::string value4 = std::to_string(std::get<4>(timing));
        fprintf(outtsfp, ",%s", csv_escape(value4).c_str());
        // pts_duration
        std::string value5 = std::to_string(std::get<5>(timing));
        fprintf(outtsfp, ",%s", csv_escape(value5).c_str());
        // pts_duration_delta
        std::string value6 = std::to_string(std::get<6>(timing));
        fprintf(outtsfp, ",%s", csv_escape(value6).c_str());
        // pts_framerate
        std::string value7 = std::to_string(std::get<7>(timing));
        fprintf(outtsfp, ",%s", csv_escape(value7).c_str());
        fprintf(outtsfp, "\n");
      }
    }
    fclose(outtsfp);
  }
  if (outfp != stdout) {
    fclose(outfp);
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
  fprintf(stderr, "\t-p policy file:\t\tSpecify policy file to be parsed\n");
  fprintf(stderr,
          "\t--policy policy file:\t\tSpecify policy file to be parsed\n");
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
    if (options->debug > 0) {
      printf("Read policy file (%ld bytes)\n", fsize);
    }
  }

  for (int i = 0; i < options->nruns; ++i) {
    parse_files(
        options->infile_list, options->outfile, options->outfile_timestamps,
        options->outfile_timestamps_sort_pts, options->debug, policy_str);
  }
  return 0;
}
