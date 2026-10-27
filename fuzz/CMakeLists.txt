# The sanitizer and coverage flags come from the top-level CMakeLists.txt,
# so that liblcvm and the isobmff/h264nal/h265nal libraries get instrumented
# too; all that is left here is linking the fuzzer against libFuzzer itself.
add_link_options(-fsanitize=fuzzer)

add_executable(liblcvm_parse_fuzzer liblcvm_parse_fuzzer.cc)
target_link_libraries(liblcvm_parse_fuzzer PUBLIC liblcvm)
