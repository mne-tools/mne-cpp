//=============================================================================================================
/**
 * @file     inv_guess_data.cpp
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
 * @brief    Definition of the InvGuessData Class.
 *
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

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace MNELIB;
using namespace FWDLIB;
using namespace INVLIB;

//ToDo remove later on
#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef FAIL
#define FAIL -1
#endif

#ifndef OK
#define OK 0
#endif

#define X_16 0
#define Y_16 1
#define Z_16 2

#define VEC_COPY_16(to,from) {\
    (to)[X_16] = (from)[X_16];\
    (to)[Y_16] = (from)[Y_16];\
    (to)[Z_16] = (from)[Z_16];\
    }

void fromFloatEigenMatrix_16(const Eigen::MatrixXf& from_mat, float **& to_mat, const int m, const int n)
{
    for ( int i = 0; i < m; ++i)
        for ( int j = 0; j < n; ++j)
            to_mat[i][j] = from_mat(i,j);
}

void fromFloatEigenMatrix_16(const Eigen::MatrixXf& from_mat, float **& to_mat)
{
    fromFloatEigenMatrix_16(from_mat, to_mat, from_mat.rows(), from_mat.cols());
}

//int
void fromIntEigenMatrix_16(const Eigen::MatrixXi& from_mat, int **&to_mat, const int m, const int n)
{
    for ( int i = 0; i < m; ++i)
        for ( int j = 0; j < n; ++j)
            to_mat[i][j] = from_mat(i,j);
}

void fromIntEigenMatrix_16(const Eigen::MatrixXi& from_mat, int **&to_mat)
{
    fromIntEigenMatrix_16(from_mat, to_mat, from_mat.rows(), from_mat.cols());
}

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
            goto bad;
        if (static_cast<int>(sp.size()) != 1) {
            printf("Incorrect number of source spaces in guess file");
            goto bad;
        }
        printf("Read guesses from %s\n",guessname.toUtf8().constData());
        guesses = std::move(sp[0]);
    }
    else {
        MNESurface*    inner_skull = nullptr;
        int            free_inner_skull = FALSE;
        Eigen::Vector3f r0 = f->r0;

        Q_ASSERT(f->mri_head_t);
        FiffCoordTrans::apply_inverse_trans(r0.data(),*f->mri_head_t,TRUE);
        if (f->bem_model) {
            printf("Using inner skull surface from the BEM (%s)...\n",f->bemname.toUtf8().constData());
            if ((inner_skull = f->bem_model->fwd_bem_find_surface(FIFFV_BEM_SURF_ID_BRAIN)) == nullptr)
                goto bad;
        }
        else if (!guess_surfname.isEmpty()) {
            printf("Reading inner skull surface from %s...\n",guess_surfname.toUtf8().data());
            if ((inner_skull = MNESurface::read_bem_surface(guess_surfname,FIFFV_BEM_SURF_ID_BRAIN,true)) == nullptr)
                goto bad;
            free_inner_skull = TRUE;
        }
        guesses.reset((MNESourceSpace*)FwdBemModel::make_guesses(inner_skull,guessrad,r0,grid,exclude,mindist).release());
        if (!guesses)
            goto bad;
        if (free_inner_skull)
            delete inner_skull;
    }
    {
        std::vector<std::unique_ptr<MNESourceSpace>> guesses_vec;
        guesses_vec.push_back(std::move(guesses));
        if (MNESourceSpace::transform_source_spaces_to(f->coord_frame,*f->mri_head_t,guesses_vec) != OK)
            goto bad;
        guesses = std::move(guesses_vec[0]);
    }
    printf("Guess locations are now in %s coordinates.\n",FiffCoordTrans::frame_name(f->coord_frame).toUtf8().constData());

    this->nguess  = guesses->nuse;
    this->rr.resize(guesses->nuse, 3);
    for (k = 0, p = 0; k < guesses->np; k++)
        if (guesses->inuse[k]) {
            this->rr.row(p) = guesses->rr.row(k);
            p++;
        }
    guesses.reset();

    printf("Go through all guess source locations...");
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
            goto bad;
#ifdef DEBUG
        sing = this->guess_fwd[k]->sing;
        printf("%f %f %f\n",sing[0],sing[1],sing[2]);
#endif
    }
    f->funcs = orig;

    printf("[done %d sources]\n",p);

    return;
//    return res;

bad : {
        return;
//        return nullptr;
    }
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
            goto bad;
        if (static_cast<int>(sp.size()) != 1) {
            qCritical("Incorrect number of source spaces in guess file");
            goto bad;
        }
        printf("Read guesses from %s\n",guessname.toUtf8().constData());
        guesses = std::move(sp[0]);
    }
    else {
        MNESurface*     inner_skull = nullptr;
        int            free_inner_skull = FALSE;
        Eigen::Vector3f r0 = f->r0;

        Q_ASSERT(f->mri_head_t);
        FiffCoordTrans::apply_inverse_trans(r0.data(),*f->mri_head_t,TRUE);
        if (f->bem_model) {
            printf("Using inner skull surface from the BEM (%s)...\n",f->bemname.toUtf8().constData());
            if ((inner_skull = f->bem_model->fwd_bem_find_surface(FIFFV_BEM_SURF_ID_BRAIN)) == nullptr)
                goto bad;
        }
        else if (!guess_surfname.isEmpty()) {
            printf("Reading inner skull surface from %s...\n",guess_surfname.toUtf8().data());
            if ((inner_skull = MNESurface::read_bem_surface(guess_surfname,FIFFV_BEM_SURF_ID_BRAIN,true)) == nullptr)
                goto bad;
            free_inner_skull = TRUE;
        }
        guesses.reset((MNESourceSpace*)FwdBemModel::make_guesses(inner_skull,guessrad,r0,grid,exclude,mindist).release());
        if (!guesses)
            goto bad;
        if (free_inner_skull)
            delete inner_skull;
    }
    /*
       * Save the guesses for future use
       */
    if (guesses->nuse == 0) {
        qCritical("No active guess locations remaining.");
        goto bad;
    }
    if (guess_save_name) {
        printf("###################DEBUG writing source spaces not yet implemented.");
        //    if (mne_write_source_spaces(guess_save_name,&guesses,1,FALSE) != OK)
        //      goto bad;
        //    printf("Wrote guess locations to %s\n",guess_save_name);
    }
    /*
     * Transform the guess locations to the appropriate coordinate frame
     */
    {
        std::vector<std::unique_ptr<MNESourceSpace>> guesses_vec;
        guesses_vec.push_back(std::move(guesses));
        if (MNESourceSpace::transform_source_spaces_to(f->coord_frame,*f->mri_head_t,guesses_vec) != OK)
            goto bad;
        guesses = std::move(guesses_vec[0]);
    }
    printf("Guess locations are now in %s coordinates.\n",FiffCoordTrans::frame_name(f->coord_frame).toUtf8().constData());

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
        goto bad;

    return;

bad : {
        return;
    }
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
    printf("Go through all guess source locations...");
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
        printf("%f %f %f\n",sing[0],sing[1],sing[2]);
#endif
    }
    f->funcs = orig;
    printf("[done %d sources]\n",this->nguess);

    return true;
}
