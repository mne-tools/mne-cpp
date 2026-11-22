//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     fwd_coil_set.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Ruben Doerfel <doerfelruben@aol.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     December 2016
 * @brief    FwdCoilSet implementation — @c coil_def.dat template parser plus the @c create_meg_coils / @c create_eeg_els factories that turn FIFF channel descriptions into integration-ready FwdCoil arrays.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_coil_set.h"

#include "fwd_coil.h"
#include "fwd_bem_solution.h"
#include <mne/base/log.h>
#include <mne/fiff/fiff_ch_info.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <vector>
#include <fstream>
#include <locale>
#include <sstream>
#include <string>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace mne;

namespace
{
constexpr float BIG = 0.5f;
}

/**
 * Read a possibly-quoted word (comments already stripped).
 */
static std::string readWord(std::istream& in)
{
    std::string word;
    in >> std::ws;
    if (in.peek() == '"') {
        in.get();
        std::getline(in, word, '"');
    } else {
        in >> word;
    }
    return word;
}

FwdCoil* FwdCoilSet::fwd_add_coil_to_set(int type, int coil_class, int acc, int np, float size, float base, const std::string& desc)
{
    if (np <= 0) {
        mne::Log::warning("Number of integration points should be positive (type = {} acc = {})", type, acc);
        return nullptr;
    }
    if (!(acc == FWD_COIL_ACCURACY_POINT ||
          acc == FWD_COIL_ACCURACY_NORMAL ||
          acc == FWD_COIL_ACCURACY_ACCURATE)) {
        mne::Log::warning("Illegal accuracy (type = {} acc = {})", type, acc);
        return nullptr;
    }
    if (!(coil_class == FWD_COILC_MAG ||
          coil_class == FWD_COILC_AXIAL_GRAD ||
          coil_class == FWD_COILC_PLANAR_GRAD ||
          coil_class == FWD_COILC_AXIAL_GRAD2)) {
        mne::Log::warning("Illegal coil class (type = {} acc = {} class = {})", type, acc, coil_class);
        return nullptr;
    }

    coils.push_back(std::make_unique<FwdCoil>(np));
    FwdCoil* def = coils.back().get();

    def->type = type;
    def->coil_class = coil_class;
    def->accuracy = acc;
    def->np = np;
    def->size = size;
    def->base = base;
    if (!desc.empty())
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

std::unique_ptr<FwdCoil> FwdCoilSet::create_meg_coil(const FiffChInfo& ch, int acc, const FiffCoordTrans& t)
{
    if (ch.kind != FIFFV_MEG_CH && ch.kind != FIFFV_REF_MEG_CH) {
        mne::Log::warning("{} is not a MEG channel. Cannot create a coil definition.", ch.ch_name);
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
        mne::Log::warning("Desired coil definition not found (type = {} acc = {})", ch.chpos.coil_type, acc);
        return nullptr;
    }
    /*
     * Create the result
     */
    auto res = std::make_unique<FwdCoil>(def->np);

    res->chname = ch.ch_name;
    if (!def->desc.empty())
        res->desc = def->desc;
    res->coil_class = def->coil_class;
    res->accuracy = def->accuracy;
    res->base = def->base;
    res->size = def->size;
    res->type = ch.chpos.coil_type;

    res->r0 = ch.chpos.r0;
    res->ex = ch.chpos.ex;
    res->ey = ch.chpos.ey;
    res->ez = ch.chpos.ez;
    /*
     * Apply a coordinate transformation if so desired
     */
    if (!t.empty()) {
        FiffCoordTrans::apply_trans(res->r0.data(), t, FIFFV_MOVE);
        FiffCoordTrans::apply_trans(res->ex.data(), t, FIFFV_NO_MOVE);
        FiffCoordTrans::apply_trans(res->ey.data(), t, FIFFV_NO_MOVE);
        FiffCoordTrans::apply_trans(res->ez.data(), t, FIFFV_NO_MOVE);
        res->coord_frame = t.to;
    } else
        res->coord_frame = FIFFV_COORD_DEVICE;

    for (int p = 0; p < res->np; p++) {
        res->w[p] = def->w[p];
        res->rmag.row(p) = (res->r0 + def->rmag(p, 0) * res->ex + def->rmag(p, 1) * res->ey + def->rmag(p, 2) * res->ez).transpose();
        res->cosmag.row(p) = (def->cosmag(p, 0) * res->ex + def->cosmag(p, 1) * res->ey + def->cosmag(p, 2) * res->ez).transpose();
    }
    return res;
}

//=============================================================================================================

std::unique_ptr<FwdCoilSet> FwdCoilSet::create_meg_coils(const std::vector<mne::FiffChInfo>& chs,
                                                         int nch,
                                                         int acc,
                                                         const FiffCoordTrans& t)
{
    auto res = std::make_unique<FwdCoilSet>();

    for (int k = 0; k < nch; k++) {
        auto next = this->create_meg_coil(chs.at(k), acc, t);
        if (!next)
            return nullptr;
        res->coils.push_back(std::move(next));
    }
    if (!t.empty())
        res->coord_frame = t.to;
    return res;
}

//=============================================================================================================

std::unique_ptr<FwdCoilSet> FwdCoilSet::create_eeg_els(const std::vector<mne::FiffChInfo>& chs,
                                                       int nch,
                                                       const FiffCoordTrans& t)
{
    auto res = std::make_unique<FwdCoilSet>();

    for (int k = 0; k < nch; k++) {
        auto next = FwdCoil::create_eeg_el(chs.at(k), t);
        if (!next)
            return nullptr;
        res->coils.push_back(std::move(next));
    }
    if (!t.empty())
        res->coord_frame = t.to;
    return res;
}

//=============================================================================================================

std::unique_ptr<FwdCoilSet> FwdCoilSet::read_coil_defs(const std::filesystem::path& name)
{
    std::ifstream file(name);
    if (!file) {
        mne::Log::warning("FwdCoilSet::read_coil_defs - Cannot open {}", mne::utf8_from_path(name));
        return nullptr;
    }

    // Read file content, stripping comments
    std::string content;
    for (std::string line; std::getline(file, line);) {
        content.append(line, 0, line.find('#'));
        content.push_back('\n');
    }
    std::istringstream in(content);
    in.imbue(std::locale::classic());

    auto res = std::make_unique<FwdCoilSet>();
    while (in >> std::ws, !in.eof()) {
        /*
         * Read basic info
         */
        int coil_class;
        if (!(in >> coil_class))
            break;

        int type, acc, np;
        float size, base;

        if (!(in >> type >> acc >> np >> size >> base)) {
            mne::Log::warning("FwdCoilSet::read_coil_defs - Error reading coil header");
            return nullptr;
        }

        const std::string desc = readWord(in);
        if (desc.empty()) {
            mne::Log::warning("FwdCoilSet::read_coil_defs - Missing coil description");
            return nullptr;
        }

        FwdCoil* def = res->fwd_add_coil_to_set(type, coil_class, acc, np, size, base, desc);
        if (!def)
            return nullptr;

        for (int p = 0; p < def->np; p++) {
            /*
             * Read and verify data for each integration point
             */
            if (!(in >> def->w[p] >> def->rmag(p, 0) >> def->rmag(p, 1) >> def->rmag(p, 2) >> def->cosmag(p, 0) >> def->cosmag(p, 1) >> def->cosmag(p, 2))) {
                mne::Log::warning("FwdCoilSet::read_coil_defs - Error reading integration point {}", p);
                return nullptr;
            }

            if (def->pos(p).norm() > BIG) {
                mne::Log::warning("Unreasonable integration point: {:f} {:f} {:f} mm (coil type = {} acc = {})", 1000 * def->rmag(p, 0), 1000 * def->rmag(p, 1), 1000 * def->rmag(p, 2), def->type, def->accuracy);
                return nullptr;
            }
            float cosmagNorm = def->dir(p).norm();
            if (cosmagNorm <= 0) {
                mne::Log::warning("Unreasonable normal: {:f} {:f} {:f} (coil type = {} acc = {})", def->cosmag(p, 0), def->cosmag(p, 1), def->cosmag(p, 2), def->type, def->accuracy);
                return nullptr;
            }
            def->cosmag.row(p).normalize();
        }
    }

    mne::Log::info("{} coil definitions read", res->ncoil());
    return res;
}

//=============================================================================================================

std::unique_ptr<FwdCoilSet> FwdCoilSet::dup_coil_set(const FiffCoordTrans& t) const
{
    std::unique_ptr<FwdCoilSet> res;

    if (!t.empty()) {
        if (this->coord_frame != t.from) {
            mne::Log::warning("Coordinate frame of the transformation does not match the coil set in fwd_dup_coil_set");
            return nullptr;
        }
    }
    res = std::make_unique<FwdCoilSet>();
    if (!t.empty())
        res->coord_frame = t.to;
    else
        res->coord_frame = this->coord_frame;

    res->coils.reserve(this->ncoil());

    for (int k = 0; k < this->ncoil(); k++) {
        auto coil = std::make_unique<FwdCoil>(*(this->coils[k]));
        /*
     * Optional coordinate transformation
     */
        if (!t.empty()) {
            FiffCoordTrans::apply_trans(coil->r0.data(), t, FIFFV_MOVE);
            FiffCoordTrans::apply_trans(coil->ex.data(), t, FIFFV_NO_MOVE);
            FiffCoordTrans::apply_trans(coil->ey.data(), t, FIFFV_NO_MOVE);
            FiffCoordTrans::apply_trans(coil->ez.data(), t, FIFFV_NO_MOVE);

            for (int p = 0; p < coil->np; p++) {
                FiffCoordTrans::apply_trans(&coil->rmag(p, 0), t, FIFFV_MOVE);
                FiffCoordTrans::apply_trans(&coil->cosmag(p, 0), t, FIFFV_NO_MOVE);
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
