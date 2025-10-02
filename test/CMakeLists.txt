# Enable testing
enable_testing()
# Find GTest package
find_package(GTest REQUIRED)
# Include directories
include_directories(PUBLIC ../src)
# MacOS requirements
include_directories(PUBLIC /usr/local/include)
link_directories(/usr/local/lib)

# define the root directory variable
set(TEST_MEDIA_DIR ${CMAKE_SOURCE_DIR}/lib/isobmff/media)
set(TEST_POLICY_DIR ${CMAKE_SOURCE_DIR}/policy)

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

  target_link_libraries(${test_name}
    PUBLIC liblcvm GTest::gtest GTest::gtest_main
  )

  add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

# Find all test files in the current directory
file(GLOB TEST_FILES "*_unittest.cc")

# Filter out policy tests if ADD_POLICY is disabled
if(NOT ADD_POLICY)
  list(FILTER TEST_FILES EXCLUDE REGEX ".*policy.*_unittest\\.cc$")
endif()

# Extract test names from file names
foreach(test_file IN LISTS TEST_FILES)
  get_filename_component(test_name ${test_file} NAME_WE)
  add_liblcvm_test(${test_name})
endforeach()
