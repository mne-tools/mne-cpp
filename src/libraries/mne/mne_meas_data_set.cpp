//=============================================================================================================
/**
 * @file     mne_meas_data_set.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the MNE Meas Data Set (MNEMeasDataSet) Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_meas_data_set.h"

#include <fiff/fiff_file.h>

#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace MNELIB;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEMeasDataSet::MNEMeasDataSet()
: first(0)
, np(0)
, nave(1)
, kind(FIFFV_ASPECT_AVERAGE)
, tmin(0.0f)
, tstep(0.0f)
{
}

//=============================================================================================================

MNEMeasDataSet::~MNEMeasDataSet()
{
}

//=============================================================================================================

int MNEMeasDataSet::getValuesAtTime(float time, float integ, int nch, bool use_abs, float *value) const
{
    constexpr float EPS = 0.05f;
    const float sfreq = 1.0f / tstep;

    for (int ch = 0; ch < nch; ch++) {
        float sum;
        if (std::fabs(sfreq * integ) < EPS) {
            /* Single-sample case */
            float s1 = sfreq * (time - tmin);
            int n1 = static_cast<int>(std::floor(s1));
            float f1 = 1.0f + n1 - s1;
            if (n1 < 0 || n1 > np - 1) {
                qWarning("Sample value out of range %d (0..%d)", n1, np - 1);
                return -1;
            }
            if (n1 == np - 1) {
                if (std::fabs(f1 - 1.0f) < 1e-3f)
                    f1 = 1.0f;
            }
            if (f1 < 1.0f && n1 > np - 2) {
                qWarning("Sample value out of range %d (0..%d) %.4f", n1, np - 1, f1);
                return -1;
            }
            if (f1 < 1.0f) {
                if (use_abs)
                    sum = f1 * std::fabs(data(n1, ch)) + (1.0f - f1) * std::fabs(data(n1 + 1, ch));
                else
                    sum = f1 * data(n1, ch) + (1.0f - f1) * data(n1 + 1, ch);
            } else {
                sum = use_abs ? std::fabs(data(n1, ch)) : data(n1, ch);
            }
        } else {
            /* Multiple samples */
            float s1 = sfreq * (time - 0.5f * integ - tmin);
            float s2 = sfreq * (time + 0.5f * integ - tmin);
            int n1 = static_cast<int>(std::ceil(s1));
            int n2 = static_cast<int>(std::floor(s2));
            if (n2 < n1) {
                /* Within one sample interval */
                n1 = static_cast<int>(std::floor(s1));
                if (n1 < 0 || n1 > np - 2)
                    return -1;
                float f1 = s1 - n1;
                float f2 = s2 - n1;
                if (use_abs)
                    sum = 0.5f * ((f1 + f2) * std::fabs(data(n1 + 1, ch)) + (2.0f - f1 - f2) * std::fabs(data(n1, ch)));
                else
                    sum = 0.5f * ((f1 + f2) * data(n1 + 1, ch) + (2.0f - f1 - f2) * data(n1, ch));
            } else {
                float f1 = n1 - s1;
                float f2 = s2 - n2;
                if (n1 < 0 || n1 > np - 1) {
                    qWarning("Sample value out of range %d (0..%d)", n1, np - 1);
                    return -1;
                }
                if (n2 < 0 || n2 > np - 1) {
                    qWarning("Sample value out of range %d (0..%d)", n2, np - 1);
                    return -1;
                }
                if (f1 != 0.0f && n1 < 1)
                    return -1;
                if (f2 != 0.0f && n2 > np - 2)
                    return -1;
                sum = 0.0f;
                float width = 0.0f;
                if (n2 > n1) {
                    if (use_abs) {
                        sum = 0.5f * std::fabs(data(n1, ch));
                        for (int k = n1 + 1; k < n2; k++)
                            sum += std::fabs(data(k, ch));
                        sum += 0.5f * std::fabs(data(n2, ch));
                    } else {
                        sum = 0.5f * data(n1, ch);
                        for (int k = n1 + 1; k < n2; k++)
                            sum += data(k, ch);
                        sum += 0.5f * data(n2, ch);
                    }
                    width = static_cast<float>(n2 - n1);
                }
                if (use_abs) {
                    if (f1 != 0.0f)
                        sum += 0.5f * f1 * (f1 * std::fabs(data(n1 - 1, ch)) + (2.0f - f1) * std::fabs(data(n1, ch)));
                    if (f2 != 0.0f)
                        sum += 0.5f * f2 * (f2 * std::fabs(data(n2 + 1, ch)) + (2.0f - f2) * std::fabs(data(n2, ch)));
                } else {
                    if (f1 != 0.0f)
                        sum += 0.5f * f1 * (f1 * data(n1 - 1, ch) + (2.0f - f1) * data(n1, ch));
                    if (f2 != 0.0f)
                        sum += 0.5f * f2 * (f2 * data(n2 + 1, ch) + (2.0f - f2) * data(n2, ch));
                }
                width += f1 + f2;
                sum /= width;
            }
        }
        value[ch] = sum;
    }
    return 0;
}

//=============================================================================================================

int MNEMeasDataSet::getValuesFromChannelData(float time, float integ, float **data, int nsamp, int nch,
                                             float tmin, float sfreq, bool use_abs, float *value)
{
    constexpr float EPS = 0.05f;

    for (int ch = 0; ch < nch; ch++) {
        float sum;
        if (std::fabs(sfreq * integ) < EPS) {
            float s1 = sfreq * (time - tmin);
            int n1 = static_cast<int>(std::floor(s1));
            float f1 = 1.0f + n1 - s1;
            if (n1 < 0 || n1 > nsamp - 1)
                return -1;
            if (f1 < 1.0f && n1 > nsamp - 2)
                return -1;
            if (f1 < 1.0f) {
                if (use_abs)
                    sum = f1 * std::fabs(data[ch][n1]) + (1.0f - f1) * std::fabs(data[ch][n1 + 1]);
                else
                    sum = f1 * data[ch][n1] + (1.0f - f1) * data[ch][n1 + 1];
            } else {
                sum = use_abs ? std::fabs(data[ch][n1]) : data[ch][n1];
            }
        } else {
            float s1 = sfreq * (time - 0.5f * integ - tmin);
            float s2 = sfreq * (time + 0.5f * integ - tmin);
            int n1 = static_cast<int>(std::ceil(s1));
            int n2 = static_cast<int>(std::floor(s2));
            if (n2 < n1) {
                n1 = static_cast<int>(std::floor(s1));
                if (n1 < 0 || n1 > nsamp - 2)
                    return -1;
                float f1 = s1 - n1;
                float f2 = s2 - n1;
                if (use_abs)
                    sum = 0.5f * ((f1 + f2) * std::fabs(data[ch][n1 + 1]) + (2.0f - f1 - f2) * std::fabs(data[ch][n1]));
                else
                    sum = 0.5f * ((f1 + f2) * data[ch][n1 + 1] + (2.0f - f1 - f2) * data[ch][n1]);
            } else {
                float f1 = n1 - s1;
                float f2 = s2 - n2;
                if (n1 < 0 || n1 > nsamp - 1 || n2 < 0 || n2 > nsamp - 1)
                    return -1;
                if (f1 != 0.0f && n1 < 1)
                    return -1;
                if (f2 != 0.0f && n2 > nsamp - 2)
                    return -1;
                sum = 0.0f;
                float width = 0.0f;
                if (n2 > n1) {
                    if (use_abs) {
                        sum = 0.5f * std::fabs(data[ch][n1]);
                        for (int k = n1 + 1; k < n2; k++)
                            sum += std::fabs(data[ch][k]);
                        sum += 0.5f * std::fabs(data[ch][n2]);
                    } else {
                        sum = 0.5f * data[ch][n1];
                        for (int k = n1 + 1; k < n2; k++)
                            sum += data[ch][k];
                        sum += 0.5f * data[ch][n2];
                    }
                    width = static_cast<float>(n2 - n1);
                }
                if (use_abs) {
                    if (f1 != 0.0f)
                        sum += 0.5f * f1 * (f1 * std::fabs(data[ch][n1 - 1]) + (2.0f - f1) * std::fabs(data[ch][n1]));
                    if (f2 != 0.0f)
                        sum += 0.5f * f2 * (f2 * std::fabs(data[ch][n2 + 1]) + (2.0f - f2) * std::fabs(data[ch][n2]));
                } else {
                    if (f1 != 0.0f)
                        sum += 0.5f * f1 * (f1 * data[ch][n1 - 1] + (2.0f - f1) * data[ch][n1]);
                    if (f2 != 0.0f)
                        sum += 0.5f * f2 * (f2 * data[ch][n2 + 1] + (2.0f - f2) * data[ch][n2]);
                }
                width += f1 + f2;
                sum /= width;
            }
        }
        value[ch] = sum;
    }
    return 0;
}
