/**
 * Copyright (c) 2021-2026 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
/**
 * @file hailomat_internal.hpp
 * @brief Internal header for accessing cv::Mat from HailoMat
 * 
 * This header should only be included by .cpp files that need direct access
 * to the underlying cv::Mat data. It requires OpenCV to be included first.
 */

#pragma once

#include <opencv2/opencv.hpp>
#include "hailomat.hpp"

/**
 * @brief Implementation structure that holds cv::Mat internally
 * 
 * This structure provides access to the underlying OpenCV matrices
 * for internal use within the library.
 */
struct HailoMatImpl {
    std::vector<cv::Mat> matrices;
    
    HailoMatImpl() = default;
    ~HailoMatImpl() {
        for (auto& mat : matrices) {
            mat.release();
        }
        matrices.clear();
    }
    
    // Disable copy
    HailoMatImpl(const HailoMatImpl&) = delete;
    HailoMatImpl& operator=(const HailoMatImpl&) = delete;
    
    // Enable move
    HailoMatImpl(HailoMatImpl&&) = default;
    HailoMatImpl& operator=(HailoMatImpl&&) = default;
    
    // Helper to get cv::Mat reference
    cv::Mat& get(size_t index) { return matrices[index]; }
    const cv::Mat& get(size_t index) const { return matrices[index]; }
    
    void push(cv::Mat mat) { matrices.push_back(mat); }
    void clear() { 
        for (auto& mat : matrices) {
            mat.release();
        }
        matrices.clear(); 
    }
    size_t size() const { return matrices.size(); }
    
    // Legacy compatibility: get_matrices() equivalent
    std::vector<cv::Mat>& get_matrices() { return matrices; }
    const std::vector<cv::Mat>& get_matrices() const { return matrices; }
};

/**
 * @brief Helper function to get cv::Mat vector from HailoMat
 * 
 * This provides backwards compatibility for code that used get_matrices()
 * 
 * @param mat The HailoMat object
 * @return Reference to the internal cv::Mat vector
 */
inline std::vector<cv::Mat>& get_cv_matrices(HailoMat& mat) {
    return mat.get_impl()->get_matrices();
}

inline const std::vector<cv::Mat>& get_cv_matrices(const HailoMat& mat) {
    return mat.get_impl()->get_matrices();
}

/**
 * @brief Helper function to crop and get cv::Mat vector
 * 
 * This provides backwards compatibility for code that used crop() returning vector<cv::Mat>
 * 
 * @param mat The HailoMat object
 * @param crop_roi The ROI to crop
 * @return Vector of cropped cv::Mat
 */
inline std::vector<cv::Mat> crop_to_cv_matrices(HailoMat& mat, HailoROIPtr crop_roi) {
    auto impl = mat.crop(crop_roi);
    return std::move(impl->get_matrices());
}

/**
 * @brief Helper function to create HailoRGBMat from cv::Mat (for internal/test use)
 * 
 * @param cv_mat The OpenCV Mat
 * @param name Name for the HailoRGBMat
 * @param line_thickness Line thickness for drawing
 * @param font_thickness Font thickness for drawing
 * @return shared_ptr to HailoRGBMat
 */
inline std::shared_ptr<HailoRGBMat> make_hailo_rgb_mat(cv::Mat& cv_mat, const std::string& name = "HailoRGBMat", int line_thickness = 1, int font_thickness = 1) {
    return std::make_shared<HailoRGBMat>(cv_mat.data, cv_mat.rows, cv_mat.cols, cv_mat.step, line_thickness, font_thickness, name);
}

