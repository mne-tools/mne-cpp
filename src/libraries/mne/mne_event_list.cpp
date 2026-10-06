//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_event_list.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Implementation of @ref MNELIB::MNEEventList (MNE-C mne_events.c).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_event_list.h"

#include <fiff/fiff_constants.h>
#include <fiff/fiff_dir_node.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

std::optional<MNEEventList> MNEEventList::readFif(const QString& path, int offset)
{
    QFile file(path);
    FiffStream::SPtr stream(new FiffStream(&file));
    if (!stream->open()) {
        return std::nullopt;
    }
    const QList<FiffDirNode::SPtr> blocks = stream->dirtree()->dir_tree_find(FIFFB_MNE_EVENTS);
    FiffTag::UPtr tag;
    if (blocks.isEmpty() || !blocks[0]->find_tag(stream, FIFF_MNE_EVENT_LIST, tag)) {
        qWarning("MNEEventList::readFif - No event data in %s", qPrintable(path));
        return std::nullopt;
    }
    // Written as signed or unsigned integers; both have the same bit pattern.
    const auto* values = reinterpret_cast<const qint32*>(tag->data());
    const int nevent = static_cast<int>(tag->size() / (3 * sizeof(qint32)));
    MNEEventList list;
    list.events.resize(nevent);
    for (int k = 0; k < nevent; ++k) {
        list.events[k].sample = values[3 * k] - offset;
        list.events[k].from = static_cast<unsigned int>(values[3 * k + 1]);
        list.events[k].to = static_cast<unsigned int>(values[3 * k + 2]);
    }
    // The comments are one NUL-separated string, one entry per event.
    if (blocks[0]->find_tag(stream, FIFF_MNE_EVENT_COMMENTS, tag)) {
        const QList<QByteArray> comments = tag->split('\0');
        for (int k = 0; k < nevent && k < comments.size(); ++k) {
            list.events[k].comment = QString::fromLatin1(comments[k]);
        }
    }
    return list;
}

//=============================================================================================================

std::optional<MNEEventList> MNEEventList::readText(const QString& path, int offset, int oldOffset, float sfreq)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("MNEEventList::readText - Cannot open %s", qPrintable(path));
        return std::nullopt;
    }
    static const QRegularExpression whitespace(QStringLiteral("\\s+"));
    MNEEventList list;
    bool first = true;
    QTextStream in(&file);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }
        const QStringList fields = line.split(whitespace);
        bool ok[4];
        int sample = fields.value(0).toInt(&ok[0]);
        const float time = fields.value(1).toFloat(&ok[1]);
        const unsigned int from = fields.value(2).toUInt(&ok[2]);
        const unsigned int to = fields.value(3).toUInt(&ok[3]);
        if (fields.size() < 4 || !(ok[0] && ok[1] && ok[2] && ok[3])) {
            break;
        }
        if (sample < 0) {
            sample = static_cast<int>(time * sfreq);
        }
        if (first) {
            first = false;
            if (from == 0 && to == 0) {
                if (offset != sample) {
                    qWarning("MNEEventList::readText - First sample of the data (%d) and of %s (%d) differ", offset, qPrintable(path), sample);
                }
                continue;
            }
            offset -= oldOffset;
        }
        MNEEvent event;
        event.sample = sample - offset;
        event.from = from;
        event.to = to;
        event.comment = line.section(whitespace, 4).trimmed();
        list.events.push_back(event);
    }
    if (list.events.empty()) {
        qWarning("MNEEventList::readText - No events read from %s", qPrintable(path));
        return std::nullopt;
    }
    return list;
}

//=============================================================================================================

bool MNEEventList::writeFif(const QString& path, int offset) const
{
    QFile file(path);
    FiffStream::SPtr stream = FiffStream::start_file(file);
    if (!stream) {
        return false;
    }
    std::vector<qint32> values;
    values.reserve(3 * events.size());
    QByteArray comments;
    bool anyComment = false;
    for (const MNEEvent& event : events) {
        values.push_back(event.sample + offset);
        values.push_back(static_cast<qint32>(event.from));
        values.push_back(static_cast<qint32>(event.to));
        comments += event.comment.toLatin1() + '\0';
        anyComment = anyComment || !event.comment.isEmpty();
    }
    stream->start_block(FIFFB_MNE_EVENTS);
    // Unsigned like MNE-C; trigger values are bit masks.
    auto list = std::make_unique<FiffTag>();
    list->kind = FIFF_MNE_EVENT_LIST;
    list->type = FIFFT_UINT;
    list->next = FIFFV_NEXT_SEQ;
    const auto nbytes = static_cast<qsizetype>(values.size()) * static_cast<qsizetype>(sizeof(qint32));
    list->append(reinterpret_cast<const char*>(values.data()), nbytes);
    stream->write_tag(list);
    if (anyComment) {
        // Latin-1 bytes, one NUL-terminated comment per event, as MNE-C writes and mne-python reads them.
        auto tag = std::make_unique<FiffTag>();
        tag->kind = FIFF_MNE_EVENT_COMMENTS;
        tag->type = FIFFT_BYTE;
        tag->next = FIFFV_NEXT_SEQ;
        tag->append(comments);
        stream->write_tag(tag);
    }
    stream->end_block(FIFFB_MNE_EVENTS);
    stream->end_file();
    return true;
}

//=============================================================================================================

void MNEEventList::sort()
{
    std::stable_sort(events.begin(), events.end(), [](const MNEEvent& a, const MNEEvent& b) {
        return a.sample < b.sample;
    });
}

//=============================================================================================================

void MNEEventList::append(const MNEEventList& other)
{
    events.insert(events.end(), other.events.cbegin(), other.events.cend());
}

//=============================================================================================================

MNEEventList MNEEventList::selectOnsets(unsigned int value) const
{
    MNEEventList selected;
    std::copy_if(events.cbegin(), events.cend(), std::back_inserter(selected.events), [value](const MNEEvent& event) {
        return event.from == 0 && (value == 0 || event.to == value);
    });
    return selected;
}

//=============================================================================================================

unsigned int MNEEventList::maxOnset() const
{
    unsigned int maximum = 0;
    for (const MNEEvent& event : events) {
        if (event.from == 0) {
            maximum = std::max(maximum, event.to);
        }
    }
    return maximum;
}

//=============================================================================================================

Eigen::MatrixXi MNEEventList::toMatrix() const
{
    Eigen::MatrixXi matrix(nevent(), 3);
    for (int k = 0; k < nevent(); ++k) {
        matrix.row(k) << events[k].sample, static_cast<int>(events[k].from), static_cast<int>(events[k].to);
    }
    return matrix;
}

//=============================================================================================================

MNEEventList MNEEventList::fromMatrix(const Eigen::MatrixXi& matrix)
{
    MNEEventList list;
    list.events.resize(matrix.rows());
    for (Eigen::Index k = 0; k < matrix.rows(); ++k) {
        list.events[k].sample = matrix(k, 0);
        list.events[k].from = static_cast<unsigned int>(matrix(k, 1));
        list.events[k].to = static_cast<unsigned int>(matrix(k, 2));
    }
    return list;
}
