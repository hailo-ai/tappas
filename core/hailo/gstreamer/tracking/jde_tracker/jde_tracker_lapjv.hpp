/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#pragma once

// General cpp includes
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

// Tappas includes
#include "lapjv.hpp"
#include "strack.hpp"
#include "tracker_macros.hpp"


/**
 * @brief Performs linear assignment on a given cost matrix.
 *        No return is made, instead vectors are filled with
 *        matching indices for row and column items.
 * 
 * @param cost  -  std::vector<std::vector<float>>
 *        A 2D cost matrix of distances between 2 sets of objects
 *
 * @param rowsol  -  std::vector<int>
 *        A vector to fill with matching indices of items in the columns
 *        ex: rowsol[0] = 2 means item 0 in the rows matches item 2 in the columns
 *
 * @param colsol  -  std::vector<int>
 *        A vector to fill with matching indices of items in the rows
 *        ex: colsol[0] = 2 means item 0 in the cols matches item 2 in the rows
 *
 * @param cost_limit  -  float
 *        The cost limit for lapjv
 *
 * @param return_cost  -  bool
 *        If true, then return the total cost, default true.
 */
inline double lapjv_external(const std::vector<std::vector<float>> &cost,
                             std::vector<int> &rowsol,
                             std::vector<int> &colsol,
                             float cost_limit = LONG_MAX, bool return_cost = true)
{
    std::vector<std::vector<float>> cost_c;
    cost_c.assign(cost.begin(), cost.end());

    std::vector<std::vector<float>> cost_c_extended;

    int n_rows = cost.size();
    int n_cols = cost[0].size();
    rowsol.resize(n_rows);
    colsol.resize(n_cols);

    int n = 0;
    if (n_rows == n_cols)
    {
        n = n_rows;
    }

    n = n_rows + n_cols;
    cost_c_extended.resize(n);
    for (uint i = 0; i < cost_c_extended.size(); i++) {
        cost_c_extended[i].resize(n);
    }

    for (uint i = 0; i < cost_c_extended.size(); i++)
    {
        for (uint j = 0; j < cost_c_extended[i].size(); j++)
        {
            cost_c_extended[i][j] = cost_limit / 2.0;
        }
    }

    for (uint i = n_rows; i < cost_c_extended.size(); i++)
    {
        for (uint j = n_cols; j < cost_c_extended[i].size(); j++)
        {
            cost_c_extended[i][j] = 0;
        }
    }
    for (int i = 0; i < n_rows; i++)
    {
        for (int j = 0; j < n_cols; j++)
        {
            cost_c_extended[i][j] = cost_c[i][j];
        }
    }

    cost_c.clear();
    cost_c.assign(cost_c_extended.begin(), cost_c_extended.end());

    double **cost_ptr;
    cost_ptr = new double *[sizeof(double *) * n];
    for (int i = 0; i < n; i++)
        cost_ptr[i] = new double[sizeof(double) * n];

    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            cost_ptr[i][j] = cost_c[i][j];
        }
    }

    int x_c[n];
    int y_c[n];

    int ret = lapjv_internal(n, cost_ptr, x_c, y_c);
    if (ret != 0)
    {
        throw std::runtime_error("JDETracker error: incorrect lapjv calculation!");
    }

    double opt = 0.0;

    if (n != n_rows)
    {
        for (int i = 0; i < n; i++)
        {
            if (x_c[i] >= n_cols)
                x_c[i] = -1;
            if (y_c[i] >= n_rows)
                y_c[i] = -1;
        }
        for (int i = 0; i < n_rows; i++)
        {
            rowsol[i] = x_c[i];
        }
        for (int i = 0; i < n_cols; i++)
        {
            colsol[i] = y_c[i];
        }

        if (return_cost)
        {
            for (uint i = 0; i < rowsol.size(); i++)
            {
                if (rowsol[i] != -1)
                {
                    opt += cost_ptr[i][rowsol[i]];
                }
            }
        }
    }
    else if (return_cost)
    {
        for (uint i = 0; i < rowsol.size(); i++)
        {
            opt += cost_ptr[i][rowsol[i]];
        }
    }

    for (int i = 0; i < n; i++)
    {
        delete[] cost_ptr[i];
    }
    delete[] cost_ptr;

    return opt;
}


/**
 * @brief Performs linear assignment on a given cost matrix.
 *        No return is made, instead a given matrix of matches is filled,
 *        and vectors are filled for unmatched members of each list.
 * 
 * @param cost_matrix  -  std::vector<std::vector<float>>
 *        A 2D cost matrix of distances between 2 sets of objects
 *
 * @param thresh  -  float
 *        The cost limit for lapjv
 *
 * @param matches  -  std::vector<std::pair<int,int>>
 *        A vector of paired matched objects
 *
 * @param unmatched_a  -  std::vector<int>
 *        Indices of unmatched objects from the row items
 *
 * @param unmatched_b  - std::vector<int>
 *        Indices of unmatched objects from the column items
 */
inline void JDETracker::linear_assignment(std::vector<std::vector<float>> &cost_matrix,
                                          int cost_matrix_rows,
                                          int cost_matrix_cols,
                                          float thresh,
                                          std::vector<std::pair<int,int>> &matches,
                                          std::vector<int> &unmatched_a,
                                          std::vector<int> &unmatched_b)
{
    // Clear the vectors to fill in
    matches.clear();
    unmatched_a.clear();
    unmatched_b.clear();

	if (cost_matrix.size() == 0)
	{
		for (int i = 0; i < cost_matrix_rows; i++)
		{
			unmatched_a.push_back(i);
		}
		for (int i = 0; i < cost_matrix_cols; i++)
		{
			unmatched_b.push_back(i);
		}
		return;
	}

    std::vector<int> rowsol;
    std::vector<int> colsol;
    lapjv_external(cost_matrix, rowsol, colsol, thresh);

    for (uint i = 0; i < rowsol.size(); i++)
    {
        if (rowsol[i] >= 0)
        {
            matches.push_back(std::make_pair(i, rowsol[i]));
        }
        else
        {
            unmatched_a.push_back(i);
        }
    }

    for (uint i = 0; i < colsol.size(); i++)
    {
        if (colsol[i] < 0)
        {
            unmatched_b.push_back(i);
        }
    }
}
