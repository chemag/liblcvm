cmake_minimum_required(VERSION 3.10)

project(lcvm VERSION 0.0)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE INTERNAL "" FORCE)

# Set compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wno-unused-parameter -Wshadow -Werror")

message(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR} ")

# Add include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/isobmff/ISOBMFF/include)


# apply isobmff fix patch
execute_process(
  COMMAND git apply ${CMAKE_CURRENT_SOURCE_DIR}/lib/isobmff.patch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib/isobmff
)

# build isobmff library
execute_process(
  COMMAND make
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib/isobmff
)

# Build liblcvm library
add_library(liblcvm src/liblcvm.cc)
target_link_libraries(liblcvm
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/isobmff/Build/Debug/Products/x86_64/libISOBMFF.a
)
target_include_directories(liblcvm
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/isobmff/ISOBMFF/include
)

# Build lcvm executable
add_executable(lcvm tools/lcvm.cc)
target_link_libraries(lcvm liblcvm)

# Configure a header file to pass some of the CMake settings
# to the source code
configure_file(
  ${CMAKE_SOURCE_DIR}/include/config.h.in
  ${CMAKE_BINARY_DIR}/config.h
)

# Include the directory where config.h is generated
target_include_directories(liblcvm PRIVATE ${CMAKE_BINARY_DIR})

# Add test target
add_custom_target(test
  COMMAND ${CMAKE_BINARY_DIR}/lcvm ${CMAKE_BINARY_DIR}/*MOV -o /tmp/full.csv
)

# Add lint target
add_custom_target(lint
  COMMAND clang-format -i -style=google ${CMAKE_CURRENT_SOURCE_DIR}/include/*h
  COMMAND clang-format -i -style=google ${CMAKE_CURRENT_SOURCE_DIR}/src/*cc
  COMMAND clang-format -i -style=google ${CMAKE_CURRENT_SOURCE_DIR}/tools/*cc
)
