// liblcvm: Low-Complexity Video Metrics Library.
// A library to detect frame dups and video freezes.

// A show case of using [ISOBMFF](https://github.com/DigiDNA/ISOBMFF) to
// detect frame dups and video freezes in ISOBMFF files.

#include <stdint.h>  // for uint32_t, uint64_t

#include <ISOBMFF.hpp>         // for various
#include <ISOBMFF/Parser.hpp>  // for Parser
#include <algorithm>           // for sort
#include <cmath>               // for sqrt
#include <cstdio>              // for fprintf, stderr, stdout
#include <memory>              // for shared_ptr, operator==, __shared...
#include <numeric>             // for accumulate
#include <string>              // for basic_string, string
#include <vector>              // for vector

#define MAX_AUDIO_VIDEO_RATIO 0.05

int get_timing_information(const char *infile, int *num_video_frames,
                           float *duration_video_sec, float *duration_audio_sec,
                           std::vector<float> &delta_timestamp_sec_list,
                           int debug) {
  // 1. parse the input file
  ISOBMFF::Parser parser;
  parser.Parse(infile);
  std::shared_ptr<ISOBMFF::File> file = parser.GetFile();
  if (file == nullptr) {
    if (debug > 0) {
      fprintf(stderr, "error: no file in %s\n", infile);
    }
    return -1;
  }

  // 2. look for a moov box
  std::shared_ptr<ISOBMFF::ContainerBox> moov =
      file->GetTypedBox<ISOBMFF::ContainerBox>("moov");
  if (moov == nullptr) {
    if (debug > 0) {
      fprintf(stderr, "error: no /moov in %s\n", infile);
    }
    return -1;
  }

  // 3. look for trak boxes
  *duration_video_sec = -1.0;
  *duration_audio_sec = -1.0;
  for (auto &box : moov->GetBoxes()) {
    std::string name = box->GetName();
    if (name.compare("trak") != 0) {
      continue;
    }
    auto trak = std::dynamic_pointer_cast<ISOBMFF::ContainerBox>(box);

    // 4. look for a mdia box
    std::shared_ptr<ISOBMFF::ContainerBox> mdia =
        trak->GetTypedBox<ISOBMFF::ContainerBox>("mdia");
    if (mdia == nullptr) {
      if (debug > 0) {
        fprintf(stderr, "error: no /moov/trak/mdia in %s\n", infile);
      }
      return -1;
    }

    // 5. look for a hdlr box
    std::shared_ptr<ISOBMFF::HDLR> hdlr =
        mdia->GetTypedBox<ISOBMFF::HDLR>("hdlr");
    if (hdlr == nullptr) {
      if (debug > 0) {
        fprintf(stderr, "error: no /moov/trak/mdia/hdlr in %s\n", infile);
      }
      return -1;
    }
    std::string handler_type = hdlr->GetHandlerType();

    // 6. look for a mdhd box
    std::shared_ptr<ISOBMFF::MDHD> mdhd =
        mdia->GetTypedBox<ISOBMFF::MDHD>("mdhd");
    if (mdhd == nullptr) {
      if (debug > 0) {
        fprintf(stderr, "error: no /moov/trak/mdia/mdhd in %s\n", infile);
      }
      return -1;
    }
    uint32_t timescale = mdhd->GetTimescale();
    uint64_t duration = mdhd->GetDuration();
    float duration_sec = (float)duration / timescale;
    if (debug > 1) {
      fprintf(stdout, "-> handler_type: %s ", handler_type.c_str());
      fprintf(stdout, "timescale: %u ", timescale);
      fprintf(stdout, "duration: %lu ", duration);
      fprintf(stdout, "duration_sec: %f\n", duration_sec);
    }
    if (handler_type.compare("soun") == 0) {
      *duration_audio_sec = duration_sec;
    } else if (handler_type.compare("vide") == 0) {
      *duration_video_sec = duration_sec;
    }

    // only keep video processing
    if (handler_type.compare("vide") != 0) {
      continue;
    }

    // 7. look for a minf box
    std::shared_ptr<ISOBMFF::ContainerBox> minf =
        mdia->GetTypedBox<ISOBMFF::ContainerBox>("minf");
    if (minf == nullptr) {
      if (debug > 0) {
        fprintf(stderr, "error: no /moov/trak/mdia/minf in %s\n", infile);
      }
      return -1;
    }

    // 8. look for a stbl box
    std::shared_ptr<ISOBMFF::ContainerBox> stbl =
        minf->GetTypedBox<ISOBMFF::ContainerBox>("stbl");
    if (stbl == nullptr) {
      if (debug > 0) {
        fprintf(stderr, "error: no /moov/trak/mdia/minf/stbl in %s\n", infile);
      }
      return -1;
    }

    // 9. look for a stts box
    std::shared_ptr<ISOBMFF::STTS> stts =
        stbl->GetTypedBox<ISOBMFF::STTS>("stts");
    if (stts == nullptr) {
      if (debug > 0) {
        fprintf(stderr, "error: no /moov/trak/mdia/minf/stbl/stts in %s\n",
                infile);
        return -1;
      }
    }

    // 10. gather the inter-frame timestamp deltas
    delta_timestamp_sec_list.clear();
    *num_video_frames = 0;
    for (unsigned int i = 0; i < stts->GetEntryCount(); i++) {
      uint32_t sample_count = stts->GetSampleCount(i);
      *num_video_frames += sample_count;
      uint32_t sample_offset = stts->GetSampleOffset(i);
      float sample_offset_sec = (float)sample_offset / timescale;
      for (uint32_t sample = 0; sample < sample_count; sample++) {
        delta_timestamp_sec_list.push_back(sample_offset_sec);
      }
      if (debug > 2) {
        fprintf(stdout, "sample_count: %u ", sample_count);
        fprintf(stdout, "sample_offset: %u ", sample_offset);
        fprintf(stdout, "sample_offset_sec: %f\n", sample_offset_sec);
      }
    }
  }

  return 0;
}

int get_frame_interframe_info(const char *infile, int *num_video_frames,
                              std::vector<float> &delta_timestamp_sec_list,
                              int debug) {
  // get the list of inter-frame timestamp distances.
  float duration_video_sec;
  float duration_audio_sec;
  if (get_timing_information(infile, num_video_frames, &duration_video_sec,
                             &duration_audio_sec, delta_timestamp_sec_list,
                             debug) < 0) {
    return -1;
  }
  return 0;
}

int get_frame_drop_info(const char *infile, int *num_video_frames,
                        float *frame_rate_fps_median,
                        float *frame_rate_fps_average,
                        float *frame_rate_fps_stddev, int *frame_drop_count,
                        float *frame_drop_ratio,
                        float *normalized_frame_drop_average_length,
                        const std::vector<float> percentile_list,
                        std::vector<float> &frame_drop_length_percentile_list,
                        int debug) {
  // 0. get the list of inter-frame timestamp distances.
  float duration_video_sec;
  float duration_audio_sec;
  std::vector<float> delta_timestamp_sec_list;
  if (get_timing_information(infile, num_video_frames, &duration_video_sec,
                             &duration_audio_sec, delta_timestamp_sec_list,
                             debug) < 0) {
    return -1;
  }
  if (debug > 1) {
    for (const auto &delta_timestamp_sec : delta_timestamp_sec_list) {
      fprintf(stdout, "%f,", delta_timestamp_sec);
    }
    fprintf(stdout, "\n");
  }

  // 1. calculate the inter-frame distance statistics
  // 1.1. get the list of frame rates
  std::vector<float> frame_rate_fps_list(delta_timestamp_sec_list.size());
  std::transform(delta_timestamp_sec_list.begin(),
                 delta_timestamp_sec_list.end(), frame_rate_fps_list.begin(),
                 [](float val) {
                   // Handle division by zero
                   return val != 0.0f ? 1.0f / static_cast<float>(val) : 0.0f;
                 });
  // 1.2. median
  sort(frame_rate_fps_list.begin(), frame_rate_fps_list.end());
  *frame_rate_fps_median =
      frame_rate_fps_list[frame_rate_fps_list.size() / 2 - 1];
  sort(delta_timestamp_sec_list.begin(), delta_timestamp_sec_list.end());
  float delta_timestamp_sec_median =
      delta_timestamp_sec_list[delta_timestamp_sec_list.size() / 2 - 1];
  // 1.3. average
  *frame_rate_fps_average =
      (1.0 * std::accumulate(frame_rate_fps_list.begin(),
                             frame_rate_fps_list.end(), 0.0)) /
      frame_rate_fps_list.size();
  // 1.4. stddev
  // vmas := value minus average square
  std::vector<float> frame_rate_fps_vmas_list(frame_rate_fps_list.size());
  std::transform(frame_rate_fps_list.begin(), frame_rate_fps_list.end(),
                 frame_rate_fps_vmas_list.begin(), [=](float val) {
                   return (val - (*frame_rate_fps_average)) *
                          (val - (*frame_rate_fps_average));
                 });
  double frame_rate_fps_square_sum = std::accumulate(
      frame_rate_fps_vmas_list.begin(), frame_rate_fps_vmas_list.end(), 0.0);
  *frame_rate_fps_stddev =
      std::sqrt(frame_rate_fps_square_sum / frame_rate_fps_vmas_list.size());

  // 2. calculate the threshold to consider frame drop: This should be 2
  // times the median, minus a factor
  double FACTOR = 0.75;
  float delta_timestamp_sec_threshold = delta_timestamp_sec_median * FACTOR * 2;

  // 3. get the list of all the drops (absolute inter-frame values)
  std::vector<float> drop_length_sec_list;
  for (const auto &delta_timestamp_sec : delta_timestamp_sec_list) {
    if (delta_timestamp_sec > delta_timestamp_sec_threshold) {
      drop_length_sec_list.push_back(delta_timestamp_sec);
    }
  }
  // drop_length_sec_list: {0.6668900000000022, 0.10025600000000168, ...}

  // 4. sum all the drops, but adding only the length over 1x frame time
  float drop_length_sec_list_sum = 0.0;
  for (const auto &drop_length_sec : drop_length_sec_list) {
    drop_length_sec_list_sum += drop_length_sec;
  }
  float drop_length_duration_sec =
      drop_length_sec_list_sum -
      delta_timestamp_sec_median * drop_length_sec_list.size();
  // drop_length_duration_sec: sum({33.35900000000022, 66.92600000000168, ...})

  // 5. get the total duration as the sum of all the inter-frame distances
  float total_duration_sec = 0.0;
  for (const auto &delta_timestamp_sec : delta_timestamp_sec_list) {
    total_duration_sec += delta_timestamp_sec;
  }

  // 6. calculate frame drop ratio as extra drop length over total duration
  *frame_drop_ratio = drop_length_duration_sec / total_duration_sec;
  *frame_drop_count = int((*frame_drop_ratio) * (*num_video_frames));

  // 7. calculate average drop length, normalized to framerate. Note that
  // a single frame drop is a normalized frame drop length of 2. When
  // frame drops are uncorrelated, the normalized average drop length
  // should be close to 2
  *normalized_frame_drop_average_length = 0.0;
  if (drop_length_sec_list.size() > 0) {
    float frame_drop_average_length =
        drop_length_sec_list_sum / drop_length_sec_list.size();
    *normalized_frame_drop_average_length =
        (frame_drop_average_length / delta_timestamp_sec_median);
  }

  // 8. calculate percentile list
  frame_drop_length_percentile_list.clear();
  if (drop_length_sec_list.size() > 0) {
    sort(drop_length_sec_list.begin(), drop_length_sec_list.end());
    for (const float &percentile : percentile_list) {
      int position = (percentile / 100.0) * drop_length_sec_list.size();
      float frame_drop_length_percentile =
          drop_length_sec_list[position] / delta_timestamp_sec_median;
      frame_drop_length_percentile_list.push_back(frame_drop_length_percentile);
    }
  } else {
    for (const float &_ : percentile_list) {
      frame_drop_length_percentile_list.push_back(0.0);
    }
  }
  return 0;
}

int get_video_freeze_info(const char *infile, bool *video_freeze,
                          float *audio_video_ratio, float *duration_video_sec,
                          float *duration_audio_sec, int debug) {
  // 0. get timing information
  int num_video_frames;
  std::vector<float> delta_timestamp_sec_list;
  if (get_timing_information(infile, &num_video_frames, duration_video_sec,
                             duration_audio_sec, delta_timestamp_sec_list,
                             debug) < 0) {
    return -1;
  }

  // 1. check both audio and video tracks, and video track at least 2 seconds
  if (*duration_video_sec == -1.0) {
    fprintf(stderr, "error: no video track in %s\n", infile);
    return -1;
  }
  if (*duration_audio_sec == -1.0) {
    fprintf(stderr, "error: no audio track in %s\n", infile);
    return -1;
  }
  if (*duration_video_sec < 2.0) {
    fprintf(stderr, "error: video track too short %s (%f seconds)\n", infile,
            *duration_video_sec);
    return -1;
  }

  // 2. calculate audio to video ratio
  *audio_video_ratio =
      (*duration_audio_sec - *duration_video_sec) / *duration_audio_sec;
  *video_freeze = *audio_video_ratio > MAX_AUDIO_VIDEO_RATIO;
  return 0;
}
