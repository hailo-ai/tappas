#pragma once

// General includes
#include <iostream>
#include <thread>
#include <map>

// Open source includes
#include <opencv2/opencv.hpp>

class CVMatSingleton
{
    private:
        CVMatSingleton(): _mat_map({}){}
        ~CVMatSingleton() {}
        CVMatSingleton(CVMatSingleton const&); // Don't Implement. CVMatSingleton should not be cloneable.
        void operator=(CVMatSingleton const&); // Don't implement. CVMatSingleton should not be assignable.
        std::map<int, cv::Mat> _mat_map;
        static std::mutex mutex_;


    public:
        static CVMatSingleton& GetInstance();
        void set_mat_at_key(int key, cv::Mat mat);
        cv::Mat get_mat_at_key(int key);

};
