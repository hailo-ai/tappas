#include <pybind11/pybind11.h>
#include <type_traits>
#include <pybind11/numpy.h>
#include <pybind11/operators.h>
#include <pybind11/stl.h>
#include "instance_segmentation/yolov5seg.hpp"

namespace py = pybind11;
using namespace pybind11::literals;

PYBIND11_MODULE(yolov5seg, yolov5seg_module)
{
    {
        py::class_<Yolov5segParams>(yolov5seg_module, "Yolov5segParams")
            .def(py::init<>());
    }
    yolov5seg_module.def("init", &init, "Init Yolov5seg", "config_path"_a, "function_name"_a);
    yolov5seg_module.def("free_resources", &free_resources, "Free_resources");
    yolov5seg_module.def("yolov5seg", &yolov5seg, "Yolov5seg", "roi"_a, "params_void_ptr"_a);
}
