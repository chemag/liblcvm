# liblcvm: A Low-Complexity, Video Metrics Library and Tool

By [Chema Gonzalez](https://github.com/chemag), 2024-07-18


# 1. Rationale

This document describes liblcvm, a low-complexity library to calculate
video metrics from ISOBMFF (e.g. mp4) media files.

The key for liblcvm is the low-complexity part, as we plan to use
it in devices with battery and thermal constrains.



# 2. Operation

1. Prerequisites

Following dependencies are required to install liblcvm and its submodules:
* cmake
* gtest-devel
* gmock-devel
* llvm-toolset (or clang-tools-extra)

Note: The requirement may vary depending on the OS, e.g., in Fedora clang-tidy came from clang-tools-extra

For linux enviroments, the following commands can be used to install the dependencies:
```
sudo dnf install cmake
sudo dnf install gtest-devel
sudo dnf install gmock-devel
sudo dnf install llvm-toolset
# or
sudo dnf install clang-tools-extra
```

2. Clone the repository.

```
$ git clone --recursive https://github.com/chemag/liblcvm
$ cd liblcvm
```

3. Build library and binary.
```
$ mkdir build
$ cd build
$ CC=gcc CXX=g++ cmake -DBUILD_CLANG_FUZZER=OFF ..
$ make
```

Notes:
* Replace the cmake line with the following one to build with clang and add
debug/gdb symbols:
```
$ CC=clang CXX=clang++ cmake -DCMAKE_BUILD_TYPE=DEBUG -DCMAKE_C_FLAGS_DEBUG="-g -O0" -DCMAKE_CXX_FLAGS_DEBUG="-g -O0" ..
```

* Replace the cmake line with the following one to build with clang, add
debug/gdb symbols, and add python (pybind11) bindings:
```
$ CC=clang CXX=clang++ cmake -DCMAKE_BUILD_TYPE=DEBUG -DCMAKE_C_FLAGS_DEBUG="-g -O0" -DCMAKE_CXX_FLAGS_DEBUG="-g -O0" -DBUILD_PYBINDINGS=ON ..
```

4. test the binary tool
```
./lcvm /tmp/test/*mp4 -o full.csv

$ csvlook -I full.csv
| infile          | num_video_frames | frame_rate_fps | video_freeze | video_freeze_ratio | frame_drop_ratio | normalized_frame_drop_average_length |
| --------------- | ---------------- | -------------- | ------------ | ------------------ | ---------------- | ------------------------------------ |
| /tmp/test/a.mp4 | 1807             | 30.010002      | 0            | -0.003015          | 0.001108         | 3.003668                             |
| /tmp/test/b.mp4 | 331              | 30.010002      | 0            | -0.008523          | 0.006015         | 3.003335                             |
| /tmp/test/c.mp4 | 570              | 29.910269      | 0            | -0.011072          | 0.124218         | 2.035057                             |
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
uint32_t timescale_video_hz = pts->get_timing()->get_timescale_video_hz();
```

As an alternative, and to keep backwards compatibility, we will keep for
a while the old API that returned a set of variables at the same time. These
include:
* `get_frame_drop_info()`
* `get_video_freeze_info()`
* `get_video_structure_info()`
* `get_video_generic_info()`
* `get_frame_interframe_info()`


# Appendix 1: Build Manually

1. Clone the repository
```
$ git clone --recursive http://github.com/chemag/liblcvm
$ cd liblcvm
```

2. build the binary tool (tools/lcvm)
```
$ make build
...
[ ISOBMFF ]> libISOBMFF.so [ Debug - x86_64 ]: Linking the x86_64 binary
make[1]: Leaving directory '/tmp/liblcvm/lib/isobmff'
LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:./lib/isobmff/Build/Debug/Products/x86_64/ g++ -c -o src/liblcvm.o -g -O0 src/liblcvm.cc -I./lib/isobmff/ISOBMFF/include/
LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:./lib/isobmff/Build/Debug/Products/x86_64/ g++ -c -o tools/lcvm.o -g -O0 tools/lcvm.cc -I./lib/isobmff/ISOBMFF/include/ -I./include
LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:./lib/isobmff/Build/Debug/Products/x86_64/ g++ -o tools/lcvm tools/lcvm.o src/liblcvm.o -g -O0 -L./lib/isobmff/Build/Debug/Products/x86_64/ -lISOBMFF
```


3. test the binary tool:
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


# Appendix 2: Build Troubleshoot

## A2.1. Mac Known Build Errors

```
clang: error: no such sysroot directory: '/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS17.5.sdk' [-Werror,-Wmissing-sysroot]
```
To resolve this issue, update the Xcode path in the Common.mk file: ```lib/isobmff/Submodules/makelib/Common.mk +75```
