cmake_minimum_required(VERSION 3.10)

project(lcvm)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wno-unused-parameter -Wshadow -Werror")

message(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR} ")

# Add include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/isobmff/ISOBMFF/include)

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
