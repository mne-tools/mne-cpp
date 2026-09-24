//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     scenecontextregistry.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     March, 2026
 * @version  dev
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
