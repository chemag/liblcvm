cmake_minimum_required(VERSION 3.10)

project(lcvm VERSION 0.16)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Set compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wno-unused-parameter -Wshadow -Werror")

message(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR} ")

# add include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)


# 1. build external libraries

# 1.1. add isobmff submodule
add_subdirectory(${CMAKE_SOURCE_DIR}/lib/isobmff)

# 1.2. add h265nal submodule

# we do not care about fuzzing either h264nal nor h265nal here
set(BUILD_CLANG_FUZZER OFF)  # Set the variable here

option(BUILD_PYBINDINGS "Build Python bindings for liblcvm" OFF)

add_subdirectory(${CMAKE_SOURCE_DIR}/lib/h265nal)
if(BUILD_PYBINDINGS)
  # Ensure h265nal is built with -fPIC
  set_target_properties(h265nal PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

# 1.3. add h264nal submodule
add_subdirectory(${CMAKE_SOURCE_DIR}/lib/h264nal)
if(BUILD_PYBINDINGS)
  # Ensure h264nal is built with -fPIC
  set_target_properties(h264nal PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

# 2. build liblcvm library

# Common settings for liblcvm
set(LIBLCVM_SOURCES src/liblcvm.cc)
set(LIBLCVM_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/isobmff/ISOBMFF/include
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/h265nal/include
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/h264nal/include
)
set(LIBLCVM_LINK_LIBRARIES
  ISOBMFF
  h265nal
  h264nal)

if(BUILD_PYBINDINGS)
  # Add Python bindings if requested
  message(STATUS "Configuring Liblcvm library with PyBind11 interface")


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

  list(APPEND LIBLCVM_SOURCES src/python_bindings.cc)
  list(APPEND LIBLCVM_INCLUDE_DIRS ${PYBIND11_INCLUDE} ${PYTHON_INCLUDE})

  if(APPLE) # For Mac
    list(APPEND LIBLCVM_LINK_LIBRARIES "-framework CoreFoundation" ${ALL_LINKER_FLAGS})
  elseif(UNIX AND NOT APPLE) # For Linux
    list(APPEND LIBLCVM_LINK_LIBRARIES ${ALL_LINKER_FLAGS})
  endif()

  add_library(liblcvm SHARED ${LIBLCVM_SOURCES})
  # set the output name to ensure the library is named liblcvm.so
  set_target_properties(liblcvm PROPERTIES OUTPUT_NAME "lcvm" SUFFIX ".so")

else()
  message(STATUS "Configuring Liblcvm library")
  # build liblcvm library without pybinding interface
  add_library(liblcvm ${LIBLCVM_SOURCES})
  # set the output name to ensure the library is named liblcvm.a
  set_target_properties(liblcvm PROPERTIES OUTPUT_NAME "lcvm")
endif()

target_include_directories(liblcvm PUBLIC ${LIBLCVM_INCLUDE_DIRS})
target_link_libraries(liblcvm PRIVATE ${LIBLCVM_LINK_LIBRARIES})


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
