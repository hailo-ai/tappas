/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#ifndef _HAILO_CITYSCAPES19_HPP_
#define _HAILO_CITYSCAPES19_HPP_
#include <map>
namespace common
{
    static const uint8_t cityscapes19_colors[] = {
        128, 64, 128,  // purple - road 1
        244, 35, 232,  // magenta - sidewalk 2
        70, 70, 70,    // grey - building 3
        102, 102, 156, // matte blue - wall 4
        190, 153, 153, // brown - fence 5
        153, 153, 153, // grey green - pole 6
        250, 170, 30,  // gold - trafficliight 7
        220, 220, 0,   // light green - trafficsign 8
        107, 142, 35,  // olive green - vegetation 9
        152, 251, 152, // neon green - terrain 10
        70, 130, 180,  // pastel blue - sky 11
        220, 20, 60,   // pastel red - person 12
        255, 0, 0,     // red - rider 13
        0, 0, 142,     // blue - car 14
        0, 0, 70,      // navy - truck 15
        0, 60, 100,    // royal blue - bus 16
        0, 80, 100,    // green blue - train 17
        0, 0, 230,     // neon blue - motorcycle 18
        119, 11, 32,   // dark red - bicycle 19
    };
}
#endif /* _HAILO_CITYSCAPES19_HPP_ */