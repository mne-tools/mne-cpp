//=============================================================================================================
/**
 * @file     fiff_evoked_set.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the FiffEvokedSet Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_evoked_set.h"
#include "fiff_events.h"
#include "fiff_raw_data.h"
#include "fiff_tag.h"
#include "fiff_dir_node.h"
#include "fiff_stream.h"
#include "fiff_file.h"
#include "fiff_constants.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/SparseCore>

#include <cmath>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffEvokedSet::FiffEvokedSet()
{
    qRegisterMetaType<FIFFLIB::FiffEvokedSet>("FIFFLIB::FiffEvokedSet");
    qRegisterMetaType<FIFFLIB::FiffEvokedSet::SPtr>("FIFFLIB::FiffEvokedSet::SPtr");
}

//=============================================================================================================

FiffEvokedSet::FiffEvokedSet(QIODevice& p_IODevice)
{
    qRegisterMetaType<FIFFLIB::FiffEvokedSet>("FIFFLIB::FiffEvokedSet");
    qRegisterMetaType<FIFFLIB::FiffEvokedSet::SPtr>("FIFFLIB::FiffEvokedSet::SPtr");

    if(!FiffEvokedSet::read(p_IODevice, *this))
    {
        qWarning("\tFiff evoked data set not found.\n");//ToDo Throw here
        return;
    }
}

//=============================================================================================================

FiffEvokedSet::FiffEvokedSet(const FiffEvokedSet& p_FiffEvokedSet)
: info(p_FiffEvokedSet.info)
, evoked(p_FiffEvokedSet.evoked)
{
}

//=============================================================================================================

FiffEvokedSet::~FiffEvokedSet()
{
}

//=============================================================================================================

void FiffEvokedSet::clear()
{
    info.clear();
    evoked.clear();
}

//=============================================================================================================

FiffEvokedSet FiffEvokedSet::pick_channels(const QStringList& include,
                                           const QStringList& exclude) const
{
    FiffEvokedSet res;

    //
    //   Update info to match the channel selection
    //
    RowVectorXi sel = FiffInfo::pick_channels(this->info.ch_names, include, exclude);
    if (sel.cols() > 0) {
        res.info = this->info.pick_info(sel);
    } else {
        res.info = this->info;
    }

    QList<FiffEvoked>::ConstIterator ev;
    for(ev = evoked.begin(); ev != evoked.end(); ++ev)
        res.evoked.push_back(ev->pick_channels(include, exclude));

    return res;
}

//=============================================================================================================

bool FiffEvokedSet::compensate_to(FiffEvokedSet& p_FiffEvokedSet,
                                  fiff_int_t to) const
{
    qint32 now = p_FiffEvokedSet.info.get_current_comp();
    FiffCtfComp ctf_comp;

    if(now == to)
    {
        qInfo("Data is already compensated as desired.\n");
        return false;
    }

    //Make the compensator and apply it to all data sets
    p_FiffEvokedSet.info.make_compensator(now,to,ctf_comp);

    for(qint16 i=0; i < p_FiffEvokedSet.evoked.size(); ++i)
    {
        p_FiffEvokedSet.evoked[i].data = ctf_comp.data->data*p_FiffEvokedSet.evoked[i].data;
    }

    //Update the compensation info in the channel descriptors
    p_FiffEvokedSet.info.set_current_comp(to);

    return true;
}

//=============================================================================================================

bool FiffEvokedSet::find_evoked(const FiffEvokedSet& p_FiffEvokedSet) const
{
    if(!p_FiffEvokedSet.evoked.size()) {
        qWarning("No evoked response data sets in %s\n",p_FiffEvokedSet.info.filename.toUtf8().constData());
        return false;
    }
    else
        qInfo("\nFound %lld evoked response data sets in %s :\n",p_FiffEvokedSet.evoked.size(),p_FiffEvokedSet.info.filename.toUtf8().constData());

    for(qint32 i = 0; i < p_FiffEvokedSet.evoked.size(); ++i) {
        qInfo("%s (%s)\n",p_FiffEvokedSet.evoked.at(i).comment.toUtf8().constData(),p_FiffEvokedSet.evoked.at(i).aspectKindToString().toUtf8().constBegin());
    }

    return true;
}

//=============================================================================================================

bool FiffEvokedSet::read(QIODevice& p_IODevice,
                         FiffEvokedSet& p_FiffEvokedSet, QPair<float,float> baseline,
                         bool proj)
{
    p_FiffEvokedSet.clear();

    //
    //   Open the file
    //
    FiffStream::SPtr t_pStream(new FiffStream(&p_IODevice));
    QString t_sFileName = t_pStream->streamName();

    qInfo("Exploring %s ...\n",t_sFileName.toUtf8().constData());

    if(!t_pStream->open())
        return false;
    //
    //   Read the measurement info
    //
    FiffDirNode::SPtr meas;
    if(!t_pStream->read_meas_info(t_pStream->dirtree(), p_FiffEvokedSet.info, meas))
        return false;
    p_FiffEvokedSet.info.filename = t_sFileName; //move fname storage to read_meas_info member function
    //
    //   Locate the data of interest
    //
    QList<FiffDirNode::SPtr> processed = meas->dir_tree_find(FIFFB_PROCESSED_DATA);
    if (processed.size() == 0)
    {
        qWarning("Could not find processed data");
        return false;
    }
    //
    QList<FiffDirNode::SPtr> evoked_node = meas->dir_tree_find(FIFFB_EVOKED);
    if (evoked_node.size() == 0)
    {
        qWarning("Could not find evoked data");
        return false;
    }

    QStringList comments;
    QList<fiff_int_t> aspect_kinds;
    QString t;
    if(!t_pStream->get_evoked_entries(evoked_node, comments, aspect_kinds, t))
        t = QString("None found, must use integer");
    qInfo("\tFound %lld datasets\n", evoked_node.size());

    for(qint32 i = 0; i < comments.size(); ++i)
    {
        QFile t_file(p_FiffEvokedSet.info.filename);
        qInfo(">> Processing %s <<\n", comments[i].toUtf8().constData());
        FiffEvoked t_FiffEvoked;
        if(FiffEvoked::read(t_file, t_FiffEvoked, i, baseline, proj))
            p_FiffEvokedSet.evoked.push_back(t_FiffEvoked);
    }

    return true;
}

//=============================================================================================================

bool FiffEvokedSet::save(const QString &fileName) const
{
    if (fileName.isEmpty()) {
        qWarning() << "[FiffEvokedSet::save] Output file not specified.";
        return false;
    }

    QFile file(fileName);
    FiffStream::SPtr pStream = FiffStream::start_file(file);
    if (!pStream) {
        qWarning() << "[FiffEvokedSet::save] Cannot open" << fileName;
        return false;
    }

    pStream->write_evoked_set(*this);
    pStream->end_file();

    qInfo() << "[FiffEvokedSet::save] Saved" << evoked.size()
            << "average(s) to" << fileName;
    return true;
}

//=============================================================================================================

FiffEvokedSet FiffEvokedSet::computeGrandAverage(const QList<FiffEvokedSet> &evokedSets)
{
    FiffEvokedSet grandAvg;

    if (evokedSets.isEmpty()) {
        qWarning() << "[FiffEvokedSet::computeGrandAverage] No evoked sets provided.";
        return grandAvg;
    }

    grandAvg = evokedSets[0];

    for (int f = 1; f < evokedSets.size(); ++f) {
        const FiffEvokedSet &eset = evokedSets[f];
        int nCat = qMin(grandAvg.evoked.size(), eset.evoked.size());
        for (int j = 0; j < nCat; ++j) {
            if (grandAvg.evoked[j].data.cols() == eset.evoked[j].data.cols() &&
                grandAvg.evoked[j].data.rows() == eset.evoked[j].data.rows()) {
                grandAvg.evoked[j].data += eset.evoked[j].data;
                grandAvg.evoked[j].nave += eset.evoked[j].nave;
            }
        }
    }

    for (int j = 0; j < grandAvg.evoked.size(); ++j) {
        grandAvg.evoked[j].data /= static_cast<double>(evokedSets.size());
    }

    return grandAvg;
}

//=============================================================================================================

void FiffEvokedSet::subtractBaseline(Eigen::MatrixXd &epoch, int bminSamp, int bmaxSamp)
{
    if (bminSamp < 0) bminSamp = 0;
    if (bmaxSamp >= epoch.cols()) bmaxSamp = static_cast<int>(epoch.cols()) - 1;
    if (bminSamp >= bmaxSamp) return;

    int nBase = bmaxSamp - bminSamp + 1;
    for (int c = 0; c < epoch.rows(); ++c) {
        double baseVal = epoch.row(c).segment(bminSamp, nBase).mean();
        epoch.row(c).array() -= baseVal;
    }
}

//=============================================================================================================

FiffEvokedSet FiffEvokedSet::computeAverages(const FiffRawData &raw,
                                              const AverageDescription &desc,
                                              const MatrixXi &events,
                                              QString &log)
{
    FiffEvokedSet evokedSet;
    evokedSet.info = raw.info;

    float sfreq = raw.info.sfreq;
    int nchan   = raw.info.nchan;

    log.clear();
    log += QString("Averaging: %1\n").arg(desc.comment);

    // Process each category
    for (int j = 0; j < desc.categories.size(); ++j) {
        const AverageCategory &cat = desc.categories[j];

        // Compute sample indices
        int minSamp = static_cast<int>(std::round(cat.tmin * sfreq));
        int maxSamp = static_cast<int>(std::round(cat.tmax * sfreq));
        int ns      = maxSamp - minSamp + 1;
        int delaySamp = static_cast<int>(std::round(cat.delay * sfreq));

        // Baseline sample range (relative to epoch start)
        int bminSamp = 0, bmaxSamp = 0;
        if (cat.doBaseline) {
            bminSamp = static_cast<int>(std::round(cat.bmin * sfreq)) - minSamp;
            bmaxSamp = static_cast<int>(std::round(cat.bmax * sfreq)) - minSamp;
        }

        // Accumulator
        MatrixXd sumData = MatrixXd::Zero(nchan, ns);
        MatrixXd sumSqData = MatrixXd::Zero(nchan, ns);
        int nave = 0;

        log += QString("\n  Category: %1\n").arg(cat.comment);
        log += QString("    t = %.1f ... %.1f ms\n").arg(1000.0 * cat.tmin).arg(1000.0 * cat.tmax);

        // Iterate over events
        for (int k = 0; k < events.rows(); ++k) {
            if (!FiffEvents::matchEvent(cat, events, k))
                continue;

            int evSample = events(k, 0);
            int epochStart = evSample + delaySamp + minSamp;
            int epochEnd   = evSample + delaySamp + maxSamp;

            // Check bounds
            if (epochStart < raw.first_samp || epochEnd > raw.last_samp)
                continue;

            // Read epoch
            MatrixXd epochData;
            MatrixXd epochTimes;
            if (!raw.read_raw_segment(epochData, epochTimes, epochStart, epochEnd)) {
                log += QString("    Error reading epoch at sample %1\n").arg(evSample);
                continue;
            }

            // Artifact rejection
            QString rejReason;
            if (!checkArtifacts(epochData, raw.info, raw.info.bads, desc.rej, rejReason)) {
                log += QString("    %1 %2 %3 %4 [%5] %6 [omit]\n")
                    .arg(evSample, 7)
                    .arg(static_cast<float>(evSample) / sfreq, -10, 'f', 3)
                    .arg(events(k, 1), 3)
                    .arg(events(k, 2), 3)
                    .arg(cat.comment)
                    .arg(rejReason);
                continue;
            }

            // Baseline correction
            if (cat.doBaseline) {
                subtractBaseline(epochData, bminSamp, bmaxSamp);
            }

            // Absolute value
            if (cat.doAbs) {
                epochData = epochData.cwiseAbs();
            }

            // Accumulate
            sumData += epochData;
            if (cat.doStdErr) {
                sumSqData += epochData.cwiseProduct(epochData);
            }
            nave++;

            log += QString("    %1 %2 %3 %4 [%5]\n")
                .arg(evSample, 7)
                .arg(static_cast<float>(evSample) / sfreq, -10, 'f', 3)
                .arg(events(k, 1), 3)
                .arg(events(k, 2), 3)
                .arg(cat.comment);
        }

        // Compute average
        FiffEvoked evoked;
        evoked.comment = cat.comment;
        evoked.first   = minSamp;
        evoked.last    = maxSamp;
        evoked.nave    = nave;

        // Build times vector
        RowVectorXf times(ns);
        for (int s = 0; s < ns; ++s)
            times(s) = static_cast<float>(minSamp + s) / sfreq;
        evoked.times = times;

        if (nave > 0) {
            evoked.data = sumData / static_cast<double>(nave);
        } else {
            evoked.data = MatrixXd::Zero(nchan, ns);
        }

        evoked.info = raw.info;

        evokedSet.evoked.append(evoked);
        log += QString("    nave = %1\n").arg(nave);
    }

    return evokedSet;
}

//=============================================================================================================

bool FiffEvokedSet::checkArtifacts(const MatrixXd &epoch,
                                   const FiffInfo &info,
                                   const QStringList &bads,
                                   const RejectionParams &rej,
                                   QString &reason)
{
    for (int c = 0; c < epoch.rows(); ++c) {
        // Skip bad channels
        if (bads.contains(info.ch_names[c]))
            continue;

        double minVal = epoch.row(c).minCoeff();
        double maxVal = epoch.row(c).maxCoeff();
        double pp = maxVal - minVal;

        int chKind = info.chs[c].kind;
        int chUnit = info.chs[c].unit;

        if (chKind == FIFFV_MEG_CH) {
            if (chUnit == FIFF_UNIT_T) {
                // Magnetometer
                if (rej.megMagReject > 0 && pp > rej.megMagReject) {
                    reason = QString("%1 : %.1f fT > %.1f fT")
                        .arg(info.ch_names[c])
                        .arg(pp * 1e15).arg(rej.megMagReject * 1e15);
                    return false;
                }
                if (rej.megMagFlat > 0 && pp < rej.megMagFlat) {
                    reason = QString("%1 : %.1f fT < %.1f fT (flat)")
                        .arg(info.ch_names[c])
                        .arg(pp * 1e15).arg(rej.megMagFlat * 1e15);
                    return false;
                }
            } else {
                // Gradiometer
                if (rej.megGradReject > 0 && pp > rej.megGradReject) {
                    reason = QString("%1 : %.1f fT/cm > %.1f fT/cm")
                        .arg(info.ch_names[c])
                        .arg(pp * 1e13).arg(rej.megGradReject * 1e13);
                    return false;
                }
                if (rej.megGradFlat > 0 && pp < rej.megGradFlat) {
                    reason = QString("%1 : %.1f fT/cm < %.1f fT/cm (flat)")
                        .arg(info.ch_names[c])
                        .arg(pp * 1e13).arg(rej.megGradFlat * 1e13);
                    return false;
                }
            }
        } else if (chKind == FIFFV_EEG_CH) {
            if (rej.eegReject > 0 && pp > rej.eegReject) {
                reason = QString("%1 : %.1f uV > %.1f uV")
                    .arg(info.ch_names[c])
                    .arg(pp * 1e6).arg(rej.eegReject * 1e6);
                return false;
            }
            if (rej.eegFlat > 0 && pp < rej.eegFlat) {
                reason = QString("%1 : %.1f uV < %.1f uV (flat)")
                    .arg(info.ch_names[c])
                    .arg(pp * 1e6).arg(rej.eegFlat * 1e6);
                return false;
            }
        } else if (chKind == FIFFV_EOG_CH) {
            if (rej.eogReject > 0 && pp > rej.eogReject) {
                reason = QString("%1 : %.1f uV > %.1f uV (EOG)")
                    .arg(info.ch_names[c])
                    .arg(pp * 1e6).arg(rej.eogReject * 1e6);
                return false;
            }
            if (rej.eogFlat > 0 && pp < rej.eogFlat) {
                reason = QString("%1 : EOG flat").arg(info.ch_names[c]);
                return false;
            }
        } else if (chKind == FIFFV_ECG_CH) {
            if (rej.ecgReject > 0 && pp > rej.ecgReject) {
                reason = QString("%1 : %.2f mV > %.2f mV (ECG)")
                    .arg(info.ch_names[c])
                    .arg(pp * 1e3).arg(rej.ecgReject * 1e3);
                return false;
            }
            if (rej.ecgFlat > 0 && pp < rej.ecgFlat) {
                reason = QString("%1 : ECG flat").arg(info.ch_names[c]);
                return false;
            }
        }
    }
    return true;
}
