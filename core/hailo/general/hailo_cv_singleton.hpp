#pragma once

// General includes
#include <iostream>
#include <thread>
#include <map>

// Tappas includes
#include "image.hpp"

// Open source includes
#include <opencv2/opencv.hpp>

class CVMatSingleton
{
    private:
        CVMatSingleton(): _mat_map({}),_mat_type(HAILO_MAT_NONE){}
        ~CVMatSingleton() {}
        CVMatSingleton(CVMatSingleton const&); // Don't Implement. CVMatSingleton should not be cloneable.
        void operator=(CVMatSingleton const&); // Don't implement. CVMatSingleton should not be assignable.
        std::map<int, cv::Mat> _mat_map;
        static std::mutex mutex_;
        hailo_mat_t _mat_type;


    public:
        static CVMatSingleton& GetInstance();
        void set_mat_at_key(int key, cv::Mat mat);
        cv::Mat get_mat_at_key(int key);
        void set_mat_type(hailo_mat_t new_type);
        hailo_mat_t get_mat_type();

};
