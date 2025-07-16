#include <pybind11/pybind11.h>
#include <type_traits>
#include <pybind11/numpy.h>
#include <pybind11/operators.h>
#include <pybind11/stl.h>
#include "detection/yolo_postprocess.hpp"

namespace py = pybind11;
using namespace pybind11::literals;

PYBIND11_MODULE(yolo, yolo_module)
{
    //YOLO POSTPROCESS
    {
        py::class_<YoloParams>(yolo_module, "YoloParams")
            .def(py::init<>())
            .def("check_params_logic", &YoloParams::check_params_logic, "Check_params_logic", "num_classes_tensors"_a);
    }
    yolo_module.def("init", &init, "Init YOLO", "config_path"_a, "function_name"_a);
    yolo_module.def("free_resources", &free_resources, "Free_resources");
    yolo_module.def("yolov5", &yolov5, "Yolov5", "roi"_a, "params_void_ptr"_a);
    yolo_module.def("yolov5_no_persons", &yolov5_no_persons, "Yolov5_no_persons", "roi"_a, "params_void_ptr"_a);
    yolo_module.def("yolov5_personface", &yolov5_personface, "Yolov5_personface", "roi"_a, "params_void_ptr"_a);
    yolo_module.def("yolov5_personface_letterbox", &yolov5_personface_letterbox, "Yolov5_personface with letterbox", "roi"_a, "params_void_ptr"_a);
    yolo_module.def("yolov5_vehicles_only", &yolov5_vehicles_only, "Yolov5_vehicles_only", "roi"_a, "params_void_ptr"_a);
    yolo_module.def("yolov5_no_faces_letterbox", &yolov5_no_faces_letterbox, "yolov5_no_faces_letterbox", "roi"_a, "params_void_ptr"_a);
    yolo_module.def("yolov5_adas", &yolov5_adas, "yolov5_adas", "roi"_a, "params_void_ptr"_a);
    yolo_module.def("yolov4", &yolov4, "Yolov4", "roi"_a, "params_void_ptr"_a);
    yolo_module.def("yolov3", &yolov3, "Yolov3", "roi"_a, "params_void_ptr"_a);
    yolo_module.def("tiny_yolov4_license_plates", &tiny_yolov4_license_plates, "Tiny_yolov4_license_plates", "roi"_a, "params_void_ptr"_a);
    yolo_module.def("yolox", &yolox, "yolox", "roi"_a, "params_void_ptr"_a);
    yolo_module.def("yoloxx", &yoloxx, "yoloxx", "roi"_a, "params_void_ptr"_a);
}
