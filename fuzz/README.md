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

## 2.1. Get the Corpus

The corpus lives in the
[liblcvm-fuzzcorpus](https://github.com/chemag/liblcvm-fuzzcorpus) repo, as
the `fuzz/corpus` submodule. The submodule is not checked out by default, not
even by `git clone --recursive`, which gets the isobmff, h264nal, and h265nal
libraries but skips every corpus. Request it explicitly:

```
$ cd liblcvm
$ git submodule update --init --checkout --depth 1 -- fuzz/corpus
```

The libraries' corpora work the same way, for example:

```
$ git -C lib/h265nal submodule update --init --checkout --depth 1 -- fuzz/corpus
```

## 2.2. Build

`BUILD_CLANG_FUZZER` requires clang. It instruments liblcvm and the isobmff,
h264nal, and h265nal libraries with ASan, UBSan (fatal), and libFuzzer
coverage, and builds the harness:

```
$ mkdir build && cd build
$ CC=clang CXX=clang++ cmake -DBUILD_CLANG_FUZZER=ON -DBUILD_TESTS=OFF \
    -DBUILD_H264_TESTS=OFF -DBUILD_H265_TESTS=OFF ..
$ make liblcvm_parse_fuzzer
```

## 2.3. Run

`make fuzz` runs the fuzzer for `RUNS` executions (a CMake cache variable,
default 100) on `fuzz/corpus/liblcvm_parse_fuzzer/`, adds the new
coverage-increasing inputs there, and writes failing inputs to
`fuzz/artifacts/liblcvm_parse_fuzzer/` in the build directory. Without the
submodule, it starts from an empty corpus in the build directory.

```
$ cmake -DRUNS=1000000 ..
$ make fuzz
```

To run the fuzzer directly:

```
$ mkdir -p artifacts
$ ./fuzz/liblcvm_parse_fuzzer -artifact_prefix=artifacts/ \
    ../fuzz/corpus/liblcvm_parse_fuzzer/
```

Keep failing inputs (`artifacts/`) apart from the corpus: libFuzzer replays
every corpus file at startup, so a crash saved into the corpus stops every
later run.
