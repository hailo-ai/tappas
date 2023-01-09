import numpy as np

import hailo

print("hailo.HailoPoint")
print(dir(hailo.HailoPoint))

_hailo_point = hailo.HailoPoint(1, 1, 1)
print(f"_hailo_point.x = {_hailo_point.x()}")
print(f"_hailo_point.y = {_hailo_point.y()}")
print(f"_hailo_point.confidence = {_hailo_point.confidence()}")

print("hailo.HailoBBox")
print(dir(hailo.HailoBBox))

_hailo_b_box = hailo.HailoBBox(1, 1, 1, 1)
print(f"_hailo_b_box.xmin = {_hailo_b_box.xmin()}")
print(f"_hailo_b_box.ymin = {_hailo_b_box.ymin()}")
print(f"_hailo_b_box.width = {_hailo_b_box.width()}")
print(f"_hailo_b_box.height = {_hailo_b_box.height()}")
print(f"_hailo_b_box.xmax = {_hailo_b_box.xmax()}")
print(f"_hailo_b_box.ymax = {_hailo_b_box.ymax()}")

print("hailo.HailoObject")
print(dir(hailo.HailoObject))

_hailo_object = hailo.HailoObject()

print("hailo.HailoMainObject")
print(dir(hailo.HailoMainObject))

_hailo_main_object = hailo.HailoMainObject()
# print(f"_hailo_main_object.add_object = {_hailo_main_object.add_object()}")
# print(f"_hailo_main_object.add_tensor = {_hailo_main_object.add_tensor()}")
# print(f"_hailo_main_object.get_tensor = {_hailo_main_object.get_tensor()}")
print(f"_hailo_main_object.has_tensors = {_hailo_main_object.has_tensors()}")
print(f"_hailo_main_object.get_tensors = {_hailo_main_object.get_tensors()}")
print(
    f"_hailo_main_object.clear_tensors = {_hailo_main_object.clear_tensors()}")
print(f"_hailo_main_object.get_objects = {_hailo_main_object.get_objects()}")
# print(f"_hailo_main_object.get_objects_typed = {_hailo_main_object.get_objects_typed()}")

print("hailo.HailoROI")
print(dir(hailo.HailoROI))

_hailo_roi = hailo.HailoROI(_hailo_b_box)
print(f"_hailo_roi.get_bbox = {_hailo_roi.get_bbox()}")
# print(f"_hailo_roi.set_bbox = {_hailo_roi.set_bbox()}")
print(f"_hailo_roi.get_type = {_hailo_roi.get_type()}")
print(f"_hailo_roi.is_main_roi = {_hailo_roi.is_main_roi()}")

print("hailo.HailoTileROI")
print(dir(hailo.HailoTileROI))

# _hailo_tile_roi = hailo.HailoTileROI()
# print(f"_hailo_tile_roi.get_type = {_hailo_tile_roi.get_type()}")
# print(f"_hailo_tile_roi.overlap_x_axis = {_hailo_tile_roi.overlap_x_axis()}")
# print(f"_hailo_tile_roi.overlap_y_axis = {_hailo_tile_roi.overlap_y_axis()}")
# print(f"_hailo_tile_roi.index = {_hailo_tile_roi.index()}")
# print(f"_hailo_tile_roi.mode = {_hailo_tile_roi.mode()}")

print("hailo.HailoDetection")
print(dir(hailo.HailoDetection))

_new_hailo_detection = hailo.HailoDetection(_hailo_b_box, "TBD", 1)
print(f"_new_hailo_detection.get_type = {_new_hailo_detection.get_type()}")
print(
    f"_new_hailo_detection.get_confidence = {_new_hailo_detection.get_confidence()}"
)
print(f"_new_hailo_detection.get_label = {_new_hailo_detection.get_label()}")
print(
    f"_new_hailo_detection.get_class_id = {_new_hailo_detection.get_class_id()}"
)

print("hailo.HailoClassification")
print(dir(hailo.HailoClassification))

_hailo_classification = hailo.HailoClassification("TBD_STRING", "TBD", 1)
print(
    f"_hailo_classification.get_confidence = {_hailo_classification.get_confidence()}"
)
print(f"_hailo_classification.get_label = {_hailo_classification.get_label()}")
print(
    f"_hailo_classification.get_classification_type = {_hailo_classification.get_classification_type()}"
)
print(
    f"_hailo_classification.get_class_id = {_hailo_classification.get_class_id()}"
)
print(f"_hailo_classification.get_type = {_hailo_classification.get_type()}")

print("hailo.HailoLandmarks")
print(dir(hailo.HailoLandmarks))

_hailo_landmarks = hailo.HailoLandmarks("TBD", 1)
print(f"_hailo_landmarks.add_point = {_hailo_landmarks.add_point()}")
print(f"_hailo_landmarks.get_type = {_hailo_landmarks.get_type()}")
print(f"_hailo_landmarks.get_points = {_hailo_landmarks.get_points()}")
print(f"_hailo_landmarks.set_points = {_hailo_landmarks.set_points()}")
print(f"_hailo_landmarks.get_threshold = {_hailo_landmarks.get_threshold()}")
print(
    f"_hailo_landmarks.get_landmarks_type = {_hailo_landmarks.get_landmarks_type()}"
)

print("hailo.HailoTensor")
print(dir(hailo.HailoTensor))

# _new_hailo_tensor = hailo.HailoTensor()
# print(f"_new_hailo_tensor.name = {_new_hailo_tensor.name()}")
# print(f"_new_hailo_tensor.vstream_info = {_new_hailo_tensor.vstream_info()}")
# print(f"_new_hailo_tensor.data = {_new_hailo_tensor.data()}")
# print(f"_new_hailo_tensor.size = {_new_hailo_tensor.size()}")
# print(f"_new_hailo_tensor.shape = {_new_hailo_tensor.shape()}")
# print(f"_new_hailo_tensor.fix_scale = {_new_hailo_tensor.fix_scale()}")
# print(f"_new_hailo_tensor.get = {_new_hailo_tensor.get()}")
# print(f"_new_hailo_tensor.get_full_percision = {_new_hailo_tensor.get_full_percision()}")
