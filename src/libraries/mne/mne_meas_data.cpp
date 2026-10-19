//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_meas_data.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implementation of @ref MNELIB::MNEMeasData.
 *
 * Implements construction, ownership of the contained measurement data
 * sets and the projection bookkeeping that the MNE C tools expected.
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
#include <QDebug>

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
, nbad(0)
, ch_major(false)
, nset(0)
, current(nullptr)
{
    meas_date.secs = 0;
    meas_date.usecs = 0;
}

//=============================================================================================================

MNEMeasData::~MNEMeasData() = default;

//=============================================================================================================

void MNEMeasData::adjust_baselines(float bmin, float bmax)
{
    if (!current)
        return;

    const float currentSfreq = 1.0f / current->tstep;
    const float tmin = current->tmin;
    const float tmax = current->tmin + (current->np - 1) / currentSfreq;

    int b1, b2;
    if (bmin < tmin)
        b1 = 0;
    else if (bmin > tmax)
        b1 = current->np;
    else {
        for (b1 = 0; b1 / currentSfreq + tmin < bmin; b1++)
            ;
        b1 = qBound(0, b1, current->np);
    }
    if (bmax < tmin)
        b2 = 0;
    else if (bmax > tmax)
        b2 = current->np;
    else {
        for (b2 = current->np; b2 / currentSfreq + tmin > bmax; b2--)
            ;
        b2 = qBound(0, b2, current->np);
    }

    Eigen::MatrixXf& bdata = current->data;
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
        qInfo("\t%s : using baseline %7.1f ... %7.1f ms\n",
              current->comment.toUtf8().constData() ? current->comment.toUtf8().constData() : "unknown",
              1000 * (tmin + b1 / sfreq),
              1000 * (tmin + b2 / sfreq));
    }
}

//=============================================================================================================

bool MNEMeasData::read_evoked_set(const QString& name,
                                  int set,
                                  MNEInverseOperator* op,
                                  MNENamedMatrix* fwd,
                                  const QStringList& namesp,
                                  int nnamesp,
                                  MNEMeasData& target,
                                  bool fresh)
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
                          QPair<float, float>(-1.0f, -1.0f), false)) {
        qCritical("Failed to read evoked data from %s\n", name.toUtf8().constData());
        return false;
    }

    /*
       * Extract fields from the FiffEvoked object
       */
    const int nchan_file = evoked.info.nchan;
    const int nsamp = evoked.last - evoked.first + 1;
    const float sfreq = evoked.info.sfreq;
    const float dtmin = static_cast<float>(evoked.first) / sfreq;
    const float highpass = evoked.info.highpass;
    const float lowpass = evoked.info.lowpass;
    const int nave = evoked.nave;
    const int aspect_kind = evoked.aspect_kind;
    const QList<FiffChInfo>& chs = evoked.info.chs;
    const FiffId& id = evoked.info.meas_id;
    const MatrixXf data = evoked.data.cast<float>(); /* nchan × nsamp, already calibrated */
    const FiffCoordTrans& devHeadT = evoked.info.dev_head_t;

    QString stim14_name;
    /*
     * Desired channels
     */
    QStringList names;
    int nchan = 0;
    /*
       * Selected channels
       */
    Eigen::VectorXi sel;
    int stim14 = -1;
    /*
       * Other stuff
       */
    float tmin, tmax;
    int k, p, c, np, n1, n2;

    stim14_name = qEnvironmentVariable(MNE_ENV_TRIGGER_CH);
    if (stim14_name.isEmpty() || stim14_name.size() == 0)
        stim14_name = MNE_DEFAULT_TRIGGER_CH;
    // FiffTag::toChInfo strips the spaces from channel names.
    stim14_name.remove(' ');

    if (!fresh) {
        for (int i = 0; i < target.nchan; i++)
            names.append(target.chs[i].ch_name);
        nchan = target.nchan;
    } else {
        if (op) {
            names = op->eigen_fields->col_names;
            nchan = op->nchan;
        } else if (fwd) {
            names = fwd->collist;
            nchan = fwd->ncol;
        } else {
            names = namesp;
            nchan = nnamesp;
        }
        if (names.isEmpty())
            nchan = 0;
    }

    if (!id.isEmpty())
        qInfo("\tMeasurement file id: %s\n", id.toString().toUtf8().constData());

    /*
       * Pick out the necessary channels
       */
    if (nchan > 0) {
        sel = Eigen::VectorXi::Constant(nchan, -1);
        for (c = 0; c < nchan_file; c++) {
            for (k = 0; k < nchan; k++) {
                if (sel[k] == -1 && QString::compare(chs[c].ch_name, names[k]) == 0) {
                    sel[k] = c;
                    break;
                }
            }
            if (QString(chs[c].ch_name).remove(' ') == stim14_name) {
                stim14 = c;
            }
        }
        for (k = 0; k < nchan; k++)
            if (sel[k] == -1) {
                qCritical("All channels needed were not in the MEG/EEG data file "
                          "(first missing: %s).",
                          names[k].toUtf8().constData());
                return false;
            }
    } else { /* Load all channels */
        sel.resize(nchan_file);
        sel.setZero();
        for (c = 0, nchan = 0; c < nchan_file; c++) {
            if (chs[c].kind == FIFFV_MEG_CH || chs[c].kind == FIFFV_EEG_CH) {
                sel[nchan] = c;
                nchan++;
            }
            if (QString(chs[c].ch_name).remove(' ') == stim14_name) {
                stim14 = c;
            }
        }
    }
    /*
       * Cut the data to the analysis time range
       */
    n1 = 0;
    n2 = nsamp;
    np = n2 - n1;
    tmin = dtmin;
    tmax = dtmin + (np - 1) / sfreq;
    qInfo("\tData time range: %8.1f ... %8.1f ms\n", 1000 * tmin, 1000 * tmax);
    /*
       * Just put it together
       */
    if (fresh) {
        target.filename = name;
        target.meas_id = id;
        /*
         * Getting starting time from measurement ID is not too accurate...
         */
        {
            FiffTime md;
            md.secs = evoked.info.meas_date[0];
            md.usecs = evoked.info.meas_date[1];
            if (md.secs != 0)
                target.meas_date = md;
            else if (!target.meas_id.isEmpty())
                target.meas_date = target.meas_id.time;
            else {
                target.meas_date.secs = 0;
                target.meas_date.usecs = 0;
            }
        }
        target.lowpass = lowpass;
        target.highpass = highpass;
        target.nchan = nchan;
        target.sfreq = sfreq;

        if (!devHeadT.isEmpty()) {
            target.meg_head_t = std::make_unique<FiffCoordTrans>(devHeadT);
            qInfo("\tUsing MEG <-> head transform from the present data set\n");
        }
        if (op != nullptr && !op->mri_head_t.isEmpty()) { /* Copy if available */
            target.mri_head_t = std::make_unique<FiffCoordTrans>(op->mri_head_t);
            qInfo("\tPicked MRI <-> head transform from the inverse operator\n");
        }
        /*
         * Channel list
         */
        for (k = 0; k < nchan; k++) {
            target.chs.append(FiffChInfo());
            target.chs[k] = chs[sel[k]];
        }

        target.op = op;   /* Attach inverse operator */
        target.fwd = fwd; /* ...or a fwd operator */
        if (op) {         /* Attach the projection operator and CTF compensation info to the data, too */
            target.proj = MNEProjOp::read(name);
            if (target.proj && target.proj->nitems > 0) {
                qInfo("\tLoaded projection from %s:\n", name.toUtf8().data());
                QTextStream errStream(stderr);
                target.proj->report(errStream, QStringLiteral("\t\t"));
            }
        } else {
            target.proj = MNEProjOp::read(name);
            if (target.proj && target.proj->nitems > 0) {
                qInfo("\tLoaded projection from %s:\n", name.toUtf8().data());
                QTextStream errStream(stderr);
                target.proj->report(errStream, QStringLiteral("\t\t"));
            }
            target.comp = MNECTFCompDataSet::read(name);
            if (!target.comp) {
                return false;
            }
            if (target.comp->ncomp > 0)
                qInfo("\tRead %d compensation data sets from %s\n", target.comp->ncomp, name.toUtf8().data());
        }
        /*
         * Bad channels — already read by FiffEvoked via FiffStream::read_meas_info()
         */
        {
            target.badlist = evoked.info.bads;
            target.nbad = target.badlist.size();
            target.bad = Eigen::VectorXi::Zero(target.nchan);

            for (int b = 0; b < target.nbad; b++) {
                for (k = 0; k < target.nchan; k++) {
                    if (QString::compare(target.chs[k].ch_name, target.badlist[b], Qt::CaseInsensitive) == 0) {
                        target.bad[k] = 1;
                        break;
                    }
                }
            }
            qInfo("\t%d bad channels read from %s%s", target.nbad, name.toUtf8().data(), target.nbad > 0 ? ":\n" : "\n");
            if (target.nbad > 0) {
                qInfo("\t\t");
                for (k = 0; k < target.nbad; k++)
                    qInfo("%s%c", target.badlist[k].toUtf8().constData(), k < target.nbad - 1 ? ' ' : '\n');
            }
        }
    }
    /*
       * New data set is created anyway
       */
    auto dataset = std::make_unique<MNEMeasDataSet>();
    dataset->tmin = tmin;
    dataset->tstep = 1.0 / sfreq;
    dataset->first = n1;
    dataset->np = np;
    dataset->nave = nave;
    dataset->kind = aspect_kind;
    dataset->data = Eigen::MatrixXf::Zero(np, nchan);
    dataset->comment = evoked.comment;
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
        for (p = 0; p < np; p++) /* Copy the data and correct for the possible non-unit calibration */
            dataset->stim14[p] = data(stim14, p + n1) / chs[stim14].cal;
    }
    const QString comment = dataset->comment.isEmpty() ? QStringLiteral("unknown") : dataset->comment;
    target.sets.push_back(std::move(dataset));
    target.nset++;
    if (fresh)
        target.current = target.sets.front().get();
    qInfo("\t%s dataset %s from %s", fresh ? "Loaded" : "Added", qPrintable(comment), qPrintable(name));
    return true;
}

//=============================================================================================================

std::unique_ptr<MNEMeasData> MNEMeasData::mne_read_meas_data(const QString& name,
                                                             int set,
                                                             MNEInverseOperator* op,
                                                             MNENamedMatrix* fwd,
                                                             const QStringList& namesp,
                                                             int nnamesp)
{
    auto data = std::make_unique<MNEMeasData>();
    if (!read_evoked_set(name, set, op, fwd, namesp, nnamesp, *data, true))
        return nullptr;
    return data;
}

//=============================================================================================================

bool MNEMeasData::mne_read_meas_data_add(const QString& name,
                                         int set,
                                         MNEInverseOperator* op,
                                         MNENamedMatrix* fwd,
                                         const QStringList& namesp,
                                         int nnamesp,
                                         MNEMeasData& add_to)
{
    return read_evoked_set(name, set, op, fwd, namesp, nnamesp, add_to, false);
}
