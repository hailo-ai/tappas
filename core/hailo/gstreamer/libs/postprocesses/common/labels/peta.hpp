/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/

#pragma once
#include <map>
#include <stdint.h>
#include <string>

namespace labels
{
    static std::map<uint8_t, std::string> peta = {
        {0, "Age16-30"},
        {1, "Age31-45"},
        {2, "Age46-60"},
        {3, "AgeAbove61"},
        {4, "Backpack"},
        {5, "CarryingOther"},
        {6, "Casual lower"},
        {7, "Casual upper"},
        {8, "Formal lower"},
        {9, "Formal upper"},
        {10, "Hat"},
        {11, "Jacket"},
        {12, "Jeans"},
        {13, "Leather shoes"},
        {14, "Logo"},
        {15, "Long hair"},
        {16, "Male"},
        {17, "Messenger bag"},
        {18, "Muffler"},
        {19, "No accesory"},
        {20, "No carrying"},
        {21, "Plaid"},
        {22, "Plastic bag"},
        {23, "Sandals"}, 
        {24, "Shoes"},
        {25, "Shorts"},
        {26, "Short sleeve"},
        {27, "Skirt"},
        {28, "Sneaker"},
        {29, "Stripes"},
        {30, "Sunglasses"},
        {31, "Trousers"},
        {32, "T-shirt"},
        {33, "UpperOther"},
        {34, "V-Neck"}};
}