/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
// Catch2 includes
#include "catch.hpp"

// General cpp includes
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

// Open source includes
#include "xtensor/xadapt.hpp"
#include "xtensor/xarray.hpp"
#include "xtensor/xmath.hpp"
#include "xtensor/xview.hpp"

/**
 * @brief Compares equivalence between two floats 
 *        to precision epsilon.
 * 
 * @param f1  -  float to compare
 * @param f2  -  float to compare
 * @param epsilon  -  float epsilon precision.
 * @return bool
 *         True if floats are the same (within epsilon precision).
 *         False if they are not.
 */
bool compare_floats(float f1, float f2, float epsilon = 0.0001f)
{
    if(fabs(f1 - f2) < epsilon)
        return true;
    return false;
}

/**
 * @brief Compare two float matricies.
 *        Flattened indices are iterated.
 *        Uses compare_floats helper above.
 * 
 * @param matrix1 
 * @param matrix2 
 * @return bool
 *         Returns true if both matrices have same size and values.
 */
bool compare_float_matrix_values(xt::xarray<float> matrix1, xt::xarray<float> matrix2)
{
    if (matrix1.size() != matrix2.size()) {
        std::cout << "compare_float_matrix_values failed, sizes don't match: "<< std::endl;
        std::cout << "matrix1 size: " << matrix1.size() << std::endl;
        std::cout << "matrix2 size: " << matrix2.size() << std::endl;
        return false;
    }

    for (uint i = 0; i < matrix1.size(); ++i)
    {
        if (!compare_floats(matrix1.flat(i), matrix2.flat(i))) {
        std::cout << "compare_float_matrix_values failed, values don't match: "<< std::endl;
        std::cout << "matrix1 value: " << matrix1.flat(i) << std::endl;
        std::cout << "matrix2 value: " << matrix2.flat(i) << std::endl;
        return false;
        }
    }
    return true;
}

/**
 * @brief Compares two matrices. Parameters are
 *        auto and then adapted to support all
 *        types of xcontainers.
 * 
 * @param matrix1 
 * @param matrix2 
 * @return bool
 *         Returns true if both matrices have same size, shape, and values. 
 */
bool compare_float_matrices(auto m1, auto m2)
{
    // Adapt the auto inputs to ensure xarrays
    xt::xarray<float> matrix1 = m1;
    xt::xarray<float> matrix2 = m2;

    // Check that the number of dimensions match
    if (matrix1.dimension() != matrix2.dimension()) {
        std::cout << "compare_float_matrices failed, dims don't match: "<< std::endl;
        std::cout << "matrix1 dims: " << matrix1.dimension() << std::endl;
        std::cout << "matrix2 dims: " << matrix2.dimension() << std::endl;
        return false;
    }

    // Check that the dimensions themselves match
    for (int i = 0; i < (int)matrix1.dimension(); ++i)
    {
        if (matrix1.shape(i) != matrix2.shape(i)) {
        std::cout << "compare_float_matrices failed, shapes don't match: "<< std::endl;
        std::cout << "matrix1 shape at dim " << i << ": " << matrix1.shape(i) << std::endl;
        std::cout << "matrix2 shape at dim " << i << ": " << matrix2.shape(i) << std::endl;
        return false;
        }
    }

    // Shapes match so now compare values
    return compare_float_matrix_values(matrix1, matrix2);
}