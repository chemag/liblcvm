/*
 *  Copyright (c) Meta Platforms, Inc. and its affiliates.
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <liblcvm.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#if defined(__APPLE__) && defined(__MACH__)
#include <mach/mach.h>
#elif defined(__linux__)
#include <fstream>
#include <string>
#endif

namespace liblcvm {

class MemoryUsageTest : public ::testing::Test {
 public:
  MemoryUsageTest() {}
  ~MemoryUsageTest() override {}

  size_t getCurrentMemoryUsageBytes() {
#if defined(__APPLE__) && defined(__MACH__)
    struct mach_task_basic_info info;
    mach_msg_type_number_t info_count = MACH_TASK_BASIC_INFO_COUNT;

    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info,
                  &info_count) != KERN_SUCCESS) {
      return 0;
    }
    return info.resident_size;
#elif defined(__linux__)
    long rss = 0L;
    FILE* fp = nullptr;
    if ((fp = fopen("/proc/self/statm", "r")) == nullptr) {
      return 0;
    }
    if (fscanf(fp, "%*s%ld", &rss) != 1) {
      fclose(fp);
      return 0;
    }
    fclose(fp);
    return rss * sysconf(_SC_PAGESIZE);
#else
    return 0;
#endif
  }

  double bytesToMB(size_t bytes) {
    return static_cast<double>(bytes) / (1024.0 * 1024.0);
  }

  struct MemorySnapshot {
    size_t timestamp_ms;
    size_t memory_bytes;
    std::string event;
  };

  void writeMemoryReport(const std::vector<MemorySnapshot>& snapshots,
                         const std::string& filename) {
    std::ofstream report(filename);
    if (!report.is_open()) {
      std::cerr << "Failed to open " << filename << " for writing" << std::endl;
      return;
    }

    report << "# Memory Usage Report (Massif-like format)\n";
    report << "# Time(ms)\tMemory(MB)\tEvent\n";
    report << "#-----------------------------------------\n";

    for (const auto& snapshot : snapshots) {
      report << snapshot.timestamp_ms << "\t"
             << bytesToMB(snapshot.memory_bytes) << "\t" << snapshot.event
             << "\n";
    }

    report.close();
    std::cout << "Memory report written to: " << filename << std::endl;
  }

  std::vector<std::string> getTestVideos() {
    std::vector<std::string> videos;
    std::string corpus_dir = std::string(TEST_CONFORMANCE_DIR) + "/corpus";

    if (!std::filesystem::exists(corpus_dir)) {
      return videos;
    }

    for (const auto& entry : std::filesystem::directory_iterator(corpus_dir)) {
      if (entry.is_regular_file()) {
        std::string ext = entry.path().extension().string();
        if (ext == ".MOV" || ext == ".mov" || ext == ".MP4" || ext == ".mp4" ||
            ext == ".m4v" || ext == ".M4V") {
          videos.push_back(entry.path().string());
        }
      }
    }
    return videos;
  }

  void processVideo(const std::string& infile) {
    auto liblcvm_config = std::make_unique<LiblcvmConfig>();
    liblcvm_config->set_sort_by_pts(true);
    liblcvm_config->set_debug(0);

    LiblcvmKeyList keys;
    LiblcvmValList vals;
    LiblcvmKeyList keys_timing;
    LiblcvmTimingList vals_timing;
    bool calculate_timestamps = true;

    int result = IsobmffFileInformation::parse_to_lists(
        infile.c_str(), *liblcvm_config, &keys, &vals, calculate_timestamps,
        &keys_timing, &vals_timing);

    ASSERT_EQ(0, result) << "Failed to parse video: " << infile;
  }
};

TEST_F(MemoryUsageTest, MultipleVideosMemoryUsage) {
  std::vector<std::string> videos = getTestVideos();

  if (videos.empty()) {
    GTEST_SKIP() << "No test videos found in corpus directory. "
                 << "Please add videos to test/corpus/";
  }

  std::cout << "\n=== Memory Usage Test ===" << std::endl;
  std::cout << "Testing with " << videos.size() << " video(s)" << std::endl;

  size_t initial_memory = getCurrentMemoryUsageBytes();
  std::cout << "Initial memory: " << bytesToMB(initial_memory) << " MB"
            << std::endl;

  size_t peak_memory = initial_memory;
  size_t memory_before = initial_memory;

  for (size_t i = 0; i < videos.size(); i++) {
    std::cout << "\nProcessing video " << (i + 1) << "/" << videos.size()
              << ": " << std::filesystem::path(videos[i]).filename().string()
              << std::endl;

    processVideo(videos[i]);

    size_t memory_after = getCurrentMemoryUsageBytes();
    size_t memory_delta = memory_after - memory_before;

    std::cout << "  Memory after: " << bytesToMB(memory_after) << " MB"
              << std::endl;
    std::cout << "  Delta: " << (memory_delta > 0 ? "+" : "")
              << bytesToMB(memory_delta) << " MB" << std::endl;

    peak_memory = std::max(peak_memory, memory_after);
    memory_before = memory_after;
  }

  size_t final_memory = getCurrentMemoryUsageBytes();
  size_t total_increase = final_memory - initial_memory;

  std::cout << "\n=== Summary ===" << std::endl;
  std::cout << "Initial memory: " << bytesToMB(initial_memory) << " MB"
            << std::endl;
  std::cout << "Final memory: " << bytesToMB(final_memory) << " MB"
            << std::endl;
  std::cout << "Peak memory: " << bytesToMB(peak_memory) << " MB" << std::endl;
  std::cout << "Total increase: " << bytesToMB(total_increase) << " MB"
            << std::endl;
  std::cout << "Average per video: "
            << bytesToMB(total_increase / videos.size()) << " MB" << std::endl;

  double memory_increase_threshold_mb = 100.0;
  EXPECT_LT(bytesToMB(total_increase), memory_increase_threshold_mb)
      << "Memory usage increased by more than " << memory_increase_threshold_mb
      << " MB after processing " << videos.size() << " video(s)";
}

TEST_F(MemoryUsageTest, SingleVideoMemoryLeakCheck) {
  std::vector<std::string> videos = getTestVideos();

  if (videos.empty()) {
    GTEST_SKIP() << "No test videos found in corpus directory. "
                 << "Please add videos to test/corpus/";
  }

  std::string test_video = videos[0];
  int iterations = 10;

  std::cout << "\n=== Memory Leak Test ===" << std::endl;
  std::cout << "Processing "
            << std::filesystem::path(test_video).filename().string() << " "
            << iterations << " times" << std::endl;

  size_t initial_memory = getCurrentMemoryUsageBytes();
  std::cout << "Initial memory: " << bytesToMB(initial_memory) << " MB"
            << std::endl;

  std::vector<size_t> memory_samples;

  for (int i = 0; i < iterations; i++) {
    processVideo(test_video);

    size_t current_memory = getCurrentMemoryUsageBytes();
    memory_samples.push_back(current_memory);

    if ((i + 1) % 5 == 0 || i == 0) {
      std::cout << "Iteration " << (i + 1) << ": " << bytesToMB(current_memory)
                << " MB" << std::endl;
    }
  }

  size_t final_memory = getCurrentMemoryUsageBytes();
  size_t total_increase = final_memory - initial_memory;

  std::cout << "\n=== Summary ===" << std::endl;
  std::cout << "Initial memory: " << bytesToMB(initial_memory) << " MB"
            << std::endl;
  std::cout << "Final memory: " << bytesToMB(final_memory) << " MB"
            << std::endl;
  std::cout << "Total increase: " << bytesToMB(total_increase) << " MB"
            << std::endl;
  std::cout << "Increase per iteration: "
            << bytesToMB(total_increase / iterations) << " MB" << std::endl;

  double leak_threshold_per_iteration_mb = 1.0;
  double increase_per_iteration = bytesToMB(total_increase) / iterations;

  EXPECT_LT(increase_per_iteration, leak_threshold_per_iteration_mb)
      << "Potential memory leak detected: " << increase_per_iteration
      << " MB per iteration (threshold: " << leak_threshold_per_iteration_mb
      << " MB)";
}

TEST_F(MemoryUsageTest, DetailedMemoryProfileMassifStyle) {
  std::vector<std::string> videos = getTestVideos();

  if (videos.empty()) {
    GTEST_SKIP() << "No test videos found in corpus directory. "
                 << "Please add videos to test/corpus/";
  }

  std::cout << "\n=== Detailed Memory Profile (Massif-style) ===" << std::endl;

  std::vector<MemorySnapshot> snapshots;
  auto start_time = std::chrono::steady_clock::now();

  auto recordSnapshot = [&](const std::string& event) {
    auto now = std::chrono::steady_clock::now();
    auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time)
            .count();
    size_t memory = getCurrentMemoryUsageBytes();
    snapshots.push_back({static_cast<size_t>(elapsed), memory, event});
  };

  recordSnapshot("Test Start");

  for (size_t i = 0; i < videos.size() && i < 5; i++) {
    std::string video_name =
        std::filesystem::path(videos[i]).filename().string();

    recordSnapshot("Before processing " + video_name);
    processVideo(videos[i]);
    recordSnapshot("After processing " + video_name);
  }

  recordSnapshot("Test End");

  std::cout << "\n=== Timeline ===" << std::endl;
  std::cout << "Time(ms)\tMemory(MB)\tEvent" << std::endl;
  std::cout << "------------------------------------------------" << std::endl;

  size_t peak_memory = 0;
  size_t peak_time = 0;

  for (const auto& snapshot : snapshots) {
    std::cout << snapshot.timestamp_ms << "\t\t" << std::fixed
              << std::setprecision(2) << bytesToMB(snapshot.memory_bytes)
              << "\t\t" << snapshot.event << std::endl;

    if (snapshot.memory_bytes > peak_memory) {
      peak_memory = snapshot.memory_bytes;
      peak_time = snapshot.timestamp_ms;
    }
  }

  std::cout << "\n=== Peak Memory ===" << std::endl;
  std::cout << "Peak: " << bytesToMB(peak_memory) << " MB at " << peak_time
            << " ms" << std::endl;

  std::string report_filename = "memory_profile.txt";
  writeMemoryReport(snapshots, report_filename);
}

}  // namespace liblcvm
