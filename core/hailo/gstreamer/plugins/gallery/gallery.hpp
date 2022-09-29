/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#pragma once
#include <vector>
#include "xtensor/xarray.hpp"
#include "xtensor/xadapt.hpp"
#include "xtensor/xsort.hpp"
#include "xtensor/xio.hpp"
#include "hailo_objects.hpp"

static xt::xarray<float> gallery_get_xtensor(HailoMatrixPtr matrix)
{
    // Adapt a HailoTensorPtr to an xarray (quantized)
    xt::xarray<float> xtensor = xt::adapt(matrix->get_data().data(), matrix->size(), xt::no_ownership(), matrix->shape());
    return xt::squeeze(xtensor);
}

static float gallery_one_dim_dot_product(xt::xarray<float> array1, xt::xarray<float> array2)
{
    if (array1.dimension() > 1 || array2.dimension() > 1)
    {
        throw std::runtime_error("One of the arrays has more than 1 dimension");
    }
    if (array1.shape(0) != array2.shape(0))
    {
        throw std::runtime_error("Arrays are with different shape");
    }
    return xt::sum(array1 * array2)[0];
}

class Gallery
{
private: 
    // Each embedding is represented by HailoMatrixPtr
    // Each global_id has a vector of embeddings (I.e. vector of HailoMatrixPtr) 
    // of all the embeddings related to this ID.
    // For the whole gallery there is a vector of global id's (i.e. vector of vectors of HailoMatrixPtr)
    // where the global ID is represented by the outer vector's index.
    std::vector<std::vector<HailoMatrixPtr>> m_embeddings;
    std::map<int, int> tracking_id_to_global_id;
    float m_similarity_thr;
    uint m_queue_size;

public:
    Gallery(float similarity_thr = 0.15, uint queue_size = 100) : m_similarity_thr(similarity_thr), m_queue_size(queue_size){};

    static float get_distance(std::vector<HailoMatrixPtr> embeddings_queue, HailoMatrixPtr matrix)
    {
        xt::xarray<float> new_embedding = gallery_get_xtensor(matrix);
        float max_thr = 0.0f;
        float thr;
        for (HailoMatrixPtr embedding_mat : embeddings_queue)
        {
            xt::xarray<float> embedding = gallery_get_xtensor(embedding_mat);
            thr = gallery_one_dim_dot_product(embedding, new_embedding);
            max_thr = thr > max_thr ? thr : max_thr;
        }
        return 1.0f - max_thr;
    }
    xt::xarray<float> get_embeddings_distances(HailoMatrixPtr matrix)
    {
        std::vector<float> distances;
        for (auto embeddings_queue : m_embeddings)
        {
            distances.push_back(get_distance(embeddings_queue, matrix));
        }
        return xt::adapt(distances);
    }

    void add_embedding(uint global_id, HailoMatrixPtr matrix)
    {
        global_id--;
        if (m_embeddings[global_id].size() >= m_queue_size)
        {
            m_embeddings[global_id].pop_back();
        }
        m_embeddings[global_id].insert(m_embeddings[global_id].begin(), matrix);
    }

    uint add_new_global_id(HailoMatrixPtr matrix)
    {
        std::vector<HailoMatrixPtr> queue;
        m_embeddings.push_back(queue);
        uint global_id = m_embeddings.size();
        add_embedding(global_id, matrix);
        return global_id;
    }

    std::pair<uint, float> get_closest_global_id(HailoMatrixPtr matrix)
    {

        auto distances = get_embeddings_distances(matrix);
        auto global_id = xt::argpartition(distances, 1, xt::xnone())[0];
        return std::pair<uint, float>(global_id+1, distances[global_id]);
    }

    void update(std::vector<HailoDetectionPtr> &detections)
    {
        for (auto detection : detections)
        {
            auto embeddings = detection->get_objects_typed(HAILO_MATRIX);
            if (embeddings.size() == 0)
            {
                // No HailoMatrix, continue to next detection.
                continue;
            }
            else if (embeddings.size() > 1)
            {
                // More than 1 HailoMatrixPtr is not allowed.
                std::runtime_error("A detection has more than 1 HailoMatrixPtr");
            }
            auto new_embedding = std::dynamic_pointer_cast<HailoMatrix>(embeddings[0]);
            auto unique_id = std::dynamic_pointer_cast<HailoUniqueID>(detection->get_objects_typed(HAILO_UNIQUE_ID)[0])->get_id();
            uint global_id;
            if (tracking_id_to_global_id.find(unique_id) != tracking_id_to_global_id.end())
            {
                global_id = tracking_id_to_global_id[unique_id];
            }
            else
            {
                uint closest_global_id;
                float min_distance;
                // If Gallery is empty
                if (m_embeddings.empty())
                {
                    global_id = add_new_global_id(new_embedding);
                }
                else
                {
                    std::tie(closest_global_id, min_distance) = get_closest_global_id(new_embedding);
                    // if smallest distance > threshold -> create new ID
                    if (min_distance > m_similarity_thr)
                    {
                        global_id = add_new_global_id(new_embedding);
                    }
                    else
                    {
                        global_id = closest_global_id;
                    }
                }
                tracking_id_to_global_id[unique_id] = global_id;
            }
            // Add global id to detection.
            add_embedding(global_id, new_embedding);
            detection->add_object(std::make_shared<HailoUniqueID>(global_id, GLOBAL_ID));

        }
    };
    void set_similarity_threshold(float thr) { m_similarity_thr = thr; };
    void set_queue_size(uint size) { m_queue_size = size; };
    float get_similarity_threshold() { return m_similarity_thr; };
    uint get_queue_size() { return m_queue_size; };
};
