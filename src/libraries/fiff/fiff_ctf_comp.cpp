//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_ctf_comp.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     October 2012
 * @brief    Implementation of @ref FiffCtfComp: read / write of FIFFB_MNE_CTF_COMP_DATA blocks and the gradient-compensation matrix algebra.
 *
 * Implements the matrix lookups used by @c FiffRawData::compensate to
 * shift continuous data between CTF software-compensation grades.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_ctf_comp.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffCtfComp::FiffCtfComp()
: ctfkind(-1)
, kind(-1)
, save_calibrated(false)
, data(FiffNamedMatrix::SDPtr(new FiffNamedMatrix()))
{
}

//=============================================================================================================

FiffCtfComp::FiffCtfComp(const FiffCtfComp& p_FiffCtfComp)
: ctfkind(p_FiffCtfComp.ctfkind)
, kind(p_FiffCtfComp.kind)
, save_calibrated(p_FiffCtfComp.save_calibrated)
, rowcals(p_FiffCtfComp.rowcals)
, colcals(p_FiffCtfComp.colcals)
, data(p_FiffCtfComp.data)
{
}

//=============================================================================================================

FiffCtfComp::~FiffCtfComp()
{
}

//=============================================================================================================

void FiffCtfComp::clear()
{
    ctfkind = -1;
    kind = -1;
    save_calibrated = false;
    rowcals = MatrixXd();
    colcals = MatrixXd();
    data = FiffNamedMatrix::SDPtr(new FiffNamedMatrix());
}

//=============================================================================================================

namespace
{

int uniqueChannel(const QList<FiffChInfo>& chs, const QString& name)
{
    int found = -1;
    for (int i = 0; i < chs.size(); ++i) {
        if (chs[i].ch_name == name) {
            if (found >= 0) {
                qWarning("Ambiguous channel %s", name.toUtf8().constData());
                return -1;
            }
            found = i;
        }
    }
    if (found < 0)
        qWarning("Channel %s is not available in data", name.toUtf8().constData());
    return found;
}

} // namespace

bool FiffCtfComp::calibrate(const QList<FiffChInfo>& chs)
{
    rowcals = MatrixXd::Ones(data->data.rows(), 1);
    colcals = MatrixXd::Ones(data->data.cols(), 1);
    if (save_calibrated)
        return true;
    for (Index col = 0; col < data->data.cols(); ++col) {
        const int p = uniqueChannel(chs, data->col_names.value(col));
        if (p < 0)
            return false;
        colcals(col) = 1.0 / (static_cast<double>(chs[p].range) * chs[p].cal);
    }
    for (Index row = 0; row < data->data.rows(); ++row) {
        const int p = uniqueChannel(chs, data->row_names.value(row));
        if (p < 0)
            return false;
        rowcals(row) = static_cast<double>(chs[p].range) * chs[p].cal;
    }
    data->data = rowcals.asDiagonal() * data->data * colcals.asDiagonal();
    return true;
}
