//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_meas_data_set.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implementation of @ref mne::MNEMeasDataSet.
 *
 * Provides constructors, deep-copy of the per-condition sample matrix
 * and the timing helpers used by the legacy C-ported algorithms.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_meas_data_set.h"
#include <base/log.h>

#include <fiff/fiff_file.h>

#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace mne;

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

int MNEMeasDataSet::getValuesAtTime(float time, float integ, bool use_abs, Eigen::Ref<Eigen::VectorXf> value) const
{
    constexpr float EPS = 0.05f;
    const float sfreq = 1.0f / tstep;
    const auto nch = static_cast<int>(value.size());

    if (value.size() > data.cols()) {
        mne::Log::warning("Cannot pick {} values from data with {} channels", value.size(), data.cols());
        return -1;
    }

    for (int ch = 0; ch < nch; ch++) {
        float sum;
        if (std::fabs(sfreq * integ) < EPS) {
            /* Single-sample case */
            float s1 = sfreq * (time - tmin);
            int n1 = static_cast<int>(std::floor(s1));
            float f1 = 1.0f + n1 - s1;
            if (n1 < 0 || n1 > np - 1) {
                mne::Log::warning("Sample value out of range {} (0..{})", n1, np - 1);
                return -1;
            }
            if (n1 == np - 1) {
                if (std::fabs(f1 - 1.0f) < 1e-3f)
                    f1 = 1.0f;
            }
            if (f1 < 1.0f && n1 > np - 2) {
                mne::Log::warning("Sample value out of range {} (0..{}) {:.4f}", n1, np - 1, f1);
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
                    mne::Log::warning("Sample value out of range {} (0..{})", n1, np - 1);
                    return -1;
                }
                if (n2 < 0 || n2 > np - 1) {
                    mne::Log::warning("Sample value out of range {} (0..{})", n2, np - 1);
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

int MNEMeasDataSet::getValuesFromChannelData(float time, float integ,
                                             const Eigen::Ref<const Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>>& data,
                                             float tmin, float sfreq, bool use_abs, Eigen::Ref<Eigen::VectorXf> value)
{
    constexpr float EPS = 0.05f;
    const auto nch = static_cast<int>(data.rows());
    const auto nsamp = static_cast<int>(data.cols());

    if (value.size() != data.rows()) {
        mne::Log::warning("Cannot pick channel values: the output holds {} values for {} channels", value.size(), nch);
        return -1;
    }

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
                    sum = f1 * std::fabs(data(ch, n1)) + (1.0f - f1) * std::fabs(data(ch, n1 + 1));
                else
                    sum = f1 * data(ch, n1) + (1.0f - f1) * data(ch, n1 + 1);
            } else {
                sum = use_abs ? std::fabs(data(ch, n1)) : data(ch, n1);
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
                    sum = 0.5f * ((f1 + f2) * std::fabs(data(ch, n1 + 1)) + (2.0f - f1 - f2) * std::fabs(data(ch, n1)));
                else
                    sum = 0.5f * ((f1 + f2) * data(ch, n1 + 1) + (2.0f - f1 - f2) * data(ch, n1));
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
                        sum = 0.5f * std::fabs(data(ch, n1));
                        for (int k = n1 + 1; k < n2; k++)
                            sum += std::fabs(data(ch, k));
                        sum += 0.5f * std::fabs(data(ch, n2));
                    } else {
                        sum = 0.5f * data(ch, n1);
                        for (int k = n1 + 1; k < n2; k++)
                            sum += data(ch, k);
                        sum += 0.5f * data(ch, n2);
                    }
                    width = static_cast<float>(n2 - n1);
                }
                if (use_abs) {
                    if (f1 != 0.0f)
                        sum += 0.5f * f1 * (f1 * std::fabs(data(ch, n1 - 1)) + (2.0f - f1) * std::fabs(data(ch, n1)));
                    if (f2 != 0.0f)
                        sum += 0.5f * f2 * (f2 * std::fabs(data(ch, n2 + 1)) + (2.0f - f2) * std::fabs(data(ch, n2)));
                } else {
                    if (f1 != 0.0f)
                        sum += 0.5f * f1 * (f1 * data(ch, n1 - 1) + (2.0f - f1) * data(ch, n1));
                    if (f2 != 0.0f)
                        sum += 0.5f * f2 * (f2 * data(ch, n2 + 1) + (2.0f - f2) * data(ch, n2));
                }
                width += f1 + f2;
                sum /= width;
            }
        }
        value[ch] = sum;
    }
    return 0;
}
