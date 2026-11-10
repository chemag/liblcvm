# Enable testing
enable_testing()
# Find GTest package
find_package(GTest REQUIRED)
# Include directories
include_directories(PUBLIC ../src)
# MacOS requirements
include_directories(PUBLIC /usr/local/include)
link_directories(/usr/local/lib)

# Define test directories
set(TEST_MEDIA_DIR ${CMAKE_SOURCE_DIR}/media)
set(TEST_POLICY_DIR ${CMAKE_SOURCE_DIR}/policy)
set(TEST_CONFORMANCE_DIR ${CMAKE_SOURCE_DIR}/test)
set(TEST_REGRESSION_DIR ${CMAKE_SOURCE_DIR}/test/regression)

# Function to add a test executable
function(add_liblcvm_test test_name)
  add_executable(${test_name} ${test_name}.cc)

  # Include the build directory for config.h
  target_include_directories(${test_name} PRIVATE ${CMAKE_BINARY_DIR})

  # pass variables to C++ as a string literal
  target_compile_definitions(${test_name}
    PRIVATE TEST_MEDIA_DIR="${TEST_MEDIA_DIR}"
  )
  target_compile_definitions(${test_name}
    PRIVATE TEST_POLICY_DIR="${TEST_POLICY_DIR}"
  )
  target_compile_definitions(${test_name}
    PRIVATE TEST_CONFORMANCE_DIR="${TEST_CONFORMANCE_DIR}"
  )
  target_compile_definitions(${test_name}
    PRIVATE TEST_REGRESSION_DIR="${TEST_REGRESSION_DIR}"
  )

  target_link_libraries(${test_name}
    PUBLIC liblcvm GTest::gtest GTest::gtest_main
  )

  add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

# Find all test files in the current directory
file(GLOB TEST_FILES "*_unittest.cc" "*_test.cc")

# Filter out policy tests if ADD_POLICY is disabled
if(NOT ADD_POLICY)
  list(FILTER TEST_FILES EXCLUDE REGEX ".*policy.*_unittest\\.cc$")
endif()

# The C interface test needs the C interface library
if(NOT ADD_C_INTERFACE)
  list(FILTER TEST_FILES EXCLUDE REGEX ".*liblcvm_c_api_unittest\\.cc$")
endif()

# Extract test names from file names
foreach(test_file IN LISTS TEST_FILES)
  get_filename_component(test_name ${test_file} NAME_WE)
  add_liblcvm_test(${test_name})
endforeach()

# CLI regressions exercise compact track fields and union-header serialization.
foreach(cli_test IN ITEMS liblcvm_multi_audio_unittest
                          liblcvm_compact_tracks_unittest
                          liblcvm_strict_cli_unittest)
  if(TARGET ${cli_test})
    target_compile_definitions(${cli_test} PRIVATE
      LCVM_TOOL_PATH="$<TARGET_FILE:lcvm>")
    add_dependencies(${cli_test} lcvm)
  endif()
endforeach()

# The C interface test opens its media relative to the repository root.
if(TARGET liblcvm_c_api_unittest)
  target_link_libraries(liblcvm_c_api_unittest PRIVATE liblcvm_c)
  target_compile_definitions(liblcvm_c_api_unittest PRIVATE
    TEST_SOURCE_ROOT="${CMAKE_SOURCE_DIR}")
endif()
