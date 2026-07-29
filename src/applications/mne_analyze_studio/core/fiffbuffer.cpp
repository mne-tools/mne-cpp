//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *

 * @file     fiffbuffer.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements the FIFF-backed studio buffer.
 */

#include "fiffbuffer.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QFileInfo>

using namespace MNEANALYZESTUDIO;

FiffBuffer::FiffBuffer(const QString& filePath, QObject* parent)
: IBuffer(parent)
, m_filePath(filePath)
, m_file(filePath)
{
}

IBuffer::BufferKind FiffBuffer::kind() const
{
    return BufferKind::Fiff;
}

QString FiffBuffer::uri() const
{
    return m_filePath;
}

bool FiffBuffer::open()
{
    if(m_stream) {
        return true;
    }

    // This used to open m_file, then hand it to Fiff::open, then hand it to
    // FiffRawData, and each of those opens the device itself. The second open
    // failed on an already open QFile, so no file ever loaded through here and
    // the third attempt threw. Only FiffRawData is needed: setup_read_raw opens
    // the device and leaves the stream on the object.
    if(!m_file.exists()) {
        return false;
    }

    if(m_file.isOpen()) {
        m_file.close();
    }

    try {
        m_rawData = FIFFLIB::FiffRawData(m_file);
    } catch(const std::exception& e) {
        // FiffRawData reports a bad file by throwing. A buffer that cannot be
        // read is an ordinary outcome for a caller handed an arbitrary path,
        // so it is turned back into a false rather than unwinding into the UI.
        qWarning() << "[FiffBuffer::open] could not read" << m_filePath << ":" << e.what();
        if(m_file.isOpen()) {
            m_file.close();
        }
        return false;
    }

    m_stream = m_rawData.file;
    loadHeaderMetadata();
    emit metadataChanged(m_metadata);

    return true;
}

bool FiffBuffer::isOpen() const
{
    return m_stream != nullptr;
}

QJsonObject FiffBuffer::getMetadata() const
{
    return m_metadata;
}

QIODevice* FiffBuffer::device() const
{
    return const_cast<QFile*>(&m_file);
}

QDataStream* FiffBuffer::getBinaryStream()
{
    return m_stream.data();
}

const FIFFLIB::FiffRawData& FiffBuffer::rawData() const
{
    return m_rawData;
}

void FiffBuffer::loadHeaderMetadata()
{
    const FIFFLIB::FiffInfo& info = m_rawData.info;

    QJsonArray channels;
    for(const QString& channelName : info.ch_names) {
        channels.append(channelName);
    }

    QJsonObject bidsInfo;
    bidsInfo.insert("subject", QFileInfo(m_filePath).baseName());
    bidsInfo.insert("source", m_filePath);

    m_metadata = QJsonObject{
        {"bufferKind", "fiff"},
        {"uri", m_filePath},
        {"channelCount", info.nchan},
        {"channels", channels},
        {"samplingRate", info.sfreq},
        {"bids", bidsInfo}
    };
}
