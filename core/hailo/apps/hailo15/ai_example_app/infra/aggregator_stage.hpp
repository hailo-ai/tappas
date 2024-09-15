#pragma once

#include "stage.hpp"
#include "buffer.hpp"
#include "hailo_common.hpp"
#include <algorithm>

class AggregatorStage : public ConnectedStage
{
protected:
    bool m_blocking;
    std::string m_main_inlet_name;
    size_t m_main_queue_size;
    std::string m_sub_inlet_name;
    size_t m_sub_queue_size;

public:
    AggregatorStage(std::string name, bool blocking, 
                    std::string main_inlet_name, size_t main_queue_size, 
                    std::string sub_inlet_name, size_t sub_queue_size, 
                    bool leaky=false, bool print_fps=false) : 
        ConnectedStage(name, main_queue_size, leaky, print_fps), m_blocking(blocking), 
        m_main_inlet_name(main_inlet_name), m_main_queue_size(main_queue_size), 
        m_sub_inlet_name(sub_inlet_name), m_sub_queue_size(sub_queue_size)
        {
            m_queues.push_back(std::make_shared<Queue>(m_main_inlet_name, m_main_queue_size, leaky));
            m_queues.push_back(std::make_shared<Queue>(m_sub_inlet_name, m_sub_queue_size, leaky));
        }

    void add_queue(std::string name) override {}

    HailoBBox create_flattened_bbox(const HailoBBox &bbox, const HailoBBox &parent_bbox)
    {
        float xmin = parent_bbox.xmin() + bbox.xmin() * parent_bbox.width();
        float ymin = parent_bbox.ymin() + bbox.ymin() * parent_bbox.height();

        float width = bbox.width() * parent_bbox.width();
        float height = bbox.height() * parent_bbox.height();

        return HailoBBox(xmin, ymin, width, height);
    }

    void flatten_hailo_roi(HailoROIPtr roi, HailoROIPtr parent_roi, hailo_object_t filter_type)
    {
        std::vector<HailoObjectPtr> objects = roi->get_objects();
        for (uint index = 0; index < objects.size(); index++)
        {
            if (objects[index]->get_type() == filter_type)
            {
                HailoROIPtr sub_obj_roi = std::dynamic_pointer_cast<HailoROI>(objects[index]);
                sub_obj_roi->set_bbox(std::move(create_flattened_bbox(sub_obj_roi->get_bbox(), roi->get_scaling_bbox())));
                parent_roi->add_object(sub_obj_roi);
                roi->remove_object(index);
                objects.erase(objects.begin() + index);
                index--;
            }
        }
    }

    void loop() override
    {
        init();

        while (!m_end_of_stream)
        {
            // the first queue (first to subscribe) is the one that is condisidered the "main stream"
            BufferPtr main_buffer = m_queues[0]->pop();
            if (main_buffer == nullptr && m_end_of_stream)
            {
                break;
            }

            // Check if the main buffer has cropping metadata
            int num_subframes = 0; // If no cropping meta provided, then no subframes are requested
            std::vector<MetadataPtr> metadata = main_buffer->get_metadata_of_type(MetadataType::EXPECTED_CROPS);
            if (metadata.size() > 0)
            {
                CroppingMetadataPtr cropping_metadata = std::dynamic_pointer_cast<CroppingMetadata>(metadata[0]);
                num_subframes = cropping_metadata->get_num_crops();
                main_buffer->remove_metadata(cropping_metadata);
            }
            // If no subframes are requested, send the main buffer as is
            if (num_subframes == 0)
            {
                main_buffer->add_time_stamp(m_stage_name);
                set_duration(main_buffer);
                send_to_subscribers(main_buffer);
                continue;
            }

            //Check how many sub frames are available, wait if blocking is enabled
            std::vector<BufferPtr> subframes;
            if (m_blocking)
            {
                for (int i = 0; i < num_subframes; i++)
                {
                    subframes.push_back(m_queues[1]->pop());
                    if (subframes[i] == nullptr && m_end_of_stream)
                    {
                        deinit();
                        return;
                    }
                }
            }
            else 
            {
                // leaky case
                if (m_queues[1]->size() >= num_subframes)
                {
                    for (int i = 0; i < num_subframes; i++)
                    {
                        subframes.push_back(m_queues[1]->pop());
                        if (subframes[i] == nullptr && m_end_of_stream)
                        {
                            deinit();
                            return;
                        }
                    }
                } else {
                    // if not blocking and not enough subframes, then continue
                    main_buffer->add_time_stamp(m_stage_name);
                    set_duration(main_buffer);
                    send_to_subscribers(main_buffer);
                    continue;
                }
            }

            if (m_print_fps && !m_first_fps_measured)
            {
                m_last_time = std::chrono::steady_clock::now();
                m_first_fps_measured = true;
            }

            // copy metadata from subframes to main frame
            //for (auto BufferPtr subframe : subframes)
            for (BufferPtr subframe : subframes)
            {
                // Flatten subframe roi detections to main_buffer roi's scales.
                // Passing HAILO_DETECTION as a filter type here request to flatten only HailoDetection objects.
                // This passes ownership of the rois to the main buffer
                flatten_hailo_roi(subframe->get_roi(), main_buffer->get_roi(), HAILO_DETECTION);
            }

            main_buffer->add_time_stamp(m_stage_name);
            // pass the main_buffer to the subscribers
            set_duration(main_buffer);
            send_to_subscribers(main_buffer);

            if (m_print_fps)
            {
                m_counter++;
                print_fps();
            }
        }
    
        deinit();
    }

};