/*
 *  Copyright (c) Meta Platforms, Inc. and its affiliates.
 */

/*
 * Memory usage and leak detection tests for liblcvm video processing.
 *
 * This test suite measures memory consumption when processing multiple videos
 * with liblcvm. It provides three types of tests:
 *
 * 1. MultipleVideosMemoryUsage: Processes all videos in test/corpus/ sequentially
 *    and tracks memory usage after each video to ensure memory doesn't grow
 *    excessively.
 *
 * 2. SingleVideoMemoryLeakCheck: Processes the same video multiple times (10
 *    iterations) to detect memory leaks. If memory continuously grows with each
 *    iteration, it indicates a leak.
 *
 * 3. DetailedMemoryProfileMassifStyle: Creates a timeline of memory snapshots
 *    before and after each video processing, similar to Valgrind Massif. Outputs
 *    both console timeline and a memory_profile.txt file.
 *
 * Memory Measurement:
 * - Uses RSS (Resident Set Size) on both macOS and Linux
 * - RSS represents the actual physical memory used by the process
 * - This is complementary to tools like Valgrind Massif which track heap
 *   allocations with detailed stack traces
 *
 * Usage:
 *   # Run all memory tests
 *   ./liblcvm_memory_test
 *
 *   # Run specific test
 *   ./liblcvm_memory_test --gtest_filter=MemoryUsageTest.SingleVideoMemoryLeakCheck
 *
 * Setup:
 *   Place test videos in test/corpus/ directory
 *   Supported formats: .MOV, .mov, .MP4, .mp4, .m4v, .M4V
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

// Test fixture for memory usage tests
class MemoryUsageTest : public ::testing::Test {
 public:
  MemoryUsageTest() {}
  ~MemoryUsageTest() override {}

  // Gets current memory usage (RSS - Resident Set Size) in bytes
  // Returns the actual physical memory currently used by the process
  // Platform-specific implementations for macOS and Linux
  size_t getCurrentMemoryUsageBytes() {
#if defined(__APPLE__) && defined(__MACH__)
    // macOS: Use Mach task_info API to get resident memory size
    struct mach_task_basic_info info;
    mach_msg_type_number_t info_count = MACH_TASK_BASIC_INFO_COUNT;

    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info,
                  &info_count) != KERN_SUCCESS) {
      return 0;
    }
    return info.resident_size;
#elif defined(__linux__)
    // Linux: Read RSS from /proc/self/statm
    // Second field is RSS in pages, multiply by page size for bytes
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

  // Converts bytes to megabytes for human-readable output
  double bytesToMB(size_t bytes) {
    return static_cast<double>(bytes) / (1024.0 * 1024.0);
  }

  // Represents a single point-in-time memory measurement
  // Used for creating timeline-based memory profiles
  struct MemorySnapshot {
    size_t timestamp_ms;     // Milliseconds since test start
    size_t memory_bytes;     // Memory usage at this point
    std::string event;       // Description of what happened
  };

  // Writes memory snapshots to a file in a Massif-like format
  // Output file can be analyzed to identify memory patterns over time
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

  // Scans test/corpus/ directory for video files to test with
  // Returns a list of full paths to all supported video files
  // Supported formats: .MOV, .mov, .MP4, .mp4, .m4v, .M4V
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

  // Processes a single video file using liblcvm
  // This is the core operation being tested for memory usage
  // Parses the video and extracts metrics and timing information
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

// Test 1: MultipleVideosMemoryUsage
// Purpose: Measures memory usage when processing multiple videos sequentially
// What it checks:
// - Initial memory footprint
// - Memory delta after processing each video
// - Peak memory usage across all videos
// - Total memory increase should be < 100 MB (configurable threshold)
// Expected behavior:
// - First video may increase memory (allocations for data structures)
// - Subsequent videos should show minimal memory increase
// - Memory should not continuously grow with each video
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

// Test 2: SingleVideoMemoryLeakCheck
// Purpose: Detects memory leaks by processing the same video multiple times
// What it checks:
// - Processes the same video 10 times (configurable)
// - Measures memory after each iteration
// - Calculates memory increase per iteration
// - Fails if memory grows > 1 MB per iteration (configurable threshold)
// Expected behavior:
// - First iteration may increase memory (initial allocations)
// - Subsequent iterations should show minimal or no memory increase
// - If memory keeps growing linearly, it indicates a leak
// Example leak pattern:
//   Iteration 1: 50 MB → 60 MB → 70 MB → 80 MB (LEAK!)
// Example healthy pattern:
//   Iteration 1: 50 MB → 52 MB → 52 MB → 52 MB (OK)
TEST_F(MemoryUsageTest, SingleVideoMemoryLeakCheck) {
  std::vector<std::string> videos = getTestVideos();

  if (videos.empty()) {
    GTEST_SKIP() << "No test videos found in corpus directory. "
                 << "Please add videos to test/corpus/";
  }

  std::string test_video = videos[0];
  int iterations = 10;  // Process the same video 10 times

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

    // Print every 5th iteration plus the first one
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

  // Threshold: If memory increases by more than 1 MB per iteration, fail
  double leak_threshold_per_iteration_mb = 1.0;
  double increase_per_iteration = bytesToMB(total_increase) / iterations;

  EXPECT_LT(increase_per_iteration, leak_threshold_per_iteration_mb)
      << "Potential memory leak detected: " << increase_per_iteration
      << " MB per iteration (threshold: " << leak_threshold_per_iteration_mb
      << " MB)";
}

// Test 3: DetailedMemoryProfileMassifStyle
// Purpose: Creates a timeline of memory usage similar to Valgrind Massif
// What it does:
// - Records memory snapshots before and after each video processing
// - Creates a timeline with timestamps (milliseconds since test start)
// - Outputs both to console and memory_profile.txt file
// - Identifies peak memory usage and when it occurred
// Use cases:
// - Understanding memory patterns over time
// - Identifying which video causes the most memory increase
// - Detecting when memory is released (or not released)
// Output format:
//   Time(ms)  Memory(MB)  Event
//   0         45.23       Test Start
//   100       78.45       After processing video1.mp4
//   250       82.11       After processing video2.mov
TEST_F(MemoryUsageTest, DetailedMemoryProfileMassifStyle) {
  std::vector<std::string> videos = getTestVideos();

  if (videos.empty()) {
    GTEST_SKIP() << "No test videos found in corpus directory. "
                 << "Please add videos to test/corpus/";
  }

  std::cout << "\n=== Detailed Memory Profile (Massif-style) ===" << std::endl;

  std::vector<MemorySnapshot> snapshots;
  auto start_time = std::chrono::steady_clock::now();

  // Lambda to capture memory snapshots with timestamps
  auto recordSnapshot = [&](const std::string& event) {
    auto now = std::chrono::steady_clock::now();
    auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time)
            .count();
    size_t memory = getCurrentMemoryUsageBytes();
    snapshots.push_back({static_cast<size_t>(elapsed), memory, event});
  };

  recordSnapshot("Test Start");

  // Process up to 5 videos (to keep output manageable)
  for (size_t i = 0; i < videos.size() && i < 5; i++) {
    std::string video_name =
        std::filesystem::path(videos[i]).filename().string();

    recordSnapshot("Before processing " + video_name);
    processVideo(videos[i]);
    recordSnapshot("After processing " + video_name);
  }

  recordSnapshot("Test End");

  // Print timeline to console
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

  // Write timeline to file for later analysis
  std::string report_filename = "memory_profile.txt";
  writeMemoryReport(snapshots, report_filename);
}
}  // namespace liblcvm
