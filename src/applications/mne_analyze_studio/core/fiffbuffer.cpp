//=============================================================================================================
/**
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

    if(!m_file.open(QIODevice::ReadOnly)) {
        return false;
    }

    FIFFLIB::FiffStream::SPtr stream;
    if(!FIFFLIB::Fiff::open(m_file, stream)) {
        m_file.close();
        return false;
    }

    m_stream = stream;
    m_file.seek(0);
    m_rawData = FIFFLIB::FiffRawData(m_file);
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
