//=============================================================================================================
/**
 * @file     viewmanager.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements file dispatch logic for workbench view creation.
 */

#include "viewmanager.h"

#include "scenecontextregistry.h"

#include <QFileInfo>

using namespace MNEANALYZESTUDIO;

namespace
{

const QStringList kSignalExtensions = {".fif", ".ave", ".edf"};
const QStringList kThreeDExtensions = {".surf", ".pial", ".white"};
const QStringList kTextExtensions = {".py", ".cpp", ".json"};

}

ViewManager::ViewManager(SceneContextRegistry* sceneRegistry, QObject* parent)
: QObject(parent)
, m_sceneRegistry(sceneRegistry)
{
}

QJsonObject ViewManager::dispatchFileSelection(const QString& filePath, const QJsonObject& metadata) const
{
    const ViewKind kind = viewKindForFile(filePath);
    QJsonObject dispatch{
        {"file", filePath}
    };

    switch(kind) {
        case ViewKind::SignalBrowser2D:
            dispatch.insert("view", "SignalBrowserView");
            dispatch.insert("buffer", "FiffBuffer");
            dispatch.insert("mode", "replace_tab");
            break;
        case ViewKind::ThreeDScene: {
            const QString subjectId = inferSubjectId(filePath, metadata);
            const QString sceneId = m_sceneRegistry ? m_sceneRegistry->ensureScene(subjectId, QFileInfo(filePath).fileName()) : QString();
            dispatch.insert("view", "ThreeDView");
            dispatch.insert("sceneId", sceneId);
            dispatch.insert("mode", sceneId.isEmpty() ? "new_scene" : "merge_or_prompt");
            dispatch.insert("agentSuggestion", QString("I see you're opening %1; I can add it to the active 3D scene.").arg(QFileInfo(filePath).fileName()));
            break;
        }
        case ViewKind::TextEditor:
            dispatch.insert("view", "CodeEditorView");
            dispatch.insert("mode", "replace_tab");
            break;
        case ViewKind::Unsupported:
            dispatch.insert("view", "Unsupported");
            dispatch.insert("mode", "none");
            break;
    }

    return dispatch;
}

ViewManager::ViewKind ViewManager::viewKindForFile(const QString& filePath)
{
    const QString suffix = QFileInfo(filePath).suffix().toLower();
    const QString extension = QString(".%1").arg(suffix);

    if(kSignalExtensions.contains(extension)) {
        return ViewKind::SignalBrowser2D;
    }
    if(kThreeDExtensions.contains(extension)) {
        return ViewKind::ThreeDScene;
    }
    if(kTextExtensions.contains(extension)) {
        return ViewKind::TextEditor;
    }

    return ViewKind::Unsupported;
}

QString ViewManager::inferSubjectId(const QString& filePath, const QJsonObject& metadata) const
{
    const QString subjectFromMetadata = metadata.value("subject").toString();
    if(!subjectFromMetadata.isEmpty()) {
        return subjectFromMetadata;
    }

    return QFileInfo(filePath).completeBaseName();
}
