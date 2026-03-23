//=============================================================================================================
/**
 * @file     scenecontextregistry.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements 3D scene context tracking and persistence helpers.
 */

#include "scenecontextregistry.h"

#include <QJsonObject>
#include <QUuid>

using namespace MNEANALYZESTUDIO;

SceneContextRegistry::SceneContextRegistry(QObject* parent)
: QObject(parent)
{
}

QString SceneContextRegistry::ensureScene(const QString& subjectId, const QString& title)
{
    const QString existingId = activeSceneForSubject(subjectId);
    if(!existingId.isEmpty()) {
        return existingId;
    }

    SceneContext context;
    context.id = QString("scene_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    context.subjectId = subjectId;
    context.title = title;
    m_scenes.insert(context.id, context);
    return context.id;
}

QString SceneContextRegistry::activeSceneForSubject(const QString& subjectId) const
{
    for(auto it = m_scenes.cbegin(); it != m_scenes.cend(); ++it) {
        if(it.value().subjectId == subjectId) {
            return it.key();
        }
    }

    return QString();
}

bool SceneContextRegistry::addLayerToScene(const QString& sceneId, const QString& filePath)
{
    auto it = m_scenes.find(sceneId);
    if(it == m_scenes.end()) {
        return false;
    }

    if(!it->layers.contains(filePath)) {
        it->layers.append(filePath);
    }

    return true;
}

QJsonArray SceneContextRegistry::serialize() const
{
    QJsonArray scenes;
    for(auto it = m_scenes.cbegin(); it != m_scenes.cend(); ++it) {
        QJsonArray layers;
        for(const QString& layer : it.value().layers) {
            layers.append(layer);
        }

        scenes.append(QJsonObject{
            {"id", it.value().id},
            {"subjectId", it.value().subjectId},
            {"title", it.value().title},
            {"layers", layers}
        });
    }

    return scenes;
}

void SceneContextRegistry::restore(const QJsonArray& serializedScenes)
{
    m_scenes.clear();
    for(const QJsonValue& value : serializedScenes) {
        const QJsonObject sceneObject = value.toObject();
        SceneContext context;
        context.id = sceneObject.value("id").toString();
        context.subjectId = sceneObject.value("subjectId").toString();
        context.title = sceneObject.value("title").toString();
        const QJsonArray layers = sceneObject.value("layers").toArray();
        for(const QJsonValue& layer : layers) {
            context.layers.append(layer.toString());
        }
        m_scenes.insert(context.id, context);
    }
}
