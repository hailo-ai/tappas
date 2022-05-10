# **HailoObjects API** 

The `TAPPAS` provides a set of abstractions for the different objects you might handle in your pipeline, such as `tensors`, `detections`, `classifications`, or `landmarks`. If you want to write your own custom pipeline behavior (`postprocessing`, `drawing`, etc..), then it is worthwhile to get familiar with these classes.  
This guide documents the different classes available in the `TAPPAS` and their interactions.

## **Table of Contents**
- [**HailoObjects API**](#hailoobjects-api)
  - [**Table of Contents**](#table-of-contents)
- [**Structs**](#structs)
  - [**HailoBBox**](#hailobbox)
    - [Constructor](#constructor)
    - [Functions](#functions)
  - [**HailoPoint**](#hailopoint)
    - [Constructor](#constructor-1)
    - [Functions](#functions-1)
- [**Enumerations**](#enumerations)
  - [**hailo_object_t**](#hailo_object_t)
- [**NewHailoTensor**](#newhailotensor)
  - [Functions](#functions-2)
- [**HailoObject**](#hailoobject)
  - [Constructor](#constructor-2)
  - [Functions](#functions-3)
- [**HailoMainObject**](#hailomainobject)
  - [Constructor](#constructor-3)
  - [Functions](#functions-4)
- [**HailoROI**](#hailoroi)
  - [Constructor](#constructor-4)
  - [Functions](#functions-5)
- [**NewHailoDetection**](#newhailodetection)
  - [Constructors](#constructors)
  - [Functions](#functions-6)
- [**HailoClassification**](#hailoclassification)
  - [Constructors](#constructors-1)
  - [Functions](#functions-7)
- [**HailoLandmarks**](#hailolandmarks)
  - [Constructors](#constructors-2)
  - [Functions](#functions-8)
- [**HailoUniqueID**](#hailouniqueid)
  - [Constructors](#constructors-3)
  - [Functions](#functions-9)
- [**HailoMask**](#hailomask)
  - [Constructors](#constructors-4)
  - [Functions](#functions-10)
- [**HailoDepthMask**](#hailodepthmask)
  - [Constructors](#constructors-5)
  - [Functions](#functions-11)
- [**HailoClassMask**](#hailoclassmask)
  - [Constructors](#constructors-6)
  - [Functions](#functions-12)
- [**HailoConfClassMask**](#hailoconfclassmask)
  - [Constructors](#constructors-7)
  - [Functions](#functions-13)
- [**HailoMatrix**](#hailomatrix)
  - [Constructors](#constructors-8)
  - [Functions](#functions-14)

# **Structs**
## **HailoBBox**
Represents a bounding box.  
`SOURCE`: [core/hailo/general/hailo_objects.hpp](../../core/hailo/general/hailo_objects.hpp)  
### Constructor
```cpp
HailoBBox(float xmin, float ymin, float width, float height)
```
### Functions
| Function   | Return Type | Description                        |
| ---------- | :---------- | :--------------------------------- |
| `xmin()`   | float       | Normalized xmin position.          |
| `ymin()`   | float       | Normalized ymin position.          |
| `width()`  | float       | Normalized width of bounding box.  |
| `height()` | float       | Normalized height of bounding box. |
| `xmax()`   | float       | Normalized xmax position.          |
| `ymax()`   | float       | Normalized ymax position.          |

<br/>
<br/>

## **HailoPoint**
Represents a detected point (landmark).  
`SOURCE`: [core/hailo/general/hailo_objects.hpp](../../core/hailo/general/hailo_objects.hpp)  
### Constructor
```cpp
HailoPoint(float x, float y, float confidence = 1.0f)
```
### Functions
| Function       | Return Type | Description                                                                        |
| -------------- | :---------- | :--------------------------------------------------------------------------------- |
| `x()`          | float       | Normalized x position.                                                             |
| `y()`          | float       | Normalized y position.                                                             |
| `confidence()` | float       | The confidence in the point's accuracy, float between 0.0 to 1.0 - default is 1.0. |

<br/>
<br/>

# **Enumerations**
## **hailo_object_t**
Typing enumeration for [HailoObject](#hailoobject) instances.  
```cpp
typedef enum
{
    HAILO_ROI,
    HAILO_CLASSIFICATION,
    HAILO_DETECTION,
    HAILO_LANDMARKS,
    HAILO_TILE,
    HAILO_UNIQUE_ID,
    HAILO_MATRIX,
    HAILO_DEPTH_MASK,
    HAILO_CLASS_MASK,
    HAILO_CONF_CLASS_MASK
} hailo_object_t;
```

<br/>
<br/>

# **NewHailoTensor**
`Tensors` are the ouput vector of your network inference. Usually these are N-dimensional matrices that hold little "human readable" value at first, but after a little postprocessing become meaningful objects such as detections or landmarks. All postprocesses start by looking at the output tensors. In fact, usually you will not be constructing a new one - they will be [provided to your postprocess](write-your-own-postprocess.md) via the [hailofilter2](../elements/hailo_filter2.md) element. To make handling these vectors easier, they are provided in a `NewHailoTensor` class.  \
`Shared pointer handle`: **NewHailoTensorPtr**  \
`SOURCE`: [core/hailo/general/hailo_tensors.hpp](../../core/hailo/general/hailo_tensors.hpp)  

## Functions
| Function                                              | Return Type                | Description                                                         |
| ----------------------------------------------------- | :------------------------- | :------------------------------------------------------------------ |
| `name()`                                              | std::string                | Get the tensor name.                                                |
| `vstream_info()`                                      | hailo_vstream_info_t       | Get the HailoRT vstream info.                                       |
| `data()`                                              | uint8_t *                  | Get the tensor data pointer.                                        |
| `width()`                                             | uint32_t                   | Get the tensor width.                                               |
| `height()`                                            | uint32_t                   | Get the tensor height.                                              |
| `features()`                                          | uint32_t                   | Get the tensor features.                                            |
| `size()`                                              | uint32_t                   | Get the tensor total length.                                        |
| `shape()`                                             | std::vector\<std::size_t\> | Get the tensor dimensions.                                          |
| `fix_scale(uint8_t num)`                              | float                      | Takes a quantized number and returns its dequantized value (float). |
| `get(uint row, uint col, uint channel)`               | uint8_t                    | Get the tensor value at this location.                              |
| `get_full_percision(uint row, uint col, uint channel` | float                      | Get the tensor dequantized value at this location.                  |

<br/>
<br/>

# **HailoObject**
`HailoObject` represents objects that are usable output after postprocessing. They can be detections, classifications, landmarks, or any other similar postprocess results.  
This class is an abstraction for other objects to inherit from. To more conveniently compare different types of inheriting classes, `HailoObject`s store their object type from an enumerated list [hailo_object_t](#enumerations).  
The `class inheritance hierarchy` is as follows:  
<div align="center">
    <img src="../resources/hailo_objects_api_hierarchy.png"/>
</div>  

`Shared pointer handle`: **HailoObjectPtr**  
`SOURCE`: [core/hailo/general/hailo_objects.hpp](../../core/hailo/general/hailo_objects.hpp)  
## Constructor
```cpp
HailoObject()
```

## Functions
| Function     | Return Type                     | Description                                                           |
| ------------ | :------------------------------ | :-------------------------------------------------------------------- |
| `get_type()` | [hailo_object_t](#enumerations) | The type of the object from the list of enumerated types shown above. |

<br/>
<br/>

# **HailoMainObject**
**Inherits from [HailoObject](#hailoobject).**  
`HailoMainObject` represents a [HailoObject](#hailoobject) that can hold other [HailoObject](#hailoobject)s. For example a face detection can hold landmarks or age classification, gender classification etc...  
`Shared pointer handle`: **HailoMainObjectPtr**  \
`SOURCE`: [core/hailo/general/hailo_objects.hpp](../../core/hailo/general/hailo_objects.hpp)  
## Constructor
```cpp
HailoMainObject()
```

## Functions
| Function                                 | Return Type                                         | Description                                                                                 |
| ---------------------------------------- | :-------------------------------------------------- | :------------------------------------------------------------------------------------------ |
| `add_object(HailoObjectPtr obj)`         | void                                                | Add a [HailoObject](#hailoobject) to this [HailoMainObject](#hailomainobject).               |
| `add_tensor(NewHailoTensorPtr tensor)`   | void                                                | Add a [NewHailoTensor](#newhailotensor) to this [HailoMainObject](#hailomainobject).         |
| `remove_object(HailoObjectPtr obj)`      | void                                                | Remove a [HailoObject](#hailoobject) from this [HailoMainObject](#hailomainobject).          |
| `remove_object(uint index)`              | void                                                | Remove a [HailoObject](#hailoobject) from this [HailoMainObject](#hailomainobject) by index. |
| `get_tensor(std::string name)`           | [NewHailoTensorPtr](#newhailotensor)                | Get a tensor from this [HailoMainObject](#hailomainobject).                                 |
| `has_tensors()`                          | bool                                                | Checks whether there are tensors attached to this [HailoMainObject](#hailomainobject).      |
| `get_tensors()`                          | std::vector\<[NewHailoTensorPtr](#newhailotensor)\> | Get a vector of the tensors attached to this [HailoMainObject](#hailomainobject).           |
| `clear_tensors()`                        | void                                                | Clear all tensors attached to this [HailoMainObject](#hailomainobject).                     |
| `get_objects()`                          | std::vector\<[HailoObjectPtr](#hailoobject)\>       | Get the objects attached to this [HailoMainObject](#hailomainobject).                       |
| `get_objects_typed(hailo_object_t type)` | std::vector\<[HailoObjectPtr](#hailoobject)\>       | Get the objects of a given type, attached to this [HailoMainObject](#hailomainobject).      |

<br/>
<br/>

# **HailoROI**
**Inherits from [HailoMainObject](#hailomainobject).**  
`HailoROI` represents an ROI (Region Of Interest): a part of an image that can hold other objects. Mostly inherited by other objects but isn't abstract. Can represent the whole image by giving the right HailoBBox.
`Shared pointer handle`: **HailoROIPtr**  \
`SOURCE`: [core/hailo/general/hailo_objects.hpp](../../core/hailo/general/hailo_objects.hpp)  
## Constructor
```cpp
HailoROI(HailoBBox bbox)
```

## Functions
| Function                           | Return Type                              | Description                                                      |
| ---------------------------------- | :--------------------------------------- | :--------------------------------------------------------------- |
| `shared_from_this()`               | std::shared_ptr\<[HailoROI](#hailoroi)\> | Get a shared pointer to this instance.                           |
| `get_type()`                       | [hailo_object_t](#enumerations)          | This [HailoObject](#hailoobject)'s type: HAILO_ROI               |
| `add_object(HailoObjectPtr obj)`   | void                                     | Get the bbox of this ROI.                                        |
| `get_bbox()`                       | [HailoBBox](#hailobbox)                  | Get a shared pointer to this instance.                           |
| `set_bbox(HailoBBox bbox)`         | void                                     | Set the bbox of this ROI.                                        |
| `get_scaling_bbox()`               | [HailoBBox](#hailobbox)                  | Get the scaling bbox of this ROI, useful in case of nested ROIs. |
| `set_scaling_bbox(HailoBBox bbox)` | void                                     | Set the scaling bbox of this ROI, useful in case of nested ROIs. |

<br/>
<br/>

# **NewHailoDetection**
**Inherits from [HailoROI](#hailoroi).**  
`NewHailoDetection` represents a detection in an ROI. It is assumed that all numbers are normalized (between 0 and 1) so that objects remain in relative size for easy image resizing.  
`Shared pointer handle`: **NewHailoDetectionPtr**  \
`SOURCE`: [core/hailo/general/hailo_objects.hpp](../../core/hailo/general/hailo_objects.hpp)  
## Constructors
```cpp
NewHailoDetection(HailoBBox bbox, const std::string &label, float confidence)
NewHailoDetection(HailoBBox bbox, int class_id, const std::string &label, float confidence)
```

## Functions
| Function                                    | Return Type                     | Description                                              |
| ------------------------------------------- | :------------------------------ | :------------------------------------------------------- |
| `get_type()`                                | [hailo_object_t](#enumerations) | This [HailoObject](#hailoobject)'s type: HAILO_DETECTION |
| `get_confidence()`                          | float                           | This detection's confidence.                             |
| `get_label()`                               | std::string                     | This detection's label.                                  |
| `get_class_id()`                            | int                             | This detection's class id.                               |
| `operator<(const NewHailoDetection &other)` | bool                            | Overload < operator, compares confidences.               |
| `operator>(const NewHailoDetection &other)` | bool                            | Overload > operator, compares confidences.               |

<br/>
<br/>

# **HailoClassification**
**Inherits from [HailoObject](#hailoobject).**  
`HailoClassification` represents a classification of an ROI. Classifications can have different `types`, for example a classification of type 'color' can have a `label` of red or blue.  
`Shared pointer handle`: **HailoClassificationPtr**  \
`SOURCE`: [core/hailo/general/hailo_objects.hpp](../../core/hailo/general/hailo_objects.hpp)  
## Constructors
```cpp
HailoClassification(const std::string &classification_type, const std::string &label, float confidence)
HailoClassification(const std::string &classification_type, int class_id, std::string label, float confidence)
```

## Functions
| Function                    | Return Type                     | Description                                                                       |
| --------------------------- | :------------------------------ | :-------------------------------------------------------------------------------- |
| `get_type()`                | [hailo_object_t](#enumerations) | This [HailoObject](#hailoobject)'s type: HAILO_CLASSIFICATION                     |
| `get_confidence()`          | float                           | This classification's confidence.                                                 |
| `get_label()`               | std::string                     | This classification's label (e.g. "Horse", "Monkey", "Tiger" for type "Animals"). |
| `get_classification_type()` | std::string                     | This classification's type (e.g. "age", "gender", "color", etc...).               |
| `get_class_id()`            | int                             | This classification's class id.                                                   |

<br/>
<br/>

# **HailoLandmarks**
**Inherits from [HailoObject](#hailoobject).**  
`HailoLandmarks` represents **a set** of landmarks on a given ROI. Like [HailoClassifications](#hailoclassification), [HailoLandmarks](#hailolandmarks) can also have different `types`, for example a landmark can be of type "pose" or "facial landmarking". Each landmark in the set is represented as a [HailoPoint](#hailopoint).  
`Shared pointer handle`: **HailoLandmarksPtr**  \
`SOURCE`: [core/hailo/general/hailo_objects.hpp](../../core/hailo/general/hailo_objects.hpp)  
## Constructors
```cpp
HailoLandmarks(std::string landmarks_name, float threshold = 0.0f, const std::vector<std::pair<int, int>> pairs = {})
std::string landmarks_name, std::vector<HailoPoint> points, float threshold = 0.0f, const std::vector<std::pair<int, int>> pairs = {})
```

## Functions
| Function                      | Return Type                              | Description                                                             |
| ----------------------------- | :--------------------------------------- | :---------------------------------------------------------------------- |
| `get_type()`                  | [hailo_object_t](#enumerations)          | This [HailoObject](#hailoobject)'s type: HAILO_LANDMARKS                 |
| `add_point(HailoPoint point)` | void                                     | Add a point to this landmarks object.                         |
| `get_points()`                | std::vector\<[HailoPoint](#hailopoint)\> | Gets the set of points held by this Landmarks object.                   |
| `get_landmarks_type()`        | std::string                              | This landmark's type (e.g. "pose estimation", "face landmark", etc...). |
| `get_pairs()`                 | std::vector<std::pair<int, int>>         | vector of pairs of joints that should be connected in overlay. |

<br/>
<br/>

# **HailoUniqueID**
**Inherits from [HailoObject](#hailoobject).**  
`HailoUniqueID` represents a unique id of an ROI. Sometimes we may want to give ROIs unique ids (for example, when tracking detections), and having a [HailoObject](#hailoobject) abstraction makes adding and removing ids very simple (via `add_object()` and `remove_object()`). If no unique if is provided at construction, then a default -1 is used.  
`Shared pointer handle`: **HailoUniqueIDPtr**  \
`SOURCE`: [core/hailo/general/hailo_objects.hpp](../../core/hailo/general/hailo_objects.hpp)  
## Constructors
```cpp
HailoUniqueID()
HailoUniqueID(int unique_id)
```

## Functions
| Function     | Return Type                     | Description                                              |
| ------------ | :------------------------------ | :------------------------------------------------------- |
| `get_type()` | [hailo_object_t](#enumerations) | This [HailoObject](#hailoobject)'s type: HAILO_UNIQUE_ID |
| `get_id()`   | int                             | Get the unique id.                                       |

<br>
<br>

# **HailoMask**
**Inherits from [HailoObject](#hailoobject).**  
`HailoMask` represents a mask of an ROI. Whenever the output of a postprocess is masks (tensors with result for every pixel) we will ROIs mask objects.   
`Shared pointer handle`: **HailoMaskPtr**  \
`SOURCE`: [core/hailo/general/hailo_objects.hpp](../../core/hailo/general/hailo_objects.hpp)  
## Constructors
```cpp
HailoMask(int mask_width, int mask_height, float transparency)
```

## Functions
| Function     | Return Type                     | Description                                              |
| ------------ | :------------------------------ | :------------------------------------------------------- |
| `get_type()` | [hailo_object_t](#enumerations) | This [HailoObject](#hailoobject)'s type: HAILO_MASK |
| `get_width()` | int | get the mask width |
| `get_height()`   | int                             | get the mask height                                   |
| `get_transparency()`  |float|  get the desired drawing transparency                                |

<br>
<br>

# **HailoDepthMask**
**Inherits from [HailoMask](#hailomask).**  
`HailoDepthMask` represents a mask of an ROI, with float values for each pixel. The values represent depth between minimum and maximum values.   
`Shared pointer handle`: **HailoDepthMaskPtr**  \
`SOURCE`: [core/hailo/general/hailo_objects.hpp](../../core/hailo/general/hailo_objects.hpp)  
## Constructors
```cpp
HailoDepthMask(std::vector<float> &&data_vec, int mask_width, int mask_height, float transparency)
```

## Functions
| Function     | Return Type                     | Description                                              |
| ------------ | :------------------------------ | :------------------------------------------------------- |
| `get_type()` | [hailo_object_t](#enumerations) | This [HailoObject](#hailoobject)'s type: HAILO_DEPTH_MASK |
| `get_data()` | const std::vector<float> | get the mask data vector |

<br>
<br>

# **HailoClassMask**
**Inherits from [HailoMask](#hailomask).**  
`HailoClassMask` represents a mask of an ROI, with uint8_t class id classification for each pixel.    
`Shared pointer handle`: **HailoClassMaskPtr**  \
`SOURCE`: [core/hailo/general/hailo_objects.hpp](../../core/hailo/general/hailo_objects.hpp)  
## Constructors
```cpp
HailoClassMask(std::vector<uint8_t> &&data_vec, int mask_width, int mask_height, float transparency) 
```

## Functions
| Function     | Return Type                     | Description                                              |
| ------------ | :------------------------------ | :------------------------------------------------------- |
| `get_type()` | [hailo_object_t](#enumerations) | This [HailoObject](#hailoobject)'s type: HAILO_CLASS_MASK |
| `get_data()` | const std::vector<uint8_t>  | get the mask data vector |

<br>
<br>

# **HailoConfClassMask**
**Inherits from [HailoMask](#hailomask).**  
`HailoConfClassMask` represents a mask of an ROI, contains mask-class-id and confidence float value for each pixel.
`Shared pointer handle`: **HailoConfClassMaskPtr**  \
`SOURCE`: [core/hailo/general/hailo_objects.hpp](../../core/hailo/general/hailo_objects.hpp)  
## Constructors
```cpp
HailoConfClassMask(std::vector<float> &&data_vec, int mask_width, int mask_height, float transparency, int class_id)
```

## Functions
| Function     | Return Type                     | Description                                              |
| ------------ | :------------------------------ | :------------------------------------------------------- |
| `get_type()` | [hailo_object_t](#enumerations) | This [HailoObject](#hailoobject)'s type: HAILO_CONF_CLASS_MASK |
| `get_data()` | const std::vector<float>  | get the mask data vector |
| `get_class_id()` | int | get the mask class id |


<br>
<br>

# **HailoMatrix**
**Inherits from [HailoObject](#hailoobject).**  
`HailoMatrix` represents a matrix, contains float values. This matrix can be added to any HailoObject for different use cases. 
`Shared pointer handle`: **HailoMatrixPtr**  \
`SOURCE`: [core/hailo/general/hailo_objects.hpp](../../core/hailo/general/hailo_objects.hpp)  
## Constructors
```cpp
    HailoMatrix(float *data_ptr, uint32_t mat_height, uint32_t mat_width, uint32_t mat_features = HailoMatrix::DEFAULT_NUMBER_OF_FEATURES) 
```

## Functions
| Function     | Return Type                     | Description                                              |
| ------------ | :------------------------------ | :------------------------------------------------------- |
| `get_type()` | [hailo_object_t](#enumerations) | This [HailoObject](#hailoobject)'s type: HAILO_MATRIX |
| `width()` | const uint32_t | get matrix width |
| `height()` | const uint32_t | get matrix height |
| `features()` | const uint32_t | get matrix number of features |
| `size()` | const uint32_t | get number of elements in matrix |
| `shape()` | std::vector<std::size_t> | get the shape of the matrix |
| `get_data_ptr()` | float * | get the matrix data pointer |

