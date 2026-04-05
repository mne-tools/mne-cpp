//=============================================================================================================
/**
 * @file     fwd_coil_set.cpp
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
 * @brief    Definition of the FwdCoilSet Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_coil_set.h"
#include "fwd_coil.h"
#include "fwd_bem_solution.h"

#include <fiff/fiff_ch_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QFile>
#include <QLocale>
#include <QTextStream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace FWDLIB;

namespace {
constexpr float BIG = 0.5f;
}

/**
 * Read a possibly-quoted word from a QTextStream (comments already stripped).
 */
static QString readWord(QTextStream &in)
{
    in.skipWhiteSpace();
    if (in.atEnd()) return QString();

    QChar ch;
    in >> ch;

    if (ch == '"') {
        QString word;
        while (!in.atEnd()) {
            in >> ch;
            if (ch == '"') break;
            word += ch;
        }
        return word;
    }

    QString word(ch);
    while (!in.atEnd()) {
        in >> ch;
        if (ch.isSpace()) break;
        word += ch;
    }
    return word;
}

FwdCoil* FwdCoilSet::fwd_add_coil_to_set(int type, int coil_class, int acc, int np, float size, float base, const QString& desc)
{
    if (np <= 0) {
        qWarning("Number of integration points should be positive (type = %d acc = %d)",type,acc);
        return nullptr;
    }
    if (! (acc == FWD_COIL_ACCURACY_POINT ||
           acc == FWD_COIL_ACCURACY_NORMAL ||
           acc == FWD_COIL_ACCURACY_ACCURATE) ) {
        qWarning("Illegal accuracy (type = %d acc = %d)",type,acc);
        return nullptr;
    }
    if (! (coil_class == FWD_COILC_MAG ||
           coil_class == FWD_COILC_AXIAL_GRAD ||
           coil_class == FWD_COILC_PLANAR_GRAD ||
           coil_class == FWD_COILC_AXIAL_GRAD2) ) {
        qWarning("Illegal coil class (type = %d acc = %d class = %d)",type,acc,coil_class);
        return nullptr;
    }

    coils.push_back(std::make_unique<FwdCoil>(np));
    FwdCoil* def = coils.back().get();

    def->type       = type;
    def->coil_class = coil_class;
    def->accuracy   = acc;
    def->np         = np;
    def->size       = size;
    def->base       = base;
    if (!desc.isEmpty())
        def->desc = desc;
    return def;
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FwdCoilSet::FwdCoilSet()
{
    coord_frame = FIFFV_COORD_UNKNOWN;
}

//=============================================================================================================

FwdCoilSet::~FwdCoilSet()
{
}

//=============================================================================================================

FwdCoil::UPtr FwdCoilSet::create_meg_coil(const FiffChInfo& ch, int acc, const FiffCoordTrans& t)
{
    if (ch.kind != FIFFV_MEG_CH && ch.kind != FIFFV_REF_MEG_CH) {
        qWarning() << ch.ch_name << "is not a MEG channel. Cannot create a coil definition.";
        return nullptr;
    }
    /*
     * Simple linear search from the coil definitions
     */
    FwdCoil* def = nullptr;
    for (int k = 0; k < this->ncoil(); k++) {
        if ((this->coils[k]->type == (ch.chpos.coil_type & 0xFFFF)) &&
                this->coils[k]->accuracy == acc) {
            def = this->coils[k].get();
        }
    }
    if (!def) {
        qWarning("Desired coil definition not found (type = %d acc = %d)",ch.chpos.coil_type,acc);
        return nullptr;
    }
    /*
     * Create the result
     */
    auto res = std::make_unique<FwdCoil>(def->np);

    res->chname   = ch.ch_name;
    if (!def->desc.isEmpty())
        res->desc   = def->desc;
    res->coil_class = def->coil_class;
    res->accuracy   = def->accuracy;
    res->base       = def->base;
    res->size       = def->size;
    res->type       = ch.chpos.coil_type;

    res->r0 = ch.chpos.r0;
    res->ex = ch.chpos.ex;
    res->ey = ch.chpos.ey;
    res->ez = ch.chpos.ez;
    /*
     * Apply a coordinate transformation if so desired
     */
    if (!t.isEmpty()) {
        FiffCoordTrans::apply_trans(res->r0.data(),t,FIFFV_MOVE);
        FiffCoordTrans::apply_trans(res->ex.data(),t,FIFFV_NO_MOVE);
        FiffCoordTrans::apply_trans(res->ey.data(),t,FIFFV_NO_MOVE);
        FiffCoordTrans::apply_trans(res->ez.data(),t,FIFFV_NO_MOVE);
        res->coord_frame = t.to;
    }
    else
        res->coord_frame = FIFFV_COORD_DEVICE;

    for (int p = 0; p < res->np; p++) {
        res->w[p] = def->w[p];
        res->rmag.row(p)   = (res->r0 + def->rmag(p, 0)*res->ex + def->rmag(p, 1)*res->ey + def->rmag(p, 2)*res->ez).transpose();
        res->cosmag.row(p) = (def->cosmag(p, 0)*res->ex + def->cosmag(p, 1)*res->ey + def->cosmag(p, 2)*res->ez).transpose();
    }
    return res;
}

//=============================================================================================================

FwdCoilSet::UPtr FwdCoilSet::create_meg_coils(const QList<FIFFLIB::FiffChInfo>& chs,
                                         int nch,
                                         int acc,
                                         const FiffCoordTrans& t)
{
    auto res = std::make_unique<FwdCoilSet>();

    for (int k = 0; k < nch; k++) {
        auto next = this->create_meg_coil(chs.at(k),acc,t);
        if (!next)
            return nullptr;
        res->coils.push_back(std::move(next));
    }
    if (!t.isEmpty())
        res->coord_frame = t.to;
    return res;
}

//=============================================================================================================

FwdCoilSet::UPtr FwdCoilSet::create_eeg_els(const QList<FIFFLIB::FiffChInfo>& chs,
                                       int nch,
                                       const FiffCoordTrans& t)
{
    auto res = std::make_unique<FwdCoilSet>();

    for (int k = 0; k < nch; k++) {
        auto next = FwdCoil::create_eeg_el(chs.at(k),t);
        if (!next)
            return nullptr;
        res->coils.push_back(std::move(next));
    }
    if (!t.isEmpty())
        res->coord_frame = t.to;
    return res;
}

//=============================================================================================================

FwdCoilSet::UPtr FwdCoilSet::read_coil_defs(const QString &name)
{
    QFile file(name);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "FwdCoilSet::read_coil_defs - Cannot open" << name;
        return nullptr;
    }

    // Read file content, stripping comments
    QString content;
    {
        QTextStream fileIn(&file);
        while (!fileIn.atEnd()) {
            QString line = fileIn.readLine();
            int idx = line.indexOf('#');
            if (idx >= 0)
                line.truncate(idx);
            content += line + '\n';
        }
    }
    file.close();

    QTextStream in(&content);
    in.setLocale(QLocale::c());

    auto res = std::make_unique<FwdCoilSet>();
    while (!in.atEnd()) {
        /*
         * Read basic info
         */
        int coil_class;
        in >> coil_class;
        if (in.status() != QTextStream::Ok)
            break;

        int type, acc, np;
        float size, base;

        in >> type >> acc >> np >> size >> base;
        if (in.status() != QTextStream::Ok) {
            qWarning("FwdCoilSet::read_coil_defs - Error reading coil header");
            return nullptr;
        }

        QString desc = readWord(in);
        if (desc.isEmpty()) {
            qWarning("FwdCoilSet::read_coil_defs - Missing coil description");
            return nullptr;
        }

        FwdCoil* def = res->fwd_add_coil_to_set(type,coil_class,acc,np,size,base,desc);
        if (!def)
            return nullptr;

        for (int p = 0; p < def->np; p++) {
            /*
             * Read and verify data for each integration point
             */
            in >> def->w[p]
               >> def->rmag(p, 0) >> def->rmag(p, 1) >> def->rmag(p, 2)
               >> def->cosmag(p, 0) >> def->cosmag(p, 1) >> def->cosmag(p, 2);
            if (in.status() != QTextStream::Ok) {
                qWarning("FwdCoilSet::read_coil_defs - Error reading integration point %d", p);
                return nullptr;
            }

            if (def->pos(p).norm() > BIG) {
                qWarning("Unreasonable integration point: %f %f %f mm (coil type = %d acc = %d)", 1000*def->rmag(p, 0),1000*def->rmag(p, 1),1000*def->rmag(p, 2), def->type,def->accuracy);
                return nullptr;
            }
            float cosmagNorm = def->dir(p).norm();
            if (cosmagNorm <= 0) {
                qWarning("Unreasonable normal: %f %f %f (coil type = %d acc = %d)", def->cosmag(p, 0),def->cosmag(p, 1),def->cosmag(p, 2), def->type,def->accuracy);
                return nullptr;
            }
            def->cosmag.row(p).normalize();
        }
    }

    qInfo("%d coil definitions read",res->ncoil());
    return res;
}

//=============================================================================================================

FwdCoilSet::UPtr FwdCoilSet::dup_coil_set(const FiffCoordTrans& t) const
{
    FwdCoilSet::UPtr res;

    if (!t.isEmpty()) {
        if (this->coord_frame != t.from) {
            qWarning("Coordinate frame of the transformation does not match the coil set in fwd_dup_coil_set");
            return nullptr;
        }
    }
    res = std::make_unique<FwdCoilSet>();
    if (!t.isEmpty())
        res->coord_frame = t.to;
    else
        res->coord_frame = this->coord_frame;

    res->coils.reserve(this->ncoil());

    for (int k = 0; k < this->ncoil(); k++) {
        auto coil = std::make_unique<FwdCoil>(*(this->coils[k]));
        /*
     * Optional coordinate transformation
     */
        if (!t.isEmpty()) {
            FiffCoordTrans::apply_trans(coil->r0.data(),t,FIFFV_MOVE);
            FiffCoordTrans::apply_trans(coil->ex.data(),t,FIFFV_NO_MOVE);
            FiffCoordTrans::apply_trans(coil->ey.data(),t,FIFFV_NO_MOVE);
            FiffCoordTrans::apply_trans(coil->ez.data(),t,FIFFV_NO_MOVE);

            for (int p = 0; p < coil->np; p++) {
                FiffCoordTrans::apply_trans(&coil->rmag(p, 0),t,FIFFV_MOVE);
                FiffCoordTrans::apply_trans(&coil->cosmag(p, 0),t,FIFFV_NO_MOVE);
            }
            coil->coord_frame = t.to;
        }
        res->coils.push_back(std::move(coil));
    }
    return res;
}

//=============================================================================================================

bool FwdCoilSet::is_planar_coil_type(int type) const
{
    if (type == FIFFV_COIL_EEG)
        return false;
    for (int k = 0; k < this->ncoil(); k++)
        if (this->coils[k]->type == type)
            return this->coils[k]->coil_class == FWD_COILC_PLANAR_GRAD;
    return false;
}

//=============================================================================================================

bool FwdCoilSet::is_axial_coil_type(int type) const
{
    if (type == FIFFV_COIL_EEG)
        return false;
    for (int k = 0; k < this->ncoil(); k++)
        if (this->coils[k]->type == type)
            return (this->coils[k]->coil_class == FWD_COILC_MAG ||
                    this->coils[k]->coil_class == FWD_COILC_AXIAL_GRAD ||
                    this->coils[k]->coil_class == FWD_COILC_AXIAL_GRAD2);
    return false;
}

//=============================================================================================================

bool FwdCoilSet::is_magnetometer_coil_type(int type) const
{
    if (type == FIFFV_COIL_EEG)
        return false;
    for (int k = 0; k < this->ncoil(); k++)
        if (this->coils[k]->type == type)
            return this->coils[k]->coil_class == FWD_COILC_MAG;
    return false;
}

//=============================================================================================================

bool FwdCoilSet::is_eeg_electrode_type(int type) const
{
    return type == FIFFV_COIL_EEG;
}

