---
type: readme
title: "liblcvm Fuzzing"
description: "How to build and run the liblcvm libFuzzer harness"
tags: [liblcvm, fuzzing, libfuzzer]
author: "Chema Gonzalez"
generated_by: "Metamate"
date: "2026-10-27"
---

# 1. Harness

liblcvm has one libFuzzer harness, `liblcvm_parse_fuzzer`, that runs the full
parse path (`IsobmffFileInformation::parse_to_lists()`): ISOBMFF parsing,
timing and keyframe derivation, AVC/HEVC parameter-set parsing, audio
parsing, and list generation. The isobmff, h264nal, and h265nal libraries
have their own per-parser fuzzers in their `fuzz/` directories.

Input layout:

* Byte 0 selects the parse options: bit 0 is `sort_by_pts`, bit 1 is
  `skip_audio_parsing`, and bit 2 is `calculate_timestamps` (per-frame timing
  lists).
* The rest of the input is the ISOBMFF file.

# 2. Operation

## 2.1. Build

`BUILD_CLANG_FUZZER` requires clang. It instruments liblcvm and the isobmff,
h264nal, and h265nal libraries with ASan, UBSan (fatal), and libFuzzer
coverage, and builds the harness:

```
$ mkdir build && cd build
$ CC=clang CXX=clang++ cmake -DBUILD_CLANG_FUZZER=ON -DBUILD_TESTS=OFF \
    -DBUILD_H264_TESTS=OFF -DBUILD_H265_TESTS=OFF ..
$ make liblcvm_parse_fuzzer
```

## 2.2. Run

```
$ mkdir -p corpus artifacts
$ ./fuzz/liblcvm_parse_fuzzer -artifact_prefix=artifacts/ corpus/
```

Keep failing inputs (`artifacts/`) apart from the corpus: libFuzzer replays
every corpus file at startup, so a crash saved into the corpus stops every
later run.
