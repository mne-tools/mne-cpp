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

QString SceneContextRegistry::createScene(const QString& subjectId, const QString& title)
{
    SceneContext context;
    context.id = QString("scene_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    context.subjectId = subjectId;
    context.title = title;
    m_scenes.insert(context.id, context);

    if(!subjectId.isEmpty()) {
        m_activeScenesBySubject.insert(subjectId, context.id);
    }

    return context.id;
}

QString SceneContextRegistry::ensureScene(const QString& subjectId, const QString& title)
{
    const QString existingId = activeSceneForSubject(subjectId);
    if(!existingId.isEmpty()) {
        return existingId;
    }

    return createScene(subjectId, title);
}

QString SceneContextRegistry::activeSceneForSubject(const QString& subjectId) const
{
    const QString activeSceneId = m_activeScenesBySubject.value(subjectId);
    if(!activeSceneId.isEmpty() && m_scenes.contains(activeSceneId)) {
        return activeSceneId;
    }

    for(auto it = m_scenes.cbegin(); it != m_scenes.cend(); ++it) {
        if(it.value().subjectId == subjectId) {
            return it.key();
        }
    }

    return QString();
}

QString SceneContextRegistry::sceneForLayer(const QString& filePath) const
{
    for(auto it = m_scenes.cbegin(); it != m_scenes.cend(); ++it) {
        if(it.value().layers.contains(filePath)) {
            return it.key();
        }
    }

    return QString();
}

QStringList SceneContextRegistry::layersForScene(const QString& sceneId) const
{
    return m_scenes.value(sceneId).layers;
}

bool SceneContextRegistry::addLayerToScene(const QString& sceneId, const QString& filePath)
{
    auto it = m_scenes.find(sceneId);
    if(it == m_scenes.end()) {
        return false;
    }

    for(auto sceneIt = m_scenes.begin(); sceneIt != m_scenes.end(); ++sceneIt) {
        if(sceneIt.key() == sceneId) {
            continue;
        }

        sceneIt->layers.removeAll(filePath);
    }

    if(!it->layers.contains(filePath)) {
        it->layers.append(filePath);
    }

    if(!it->subjectId.isEmpty()) {
        m_activeScenesBySubject.insert(it->subjectId, sceneId);
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
            {"active", m_activeScenesBySubject.value(it.value().subjectId) == it.value().id},
            {"layers", layers}
        });
    }

    return scenes;
}

void SceneContextRegistry::restore(const QJsonArray& serializedScenes)
{
    m_scenes.clear();
    m_activeScenesBySubject.clear();

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

        if(sceneObject.value("active").toBool(false) && !context.subjectId.isEmpty()) {
            m_activeScenesBySubject.insert(context.subjectId, context.id);
        }
    }
}
