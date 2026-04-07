//=============================================================================================================
/**
 * @file     mne_meas_data.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the MNE Meas Data (MNEMeasData) Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_meas_data.h"
#include "mne_meas_data_set.h"
#include "mne_inverse_operator.h"

#include "mne_types.h"
#include "mne_named_matrix.h"

#include <fiff/fiff_types.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_evoked.h>

#include <vector>

#include <QFile>
#include <QTextStream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace MNELIB;
using namespace MNELIB;


//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEMeasData::MNEMeasData()
: sfreq(0.0f)
, nchan(0)
, highpass(0.0f)
, lowpass(0.0f)
, op(nullptr)
, fwd(nullptr)
, chsel(nullptr)
, nbad(0)
, ch_major(false)
, nset(0)
, current(nullptr)
{
    meas_date.secs = 0;
    meas_date.usecs = 0;
}

//=============================================================================================================

MNEMeasData::~MNEMeasData()
{
    for (int k = 0; k < nset; k++)
        delete sets[k];

    delete chsel;
}

//=============================================================================================================

void MNEMeasData::adjust_baselines(float bmin, float bmax)
{
    if (!current)
        return;

    const float sfreq = 1.0f / current->tstep;
    const float tmin  = current->tmin;
    const float tmax  = current->tmin + (current->np - 1) / sfreq;

    int b1, b2;
    if (bmin < tmin)
        b1 = 0;
    else if (bmin > tmax)
        b1 = current->np;
    else {
        for (b1 = 0; b1 / sfreq + tmin < bmin; b1++)
            ;
        b1 = qBound(0, b1, current->np);
    }
    if (bmax < tmin)
        b2 = 0;
    else if (bmax > tmax)
        b2 = current->np;
    else {
        for (b2 = current->np; b2 / sfreq + tmin > bmax; b2--)
            ;
        b2 = qBound(0, b2, current->np);
    }

    Eigen::MatrixXf &bdata = current->data;
    if (b2 > b1) {
        for (int c = 0; c < nchan; c++) {
            float ave = 0.0f;
            for (int s = b1; s < b2; s++)
                ave += bdata(s, c);
            ave /= (b2 - b1);
            current->baselines[c] += ave;
            for (int s = 0; s < current->np; s++)
                bdata(s, c) -= ave;
        }
        printf("\t%s : using baseline %7.1f ... %7.1f ms\n",
                current->comment.toUtf8().constData() ? current->comment.toUtf8().constData() : "unknown",
                1000 * (tmin + b1 / sfreq),
                1000 * (tmin + b2 / sfreq));
    }
}

//=============================================================================================================

MNEMeasData *MNEMeasData::mne_read_meas_data_add(const QString &name,
                                                 int set,
                                                 MNEInverseOperator* op,
                                                 MNENamedMatrix *fwd,
                                                 const QStringList& namesp,
                                                 int nnamesp,
                                                 MNEMeasData *add_to)     /* Add to this */
/*
          * Read an evoked-response data file
          */
{
    /*
       * Read evoked data via FiffEvoked (no baseline correction, no projection
       * — MNEMeasData handles projections separately via MNEProjOp).
       */
    QFile file(name);
    FiffEvoked evoked;
    if (!FiffEvoked::read(file, evoked, set - 1,
                          QPair<float,float>(-1.0f, -1.0f), false))
    {
        printf("Failed to read evoked data from %s\n", name.toUtf8().constData());
        return nullptr;
    }

    /*
       * Extract fields from the FiffEvoked object
       */
    const int nchan_file  = evoked.info.nchan;
    const int nsamp       = evoked.last - evoked.first + 1;
    const float sfreq     = evoked.info.sfreq;
    const float dtmin     = static_cast<float>(evoked.first) / sfreq;
    const float highpass  = evoked.info.highpass;
    const float lowpass   = evoked.info.lowpass;
    const int nave        = evoked.nave;
    const int aspect_kind = evoked.aspect_kind;
    const QList<FiffChInfo>& chs = evoked.info.chs;
    const FiffId& id      = evoked.info.meas_id;
    const MatrixXf data   = evoked.data.cast<float>();   /* nchan × nsamp, already calibrated */
    const FiffCoordTrans& devHeadT = evoked.info.dev_head_t;

    QString        stim14_name;
    /*
     * Desired channels
     */
    QStringList         names;
    int         nchan   = 0;
    /*
       * Selected channels
       */
    Eigen::VectorXi sel;
    int         stim14 = -1;
    /*
       * Other stuff
       */
    float       tmin,tmax;
    int         k,p,c,np,n1,n2;
    MNEMeasData*    res = nullptr;
    MNEMeasData*    new_data = add_to;
    MNEMeasDataSet* dataset = nullptr;

    stim14_name = qEnvironmentVariable(MNE_ENV_TRIGGER_CH);
    if (stim14_name.isEmpty() || stim14_name.size() == 0)
        stim14_name = MNE_DEFAULT_TRIGGER_CH;

    if (add_to) {
        for (int i = 0; i < add_to->nchan; i++)
            names.append(add_to->chs[i].ch_name);
        nchan = add_to->nchan;
    }
    else {
        if (op) {
            names = op->eigen_fields->col_names;
            nchan = op->nchan;
        }
        else if (fwd) {
            names = fwd->collist;
            nchan = fwd->ncol;
        }
        else {
            names = namesp;
            nchan = nnamesp;
        }
        if (names.isEmpty())
            nchan = 0;
    }

    if (!id.isEmpty())
        printf("\tMeasurement file id: %s\n", id.toString().toUtf8().constData());

    /*
       * Pick out the necessary channels
       */
    if (nchan > 0) {
        sel = Eigen::VectorXi::Constant(nchan, -1);
        for (c = 0; c < nchan_file; c++) {
            for (k = 0; k < nchan; k++) {
                if (sel[k] == -1 && QString::compare(chs[c].ch_name,names[k]) == 0) {
                    sel[k] = c;
                    break;
                }
            }
            if (QString::compare(stim14_name,chs[c].ch_name) == 0) {
                stim14 = c;
            }
        }
        for (k = 0; k < nchan; k++)
            if (sel[k] == -1) {
                printf("All channels needed were not in the MEG/EEG data file "
                       "(first missing: %s).",names[k].toUtf8().constData());
                goto out;
            }
    }
    else {  /* Load all channels */
        sel.resize(nchan_file);
        sel.setZero();
        for (c = 0, nchan = 0; c < nchan_file; c++) {
            if (chs[c].kind == FIFFV_MEG_CH || chs[c].kind == FIFFV_EEG_CH) {
                sel[nchan] = c;
                nchan++;
            }
            if (QString::compare(stim14_name,chs[c].ch_name) == 0) {
                stim14 = c;
            }
        }
    }
    /*
       * Cut the data to the analysis time range
       */
    n1    = 0;
    n2    = nsamp;
    np    = n2 - n1;
    tmin  = dtmin;
    tmax  = dtmin + (np-1)/sfreq;
    printf("\tData time range: %8.1f ... %8.1f ms\n",1000*tmin,1000*tmax);
    /*
       * Just put it together
       */
    if (!new_data) {			/* We need a new meas data structure */
        new_data     = new MNEMeasData;
        new_data->filename  = name;
        new_data->meas_id   = id;
        /*
         * Getting starting time from measurement ID is not too accurate...
         */
        {
            FiffTime md;
            md.secs  = evoked.info.meas_date[0];
            md.usecs = evoked.info.meas_date[1];
            if (md.secs != 0)
                new_data->meas_date = md;
            else if (!new_data->meas_id.isEmpty())
                new_data->meas_date = new_data->meas_id.time;
            else {
                new_data->meas_date.secs = 0;
                new_data->meas_date.usecs = 0;
            }
        }
        new_data->lowpass   = lowpass;
        new_data->highpass  = highpass;
        new_data->nchan     = nchan;
        new_data->sfreq     = sfreq;

        if (!devHeadT.isEmpty()) {
            new_data->meg_head_t = std::make_unique<FiffCoordTrans>(devHeadT);
            printf("\tUsing MEG <-> head transform from the present data set\n");
        }
        if (op != nullptr && !op->mri_head_t.isEmpty()) { /* Copy if available */
            new_data->mri_head_t = std::make_unique<FiffCoordTrans>(op->mri_head_t);
            printf("\tPicked MRI <-> head transform from the inverse operator\n");
        }
        /*
         * Channel list
         */
        for (k = 0; k < nchan; k++) {
            new_data->chs.append(FiffChInfo());
            new_data->chs[k] = chs[sel[k]];
        }

        new_data->op  = op;		/* Attach inverse operator */
        new_data->fwd = fwd;		/* ...or a fwd operator */
        if (op) {			/* Attach the projection operator and CTF compensation info to the data, too */
            new_data->proj.reset(MNEProjOp::read(name));
            if (new_data->proj && new_data->proj->nitems > 0) {
                printf("\tLoaded projection from %s:\n",name.toUtf8().data());
                QTextStream errStream(stderr);
                new_data->proj->report(errStream, QStringLiteral("\t\t"));
            }
        }
        else {
            new_data->proj.reset(MNEProjOp::read(name));
            if (new_data->proj && new_data->proj->nitems > 0) {
                printf("\tLoaded projection from %s:\n",name.toUtf8().data());
                QTextStream errStream(stderr);
                new_data->proj->report(errStream, QStringLiteral("\t\t"));
            }
            new_data->comp = MNECTFCompDataSet::read(name);
            if (!new_data->comp)
                goto out;
            if (new_data->comp->ncomp > 0)
                printf("\tRead %d compensation data sets from %s\n",new_data->comp->ncomp,name.toUtf8().data());
        }
        /*
         * Bad channels — already read by FiffEvoked via FiffStream::read_meas_info()
         */
        {
            new_data->badlist = evoked.info.bads;
            new_data->nbad    = new_data->badlist.size();
            new_data->bad = Eigen::VectorXi::Zero(new_data->nchan);

            for (int b = 0; b < new_data->nbad; b++) {
                for (k = 0; k < new_data->nchan; k++) {
                    if (QString::compare(new_data->chs[k].ch_name,new_data->badlist[b],Qt::CaseInsensitive) == 0) {
                        new_data->bad[k] = 1;
                        break;
                    }
                }
            }
            printf("\t%d bad channels read from %s%s",new_data->nbad,name.toUtf8().data(),new_data->nbad > 0 ? ":\n" : "\n");
            if (new_data->nbad > 0) {
                printf("\t\t");
                for (k = 0; k < new_data->nbad; k++)
                    printf("%s%c",new_data->badlist[k].toUtf8().constData(),k < new_data->nbad-1 ? ' ' : '\n');
            }
        }
    }
    /*
       * New data set is created anyway
       */
    dataset = new MNEMeasDataSet;
    dataset->tmin      = tmin;
    dataset->tstep     = 1.0/sfreq;
    dataset->first     = n1;
    dataset->np        = np;
    dataset->nave      = nave;
    dataset->kind      = aspect_kind;
    dataset->data      = Eigen::MatrixXf::Zero(np, nchan);
    dataset->comment   = evoked.comment;
    dataset->baselines = Eigen::VectorXf::Zero(nchan);
    /*
       * Pick data from all channels
       */
    for (k = 0; k < nchan; k++) {
        /*
         * Shift the response
         */
        for (p = 0; p < np; p++)
            dataset->data(p, k) = data(sel[k], p + n1);
    }
    /*
       * Pick the digital trigger channel, too
       */
    if (stim14 >= 0) {
        dataset->stim14 = Eigen::VectorXf(np);
        for (p = 0; p < np; p++) 	/* Copy the data and correct for the possible non-unit calibration */
            dataset->stim14[p] = data(stim14, p + n1) / chs[stim14].cal;
    }
    new_data->sets.append(dataset); dataset = nullptr;
    new_data->nset++;
    if (!add_to)
        new_data->current = new_data->sets[0];
    res = new_data;
    printf("\t%s dataset %s from %s\n",
            add_to ? "Added" : "Loaded",
            new_data->sets[new_data->nset-1]->comment.toUtf8().constData() ? new_data->sets[new_data->nset-1]->comment.toUtf8().constData() : "unknown",name.toUtf8().data());

out : {
        if (res == nullptr && !add_to)
            delete new_data;
        return res;
    }
}

//=============================================================================================================

MNEMeasData *MNEMeasData::mne_read_meas_data(const QString &name,
                                             int set,
                                             MNEInverseOperator* op,
                                             MNENamedMatrix *fwd,
                                             const QStringList& namesp,
                                             int nnamesp)

{
    return mne_read_meas_data_add(name,set,op,fwd,namesp,nnamesp,nullptr);
}
