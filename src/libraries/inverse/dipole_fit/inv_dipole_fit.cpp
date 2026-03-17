
//=============================================================================================================
/**
 * @file     inv_dipole_fit.cpp
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
 * @brief    Definition of the InvDipoleFit class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_dipole_fit.h"
#include "../inv_meas_data_set.h"
#include "inv_guess_data.h"

#include <memory>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace MNELIB;
using namespace FWDLIB;

//=============================================================================================================
// CONSTANTS
//=============================================================================================================

static constexpr float SEG_LEN    = 10.0f;
static constexpr float EPS_VALUES = 0.05f;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

static mneChSelection mne_ch_selection_these(const QString& selname, const QStringList& names, int nch)
{
    auto sel  = new MNEChSelection();
    sel->name = selname;
    sel->ndef = nch;
    sel->kind = MNE_CH_SELECTION_USER;

    for (int c = 0; c < nch; c++)
        sel->chdef.append(names[c]);

    return sel;
}

static int mne_ch_selection_assign_chs(mneChSelection sel,
                                       MNERawData*     data)
{
    if (!sel || !data)
        return 0;

    auto info = data->info.get();
    sel->chspick         = sel->chdef;
    sel->chspick_nospace = sel->chdef;
    for (auto& name : sel->chspick_nospace)
        name = name.trimmed();
    sel->nchan = sel->ndef;

    sel->pick.setConstant(sel->nchan, -1);
    sel->pick_deriv.setConstant(sel->nchan, -1);
    sel->ch_kind.setConstant(sel->nchan, -1);

    for (int c = 0; c < sel->nchan; c++) {
        for (int rc = 0; rc < info->nchan; rc++) {
            if (QString::compare(sel->chspick[c],info->chInfo[rc].ch_name,Qt::CaseInsensitive) == 0 ||
                    QString::compare(sel->chspick_nospace[c],info->chInfo[rc].ch_name,Qt::CaseInsensitive) == 0) {
                sel->pick[c]    = rc;
                sel->ch_kind[c] = info->chInfo[rc].kind;
                break;
            }
        }
    }
    /*
     * Maybe the derivations will help
     */
    sel->nderiv = 0;
    if (data->deriv_matched) {
        QStringList deriv_names = data->deriv_matched->deriv_data->rowlist;
        int  nderiv        = data->deriv_matched->deriv_data->nrow;

        for (int c = 0; c < sel->nchan; c++) {
            if (sel->pick[c] == -1) {
                for (int d = 0; d < nderiv; d++) {
                    if (QString::compare(sel->chspick[c],deriv_names[d],Qt::CaseInsensitive) == 0 &&
                            data->deriv_matched->valid.size() > 0 && data->deriv_matched->valid[d]) {
                        sel->pick_deriv[c] = d;
                        sel->ch_kind[c]    = data->deriv_matched->chs[d].kind;
                        sel->nderiv++;
                        break;
                    }
                }
            }
        }
    }
    /*
     * Try simple channels again without the part after dashes
     */
    for (int c = 0; c < sel->nchan; c++) {
        if (sel->pick[c] == -1 && sel->pick_deriv[c] == -1) {
            for (int rc = 0; rc < info->nchan; rc++) {
                QString dash = QString(info->chInfo[rc].ch_name).mid(QString(info->chInfo[rc].ch_name).indexOf("-")+1);
                if (!dash.isNull()) {
                    if (QString::compare(sel->chspick[c],info->chInfo[rc].ch_name,Qt::CaseInsensitive) == 0 ||
                            QString::compare(sel->chspick_nospace[c],info->chInfo[rc].ch_name,Qt::CaseInsensitive) == 0) {
                        sel->pick[c] = rc;
                        sel->ch_kind[c] = info->chInfo[rc].kind;
                        break;
                    }
                }
            }
        }
    }
    int nch = 0;
    for (int c = 0; c < sel->nchan; c++) {
        if (sel->pick[c] >= 0)
            nch++;
    }
    if (sel->nderiv > 0)
        qInfo("Selection \"%s\" has %d matched derived channels.",sel->name.toUtf8().constData(),sel->nderiv);
    return nch;
}

//============================= mne_get_values.c =============================

int mne_get_values_from_data (float time,         /* Interesting time point */
                              float integ,	  /* Time integration */
                              float **data,	  /* The data values (time by time) */
                              int   nsamp,	  /* How many time points? */
                              int   nch,          /* How many channels */
                              float tmin,	  /* Time of first sample */
                              float sfreq,	  /* Sampling frequency */
                              int   use_abs,      /* Use absolute values */
                              float *value)	  /* The picked values */
/*
      * Pick a signal value using linear interpolation
      */
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
        if (std::fabs(sfreq*integ) < EPS_VALUES) { /* This is the single-sample case */
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
                if (std::fabs(f1-1.0) < 1e-3)
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
                        sum = sum + 0.5 * f1 * (f1 * std::fabs(data[n1-1][ch]) + (2.0 - f1) * std::fabs(data[n1][ch]));
                    if (f2 != 0.0)
                        sum = sum + 0.5 * f2 * (f2 * std::fabs(data[n2+1][ch]) + (2.0 - f2) * std::fabs(data[n2][ch]));
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

int mne_get_values_from_data_ch (float time,      /* Interesting time point */
                                 float integ,	  /* Time integration */
                                 float **data,	  /* The data values (channel by channel) */
                                 int   nsamp,	  /* How many time points? */
                                 int   nch,       /* How many channels */
                                 float tmin,	  /* Time of first sample */
                                 float sfreq,	  /* Sampling frequency */
                                 int   use_abs,   /* Use absolute values */
                                 float *value)	  /* The picked values */
/*
      * Pick a signal value using linear interpolation
      */
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
        if (std::fabs(sfreq * integ) < EPS_VALUES) { /* This is the single-sample case */
            s1 = sfreq*(time - tmin);
            n1 = floor(s1);
            f1 = 1.0 + n1 - s1;
            if (n1 < 0 || n1 > nsamp-1)
                return(-1);
            if (f1 < 1.0 && n1 > nsamp-2)
                return(-1);
            if (f1 < 1.0) {
                if (use_abs)
                    sum = f1 * std::fabs(data[ch][n1]) + (1.0 - f1) * std::fabs(data[ch][n1+1]);
                else
                    sum = f1*data[ch][n1] + (1.0-f1)*data[ch][n1+1];
            }
            else {
                if (use_abs)
                    sum = std::fabs(data[ch][n1]);
                else
                    sum = data[ch][n1];
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
                    sum = 0.5*((f1+f2)*std::fabs(data[ch][n1+1]) + (2.0-f1-f2)*std::fabs(data[ch][n1]));
                else
                    sum = 0.5*((f1+f2)*data[ch][n1+1] + (2.0-f1-f2)*data[ch][n1]);
            }
            else {
                f1 = n1 - s1;
                f2 = s2 - n2;
                if (n1 < 0 || n1 > nsamp-1 || n2 < 0 || n2 > nsamp-1)
                    return(-1);
                if (f1 != 0.0 && n1 < 1)
                    return(-1);
                if (f2 != 0.0 && n2 > nsamp-2)
                    return(-1);
                sum = 0.0;
                width = 0.0;
                if (n2 > n1) {		/* Do the whole intervals */
                    if (use_abs) {
                        sum = 0.5 * std::fabs(data[ch][n1]);
                        for (k = n1+1; k < n2; k++)
                            sum = sum + std::fabs(data[ch][k]);
                        sum = sum + 0.5 * std::fabs(data[ch][n2]);
                    }
                    else {
                        sum = 0.5*data[ch][n1];
                        for (k = n1+1; k < n2; k++)
                            sum = sum + data[ch][k];
                        sum = sum + 0.5*data[ch][n2];
                    }
                    width = n2 - n1;
                }
                /*
         * Add tails
         */
                if (use_abs) {
                    if (f1 != 0.0)
                        sum = sum + 0.5 * f1 * (f1 * std::fabs(data[ch][n1-1]) + (2.0 - f1) * std::fabs(data[ch][n1]));
                    if (f2 != 0.0)
                        sum = sum + 0.5 * f2 * (f2 * std::fabs(data[ch][n2+1]) + (2.0 - f2) * std::fabs(data[ch][n2]));
                }
                else {
                    if (f1 != 0.0)
                        sum = sum + 0.5*f1*(f1*data[ch][n1-1]+ (2.0-f1)*data[ch][n1]);
                    if (f2 != 0.0)
                        sum = sum + 0.5*f2*(f2*data[ch][n2+1] + (2.0-f2)*data[ch][n2]);
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
// DEFINE MEMBER METHODS
//=============================================================================================================

InvDipoleFit::InvDipoleFit(InvDipoleFitSettings* p_settings)
: settings(p_settings)
{
}

//=============================================================================================================
//todo split in initFit where the settings are handed over and the actual fit
InvEcdSet InvDipoleFit::calculateFit() const
{
    InvEcdSet set;

    qInfo("---- Setting up...\n");
    std::unique_ptr<FwdEegSphereModel> eeg_model;
    if (settings->include_eeg) {
        eeg_model = FwdEegSphereModel::setup_eeg_sphere_model(settings->eeg_model_file,settings->eeg_model_name,settings->eeg_sphere_rad);
        if (!eeg_model)
            return set;
    }

    std::unique_ptr<InvDipoleFitData> fit_data(InvDipoleFitData::setup_dipole_fit_data(
                                                            settings->mriname,
                                                            settings->measname,
                                                            settings->bemname,
                                                            &settings->r0,
                                                            eeg_model.get(),
                                                            settings->accurate,
                                                            settings->badname,
                                                            settings->noisename,
                                                            settings->grad_std,
                                                            settings->mag_std,
                                                            settings->eeg_std,
                                                            settings->mag_reg,
                                                            settings->grad_reg,
                                                            settings->eeg_reg,
                                                            settings->diagnoise,
                                                            settings->projnames,
                                                            settings->include_meg,
                                                            settings->include_eeg));
    if (!fit_data)
        return set;

    fit_data->fit_mag_dipoles = settings->fit_mag_dipoles;

    std::unique_ptr<MNERawData>        raw;
    std::unique_ptr<InvMeasData>       data;
    std::unique_ptr<MNEChSelection>    sel;

    if (settings->is_raw) {
        qInfo("\n---- Opening a raw data file...\n");
        raw.reset(MNERawData::open_file(settings->measname, true, false, settings->filter));
        if (!raw)
            return set;
        /*
         * A channel selection is needed to access the data
         */
        sel.reset(mne_ch_selection_these("fit",fit_data->ch_names,fit_data->nmeg+fit_data->neeg));
        mne_ch_selection_assign_chs(sel.get(),raw.get());
        for (int c = 0; c < sel->nchan; c++)
            if (sel->pick[c] < 0) {
                qCritical("All desired channels were not available");
                return set;
            }
        qInfo("\tChannel selection created.");
        /*
         * Let's be a little generous here
         */
        float t1 = raw->first_samp/raw->info->sfreq;
        float t2 = (raw->first_samp+raw->nsamp-1)/raw->info->sfreq;
        if (settings->tmin < t1 + settings->integ)
            settings->tmin = t1 + settings->integ;
        if (settings->tmax > t2 - settings->integ)
            settings->tmax =  t2 - settings->integ;
        if (settings->tstep < 0)
            settings->tstep = 1.0f/raw->info->sfreq;

        qInfo("\tOpened raw data file %s : %d MEG and %d EEG",
              settings->measname.toUtf8().constData(),fit_data->nmeg,fit_data->neeg);
    }
    else {
        qInfo("\n---- Reading data...\n");
        data.reset(InvMeasData::mne_read_meas_data(settings->measname,
                                                   settings->setno,
                                                   nullptr,
                                                   nullptr,
                                                   fit_data->ch_names,
                                                   fit_data->nmeg+fit_data->neeg));
        if (!data)
            return set;
        if (settings->do_baseline)
            data->adjust_baselines(settings->bmin,settings->bmax);
        else
            qInfo("\tNo baseline setting in effect.");
        if (settings->tmin < data->current->tmin + settings->integ/2.0f)
            settings->tmin = data->current->tmin + settings->integ/2.0f;
        if (settings->tmax > data->current->tmin + (data->current->np-1)*data->current->tstep - settings->integ/2.0f)
            settings->tmax =  data->current->tmin + (data->current->np-1)*data->current->tstep - settings->integ/2.0f;
        if (settings->tstep < 0)
            settings->tstep = data->current->tstep;

        qInfo("\tRead data set %d from %s : %d MEG and %d EEG",
              settings->setno,settings->measname.toUtf8().constData(),fit_data->nmeg,fit_data->neeg);
        if (!settings->noisename.isEmpty()) {
            qInfo("Scaling the noise covariance...");
            if (InvDipoleFitData::scale_noise_cov(fit_data.get(),data->current->nave) < 0)
                return set;
        }
    }

    /*
     * Proceed to computing the fits
     */
    qInfo("\n---- Computing the forward solution for the guesses...\n");
    auto guess = std::make_unique<InvGuessData>(settings->guessname,
                               settings->guess_surfname,
                               settings->guess_mindist, settings->guess_exclude, settings->guess_grid, fit_data.get());
    if (!guess)
        return set;

    qInfo("\n---- Fitting : %7.1f ... %7.1f ms (step: %6.1f ms integ: %6.1f ms)\n",
          1000*settings->tmin,1000*settings->tmax,1000*settings->tstep,1000*settings->integ);

    if (raw) {
        if (!fit_dipoles_raw(settings->measname,raw.get(),sel.get(),fit_data.get(),guess.get(),settings->tmin,settings->tmax,settings->tstep,settings->integ,settings->verbose,set))
            return set;
    }
    else {
        if (!fit_dipoles(settings->measname,data.get(),fit_data.get(),guess.get(),settings->tmin,settings->tmax,settings->tstep,settings->integ,settings->verbose,set))
            return set;
    }
    qInfo("%d dipoles fitted",set.size());

    return set;
}

//=============================================================================================================

bool InvDipoleFit::fit_dipoles( const QString& dataname, InvMeasData* data, InvDipoleFitData* fit, InvGuessData* guess, float tmin, float tmax, float tstep, float integ, int verbose, InvEcdSet& p_set)
{
    Eigen::VectorXf one(data->nchan);
    InvEcdSet set;
    InvEcd   dip;
    constexpr int report_interval = 10;

    set.dataname = dataname;

    if (verbose)
        qInfo("Fitting...");
    for (int s = 0; tmin + s*tstep < tmax; s++) {
        float time = tmin + s*tstep;
        if (mne_get_values_from_data(time,integ,data->current->data,data->current->np,data->nchan,data->current->tmin,
                                     1.0f/data->current->tstep,false,one.data()) < 0) {
            qWarning("Cannot pick time: %7.1f ms",1000.0f*time);
            continue;
        }

        if (!InvDipoleFitData::fit_one(fit,guess,time,one,verbose,dip))
            qWarning("t = %7.1f ms : fit error",1000.0f*time);
        else {
            set.addEcd(dip);
            if (verbose)
                dip.print();
            else {
                if (set.size() % report_interval == 0)
                    qInfo("%d..",set.size());
            }
        }
    }
    if (!verbose)
        qInfo("[done]");
    p_set = set;
    return true;
}

//=============================================================================================================

bool InvDipoleFit::fit_dipoles_raw(const QString& dataname, MNERawData* raw, mneChSelection sel, InvDipoleFitData* fit, InvGuessData* guess, float tmin, float tmax, float tstep, float integ, int verbose, InvEcdSet& p_set)
{
    const int   nchan   = sel->nchan;
    const float sfreq   = raw->info->sfreq;
    const float myinteg = integ > 0.0f ? 2*integ : 0.1f;
    const int   overlap = static_cast<int>(std::ceil(myinteg*sfreq));
    const int   length  = static_cast<int>(SEG_LEN*sfreq);
    const int   step    = length - overlap;
    const int   stepo   = step + overlap/2;
    int         start   = raw->first_samp;
    constexpr int report_interval = 10;

    Eigen::VectorXf one(nchan);

    // Row-major storage compatible with float** interface
    std::vector<float> storage(nchan * length);
    std::vector<float*> rows(nchan);
    for (int i = 0; i < nchan; ++i)
        rows[i] = storage.data() + i * length;
    float** data = rows.data();

    InvEcd    dip;
    InvEcdSet set;
    set.dataname = dataname;

    /*
     * Load the initial data segment
     */
    float stime = start/sfreq;
    if (raw->pick_data_filt(sel,start,length,data) < 0)
        return false;
    if (verbose)
        qInfo("Fitting...");
    for (int s = 0; tmin + s*tstep < tmax; s++) {
        float time = tmin + s*tstep;
        int picks = time*sfreq - start;
        if (picks > stepo) {
            start = start + step;
            if (raw->pick_data_filt(sel,start,length,data) < 0)
                return false;
            picks = time*sfreq - start;
            stime = start/sfreq;
        }
        if (mne_get_values_from_data_ch(time,integ,data,length,nchan,stime,sfreq,false,one.data()) < 0) {
            qWarning("Cannot pick time: %8.3f s",time);
            continue;
        }
        if (!InvDipoleFitData::fit_one(fit,guess,time,one,verbose,dip))
            qWarning("t = %8.3f s : fit error",time);
        else {
            set.addEcd(dip);
            if (verbose)
                dip.print();
            else {
                if (set.size() % report_interval == 0)
                    qInfo("%d..",set.size());
            }
        }
    }
    if (!verbose)
        qInfo("[done]");
    p_set = set;
    return true;
}

//=============================================================================================================

bool InvDipoleFit::fit_dipoles_raw(const QString& dataname, MNERawData* raw, mneChSelection sel, InvDipoleFitData* fit, InvGuessData* guess, float tmin, float tmax, float tstep, float integ, int verbose)
{
    InvEcdSet set;
    return fit_dipoles_raw(dataname, raw, sel, fit, guess, tmin, tmax, tstep, integ, verbose, set);
}
