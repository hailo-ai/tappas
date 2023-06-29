// General includes
#include <iostream>
#include <thread>
#include <map>

// Tappas includes
#include "hailo_cv_singleton.hpp"

// Open source includes
#include <opencv2/opencv.hpp>

std::mutex CVMatSingleton::mutex_;

CVMatSingleton& CVMatSingleton::GetInstance()
{
    std::lock_guard<std::mutex> lock(mutex_);
    static CVMatSingleton instance;
    return instance;
}

void CVMatSingleton::set_mat_at_key(int key, cv::Mat mat) {
    std::lock_guard<std::mutex> lock(mutex_);
    _mat_map[key] = mat.clone();
}

cv::Mat CVMatSingleton::get_mat_at_key(int key) {
    std::lock_guard<std::mutex> lock(mutex_);
    if ( _mat_map.find(key) == _mat_map.end() )
    {
        return cv::Mat{};
    }
    return _mat_map[key].clone();
}

void CVMatSingleton::set_mat_type(hailo_mat_t new_type)
{
    _mat_type = new_type;
}

hailo_mat_t CVMatSingleton::get_mat_type()
{
    return _mat_type;
}