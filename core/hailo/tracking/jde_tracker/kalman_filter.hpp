/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
/*
  A simple Kalman filter for tracking bounding boxes in image space.

  The 8-dimensional state space:

      x, y, a, h, vx, vy, va, vh

  contains the bounding box center position (x, y), aspect ratio a (width/height), height h,
  and their respective velocities.

  Object motion follows a constant velocity model. The bounding box location
  (x, y, a, h) is taken as direct observation of the state space (linear
  observation model).

 */

#pragma once

// General cpp includes
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

// Tappas includes
#include "hailo_common.hpp"
#include "tracker_macros.hpp"

// Open source includes
#include "xtensor/xadapt.hpp"
#include "xtensor/xarray.hpp"
#include "xtensor/xio.hpp"
#include "xtensor/xmath.hpp"
#include "xtensor/xview.hpp"

using namespace xt::placeholders;

__BEGIN_DECLS
class KalmanFilter
{
    //******************************************************************
    // CLASS MEMBERS
    //******************************************************************
    public:
    /* Table for the 0.95 quantile of the chi-square distribution with N degrees of
    freedom (contains values for N=1, ..., 9). Taken from MATLAB/Octave's chi2inv
    function and used as Mahalanobis gating threshold. */
    static constexpr float chi2inv95[10] = {
        0,
        3.8415,
        5.9915,
        7.8147,
        9.4877,
        11.070,
        12.592,
        14.067,
        15.507,
        16.919};

    private:
    // Identity matrices by which to multiply later means and covariances, initialized in the constructor and unchanged later
    xt::xtensor_fixed<float, xt::xshape<8, 8>, xt::layout_type::row_major> m_motion_matrix;
    xt::xtensor_fixed<float, xt::xshape<4, 8>, xt::layout_type::row_major> m_update_matrix;
    float m_std_weight_position;  // weight of standard deviation for x and y
    float m_std_weight_position_box;  // weight of standard deviation for a and h
    float m_std_weight_velocity;  // weight of standard deviation for vx and vy
    float m_std_weight_velocity_box;  // weight of standard deviation for va and vh

    //******************************************************************
    // CLASS RESOURCE MANAGEMENT
    //******************************************************************
    public:
    //Constructor
    KalmanFilter(float std_weight_position = 0.01, float std_weight_position_box = 0.01, float std_weight_velocity = 0.001, float std_weight_velocity_box = 0.001) :
    m_std_weight_position(std_weight_position), m_std_weight_position_box(std_weight_position_box),
    m_std_weight_velocity(std_weight_velocity), m_std_weight_velocity_box(std_weight_velocity_box)
    {
        int ndim = 4;
        float dt = 1.;

        m_motion_matrix = xt::eye<float>({8, 8}, 0); // Identity matrix of shape 8 x 8
        for (int i = 0; i < ndim; i++)
        {
            m_motion_matrix(i, ndim + i) = dt;
        }
        m_update_matrix = xt::eye<float>({4, 8}, 0); // Identity matrix of shape 4 x 8
    }

    // Params setters
    void set_std_weight_position(float std_weight_position) { m_std_weight_position = std_weight_position; }
    void set_std_weight_position_box(float std_weight_position_box) { m_std_weight_position_box = std_weight_position_box; }
    void set_std_weight_velocity(float std_weight_velocity) { m_std_weight_velocity = std_weight_velocity; }
    void set_std_weight_velocity_box(float std_weight_velocity_box) { m_std_weight_velocity_box = std_weight_velocity_box; }
    
    // Params getters
    float get_std_weight_position() { return m_std_weight_position; }
    float get_std_weight_position_box() { return m_std_weight_position_box; }
    float get_std_weight_velocity() { return m_std_weight_velocity; }
    float get_std_weight_velocity_box() { return m_std_weight_velocity_box; }

    //******************************************************************
    // LINEAR ALGEBRA HELPER FUNCTIONS
    //******************************************************************
    private:
    /**
     * @brief Performs a LL^T Cholesky decomposition of a symmetric, positive definite 
     *        matrix A such that A = LLT, where L is a lower triangular matrix and LT it's transpose.
     * 
     * @param matrix  -  TrackerTypes::KAL_HCOVA : <4x4>
     *        The matrix to decompose, expected to be 4x4, and positive definite
     *
     * @return TrackerTypes::KAL_HCOVA  : <4x4>
     *         The lower triamgular matrix of the cholesky decomposition.
     */
    TrackerTypes::KAL_HCOVA cholesky_decomposition(TrackerTypes::KAL_HCOVA &matrix)
    {
        TrackerTypes::KAL_HCOVA lower_matrix = xt::zeros<float>({(int)matrix.shape(0), (int)matrix.shape(0)});

        int sum = 0;
        // Decomposing a matrix into Lower Triangular
        for (uint i = 0; i < matrix.shape(0); i++) {
            for (uint j = 0; j <= i; j++) {
                sum = 0;
                if (j == i) // summation for diagonals
                {
                    for (uint k = 0; k < j; k++)
                        sum += std::pow(lower_matrix(j, k), 2);
                    lower_matrix(j,j) = std::sqrt(matrix(j,j) - sum);
                } else {
                    // Evaluating L(i, j) using L(j, j)
                    for (uint k = 0; k < j; k++)
                        sum += lower_matrix(i, k) * lower_matrix(j, k);
                    lower_matrix(i, j) = (matrix(i, j) - sum) / lower_matrix(j, j);
                }
            }
        }
        return lower_matrix;
    }

    /**
     * @brief Solves the system of linear equations Ax=B
     *        where A is a lower triangular matrix L.
     *        In short, performs forward-substitution.
     * 
     * @param L  -  xt::xarray<float>
     *        A lower trangular matrix.
     *
     * @param B  -  xt::xarray<float>
     *        The right-hand-side of the system Ax=B, the number of rows
     *        must match the rows of L.
     *
     * @return xt::xarray<float> 
     *         The solution x to the system Ax=B.
     */
    xt::xarray<float> forward_substitution(xt::xarray<float> L, xt::xarray<float> B)
    {
        // Check dimensionality
        if (L.dimension() != 2 || B.dimension() != 2)
            throw std::invalid_argument("forward_substitution broadcast error: only 2D matrices supported!");

        // Get the rows and cols
        int L_rows = L.shape(0);
        int L_cols = L.shape(1);
        int B_rows = B.shape(0);
        int B_cols = B.shape(1);

        // Check broadcasting rules
        if (L_rows != B_rows)
            throw std::invalid_argument("forward_substitution broadcast error: rows don't match!");

        // Prepare x matrix
        float partial_sum = 0.0;
        xt::xarray<float>::shape_type shape = {(long unsigned int)L_cols, (long unsigned int)B_cols};
        xt::xarray<float, xt::layout_type::row_major> x = xt::zeros<float>(shape);

        // For each column of x (=B_cols)
        for (int i = 0; i < B_cols; ++i)
        {
            // For each row of x (and row of L, since symmetric)
            for (int j = 0; j < L_rows; ++j)
            {
                partial_sum = 0;  // Reset the partial sum
                // For each column of L up to the current diagonal (the j current row in L)
                // This process is forward substitution
                for (int k = 0; k < j; ++k)
                {
                    // Sum the dot product of the L_row*x_col up to the missing diagonal 
                    partial_sum += L(j, k) * x(k, i);
                }
                // x at the missing diagonal is (B - the known sum)/the known L
                x(j, i) = (B(j, i) - partial_sum) / L(j, j);
            }
        }
        return x;
    }

    /**
     * @brief Solves the system of linear equations Ax=B
     *        where A is an upper triangular matrix U.
     *        In short, performs back-substitution.
     * 
     * @param U  -  xt::xarray<float>
     *        An upper trangular matrix.
     *
     * @param B  -  xt::xarray<float>
     *        The right-hand-side of the system Ax=B, the number of rows
     *        must match the rows of U.
     *
     * @return xt::xarray<float> 
     *         The solution x to the system Ax=B.
     */
    xt::xarray<float> back_substitution(xt::xarray<float> U, xt::xarray<float> B)
    {
        // Check dimensionality
        if (U.dimension() != 2 || B.dimension() != 2)
            throw std::invalid_argument("forward_substitution broadcast error: only 2D matrices supported!");

        // Get the rows and cols
        int U_rows = U.shape(0);
        int U_cols = U.shape(1);
        int B_rows = B.shape(0);
        int B_cols = B.shape(1);

        // Check broadcasting rules
        if (U_rows != B_rows)
            throw std::invalid_argument("forward_substitution broadcast error: rows don't match!");

        // Prepare x matrix
        float partial_sum = 0.0;
        xt::xarray<float>::shape_type shape = {(long unsigned int)U_cols, (long unsigned int)B_cols};
        xt::xarray<float, xt::layout_type::row_major> x = xt::zeros<float>(shape);

        // For each column of x (=y_cols=B_cols)
        for (int i = 0; i < B_cols; ++i)
        {
            // For each row of U
            // Since U is an upper matrix, we have to iterate in ascending order (starting from the bottom rows)
            for (int j = U_rows - 1; j >= 0; j--)
            {
                partial_sum = 0;  // Reset the partial sum
                // For each column of U up to the current diagonal
                // Since U is an upper matrix, we have to iterate backwards
                // This process is back substitution
                for (int k = U_rows - 1; k > j; k--)
                {
                    // Sum the dot product of the U_row*x_col up to the missing diagonal 
                    partial_sum += U(j, k) * x(k, i);
                }
                // x at the missing diagonal is (B - the known sum)/the known U
                x(j, i) = (B(j, i) - partial_sum) / U(j, j);
            }
        }
        return x;
    }

    /**
     * @brief Solves the system of linear equations Ax=B
     *        using the cholesky decomposition of A.
     *        Parameters are auto and then adapted to 
     *        support all types of xcontainers.
     *
     *        Given the cholesky A=LLT (where LT = L transposed),
     *        we can turn Ax=B into LLTx=B, and split this into
     *        the equations Ly=B, LTx=y. We first solve for y in 
     *        Ly=B using forward-substitution, then solve for x in
     *        LTx=y using back-substitution.
     * 
     * @param L_  -  any xcontainer
     *        The cholesky decomposition of A, where A=LLT
     *
     * @param B_  -  any xcontainer
     *        The right-hand-side of the system Ax=B, the number of rows
     *        must match the sides of L_
     *
     * @return xt::xarray<float>
     *         The solution x to the system Ax=B.
     */
    xt::xarray<float> solve_linear_eq_with_cholesky(auto L_, auto B_)
    {
        // Adapt the auto inputs to ensure xarrays
        xt::xarray<float> L = L_;
        xt::xarray<float> LT = xt::transpose(L);
        xt::xarray<float> B = B_;

        // Check dimensionality
        if (L.dimension() != 2 || B.dimension() != 2)
            throw std::invalid_argument("solve_cholesky broadcast error: only 2D matrices supported!");

        // Check broadcasting rules: the rows of L and B should match
        if (L.shape(0) != B.shape(0))
            throw std::invalid_argument("solve_cholesky broadcast error: rows don't match!");

        // First solve Ly=B with forward-substitution
        xt::xarray<float, xt::layout_type::row_major> y = forward_substitution(L, B);

        // Then solve LTx=y with back-substitution
        xt::xarray<float, xt::layout_type::row_major> x = back_substitution(LT, y);

        return x;
    }

    /**
     * @brief Compute matrix multiplication between two 2 dimensional matrices .
     * 
     * @param matrix_1  -  xt::xarray<float, xt::layout_type::row_major>
     *        The LHS matrix, columns must match rows in RHS matrix.
     * 
     * @param matrix_2  -  xt::xarray<float, xt::layout_type::row_major>
     *        The RHS matrix, rows must match columns in LHS matrix.
     * 
     * @return xt::xarray<float, xt::layout_type::row_major>
     *         The matrix multiplication of the two matrices.
     *         Normal matrix broadcasting rules apply.
     */
    xt::xarray<float, xt::layout_type::row_major> mat_mul_2D(xt::xarray<float, xt::layout_type::row_major> matrix_1,
                                                             xt::xarray<float, xt::layout_type::row_major> matrix_2)
    {
        uint axis_length = matrix_1.shape(1);
        if (axis_length != matrix_2.shape(0))
        {
            throw std::invalid_argument("mat_mul_2D broadcast error: axis don't match!");
        }

        float row_sum;
        xt::xarray<float>::shape_type shape = {matrix_1.shape(0), matrix_2.shape(1)};
        xt::xarray<float, xt::layout_type::row_major> product_matrix(shape);
        for (uint i = 0; i < matrix_1.shape(0); ++i)
        {
            for (uint j = 0; j < matrix_2.shape(1); ++j)
            {
                row_sum = 0.0;
                for (uint k = 0; k < matrix_1.shape(1); ++k)
                {
                    row_sum += matrix_1(i, k) * matrix_2(k, j);
                }
                product_matrix(i, j) = row_sum;
            }
        }
        return product_matrix;
    }

    //******************************************************************
    // TRACKING FUNCTIONS
    //******************************************************************
    public:
    /**
     * @brief Create a track from an unassociated measurement.
     * 
     * @param measurement  -  TrackerTypes::DETECTBOX : <1x4>
     *        Bounding box coordinates (x, y, a, h) with center position (x, y),
     *        aspect ratio a, and height h.
     * 
     * @return TrackerTypes::KAL_DATA --> pair<KAL_MEAN, KAL_COVA>: <1x8>,<8x8>
     *         Returns the mean vector (1x8) and covariance matrix (8x8)
     *         of the new track. Unobserved velocities are initialized to 0 mean.
     *         These newly generated mean and covariance are used to iniate an Strack.
     */
    TrackerTypes::KAL_DATA initiate(const TrackerTypes::DETECTBOX &measurement)
    {
        TrackerTypes::KAL_MEAN mean;
        xt::view(mean, xt::all(), xt::range(_, 4)) = xt::squeeze(measurement);
        xt::view(mean, xt::all(), xt::range(4, _)) = xt::zeros<float>({4});

        float measured_height = measurement(3);
        TrackerTypes::KAL_MEAN standard_deviation;
        // Build standard deviation to the position (x, y, a, h)
        standard_deviation(0) = 2 * m_std_weight_position * measured_height;
        standard_deviation(1) = 2 * m_std_weight_position * measured_height;
        standard_deviation(2) = 2 * m_std_weight_position_box * measured_height;
        standard_deviation(3) = 2 * m_std_weight_position_box * measured_height;
        // Build standard deviation to the velocities (vx, vy, va, vh)
        standard_deviation(4) = 10 * m_std_weight_velocity * measured_height;
        standard_deviation(5) = 10 * m_std_weight_velocity * measured_height;
        standard_deviation(6) = 5 * m_std_weight_velocity_box * measured_height;
        standard_deviation(7) = 5 * m_std_weight_velocity_box * measured_height;

        // The standard deviations form the diagonal of the new covariance
        TrackerTypes::KAL_COVA var = xt::diag(xt::squeeze(xt::square(standard_deviation)));
        return std::make_pair(mean, var);
    }

    /**
     * @brief Run Kalman filter prediction step.
     *        Updates the mean vector and covariance matrix of the predicted
     *        state. Unobserved velocities are initialized to 0 mean.
     * 
     * @param mean  -  TrackerTypes::KAL_MEAN : <1x8>
     *        The 8 dimensional mean vector of the object state at the previous time step.
     * 
     * @param covariance  -  TrackerTypes::KAL_COVA: <8x8>
     *        The 8x8 dimensional covariance matrix of the object state at the 
     *        previous time step.
     */
    void predict(TrackerTypes::KAL_MEAN &mean, TrackerTypes::KAL_COVA &covariance)
    {
        float mean_height = mean(3);
        TrackerTypes::KAL_MEAN standard_deviation;
        // Build standard deviation for the position (x, y, a, h)
        standard_deviation(0) = m_std_weight_position * mean_height;
        standard_deviation(1) = m_std_weight_position * mean_height;
        standard_deviation(2) = m_std_weight_position_box * mean_height;
        standard_deviation(3) = m_std_weight_position_box * mean_height;
        // Build standard deviation for the velocities (vx, vy, va, vh)
        standard_deviation(4) = m_std_weight_velocity * mean_height;
        standard_deviation(5) = m_std_weight_velocity * mean_height;
        standard_deviation(6) = m_std_weight_velocity_box * mean_height;
        standard_deviation(7) = m_std_weight_velocity_box * mean_height;
        // Square and reshape the deviations to match the covariance space
        TrackerTypes::KAL_COVA motion_covariance = xt::diag(xt::squeeze(xt::square(standard_deviation)));

        TrackerTypes::KAL_MEAN predicted_mean = mat_mul_2D(this->m_motion_matrix, xt::transpose(mean));
        TrackerTypes::KAL_COVA predicted_covariance = mat_mul_2D(this->m_motion_matrix, mat_mul_2D(covariance, xt::transpose(m_motion_matrix)));
        predicted_covariance += motion_covariance;  // Apply the standard deviation of motion to the covariance

        // Update the input mean / covariance 
        mean = predicted_mean;
        covariance = predicted_covariance;
    }

    /**
     * @brief Project state distribution to measurement space.
     * 
     * @param mean  -  TrackerTypes::KAL_MEAN : <1x8>
     *        The state's mean vector (1x8 dimensional array).
     * 
     * @param covariance  -  TrackerTypes::KAL_COVA: <8x8>
     *        The state's covariance matrix (8x8 dimensional).
     * 
     * @return TrackerTypes::KAL_HDATA --> pair<KAL_HMEAN, KAL_HCOVA> : <1x4>,<4x4>
     *         Returns the projected mean (x,y,a,h) and covariance matrix 
     *         of the given state estimate.
     */
    TrackerTypes::KAL_HDATA project(const TrackerTypes::KAL_MEAN &mean, const TrackerTypes::KAL_COVA &covariance)
    {
        float mean_height = mean(3);
        TrackerTypes::DETECTBOX standard_deviation;
        // Build standard deviation for the position (x, y, a, h)
        standard_deviation(0) = m_std_weight_position * mean_height;
        standard_deviation(1) = m_std_weight_position * mean_height;
        standard_deviation(2) = m_std_weight_position_box * mean_height;
        standard_deviation(3) = m_std_weight_position_box * mean_height;
        // Square and reshape the deviations to match the covariance space
        TrackerTypes::KAL_HCOVA innovation_covariance = xt::diag(xt::square(xt::squeeze(standard_deviation)));

        TrackerTypes::KAL_HMEAN mean1 = mat_mul_2D(this->m_update_matrix, xt::transpose(mean));
        TrackerTypes::KAL_HCOVA covariance1 = mat_mul_2D(this->m_update_matrix, mat_mul_2D(covariance, xt::transpose(this->m_update_matrix)));
        covariance1 += innovation_covariance;
        return std::make_pair(mean1, covariance1);
    }

    /**
     * @brief Run Kalman filter correction step.
     *        This step updates the predicted mean and covariance of a tracklet
     *        based on the newly measured detection.
     * 
     * @param mean  -  TrackerTypes::KAL_MEAN : <1x8>
     *        The predicted state's mean vector (8 dimensional).
     * 
     * @param covariance  -  TrackerTypes::KAL_COVA: <8x8>
     *        The state's covariance matrix (8x8 dimensional).
     * 
     * @param measurement  -  TrackerTypes::DETECTBOX : <1x4>
     *        The 4 dimensional measurement vector (x, y, a, h), where (x, y)
     *        is the center position, a the aspect ratio, and h the height of the
     *        bounding box.
     * 
     * @return TrackerTypes::KAL_DATA --> pair<KAL_MEAN, KAL_COVA> : <1x8>,<8x8>
     *         Returns the measurement-corrected state distribution (the new
     *         mean and covariance).
     */
    TrackerTypes::KAL_DATA update(const TrackerTypes::KAL_MEAN &mean,
                                  const TrackerTypes::KAL_COVA &covariance,
                                  const TrackerTypes::DETECTBOX &measurement)
    {
        TrackerTypes::KAL_HDATA projection_results = project(mean, covariance);
        TrackerTypes::KAL_HMEAN projected_mean = projection_results.first;
        TrackerTypes::KAL_HCOVA projected_covariance = projection_results.second;

        // Solve Ax=B using cholesky decomposition
        xt::xtensor_fixed<float, xt::xshape<4, 8>> B = xt::transpose(mat_mul_2D(covariance, xt::transpose(this->m_update_matrix)));
        auto cholesky_factor = cholesky_decomposition(projected_covariance);
        xt::xtensor_fixed<float, xt::xshape<8, 4>> kalman_gain = xt::transpose(solve_linear_eq_with_cholesky(cholesky_factor, B));
        xt::xtensor_fixed<float, xt::xshape<1, 4>> innovation = measurement - projected_mean;
        auto tmp = mat_mul_2D(innovation, xt::transpose(kalman_gain));

        TrackerTypes::KAL_MEAN new_mean = mean + tmp;
        TrackerTypes::KAL_COVA new_covariance = covariance - mat_mul_2D(kalman_gain, mat_mul_2D(projected_covariance, xt::transpose(kalman_gain)));
        return std::make_pair(new_mean, new_covariance);
    }

    /**
     * @brief Compute gating distance between state distribution and measurements. 
     *        A suitable distance threshold can be obtained from `chi2inv95`. 
     * 
     * @param mean  -  TrackerTypes::KAL_MEAN : <1x8>
     *        Mean vector over the state distribution (1x8 dimensional).
     * 
     * @param covariance  -  TrackerTypes::KAL_COVA <8x8>
     *        Covariance of the state distribution (8x8 dimensional).
     * 
     * @param measurements  -  vector<TrackerTypes::DETECTBOX> : vector<<1x4>>
     *        An Nx4 dimensional matrix of N measurements, each in 
     *        format (x, y, a, h) where (x, y) is the bounding box center
     *        position, a the aspect ratio, and h the height.
     * 
     * @return xt::xarray<float>: <1, -1>
     *         Returns an array of length N, where the i-th element contains the
     *         squared Mahalanobis distance between (mean, covariance) and 
     *         `measurements[i]`.
     */
    xt::xarray<float> gating_distance(const TrackerTypes::KAL_MEAN &mean,
                                      const TrackerTypes::KAL_COVA &covariance,
                                      const std::vector<TrackerTypes::DETECTBOX> &measurements)
    {
        TrackerTypes::KAL_HDATA projection_results = project(mean, covariance);
        TrackerTypes::KAL_HMEAN mean1 = projection_results.first;
        TrackerTypes::KAL_HCOVA covariance1 = projection_results.second;
        
        // DETECTBOXSS differs from DETECTBOX in that DETECTBOXSS is Nx4 instead of 1x4
        TrackerTypes::DETECTBOXSS d = xt::zeros<float>({(int)measurements.size(), 4});
        for (uint i = 0; i < measurements.size(); ++i)
        {
            xt::row(d, i) = xt::squeeze(measurements[i] - mean1);
        }
        // Extract lower triangular matrix from cholesky decomposition
        xt::xarray<float, xt::layout_type::row_major> cholesky_factor = cholesky_decomposition(covariance1);
        xt::xarray<float, xt::layout_type::row_major> z = xt::transpose(forward_substitution(cholesky_factor, xt::transpose(d)));
        auto zz = z * z;  // Element-wise multiplication
        xt::xarray<float> square_mahalanobis = xt::sum(zz, {1});
        return square_mahalanobis;
    }
};
__END_DECLS
