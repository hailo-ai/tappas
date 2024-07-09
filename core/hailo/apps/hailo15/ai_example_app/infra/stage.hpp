#pragma once

// General includes
#include <queue>
#include <mutex>
#include <thread>
#include <shared_mutex>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

// Tappas includes
#include "hailo_objects.hpp"
#include "hailo_common.hpp"

// Media library includes
#include "media_library/media_library_types.hpp"

// Infra includes
#include "buffer.hpp"
#include "queue.hpp"

enum class AppStatus
{
    SUCCESS = 0,
    INVALID_ARGUMENT,
    CONFIGURATION_ERROR,
    BUFFER_ALLOCATION_ERROR,
    HAILORT_ERROR,
    DSP_OPERATION_ERROR,
    UNINITIALIZED,
    PIPELINE_ERROR,
    DMA_ERROR
};

class Stage
{
protected:
    bool m_end_of_stream = false;
    std::unique_ptr<std::condition_variable> m_condvar;
    std::shared_ptr<std::mutex> m_mutex;
    std::string m_stage_name;
    std::thread m_thread;

    // FPS Related memebers
    bool m_print_fps = false;
    bool m_first_fps_measured = false;
    std::chrono::steady_clock::time_point m_last_time;
    std::chrono::duration<double, std::micro> m_duration;
    
    int m_counter = 0;

public:
    Stage(std::string name, bool print_fps) : m_stage_name(name), m_print_fps(print_fps)
    {
        m_mutex = std::make_shared<std::mutex>();
        m_condvar = std::make_unique<std::condition_variable>();
    }

    virtual ~Stage() = default;

    std::string get_name()
    {
        return m_stage_name;
    }

    virtual AppStatus start()
    {
        m_end_of_stream = false;
        m_thread = std::thread(&Stage::loop, this);
        return AppStatus::SUCCESS;
    }

    virtual AppStatus stop()
    {
        set_end_of_stream(true);
        m_thread.join();
        return AppStatus::SUCCESS;
    }

    virtual AppStatus init()
    {
        return AppStatus::SUCCESS;
    }

    virtual AppStatus deinit()
    {
        return AppStatus::SUCCESS;
    }

    virtual void add_queue(std::string name){};

    virtual void push(BufferPtr buffer, std::string caller_name){};

    virtual void loop(){};

    virtual AppStatus process(BufferPtr buffer)
    {
        return AppStatus::SUCCESS;
    }

    virtual void set_end_of_stream(bool end_of_stream)
    {
        m_end_of_stream = end_of_stream;
    }

    void set_print_fps(bool print_fps)
    {
        m_print_fps = print_fps;
    }
    std::chrono::duration<double, std::micro> get_duration()
    {
        return m_duration;
    }

    void set_duration(BufferPtr buff)
    {
       if(buff->get_num_stages() >= 2)
       {
        std::chrono::steady_clock::time_point ts_start = buff->get_time_stamp(buff->get_num_stages()-2);
        std::chrono::steady_clock::time_point ts_end = buff->get_time_stamp(buff->get_num_stages()-1);
        m_duration = std::chrono::duration_cast<std::chrono::microseconds>(ts_end - ts_start);
       }
        
    }

    void print_fps()
    {
        std::chrono::steady_clock::time_point current_time = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed_seconds = current_time - m_last_time;

        if (elapsed_seconds.count() >= 1.0) {
            std::cout << "[ " << m_stage_name << " ] Buffers processed per second: " << m_counter << std::endl;
            m_counter = 0;
            m_last_time = current_time;
        }
    }
};
using StagePtr = std::shared_ptr<Stage>;

class ConnectedStage;
using ConnectedStagePtr = std::shared_ptr<ConnectedStage>;
class ConnectedStage : public Stage
{
protected:
    size_t m_queue_size;
    bool m_leaky;
    std::vector<QueuePtr> m_queues;
    std::vector<ConnectedStagePtr> m_subscribers;

public:
    ConnectedStage(std::string name, size_t queue_size, bool leaky=false, bool print_fps=false) :
        Stage(name, print_fps), m_queue_size(queue_size), m_leaky(leaky)
    {
    }

    void add_queue(std::string name) override
    {
        m_queues.push_back(std::make_shared<Queue>(name, m_queue_size, m_leaky));
    }

    void add_subscriber(ConnectedStagePtr subscriber)
    {
        m_subscribers.push_back(subscriber);
        subscriber->add_queue(m_stage_name);
    }

    void push(BufferPtr data, std::string caller_name) override
    {
        for (auto &queue : m_queues)
        {
            if (queue->name() == caller_name)
            {
                queue->push(data);
                break;
            }
        }
        m_condvar->notify_one();
    }

    void set_end_of_stream(bool end_of_stream)
    {
        m_end_of_stream = end_of_stream;
        if (end_of_stream)
        {
            for (auto &queue : m_queues)
            {
                queue->flush();
            }
        }
    }

    void send_to_subscribers(BufferPtr data)
    {
        for (auto &subscriber : m_subscribers)
        {
            subscriber->push(data, m_stage_name);
        }
    }

    void send_to_specific_subsciber(std::string stage_name, BufferPtr data)
    {
        for (auto &subscriber : m_subscribers)
        {
            if (stage_name == subscriber->get_name())
            {
                subscriber->push(data, m_stage_name);
            }
        } 
    }

    void loop() override
    {
        init();

        while (!m_end_of_stream)
        {
            BufferPtr data = m_queues[0]->pop(); // The first connected queue is always considered "main stream"
            if (data == nullptr && m_end_of_stream)
            {
                break;
            }

            if (m_print_fps && !m_first_fps_measured)
            {
                m_last_time = std::chrono::steady_clock::now();
                m_first_fps_measured = true;
            }

            process(data);

            if (m_print_fps)
            {
                m_counter++;
                print_fps();
            }
        }

        deinit();
    }

};

class CallbackStage : public ConnectedStage
{
protected:
    std::function<void(BufferPtr)> m_callback;
public:
    CallbackStage(std::string name, size_t queue_size, bool leaky=false, std::function<void(BufferPtr)> callback=NULL, bool print_fps=false) :
        ConnectedStage(name, queue_size, leaky, false), m_callback(callback) {}

    AppStatus process(BufferPtr data) override
    {
        if (m_callback)
            m_callback(data);
        
        
        data->add_time_stamp(m_stage_name);
        set_duration(data);

        return AppStatus::SUCCESS;
    }

    void set_callback(std::function<void(BufferPtr)> callback)
    {
        m_callback = callback;
    }
};
using CallbackStagePtr = std::shared_ptr<CallbackStage>;

class AggregatorStage : public ConnectedStage
{
protected:
    bool m_blocking;

public:
    AggregatorStage(std::string name, bool blocking=true, size_t queue_size=5, bool leaky=false, bool print_fps=false) : 
        ConnectedStage(name, queue_size, leaky, print_fps), m_blocking(blocking) {}

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

            // If no subframes are added then pass the buffer and continue
            if (m_queues.size() == 1)
            {
                main_buffer->add_time_stamp(m_stage_name);
                set_duration(main_buffer);
                send_to_subscribers(main_buffer);
                continue;
            }

            CroppingMetadataPtr cropping_metadata = std::dynamic_pointer_cast<CroppingMetadata>(main_buffer->get_metadata_of_type(MetadataType::EXPECTED_CROPS)[0]);
            int num_subframes = cropping_metadata->get_num_crops();
            main_buffer->remove_metadata(cropping_metadata);
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