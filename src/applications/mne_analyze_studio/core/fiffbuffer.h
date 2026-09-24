//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fiffbuffer.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     March, 2026
 * @version  dev
 * @brief    Declares the FIFF-backed buffer implementation used by MNE Analyze Studio.
 */

#ifndef MNE_ANALYZE_STUDIO_FIFFBUFFER_H
#define MNE_ANALYZE_STUDIO_FIFFBUFFER_H

#include "ibuffer.h"

#include <fiff/fiff.h>
#include <fiff/fiff_raw_data.h>

#include <QFile>

namespace MNEANALYZESTUDIO
{

/**
 * @brief Buffer implementation that exposes FIFF metadata and binary streaming access.
 */
class STUDIOCORESHARED_EXPORT FiffBuffer : public IBuffer
{
    Q_OBJECT

public:
    explicit FiffBuffer(const QString& filePath, QObject* parent = nullptr);

    BufferKind kind() const override;
    QString uri() const override;
    bool open() override;
    bool isOpen() const override;
    QJsonObject getMetadata() const override;
    QIODevice* device() const override;
    QDataStream* getBinaryStream() override;

    const FIFFLIB::FiffRawData& rawData() const;

private:
    void loadHeaderMetadata();

    QString m_filePath;
    mutable QJsonObject m_metadata;
    QFile m_file;
    FIFFLIB::FiffStream::SPtr m_stream;
    FIFFLIB::FiffRawData m_rawData;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_FIFFBUFFER_H
