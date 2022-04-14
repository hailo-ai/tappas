/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#ifndef _HAILO_CENTERPOSE_POST_HPP_
#define _HAILO_CENTERPOSE_POST_HPP_
#include "hailo_frame.hpp"

G_BEGIN_DECLS
void filter(HailoFramePtr hailo_frame);
void centerpose(HailoFramePtr hailo_frame);
void centerpose_416(HailoFramePtr hailo_frame);
void centerpose_merged(HailoFramePtr hailo_frame);
G_END_DECLS
#endif