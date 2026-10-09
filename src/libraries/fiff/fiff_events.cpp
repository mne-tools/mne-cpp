//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fiff_events.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February 2026
 * @brief    Implementation of @ref FiffEvents: FIFFB_MNE_EVENTS read / write and the stim-channel event-detection path.
 *
 * Detection follows @c mne.find_events; the (sample, prev, new) triples
 * are written under @c FIFFB_MNE_EVENTS so ``-eve.fif'' files round-trip
 * through MNE-Python unchanged.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_events.h"
#include "fiff_evoked_set.h"
#include "fiff_raw_data.h"
#include "fiff_stream.h"
#include "fiff_dir_node.h"
#include "fiff_tag.h"
#include "fiff_constants.h"
#include "fiff_file.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QDebug>
#include <QRegularExpression>
#include <QTextStream>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffEvents::FiffEvents()
{
}

//=============================================================================================================

FiffEvents::FiffEvents(QIODevice& p_IODevice)
{
    // Try FIFF first, then ASCII
    if (!read_from_fif(p_IODevice, *this)) {
        // The FIFF attempt leaves the device open in binary mode.
        p_IODevice.close();
        read_from_ascii(p_IODevice, *this);
    }
}

//=============================================================================================================

bool FiffEvents::read(const QString& t_sEventName,
                      const QString& t_fileRawName,
                      FiffEvents& p_Events)
{
    QString eventName = t_sEventName;
    QFile t_EventFile;
    qint32 p;

    if (eventName.isEmpty()) {
        eventName = t_fileRawName;
        p = eventName.indexOf(".fif");
        if (p > 0) {
            eventName.replace(p, 4, "-eve.fif");
        } else {
            qWarning("Raw file name does not end properly\n");
            return false;
        }

        t_EventFile.setFileName(eventName);
        if (!read_from_fif(t_EventFile, p_Events)) {
            qWarning("Error while read events.\n");
            return false;
        }
        qInfo("Events read from %s\n", eventName.toUtf8().constData());
    } else {
        // Binary file
        if (eventName.contains(".fif")) {
            t_EventFile.setFileName(eventName);
            if (!read_from_fif(t_EventFile, p_Events)) {
                qWarning("Error while read events.\n");
                return false;
            }
            qInfo("Binary event file %s read\n", eventName.toUtf8().constData());
        } else if (eventName.contains(".eve")) {
        } else {
            // Text file
            qWarning("Text file %s is not supported jet.\n", eventName.toUtf8().constData());
        }
    }

    return true;
}

//=============================================================================================================

bool FiffEvents::read_from_fif(QIODevice& p_IODevice,
                               FiffEvents& p_Events)
{
    //
    // Open file
    //
    FiffStream::SPtr t_pStream(new FiffStream(&p_IODevice));

    if (!t_pStream->open()) {
        return false;
    }

    //
    //   Find the desired block
    //
    QList<FiffDirNode::SPtr> eventsBlocks = t_pStream->dirtree()->dir_tree_find(FIFFB_MNE_EVENTS);

    if (eventsBlocks.size() == 0) {
        qWarning("Could not find event data\n");
        return false;
    }

    // nelem is only assigned when a matching tag is found; the guard below
    // returns before it is read in that case, but it must not start out
    // indeterminate.
    qint32 k, nelem = 0;
    fiff_int_t kind, pos;
    FiffTag::UPtr t_pTag;
    quint32* serial_eventlist_uint = nullptr;
    qint32* serial_eventlist_int = nullptr;

    for (k = 0; k < eventsBlocks[0]->nent(); ++k) {
        kind = eventsBlocks[0]->dir[k]->kind;
        pos = eventsBlocks[0]->dir[k]->pos;
        if (kind == FIFF_MNE_EVENT_LIST) {
            t_pStream->read_tag(t_pTag, pos);
            if (t_pTag->type == FIFFT_UINT) {
                serial_eventlist_uint = t_pTag->toUnsignedInt();
                nelem = t_pTag->size() / 4;
            }

            if (t_pTag->type == FIFFT_INT) {
                serial_eventlist_int = t_pTag->toInt();
                nelem = t_pTag->size() / 4;
            }

            break;
        }
    }

    if (serial_eventlist_uint == nullptr && serial_eventlist_int == nullptr) {
        qWarning("Could not find any events\n");
        return false;
    }

    p_Events.events.resize(nelem / 3, 3);
    if (serial_eventlist_uint != nullptr) {
        for (k = 0; k < nelem / 3; ++k) {
            p_Events.events(k, 0) = serial_eventlist_uint[k * 3];
            p_Events.events(k, 1) = serial_eventlist_uint[k * 3 + 1];
            p_Events.events(k, 2) = serial_eventlist_uint[k * 3 + 2];
        }
    }

    if (serial_eventlist_int != nullptr) {
        for (k = 0; k < nelem / 3; ++k) {
            p_Events.events(k, 0) = serial_eventlist_int[k * 3];
            p_Events.events(k, 1) = serial_eventlist_int[k * 3 + 1];
            p_Events.events(k, 2) = serial_eventlist_int[k * 3 + 2];
        }
    }

    return true;
}

//=============================================================================================================

bool FiffEvents::read_from_ascii(QIODevice& p_IODevice,
                                 FiffEvents& p_Events)
{
    if (!p_IODevice.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    QTextStream textStream(&p_IODevice);

    QList<int> sampleList;
    QList<int> beforeList;
    QList<int> afterList;

    while (!textStream.atEnd()) {
        QString line = textStream.readLine().trimmed();
        if (line.isEmpty())
            continue;

        // A standard .eve line has four fields: sample, onset in seconds,
        // value before and value after the transition. The onset is redundant
        // with the sample and is skipped, which is what write_to_ascii emits.
        // Reading it as an integer used to shift every later field along by
        // one, so the trigger code was lost and the event looked like it had
        // a code of zero. Files carrying only the three integer fields are
        // still accepted.
        const QStringList fields = line.split(QRegularExpression("\\s+"),
                                              Qt::SkipEmptyParts);
        if (fields.isEmpty())
            continue;

        const int iOffset = (fields.size() >= 4) ? 1 : 0;

        int iSample = fields.at(0).toInt();
        int iBefore = (fields.size() > 1 + iOffset) ? fields.at(1 + iOffset).toInt() : 0;
        int iAfter = (fields.size() > 2 + iOffset) ? fields.at(2 + iOffset).toInt() : 0;

        sampleList.append(iSample);
        beforeList.append(iBefore);
        afterList.append(iAfter);
    }

    p_Events.events.resize(sampleList.size(), 3);

    for (int i = 0; i < sampleList.size(); i++) {
        p_Events.events(i, 0) = sampleList[i];
        p_Events.events(i, 1) = beforeList[i];
        p_Events.events(i, 2) = afterList[i];
    }
    return true;
}

//=============================================================================================================

bool FiffEvents::write_to_fif(QIODevice& p_IODevice) const
{
    if (events.rows() == 0 || events.cols() < 3)
        return false;

    FiffStream::SPtr pStream = FiffStream::start_file(p_IODevice);
    if (!pStream)
        return false;

    // FIFF stores the list row by row (sample, before, after).
    const Eigen::Matrix<int, Eigen::Dynamic, 3, Eigen::RowMajor> rows = events.leftCols(3);
    pStream->start_block(FIFFB_MNE_EVENTS);
    pStream->write_int(FIFF_MNE_EVENT_LIST, rows.data(), rows.rows() * 3);
    pStream->end_block(FIFFB_MNE_EVENTS);
    pStream->end_file();

    return true;
}

//=============================================================================================================

bool FiffEvents::write_to_ascii(QIODevice& p_IODevice,
                                float sfreq) const
{
    if (!p_IODevice.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    for (int k = 0; k < events.rows(); ++k) {
        int sample = events(k, 0);
        int before = (events.cols() > 1) ? events(k, 1) : 0;
        int after = (events.cols() > 2) ? events(k, 2) : 0;
        float time = (sfreq > 0.0f) ? static_cast<float>(sample) / sfreq : 0.0f;
        QTextStream out(&p_IODevice);
        out << QString("%1 %2 %3 %4\n")
                   .arg(sample, 6)
                   .arg(time, -10, 'f', 3)
                   .arg(before, 3)
                   .arg(after, 3);
    }

    p_IODevice.close();
    return true;
}

//=============================================================================================================

bool FiffEvents::detect_from_raw(const FiffRawData& raw,
                                 FiffEvents& p_Events,
                                 const QString& triggerCh,
                                 unsigned int triggerMask,
                                 bool leadingEdge)
{
    QString stimCh = triggerCh.isEmpty() ? QString("STI 014") : triggerCh;

    // Channel names are read without spaces ("STI014"), so compare without them.
    const QString wanted = QString(stimCh).remove(' ');
    int triggerChIdx = -1;
    for (int k = 0; k < raw.info.ch_names.size(); ++k) {
        if (QString(raw.info.ch_names[k]).remove(' ') == wanted) {
            triggerChIdx = k;
            break;
        }
    }
    if (triggerChIdx < 0) {
        qWarning() << "[FiffEvents::detect_from_raw] Trigger channel" << stimCh << "not found.";
        return false;
    }

    // Read trigger channel data
    MatrixXd data;
    MatrixXd times;
    if (!raw.read_raw_segment(data, times, raw.first_samp, raw.last_samp,
                              RowVectorXi::LinSpaced(1, triggerChIdx, triggerChIdx))) {
        qWarning() << "[FiffEvents::detect_from_raw] Could not read trigger channel data.";
        return false;
    }

    RowVectorXd trigData = data.row(0);
    int nSamples = static_cast<int>(trigData.cols());

    // Detect flanks
    QList<int> eventSamples;
    QList<int> eventBefore;
    QList<int> eventAfter;

    int prevVal = static_cast<int>(trigData(0)) & triggerMask;
    for (int s = 1; s < nSamples; ++s) {
        int curVal = static_cast<int>(trigData(s)) & triggerMask;
        if (curVal != prevVal) {
            if (!leadingEdge || (leadingEdge && prevVal == 0 && curVal != 0)) {
                eventSamples.append(static_cast<int>(raw.first_samp) + s);
                eventBefore.append(prevVal);
                eventAfter.append(curVal);
            }
        }
        prevVal = curVal;
    }

    int nEvents = eventSamples.size();
    p_Events.events.resize(nEvents, 3);
    for (int k = 0; k < nEvents; ++k) {
        p_Events.events(k, 0) = eventSamples[k];
        p_Events.events(k, 1) = eventBefore[k];
        p_Events.events(k, 2) = eventAfter[k];
    }

    return nEvents > 0;
}

//=============================================================================================================

bool FiffEvents::matchEvent(const AverageCategory& cat,
                            const MatrixXi& events,
                            int eventIdx)
{
    if (eventIdx < 0 || eventIdx >= events.rows())
        return false;

    int evFrom = events(eventIdx, 1);
    int evTo = events(eventIdx, 2);

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

namespace
{

/** The rows of @p events whose flag is set, in order. */
MatrixXi selectRows(const MatrixXi& events, const std::vector<bool>& keep)
{
    MatrixXi out(std::count(keep.cbegin(), keep.cend(), true), events.cols());
    Index r = 0;
    for (Index i = 0; i < events.rows(); ++i) {
        if (keep[i])
            out.row(r++) = events.row(i);
    }
    return out;
}

} // namespace

//=============================================================================================================

FiffEvents FiffEvents::pick(const QList<int>& include, const QList<int>& exclude, bool step) const
{
    std::vector<bool> keep(events.rows());
    for (Index i = 0; i < events.rows(); ++i) {
        const bool matches = !include.isEmpty()
            ? include.contains(events(i, 2)) || (step && include.contains(events(i, 1)))
            : !(exclude.contains(events(i, 2)) || (step && exclude.contains(events(i, 1))));
        keep[i] = matches;
    }
    FiffEvents out;
    out.events = selectRows(events, keep);
    return out;
}

//=============================================================================================================

FiffEvents FiffEvents::merge(const QList<int>& ids, int newId, bool replaceEvents) const
{
    FiffEvents out;
    out.events = events;
    std::vector<bool> touched(events.rows(), false);
    for (Index i = 0; i < events.rows(); ++i) {
        for (int col = 1; col <= 2; ++col) {
            if (ids.contains(events(i, col))) {
                out.events(i, col) = newId;
                touched[i] = true;
            }
        }
    }
    if (!replaceEvents) {
        const MatrixXi originals = selectRows(events, touched);
        MatrixXi all(out.events.rows() + originals.rows(), 3);
        all << out.events, originals;
        std::vector<Index> order(all.rows());
        std::iota(order.begin(), order.end(), 0);
        std::stable_sort(order.begin(), order.end(), [&all](Index a, Index b) {
            return std::lexicographical_compare(all.row(a).begin(), all.row(a).end(), all.row(b).begin(), all.row(b).end());
        });
        out.events.resize(all.rows(), 3);
        for (Index i = 0; i < all.rows(); ++i)
            out.events.row(i) = all.row(order[i]);
    }
    return out;
}

//=============================================================================================================

QMap<int, int> FiffEvents::count(const QList<int>& ids) const
{
    QMap<int, int> counts;
    for (const int id : ids)
        counts.insert(id, 0);
    for (Index i = 0; i < events.rows(); ++i) {
        if (ids.isEmpty() || ids.contains(events(i, 2)))
            ++counts[events(i, 2)];
    }
    return counts;
}

//=============================================================================================================

FiffEvents FiffEvents::concatenate(const QList<FiffEvents>& events, const QList<int>& firstSamps, const QList<int>& lastSamps)
{
    FiffEvents out;
    if (events.isEmpty() || events.size() != firstSamps.size() || events.size() != lastSamps.size())
        return out;
    Index rows = 0;
    for (const FiffEvents& e : events)
        rows += e.events.rows();
    out.events.resize(rows, 3);
    Index r = 0;
    int offset = 0;
    for (qsizetype k = 0; k < events.size(); ++k) {
        MatrixXi shifted = events[k].events;
        if (k > 0)
            shifted.col(0).array() += offset + firstSamps[0] - firstSamps[k];
        out.events.middleRows(r, shifted.rows()) = shifted;
        r += shifted.rows();
        offset += lastSamps[k] - firstSamps[k] + 1;
    }
    return out;
}

//=============================================================================================================

FiffEvents FiffEvents::find_stim_steps(const MatrixXi& stimData, int firstSamp, std::optional<int> padStart, std::optional<int> padStop, int merge)
{
    std::vector<RowVector3i> steps;
    for (Index t = 0; t + 1 < stimData.cols(); ++t) {
        if ((stimData.col(t + 1).array() != stimData.col(t).array()).all())
            steps.emplace_back(static_cast<int>(t + 1) + firstSamp, stimData(0, t), stimData(0, t + 1));
    }
    FiffEvents out;
    if (steps.empty())
        return out;
    if (padStart && steps.front()(1) != *padStart)
        steps.insert(steps.begin(), RowVector3i(0, *padStart, steps.front()(1)));
    if (padStop && steps.back()(2) != *padStop)
        steps.emplace_back(static_cast<int>(stimData.cols()) + firstSamp, steps.back()(2), *padStop);
    if (merge != 0) {
        const size_t n = steps.size();
        std::vector<bool> close(n > 0 ? n - 1 : 0);
        for (size_t i = 0; i + 1 < n; ++i)
            close[i] = steps[i + 1](0) - steps[i](0) <= std::abs(merge);
        if (std::find(close.cbegin(), close.cend(), true) != close.cend()) {
            // Merged steps take the outer values; the step that is merged away is dropped
            std::vector<bool> keep(n, true);
            const std::vector<RowVector3i> original = steps;
            for (size_t i = 0; i + 1 < n; ++i) {
                if (!close[i])
                    continue;
                if (merge > 0) {
                    steps[i + 1](1) = original[i](1);
                    keep[i] = false;
                } else {
                    steps[i](2) = original[i + 1](2);
                    keep[i + 1] = false;
                }
            }
            std::vector<RowVector3i> kept;
            for (size_t i = 0; i < n; ++i) {
                if (keep[i] && steps[i](1) != steps[i](2))
                    kept.push_back(steps[i]);
            }
            steps.swap(kept);
        }
    }
    out.events.resize(static_cast<Index>(steps.size()), 3);
    for (size_t i = 0; i < steps.size(); ++i)
        out.events.row(static_cast<Index>(i)) = steps[i];
    return out;
}

//=============================================================================================================

FiffEvents FiffEvents::make_fixed_length(const FiffRawData& raw, int id, double start, double stop, double duration, bool firstSamp, double overlap)
{
    FiffEvents out;
    const double sfreq = raw.info.sfreq;
    if (overlap < 0.0 || overlap >= duration || sfreq <= 0.0)
        return out;
    double first = std::nearbyint(start * sfreq);
    double last = stop >= 0.0 ? std::nearbyint(stop * sfreq) : raw.last_samp + 1.0;
    if (firstSamp) {
        first += raw.first_samp;
        last = std::min(last + (stop >= 0.0 ? raw.first_samp : 0), raw.last_samp + 1.0);
    } else {
        last = std::min(last - (stop >= 0.0 ? 0 : raw.first_samp), static_cast<double>(raw.last_samp - raw.first_samp + 1));
    }
    last -= std::nearbyint(sfreq * duration);
    const double stepSamples = sfreq * (duration - overlap);
    const auto count = static_cast<Index>(std::max(0.0, std::ceil((last + 1.0 - first) / stepSamples)));
    out.events = MatrixXi::Zero(count, 3);
    for (Index i = 0; i < count; ++i) {
        out.events(i, 0) = static_cast<int>(first + static_cast<double>(i) * stepSamples);
        out.events(i, 2) = id;
    }
    return out;
}
