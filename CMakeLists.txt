cmake_minimum_required(VERSION 3.12)

project(lcvm VERSION 0.12)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE INTERNAL "" FORCE)

# Set compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wno-unused-parameter -Wshadow -Werror")

message(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR} ")

# add include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/isobmff/ISOBMFF/include)


# 1. build external libraries

# 1.1. build ISOBMFF library
# apply isobmff fix patch
execute_process(
  COMMAND git apply ${CMAKE_CURRENT_SOURCE_DIR}/lib/isobmff.patch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib/isobmff
)

# add the ISOBMFF submodule
include(ExternalProject)
ExternalProject_Add(
  ISOBMFF
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/lib/isobmff
  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND ""
  BUILD_COMMAND make
  INSTALL_COMMAND ""
)

# build isobmff library
execute_process(
  COMMAND make
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib/isobmff
)


# we do not care about fuzzing either h264nal nor h265nal
set(BUILD_CLANG_FUZZER OFF)  # Set the variable here

# 1.2. add h265nal submodule
add_subdirectory(${CMAKE_SOURCE_DIR}/lib/h265nal)

# 1.3. add h264nal submodule
add_subdirectory(${CMAKE_SOURCE_DIR}/lib/h264nal)


# 2. build liblcvm library
add_library(liblcvm src/liblcvm.cc)

target_link_libraries(liblcvm
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/isobmff/Build/Debug/Products/x86_64/libISOBMFF.a
  h265nal
  h264nal
)
target_include_directories(liblcvm
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/isobmff/ISOBMFF/include
)
target_include_directories(liblcvm
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/h265nal/include
)
target_include_directories(liblcvm
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/h264nal/include
)

# set the output name to ensure the library is named liblcvm.a
set_target_properties(liblcvm PROPERTIES OUTPUT_NAME "lcvm")


# 3. build lcvm executable
add_executable(lcvm tools/lcvm.cc)
target_link_libraries(lcvm liblcvm)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file(
  ${CMAKE_SOURCE_DIR}/include/config.h.in
  ${CMAKE_BINARY_DIR}/config.h
)

# Include the directory where config.h is generated
target_include_directories(liblcvm PRIVATE ${CMAKE_BINARY_DIR})


# 4. others

# 4.1. add test target
add_custom_target(test
  COMMAND ${CMAKE_BINARY_DIR}/lcvm ${CMAKE_BINARY_DIR}/*MOV -o /tmp/full.csv
)

# 4.2. add lint target
add_custom_target(lint
  COMMAND clang-format -i -style=google ${CMAKE_CURRENT_SOURCE_DIR}/include/*h
  COMMAND clang-format -i -style=google ${CMAKE_CURRENT_SOURCE_DIR}/src/*cc
  COMMAND clang-format -i -style=google ${CMAKE_CURRENT_SOURCE_DIR}/tools/*cc
)


# 5. build liblcvm pybindings library

execute_process(COMMAND python3 -m pybind11 --includes
  OUTPUT_VARIABLE PYBIND11_INCLUDE OUTPUT_STRIP_TRAILING_WHITESPACE)

string(REGEX REPLACE "-I" "" PYBIND11_INCLUDE "${PYBIND11_INCLUDE}")
separate_arguments(PYBIND11_INCLUDE)

# Get Python include paths and libraries
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

# Combine linker flags, ensuring no duplicates
list(APPEND ALL_LINKER_FLAGS ${PYTHON_LDFLAGS} "-lpython${PYTHON_VERSION}")
list(REMOVE_DUPLICATES ALL_LINKER_FLAGS)

add_library(liblcvm_pybind SHARED src/python_binding.cc tools/lcvm.cc)
target_include_directories(liblcvm_pybind PUBLIC ${PYBIND11_INCLUDE} ${PYTHON_INCLUDE})
target_link_libraries(liblcvm_pybind PRIVATE liblcvm "-framework CoreFoundation" ${ALL_LINKER_FLAGS})

set_target_properties(liblcvm_pybind PROPERTIES OUTPUT_NAME "lcvm_pybind" SUFFIX ".so")
