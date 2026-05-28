//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_guess_data.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implementation of the initial-guess grid generation and forward-field pre-computation.
 *
 * Implements the constructors that either load a precomputed guess
 * source space from disk or build a regular grid inside the inner-skull
 * BEM surface, the cleanup logic and @c compute_guess_fields, which
 * walks every guess location and stores the corresponding column-
 * normalised forward-field SVD into the per-guess
 * @ref InvDipoleForward. Refactored from @c dipole_fit_setup.c (MNE-C).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_guess_data.h"
#include "inv_dipole_fit_data.h"
#include "inv_dipole_forward.h"
#include <mne/mne_surface.h>
#include <mne/mne_source_space.h>

#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>

#include <memory>
#include <QFile>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace MNELIB;
using namespace FWDLIB;
using namespace INVLIB;

constexpr int FAIL = -1;
constexpr int OK   =  0;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

InvGuessData::InvGuessData()
: nguess(0)
{
}

//=============================================================================================================

//InvGuessData::InvGuessData(const InvGuessData& p_GuessData)
//{
//}

//=============================================================================================================

InvGuessData::InvGuessData(const QString &guessname, const QString &guess_surfname, float mindist, float exclude, float grid, InvDipoleFitData *f)
{
//    InvGuessData*      res = new InvGuessData();
    int            k,p;
    float          guessrad = 0.080;
    std::unique_ptr<MNESourceSpace> guesses;
    dipoleFitFuncs orig;

    if (!guessname.isEmpty()) {
        /*
            * Read the guesses and transform to the appropriate coordinate frame
            */
        std::vector<std::unique_ptr<MNESourceSpace>> sp;
        if (MNESourceSpace::read_source_spaces(guessname,sp) == FAIL)
            return;
        if (static_cast<int>(sp.size()) != 1) {
            qCritical("Incorrect number of source spaces in guess file");
            return;
        }
        qInfo("Read guesses from %s\n",guessname.toUtf8().constData());
        guesses = std::move(sp[0]);
    }
    else {
        MNESurface*    inner_skull = nullptr;
        std::unique_ptr<MNESurface> inner_skull_owned;
        Eigen::Vector3f r0 = f->r0;

        Q_ASSERT(f->mri_head_t);
        FiffCoordTrans::apply_inverse_trans(r0.data(),*f->mri_head_t,true);
        if (f->bem_model) {
            qInfo("Using inner skull surface from the BEM (%s)...\n",f->bemname.toUtf8().constData());
            if ((inner_skull = f->bem_model->fwd_bem_find_surface(FIFFV_BEM_SURF_ID_BRAIN)) == nullptr)
                return;
        }
        else if (!guess_surfname.isEmpty()) {
            qInfo("Reading inner skull surface from %s...\n",guess_surfname.toUtf8().data());
            inner_skull_owned = MNESurface::read_bem_surface(guess_surfname,FIFFV_BEM_SURF_ID_BRAIN,true);
            if (!inner_skull_owned)
                return;
            inner_skull = inner_skull_owned.get();
        }
        guesses.reset(reinterpret_cast<MNESourceSpace*>(FwdBemModel::make_guesses(inner_skull,guessrad,r0,grid,exclude,mindist).release()));
        if (!guesses)
            return;
    }
    {
        std::vector<std::unique_ptr<MNESourceSpace>> guesses_vec;
        guesses_vec.push_back(std::move(guesses));
        if (MNESourceSpace::transform_source_spaces_to(f->coord_frame,*f->mri_head_t,guesses_vec) != OK)
            return;
        guesses = std::move(guesses_vec[0]);
    }
    qInfo("Guess locations are now in %s coordinates.\n",FiffCoordTrans::frame_name(f->coord_frame).toUtf8().constData());

    this->nguess  = guesses->nuse;
    this->rr.resize(guesses->nuse, 3);
    for (k = 0, p = 0; k < guesses->np; k++)
        if (guesses->inuse[k]) {
            this->rr.row(p) = guesses->rr.row(k);
            p++;
        }
    guesses.reset();

    qInfo("Go through all guess source locations...");
    this->guess_fwd.resize(this->nguess);
    /*
        * Compute the guesses using the sphere model for speed
        */
    orig = f->funcs;
    if (f->fit_mag_dipoles)
        f->funcs = f->mag_dipole_funcs.get();
    else
        f->funcs = f->sphere_funcs.get();

    for (k = 0; k < this->nguess; k++) {
        this->guess_fwd[k].reset(InvDipoleFitData::dipole_forward_one(f,Eigen::Vector3f(this->rr.row(k).transpose()),nullptr));
        if (!this->guess_fwd[k])
            return;
#ifdef DEBUG
        sing = this->guess_fwd[k]->sing;
        qInfo("%f %f %f\n",sing[0],sing[1],sing[2]);
#endif
    }
    f->funcs = orig;

    qInfo("[done %d sources]\n",p);

    return;
//    return res;
}

//=============================================================================================================

InvGuessData::InvGuessData(const QString &guessname, const QString &guess_surfname, float mindist, float exclude, float grid, InvDipoleFitData *f, char *guess_save_name)
{
    int             k,p;
    float           guessrad = 0.080f;
    std::unique_ptr<MNESourceSpace> guesses;

    if (!guessname.isEmpty()) {
        /*
         * Read the guesses and transform to the appropriate coordinate frame
         */
        std::vector<std::unique_ptr<MNESourceSpace>> sp;
        if (MNESourceSpace::read_source_spaces(guessname,sp) == FIFF_FAIL)
            return;
        if (static_cast<int>(sp.size()) != 1) {
            qCritical("Incorrect number of source spaces in guess file");
            return;
        }
        qInfo("Read guesses from %s\n",guessname.toUtf8().constData());
        guesses = std::move(sp[0]);
    }
    else {
        MNESurface*     inner_skull = nullptr;
        std::unique_ptr<MNESurface> inner_skull_owned;
        Eigen::Vector3f r0 = f->r0;

        Q_ASSERT(f->mri_head_t);
        FiffCoordTrans::apply_inverse_trans(r0.data(),*f->mri_head_t,true);
        if (f->bem_model) {
            qInfo("Using inner skull surface from the BEM (%s)...\n",f->bemname.toUtf8().constData());
            if ((inner_skull = f->bem_model->fwd_bem_find_surface(FIFFV_BEM_SURF_ID_BRAIN)) == nullptr)
                return;
        }
        else if (!guess_surfname.isEmpty()) {
            qInfo("Reading inner skull surface from %s...\n",guess_surfname.toUtf8().data());
            inner_skull_owned = MNESurface::read_bem_surface(guess_surfname,FIFFV_BEM_SURF_ID_BRAIN,true);
            if (!inner_skull_owned)
                return;
            inner_skull = inner_skull_owned.get();
        }
        guesses.reset(reinterpret_cast<MNESourceSpace*>(FwdBemModel::make_guesses(inner_skull,guessrad,r0,grid,exclude,mindist).release()));
        if (!guesses)
            return;
    }
    /*
       * Save the guesses for future use
       */
    if (guesses->nuse == 0) {
        qCritical("No active guess locations remaining.");
        return;
    }
    if (guess_save_name) {
        qCritical("###################DEBUG writing source spaces not yet implemented.");
        //    if (mne_write_source_spaces(guess_save_name,&guesses,1,false) != OK)
        //      goto bad;
        //    qInfo("Wrote guess locations to %s\n",guess_save_name);
    }
    /*
     * Transform the guess locations to the appropriate coordinate frame
     */
    {
        std::vector<std::unique_ptr<MNESourceSpace>> guesses_vec;
        guesses_vec.push_back(std::move(guesses));
        if (MNESourceSpace::transform_source_spaces_to(f->coord_frame,*f->mri_head_t,guesses_vec) != OK)
            return;
        guesses = std::move(guesses_vec[0]);
    }
    qInfo("Guess locations are now in %s coordinates.\n",FiffCoordTrans::frame_name(f->coord_frame).toUtf8().constData());

    this->nguess  = guesses->nuse;
    this->rr.resize(guesses->nuse, 3);
    for (k = 0, p = 0; k < guesses->np; k++)
        if (guesses->inuse[k]) {
            this->rr.row(p) = guesses->rr.row(k);
            p++;
        }
    guesses.reset();

    this->guess_fwd.resize(this->nguess);
    /*
        * Compute the guesses using the sphere model for speed
        */
    if (!this->compute_guess_fields(f))
        return;

    return;
}

//=============================================================================================================

InvGuessData::~InvGuessData() = default;

//=============================================================================================================

bool InvGuessData::compute_guess_fields(InvDipoleFitData* f)
{
    dipoleFitFuncs orig = nullptr;

    if (!f) {
        qCritical("Data missing in compute_guess_fields");
        return false;
    }
    if (!f->noise) {
        qCritical("Noise covariance missing in compute_guess_fields");
        return false;
    }
    qInfo("Go through all guess source locations...");
    orig = f->funcs;
    if (f->fit_mag_dipoles)
        f->funcs = f->mag_dipole_funcs.get();
    else
        f->funcs = f->sphere_funcs.get();
    for (int k = 0; k < this->nguess; k++) {
        this->guess_fwd[k].reset(InvDipoleFitData::dipole_forward_one(f,Eigen::Vector3f(this->rr.row(k).transpose()),this->guess_fwd[k].release()));
        if (!this->guess_fwd[k]) {
            if (orig)
                f->funcs = orig;
            return false;
        }
#ifdef DEBUG
        sing = this->guess_fwd[k]->sing;
        qInfo("%f %f %f\n",sing[0],sing[1],sing[2]);
#endif
    }
    f->funcs = orig;
    qInfo("[done %d sources]\n",this->nguess);

    return true;
}
