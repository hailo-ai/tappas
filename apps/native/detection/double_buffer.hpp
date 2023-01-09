/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
/**
 * @file double_buffer.hpp
 * @brief Implementation of DoubleBuffer class for detection app example
 **/
#pragma once

#include <stdint.h>
#include <vector>
#include <mutex>
#include <condition_variable>

class DoubleBuffer {
public:
    DoubleBuffer(uint32_t size) : m_first_buffer(size), m_second_buffer(size),
        m_write_ptr(&m_first_buffer), m_read_ptr(&m_first_buffer)
    {}

    std::vector<uint8_t> &get_write_buffer()
    {
        return m_write_ptr->acquire_write_buffer();
    }

    void release_write_buffer()
    {
        m_write_ptr->release_buffer();
        m_write_ptr = (m_write_ptr == &m_first_buffer) ? &m_second_buffer : &m_first_buffer;
    }

    std::vector<uint8_t> &get_read_buffer()
    {
        return m_read_ptr->acquire_read_buffer();
    }

    void release_read_buffer()
    {
        m_read_ptr->release_buffer();
        m_read_ptr = (m_read_ptr == &m_first_buffer) ? &m_second_buffer : &m_first_buffer;
    }

private:
    class SafeBuffer {
    public:
        SafeBuffer(uint32_t size) :
        m_state(State::WRITE), m_cv(), m_mutex(), m_buffer(size)
        {}

        std::vector<uint8_t> &acquire_write_buffer()
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this]{ return (State::WRITE == m_state); });

            return m_buffer;
        }

        std::vector<uint8_t> &acquire_read_buffer()
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this]{ return (State::READ == m_state); });

            return m_buffer;
        }

        void release_buffer()
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            swap_state();
            m_cv.notify_one();
        }

    private:
        void swap_state() {
            m_state = (State::WRITE == m_state) ? State::READ : State::WRITE;
        }

        enum class State {
            READ = 0,
            WRITE = 1,
        };

        State m_state;
        std::condition_variable m_cv;
        std::mutex m_mutex;
        std::vector<uint8_t> m_buffer;
    };

    SafeBuffer m_first_buffer;
    SafeBuffer m_second_buffer;

    SafeBuffer *m_write_ptr;
    SafeBuffer *m_read_ptr;
};