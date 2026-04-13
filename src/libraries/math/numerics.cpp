//=============================================================================================================
/**
 * @file     numerics.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Definition of the Numerics class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include "numerics.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QStringList>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

int Numerics::gcd(int iA, int iB)
{
    if (iB == 0) {
        return iA;
    }

    return gcd(iB, iA % iB);
}

//=============================================================================================================

bool Numerics::issparse(VectorXd &v)
{
    qint32 c = 0;
    qint32 n = v.rows();
    qint32 t = n/2;

    for(qint32 i = 0; i < n; ++i)
    {
        if(v(i) == 0)
            ++c;
        if(c > t)
            return true;
    }

    return false;
}

//=============================================================================================================

int Numerics::nchoose2(int n)
{
    int t_iNumOfCombination = static_cast<int>(n*(n-1)*0.5);

    return t_iNumOfCombination;
}

//=============================================================================================================

MatrixXd Numerics::rescale(const MatrixXd &data,
                           const RowVectorXf &times,
                           const QPair<float,float>& baseline,
                           QString mode)
{
    MatrixXd data_out = data;
    QStringList valid_modes;
    valid_modes << "logratio" << "ratio" << "zscore" << "mean" << "percent";
    if(!valid_modes.contains(mode))
    {
        qWarning().noquote() << "[Numerics::rescale] Mode" << mode << "is not supported. Supported modes are:" << valid_modes << "Returning input data.";
        return data_out;
    }

    qInfo().noquote() << QString("[Numerics::rescale] Applying baseline correction ... (mode: %1)").arg(mode);

    qint32 imin = 0;
    qint32 imax = times.size();

    if (baseline.second == baseline.first) {
        imin = 0;
    } else {
        float bmin = baseline.first;
        for(qint32 i = 0; i < times.size(); ++i) {
            if(times[i] >= bmin) {
                imin = i;
                break;
            }
        }
    }

    float bmax = baseline.second;

    if (baseline.second == baseline.first) {
        bmax = 0;
    }

    for(qint32 i = times.size()-1; i >= 0; --i) {
        if(times[i] <= bmax) {
            imax = i+1;
            break;
        }
    }

    if(imax < imin) {
        qWarning() << "[Numerics::rescale] imax < imin. Returning input data.";
        return data_out;
    }

    VectorXd mean = data_out.block(0, imin, data_out.rows(), imax-imin).rowwise().mean();
    if(mode.compare("mean") == 0) {
        data_out -= mean.rowwise().replicate(data.cols());
    } else if(mode.compare("logratio") == 0) {
        for(qint32 i = 0; i < data_out.rows(); ++i)
            for(qint32 j = 0; j < data_out.cols(); ++j)
                data_out(i,j) = log10(data_out(i,j)/mean[i]);
    } else if(mode.compare("ratio") == 0) {
        data_out = data_out.cwiseQuotient(mean.rowwise().replicate(data_out.cols()));
    } else if(mode.compare("zscore") == 0) {
        MatrixXd std_mat = data.block(0, imin, data.rows(), imax-imin) - mean.rowwise().replicate(imax-imin);
        std_mat = std_mat.cwiseProduct(std_mat);
        VectorXd std_v = std_mat.rowwise().mean();
        for(qint32 i = 0; i < std_v.size(); ++i)
            std_v[i] = sqrt(std_v[i] / static_cast<float>(imax-imin));

        data_out -= mean.rowwise().replicate(data_out.cols());
        data_out = data_out.cwiseQuotient(std_v.rowwise().replicate(data_out.cols()));
    } else if(mode.compare("percent") == 0) {
        data_out -= mean.rowwise().replicate(data_out.cols());
        data_out = data_out.cwiseQuotient(mean.rowwise().replicate(data_out.cols()));
    }

    return data_out;
}

//=============================================================================================================

MatrixXd Numerics::rescale(const MatrixXd& data,
                           const RowVectorXf& times,
                           const std::pair<float,float>& baseline,
                           const std::string& mode)
{
    MatrixXd data_out = data;
    std::vector<std::string> valid_modes{"logratio", "ratio", "zscore", "mean", "percent"};
    if(std::find(valid_modes.begin(), valid_modes.end(), mode) == valid_modes.end())
    {
        qWarning().noquote() << "[Numerics::rescale] Mode" << mode.c_str() << "is not supported. Supported modes are:";
        for(auto& m : valid_modes){
            std::cout << m << " ";
        }
        std::cout << "\n" << "Returning input data.\n";
        return data_out;
    }

    qInfo().noquote() << QString("[Numerics::rescale] Applying baseline correction ... (mode: %1)").arg(mode.c_str());

    qint32 imin = 0;
    qint32 imax = times.size();

    if (baseline.second == baseline.first) {
        imin = 0;
    } else {
        float bmin = baseline.first;
        for(qint32 i = 0; i < times.size(); ++i) {
            if(times[i] >= bmin) {
                imin = i;
                break;
            }
        }
    }

    float bmax = baseline.second;

    if (baseline.second == baseline.first) {
        bmax = 0;
    }

    for(qint32 i = times.size()-1; i >= 0; --i) {
        if(times[i] <= bmax) {
            imax = i+1;
            break;
        }
    }

    if(imax < imin) {
        qWarning() << "[Numerics::rescale] imax < imin. Returning input data.";
        return data_out;
    }

    VectorXd mean = data_out.block(0, imin, data_out.rows(), imax-imin).rowwise().mean();
    if(mode.compare("mean") == 0) {
        data_out -= mean.rowwise().replicate(data.cols());
    } else if(mode.compare("logratio") == 0) {
        for(qint32 i = 0; i < data_out.rows(); ++i)
            for(qint32 j = 0; j < data_out.cols(); ++j)
                data_out(i,j) = log10(data_out(i,j)/mean[i]);
    } else if(mode.compare("ratio") == 0) {
        data_out = data_out.cwiseQuotient(mean.rowwise().replicate(data_out.cols()));
    } else if(mode.compare("zscore") == 0) {
        MatrixXd std_mat = data.block(0, imin, data.rows(), imax-imin) - mean.rowwise().replicate(imax-imin);
        std_mat = std_mat.cwiseProduct(std_mat);
        VectorXd std_v = std_mat.rowwise().mean();
        for(qint32 i = 0; i < std_v.size(); ++i)
            std_v[i] = sqrt(std_v[i] / static_cast<float>(imax-imin));

        data_out -= mean.rowwise().replicate(data_out.cols());
        data_out = data_out.cwiseQuotient(std_v.rowwise().replicate(data_out.cols()));
    } else if(mode.compare("percent") == 0) {
        data_out -= mean.rowwise().replicate(data_out.cols());
        data_out = data_out.cwiseQuotient(mean.rowwise().replicate(data_out.cols()));
    }

    return data_out;
}
