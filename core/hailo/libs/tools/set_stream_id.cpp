/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#include <iostream>
#include <string.h>

#include "set_stream_id.hpp"

void print_stream_id(HailoROIPtr roi)
{
    std::cout << "Stream ID: " << roi->get_stream_id() << std::endl;
}

char *init(std::string config_path, std::string func_name)
{
    // Use the config path as the new desired stream ID.
    return strdup(config_path.c_str());
}
void set_stream_id(HailoROIPtr roi, void *params)
{
    char *stream_id = reinterpret_cast<char *>(params);
    roi->set_stream_id(std::string(stream_id));
}

void filter(HailoROIPtr roi, void *params)
{
    set_stream_id(roi, params);
}

void free_resources(void *params_void_ptr)
{
    char *stream_id = reinterpret_cast<char *>(params_void_ptr);
    free(stream_id);
}
