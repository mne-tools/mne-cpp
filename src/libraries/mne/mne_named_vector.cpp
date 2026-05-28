//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_named_vector.cpp
 * @since 2026
 * @date  April 2026
 * @brief Implementation of @ref MNELIB::MNENamedVector.
 *
 * Implements constructors and the name-based lookup helpers; FIFF I/O is
 * handled by the surrounding matrix readers because named vectors are
 * almost always written as a degenerate named matrix.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_named_vector.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace MNELIB;

constexpr int FAIL = -1;
constexpr int OK   =  0;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

int MNENamedVector::pick(const QStringList& pick_names, int nnames, bool require_all, Eigen::Ref<Eigen::VectorXf> res) const
{
    int found;
    int k,p;

    if (names.size() == 0) {
        qCritical("No names present in vector. Cannot pick.");
        return FAIL;
    }

    for (k = 0; k < nnames; k++)
        res[k] = 0.0;

    for (k = 0; k < nnames; k++) {
        found = 0;
        for (p = 0; p < nvec; p++) {
            if (QString::compare(names[p],pick_names[k]) == 0) {
                res[k] = data[p];
                found = true;
                break;
            }
        }
        if (!found && require_all) {
            qCritical("All required elements not found in named vector.");
            return FAIL;
        }
    }
    return OK;
}
