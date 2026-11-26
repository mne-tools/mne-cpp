//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rt_raw_display.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Implementation of RtRawDisplay.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rt_raw_display.h"
#include "../sphara.h"
#include <mne/base/log.h>
#include <mne/base/thread_pool.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/fiff/fiff_info.h>
#include <mne/math/matrix_io.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE STATIC FUNCTIONS
//=============================================================================================================

namespace
{

VectorXi rows_where(const std::vector<FiffChInfo>& chs, bool (*pick)(const FiffChInfo&))
{
    std::vector<int> rows;
    rows.reserve(chs.size());
    for (int r = 0; r < std::ssize(chs); ++r) {
        if (pick(chs[static_cast<std::size_t>(r)])) {
            rows.push_back(r);
        }
    }
    return Map<const VectorXi>(rows.data(), static_cast<Index>(rows.size()));
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void RtRawDisplay::set_info(std::shared_ptr<const FiffInfo> info)
{
    m_info = std::move(info);
    const int channels = m_info ? static_cast<int>(m_info->chs.size()) : 0;

    m_correction.reset(channels);
    m_sphara = SparseMatrix<double>(channels, channels);
    m_sphara.setIdentity();
    m_overlap = MatrixXd::Zero(channels, m_maxFilterLength);

    if (!m_info) {
        m_filteredRows.clear();
        return;
    }

    update_compensator(0);
    set_filter_channel_type(m_filterChannelType);

    m_indicesVVGrad = rows_where(m_info->chs, [](const FiffChInfo& ch) { return ch.chpos.coil_type == FIFFV_COIL_VV_PLANAR_T1; });
    m_indicesVVMag = rows_where(m_info->chs, [](const FiffChInfo& ch) { return ch.chpos.coil_type == FIFFV_COIL_VV_MAG_T3; });
    m_indicesBabyMEGInner = rows_where(m_info->chs, [](const FiffChInfo& ch) { return ch.chpos.coil_type == FIFFV_COIL_BABY_MAG; });
    m_indicesEEG = rows_where(m_info->chs, [](const FiffChInfo& ch) { return ch.kind == FIFFV_EEG_CH; });
}

//=============================================================================================================

void RtRawDisplay::update_projection()
{
    if (m_info) {
        m_correction.update_projection(*m_info);
    }
}

//=============================================================================================================

void RtRawDisplay::update_compensator(int to)
{
    if (m_info) {
        m_correction.update_compensator(*m_info, to);
    }
}

//=============================================================================================================

void RtRawDisplay::load_sphara_bases(const std::string& directory)
{
    read_matrix(m_spharaVVGrad, directory + "/Vectorview_SPHARA_InvEuclidean_Grad.txt");
    read_matrix(m_spharaVVMag, directory + "/Vectorview_SPHARA_InvEuclidean_Mag.txt");
    read_matrix(m_spharaBabyMEGInner, directory + "/BabyMEG_SPHARA_InvEuclidean_Inner.txt");
    read_matrix(m_spharaEEG, directory + "/Current_SPHARA_EEG.txt");

    Log::debug("RtRawDisplay::load_sphara_bases - VectorView mag {}x{}, grad {}x{}, BabyMEG inner {}x{}, EEG {}x{}",
               m_spharaVVMag.rows(), m_spharaVVMag.cols(), m_spharaVVGrad.rows(), m_spharaVVGrad.cols(),
               m_spharaBabyMEGInner.rows(), m_spharaBabyMEGInner.cols(), m_spharaEEG.rows(), m_spharaEEG.cols());
}

//=============================================================================================================

void RtRawDisplay::update_sphara_options(const std::string& system, int first, int second)
{
    if (!m_info) {
        return;
    }

    const auto channels = static_cast<Index>(m_info->chs.size());
    MatrixXd multFirst = MatrixXd::Identity(channels, channels);
    MatrixXd multSecond = MatrixXd::Identity(channels, channels);

    if (system == "VectorView" && m_spharaVVGrad.size() != 0 && m_spharaVVMag.size() != 0) {
        multFirst = makeSpharaProjector(m_spharaVVGrad, m_indicesVVGrad, m_info->nchan, first, 1);
        multSecond = makeSpharaProjector(m_spharaVVMag, m_indicesVVMag, m_info->nchan, second, 0);
    }

    if (system == "BabyMEG" && m_spharaBabyMEGInner.size() != 0) {
        multFirst = makeSpharaProjector(m_spharaBabyMEGInner, m_indicesBabyMEGInner, m_info->nchan, first, 0);
    }

    if (system == "EEG" && m_spharaEEG.size() != 0) {
        multFirst = makeSpharaProjector(m_spharaEEG, m_indicesEEG, m_info->nchan, first, 0);
    }

    const SparseMatrix<double> sparseFirst = multFirst.sparseView();
    const SparseMatrix<double> sparseSecond = multSecond.sparseView();
    m_sphara = sparseFirst * sparseSecond;
}

//=============================================================================================================

bool RtRawDisplay::applies_sphara(Index rows) const
{
    return m_spharaActive && m_sphara.cols() > 0 && rows == m_sphara.cols();
}

//=============================================================================================================

void RtRawDisplay::set_filters(std::vector<FilterKernel> kernels)
{
    m_kernels = std::move(kernels);

    m_maxFilterLength = 1;
    for (const FilterKernel& kernel : m_kernels) {
        m_maxFilterLength = std::max(m_maxFilterLength, kernel.filter_order());
    }

    m_overlap = MatrixXd::Zero(m_overlap.rows(), m_maxFilterLength);
    m_drawFilterFront = false;
}

//=============================================================================================================

void RtRawDisplay::set_filter_channel_type(const std::string& type)
{
    m_filterChannelType = type;
    m_filteredRows.assign(m_info ? m_info->chs.size() : 0, false);
    if (!m_info) {
        return;
    }

    for (std::size_t r = 0; r < m_info->chs.size(); ++r) {
        const FiffChInfo& ch = m_info->chs[r];
        const bool filterable = ch.kind == FIFFV_MEG_CH || ch.kind == FIFFV_EEG_CH || ch.kind == FIFFV_EOG_CH ||
            ch.kind == FIFFV_ECG_CH || ch.kind == FIFFV_EMG_CH;
        m_filteredRows[r] = filterable && (type == "All" || ch.ch_name.find(type) != std::string::npos);
    }
}

//=============================================================================================================

void RtRawDisplay::clear_filter_state()
{
    m_overlap.setZero();
}

//=============================================================================================================

void RtRawDisplay::set_trigger_detection(bool active, int channel, double threshold)
{
    m_triggerActive = active;
    m_triggerChannel = channel;
    m_triggerThreshold = threshold;
}

//=============================================================================================================

RtRawDisplay::Corrected RtRawDisplay::correct(const MatrixXd& block) const
{
    Corrected result;
    result.data = m_correction.apply(block);
    if (m_triggerActive) {
        result.triggers = detect_trigger_flanks_max(block, m_triggerChannel, 0, m_triggerThreshold, true, 500);
    }
    return result;
}

//=============================================================================================================

std::optional<RtRawDisplay::Filtered> RtRawDisplay::filter(const MatrixXd& corrected)
{
    if (corrected.cols() < m_maxFilterLength) {
        return std::nullopt;
    }

    Filtered result;
    result.data = corrected;
    result.zero_columns = m_drawFilterFront ? 0 : m_maxFilterLength;
    for (int r = 0; r < corrected.rows(); ++r) {
        const bool filtered = r < std::ssize(m_filteredRows) && m_filteredRows[static_cast<std::size_t>(r)];
        (filtered ? result.filtered_rows : result.passthrough_rows).push_back(r);
    }

    // Every row filters with its own copy of the kernels, since the FFT filter is not const.
    std::vector<RowVectorXd> rows(result.filtered_rows.size());
    ThreadPool::global().parallel_for(0, rows.size(), [&](std::size_t i) {
        rows[i] = corrected.row(result.filtered_rows[i]);
        std::vector<FilterKernel> kernels = m_kernels;
        for (FilterKernel& kernel : kernels) {
            // FFT convolution with the overhead kept, so the overlap carries into the next block.
            kernel.apply_fft_filter(rows[i], true);
        }
    });

    const Index cols = corrected.cols();
    if (m_overlap.rows() < corrected.rows()) {
        m_overlap.conservativeResize(corrected.rows(), m_maxFilterLength);
    }

    for (std::size_t i = 0; i < rows.size(); ++i) {
        const int row = result.filtered_rows[i];
        RowVectorXd& data = rows[i];

        if (m_drawFilterFront) {
            data.head(m_maxFilterLength) += m_overlap.row(row);
        } else {
            data.head(m_maxFilterLength).setZero();
        }

        result.data.row(row) = data.head(cols);
        m_overlap.row(row) = data.tail(m_maxFilterLength);
    }

    m_drawFilterFront = true;
    return result;
}
