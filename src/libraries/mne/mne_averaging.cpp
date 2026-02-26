//=============================================================================================================
/**
 * @file     mne_averaging.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     February, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * @brief    MNEAveraging class implementation.
 *           Ported from average.c by Matti Hamalainen.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_averaging.h"

#include <cmath>
#include <QFile>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

bool MNEAveraging::matchEvent(const AverageCategory &cat,
                              const MatrixXi &events,
                              int eventIdx)
{
    if (eventIdx < 0 || eventIdx >= events.rows())
        return false;

    int evFrom = events(eventIdx, 1);
    int evTo   = events(eventIdx, 2);

    // Check if any of the category's event codes match
    bool match = false;
    for (int k = 0; k < cat.events.size(); ++k) {
        if ((evFrom & ~cat.ignore) == 0 &&
            (evTo & ~cat.ignore) == cat.events[k]) {
            match = true;
            break;
        }
    }
    if (!match)
        return false;

    // Check previous event constraint
    if (cat.prevEvent != 0) {
        bool found = false;
        for (int j = eventIdx - 1; j >= 0; --j) {
            if ((events(j, 1) & ~cat.prevIgnore) == 0) {
                found = true;
                match = match && ((events(j, 2) & ~cat.prevIgnore) == cat.prevEvent);
                break;
            }
        }
        if (!found)
            match = false;
    }

    // Check next event constraint
    if (cat.nextEvent != 0) {
        bool found = false;
        for (int j = eventIdx + 1; j < events.rows(); ++j) {
            if ((events(j, 1) & ~cat.nextIgnore) == 0) {
                found = true;
                match = match && ((events(j, 2) & ~cat.nextIgnore) == cat.nextEvent);
                break;
            }
        }
        if (!found)
            match = false;
    }

    return match;
}

//=============================================================================================================

bool MNEAveraging::checkArtifacts(const MatrixXd &epoch,
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

//=============================================================================================================

void MNEAveraging::subtractBaseline(MatrixXd &epoch, int bminSamp, int bmaxSamp)
{
    if (bminSamp < 0) bminSamp = 0;
    if (bmaxSamp >= epoch.cols()) bmaxSamp = static_cast<int>(epoch.cols()) - 1;
    if (bminSamp >= bmaxSamp) return;

    int nBase = bmaxSamp - bminSamp;
    for (int c = 0; c < epoch.rows(); ++c) {
        double baseVal = epoch.row(c).segment(bminSamp, nBase).mean();
        epoch.row(c).array() -= baseVal;
    }
}

//=============================================================================================================

FiffEvokedSet MNEAveraging::computeAverages(const FiffRawData &raw,
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
            if (!matchEvent(cat, events, k))
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
