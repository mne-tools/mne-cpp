//=============================================================================================================
/**
 * @file     inv_dipole_fit_data.cpp
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
 * @brief    Definition of the InvDipoleFitData class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fwd/fwd_types.h>

#include "inv_dipole_fit_data.h"
#include "inv_guess_data.h"
#include "../inv_meas_data.h"
#include "../inv_meas_data_set.h"
#include <mne/mne_proj_item.h>
#include <mne/mne_cov_matrix.h>
#include "inv_ecd.h"

#include <fiff/fiff_stream.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_coord_trans.h>
#include <fwd/fwd_bem_model.h>
#include <mne/mne_surface.h>

#include <fwd/fwd_comp_data.h>

#include <utils/simplex_algorithm.h>
#include <utils/sphere.h>

#include <Eigen/Dense>

#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QDebug>

#include <cmath>

using namespace Eigen;
using namespace FIFFLIB;
using namespace MNELIB;
using namespace FWDLIB;
using namespace INVLIB;

// CTF coil type constants

#ifndef FIFFV_COIL_CTF_GRAD
#define FIFFV_COIL_CTF_GRAD           5001
#endif

#ifndef FIFFV_COIL_CTF_REF_MAG
#define FIFFV_COIL_CTF_REF_MAG        5002
#endif

#ifndef FIFFV_COIL_CTF_REF_GRAD
#define FIFFV_COIL_CTF_REF_GRAD       5003
#endif

#ifndef FIFFV_COIL_CTF_OFFDIAG_REF_GRAD
#define FIFFV_COIL_CTF_OFFDIAG_REF_GRAD 5004
#endif

constexpr int FAIL = -1;
constexpr int OK   = 0;

//=============================================================================================================
// Row-major Eigen matrix types and helpers for C API interop
//=============================================================================================================

using RowMatrixXf = Matrix<float,  Dynamic, Dynamic, RowMajor>;
using RowMatrixXd = Matrix<double, Dynamic, Dynamic, RowMajor>;

//=============================================================================================================
// Vector / matrix helpers (Eigen-based replacements for MNE-C routines)
//=============================================================================================================

//=============================================================================================================

constexpr float EPS_3 = 0.05f;

/**
 * @brief Pick signal values at a specified time point using linear interpolation.
 *
 * @param[in]  time     Target time point (seconds).
 * @param[in]  integ    Integration window width (seconds).
 * @param[in]  data     Data matrix (nch x nsamp, stored time-by-time).
 * @param[in]  nsamp    Number of time samples.
 * @param[in]  nch      Number of channels.
 * @param[in]  tmin     Time of first sample (seconds).
 * @param[in]  sfreq    Sampling frequency (Hz).
 * @param[in]  use_abs  If non-zero, take absolute values before averaging.
 * @param[out] value    Output array of picked values (nch elements).
 * @return OK on success, FAIL on error.
 */
int mne_get_values_from_data_3 (float time,
                              float integ,
                              float **data,
                              int   nsamp,
                              int   nch,
                              float tmin,
                              float sfreq,
                              int   use_abs,
                              float *value)
{
    int   n1,n2,k;
    float s1,s2;
    float f1,f2;
    float sum;
    float width;
    int   ch;

    for (ch = 0; ch < nch; ch++) {
        /*
     * Find out the correct samples
     */
        if (std::fabs(sfreq*integ) < EPS_3) { /* This is the single-sample case */
            s1 = sfreq*(time - tmin);
            n1 = floor(s1);
            f1 = 1.0 + n1 - s1;
            if (n1 < 0 || n1 > nsamp-1) {
                qWarning("Sample value out of range %d (0..%d)",n1,nsamp-1);
                return(-1);
            }
            /*
       * Avoid rounding error
       */
            if (n1 == nsamp-1) {
                if (std::fabs(f1 - 1.0) < 1e-3)
                    f1 = 1.0;
            }
            if (f1 < 1.0 && n1 > nsamp-2) {
                qWarning("Sample value out of range %d (0..%d) %.4f",n1,nsamp-1,f1);
                return(-1);
            }
            if (f1 < 1.0) {
                if (use_abs)
                    sum = f1*std::fabs(data[n1][ch]) + (1.0-f1)*std::fabs(data[n1+1][ch]);
                else
                    sum = f1*data[n1][ch] + (1.0-f1)*data[n1+1][ch];
            }
            else {
                if (use_abs)
                    sum = std::fabs(data[n1][ch]);
                else
                    sum = data[n1][ch];
            }
        }
        else {			/* Multiple samples */
            s1 = sfreq*(time - 0.5*integ - tmin);
            s2 = sfreq*(time + 0.5*integ - tmin);
            n1 = ceil(s1); n2 = floor(s2);
            if (n2 < n1) {		/* We are within one sample interval */
                n1 = floor(s1);
                if (n1 < 0 || n1 > nsamp-2)
                    return (-1);
                f1 = s1 - n1;
                f2 = s2 - n1;
                if (use_abs)
                    sum = 0.5*((f1+f2)*std::fabs(data[n1+1][ch]) + (2.0-f1-f2)*std::fabs(data[n1][ch]));
                else
                    sum = 0.5*((f1+f2)*data[n1+1][ch] + (2.0-f1-f2)*data[n1][ch]);
            }
            else {
                f1 = n1 - s1;
                f2 = s2 - n2;
                if (n1 < 0 || n1 > nsamp-1) {
                    qWarning("Sample value out of range %d (0..%d)",n1,nsamp-1);
                    return(-1);
                }
                if (n2 < 0 || n2 > nsamp-1) {
                    qWarning("Sample value out of range %d (0..%d)",n2,nsamp-1);
                    return(-1);
                }
                if (f1 != 0.0 && n1 < 1)
                    return(-1);
                if (f2 != 0.0 && n2 > nsamp-2)
                    return(-1);
                sum = 0.0;
                width = 0.0;
                if (n2 > n1) {		/* Do the whole intervals */
                    if (use_abs) {
                        sum = 0.5 * std::fabs(data[n1][ch]);
                        for (k = n1+1; k < n2; k++)
                            sum = sum + std::fabs(data[k][ch]);
                        sum = sum + 0.5 * std::fabs(data[n2][ch]);
                    }
                    else {
                        sum = 0.5*data[n1][ch];
                        for (k = n1+1; k < n2; k++)
                            sum = sum + data[k][ch];
                        sum = sum + 0.5*data[n2][ch];
                    }
                    width = n2 - n1;
                }
                /*
         * Add tails
         */
                if (use_abs) {
                    if (f1 != 0.0)
                        sum = sum + 0.5*f1*(f1*std::fabs(data[n1-1][ch]) + (2.0-f1)*std::fabs(data[n1][ch]));
                    if (f2 != 0.0)
                        sum = sum + 0.5*f2*(f2*std::fabs(data[n2+1][ch]) + (2.0-f2)*std::fabs(data[n2][ch]));
                }
                else {
                    if (f1 != 0.0)
                        sum = sum + 0.5*f1*(f1*data[n1-1][ch] + (2.0-f1)*data[n1][ch]);
                    if (f2 != 0.0)
                        sum = sum + 0.5*f2*(f2*data[n2+1][ch] + (2.0-f2)*data[n2][ch]);
                }
                width = width + f1 + f2;
                sum = sum/width;
            }
        }
        value[ch] = sum;
    }
    return (0);
}

//=============================================================================================================
// Dipole fit setup

static void free_dipole_fit_funcs(dipoleFitFuncs f)

{
    if (!f)
        return;

    if (f->meg_client_free && f->meg_client)
        f->meg_client_free(f->meg_client);
    if (f->eeg_client_free && f->eeg_client)
        f->eeg_client_free(f->eeg_client);

    delete f;
    return;
}

static dipoleFitFuncs new_dipole_fit_funcs()

{
    dipoleFitFuncs f = new dipoleFitFuncsRec{};
    return f;
}

// Data channel selection



/**
 * @brief Check whether a channel is selected in the measurement data.
 */
int is_selected_in_data(mshMegEegData d, const QString& ch_name)
{
    int issel = false;
    int k;

    for (k = 0; k < d->meas->nchan; k++)
        if (QString::compare(ch_name,d->meas->chs[k].ch_name,Qt::CaseInsensitive) == 0) {
            issel = d->sels[k];
            break;
        }
    return issel;
}

// Bad channel list I/O

static int whitespace_3(char *text)

{
    if (text == nullptr || strlen(text) == 0)
        return true;
    if (strspn(text," \t\n\r") == strlen(text))
        return true;
    return false;
}

static char *next_line_3(char *line, int n, FILE *in)
{
    char *res;

    for (res = fgets(line,n,in); res != nullptr; res = fgets(line,n,in))
        if (!whitespace_3(res))
            if (res[0] != '#')
                break;
    return res;
}

constexpr int MAXLINE = 500;

/**
 * @brief Read bad channel names from a plain text file.
 */
int mne_read_bad_channels_3(const QString& name, QStringList& listp, int& nlistp)
{
    FILE *in = nullptr;
    QStringList list;
    char line[MAXLINE+1];
    char *next;

    if (name.isEmpty())
        return OK;

    if ((in = fopen(name.toUtf8().data(),"r")) == nullptr) {
        qCritical() << name;
        return FAIL;
    }
    while ((next = next_line_3(line,MAXLINE,in)) != nullptr) {
        if (strlen(next) > 0) {
            if (next[strlen(next)-1] == '\n')
                next[strlen(next)-1] = '\0';
            list.append(next);
        }
    }
    if (ferror(in)) {
        fclose(in);
        return FAIL;
    }

    fclose(in);
    listp  = list;
    nlistp = list.size();

    return OK;
}

constexpr float TOO_CLOSE = 1e-4f;

static int at_origin (const Eigen::Vector3f& rr)
{
    return (rr.norm() < TOO_CLOSE);
}

/**
 * @brief Check whether a channel has valid EEG electrode position information.
 */
static int is_valid_eeg_ch(const FiffChInfo& ch)
{
    if (ch.kind == FIFFV_EEG_CH) {
        if (at_origin(ch.chpos.r0) ||
                ch.chpos.coil_type == FIFFV_COIL_NONE)
            return false;
        else
            return true;
    }
    return false;
}

static int accept_ch(const FiffChInfo& ch,
                     const QStringList& bads,
                     int        nbad)

{
    int k;
    for (k = 0; k < nbad; k++)
        if (QString::compare(ch.ch_name,bads[k]) == 0)
            return false;
    return true;
}

/**
 * @brief Read MEG and EEG channel info, split by type, excluding bad channels.
 *
 * Uses FiffStream::read_meas_info to read all channel information from the FIFF file.
 */
static int read_meg_eeg_ch_info(const QString& name,
                                int        do_meg,
                                int        do_eeg,
                                const QStringList& bads,
                                int        nbad,
                                QList<FiffChInfo>& chsp,
                                int        &nmegp,
                                int        &neegp)
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    if (!stream->open())
        return FIFF_FAIL;

    FiffInfo info;
    FiffDirNode::SPtr infoNode;
    if (!stream->read_meas_info(stream->dirtree(), info, infoNode)) {
        qCritical("%s : could not read measurement info", name.toUtf8().data());
        stream->close();
        return FIFF_FAIL;
    }
    stream->close();

    QList<FiffChInfo> meg;
    int nmeg = 0;
    QList<FiffChInfo> eeg;
    int neeg = 0;

    for (int k = 0; k < info.chs.size(); k++) {
        if (accept_ch(info.chs[k], bads, nbad)) {
            if (do_meg && info.chs[k].kind == FIFFV_MEG_CH) {
                meg.append(info.chs[k]);
                nmeg++;
            } else if (do_eeg && info.chs[k].kind == FIFFV_EEG_CH && is_valid_eeg_ch(info.chs[k])) {
                eeg.append(info.chs[k]);
                neeg++;
            }
        }
    }

    chsp.clear();
    chsp.reserve(nmeg + neeg);
    chsp.append(meg);
    chsp.append(eeg);

    nmegp = nmeg;
    neegp = neeg;
    return FIFF_OK;
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

InvDipoleFitData::InvDipoleFitData()
: coord_frame(FIFFV_COORD_UNKNOWN)
, nmeg (0)
, neeg (0)
, sphere_funcs (nullptr)
, bem_funcs (nullptr)
, funcs (nullptr)
, mag_dipole_funcs (nullptr)
, fixed_noise (false)
, nave (1)
, column_norm (COLUMN_NORM_NONE)
, fit_mag_dipoles (false)
, user (nullptr)
, r0(Eigen::Vector3f::Zero())
{
}

//=============================================================================================================

InvDipoleFitData::~InvDipoleFitData()
{
    // unique_ptr members (pick, meg_coils, eeg_els, eeg_model, bem_model, noise_orig, noise, proj) auto-cleanup

    free_dipole_fit_funcs(sphere_funcs);
    free_dipole_fit_funcs(bem_funcs);
    free_dipole_fit_funcs(mag_dipole_funcs);
}

//=============================================================================================================

/**
 * @brief Set up forward model including BEM and sphere models, coil definitions, and compensation.
 */
int InvDipoleFitData::setup_forward_model(InvDipoleFitData *d, MNECTFCompDataSet* comp_data, FwdCoilSet *comp_coils)
{
    FwdCompData* comp;
    dipoleFitFuncs f;
    int fit_sphere_to_bem = true;

    if (!d->bemname.isEmpty()) {
        /*
         * Set up the boundary-element model
         */
        QString bemsolname = FwdBemModel::fwd_bem_make_bem_sol_name(d->bemname);
        d->bemname = bemsolname;

        qInfo("\nSetting up the BEM model using %s...",d->bemname.toUtf8().constData());
        qInfo("\nLoading surfaces...");
        d->bem_model = FwdBemModel::fwd_bem_load_three_layer_surfaces(d->bemname);
        if (d->bem_model) {
            qInfo("Three-layer model surfaces loaded.");
        }
        else {
            d->bem_model = FwdBemModel::fwd_bem_load_homog_surface(d->bemname);
            if (!d->bem_model)
                return FAIL;
            qInfo("Homogeneous model surface loaded.");
        }
        if (d->neeg > 0 && d->bem_model->nsurf == 1) {
            qCritical("Cannot use a homogeneous model in EEG calculations.");
            return FAIL;
        }
        qInfo("\nLoading the solution matrix...");
        if (d->bem_model->fwd_bem_load_recompute_solution(d->bemname,FWD_BEM_UNKNOWN,false) == FAIL)
            return FAIL;
        qInfo("Employing the head->MRI coordinate transform with the BEM model.");
        Q_ASSERT(d->mri_head_t);
        if (d->bem_model->fwd_bem_set_head_mri_t(*d->mri_head_t) == FAIL)
            return FAIL;
        qInfo("BEM model %s is now set up",d->bem_model->sol_name.toUtf8().constData());
        /*
         * Find the best-fitting sphere
         */
        if (fit_sphere_to_bem) {
            MNESurface* inner_skull;
            float      simplex_size = 2e-2;
            float      R;
            VectorXf   r0_vec;

            if ((inner_skull = d->bem_model->fwd_bem_find_surface(FIFFV_BEM_SURF_ID_BRAIN)) == nullptr)
                return FAIL;

            if (!UTILSLIB::Sphere::fit_sphere_to_points(inner_skull->rr,simplex_size,r0_vec,R))
                return FAIL;
            d->r0 = r0_vec.head<3>();

            FiffCoordTrans::apply_trans(d->r0.data(),*d->mri_head_t,true);
            qInfo("Fitted sphere model origin : %6.1f %6.1f %6.1f mm rad = %6.1f mm.",
                   1000*d->r0[0],1000*d->r0[1],1000*d->r0[2],1000*R);
        }
        d->bem_funcs = f = new_dipole_fit_funcs();
        if (d->nmeg > 0) {
            /*
           * Use the new compensated field computation
           * It works the same way independent of whether or not the compensation is in effect
           */
            comp = FwdCompData::fwd_make_comp_data(comp_data,d->meg_coils.get(),comp_coils,
                                      FwdBemModel::fwd_bem_field,nullptr,nullptr,d->bem_model.get());
            if (!comp)
                return FAIL;
            qInfo("Compensation setup done.");

            qInfo("MEG solution matrix...");
            if (d->bem_model->fwd_bem_specify_coils(d->meg_coils.get()) == FAIL)
                return FAIL;
            if (d->bem_model->fwd_bem_specify_coils(comp->comp_coils) == FAIL)
                return FAIL;
            qInfo("[done]");

            f->meg_field       = FwdCompData::fwd_comp_field;
            f->meg_vec_field   = nullptr;
            f->meg_client      = comp;
            f->meg_client_free = [](void* d) { delete static_cast<FwdCompData*>(d); };
        }
        if (d->neeg > 0) {
            qInfo("\tEEG solution matrix...");
            if (d->bem_model->fwd_bem_specify_els(d->eeg_els.get()) == FAIL)
                return FAIL;
            qInfo("[done]");
            f->eeg_pot     = FwdBemModel::fwd_bem_pot_els;
            f->eeg_vec_pot = nullptr;
            f->eeg_client  = d->bem_model.get();
        }
    }
    if (d->neeg > 0 && !d->eeg_model) {
        qCritical("EEG sphere model not defined.");
        return FAIL;
    }
    d->sphere_funcs = f = new_dipole_fit_funcs();
    if (d->neeg > 0) {
        d->eeg_model->r0 = d->r0;
        f->eeg_pot     = FwdEegSphereModel::fwd_eeg_spherepot_coil;
        f->eeg_vec_pot = FwdEegSphereModel::fwd_eeg_spherepot_coil_vec;
        f->eeg_client  = d->eeg_model.get();
    }
    if (d->nmeg > 0) {
        /*
         * Use the new compensated field computation
         * It works the same way independent of whether or not the compensation is in effect
         */
        comp = FwdCompData::fwd_make_comp_data(comp_data,d->meg_coils.get(),comp_coils,
                                  FwdBemModel::fwd_sphere_field,
                                  FwdBemModel::fwd_sphere_field_vec,
                                  nullptr,
                                  d->r0.data());
        if (!comp)
            return FAIL;
        f->meg_field       = FwdCompData::fwd_comp_field;
        f->meg_vec_field   = FwdCompData::fwd_comp_field_vec;
        f->meg_client      = comp;
        f->meg_client_free = [](void* d) { delete static_cast<FwdCompData*>(d); };
    }
    qInfo("Sphere model origin : %6.1f %6.1f %6.1f mm.",
           1000*d->r0[0],1000*d->r0[1],1000*d->r0[2]);
    /*
       * Finally add the magnetic dipole fitting functions (for special purposes)
       */
    d->mag_dipole_funcs = f = new_dipole_fit_funcs();
    if (d->nmeg > 0) {
        /*
         * Use the new compensated field computation
         * It works the same way independent of whether or not the compensation is in effect
         */
        comp = FwdCompData::fwd_make_comp_data(comp_data,d->meg_coils.get(),comp_coils,
                                  FwdBemModel::fwd_mag_dipole_field,
                                  FwdBemModel::fwd_mag_dipole_field_vec,
                                  nullptr,
                                  nullptr);
        if (!comp)
            return FAIL;
        f->meg_field       = FwdCompData::fwd_comp_field;
        f->meg_vec_field   = FwdCompData::fwd_comp_field_vec;
        f->meg_client      = comp;
        f->meg_client_free = [](void* d) { delete static_cast<FwdCompData*>(d); };
    }
    f->eeg_pot     = FwdBemModel::fwd_mag_dipole_field;
    f->eeg_vec_pot = FwdBemModel::fwd_mag_dipole_field_vec;
    /*
        * Select the appropriate fitting function
        */
    d->funcs = !d->bemname.isEmpty() ? d->bem_funcs : d->sphere_funcs;

    qWarning("");
    return OK;
}

//=============================================================================================================

/**
 * @brief Specify constant ad-hoc noise standard deviations for MEG and EEG channels.
 */
std::unique_ptr<MNECovMatrix> InvDipoleFitData::ad_hoc_noise(FwdCoilSet *meg, FwdCoilSet *eeg, float grad_std, float mag_std, float eeg_std)
{
    int    nchan;
    Eigen::VectorXd stds;
    QStringList names, ch_names;
    int   k,n;

    qInfo("Using standard noise values "
           "(MEG grad : %6.1f fT/cm MEG mag : %6.1f fT EEG : %6.1f uV)\n",
           1e13*grad_std,1e15*mag_std,1e6*eeg_std);

    nchan = 0;
    if (meg)
        nchan = nchan + meg->ncoil();
    if (eeg)
        nchan = nchan + eeg->ncoil();

    stds.resize(nchan);

    n = 0;
    if (meg) {
        for (k = 0; k < meg->ncoil(); k++, n++) {
            if (meg->coils[k]->is_axial_coil()) {
                stds[n] = mag_std*mag_std;
#ifdef TEST_REF
                if (meg->coils[k]->type == FIFFV_COIL_CTF_REF_MAG ||
                        meg->coils[k]->type == FIFFV_COIL_CTF_REF_GRAD ||
                        meg->coils[k]->type == FIFFV_COIL_CTF_OFFDIAG_REF_GRAD)
                    stds[n] = 1e6*stds[n];
#endif
            }
            else
                stds[n] = grad_std*grad_std;
            ch_names.append(meg->coils[k]->chname);
        }
    }
    if (eeg) {
        for (k = 0; k < eeg->ncoil(); k++, n++) {
            stds[n]     = eeg_std*eeg_std;
            ch_names.append(eeg->coils[k]->chname);
        }
    }
    names = ch_names;
    return MNECovMatrix::create(FIFFV_MNE_NOISE_COV,nchan,names,Eigen::VectorXd(),stds);
}

//=============================================================================================================

/**
 * @brief Load and combine SSP projection operators from files for the selected channels.
 */
int InvDipoleFitData::make_projection(const QList<QString> &projnames,
                                   const QList<FiffChInfo>& chs,
                                   int nch,
                                   MNEProjOp* *res)
{
    MNEProjOp* all  = nullptr;
    MNEProjOp* one  = nullptr;
    int       k,found;
    int       neeg;

    for (k = 0, neeg = 0; k < nch; k++)
        if (chs[k].kind == FIFFV_EEG_CH)
            neeg++;

    if (projnames.size() == 0 && neeg == 0)
        return OK;

    for (k = 0; k < projnames.size(); k++) {
        if ((one = MNEProjOp::read(projnames[k])) == nullptr) {
            delete all;
            return FAIL;
        }
        if (one->nitems == 0) {
            qInfo("No linear projection information in %s.",projnames[k].toUtf8().data());
            if(one)
                delete one;
            one = nullptr;
        }
        else {
            qInfo("Loaded projection from %s:",projnames[k].toUtf8().data());
            { QTextStream errStream(stderr); one->report(errStream,"\t"); }
            all = all ? all->combine(one) : (new MNEProjOp())->combine(one);
            if(one)
                delete one;
            one = nullptr;
        }
    }

    if (neeg > 0) {
        found = false;
        if (all) {
            for (k = 0; k < all->nitems; k++)
                if (all->items[k].kind == FIFFV_MNE_PROJ_ITEM_EEG_AVREF) {
                    found = true;
                    break;
                }
        }
        if (!found) {
            if ((one = MNEProjOp::create_average_eeg_ref(chs,nch)) != nullptr) {
                qInfo("Average EEG reference projection added:");
                { QTextStream errStream(stderr); one->report(errStream,"\t"); }
                all = all ? all->combine(one) : (new MNEProjOp())->combine(one);
                if(one)
                    delete one;
                one = nullptr;
            }
        }
    }
    if (all && all->affect_chs(chs,nch) == 0) {
        qInfo("Projection will not have any effect on selected channels. Projection omitted.");
        if(all)
            delete all;
        all = nullptr;
    }
     *res = all;
    return OK;
}

//=============================================================================================================

int InvDipoleFitData::scale_noise_cov(InvDipoleFitData *f, int nave)
{
    float nave_ratio = static_cast<float>(f->nave) / static_cast<float>(nave);
    int   k;

    if (!f->noise)
        return OK;

    if (f->noise->cov.size() > 0) {
        qInfo("Decomposing the sensor noise covariance matrix...");
        if (f->noise->decompose_eigen() == FAIL)
            return FAIL;

        for (k = 0; k < f->noise->ncov*(f->noise->ncov+1)/2; k++)
            f->noise->cov[k] = nave_ratio*f->noise->cov[k];
        for (k = 0; k < f->noise->ncov; k++) {
            f->noise->lambda[k] = nave_ratio*f->noise->lambda[k];
            if (f->noise->lambda[k] < 0.0)
                f->noise->lambda[k] = 0.0;
        }
        if (f->noise->add_inv() == FAIL)
            return FAIL;
    }
    else {
        for (k = 0; k < f->noise->ncov; k++)
            f->noise->cov_diag[k] = nave_ratio*f->noise->cov_diag[k];
        qInfo("Decomposition not needed for a diagonal noise covariance matrix.");
        if (f->noise->add_inv() == FAIL)
            return FAIL;
    }
    qInfo("Effective nave is now %d",nave);
    f->nave = nave;
    return OK;
}

//=============================================================================================================

int InvDipoleFitData::scale_dipole_fit_noise_cov(InvDipoleFitData *f, int nave)
{
    float nave_ratio = static_cast<float>(f->nave) / static_cast<float>(nave);
    int   k;

    if (!f->noise)
        return OK;
    if (f->fixed_noise)
        return OK;

    if (f->noise->cov.size() > 0) {
        /*
         * Do the decomposition and check that the matrix is positive definite
         */
        qInfo("Decomposing the noise covariance...");
        if (f->noise->cov.size() > 0) {
            if (f->noise->decompose_eigen() == FAIL)
                return FAIL;
            for (k = 0; k < f->noise->ncov; k++) {
                if (f->noise->lambda[k] < 0.0)
                    f->noise->lambda[k] = 0.0;
            }
        }
        for (k = 0; k < f->noise->ncov*(f->noise->ncov+1)/2; k++)
            f->noise->cov[k] = nave_ratio*f->noise->cov[k];
        for (k = 0; k < f->noise->ncov; k++) {
            f->noise->lambda[k] = nave_ratio*f->noise->lambda[k];
            if (f->noise->lambda[k] < 0.0)
                f->noise->lambda[k] = 0.0;
        }
        if (f->noise->add_inv() == FAIL)
            return FAIL;
    }
    else {
        for (k = 0; k < f->noise->ncov; k++)
            f->noise->cov_diag[k] = nave_ratio*f->noise->cov_diag[k];
        qInfo("Decomposition not needed for a diagonal noise covariance matrix.");
        if (f->noise->add_inv() == FAIL)
            return FAIL;
    }
    qInfo("Effective nave is now %d",nave);
    f->nave = nave;
    return OK;
}

//=============================================================================================================

/**
 * @brief Select channels and scale the noise covariance for the dipole fit.
 */
int InvDipoleFitData::select_dipole_fit_noise_cov(InvDipoleFitData *f, mshMegEegData d)
{
    int   nave,j,k;
    float nonsel_w  = 30;
    int   min_nchan = 20;

    if (!f || !f->noise_orig)
        return OK;
    if (!d)
        nave = 1;
    else {
        if (d->nave < 0)
            nave = d->meas->current->nave;
        else
            nave = d->nave;
    }
    /*
       * Channel selection
       */
    if (d) {
        std::vector<float> wVec(f->noise_orig->ncov);
        float  *w    = wVec.data();
        int    nomit_meg,nomit_eeg,nmeg,neeg;

        nmeg = neeg = 0;
        nomit_meg = nomit_eeg = 0;
        for (k = 0; k < f->noise_orig->ncov; k++) {
            if (f->noise_orig->ch_class[k] == MNE_COV_CH_EEG)
                neeg++;
            else
                nmeg++;
            if (is_selected_in_data(d,f->noise_orig->names[k]))
                w[k] = 1.0;
            else {
                w[k] = nonsel_w;
                if (f->noise_orig->ch_class[k] == MNE_COV_CH_EEG)
                    nomit_eeg++;
                else
                    nomit_meg++;
            }
        }
        f->noise.reset();
        if (nmeg > 0 && nmeg-nomit_meg > 0 && nmeg-nomit_meg < min_nchan) {
            qCritical("Too few MEG channels remaining");
            return FAIL;
        }
        if (neeg > 0 && neeg-nomit_eeg > 0 && neeg-nomit_eeg < min_nchan) {
            qCritical("Too few EEG channels remaining");
            return FAIL;
        }
        f->noise = f->noise_orig->dup();
        if (nomit_meg+nomit_eeg > 0) {
            if (f->noise->cov.size() > 0) {
                for (j = 0; j < f->noise->ncov; j++)
                    for (k = 0; k <= j; k++) {
                        f->noise->cov[MNECovMatrix::lt_packed_index(j,k)] *= w[j]*w[k];
                    }
            }
            else {
                for (j = 0; j < f->noise->ncov; j++) {
                    f->noise->cov_diag[j] *= w[j]*w[j];
                }
            }
        }
    }
    else {
        if (f->noise && f->nave == nave)
            return OK;
        f->noise = f->noise_orig->dup();
    }

    return scale_dipole_fit_noise_cov(f,nave);
}

//=============================================================================================================

/**
 * @brief Set up all resources needed for dipole fitting: channels, covariance, projection, and forward model.
 */
InvDipoleFitData *InvDipoleFitData::setup_dipole_fit_data(const QString &mriname,
                                                    const QString &measname,
                                                    const QString& bemname,
                                                    Vector3f *r0,
                                                    FwdEegSphereModel *eeg_model,
                                                    int accurate_coils,
                                                    const QString &badname,
                                                    const QString &noisename,
                                                    float grad_std,
                                                    float mag_std,
                                                    float eeg_std,
                                                    float mag_reg,
                                                    float grad_reg,
                                                    float eeg_reg,
                                                    int diagnoise,
                                                    const QList<QString> &projnames,
                                                    int include_meg,
                                                    int include_eeg)
{
    auto res = std::make_unique<InvDipoleFitData>();
    int             k;
    QStringList     badlist;
    int             nbad      = 0;
    QStringList     file_bads;
    int             file_nbad;
    int             coord_frame = FIFFV_COORD_HEAD;
    std::unique_ptr<MNECovMatrix> cov;
    std::unique_ptr<FwdCoilSet> templates;
    std::unique_ptr<MNECTFCompDataSet> comp_data;
    std::unique_ptr<FwdCoilSet> comp_coils;

    /*
       * Read the coordinate transformations
       */
    if (!mriname.isEmpty()) {
        res->mri_head_t = std::make_unique<FiffCoordTrans>(FiffCoordTrans::readMriTransform(mriname));
        if (res->mri_head_t->isEmpty())
            return nullptr;
    }
    else if (!bemname.isEmpty()) {
        qWarning("Source of MRI / head transform required for the BEM model is missing");
        return nullptr;
    }
    else {
        float move[] = { 0.0, 0.0, 0.0 };
        float rot[3][3] = { { 1.0, 0.0, 0.0 },
                            { 0.0, 1.0, 0.0 },
                            { 0.0, 0.0, 1.0 } };
        Eigen::Matrix3f rotMat;
        rotMat << rot[0][0], rot[0][1], rot[0][2],
                  rot[1][0], rot[1][1], rot[1][2],
                  rot[2][0], rot[2][1], rot[2][2];
        Eigen::Vector3f moveVec = Eigen::Map<Eigen::Vector3f>(move);
        res->mri_head_t = std::make_unique<FiffCoordTrans>(FIFFV_COORD_MRI,FIFFV_COORD_HEAD,rotMat,moveVec);
    }

    res->mri_head_t->print();
    res->meg_head_t = std::make_unique<FiffCoordTrans>(FiffCoordTrans::readMeasTransform(measname));
    if (res->meg_head_t->isEmpty())
        return nullptr;
    res->meg_head_t->print();
    /*
       * Read the bad channel lists
       */
    if (!badname.isEmpty()) {
        if (mne_read_bad_channels_3(badname,badlist,nbad) != OK)
            return nullptr;
        qInfo("%d bad channels read from %s.",nbad,badname.toUtf8().data());
    }
    {
        QFile measFile(measname);
        FiffStream::SPtr measStream(new FiffStream(&measFile));
        if (measStream->open()) {
            file_bads = measStream->read_bad_channels(measStream->dirtree());
            file_nbad = file_bads.size();
            measStream->close();
        }
    }
    if (file_nbad > 0) {
        if (badlist.isEmpty())
            nbad = 0;
        for (k = 0; k < file_nbad; k++) {
            badlist.append(file_bads[k]);
            nbad++;
        }
        file_bads.clear();
        qInfo("%d bad channels read from the data file.",file_nbad);
    }
    qInfo("%d bad channels total.",nbad);
    /*
       * Read the channel information
       */
    if (read_meg_eeg_ch_info(measname,
                             include_meg,
                             include_eeg,
                             badlist,
                             nbad,
                             res->chs,
                             res->nmeg,
                             res->neeg) != OK)
        return nullptr;

    if (res->nmeg > 0)
        qInfo("Will use %3d MEG channels from %s",res->nmeg,measname.toUtf8().data());
    if (res->neeg > 0)
        qInfo("Will use %3d EEG channels from %s",res->neeg,measname.toUtf8().data());
    {
        int nch_total = res->nmeg + res->neeg;
        res->ch_names.clear();
        for (int i = 0; i < nch_total; i++)
            res->ch_names.append(res->chs[i].ch_name);
    }
    /*
       * Make coil definitions
       */
    res->coord_frame = coord_frame;
    if (coord_frame == FIFFV_COORD_HEAD) {
        //#ifdef USE_SHARE_PATH
        //        char *coilfile = mne_compose_mne_name("share/mne","coil_def.dat");
        //#else
        //    const char *path = "setup/mne";
        //    const char *filename = "coil_def.dat";
        //    const char *coilfile = mne_compose_mne_name(path,filename);

        //    QString qPath("/usr/pubsw/packages/mne/stable/share/mne/coil_def.dat");

        QString qPath = QString(QCoreApplication::applicationDirPath() + "/../resources/general/coilDefinitions/coil_def.dat");
        QFile file(qPath);
        if ( !QCoreApplication::startingUp() )
            qPath = QCoreApplication::applicationDirPath() + QString("/../resources/general/coilDefinitions/coil_def.dat");
        else if (!file.exists())
            qPath = "../resources/general/coilDefinitions/coil_def.dat";

        QByteArray coilfileBytes = qPath.toUtf8();
        const char *coilfile = coilfileBytes.constData();
        //#endif

        if (!coilfile)
            return nullptr;
        templates = FwdCoilSet::read_coil_defs(coilfile);
        if (!templates) {
            return nullptr;
        }

        Q_ASSERT(res->meg_head_t);
        res->meg_coils = templates->create_meg_coils(res->chs,
                                                          res->nmeg,
                                                          accurate_coils ? FWD_COIL_ACCURACY_ACCURATE : FWD_COIL_ACCURACY_NORMAL,
                                                          *res->meg_head_t);
        if (!res->meg_coils)
            return nullptr;
        res->eeg_els = FwdCoilSet::create_eeg_els(res->chs.mid(res->nmeg),
                                                       res->neeg);
        if (!res->eeg_els)
            return nullptr;
        qInfo("Head coordinate coil definitions created.");
    }
    else {
        qWarning("Cannot handle computations in %s coordinates",FiffCoordTrans::frame_name(coord_frame).toUtf8().constData());
        return nullptr;
    }
    /*
       * Forward model setup
       */
    res->bemname   = bemname;
    if (r0) {
        res->r0 = *r0;
    }
    res->eeg_model.reset(eeg_model);
    /*
       * Compensation data
       */
    comp_data = MNECTFCompDataSet::read(measname);
    if (!comp_data)
        return nullptr;
    if (comp_data->ncomp > 0) {	/* Compensation channel information may be needed */
        QList<FiffChInfo> comp_chs;
        int        ncomp    = 0;

        qInfo("%d compensation data sets in %s",comp_data->ncomp,measname.toUtf8().data());
        {
            QFile compFile(measname);
            FiffStream::SPtr compStream(new FiffStream(&compFile));
            if (!compStream->open())
                return nullptr;
            FiffInfo compInfo;
            FiffDirNode::SPtr compInfoNode;
            if (!compStream->read_meas_info(compStream->dirtree(), compInfo, compInfoNode)) {
                compStream->close();
                return nullptr;
            }
            compStream->close();
            for (int k = 0; k < compInfo.chs.size(); k++) {
                if (compInfo.chs[k].kind == FIFFV_REF_MEG_CH) {
                    comp_chs.append(compInfo.chs[k]);
                    ncomp++;
                }
            }
        }
        if (ncomp > 0) {
            comp_coils = templates->create_meg_coils(comp_chs,
                                                          ncomp,
                                                          FWD_COIL_ACCURACY_NORMAL,
                                                          *res->meg_head_t);
            if (!comp_coils) {
                return nullptr;
            }
            qInfo("%d compensation channels in %s",comp_coils->ncoil(),measname.toUtf8().data());
        }
    }
    else {          /* Get rid of the empty data set */
        comp_data.reset();
    }
    /*
       * Ready to set up the forward model
       */
    if (setup_forward_model(res.get(),comp_data.get(),comp_coils.get()) == FAIL)
        return nullptr;
    res->column_norm = COLUMN_NORM_LOC;
    /*
       * Projection data should go here
       */
    {
        MNEProjOp* proj_raw = nullptr;
        if (make_projection(projnames,
                            res->chs,
                            res->nmeg+res->neeg,
                            &proj_raw) == FAIL)
            return nullptr;
        res->proj.reset(proj_raw);
    }
    if (res->proj && res->proj->nitems > 0) {
        qInfo("Final projection operator is:");
        { QTextStream errStream(stderr); res->proj->report(errStream,"\t"); }

        if (res->proj->assign_channels(res->ch_names,res->nmeg+res->neeg) == FAIL)
            return nullptr;
        if (res->proj->make_proj() == FAIL)
            return nullptr;
    }
    else
        qInfo("No projection will be applied to the data.");

    /*
        * Noise covariance
        */
    if (!noisename.isEmpty()) {
        if ((cov = MNECovMatrix::read(noisename,FIFFV_MNE_SENSOR_COV)) == nullptr)
            return nullptr;
        qInfo("Read a %s noise-covariance matrix from %s",
               cov->cov_diag.size() > 0 ? "diagonal" : "full", noisename.toUtf8().data());
    }
    else {
        if ((cov = ad_hoc_noise(res->meg_coils.get(),res->eeg_els.get(),grad_std,mag_std,eeg_std)) == nullptr)
            return nullptr;
    }
    res->noise = cov->pick_chs_omit(res->ch_names,
                                    res->nmeg+res->neeg,
                                    true,
                                    res->chs);
    if (!res->noise) {
        return nullptr;
    }

    qInfo("Picked appropriate channels from the noise-covariance matrix.");
    cov.reset();

    /*
       * Apply the projection operator to the noise-covariance matrix
       */
    if (res->proj && res->proj->nitems > 0 && res->proj->nvec > 0) {
        if (res->proj->apply_cov(res->noise.get()) == FAIL)
            return nullptr;
        qInfo("Projection applied to the covariance matrix.");
    }

    /*
       * Force diagonal noise covariance?
       */
    if (diagnoise) {
        res->noise->revert_to_diag();
        qInfo("Using only the main diagonal of the noise-covariance matrix.");
    }

    /*
       * Regularize the possibly deficient noise-covariance matrix
       */
    if (res->noise->cov.size() > 0) {
        Eigen::Vector3f regs;
        int   do_it;

        regs[MNE_COV_CH_MEG_MAG]  = mag_reg;
        regs[MNE_COV_CH_MEG_GRAD] = grad_reg;
        regs[MNE_COV_CH_EEG]      = eeg_reg;
        /*
         * Classify the channels
         */
        if (res->noise->classify_channels(res->chs,
                                          res->nmeg+res->neeg) == FAIL)
            return nullptr;
        /*
         * Do we need to do anything?
         */
        for (k = 0, do_it = 0; k < res->noise->ncov; k++) {
            if (res->noise->ch_class[k] != MNE_COV_CH_UNKNOWN &&
                    regs[res->noise->ch_class[k]] > 0.0)
                do_it++;
        }
        /*
         * Apply regularization if necessary
         */
        if (do_it > 0)
            res->noise->regularize(regs);
        else
            qInfo("No regularization applied to the noise-covariance matrix");
    }

    /*
       * Do the decomposition and check that the matrix is positive definite
       */
    qInfo("Decomposing the noise covariance...");
    if (res->noise->cov.size() > 0) {
        if (res->noise->decompose_eigen() == FAIL)
            return nullptr;
        qInfo("Eigenvalue decomposition done.");
        for (k = 0; k < res->noise->ncov; k++) {
            if (res->noise->lambda[k] < 0.0)
                res->noise->lambda[k] = 0.0;
        }
    }
    else {
        qInfo("Decomposition not needed for a diagonal covariance matrix.");
        if (res->noise->add_inv() == FAIL)
            return nullptr;
    }

    badlist.clear();
    return res.release();
}

//=============================================================================================================

// Dipole forward computation

void print_fields(const Eigen::Vector3f& rd,
                  const Eigen::Vector3f& Q,
                  float       time,
                  float       integ,
                  InvDipoleFitData* fit,
                  InvMeasData* data)

{
    Eigen::VectorXf oneVec(data->nchan);
    int   k;
    int   nch = fit->nmeg + fit->neeg;

    if (mne_get_values_from_data_3(time,integ,data->current->data,data->current->np,data->nchan,data->current->tmin,
                                   1.0/data->current->tstep,false,oneVec.data()) == FAIL) {
        qWarning("Cannot pick time: %7.1f ms",1000*time);
        return;
    }
    for (k = 0; k < data->nchan; k++)
        if (data->chs[k].chpos.coil_type == FIFFV_COIL_CTF_REF_GRAD ||
                data->chs[k].chpos.coil_type == FIFFV_COIL_CTF_OFFDIAG_REF_GRAD) {
            qInfo("%g ",1e15*oneVec[k]);
        }
    qInfo("");

    Eigen::MatrixXf fwd = Eigen::MatrixXf::Zero(nch, 3);
    if (InvDipoleFitData::compute_dipole_field(*fit,rd,false,fwd) == FAIL)
        return;

    for (k = 0; k < data->nchan; k++)
        if (data->chs[k].chpos.coil_type == FIFFV_COIL_CTF_REF_GRAD ||
                data->chs[k].chpos.coil_type == FIFFV_COIL_CTF_OFFDIAG_REF_GRAD) {
            qInfo("%g ",1e15*(Q[0]*fwd(k,0)+Q[1]*fwd(k,1)+Q[2]*fwd(k,2)));
        }
    qInfo("");

    return;
}

//=============================================================================================================

/**
 * @brief Compute the forward solution for one or more dipoles, applying projections and whitening.
 */
InvDipoleForward* dipole_forward(InvDipoleFitData* d,
                              float         **rd,
                              int           ndip,
                              InvDipoleForward* old)
{
    InvDipoleForward* res;
    float         S[3];
    int           k,p;
    /*
   * Allocate data if necessary
   */
    if (old && old->ndip == ndip && old->nch == d->nmeg+d->neeg) {
        res = old;
    }
    else {
        delete old; old = nullptr;
        res = new InvDipoleForward;
        int nch = d->nmeg + d->neeg;
        int m   = 3 * ndip;
        res->fwd.resize(m, nch);
        res->uu.resize(m, nch);
        res->vv.resize(m, m);
        res->sing.resize(m);
        res->scales.resize(m);
        res->rd.resize(ndip, 3);
        res->nch  = nch;
        res->ndip = ndip;
    }

    for (k = 0; k < ndip; k++) {
        res->rd.row(k) = Eigen::Map<const Eigen::Vector3f>(rd[k]);
        /*
     * Calculate the field of three orthogonal dipoles
     */
        Eigen::MatrixXf this_fwd(d->nmeg + d->neeg, 3);
        Eigen::Map<const Eigen::Vector3f> rd_k(rd[k]);
        if ((InvDipoleFitData::compute_dipole_field(*d,rd_k,true,this_fwd)) == FAIL) {
            if (!old)
                delete res;
            return nullptr;
        }
        for (int p = 0; p < 3; p++)
            res->fwd.row(3*k+p) = this_fwd.col(p).transpose();
        /*
     * Choice of column normalization
     * (componentwise normalization is not recommended)
     */
        if (d->column_norm == COLUMN_NORM_LOC || d->column_norm == COLUMN_NORM_COMP) {
            for (p = 0; p < 3; p++)
                S[p] = res->fwd.row(3*k+p).squaredNorm();
            if (d->column_norm == COLUMN_NORM_COMP) {
                for (p = 0; p < 3; p++)
                    res->scales[3*k+p] = sqrt(S[p]);
            }
            else {
                /*
     * Divide by three or not?
     */
                res->scales[3*k+0] = res->scales[3*k+1] = res->scales[3*k+2] = sqrt(S[0]+S[1]+S[2])/3.0;
            }
            for (p = 0; p < 3; p++) {
                if (res->scales[3*k+p] > 0.0) {
                    res->scales[3*k+p] = 1.0/res->scales[3*k+p];
                    res->fwd.row(3*k+p) *= res->scales[3*k+p];
                }
                else
                    res->scales[3*k+p] = 1.0;
            }
        }
        else {
            res->scales[3*k]   = 1.0;
            res->scales[3*k+1] = 1.0;
            res->scales[3*k+2] = 1.0;
        }
    }

    /*
   * SVD: A = U · Σ · V^T  where A is m×n (3*ndip × nch)
   *   uu stores right singular vectors (V^T rows, length nch) for data-space projections
   *   vv stores left  singular vectors (U^T rows, length m)   for dipole-moment reconstruction
   */
    {
        int m = 3*ndip;
        int n = d->nmeg+d->neeg;
        int udim = std::min(m,n);
        JacobiSVD<MatrixXf> svd(res->fwd, ComputeFullU | ComputeFullV);
        res->sing = svd.singularValues();
        res->uu = svd.matrixV().transpose().topRows(udim);
        res->vv = svd.matrixU().transpose().topRows(udim);
    }

    return res;
}

//=============================================================================================================

/**
 * @brief Convenience function to compute the forward field of a single dipole.
 */
InvDipoleForward* InvDipoleFitData::dipole_forward_one(InvDipoleFitData* d,
                                                 const Eigen::Vector3f& rd,
                                                 InvDipoleForward* old)
{
    float *rds[1];
    rds[0] = const_cast<float*>(rd.data());
    return dipole_forward(d,rds,1,old);
}

//=============================================================================================================
// Dipole fitting - evaluation
/**
 * @brief Calculate the residual sum of squares for dipole fit evaluation.
 */
static float fit_eval(const VectorXf& rd, const void *user)
{
    InvDipoleFitData* fit   = const_cast<InvDipoleFitData*>(static_cast<const InvDipoleFitData*>(user));
    InvDipoleForward* fwd;
    FitDipUserRec*   fuser = fit->user;
    double        Bm2,one;
    int           ncomp,c;

    fwd = fuser->fwd = InvDipoleFitData::dipole_forward_one(fit,rd.head<3>(),fuser->fwd);
    ncomp = fwd->sing[2]/fwd->sing[0] > fuser->limit ? 3 : 2;
    if (fuser->report_dim)
        qInfo("ncomp = %d",ncomp);

    Eigen::Map<const VectorXf> Bmap(fuser->B, fwd->nch);
    for (c = 0, Bm2 = 0.0; c < ncomp; c++) {
        one = fwd->uu.row(c).dot(Bmap);
        Bm2 = Bm2 + one*one;
    }
    return fuser->B2-Bm2;
}

/**
 * @brief Find the best initial guess using precomputed SVD of the forward solutions.
 */
static int find_best_guess(const Eigen::Ref<const Eigen::VectorXf>& B,
                           int       nch,
                           InvGuessData* guess,
                           float     limit,
                           int       &bestp,
                           float     &goodp)
{
    int    k,c;
    double B2,Bm2,this_good,one;
    int    best = -1;
    float  good = 0.0;
    InvDipoleForward* fwd;
    int    ncomp;

    B2 = B.squaredNorm();
    for (k = 0; k < guess->nguess; k++) {
        fwd = guess->guess_fwd[k].get();
        if (fwd->nch == nch) {
            ncomp = fwd->sing[2]/fwd->sing[0] > limit ? 3 : 2;
            for (c = 0, Bm2 = 0.0; c < ncomp; c++) {
                one = fwd->uu.row(c).dot(B);
                Bm2 = Bm2 + one*one;
            }
            this_good = 1.0 - (B2 - Bm2)/B2;
            if (this_good > good) {
                best = k;
                good = this_good;
            }
        }
    }
    if (best < 0) {
        qWarning("No reasonable initial guess found.");
        return FAIL;
    }
    bestp = best;
    goodp = good;
    return OK;
}

/**
 * @brief Create the initial tetrahedron simplex for dipole position search.
 */
static MatrixXf make_initial_dipole_simplex(const Eigen::Vector3f& r0,
                                           float  size)
{
    /*
   * For this definition of a regular tetrahedron, see
   *
   * http://mathworld.wolfram.com/Tetrahedron.html
   *
   */
    float x = sqrt(3.0f)/3.0f;
    float r = sqrt(6.0f)/12.0f;
    float R = 3*r;
    float d = x/2.0f;
    float rr[][3] = { { x , 0.0f,  -r },
                      { -d, 0.5f,  -r },
                      { -d, -0.5f, -r },
                      { 0.0f, 0.0f, R } };

    MatrixXf simplex = MatrixXf::Zero(4, 3);

    for (int j = 0; j < 4; j++) {
        simplex.row(j) = Eigen::Map<const Vector3f>(rr[j]).transpose() * size + r0.transpose();
    }
    return simplex;
}

static bool dipole_report_func(int     loop,
                               const VectorXf& fitpar,
                               double  fval_lo,
                               double  fval_hi,
                               double  par_diff)
{
    qInfo("loop %d rd %7.2f %7.2f %7.2f fval %g %g par diff %g",
            loop,1000*fitpar[0],1000*fitpar[1],1000*fitpar[2],fval_lo,fval_hi,1000*par_diff);

    return true;
}

/**
 * @brief Fit the dipole moment once the location is known.
 */
static int fit_Q(InvDipoleFitData* fit,
                 const Eigen::Ref<const Eigen::VectorXf>& B,
                 const Eigen::Vector3f& rd,
                 float limit,
                 Eigen::Vector3f& Q,
                 int   &ncomp,
                 float &res)
{
    int c;
    InvDipoleForward* fwd = InvDipoleFitData::dipole_forward_one(fit,rd,nullptr);
    float Bm2,one;

    if (!fwd)
        return FAIL;

    ncomp = fwd->sing[2]/fwd->sing[0] > limit ? 3 : 2;

    Q.setZero();
    for (c = 0, Bm2 = 0.0; c < ncomp; c++) {
        one = fwd->uu.row(c).dot(B);
        Q += (one/fwd->sing[c]) * fwd->vv.row(c).head(3).transpose();
        Bm2 = Bm2 + one*one;
    }
    /*
   * Counteract the effect of column normalization
   */
    for (c = 0; c < 3; c++)
        Q[c] = fwd->scales[c]*Q[c];
    res = B.squaredNorm() - Bm2;

    delete fwd;

    return OK;
}

//=============================================================================================================
// Dipole fitting - main entry point
/**
 * @brief Fit a single dipole to the measured field at a given time point using simplex optimization.
 *
 * @param[in] fit       Precomputed fitting data (forward model, noise, projection).
 * @param[in] guess     Initial guess positions for the dipole search.
 * @param[in] time      Time point being fitted (seconds).
 * @param[in,out] B     Measured field vector (whitened in-place).
 * @param[in] verbose   If non-zero, print intermediate results.
 * @param[out] res      The fitted dipole result.
 * @return true if fit succeeded, false on failure.
 */
bool InvDipoleFitData::fit_one(InvDipoleFitData* fit,
                    InvGuessData*     guess,
                    float         time,
                    Eigen::Ref<Eigen::VectorXf> B,
                    int           verbose,
                    InvEcd&          res
                    )
{
    VectorXf   vals(4);                        /* Values at the vertices */
    float  limit           = 0.2f;	               /* (pseudo) radial component omission limit */
    float  size            = 1e-2f;	       /* Size of the initial simplex */
    float  ftol[]          = { 1e-2f, 1e-2f };   /* Tolerances on the the two passes */
    float  atol[]          = { 0.2e-3f, 0.2e-3f }; /* If dipole movement between two iterations is less than this,
                                                  we consider to have converged */
    int    ntol            = 2;
    int    max_eval        = 1000;	       /* Limit for fit function evaluations */
    int    report_interval = verbose ? 1 : -1;   /* How often to report the intermediate result */

    int        best;
    float      good,final_val;
    Eigen::Vector3f rd_final, Q;
    FitDipUserRec user;
    int        k,neval,neval_tot,nchan,ncomp;
    int        fit_fail;
    Vector3f   rd_guess;

    nchan = fit->nmeg+fit->neeg;
    user.fwd = nullptr;

    if (fit->proj && fit->proj->project_vector(B.data(),nchan,true) == FAIL)
        return false;

    if (fit->noise->whiten_vector(B,B,nchan) == FAIL)
        return false;
    /*
   * Get the initial guess
   */
    if (find_best_guess(B,nchan,guess,limit,best,good) < 0)
        return false;

    user.limit = limit;
    user.B     = B.data();
    user.B2    = B.squaredNorm();
    user.fwd   = nullptr;
    user.report_dim = false;
    fit->user  = &user;

    rd_guess = guess->rr.row(best).transpose();
    rd_final = rd_guess;

    neval_tot = 0;
    fit_fail = false;
    for (k = 0; k < ntol; k++) {
        /*
     * Do first pass with the sphere model
     */
        if (k == 0)
            fit->funcs = fit->sphere_funcs;
        else
            fit->funcs = !fit->bemname.isEmpty() ? fit->bem_funcs : fit->sphere_funcs;

        MatrixXf simplexMat = make_initial_dipole_simplex(rd_guess,size);
        for (int p = 0; p < 4; p++)
            vals[p] = fit_eval(simplexMat.row(p),fit);
        if (!UTILSLIB::SimplexAlgorithm::simplex_minimize<float>(
                             simplexMat,        /* The initial simplex */
                             vals,              /* Function values at the vertices */
                             ftol[k],           /* Relative convergence tolerance for the target function */
                             atol[k],           /* Absolute tolerance for the change in the parameters */
                             fit_eval,          /* The function to be evaluated */
                             fit,               /* Data to be passed to the above function in each evaluation */
                             max_eval,          /* Maximum number of function evaluations */
                             neval,             /* Number of function evaluations */
                             report_interval,   /* How often to report (-1 = no_reporting) */
                             dipole_report_func)) {
            if (k == 0) {
                delete user.fwd;
                return false;
            }
            else {
                float rv = 2.0f*(vals.maxCoeff()-vals.minCoeff())/(vals.maxCoeff()+vals.minCoeff());
                qWarning("Warning (t = %8.1f ms) : g = %6.1f %% final val = %7.3f rtol = %f",
                       1000*time,100*(1 - vals[0]/user.B2),vals[0],rv);
                fit_fail = true;
            }
        }
        rd_final = simplexMat.row(0).transpose();
        rd_guess = simplexMat.row(0).transpose();

        neval_tot += neval;
        final_val  = vals[0];
    }
    /*
   * Confidence limits should be computed here
   */
    /*
   * Compute the dipole moment at the final point
   */
    if (fit_Q(fit,B,rd_final,user.limit,Q,ncomp,final_val) == OK) {
        res.time  = time;
        res.valid = true;
        res.rd    = rd_final;
        res.Q     = Q;
        res.good  = 1.0 - final_val/user.B2;
        if (fit_fail)
            res.good = -res.good;
        res.khi2  = final_val;
        if (fit->proj)
            res.nfree = nchan-3-ncomp-fit->proj->nvec;
        else
            res.nfree = nchan-3-ncomp;
        res.neval = neval_tot;
    }
    else {
        delete user.fwd;
        return false;
    }
    delete user.fwd;

    return true;
}

//=============================================================================================================

/**
 * @brief Compute the forward field for a dipole at position rd, applying projection and whitening.
 *
 * The output matrix fwd is nch x 3, with columns corresponding to X, Y, Z orientations.
 */
int InvDipoleFitData::compute_dipole_field(InvDipoleFitData& d, const Eigen::Vector3f& rd, int whiten, Eigen::Ref<Eigen::MatrixXf> fwd)
{
    static const Eigen::Vector3f Qx(1.0f, 0.0f, 0.0f);
    static const Eigen::Vector3f Qy(0.0f, 1.0f, 0.0f);
    static const Eigen::Vector3f Qz(0.0f, 0.0f, 1.0f);
    int nch = d.nmeg + d.neeg;
    int k;
    /*
   * Compute the fields
   */
    if (d.nmeg > 0) {
        int nmeg = d.meg_coils->ncoil();
        if (d.funcs->meg_vec_field) {
            /*
             * Use the vector field function: computes all three dipole
             * orientations at once. Output is 3 x ncoil, we need nch x 3.
             */
            Eigen::MatrixXf vec_meg(3, nmeg);
            if (d.funcs->meg_vec_field(rd,*d.meg_coils,vec_meg,d.funcs->meg_client) != OK)
                return FAIL;
            fwd.topRows(nmeg) = vec_meg.transpose();
        } else {
            auto fwd0 = fwd.col(0).head(nmeg);
            auto fwd1 = fwd.col(1).head(nmeg);
            auto fwd2 = fwd.col(2).head(nmeg);
            if (d.funcs->meg_field(rd,Qx,*d.meg_coils,fwd0,d.funcs->meg_client) != OK)
                return FAIL;
            if (d.funcs->meg_field(rd,Qy,*d.meg_coils,fwd1,d.funcs->meg_client) != OK)
                return FAIL;
            if (d.funcs->meg_field(rd,Qz,*d.meg_coils,fwd2,d.funcs->meg_client) != OK)
                return FAIL;
        }
    }

    if (d.neeg > 0) {
        int neeg = d.eeg_els->ncoil();
        if (d.funcs->eeg_vec_pot) {
            /*
             * Use the vector potential function: computes all three dipole
             * orientations at once. Output is 3 x ncoil, we need nch x 3.
             */
            Eigen::MatrixXf vec_eeg(3, neeg);
            if (d.funcs->eeg_vec_pot(rd,*d.eeg_els,vec_eeg,d.funcs->eeg_client) != OK)
                return FAIL;
            fwd.block(d.nmeg, 0, neeg, 3) = vec_eeg.transpose();
        } else {
            auto fwd0 = fwd.col(0).segment(d.nmeg, neeg);
            auto fwd1 = fwd.col(1).segment(d.nmeg, neeg);
            auto fwd2 = fwd.col(2).segment(d.nmeg, neeg);
            if (d.funcs->eeg_pot(rd,Qx,*d.eeg_els,fwd0,d.funcs->eeg_client) != OK)
                return FAIL;
            if (d.funcs->eeg_pot(rd,Qy,*d.eeg_els,fwd1,d.funcs->eeg_client) != OK)
                return FAIL;
            if (d.funcs->eeg_pot(rd,Qz,*d.eeg_els,fwd2,d.funcs->eeg_client) != OK)
                return FAIL;
        }
    }

    /*
   * Apply projection
   */
    for (k = 0; k < 3; k++)
        if (d.proj && d.proj->project_vector(fwd.col(k).data(),nch,true) == FAIL)
            return FAIL;

    /*
   * Whiten
   */
    if (d.noise && whiten) {
        for (k = 0; k < 3; k++) {
            auto col_k = fwd.col(k);
            if (d.noise->whiten_vector(col_k,col_k,nch) == FAIL)
                return FAIL;
        }
    }

    return OK;
}
