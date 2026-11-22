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

#include <mne/fiff/fiff.h>
#include <mne/fiff/fiff_raw_data.h>

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

    const mne::FiffRawData& rawData() const;

private:
    void loadHeaderMetadata();

    QString m_filePath;
    mutable QJsonObject m_metadata;
    QFile m_file;
    mne::FiffStream* m_stream = nullptr; // owned by m_rawData
    mne::FiffRawData m_rawData;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_FIFFBUFFER_H
