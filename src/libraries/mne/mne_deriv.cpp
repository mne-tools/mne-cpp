//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_deriv.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implementation of @ref MNELIB::MNEDeriv (MNE-C mne_derivations.c).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_deriv.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEDeriv::MNEDeriv() = default;

//=============================================================================================================

MNEDeriv::MNEDeriv(const MNEDeriv& other)
: filename(other.filename)
, shortname(other.shortname)
, deriv_data(other.deriv_data ? std::make_unique<MNESparseNamedMatrix>() : nullptr)
, in_use(other.in_use)
, valid(other.valid)
, chs(other.chs)
{
    if (deriv_data) {
        deriv_data->nrow = other.deriv_data->nrow;
        deriv_data->ncol = other.deriv_data->ncol;
        deriv_data->rowlist = other.deriv_data->rowlist;
        deriv_data->collist = other.deriv_data->collist;
        if (other.deriv_data->data) {
            deriv_data->data = std::make_unique<FiffSparseMatrix>(*other.deriv_data->data);
        }
    }
}

//=============================================================================================================

MNEDeriv::~MNEDeriv() = default;

//=============================================================================================================

int MNEDeriv::validate(const QList<FiffChInfo>& chInfo)
{
    const int nrow = deriv_data ? deriv_data->nrow : 0;
    valid = Eigen::VectorXi::Zero(nrow);
    chs.clear();
    for (int j = 0; j < nrow; ++j) {
        chs.append(FiffChInfo());
    }
    if (chInfo.isEmpty() || nrow == 0) {
        return 0;
    }
    QStringList names;
    for (const FiffChInfo& ch : chInfo) {
        names << ch.ch_name;
    }
    // Row-major view: the inputs of derived channel j are the non-zeros of row j.
    const Eigen::SparseMatrix<float, Eigen::RowMajor> rows = deriv_data->data->eigen();
    int nvalid = 0;
    for (int j = 0; j < nrow; ++j) {
        const FiffChInfo* first = nullptr;
        bool ok = true;
        for (Eigen::SparseMatrix<float, Eigen::RowMajor>::InnerIterator it(rows, j); it && ok; ++it) {
            const int pick = static_cast<int>(names.indexOf(deriv_data->collist.value(static_cast<int>(it.col()))));
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
