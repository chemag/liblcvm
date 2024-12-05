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
$ git clone --recursive http://github.com/chemag/liblcvm
$ cd liblcvm
```

2. Build library and binary.
```
$ mkdir build
$ cd build
$ cmake ..
$ make
```

3. test the binary tool
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


# Appendix 1: Build Manually

1. Clone the repository
```
$ git clone --recursive http://github.com/chemag/liblcvm
$ cd liblcvm
```

2. fix the ISOBMFF Makefile (needed for Fedora)
```
$ git diff
diff --git a/Makefile b/Makefile
index 63d90c7..ace112e 100644
--- a/Makefile
+++ b/Makefile
@@ -45,14 +45,14 @@ EXT_CPP             := .cpp
 EXT_M               := .m
 EXT_MM              := .mm
 EXT_H               := .hpp
-CC                  := clang
+CC                  := g++
 FLAGS_OPTIM         := Os
 FLAGS_WARN          := -Werror -Wall
 FLAGS_STD_C         := c99
 FLAGS_STD_CPP       := c++14
 FLAGS_OTHER         := -fno-strict-aliasing
 FLAGS_C             :=
-FLAGS_CPP           := -x c++ -stdlib=libc++
+FLAGS_CPP           :=
 FLAGS_M             := -fobjc-arc
 FLAGS_MM            := -fobjc-arc
 XCODE_PROJECT       := ISOBMFF.xcodeproj
@@ -70,7 +70,7 @@ FILES_M_EXCLUDE     :=
 FILES_MM            :=
 FILES_MM_EXCLUDE    :=

-LIBS                := -lc++
+LIBS                :=

 FILES               := $(filter-out $(FILES_C_EXCLUDE),$(FILES_C))      \
                        $(filter-out $(FILES_CPP_EXCLUDE),$(FILES_CPP))  \
```

3. build the binary tool (tools/lcvm)
```
$ make build
...
[ ISOBMFF ]> libISOBMFF.so [ Debug - x86_64 ]: Linking the x86_64 binary
make[1]: Leaving directory '/tmp/liblcvm/lib/isobmff'
LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:./lib/isobmff/Build/Debug/Products/x86_64/ g++ -c -o src/liblcvm.o -g -O0 src/liblcvm.cc -I./lib/isobmff/ISOBMFF/include/
LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:./lib/isobmff/Build/Debug/Products/x86_64/ g++ -c -o tools/lcvm.o -g -O0 tools/lcvm.cc -I./lib/isobmff/ISOBMFF/include/ -I./include
LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:./lib/isobmff/Build/Debug/Products/x86_64/ g++ -o tools/lcvm tools/lcvm.o src/liblcvm.o -g -O0 -L./lib/isobmff/Build/Debug/Products/x86_64/ -lISOBMFF
```


4. test the binary tool:
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


