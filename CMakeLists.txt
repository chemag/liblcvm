cmake_minimum_required(VERSION 3.10)

project(lcvm VERSION 0.22)

# Ensure modern behavior for add_custom_command(TARGET ...)
if(POLICY CMP0175)
  cmake_policy(SET CMP0175 NEW)
endif()

# ---- Options -----------------------------------------------------------------
option(BUILD_PYBINDINGS "Build Python bindings for liblcvm" OFF)
option(ADD_POLICY "Build policy system with ANTLR and protobuf support" OFF)
option(ADD_C_INTERFACE "Build C interface library for Android/exception-disabled environments" OFF)

# ---- Language / flags --------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Debug flags (you can still override from the CLI)
# Set debug flags for both C and C++
set(COMMON_DEBUG_FLAGS "-g -O0 -Wall -Wextra -Wno-unused-parameter -Wshadow -Werror")
set(CMAKE_C_FLAGS_DEBUG "${COMMON_DEBUG_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${COMMON_DEBUG_FLAGS}")

message(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR} ")

# ---- Generated code dirs -----------------------------------------------------
set(AUTOGEN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/autogen)
file(MAKE_DIRECTORY ${AUTOGEN_DIR})

if(ADD_POLICY)
  set(ANTLR_GEN_DIR ${AUTOGEN_DIR}/antlr-gen)
  set(PROTO_GEN_DIR ${AUTOGEN_DIR}/protobuf-gen)
  file(MAKE_DIRECTORY ${ANTLR_GEN_DIR})
  file(MAKE_DIRECTORY ${PROTO_GEN_DIR})
endif()

# ---- Dependencies ------------------------------------------------------------
find_package(absl REQUIRED)
if(ADD_POLICY)
  find_package(Protobuf REQUIRED)
  find_package(antlr4-runtime REQUIRED)
endif()

if(ADD_POLICY)
  # Download ANTLR jar to build dir if missing
  set(ANTLR_JAR ${CMAKE_BINARY_DIR}/antlr-4.13.1-complete.jar)
  set(ANTLR_JAR_URL https://www.antlr.org/download/antlr-4.13.1-complete.jar)
  if(NOT EXISTS ${ANTLR_JAR})
    message(STATUS "Downloading ANTLR jar to ${ANTLR_JAR} ...")
    file(DOWNLOAD ${ANTLR_JAR_URL} ${ANTLR_JAR} SHOW_PROGRESS)
  endif()

  # ---- Protobuf generation -----------------------------------------------------
  set(PROTO_PATH ${CMAKE_CURRENT_SOURCE_DIR}/policy)
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

  # ---- ANTLR generation --------------------------------------------------------
  set(ANTLR_GRAMMAR ${CMAKE_CURRENT_SOURCE_DIR}/policy/rules.g4)
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
    COMMAND java -Xmx500M -cp ${ANTLR_JAR} org.antlr.v4.Tool
            -Dlanguage=Cpp -visitor -o ${ANTLR_GEN_DIR} ${ANTLR_GRAMMAR}
    DEPENDS ${ANTLR_GRAMMAR} ${ANTLR_JAR}
    COMMENT "Generating ANTLR sources"
  )
  add_custom_target(gen_antlr DEPENDS ${ANTLR_GEN_SRCS})
  add_custom_target(gen_all  DEPENDS gen_proto gen_antlr)
endif()

# ---- Includes ----------------------------------------------------------------
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  /opt/homebrew/include
)

if(ADD_POLICY)
  include_directories(
    ${ANTLR_GEN_DIR}
    ${PROTO_GEN_DIR}
    /usr/include/antlr4-runtime
    /usr/local/include/antlr4-runtime
    ${Protobuf_INCLUDE_DIRS}
  )
endif()

# ---- Third-party subprojects -------------------------------------------------
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/isobmff)

set(BUILD_CLANG_FUZZER OFF)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/h265nal)
if(BUILD_PYBINDINGS)
  set_target_properties(h265nal PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/h264nal)
if(BUILD_PYBINDINGS)
  set_target_properties(h264nal PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

# ---- lcvm library ------------------------------------------------------------
set(LIBLCVM_SOURCES
  src/liblcvm.cc
)

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

target_include_directories(liblcvm PUBLIC ${LIBLCVM_INCLUDE_DIRS})
target_link_libraries(liblcvm PRIVATE ${LIBLCVM_LINK_LIBRARIES})

if(ADD_POLICY)
  # Add policy-specific include directories
  target_include_directories(liblcvm PUBLIC ${ANTLR_GEN_DIR} ${PROTO_GEN_DIR})

  # Ensure code generation before building / using lib
  add_dependencies(liblcvm gen_proto gen_antlr)

  # Add generated & implementation sources to lib
  target_sources(liblcvm PRIVATE
    src/policy_protovisitor.cc
    src/policy_runner.cc
    ${ANTLR_GEN_SRCS}
    ${PROTO_GEN_SRCS}
  )
endif()

# Also link public deps so tools/tests get them transitively
target_link_libraries(liblcvm
  PUBLIC
    absl::strings
    absl::base
    absl::log
    absl::log_internal_check_op
    absl::log_internal_message
    absl::log_internal_log_sink_set
    absl::log_internal_globals
    absl::log_severity
)

if(ADD_POLICY)
  target_link_libraries(liblcvm
    PUBLIC
      protobuf::libprotobuf
      antlr4-runtime
  )
endif()

# ---- C Interface Library -----------------------------------------------------
if(ADD_C_INTERFACE)
  message(STATUS "Configuring liblcvm C interface library")

  # C interface library sources
  set(LIBLCVM_C_SOURCES
    src/liblcvm_c_impl.cpp
  )

  # Create the C interface library
  add_library(liblcvm_c SHARED ${LIBLCVM_C_SOURCES})

  # Set library properties
  set_target_properties(liblcvm_c PROPERTIES
    OUTPUT_NAME "lcvm_c"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "include/liblcvm_c.h"
    POSITION_INDEPENDENT_CODE ON
  )

  # Ensure main liblcvm library is built with PIC for C interface
  set_target_properties(liblcvm PROPERTIES POSITION_INDEPENDENT_CODE ON)
  set_target_properties(isobmff PROPERTIES POSITION_INDEPENDENT_CODE ON)
  set_target_properties(h265nal PROPERTIES POSITION_INDEPENDENT_CODE ON)
  set_target_properties(h264nal PROPERTIES POSITION_INDEPENDENT_CODE ON)

  # Include directories for C interface
  target_include_directories(liblcvm_c
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/include
      ${LIBLCVM_INCLUDE_DIRS}
  )

  # Link against the main liblcvm library
  target_link_libraries(liblcvm_c
    PRIVATE
      liblcvm
  )

  # Ensure proper symbol visibility for C interface
  target_compile_definitions(liblcvm_c PRIVATE
    LIBLCVM_C_EXPORTS=1
  )

  # Set up installation for C interface
  install(TARGETS liblcvm_c
    EXPORT liblcvm_c_targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include
  )

  # Export the targets
  install(EXPORT liblcvm_c_targets
    FILE liblcvm_c_targets.cmake
    NAMESPACE liblcvm::
    DESTINATION lib/cmake/liblcvm_c
  )

  # Create config file
  include(CMakePackageConfigHelpers)
  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/liblcvm_c_config_version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
  )

  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/liblcvm_c_config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/liblcvm_c_config.cmake"
    INSTALL_DESTINATION lib/cmake/liblcvm_c
  )

  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/liblcvm_c_config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/liblcvm_c_config_version.cmake"
    DESTINATION lib/cmake/liblcvm_c
  )
endif()

# ---- Tool executable ---------------------------------------------------------
# NOTE: The real tool file is tools/lcvm.cc (not lcvm_tool.cpp).
add_executable(lcvm tools/lcvm.cc)
target_link_libraries(lcvm PRIVATE liblcvm)

# Generate config.h into the build dir and include it from the lib
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/config.h.in
  ${CMAKE_BINARY_DIR}/config.h
)
target_include_directories(liblcvm PRIVATE ${CMAKE_BINARY_DIR})
target_include_directories(lcvm PRIVATE ${CMAKE_BINARY_DIR})

# ---- Tests -------------------------------------------------------------------
option(BUILD_TESTS "Build liblcvm tests" ON)
if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()

# Quick local commands
add_custom_target(test2
  COMMAND ${CMAKE_COMMAND} -E echo "Running: ${CMAKE_BINARY_DIR}/lcvm ${CMAKE_CURRENT_SOURCE_DIR}/lib/isobmff/media/*MOV -o /tmp/full.csv"
  COMMAND ${CMAKE_BINARY_DIR}/lcvm ${CMAKE_CURRENT_SOURCE_DIR}/lib/isobmff/media/*MOV -o /tmp/full.csv
  COMMAND ${CMAKE_COMMAND} -E echo "Running: ${CMAKE_BINARY_DIR}/lcvm ${CMAKE_CURRENT_SOURCE_DIR}/lib/isobmff/media/*MOV -p ${CMAKE_CURRENT_SOURCE_DIR}/policy/example.txt -o /tmp/full.policy.csv"
  COMMAND ${CMAKE_BINARY_DIR}/lcvm ${CMAKE_CURRENT_SOURCE_DIR}/lib/isobmff/media/*MOV -p ${CMAKE_CURRENT_SOURCE_DIR}/policy/example.txt -o /tmp/full.policy.csv
)

add_custom_target(lint
  COMMAND clang-format -i -style=google ${CMAKE_CURRENT_SOURCE_DIR}/include/*h
  COMMAND clang-format -i -style=google ${CMAKE_CURRENT_SOURCE_DIR}/src/*cc
  COMMAND clang-format -i -style=google ${CMAKE_CURRENT_SOURCE_DIR}/test/*cc
  COMMAND clang-format -i -style=google ${CMAKE_CURRENT_SOURCE_DIR}/tools/*cc
)

# ---- Fuzz harness convenience target -----------------------------------------
set(CORPUS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/isobmff/fuzz/corpus)
set(RUNS 100 CACHE STRING "Number of runs for fuzzers")
add_custom_target(fuzz
  COMMAND ${CMAKE_COMMAND} -E echo "Running all fuzzers with ${RUNS} runs..."
)
# Find fuzzers built in the *build* tree
file(GLOB FUZZER_EXECUTABLES "${CMAKE_BINARY_DIR}/lib/isobmff/fuzz/*_fuzzer")
foreach(FUZZER_EXECUTABLE IN LISTS FUZZER_EXECUTABLES)
  get_filename_component(FUZZER_NAME ${FUZZER_EXECUTABLE} NAME_WE)
  add_custom_command(
    TARGET fuzz POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CORPUS_DIR}/${FUZZER_NAME}
  )
  add_custom_command(
    TARGET fuzz POST_BUILD
    COMMAND ${FUZZER_EXECUTABLE}
            -artifact_prefix=${CORPUS_DIR}/${FUZZER_NAME}/
            ${CORPUS_DIR}/${FUZZER_NAME}/
            -runs=${RUNS}
  )
endforeach()

# ---- Cleanup helpers ---------------------------------------------------------
add_custom_target(clean-gen
  COMMENT "Cleaning generated files"
)

if(ADD_POLICY)
  add_custom_command(TARGET clean-gen POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove -f ${PROTO_GEN_SRCS}
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${ANTLR_GEN_DIR}
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${PROTO_GEN_DIR}
    COMMAND ${CMAKE_COMMAND} -E remove -f ${ANTLR_JAR}
  )
endif()
