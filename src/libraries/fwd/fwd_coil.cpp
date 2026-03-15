//=============================================================================================================
/**
 * @file     fwd_coil.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     December, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the FwdCoil Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_coil.h"
#include <fiff/fiff_ch_info.h>
#include <stdio.h>
#include <algorithm>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace FWDLIB;



//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FwdCoil::FwdCoil(int p_np)
{
    coil_class = FWD_COILC_UNKNOWN;
    accuracy   = FWD_COIL_ACCURACY_POINT;
    base       = 0.0;
    size       = 0.0;
    np         = p_np;
    rmag       = Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor>::Zero(np, 3);
    cosmag     = Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor>::Zero(np, 3);
    w          = Eigen::VectorXf::Zero(np);
    /*
   * Reasonable defaults
   */
    for (int k = 0; k < 3; k++) {
        r0[k] = 0.0;
        ex[k] = 0.0;
        ey[k] = 0.0;
        ez[k] = 0.0;
    }
    ex[0] = 1.0;
    ey[1] = 1.0;
    ez[2] = 1.0;
}

//=============================================================================================================

FwdCoil::FwdCoil(const FwdCoil& p_FwdCoil)
{
    if (!p_FwdCoil.chname.isEmpty())
        this->chname   = p_FwdCoil.chname;
    if (!p_FwdCoil.desc.isEmpty())
        this->desc   = p_FwdCoil.desc;
    this->coil_class = p_FwdCoil.coil_class;
    this->accuracy   = p_FwdCoil.accuracy;
    this->base       = p_FwdCoil.base;
    this->size       = p_FwdCoil.size;
    this->np         = p_FwdCoil.np;
    this->type       = p_FwdCoil.type;

    rmag   = p_FwdCoil.rmag;
    cosmag = p_FwdCoil.cosmag;
    w      = p_FwdCoil.w;

    this->r0 = p_FwdCoil.r0;
    this->ex = p_FwdCoil.ex;
    this->ey = p_FwdCoil.ey;
    this->ez = p_FwdCoil.ez;

    this->coord_frame = p_FwdCoil.coord_frame;
}

//=============================================================================================================

FwdCoil::~FwdCoil()
{
}

//=============================================================================================================

FwdCoil::UPtr FwdCoil::create_eeg_el(const FiffChInfo& ch, const FiffCoordTrans& t)
{
    if (ch.kind != FIFFV_EEG_CH) {
        qWarning() << ch.ch_name << "is not an EEG channel. Cannot create an electrode definition.";
        return nullptr;
    }
    if (!t.isEmpty() && t.from != FIFFV_COORD_HEAD) {
        qWarning("Inappropriate coordinate transformation in fwd_create_eeg_el");
        return nullptr;
    }

    FwdCoil::UPtr res;
    if (ch.chpos.ex.norm() < 1e-4)
        res = std::make_unique<FwdCoil>(1);   /* No reference electrode */
    else
        res = std::make_unique<FwdCoil>(2);   /* Reference electrode present */

    res->chname     = ch.ch_name;
    res->desc       = "EEG electrode";
    res->coil_class = FWD_COILC_EEG;
    res->accuracy   = FWD_COIL_ACCURACY_NORMAL;
    res->type       = ch.chpos.coil_type;
    res->r0 = ch.chpos.r0;
    res->ex = ch.chpos.ex;
    /*
     * Optional coordinate transformation
     */
    if (!t.isEmpty()) {
        FiffCoordTrans::apply_trans(res->r0.data(),t,FIFFV_MOVE);
        FiffCoordTrans::apply_trans(res->ex.data(),t,FIFFV_MOVE);
        res->coord_frame = t.to;
    }
    else
        res->coord_frame = FIFFV_COORD_HEAD;
    /*
     * The electrode location
     */
    res->rmag.row(0) = res->r0.transpose();
    res->cosmag.row(0) = res->r0.transpose();
    res->cosmag.row(0).normalize();
    res->w[0] = 1.0;
    /*
     * Add the reference electrode, if appropriate
     */
    if (res->np == 2) {
        res->rmag.row(1) = res->ex.transpose();
        res->cosmag.row(1) = res->ex.transpose();
        res->cosmag.row(1).normalize();
        res->w[1] = -1.0;
    }
    return res;
}

//=============================================================================================================

bool FwdCoil::is_axial_coil() const
{
    return (this->coil_class == FWD_COILC_MAG ||
            this->coil_class == FWD_COILC_AXIAL_GRAD ||
            this->coil_class == FWD_COILC_AXIAL_GRAD2);
}

//=============================================================================================================

bool FwdCoil::is_magnetometer_coil() const
{
    return this->coil_class == FWD_COILC_MAG;
}

//=============================================================================================================

bool FwdCoil::is_planar_coil() const
{
    return this->coil_class == FWD_COILC_PLANAR_GRAD;
}

//=============================================================================================================

bool FwdCoil::is_eeg_electrode() const
{
    return this->coil_class == FWD_COILC_EEG;
}
