// lcvm: frame dups and freezes detector

// A show case of using liblcvm to detect frame dups and
// video freezes in ISOBMFF files.

#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>  // for optarg

#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>
#include <string>  // for basic_string, string
#include <vector>

#include "config.h"
#include "liblcvm.h"
#if ADD_POLICY
#include "policy_protovisitor.h"
#endif

extern int optind;

/* option values */
typedef struct arg_options {
  int debug;
  int nruns;
  char* outfile;
  char* outfile_timestamps;
  bool outfile_timestamps_sort_pts;
  bool skip_audio_parsing;
  uint32_t max_num_frames;
  int64_t audio_track_id;
  int64_t video_track_id;
  std::vector<std::string> infile_list;
#if ADD_POLICY
  char* policy_file;
#endif
} arg_options;

/* default option values */
arg_options DEFAULT_OPTIONS{
    .debug = 0,
    .nruns = 1,
    .outfile = nullptr,
    .outfile_timestamps = nullptr,
    .outfile_timestamps_sort_pts = true,
    .skip_audio_parsing = false,
    .max_num_frames = LiblcvmConfig::kDefaultMaxNumFrames,
    .audio_track_id = -1,
    .video_track_id = -1,
    .infile_list = {},
#if ADD_POLICY
    .policy_file = nullptr,
#endif
};

std::string csv_escape(const std::string& value) {
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

// Recognize explicitly qualified library fields, then remove singleton suffix
// segments from right to left so a retained audio suffix does not hide the
// preceding video suffix in a pair field. Unsuffixed aliases return false.
bool compact_track_key(const std::string& key, size_t num_video_tracks,
                       size_t num_audio_tracks, std::string* compact_key) {
  *compact_key = key;
  bool qualified = false;
  size_t end = key.size();
  while (end > 0) {
    const auto marker = key.find_last_not_of("0123456789", end - 1);
    if (marker == std::string::npos || marker == 0 || marker + 1 == end ||
        key[marker - 1] != '_' || (key[marker] != 'v' && key[marker] != 'a')) {
      break;
    }
    const auto start = marker - 1;
    const auto count = key[marker] == 'v' ? num_video_tracks : num_audio_tracks;
    if (count == 1)
      compact_key->erase(start, end - start);
    qualified = true;
    end = start;
  }
  return qualified;
}

int parse_files(std::vector<std::string>& infile_list, char* outfile,
                char* outfile_timestamps, bool outfile_timestamps_sort_pts,
                bool skip_audio_parsing, uint32_t max_num_frames, int debug,
                const std::string& policy_str, int64_t audio_track_id,
                int64_t video_track_id) {
  // 1. open outfile
  FILE* outfp;
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
  liblcvm_config->set_skip_audio_parsing(skip_audio_parsing);
  liblcvm_config->set_max_num_frames(max_num_frames);
  liblcvm_config->set_policy(policy_str);
  liblcvm_config->set_debug(debug);
  liblcvm_config->set_audio_track_id(audio_track_id);
  liblcvm_config->set_video_track_id(video_track_id);
  liblcvm_config->set_include_track_keys(true);

  // 3. parse the input files
  int ret = 0;
  // Inputs can have different IDs and numbers of video/audio tracks. Build a
  // stable union header before emitting rows; absent fields stay empty.
  LiblcvmKeyList csv_keys;
  std::set<std::string> seen_csv_keys;
  // Filter library selected-track aliases BEFORE compacting qualified fields.
  // File-wide metadata is never track-qualified; singleton dimensions need no
  // suffix in CLI output. Selectors do not limit the exported track results.
  const std::set<std::string> file_keys = {
      "infile",         "filesize",  "bitrate_bps", "timescale_movie_hz",
      "policy_version", "warn_list", "error_list"};
  std::vector<std::map<std::string, std::string>> csv_rows;
  LiblcvmKeyList keys_timing;
  std::map<std::string, LiblcvmTimingList> vals_timing_map;
  bool calculate_timestamps = outfile_timestamps != nullptr;
  for (const auto& infile : infile_list) {
    LiblcvmKeyList keys;
    LiblcvmValList vals;
    LiblcvmKeyList pkeys_timing;
    LiblcvmTimingList vals_timing;
    // Retain the parsed object for cheap cardinality queries, rather than
    // parsing twice or copying the per-track timing analyses just to count.
    const auto file =
        IsobmffFileInformation::parse(infile.c_str(), *liblcvm_config);
    if (file && file->get_num_video_tracks() == 0) {
      fprintf(stderr, "error: no video track in %s\n", infile.c_str());
      ret = -1;
      continue;
    }
    if (!file ||
        IsobmffFileInformation::LiblcvmConfig_to_lists(
            file, &keys, &vals, calculate_timestamps, &pkeys_timing,
            &vals_timing, debug, audio_track_id, true, video_track_id)) {
      fprintf(stderr, "error: failed to parse/export %s\n", infile.c_str());
      // Keep processing the remaining files, but report the error. Local
      // timestamp lists ensure a later failure cannot erase a valid header.
      ret = -1;
      continue;
    }
    const auto num_video_tracks = file->get_num_video_tracks();
    const auto num_audio_tracks = file->get_num_audio_tracks();
    std::map<std::string, std::string> row;
    for (size_t i = 0; i < keys.size(); ++i) {
      const auto& key = keys[i];
      std::string csv_key;
      if (file_keys.count(key) != 0) {
        csv_key = key;
      } else if (!compact_track_key(key, num_video_tracks, num_audio_tracks,
                                    &csv_key)) {
        continue;
      }
      if (seen_csv_keys.insert(csv_key).second)
        csv_keys.push_back(csv_key);
      std::string value;
      if (liblcvmvalue_to_string(vals[i], &value) != 0)
        value = "ERROR";
      row.emplace(std::move(csv_key), std::move(value));
    }
    csv_rows.push_back(std::move(row));

    // capture outfile timestamps
    if (calculate_timestamps) {
      keys_timing = std::move(pkeys_timing);
      vals_timing_map.emplace(infile, std::move(vals_timing));
    }
  }

  if (!csv_rows.empty()) {
    for (size_t i = 0; i < csv_keys.size(); ++i) {
      fprintf(outfp, "%s%s", csv_keys[i].c_str(),
              i + 1 < csv_keys.size() ? "," : "\n");
    }
    for (const auto& row : csv_rows) {
      for (size_t i = 0; i < csv_keys.size(); ++i) {
        const auto value = row.find(csv_keys[i]);
        const auto escaped =
            value == row.end() ? "" : csv_escape(value->second);
        fprintf(outfp, "%s%s", escaped.c_str(),
                i + 1 < csv_keys.size() ? "," : "\n");
      }
    }
  }

  // 4. dump outfile timestamps
  if (calculate_timestamps) {
    // 3.1. open outfile_timestamps
    FILE* outtsfp = fopen(outfile_timestamps, "wb");
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
    fprintf(outtsfp, "filename,frame_num");
    for (const auto& key : keys_timing) {
      fprintf(outtsfp, ",%s", key.c_str());
    }
    fprintf(outtsfp, "\n");

    // 3.3. write CSV rows
    for (const auto& entry : vals_timing_map) {
      const auto& filename = entry.first;
      const auto& vals_timing = entry.second;
      for (size_t frame_num = 0; frame_num < vals_timing.size(); ++frame_num) {
        fprintf(outtsfp, "%s", filename.c_str());
        fprintf(outtsfp, ",%zu", frame_num);
        const LiblcvmTiming& timing = vals_timing[frame_num];
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
  return ret;
}

void usage(char* name) {
  fprintf(stderr, "usage: %s [options] <infile(s)>\n", name);
  fprintf(stderr, "where options are:\n");
  fprintf(stderr, "\t-d:\t\tIncrease debug verbosity [%i]\n",
          DEFAULT_OPTIONS.debug);
  fprintf(stderr, "\t-q:\t\tZero debug verbosity\n");
  fprintf(stderr, "\t--runs <nruns>:\t\tRun the analysis multiple times [%i]\n",
          DEFAULT_OPTIONS.nruns);
#if ADD_POLICY
  fprintf(stderr, "\t-p policy file:\t\tSpecify policy file to be parsed\n");
  fprintf(stderr,
          "\t--policy policy file:\t\tSpecify policy file to be parsed\n");
#endif
  fprintf(stderr, "\t-o outfile:\t\tSelect outfile\n");
  fprintf(stderr,
          "\t--outfile-timestamps outfile_timestamps:\t\tSelect outfile to "
          "dump timestamps\n");
  fprintf(stderr, "\t--sort-pts:\t\tSort outfile timestamps by PTS\n");
  fprintf(stderr, "\t--no-sort-pts:\t\tDo not outfile timestamps by PTS\n");
  fprintf(stderr, "\t--skip-audio-parsing:\tSkip all audio-track analysis\n");
  fprintf(stderr,
          "\t--no-skip-audio-parsing:\tParse the audio sample entry "
          "(default)\n");
  fprintf(stderr,
          "\t--max-num-frames <num>:\tReject files with more video frames "
          "[%u]\n",
          DEFAULT_OPTIONS.max_num_frames);
  fprintf(stderr,
          "\t--audio-track-id <id>:\tSelect audio for policy/default view; "
          "-1 selects the first audio track (default)\n");
  fprintf(stderr,
          "\t--video-track-id <id>:\tSelect video for policy and timestamps; "
          "-1 selects the first video track (default)\n");
  fprintf(stderr,
          "\tCSV contains all tracks and file-wide metadata; _v<ID> and "
          "_a<ID> suffixes appear only for non-singleton dimensions\n");
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
  SKIP_AUDIO_PARSING_OPTION,
  NO_SKIP_AUDIO_PARSING_OPTION,
  MAX_NUM_FRAMES_OPTION,
  AUDIO_TRACK_ID_OPTION,
  VIDEO_TRACK_ID_OPTION,
  RUNS_OPTION,
  VERSION_OPTION,
};

arg_options* parse_args(int argc, char** argv) {
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
#if ADD_POLICY
      {"policy", required_argument, nullptr, 'p'},
#endif
      {"outfile-timestamps", required_argument, nullptr,
       OUTFILE_TIMESTAMPS_OPTION},
      {"sort-pts", no_argument, nullptr, SORT_PTS_OPTION},
      {"no-sort-pts", no_argument, nullptr, NO_SORT_PTS_OPTION},
      {"skip-audio-parsing", no_argument, nullptr, SKIP_AUDIO_PARSING_OPTION},
      {"no-skip-audio-parsing", no_argument, nullptr,
       NO_SKIP_AUDIO_PARSING_OPTION},
      // options without a short option
      {"max-num-frames", required_argument, nullptr, MAX_NUM_FRAMES_OPTION},
      {"audio-track-id", required_argument, nullptr, AUDIO_TRACK_ID_OPTION},
      {"video-track-id", required_argument, nullptr, VIDEO_TRACK_ID_OPTION},
      {"runs", required_argument, nullptr, RUNS_OPTION},
      {"quiet", no_argument, nullptr, QUIET_OPTION},
      {"version", no_argument, NULL, VERSION_OPTION},
      {"help", no_argument, nullptr, HELP_OPTION},
      {nullptr, 0, nullptr, 0}};

  // parse arguments
  while (true) {
#if ADD_POLICY
    c = getopt_long(argc, argv, "do:hp:", longopts, &optindex);
#else
    c = getopt_long(argc, argv, "do:h", longopts, &optindex);
#endif
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

#if ADD_POLICY
      case 'p':
        options.policy_file = optarg;
        break;
#endif

      case OUTFILE_TIMESTAMPS_OPTION:
        options.outfile_timestamps = optarg;
        break;

      case SORT_PTS_OPTION:
        options.outfile_timestamps_sort_pts = true;
        break;

      case NO_SORT_PTS_OPTION:
        options.outfile_timestamps_sort_pts = false;
        break;

      case SKIP_AUDIO_PARSING_OPTION:
        options.skip_audio_parsing = true;
        break;

      case NO_SKIP_AUDIO_PARSING_OPTION:
        options.skip_audio_parsing = false;
        break;

      case MAX_NUM_FRAMES_OPTION: {
        char* endptr;
        unsigned long val = strtoul(optarg, &endptr, 0);
        if (*endptr != '\0' || val == 0 || val > UINT32_MAX) {
          fprintf(stderr, "error: invalid --max-num-frames parameter: %s\n",
                  optarg);
          exit(-1);
        }
        options.max_num_frames = static_cast<uint32_t>(val);
      } break;

      case AUDIO_TRACK_ID_OPTION:
      case VIDEO_TRACK_ID_OPTION: {
        char* endptr;
        errno = 0;
        const long long val = strtoll(optarg, &endptr, 0);
        if (endptr == optarg || *endptr != '\0' || errno == ERANGE ||
            val < -1 || val > UINT32_MAX) {
          fprintf(
              stderr, "error: invalid --%s parameter: %s\n",
              c == AUDIO_TRACK_ID_OPTION ? "audio-track-id" : "video-track-id",
              optarg);
          exit(-1);
        }
        if (c == AUDIO_TRACK_ID_OPTION) {
          options.audio_track_id = static_cast<int64_t>(val);
        } else {
          options.video_track_id = static_cast<int64_t>(val);
        }
      } break;

      case QUIET_OPTION:
        options.debug = 0;
        break;

      case RUNS_OPTION: {
        char* endptr;
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

int main(int argc, char** argv) {
  arg_options* options = parse_args(argc, argv);
  if (!options) {
    usage(argv[0]);
    exit(-1);
  }

  std::string policy_str;
#if ADD_POLICY
  if (options->policy_file) {
    FILE* pf = fopen(options->policy_file, "r");
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
#endif

  int ret = 0;
  for (int i = 0; i < options->nruns; ++i) {
    if (parse_files(
            options->infile_list, options->outfile, options->outfile_timestamps,
            options->outfile_timestamps_sort_pts, options->skip_audio_parsing,
            options->max_num_frames, options->debug, policy_str,
            options->audio_track_id, options->video_track_id) != 0) {
      ret = -1;
    }
  }
  return ret;
}
