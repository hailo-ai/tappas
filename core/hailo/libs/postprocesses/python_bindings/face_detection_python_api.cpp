#include <pybind11/pybind11.h>
#include <type_traits>
#include <pybind11/numpy.h>
#include <pybind11/operators.h>
#include <pybind11/stl.h>
#include "detection/face_detection.hpp"

namespace py = pybind11;
using namespace pybind11::literals;

PYBIND11_MODULE(face_detection, face_detection)
{
    //Face Detction POSTPROCESS
    {
        py::class_<FaceDetectionParams>(face_detection, "FaceDetectionParams");
    }
    face_detection.def("init", &init, "Init Face Detection", "config_path"_a, "function_name"_a);
    face_detection.def("free_resources", &free_resources, "Free_resources");
    face_detection.def("retinaface", &retinaface, "Retinaface", "roi"_a, "params_void_ptr"_a);
    face_detection.def("lightface", &lightface, "LightFace", "roi"_a, "params_void_ptr"_a);
}
