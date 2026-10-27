// liblcvm_parse_fuzzer: libFuzzer harness for the full liblcvm parse path.
//
// It exercises IsobmffFileInformation::parse_to_lists(): ISOBMFF parsing,
// timing and keyframe derivation, AVC/HEVC parameter-set parsing, audio
// parsing, and list generation, plus the value conversion helpers.
//
// Input layout: byte 0 selects the configuration, the rest is the file.
//   bit 0: sort_by_pts
//   bit 1: skip_audio_parsing
//   bit 2: calculate_timestamps (per-frame timing lists)
//
// parse() takes a path, so the input is written to an in-memory file
// (memfd) and opened through /proc/self/fd/<fd>.

#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "liblcvm.h"

static int g_fd = -1;
static char g_path[64];

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv) {
  g_fd = memfd_create("liblcvm_fuzz", 0);
  if (g_fd < 0) {
    perror("memfd_create");
    abort();
  }
  snprintf(g_path, sizeof(g_path), "/proc/self/fd/%d", g_fd);
  return 0;
}

static bool write_input(const uint8_t* data, size_t size) {
  if (ftruncate(g_fd, 0) != 0) {
    return false;
  }
  if (lseek(g_fd, 0, SEEK_SET) != 0) {
    return false;
  }
  size_t done = 0;
  while (done < size) {
    ssize_t n = write(g_fd, data + done, size - done);
    if (n <= 0) {
      return false;
    }
    done += static_cast<size_t>(n);
  }
  return true;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size < 1) {
    return 0;
  }
  const uint8_t flags = data[0];
  data += 1;
  size -= 1;
  if (!write_input(data, size)) {
    abort();
  }

  LiblcvmConfig config;
  config.set_sort_by_pts((flags & 0x01) != 0);
  config.set_skip_audio_parsing((flags & 0x02) != 0);
  config.set_debug(0);
  const bool calculate_timestamps = (flags & 0x04) != 0;

  LiblcvmKeyList keys;
  LiblcvmValList vals;
  LiblcvmKeyList keys_timing;
  LiblcvmTimingList vals_timing;
  IsobmffFileInformation::parse_to_lists(g_path, config, &keys, &vals,
                                         calculate_timestamps, &keys_timing,
                                         &vals_timing);
  for (const auto& v : vals) {
    std::string s;
    (void)liblcvmvalue_to_string(v, &s);
    double d;
    (void)liblcvmvalue_to_double(v, &d);
  }
  return 0;
}
