// @brief Extracts the video information from the ISOBMFF information.
// Python scripts interfaces this C++ function via Pybind11.
//
// @param[in] infile_list: input file list.
// @param[out] outfile: output csv file
// @param[out] outfile_timestamps: output timestamp file
// @param[in] outfile_timestamps_sort_pts: enable/disable sorting by pts in output timestamps file
// @param[in] debug: enable/disable debug output
int parse_files(std::vector<std::string> &infile_list, char *outfile,
                char *outfile_timestamps, bool outfile_timestamps_sort_pts,
                int debug);
