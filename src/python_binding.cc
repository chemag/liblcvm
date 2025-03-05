#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "lcvm.h"

namespace py = pybind11;

PYBIND11_MODULE(liblcvm_pybind, m) {
    m.doc() = "Pybind11 interface for liblcvm_pybind shared library";

    // Expose parse_files C++ function to Python scripts via Pybind11
    m.def("parse_files",
            &parse_files,
            py::arg("infile_list"),
            py::arg("outfile"),
            py::arg("outfile_timestamps"),
            py::arg("outfile_timestamps_sort_pts"),
            py::arg("debug"));
}
