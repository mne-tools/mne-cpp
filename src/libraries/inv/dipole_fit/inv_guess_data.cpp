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
#include <base/log.h>
#include <base/string_utils.h>
#include "inv_dipole_fit_data.h"
#include "inv_dipole_forward.h"
#include <mne/mne_surface.h>
#include <mne/mne_source_space.h>

#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>

#include <cassert>
#include <memory>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace mne;

constexpr int OK = 0;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

InvGuessData::InvGuessData()
: nguess(0)
{
}

//=============================================================================================================

InvGuessData::InvGuessData(const std::filesystem::path& guessname, const std::filesystem::path& guess_surfname, float mindist, float exclude, float grid, InvDipoleFitData* f, float guessrad)
{
    int k, p;
    std::unique_ptr<MNESourceSpace> guesses;

    if (!guessname.empty()) {
        /*
         * Read the guesses and transform to the appropriate coordinate frame
         */
        std::vector<std::unique_ptr<MNESourceSpace>> sp;
        if (MNESourceSpace::read_source_spaces(guessname, sp) == FIFF_FAIL)
            return;
        if (static_cast<int>(sp.size()) != 1) {
            mne::Log::critical("Incorrect number of source spaces in guess file");
            return;
        }
        mne::Log::info("Read guesses from {}\n", mne::utf8FromPath(guessname));
        guesses = std::move(sp[0]);
    } else {
        MNESurface* inner_skull = nullptr;
        std::unique_ptr<MNESurface> inner_skull_owned;
        Eigen::Vector3f r0 = f->r0;

        assert(f->mri_head_t);
        FiffCoordTrans::apply_inverse_trans(r0.data(), *f->mri_head_t, true);
        if (f->bem_model) {
            mne::Log::info("Using inner skull surface from the BEM ({})...\n", f->bemname);
            if ((inner_skull = f->bem_model->fwd_bem_find_surface(FIFFV_BEM_SURF_ID_BRAIN)) == nullptr)
                return;
        } else if (!guess_surfname.empty()) {
            mne::Log::info("Reading inner skull surface from {}...\n", mne::utf8FromPath(guess_surfname));
            inner_skull_owned = MNESurface::read_bem_surface(guess_surfname, FIFFV_BEM_SURF_ID_BRAIN, true);
            if (!inner_skull_owned)
                return;
            inner_skull = inner_skull_owned.get();
        }
        guesses = FwdBemModel::make_guesses(inner_skull, guessrad, r0, grid, exclude, mindist);
        if (!guesses)
            return;
    }
    if (guesses->nuse == 0) {
        mne::Log::critical("No active guess locations remaining.");
        return;
    }
    /*
     * Transform the guess locations to the appropriate coordinate frame
     */
    {
        std::vector<std::unique_ptr<MNESourceSpace>> guesses_vec;
        guesses_vec.push_back(std::move(guesses));
        if (MNESourceSpace::transform_source_spaces_to(f->coord_frame, *f->mri_head_t, guesses_vec) != OK)
            return;
        guesses = std::move(guesses_vec[0]);
    }
    mne::Log::info("Guess locations are now in {} coordinates.\n", std::string(FiffCoordTrans::frame_name(f->coord_frame)));

    this->nguess = guesses->nuse;
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
        mne::Log::critical("Data missing in compute_guess_fields");
        return false;
    }
    if (!f->noise) {
        mne::Log::critical("Noise covariance missing in compute_guess_fields");
        return false;
    }
    mne::Log::info("Go through all guess source locations...");
    orig = f->funcs;
    if (f->fit_mag_dipoles)
        f->funcs = f->mag_dipole_funcs.get();
    else
        f->funcs = f->sphere_funcs.get();
    for (int k = 0; k < this->nguess; k++) {
        this->guess_fwd[k] = InvDipoleFitData::dipole_forward_one(f, Eigen::Vector3f(this->rr.row(k).transpose()), std::move(this->guess_fwd[k]));
        if (!this->guess_fwd[k]) {
            if (orig)
                f->funcs = orig;
            return false;
        }
#ifdef DEBUG
        sing = this->guess_fwd[k]->sing;
        mne::Log::info("{:f} {:f} {:f}\n", sing[0], sing[1], sing[2]);
#endif
    }
    f->funcs = orig;
    mne::Log::info("[done {} sources]\n", this->nguess);

    return true;
}
