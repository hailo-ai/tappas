/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#ifndef _HAILO_YOLACT20_HPP_
#define _HAILO_YOLACT20_HPP_
#include <map>
#include <vector>
namespace common
{
    static std::map<uint8_t, std::vector<uint8_t>> yolact20_colors = {
        {0, {70, 70, 70}},     // grey - background 0
        {1, {220, 20, 60}},    // pastel red - person 1
        {2, {119, 11, 32}},    // dark red - bicycle 2
        {3, {107, 142, 35}},   // olive green - bench 3
        {4, {128, 64, 128}},   // purple - backpack 4
        {5, {244, 35, 232}},   // magenta - handbag 5
        {6, {102, 102, 156}},  // matte blue - tie 6
        {7, {190, 153, 153}},  // brown - suitcase 7
        {8, {255, 255, 0}},    // yellow - bottle 8
        {9, {250, 170, 30}},   // gold - cup 9
        {10, {220, 220, 0}},   // light green - bowl 10
        {11, {255, 0, 255}},   // magenta - chair 11
        {12, {152, 251, 152}}, // neon green - couch 12
        {13, {127, 255, 0}},   // green blue - potted_plant 13
        {14, {0, 0, 70}},      // navy - dinning table 14
        {15, {255, 165, 0}},   // orange - tv 15
        {16, {0, 0, 142}},     // blue - laptop 16
        {17, {0, 0, 70}},      // navy - keyboard 17
        {18, {255, 128, 0}},   // dark orange - cellphone 18
        {19, {0, 0, 230}},     // neon blue - book 19
    };
}
#endif /* _HAILO_YOLACT20_HPP_ */