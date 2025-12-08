# CLAUDE.md - liblcvm Analysis

## Project Overview

**liblcvm** is a Low-Complexity Video Metrics Library and Tool designed to analyze video files (specifically ISOBMFF/MP4 containers) for performance metrics with a focus on battery and thermal constrained devices. The library detects frame drops, video freezes, and calculates various video timing and quality metrics.

## Architecture

### Core Components

#### 1. Main Library (`src/liblcvm.cc`, `include/liblcvm.h`)
The core library providing the main API with several key classes:

- **`IsobmffFileInformation`**: Main class with factory method `parse()` for full video analysis
- **`TimingInformation`**: Handles frame timing, PTS/DTS calculations, frame rate analysis, and frame drop detection
- **`FrameInformation`**: Manages video codec information (H.264/H.265), resolution, bitrate, and colorimetry
- **`AudioInformation`**: Processes audio track metadata
- **`LiblcvmConfig`**: Configuration object for parsing parameters

#### 2. Command Line Tool (`tools/lcvm.cc`)
A comprehensive CLI tool that:
- Accepts multiple input files
- Outputs CSV format with video metrics
- Supports policy-based validation
- Provides detailed timing information export
- Handles multiple analysis runs for performance testing

#### 3. Policy System
Advanced rule-based validation system with:
- **Grammar Definition** (`policy/rules.g4`): ANTLR4 grammar for policy language
- **Protocol Buffers** (`policy/rules.proto`): Structured representation of policy rules
- **Policy Runner** (`src/policy_runner.cc`): Executes validation rules
- **Proto Visitor** (`src/policy_protovisitor.cc`): Converts ANTLR parse tree to protobuf

### Policy Language Features
The policy system supports:
- Comparison operators: `==`, `!=`, `<`, `>`, `<=`, `>=`
- Logical operators: `and`, `or`, `not`
- Range checks: `variable in range(min, max)`
- Warning and error severity levels
- String and numeric value comparisons

## Dependencies

### Core Dependencies
- **ISOBMFF Library**: Parses ISO Base Media File Format containers
- **h264nal/h265nal**: Video codec analysis libraries for SPS/VUI parsing
- **Protocol Buffers**: Structured data serialization for policy rules
- **ANTLR4**: Parser generator for policy language grammar
- **Abseil**: Google's C++ library collection

### Build System
- **CMake**: Cross-platform build system
- **C++17**: Modern C++ standard required
- **Optional Python Bindings**: PyBind11 integration available

## Video Analysis Capabilities

### Timing Analysis
- Frame counting and duration calculation
- PTS (Presentation Time Stamp) and DTS (Decoding Time Stamp) processing
- Frame rate calculation (average, median, standard deviation)
- Frame drop detection and quantification
- Video freeze detection
- Audio/video synchronization analysis

### Video Quality Metrics
- Keyframe analysis and ratios
- Bitrate calculation
- Resolution and aspect ratio detection
- Colorimetry information (primaries, transfer characteristics, matrix coefficients)
- Chroma format and bit depth analysis

### Output Formats
- **CSV Reports**: Comprehensive metrics in tabular format
- **Timestamp Files**: Frame-by-frame timing data
- **Policy Validation**: Rule-based pass/fail analysis

## Build Configuration

### Standard Build
```bash
mkdir build && cd build
CC=gcc CXX=g++ cmake -DBUILD_CLANG_FUZZER=OFF ..
make
```

### Development Build Options
- Debug builds with symbols: `-DCMAKE_BUILD_TYPE=DEBUG`
- Python bindings: `-DBUILD_PYBINDINGS=ON`
- Testing framework: `-DBUILD_TESTS=ON` (default)
- Fuzzing support: `-DBUILD_CLANG_FUZZER=ON`

### Generated Code Management
The build system automatically:
- Downloads ANTLR JAR file for grammar compilation
- Generates parser code from `rules.g4` grammar
- Compiles Protocol Buffer definitions
- Creates autogen directories for generated sources

## Key Box Parsing

The library extracts information from specific ISOBMFF boxes:
- `/moov/trak/mdia/hdlr`: Track type identification (audio/video)
- `/moov/trak/mdia/mdhd`: Track timescale and duration
- `/moov/trak/mdia/minf/stbl/stts`: Sample-to-time mappings
- `/moov/trak/mdia/minf/stbl/stss`: Sync sample (keyframe) information
- `/moov/trak/mdia/minf/stbl/ctts`: Composition time offset

## Usage Patterns

### Library Integration
```cpp
auto config = std::make_unique<LiblcvmConfig>();
config->set_debug(debug_level);
config->set_policy(policy_string);

auto file_info = IsobmffFileInformation::parse(filename, *config);
uint32_t timescale_movie_hz = file_info->get_timing()->get_timescale_movie_hz();
uint32_t timescale_video_hz = file_info->get_timing()->get_timescale_video_hz();
```

Note: `timescale_movie_hz` is the movie-level timescale from the mvhd box,
while `timescale_video_hz` is the track-level timescale from the mdhd box.

### Command Line Usage
```bash
# Basic analysis
./lcvm input.mp4 -o metrics.csv

# With policy validation
./lcvm input.mp4 -p policy.txt -o metrics.csv

# Detailed timestamp export
./lcvm input.mp4 --outfile-timestamps timestamps.csv
```

## Testing and Quality

### Test Infrastructure
- Unit tests for core functionality
- Fuzzing harnesses for robustness testing
- Example media files for validation
- Policy validation test cases

### Code Quality Tools
- Clang-format integration with Google style
- Static analysis support
- Memory safety through smart pointers
- Comprehensive error handling

## Development Notes

### Code Generation
The project uses significant code generation:
- ANTLR generates lexer/parser from grammar
- Protocol Buffers generate serialization code
- CMake manages the generation pipeline

### Python Bindings
Optional PyBind11 integration allows Python usage:
- Cross-platform compatibility handling
- Dynamic library generation
- Python version detection and linking

### Performance Considerations
- Designed for low-complexity operation
- Minimal memory allocation
- Efficient box parsing
- Optional multi-run analysis for benchmarking

This codebase represents a sophisticated video analysis tool with enterprise-grade policy validation capabilities, suitable for both library integration and standalone analysis workflows.