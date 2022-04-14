# Writing Your Own Postprocess

## Table of Contents

- [Writing Your Own Postprocess](#writing-your-own-postprocess)
  - [Table of Contents](#table-of-contents)
  - [Overview](#overview)
  - [Getting Started](#getting-started)
    - [Where are all the files?](#where-are-all-the-files)
    - [Preparing the Header File: Default Filter Function](#preparing-the-header-file-default-filter-function)
    - [Implementing filter()](#implementing-filter)
  - [Compiling and Running](#compiling-and-running)
    - [Building with Meson](#building-with-meson)
    - [Compiling the .so](#compiling-the-so)
    - [Running the .so](#running-the-so)
  - [Filter Basics](#filter-basics)
    - [Working with Tensors](#working-with-tensors)
    - [Attaching Detection Objects to the Frame](#attaching-detection-objects-to-the-frame)
  - [Next Steps](#next-steps)
    - [Drawing](#drawing)
    - [Multiple Filters in One .so](#multiple-filters-in-one-so)

## Overview

If you want to add a network to the Tappas that is not already supported, then you will likely need to implement a new postprocess and drawing filter. Fortunately with the use of the [hailofilter2](../elements/hailo_filter2.md), you don't need to create any new GStreamer elements, just provide the .so (compiled shared object binary) that applies your filter! \
In this guide we will go over how to create such an .so and what mechanisms/structures are available to you as you create your postprocess.

## Getting Started

### Where are all the files?

To begin your postprocess writng journey, it is good to understand where you can find all the relevant source files that already exist, and how to add your own. \
From the Tappas home directory, you can find the `core/` folder. Inside this `core/` directory are a few subdirectories that host different types of source files. The `open_source` folder contains source files from 3rd party libraries (opencv, xtensor, etc..), while the `hailo` folder contains source files for all kinds of Hailo tools, such as the Hailo Gstreamer elements, the different metas provided, and the source files for the postprocesses of the networks that were already provided in the Tappas. Inside this directory is one titled `general/`, which contains sources for [the different object classes](hailo-objects-api.md) (detections, classifications, etc..) available. Next to `general` is a directory titled `gstreamer/`, and inside that are two folders of interest: `libs/` and `plugins/`. The former contains the source code for all the postprocess and drawing functions packaged in the Tappas, while the latter contains source code for the different Hailo GStreamer elements, and the different metas available. This guide will mostly focus on this `core/hailo/gstreamer/` directory, as it has everything we need to create and compile a new .so! You can take a moment to peruse around, when you are ready to continue enter the `postprocesses/` directory:

<div align="center">
    <img src="../resources/core_hierarchy.png"/>
</div>

### Preparing the Header File: Default Filter Function

We can create our new postprocess here in the `postprocesses/` folder. Create a new header file named `my_post.hpp`.\
In the first lines we want to import useful classes to our postprocess, so add the following includes:

```cpp
#pragma once
#include "hailo_objects.hpp"
#include "hailo_common.hpp"
```
`"hailo_objects.hpp"` contains classes that represent the [different inputs (tensors) and outputs](hailo-objects-api.md) (detections, classifications, etc...) that your postprocess might handle. You can find the header in [core/hailo/general/hailo_objects.hpp](../../core/hailo/general/hailo_objects.hpp). Your main point of entry for data in the postprocess is the `HailoROI`, which can have a tensor or a number of tensors attached. `"hailo_common.hpp"` provides common useful functions for handling these classes. Let's wrap up the header file by adding a function prototype for our filter, your whole header file should look like:

```cpp
#pragma once
#include "hailo_objects.hpp"
#include "hailo_common.hpp"

__BEGIN_DECLS
void filter(HailoROIPtr roi);
__END_DECLS
```

Yes really, that's it! The `hailofilter2` element does not expect much, just that the above `filter` function be provided. We will discuss [adding multiple filters in the same .so](#multiple-filters-in-one-so) later. Note that the `filter` function takes a `HailoROIPtr` as a parameter; this will provide you with the `HailoROI` of each passing image.

### Implementing filter()

Let's start implementing the actual filter so that you can see how to access and work with tensors. Start by creating a new file called `my_post.cpp`. Open it and include the following:

```cpp
#include <iostream>
#include "my_post.hpp"
```
The `<iostream>` will allow us to print to the console, and the `"my_post.hpp"` includes the header file we just wrote.  
For now add the follwoing implmentation for filter() so that we have a working postprocess we can test:

```cpp
// Default filter function
void filter(HailoROIPtr roi)
{
  std::cout << "My first postprocess!" << std::endl;
}
```

That should be enough to try compiling and running a pipeline! Next we will see how to add our postprocess to the meson project so that it compiles.

## Compiling and Running

### Building with Meson

[Meson](https://mesonbuild.com/) is an open source build system that puts an emphasis on speed and ease of use. [GStreamer uses meson](https://gstreamer.freedesktop.org/documentation/installing/building-from-source-using-meson.html?gi-language=c) for all subprojects to generate build instructions to be executed by [ninja](https://ninja-build.org/), another build system focused soley on speed that requires a higher level build system (ie: meson) to generate its input files. \
Like GStreamer, Tappas also uses meson, and compiling new projects requires adjusting the `meson.build` files. Here we will discuss how to add yours. \
In the `gstreamer/libs/` path you will find a [meson.build](../../core/hailo/gstreamer/libs/meson.build), open it and add the following entry for our postprocess:

```cpp
################################################
# MY POST SOURCES
################################################
my_post_sources = [
  'postprocesses/my_post.cpp',
]

my_post_lib = shared_library('my_post',
  my_post_sources,
  cpp_args : hailo_lib_args,
  include_directories: [hailo_general_inc] + xtensor_inc,
  dependencies : post_deps,
  gnu_symbol_visibility : 'default',
)
```

This should give meson all the information it needs to compile our postprocess. In short, we are providing paths to cpp compilers, linked libraries, included directories, and dependencies. Where are all these path variables coming from? Great question: from the parent meson project, you can read that meson file to see what packages and directories are available at [core/hailo/gstreamer/meson.build](../../core/hailo/gstreamer/meson.build).

### Compiling the .so

You should now be ready to compile your postprocess. To help streamline this process we have gone ahead and provided a script that handles most of the work. You can find this script at [scripts/gstreamer/install_hailo_gstreamer.sh](../../scripts/gstreamer/install_hailo_gstreamer.sh). This script includes some flags that allow you do more specific operations, but they are not needed right now.  
From the Tappas home directory folder you can run:

```sh
./docker/scripts/gstreamer/install_hailo_gstreamer.sh
```

<div align="center">
    <img src="../resources/compiling.png"/>
</div>

If all goes well you should see some happy green `YES`, and our .so should appear in `apps/gstreamer/x86/libs/`!
<div align="center">
    <img src="../resources/my_post_so.png"/>
</div>

### Running the .so

Congratulations! You've compiled your first postprocess! Now you are ready to run the postprocess and see the results. Since it is still so generic, we can try it. Run this test pipeline in your terminal to see if it works:

```sh
gst-launch-1.0 videotestsrc ! hailofilter2 so-path=$TAPPAS_WORKSPACE/apps/gstreamer/x86/libs/libmy_post.so ! fakesink
```

See in the above pipeline that we gave the `hailofilter2` the path to `libmy_post.so` in the `so-path` property. So now every time a buffer is received in that `hailofilter2`'s sink pad, it calls the `filter()` function in `libmy_post.so`. The resulting app should print our chosen text `"My first postprocess!"` in the console:
<div align="center">
    <img src="../resources/my_first_post.png"/>
</div>

## Filter Basics

### Working with Tensors

Printing statements on every buffer is cool and all, but we would like a postprocess that can actually do operations on inference tensors. Let's take a look at how we can do that. \
Head back to `my_post.cpp` and swap our print statement with the following:

```cpp
// Get the output layers from the hailo frame.
std::vector<NewHailoTensorPtr> tensors = roi->get_tensors();
```

The `HailoROI` has two ways of providing the output tensors of a network: via the `get_tensors()` and `get_tensor(std::string name)` functions. The first (which we used here) returns an `std::vector` of `NewHailoTensorPtr` objects. These are an `std::shared_ptr` to a `NewHailoTensor`: a class that represents an output tensor of a network. `NewHailoTensor` holds all kinds of important tensor metadata besides the data itself; such as the width, height, number of channels, and even quantization parameters. You can see the full implementation for this class at [core/hailo/general/hailo_tensors.hpp](../../core/hailo/general/hailo_tensors.hpp). \
`get_tensor(std::string name)` also returns a `NewHailoTensorPtr`, but only the one with  the given name output layer name. This can be convenient if you want to perform operations on specific layers whose names you know in advanced. \
\
So now that we have a vector of `NewHailoTensorPtr` objects, lets get some information out of one. Add the following lines to our `filter()` function:

```cpp
// Get the first output tensor
NewHailoTensorPtr first_tensor = tensors[0];
std::cout << "Tensor: " << first_tensor->name();
std::cout << " has width: " << first_tensor->shape()[0];
std::cout << " height: " << first_tensor->shape()[1];
std::cout << " channels: " << first_tensor->shape()[2] << std::endl;
```

Recompile with the same [script we used earlier](#compiling-the-so). Run a test pipeline, and this time see actual parameters of the tensor printed out:

```sh
gst-launch-1.0 filesrc location=$TAPPAS_WORKSPACE/apps/gstreamer/x86/detection/resources/detection.mp4 name=src_0 ! decodebin ! videoscale ! video/x-raw, pixel-aspect-ratio=1/1 ! videoconvert ! queue ! hailonet hef-path=$TAPPAS_WORKSPACE/apps/gstreamer/x86/detection/resources/yolov5m.hef debug=False is-active=true qos=false device-count=1 vdevice-key=1 ! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! hailofilter2 so-path=$TAPPAS_WORKSPACE/apps/gstreamer/x86/libs/libmy_post.so qos=false ! videoconvert ! fpsdisplaysink video-sink=ximagesink name=hailo_display sync=true text-overlay=false
```

<div align="center">
    <img src="../resources/tensor_data.png"/>
</div>

With a `NewHailoTensorPtr` in hand, you have everything you need to perform your postprocess operations. You can access the actual tensor values from the `NewHailoTensorPtr` with:

```cpp
auto first_tensor_data = first_tensor->data();
```

Keep in mind that at this point the data is of type `uint8_t`, You will have to dequantize the tensor to a `float` if you want the full precision. Luckily the quantization parameters (scale and zero point) are stored in the `NewHailoTensorPtr` and can be applied through `tensor->fix_scale(uint8_t num)`.

### Attaching Detection Objects to the Frame

Now that you know how to create a basic filter and access your inference tensor, let's take a look at how to add a detection object to your `hailo_frame`.\
Remove the prints from the `filter()` function and replace them with the following function call:

```cpp
std::vector<NewHailoDetectionPtr> detections = demo_detection_objects();
```

Here we are calling a function `demo_detection_objects()` that will return some detection objects. Copy the following function definition into your `my_post.cpp`:

```cpp
std::vector<NewHailoDetection> demo_detection_objects()
{
  std::vector<NewHailoDetection> objects; // The detection objects we will eventually return
  NewHailoDetection first_detection = NewHailoDetection(HailoBBox(0.2, 0.2, 0.2, 0.2), "person", 0.99);
  NewHailoDetection second_detection = NewHailoDetection(HailoBBox(0.6, 0.6, 0.2, 0.2), "person", 0.89);
  objects.push_back(first_detection);
  objects.push_back(second_detection);

  return objects;
}
```

In this function we are creating two instances of a `NewHailoDetection` and pushing them into a vector that we return. Note that when creating a `NewHailoDetection`, we give a series of parameters. The expected parameters are as follows:

```cpp
NewHailoDetection(HailoBBox bbox, const std::string &label, float confidence)
```
Where `HailoBBox` is a class that represents a bounding box, it is initialized as `HailoBBox(float xmin, float ymin, float width, float height)`.  
**NOTE:**  It is assumed that the `xmin, ymin, width, and height` given are a **percentage of the image size** (meaning, if the box is **half** as wide as the width of the image, then `width=0.5`). This protects the pipeline's ability to resize buffers without comprimising the correct relative size of the detection boxes. \
\
Looking back at the demo function we just introduced, we are adding two instances of `NewHailoDetection`: `first_detection` and `second_detection`. According to the parameters we saw, `first_detection` has an `xmin` 20% along the x axis, and a `ymin` 20% down the y axis. The `width` and `height` are also 20% of the image. The last two parameters, `label` and `confidence`, show that this instance has a 99% `confidence` for `label` person. \
\
Now that we have a couple of `NewHailoDetection`s in hand, lets add them to the original `HailoROIPtr`. There is a helper function we need in the [core/hailo/general/hailo_common.hpp](../../core/hailo/general/hailo_common.hpp) file that we included earlier in `my_post.hpp`.
This file will no doubt have other features you will find useful, so it is recommended to keep the file handy. \
With the include in place, let's add the following function call to the end of the `filter()` function:

```cpp
// Update the frame with the found detections.
hailo_common::add_detections(roi, detections);
```

This function takes a `HailoROIPtr` and a `NewHailoDetection` vector, then adds each `NewHailoDetection` to the `HailoROIPtr`. Now that our detections have been added to the `hailo_frame` our postprocess is done!  
To recap, our whole `my_post.cpp` should look like this:

```cpp
#include <iostream>
#include "my_post.hpp"

std::vector<NewHailoDetection> demo_detection_objects()
{
  std::vector<NewHailoDetection> objects; // The detection objects we will eventually return
  NewHailoDetection first_detection = NewHailoDetection(HailoBBox(0.2, 0.2, 0.2, 0.2), "person", 0.99);
  NewHailoDetection second_detection = NewHailoDetection(HailoBBox(0.6, 0.6, 0.2, 0.2), "person", 0.89);
  objects.push_back(first_detection);
  objects.push_back(second_detection);
  return objects;
}

// Default filter function
void filter(HailoROIPtr roi)
{
    std::vector<NewHailoTensorPtr> tensors = roi->get_tensors();
    
    std::vector<NewHailoDetection> detections = demo_detection_objects();
    hailo_common::add_detections(roi, detections);
}
```

Recompile again and run the test pipeline, if all goes well then you should see the original video run with no problems! But we still don't see any detections? Don't worry, they are attached to each buffer, however no overlay is drawing them onto the image itself. To see how our detection boxes can be drawn, read on to [Next Steps: Drawing](#drawing).

## Next Steps

### Drawing

At this point we have a working postprocess that attaches two detection boxes to each passing buffer. But how do we get the GStreamer pipeline to draw those boxes onto the image? We have provided a GStreamer element - [hailooverlay](../elements/hailo_overlay.md) - that draws any Hailo provided output classes (detections, classifications, landmarks, etc..) on the buffer, all you have to do is include it in your pipeline!  
The element should be added in the pipeline after the `hailofilter2` element with our postprocess.  
Now our pipeline should look like:

```sh
gst-launch-1.0 filesrc location=$TAPPAS_WORKSPACE/apps/gstreamer/x86/detection/resources/detection.mp4 name=src_0 ! decodebin ! videoscale ! video/x-raw, pixel-aspect-ratio=1/1 ! videoconvert ! queue ! hailonet hef-path=$TAPPAS_WORKSPACE/apps/gstreamer/x86/detection/resources/yolov5m.hef debug=False is-active=true qos=false device-count=1 vdevice-key=1 ! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! hailofilter2 so-path=$TAPPAS_WORKSPACE/apps/gstreamer/x86/libs/libmy_post.so qos=false ! queue ! hailooverlay ! videoconvert ! fpsdisplaysink video-sink=ximagesink name=hailo_display sync=true text-overlay=false
```

Run the expanded pipeline above to see the original video, but this time with the two detection boxes we added!
<div align="center">
    <img src="../resources/demo_detection.png"/>
</div>

As expected, both boxes are labeled as `person`, and each is shown with the assigned `confidence`. Obviously, the two boxes don't move or match any object in the video; this is because we hardcoded their values for the sake of this tutorial. It is up to you to extract the correct numbers from the inferred tensor of your network, as you can see among the postprocesses already implemented in the Tappas each network can be different. We hope that this guide gives you a strong starting point on your development journey, good luck!

### Multiple Filters in One .so

While the `hailofilter2` always calls on a `filter()` function by default, you can provide the element access to other functions in your `.so` to call instead. This may be of interest if you are developing a postprocess that applies to mutliple networks, but each network needs slightly different starting parameters (in the Tappas case, mutliple flavors of the [Yolo detection network are handled via the same .so](../../core/hailo/gstreamer/libs/postprocesses/yolo/yolo_postprocess.cpp)). \
So how do you do it? Simply by declaring the extra functions in the header file, then pointing the `hailofilter2` to that function via the `function-name` property. \
Let's look at the yolo networks as an example, open up [libs/postprocesses/yolo/yolo_postprocess.hpp](../../core/hailo/gstreamer/libs/postprocesses/yolo/yolo_postprocess.hpp) to see what functions are made available to the `hailofilter2`:

```cpp
#ifndef _HAILO_YOLO_POST_HPP_
#define _HAILO_YOLO_POST_HPP_
#include "hailo_objects.hpp"
#include "hailo_common.hpp"


__BEGIN_DECLS
void filter(HailoROIPtr roi);
void yolox(HailoROIPtr roi);
void yoloxx (HailoROIPtr roi);
void yolov3(HailoROIPtr roi);
void yolov4(HailoROIPtr roi);
void tiny_yolov4_license_plates(HailoROIPtr roi);
void yolov5(HailoROIPtr roi);
void yolov5_no_persons(HailoROIPtr roi);
void yolov5_counter(HailoROIPtr roi);
void yolov5_vehicles_only(HailoROIPtr roi);
__END_DECLS
#endif
```

Any of the functions declared here can be given as a `function-name` property to the `hailofilter` element. Condsider this pipeline for running the `Yolov5` network:

```sh
gst-launch-1.0 filesrc location=/local/workspace/tappas/apps/gstreamer/x86/detection/resources/detection.mp4 name=src_0 ! decodebin ! videoscale ! video/x-raw, pixel-aspect-ratio=1/1 ! videoconvert ! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! hailonet hef-path=/local/workspace/tappas/apps/gstreamer/x86/detection/resources/yolov5m.hef device-count=1 vdevice-key=1 debug=False is-active=true qos=false batch-size=1 ! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! hailofilter2 function-name=yolov5 so-path=/local/workspace/tappas/apps/gstreamer/x86/libs//libnew_yolo_post.so qos=false ! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! hailooverlay ! videoconvert ! fpsdisplaysink video-sink=xvimagesink name=hailo_display sync=false text-overlay=false
```

The `hailofilter2` above that performs the postprecess points to `libyolo_post.so` in the `so-path`, but it also includes the property `function-name=yolov5`. This lets the `hailofilter2` know that instead of the default `filter()` function it should call on the `yolov5` function instead.
