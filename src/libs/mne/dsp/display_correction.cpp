//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     display_correction.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Implementation of DisplayCorrection.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "display_correction.h"
#include <mne/fiff/fiff_ctf_comp.h>
#include <mne/fiff/fiff_info.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void DisplayCorrection::reset(int channels)
{
    m_projActive = false;
    m_compActive = false;
    m_proj.resize(0, 0);
    m_comp.resize(0, 0);
    m_sparseProj = SparseMatrix<double>(channels, channels);
    m_sparseProj.setIdentity();
    m_sparseComp = m_sparseProj;
    m_sparseProjComp = m_sparseProj;
}

//=============================================================================================================

void DisplayCorrection::update_projection(const FiffInfo& info)
{
    m_projActive = std::ranges::any_of(info.projs, [](const FiffProj& proj) { return proj.active; });

    info.make_projector(m_proj);

    StringList emptyExclude;
    const RowVectorXi bads = info.bads.empty() ? RowVectorXi() : FiffInfoBase::pick_channels(info.ch_names, info.bads, emptyExclude);
    for (Index j = 0; j < bads.size(); ++j) {
        if (bads[j] < m_proj.cols()) {
            m_proj.col(bads[j]).setZero();
        }
    }

    m_sparseProj = m_proj.sparseView();
    combine();
}

//=============================================================================================================

void DisplayCorrection::update_compensator(const FiffInfo& info, int to)
{
    m_compActive = to != 0;

    // Always from grade 0: the viewer receives uncompensated data and never re-applies to corrected data.
    FiffCtfComp comp;
    info.make_compensator(0, to, comp);
    m_comp = comp.data.data;

    m_sparseComp = m_comp.sparseView();
    combine();
}

//=============================================================================================================

MatrixXd DisplayCorrection::apply(const MatrixXd& data) const
{
    const bool doProj = m_projActive && data.cols() > 0 && data.rows() == m_proj.cols();
    const bool doComp = m_compActive && data.cols() > 0 && data.rows() == m_comp.cols();

    if (doComp) {
        return doProj ? MatrixXd(m_sparseProjComp * data) : MatrixXd(m_sparseComp * data);
    }
    return doProj ? MatrixXd(m_sparseProj * data) : data;
}

//=============================================================================================================

void DisplayCorrection::combine()
{
    if (m_sparseProj.cols() == m_sparseComp.rows()) {
        m_sparseProjComp = m_sparseProj * m_sparseComp;
    }
}
