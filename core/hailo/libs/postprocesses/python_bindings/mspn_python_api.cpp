#include <pybind11/pybind11.h>
#include <type_traits>
#include <pybind11/numpy.h>
#include <pybind11/operators.h>
#include <pybind11/stl.h>
#include "pose_estimation/mspn.hpp"

namespace py = pybind11;
using namespace pybind11::literals;

PYBIND11_MODULE(mspn, mspn_module)
{
    mspn_module.def("mspn", &mspn, "MSPN", "roi"_a);
}
