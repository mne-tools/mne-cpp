//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     mne_deriv.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     January 2017
 * @brief    Implementation of @ref mne::MNEDeriv (MNE-C mne_derivations.c).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_deriv.h"

#include <mne/fiff/fiff_ch_name.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEDeriv::MNEDeriv() = default;

//=============================================================================================================

MNEDeriv::MNEDeriv(const MNEDeriv& other)
: filename(other.filename)
, shortname(other.shortname)
, deriv_data(other.deriv_data ? std::make_unique<MNESparseNamedMatrix>(*other.deriv_data) : nullptr)
, in_use(other.in_use)
, valid(other.valid)
, chs(other.chs)
{
}

//=============================================================================================================

MNEDeriv::~MNEDeriv() = default;

//=============================================================================================================

int MNEDeriv::validate(const std::vector<FiffChInfo>& chInfo)
{
    const int nrow = deriv_data ? deriv_data->nrow() : 0;
    valid = Eigen::VectorXi::Zero(nrow);
    chs.clear();
    for (int j = 0; j < nrow; ++j) {
        chs.push_back(FiffChInfo());
    }
    if (chInfo.empty() || nrow == 0) {
        return 0;
    }
    StringList names;
    for (const FiffChInfo& ch : chInfo) {
        names.push_back(ch.ch_name);
    }
    // Row-major view: the inputs of derived channel j are the non-zeros of row j.
    const Eigen::SparseMatrix<float, Eigen::RowMajor> rows = deriv_data->data.eigen();
    int nvalid = 0;
    for (int j = 0; j < nrow; ++j) {
        const FiffChInfo* first = nullptr;
        bool ok = true;
        for (Eigen::SparseMatrix<float, Eigen::RowMajor>::InnerIterator it(rows, j); it && ok; ++it) {
            const int pick = static_cast<int>(mne::channel_index(names, ((deriv_data->collist[static_cast<int>(it.col())]))));
            if (pick < 0) {
                ok = false;
            } else if (!first) {
                first = &chInfo[pick];
            } else {
                const FiffChInfo& ch = chInfo[pick];
                ok = ch.unit == first->unit && ch.unit_mul == first->unit_mul && ch.kind == first->kind;
            }
        }
        if (ok && first) {
            valid[j] = 1;
            chs[j] = *first;
            ++nvalid;
        }
    }
    return nvalid;
}
