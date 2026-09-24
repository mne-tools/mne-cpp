//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     ibuffer.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     March, 2026
 * @version  dev
 * @brief    Declares the abstract buffer interface used by MNE Analyze Studio.
 */

#ifndef MNE_ANALYZE_STUDIO_IBUFFER_H
#define MNE_ANALYZE_STUDIO_IBUFFER_H

#include "studio_core_global.h"

#include <QDataStream>
#include <QIODevice>
#include <QJsonObject>
#include <QObject>
#include <QString>

namespace MNEANALYZESTUDIO
{

/**
 * @brief Abstract interface for metadata-aware binary buffers in the studio.
 */
class STUDIOCORESHARED_EXPORT IBuffer : public QObject
{
    Q_OBJECT

public:
    enum class BufferKind {
        Fiff,
        MriVolume,
        Surface,
        Text,
        Unknown
    };
    Q_ENUM(BufferKind)

    explicit IBuffer(QObject* parent = nullptr);
    ~IBuffer() override;

    virtual BufferKind kind() const = 0;
    virtual QString uri() const = 0;
    virtual bool open() = 0;
    virtual bool isOpen() const = 0;
    virtual QJsonObject getMetadata() const = 0;
    virtual QIODevice* device() const = 0;
    virtual QDataStream* getBinaryStream() = 0;

signals:
    void metadataChanged(const QJsonObject& metadata);
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_IBUFFER_H
