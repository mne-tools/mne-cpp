//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     mne_epoch_data_list.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>
 * @since    0.1.0
 * @date     October 2012
 * @brief    Implementation of @ref mne::MNEEpochDataList.
 *
 * Implements list-level operations: appending epochs, applying SSP and
 * baseline, averaging by event id, dropping rejected trials and writing
 * the result as an @c -epo.fif file.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_epoch_data_list.h"

#include <fiff/fiff_ch_name.h>
#include <base/log.h>
#include <base/string_utils.h>
#include <fiff/fiff_evoked_set.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <memory>
#include <vector>
#include <utility>
#include <format>
#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEEpochDataList::MNEEpochDataList()
{
}

//=============================================================================================================

MNEEpochDataList::~MNEEpochDataList()
{
    //    MNEEpochDataList::iterator i;
    //    for( i = this->begin(); i!=this->end(); ++i) {
    //        if (*i)
    //            delete (*i);
    //    }
}

//=============================================================================================================

MNEEpochDataList MNEEpochDataList::readEpochs(const FiffRawData& raw,
                                              const MatrixXi& events,
                                              float tmin,
                                              float tmax,
                                              std::int32_t event,
                                              const std::map<std::string, double>& mapReject,
                                              const StringList& lExcludeChs,
                                              const RowVectorXi& picks)
{
    MNEEpochDataList data;

    // Select the desired events
    std::int32_t count = 0;
    std::int32_t p;
    MatrixXi selected = MatrixXi::Zero(1, events.rows());
    for (p = 0; p < events.rows(); ++p) {
        if (events(p, 1) == 0 && events(p, 2) == event) {
            selected(0, count) = p;
            ++count;
        }
    }
    selected.conservativeResize(1, count);
    if (count > 0) {
        Log::info("[MNEEpochDataList::readEpochs] {} matching events found", count);
    } else {
        Log::warning("[MNEEpochDataList::readEpochs] No desired events found.");
        return MNEEpochDataList();
    }

    // If picks are empty, pick all
    RowVectorXi picksNew = picks;
    if (picks.cols() <= 0) {
        picksNew.resize(raw.info.chs.size());
        for (int i = 0; i < std::ssize(raw.info.chs); ++i) {
            picksNew(i) = i;
        }
    }

    fiff_int_t event_samp, from, to;
    fiff_int_t dropCount = 0;
    MatrixXd timesDummy;
    MatrixXd times;

    std::unique_ptr<MNEEpochData> epoch(nullptr);

    for (p = 0; p < count; ++p) {
        // Read a data segment
        event_samp = events(selected(p), 0);
        // Like mne.Epochs: both ends rounded to the nearest sample
        from = event_samp + static_cast<fiff_int_t>(std::lround(tmin * raw.info.sfreq));
        to = event_samp + static_cast<fiff_int_t>(std::lround(tmax * raw.info.sfreq));

        epoch.reset(new MNEEpochData());

        if (raw.read_raw_segment(epoch->epoch, timesDummy, from, to, picksNew)) {
            if (p == 0) {
                times.resize(1, to - from + 1);
                for (std::int32_t i = 0; i < times.cols(); ++i)
                    times(0, i) = static_cast<float>(from - event_samp + i) / raw.info.sfreq;
            }

            epoch->event = event;
            epoch->eventSample = event_samp;
            epoch->tmin = tmin;
            epoch->tmax = tmax;

            epoch->bReject = checkForArtifact(epoch->epoch,
                                              raw.info,
                                              mapReject,
                                              lExcludeChs);

            if (epoch->bReject) {
                dropCount++;
            }

            //Check if data block has the same size as the previous one
            if (!data.empty()) {
                if (epoch->epoch.size() == data.back()->epoch.size()) {
                    data.push_back(std::shared_ptr<MNEEpochData>(epoch.release())); //List takes ownwership of the pointer - no delete need
                }
            } else {
                data.push_back(std::shared_ptr<MNEEpochData>(epoch.release())); //List takes ownwership of the pointer - no delete need
            }
        } else {
            Log::warning("[MNEEpochDataList::readEpochs] Can't read the event data segments.");
        }
    }

    Log::info("[MNEEpochDataList::readEpochs] Read a total of {} epochs of type {} and marked {} for rejection.", data.size(), event, dropCount);

    return data;
}

//=============================================================================================================

FiffEvoked MNEEpochDataList::average(const FiffInfo& info,
                                     fiff_int_t first,
                                     fiff_int_t last,
                                     VectorXi sel,
                                     bool proj) const
{
    FiffEvoked p_evoked;

    Log::info("[MNEEpochDataList::average] Calculate evoked. ");

    MatrixXd matAverage;

    if (this->size() > 0) {
        matAverage = MatrixXd::Zero(this->at(0)->epoch.rows(), this->at(0)->epoch.cols());
    } else {
        p_evoked.aspect_kind = FIFFV_ASPECT_STD_ERR;
        return p_evoked;
    }

    if (sel.size() > 0) {
        p_evoked.nave = sel.size();

        for (Eigen::Index i = 0; i < sel.size(); ++i) {
            matAverage.array() += this->at(static_cast<std::size_t>(sel(i)))->epoch.array();
        }
    } else {
        p_evoked.nave = static_cast<int>(this->size());

        for (const std::shared_ptr<MNEEpochData>& epoch : *this) {
            matAverage.array() += epoch->epoch.array();
        }
    }
    matAverage.array() /= p_evoked.nave;

    Log::info("[MNEEpochDataList::average] {} averages used [done]", p_evoked.nave);

    p_evoked.setInfo(info, proj);

    p_evoked.aspect_kind = FIFFV_ASPECT_AVERAGE;

    p_evoked.first = first;
    p_evoked.last = last;

    // Sample times relative to the event, as in readEpochs (and mne.Epochs.times)
    const long firstSample = std::lround(this->front()->tmin * info.sfreq);
    p_evoked.times.resize(this->front()->epoch.cols());
    for (Eigen::Index i = 0; i < p_evoked.times.size(); ++i) {
        p_evoked.times[i] = static_cast<float>((firstSample + i) / info.sfreq);
    }

    p_evoked.comment = std::to_string(this->front()->event);

    if (p_evoked.proj.rows() > 0) {
        matAverage = p_evoked.proj * matAverage;
        Log::info("[MNEEpochDataList::average] SSP projectors applied to the evoked data");
    }

    p_evoked.data = matAverage;

    return p_evoked;
}

//=============================================================================================================

void MNEEpochDataList::applyBaselineCorrection(const std::pair<float, float>& baseline)
{
    // Run baseline correction
    for (const std::shared_ptr<MNEEpochData>& epoch : *this) {
        epoch->applyBaselineCorrection(baseline);
    }
}

//=============================================================================================================

void MNEEpochDataList::dropRejected()
{
    std::erase_if(*this, [](const std::shared_ptr<MNEEpochData>& epoch) { return epoch->isRejected(); });
}

//=============================================================================================================

void MNEEpochDataList::pick_channels(const RowVectorXi& sel)
{
    for (const std::shared_ptr<MNEEpochData>& epoch : *this) {
        epoch->pick_channels(sel);
    }
}

//=============================================================================================================

bool MNEEpochDataList::checkForArtifact(const MatrixXd& data,
                                        const FiffInfo& pFiffInfo,
                                        const std::map<std::string, double>& mapReject,
                                        const StringList& lExcludeChs)
{
    bool bReject = false;

    //Prepare concurrent data handling
    std::vector<ArtifactRejectionData> lchData;
    if (mapReject.empty()) {
        return bReject;
    }

    for (int i = 0; i < std::ssize(pFiffInfo.chs); ++i) {
        const FiffChInfo& ch = pFiffInfo.chs.at(i);
        const char* key = nullptr;
        if (ch.kind == FIFFV_MEG_CH && ch.unit == FIFF_UNIT_T) {
            key = "mag";
        } else if (ch.kind == FIFFV_MEG_CH && ch.unit == FIFF_UNIT_T_M) {
            key = "grad";
        } else if (ch.kind == FIFFV_EEG_CH) {
            key = "eeg";
        } else if (ch.kind == FIFFV_EOG_CH) {
            key = "eog";
        }
        // Like mne.Epochs(reject=...): only channel types with a limit are checked
        const auto limit = key ? mapReject.find(key) : mapReject.end();
        if (limit == mapReject.end() || mne::has_channel(lExcludeChs, ch.ch_name) || mne::has_channel(pFiffInfo.bads, ch.ch_name) || ch.chpos.coil_type == FIFFV_COIL_BABY_REF_MAG || ch.chpos.coil_type == FIFFV_COIL_BABY_REF_MAG2) {
            continue;
        }
        ArtifactRejectionData tempData;
        tempData.data = data.row(i);
        tempData.dThreshold = limit->second;
        tempData.sChName = ch.ch_name;
        lchData.push_back(tempData);
    }

    if (lchData.empty()) {
        Log::warning("[MNEEpochDataList::checkForArtifact] No channels found to scan for artifacts. Do not reject. Returning.");

        return bReject;
    }


    for (ArtifactRejectionData& channel : lchData) {
        checkChThreshold(channel);
        if (channel.bRejected) {
            bReject = true;
            Log::info("[MNEEpochDataList::checkForArtifact] Reject trial because of channel {}", channel.sChName);
            break;
        }
    }

    return bReject;
}

//=============================================================================================================

void MNEEpochDataList::checkChThreshold(ArtifactRejectionData& inputData)
{
    RowVectorXd temp = inputData.data;

    // Remove offset
    //temp = temp.array() - temp(0);

    double min = temp.minCoeff();
    double max = temp.maxCoeff();

    // Peak to Peak
    double pp = max - min;

    if (std::fabs(pp) > inputData.dThreshold) {
        inputData.bRejected = true;
    } else {
        inputData.bRejected = false;
    }


    //    //If absolute vaue of min or max if bigger than threshold -> reject
    //    if((std::fabs(min) > inputData.dThreshold) || (std::fabs(max) > inputData.dThreshold)) {
    //        inputData.bRejected = true;
    //    } else {
    //        inputData.bRejected = false;
    //    }
}

//=============================================================================================================

FiffEvokedSet MNEEpochDataList::averageCategories(const FiffRawData& raw,
                                                  const MatrixXi& events,
                                                  const std::vector<int>& eventCodes,
                                                  const StringList& comments,
                                                  float tmin,
                                                  float tmax,
                                                  const std::map<std::string, double>& mapReject,
                                                  const std::pair<float, float>& baseline,
                                                  bool proj)
{
    FiffEvokedSet evokedSet;
    evokedSet.info = raw.info;

    float sfreq = raw.info.sfreq;

    bool doBaseline = (baseline.first != baseline.second);

    // Process each category (event code)
    for (int j = 0; j < std::ssize(eventCodes); ++j) {
        int eventCode = eventCodes[j];
        const std::string comment = (j < std::ssize(comments)) ? comments[static_cast<std::size_t>(j)]
                                                               : std::format("cat_{}", eventCode);

        // Read epochs for this event code using the existing readEpochs
        MNEEpochDataList epochList = MNEEpochDataList::readEpochs(raw, events,
                                                                  tmin, tmax,
                                                                  eventCode,
                                                                  mapReject);

        if (epochList.empty()) {
            Log::warning("[MNEEpochDataList::averageCategories] No epochs found for event {} - skipping category.", eventCode);
            continue;
        }

        // Apply baseline correction
        if (doBaseline) {
            epochList.applyBaselineCorrection(baseline);
        }

        // Drop rejected epochs
        epochList.dropRejected();

        if (epochList.empty()) {
            Log::warning("[MNEEpochDataList::averageCategories] All epochs rejected for event {} - skipping category.", eventCode);
            continue;
        }

        // Compute the average
        int minSamp = static_cast<int>(std::round(tmin * sfreq));
        int maxSamp = static_cast<int>(std::round(tmax * sfreq));

        FiffEvoked evoked = epochList.average(raw.info,
                                              minSamp,
                                              maxSamp,
                                              defaultVectorXi,
                                              proj);

        evoked.comment = comment;
        evoked.baseline = doBaseline ? baseline : std::pair<float, float>(0.0f, 0.0f);
        evokedSet.evoked.push_back(evoked);
    }

    return evokedSet;
}

//=============================================================================================================

FiffEvoked MNEEpochDataList::computeAverage(const FiffRawData& raw,
                                            const MatrixXi& matEvents,
                                            float fTMinS,
                                            float fTMaxS,
                                            std::int32_t eventType,
                                            bool bApplyBaseline,
                                            float fTBaselineFromS,
                                            float fTBaselineToS,
                                            const std::map<std::string, double>& mapReject,
                                            const StringList& lExcludeChs,
                                            const RowVectorXi& picks)
{
    MNEEpochDataList lstEpochDataList = MNEEpochDataList::readEpochs(raw,
                                                                     matEvents,
                                                                     fTMinS,
                                                                     fTMaxS,
                                                                     eventType,
                                                                     mapReject,
                                                                     lExcludeChs,
                                                                     picks);

    if (bApplyBaseline) {
        std::pair<float, float> baselinePair(fTBaselineFromS, fTBaselineToS);
        lstEpochDataList.applyBaselineCorrection(baselinePair);
    }

    if (!mapReject.empty()) {
        lstEpochDataList.dropRejected();
    }

    if (lstEpochDataList.empty())
        return FiffEvoked();

    const fiff_int_t first = static_cast<fiff_int_t>(std::lround(fTMinS * raw.info.sfreq));
    FiffEvoked evoked = lstEpochDataList.average(raw.info,
                                                 first,
                                                 first + static_cast<fiff_int_t>(lstEpochDataList.front()->epoch.cols()) - 1);
    evoked.baseline = bApplyBaseline ? std::pair<float, float>(fTBaselineFromS, fTBaselineToS)
                                     : std::pair<float, float>(0.0f, 0.0f);
    return evoked;
}
