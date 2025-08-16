cmake_minimum_required(VERSION 3.10)

project(lcvm VERSION 0.17)

# use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wno-unused-parameter -Wshadow")

message(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR} ")

# --- ANTLR/Protobuf Autogen Setup ---
set(AUTOGEN_DIR ${CMAKE_SOURCE_DIR}/autogen)
set(ANTLR_GEN_DIR ${AUTOGEN_DIR}/antlr-gen)
set(PROTO_GEN_DIR ${AUTOGEN_DIR}/protobuf-gen)
file(MAKE_DIRECTORY ${ANTLR_GEN_DIR})
file(MAKE_DIRECTORY ${PROTO_GEN_DIR})

# Download ANTLR jar at configure time (to build dir)
set(ANTLR_JAR ${CMAKE_BINARY_DIR}/antlr-4.13.1-complete.jar)
set(ANTLR_JAR_URL https://www.antlr.org/download/antlr-4.13.1-complete.jar)
if(NOT EXISTS ${ANTLR_JAR})
    message(STATUS "Downloading ANTLR jar to ${ANTLR_JAR} ...")
    file(DOWNLOAD ${ANTLR_JAR_URL} ${ANTLR_JAR} SHOW_PROGRESS)
endif()

find_package(Protobuf REQUIRED)
find_package(absl REQUIRED)
find_package(antlr4-runtime REQUIRED)

# Protobuf generation
set(PROTO_PATH ${CMAKE_SOURCE_DIR}/policy)
set(PROTO_SRC rules.proto)
set(PROTO_GEN_SRCS
    ${PROTO_GEN_DIR}/rules.pb.cc
    ${PROTO_GEN_DIR}/rules.pb.h
)
add_custom_command(
    OUTPUT ${PROTO_GEN_SRCS}
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
        --proto_path=${PROTO_PATH}
        --cpp_out=${PROTO_GEN_DIR}
        ${PROTO_SRC}
    WORKING_DIRECTORY ${PROTO_PATH}
    DEPENDS ${PROTO_PATH}/${PROTO_SRC}
    COMMENT "Generating Protobuf sources in protobuf-gen/"
)
add_custom_target(gen_proto DEPENDS ${PROTO_GEN_SRCS})

# ANTLR generation
set(ANTLR_GRAMMAR ${CMAKE_SOURCE_DIR}/policy/rules.g4)
set(ANTLR_GEN_SRCS
    ${ANTLR_GEN_DIR}/rulesLexer.cpp
    ${ANTLR_GEN_DIR}/rulesParser.cpp
    ${ANTLR_GEN_DIR}/rulesBaseListener.cpp
    ${ANTLR_GEN_DIR}/rulesListener.cpp
    ${ANTLR_GEN_DIR}/rulesBaseVisitor.cpp
    ${ANTLR_GEN_DIR}/rulesVisitor.cpp
)
add_custom_command(
    OUTPUT ${ANTLR_GEN_SRCS}
    COMMAND java -Xmx500M -cp ${ANTLR_JAR} org.antlr.v4.Tool -Dlanguage=Cpp -visitor -o ${ANTLR_GEN_DIR} ${ANTLR_GRAMMAR}
    DEPENDS ${ANTLR_GRAMMAR} ${ANTLR_JAR}
    COMMENT "Generating ANTLR sources"
)
add_custom_target(gen_antlr DEPENDS ${ANTLR_GEN_SRCS})

add_custom_target(gen_all DEPENDS gen_proto gen_antlr)

# Add include directories (including generated)
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${ANTLR_GEN_DIR}
    ${PROTO_GEN_DIR}
    /usr/include/antlr4-runtime
    /usr/local/include/antlr4-runtime
    /opt/homebrew/include
    ${Protobuf_INCLUDE_DIRS}
)

# 1. build external libraries
add_subdirectory(${CMAKE_SOURCE_DIR}/lib/isobmff)

set(BUILD_CLANG_FUZZER OFF)
option(BUILD_PYBINDINGS "Build Python bindings for liblcvm" OFF)

add_subdirectory(${CMAKE_SOURCE_DIR}/lib/h265nal)
if(BUILD_PYBINDINGS)
  set_target_properties(h265nal PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

add_subdirectory(${CMAKE_SOURCE_DIR}/lib/h264nal)
if(BUILD_PYBINDINGS)
  set_target_properties(h264nal PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()


# 2. build liblcvm library

set(LIBLCVM_SOURCES src/liblcvm.cc)

set(LIBLCVM_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/isobmff/include
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/h265nal/include
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/h264nal/include
)

set(LIBLCVM_LINK_LIBRARIES
  isobmff
  h265nal
  h264nal
)

if(BUILD_PYBINDINGS)
  message(STATUS "Configuring Liblcvm library with PyBind11 interface")
  execute_process(COMMAND python3 -m pybind11 --includes
      OUTPUT_VARIABLE PYBIND11_INCLUDE OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(REGEX REPLACE "-I" "" PYBIND11_INCLUDE "${PYBIND11_INCLUDE}")
  separate_arguments(PYBIND11_INCLUDE)

  execute_process(COMMAND python3-config --includes
      OUTPUT_VARIABLE PYTHON_INCLUDE OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND python3-config --ldflags
      OUTPUT_VARIABLE PYTHON_LDFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND python3 -c "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}')"
      OUTPUT_VARIABLE PYTHON_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(REPLACE "-framework CoreFoundation" "" PYTHON_LDFLAGS "${PYTHON_LDFLAGS}")
  string(REGEX REPLACE "-I" "" PYTHON_INCLUDE "${PYTHON_INCLUDE}")
  separate_arguments(PYTHON_INCLUDE)
  separate_arguments(PYTHON_LDFLAGS)

  list(APPEND ALL_LINKER_FLAGS ${PYTHON_LDFLAGS} "-lpython${PYTHON_VERSION}")
  list(REMOVE_DUPLICATES ALL_LINKER_FLAGS)

  list(APPEND LIBLCVM_SOURCES src/python_bindings.cc)
  list(APPEND LIBLCVM_INCLUDE_DIRS ${PYBIND11_INCLUDE} ${PYTHON_INCLUDE})

  if(APPLE)
    list(APPEND LIBLCVM_LINK_LIBRARIES "-framework CoreFoundation" ${ALL_LINKER_FLAGS})
  elseif(UNIX AND NOT APPLE)
    list(APPEND LIBLCVM_LINK_LIBRARIES ${ALL_LINKER_FLAGS})
  endif()

  add_library(liblcvm SHARED ${LIBLCVM_SOURCES})
  set_target_properties(liblcvm PROPERTIES OUTPUT_NAME "lcvm" SUFFIX ".so")
else()
  message(STATUS "Configuring Liblcvm library")
  add_library(liblcvm ${LIBLCVM_SOURCES})
  set_target_properties(liblcvm PROPERTIES OUTPUT_NAME "lcvm")
endif()

target_include_directories(liblcvm PUBLIC ${LIBLCVM_INCLUDE_DIRS} ${ANTLR_GEN_DIR} ${PROTO_GEN_DIR})
target_link_libraries(liblcvm PRIVATE ${LIBLCVM_LINK_LIBRARIES})

# Ensure code generation before building liblcvm
add_dependencies(liblcvm gen_proto gen_antlr)

# Move implementation sources (including generated) into the lib
target_sources(liblcvm PRIVATE
  src/policy_protovisitor.cc
  src/policy_runner.cc
  ${ANTLR_GEN_SRCS}
  ${PROTO_GEN_SRCS}
)

# Link third-party deps that liblcvm itself uses (PUBLIC so tests & tools get them)
target_link_libraries(liblcvm
  PUBLIC
    protobuf::libprotobuf
    antlr4-runtime
    absl::strings
    absl::base
    absl::log
    absl::log_internal_check_op
    absl::log_internal_message
    absl::log_internal_log_sink_set
    absl::log_internal_globals
    absl::log_severity
)

# Generated-code dependencies should be attached to the library
add_dependencies(liblcvm gen_proto gen_antlr)

# Executable is now just the tool front-end
add_executable(lcvm tools/lcvm.cc)
target_link_libraries(lcvm PRIVATE liblcvm)


# configure a header file to pass some of the CMake settings
configure_file(
  ${CMAKE_SOURCE_DIR}/include/config.h.in
  ${CMAKE_BINARY_DIR}/config.h
)
target_include_directories(liblcvm PRIVATE ${CMAKE_BINARY_DIR})

# 4. others

option(BUILD_TESTS "Build liblcvm tests" ON)
if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()


add_custom_target(test2
  COMMAND ${CMAKE_COMMAND} -E echo "Running: ${CMAKE_BINARY_DIR}/lcvm ${CMAKE_SOURCE_DIR}/lib/isobmff/media/*MOV -o /tmp/full.csv"
  COMMAND ${CMAKE_BINARY_DIR}/lcvm ${CMAKE_SOURCE_DIR}/lib/isobmff/media/*MOV -o /tmp/full.csv
  COMMAND ${CMAKE_COMMAND} -E echo "Running: ${CMAKE_BINARY_DIR}/lcvm ${CMAKE_SOURCE_DIR}/lib/isobmff/media/*MOV -p ${CMAKE_SOURCE_DIR}/policy/example.txt -o /tmp/full.policy.csv"
  COMMAND ${CMAKE_BINARY_DIR}/lcvm ${CMAKE_SOURCE_DIR}/lib/isobmff/media/*MOV -p ${CMAKE_SOURCE_DIR}/policy/example.txt -o /tmp/full.policy.csv
)

add_custom_target(lint
  COMMAND clang-format -i -style=google ${CMAKE_CURRENT_SOURCE_DIR}/include/*h
  COMMAND clang-format -i -style=google ${CMAKE_CURRENT_SOURCE_DIR}/src/*cc
  COMMAND clang-format -i -style=google ${CMAKE_CURRENT_SOURCE_DIR}/test/*cc
  COMMAND clang-format -i -style=google ${CMAKE_CURRENT_SOURCE_DIR}/tools/*cc
)

set(CORPUS_DIR ${CMAKE_SOURCE_DIR}/lib/isobmff/fuzz/corpus)
set(RUNS 100 CACHE STRING "Number of runs for fuzzers")
add_custom_target(fuzz
  COMMAND ${CMAKE_COMMAND} -E echo "Running all fuzzers with ${RUNS} runs..."
)
file(GLOB FUZZER_EXECUTABLES "${CMAKE_SOURCE_DIR}/build/lib/isobmff/fuzz/*_fuzzer")
foreach(FUZZER_EXECUTABLE IN LISTS FUZZER_EXECUTABLES)
  get_filename_component(FUZZER_NAME ${FUZZER_EXECUTABLE} NAME_WE)
  add_custom_command(
    TARGET fuzz
    COMMAND ${FUZZER_EXECUTABLE}
            -artifact_prefix=${CORPUS_DIR}/${FUZZER_NAME}/
            ${CORPUS_DIR}/${FUZZER_NAME}/
            -runs=$$RUNS
  )
endforeach()

# Clean generated files
add_custom_target(clean-gen
    COMMAND ${CMAKE_COMMAND} -E remove -f ${PROTO_GEN_SRCS}
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${ANTLR_GEN_DIR}
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${PROTO_GEN_DIR}
    COMMAND ${CMAKE_COMMAND} -E remove -f ${ANTLR_JAR}
    COMMENT "Cleaning generated files"
)
