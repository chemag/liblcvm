# liblcvm: A Low-Complexity, Video Metrics Library and Tool

By [Chema Gonzalez](https://github.com/chemag), 2024-07-18


# 1. Rationale

This document describes liblcvm, a low-complexity library to calculate
video metrics from ISOBMFF (e.g. mp4) media files.

The key for liblcvm is the low-complexity part, as we plan to use
it in devices with battery and thermal constrains.



# 2. Operation


1. Clone the repository.

```
$ git clone --recursive https://github.com/chemag/liblcvm
$ cd liblcvm
```

2. Build library and binary.
```
$ mkdir build
$ cd build
$ CC=gcc CXX=g++ cmake -DBUILD_CLANG_FUZZER=OFF ..
$ make
```

3. test the binary tool
```
$ ./lcvm /tmp/test/*mp4 -o full.csv

$ csvlook -I full.csv
| infile          | num_video_frames | frame_rate_fps | video_freeze | video_freeze_ratio | frame_drop_ratio | normalized_frame_drop_average_length |
| --------------- | ---------------- | -------------- | ------------ | ------------------ | ---------------- | ------------------------------------ |
| /tmp/test/a.mp4 | 1807             | 30.010002      | 0            | -0.003015          | 0.001108         | 3.003668                             |
| /tmp/test/b.mp4 | 331              | 30.010002      | 0            | -0.008523          | 0.006015         | 3.003335                             |
| /tmp/test/c.mp4 | 570              | 29.910269      | 0            | -0.011072          | 0.124218         | 2.035057                             |
```

Notes:
* (a) Replace gcc with clang by re-running the cmake line as follows:
```
$ CC=clang CXX=clang++ cmake ..
```

* (b) Replace gcc with clang and add debug/gdb symbols:
```
$ CC=clang CXX=clang++ cmake -DCMAKE_BUILD_TYPE=DEBUG -DCMAKE_C_FLAGS_DEBUG="-g -O0" -DCMAKE_CXX_FLAGS_DEBUG="-g -O0" ..
```

* (c) Replace gcc with clang, add debug/gdb symbols, and add python (pybind11)
bindings:
```
$ CC=clang CXX=clang++ cmake -DCMAKE_BUILD_TYPE=DEBUG -DCMAKE_C_FLAGS_DEBUG="-g -O0" -DCMAKE_CXX_FLAGS_DEBUG="-g -O0" -DBUILD_PYBINDINGS=ON ..
```


# 3. Technical Discussion

liblcvm uses the [ISOBMFF](https://github.com/DigiDNA/ISOBMFF) library to
parse a series of boxes in ISOBMFF files. More concretely, it gets the
following information:

* `/moov/trak/mdia/hdlr`: box to get whether a track is audio or video.
* `/moov/trak/mdia/mdhd`: box to get track timescale and duration.
* `/moov/trak/mdia/minf/stbl/stts`: box to get inter-sample distance.
* `/moov/trak/mdia/minf/stbl/stss`: box to get keyframe information.

Since v0.2, liblcvm also uses the [h264nal](https://github.com/chemag/h264nal)
and [h265nal](https://github.com/chemag/h265nal) libraries to peek inside
the SPS VUI in the actual media boxes, and get some information (e.g.
colorimetry).



# 4. Library Operation

The library is based on a few classes. The main one is IsobmffFileInformation,
which contains a factory method ("parse()") which does the full analysis. The
"parse()" method expects a configuration object.

```
// create the config object
auto *liblcvm_config = new LiblcvmConfig();
// init any parameters where you do not like the default
liblcvm_config->set_debug(debug);
// parse the file
std::shared_ptr<IsobmffFileInformation> ptr =
    IsobmffFileInformation::parse(infile.c_str(), *liblcvm_config);
```

The output IsobmffFileInformation pointer can be used directly through its
getters. For example, to get the video timescale, use the getter under the
TimingInformation class:

```
uint32_t timescale_movie_hz = pts->get_timing()->get_timescale_movie_hz();
uint32_t timescale_video_hz = pts->get_timing()->get_timescale_video_hz();
```

Note: `timescale_movie_hz` is the movie-level timescale from the mvhd box,
while `timescale_video_hz` is the track-level timescale from the mdhd box.

## 4.1. Full Track Access

`IsobmffFileInformation` owns one `video_tracks` collection and one
`audio_tracks` collection. Each `VideoTrack` or `AudioTrack` owns both its
metadata and its `TimingInformation`; there is no separate metadata wrapper.

`get_video_track(id)` and `get_audio_track(id)` return borrowed const pointers.
`-1` selects the first record in container order. Missing or invalid IDs return
`nullptr`; they never select another record. Pointers require their owning
storage to remain alive: Owner assignment invalidates them, and moves transfer
storage ownership. The old information classes/getters have no aliases.

Use a track's `get_timing()` const reference for intrinsic timing. The file's
`get_timing(audio_id, video_id)` still returns a selected AV view, with audio
first for compatibility. AV ratios are derived for that pair rather than
stored as an audio association in the video track. Python maps absent pointers
to `None` and retains the owner for borrowed track/timing objects.

This patch does not redesign the timing parser, derivation, or codec policy.

## 4.2. Shared Timing Reader and Derivation

Video and audio use the same owner-independent
`TimingInformation::parse_timing_information(stbl, timescale_hz,
max_num_frames, per_frame_info, filename, debug)` reader. It stores compact
STTS/CTTS values and optional raw sync-sample values, with no expanded
DTS/PTS arrays or frame-drop calculations. Successful reads replace prior
source/header context; failed reads leave the prior result intact.

`derive_timing_info(sort_by_pts, filename, debug)` computes sample counts,
STTS tick totals, and durations. With `per_frame_info=true`, it also expands
and orders frame timing and computes cadence, keyframes, and drops. Video
uses true; audio uses false and retains only scalar summaries. Header-only
scalar audio descriptions remain supported when no STTS table is present.

Track header values are attached after the read as raw MDHD/TKHD duration
units. Derivation preserves TKHD priority when the movie timescale is known,
and falls back to MDHD otherwise. Intrinsic timing uses `get_duration_sec()`
and `get_timescale_hz()`; audio/video-specific getters exist only on the
separate `AudioVideoTimingInformation` view returned by file `get_timing()`.
That view does not expose the track reader/deriver through its normal API;
mutable base-reference casts are outside this snapshot contract. `get_stts_duration_sec()` is the decode-table
summary, not a replacement for the presentation duration.

`skip_audio_parsing` bypasses every audio track; it is not scalar-only mode.
No audio records or AV fields are produced for skipped tracks. The lower
ISOBMFF parser still validates container/box syntax before track analysis.

As an alternative, and to keep backwards compatibility, we will keep for
a while the old API that returned a set of variables at the same time. These
include:
* `get_frame_drop_info()`
* `get_video_freeze_info()`
* `get_video_structure_info()`
* `get_video_generic_info()`
* `get_frame_interframe_info()`



# Appendix 1: Prerequisites

## A1.1: Linux Prerequisites
The following dependencies are required to install liblcvm and its submodules:
* cmake
* gtest-devel
* gmock-devel
* llvm-toolset (clang-tools-extra in some distros)

Note: The requirement may vary depending on the OS, e.g., in Fedora clang-tidy came from clang-tools-extra.

For linux enviroments, the following commands can be used to install the dependencies:
```
sudo dnf install cmake
sudo dnf install gtest-devel
sudo dnf install gmock-devel
sudo dnf install llvm-toolset
sudo dnf install abseil-cpp-devel
# or
sudo dnf install clang-tidy  # ubuntu
sudo dnf install clang-tools-extra  # fedora
```


## A1.2: MacOS Prerequisites
The following packages can be installed using brew.

```
$ brew install llvm cmake googletest git clang-format
```

You also need to install Xcode.


# Appendix 2: Test the Binary Tool Manually

```
$ TESTDIR=/tmp/test make test
LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:./lib/isobmff/Build/Debug/Products/x86_64/ ./tools/lcvm /tmp/test/*mp4 -o /tmp/full.csv

$ csvlook -I /tmp/full.csv
| infile          | num_video_frames | frame_rate_fps | video_freeze | video_freeze_ratio | frame_drop_ratio | normalized_frame_drop_average_length |
| --------------- | ---------------- | -------------- | ------------ | ------------------ | ---------------- | ------------------------------------ |
| /tmp/test/a.mp4 | 1807             | 30.010002      | 0            | -0.003015          | 0.001108         | 3.003668                             |
| /tmp/test/b.mp4 | 331              | 30.010002      | 0            | -0.008523          | 0.006015         | 3.003335                             |
| /tmp/test/c.mp4 | 570              | 29.910269      | 0            | -0.011072          | 0.124218         | 2.035057                             |
```


# Appendix 3: Build Troubleshooting

## A3.1. Mac Known Build Errors

```
clang: error: no such sysroot directory: '/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS17.5.sdk' [-Werror,-Wmissing-sysroot]
```

To resolve this issue, update the Xcode path in the Common.mk file:

```
lib/isobmff/Submodules/makelib/Common.mk +75
```


# Appendix 4: autogen notes

* The antlr files were generated using antlr-4.13.1-complete.jar.


# Appendix 5: Additional Build Notes

Build library and binary with policy mode and debug enabled, but disabling fuzzing.
```
$ mkdir build
$ cd build
$ cmake -DBUILD_CLANG_FUZZER=OFF -DADD_POLICY=ON -DCMAKE_BUILD_TYPE=DEBUG ..
$ make
```
