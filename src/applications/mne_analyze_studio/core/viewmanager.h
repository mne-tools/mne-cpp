//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     viewmanager.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     March, 2026
 * @version  dev
 * @brief    Declares the file-to-view dispatch logic used by the workbench.
 */

#ifndef MNE_ANALYZE_STUDIO_VIEWMANAGER_H
#define MNE_ANALYZE_STUDIO_VIEWMANAGER_H

#include "studio_core_global.h"

#include <QJsonObject>
#include <QObject>
#include <QStringList>

namespace MNEANALYZESTUDIO
{

class SceneContextRegistry;
class ViewProviderRegistry;

/**
 * @brief Dispatches selected workspace files to the appropriate studio view type.
 */
class STUDIOCORESHARED_EXPORT ViewManager : public QObject
{
    Q_OBJECT

public:
    enum class ViewKind {
        SignalBrowser2D,
        ThreeDScene,
        TextEditor,
        Unsupported
    };
    Q_ENUM(ViewKind)

    explicit ViewManager(SceneContextRegistry* sceneRegistry,
                         ViewProviderRegistry* viewProviderRegistry = nullptr,
                         QObject* parent = nullptr);

    QJsonObject dispatchFileSelection(const QString& filePath, const QJsonObject& metadata = QJsonObject()) const;
    static ViewKind viewKindForFile(const QString& filePath);

private:
    QString inferSubjectId(const QString& filePath, const QJsonObject& metadata) const;

    SceneContextRegistry* m_sceneRegistry;
    ViewProviderRegistry* m_viewProviderRegistry;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_VIEWMANAGER_H
