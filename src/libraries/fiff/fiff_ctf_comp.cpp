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
, kind (-1)
, save_calibrated(false)
, data(new FiffNamedMatrix())
{
}

//=============================================================================================================

FiffCtfComp::FiffCtfComp(const FiffCtfComp &p_FiffCtfComp)
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
