//=============================================================================================================
/**
 * @file     scenecontextregistry.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
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
 * @brief    Declares scene persistence and merge-tracking helpers for 3D view contexts.
 */

#ifndef MNE_ANALYZE_STUDIO_SCENECONTEXTREGISTRY_H
#define MNE_ANALYZE_STUDIO_SCENECONTEXTREGISTRY_H

#include "studio_core_global.h"

#include <QHash>
#include <QJsonArray>
#include <QObject>
#include <QString>

namespace MNEANALYZESTUDIO
{

/**
 * @brief Serializable description of a logical 3D scene instance.
 */
struct SceneContext {
    QString id;
    QString subjectId;
    QString title;
    QStringList layers;
};

/**
 * @brief Registry that tracks active 3D scene contexts and their layer membership.
 */
class STUDIOCORESHARED_EXPORT SceneContextRegistry : public QObject
{
    Q_OBJECT

public:
    explicit SceneContextRegistry(QObject* parent = nullptr);

    QString createScene(const QString& subjectId, const QString& title);
    QString ensureScene(const QString& subjectId, const QString& title);
    QString activeSceneForSubject(const QString& subjectId) const;
    QString sceneForLayer(const QString& filePath) const;
    QStringList layersForScene(const QString& sceneId) const;
    bool addLayerToScene(const QString& sceneId, const QString& filePath);
    QJsonArray serialize() const;
    void restore(const QJsonArray& serializedScenes);

private:
    QHash<QString, SceneContext> m_scenes;
    QHash<QString, QString> m_activeScenesBySubject;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_SCENECONTEXTREGISTRY_H
