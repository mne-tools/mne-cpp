//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     fwd_coil.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     March 2017
 * @brief    FwdCoil implementation — construction, copy, coordinate-frame transforms and coil-class predicates for a single MEG sensor coil or EEG electrode.
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
