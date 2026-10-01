//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *

 * @file     shot_app_common.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @brief    Implementation of the shared real-app shot helpers.
 */

#include "shot_app_common.h"

#include <QHash>
#include <QRhiWidget>
#include <QWidget>

namespace DOCSHOTS
{

bool forceQRhiNullOnRhiWidgets(QWidget* root)
{
    if (!root) {
        return false;
    }
    if (auto* rw = qobject_cast<QRhiWidget*>(root)) {
        rw->setApi(QRhiWidget::Api::Null);
    }
    const QList<QRhiWidget*> children = root->findChildren<QRhiWidget*>();
    for (QRhiWidget* rw : children) {
        rw->setApi(QRhiWidget::Api::Null);
    }
    return true;
}

namespace {

QHash<QString, AppFixtureLoaders::Loader>& registry()
{
    static QHash<QString, AppFixtureLoaders::Loader> r;
    return r;
}

}  // namespace

void AppFixtureLoaders::registerLoader(const QString& name, Loader loader)
{
    registry().insert(name, std::move(loader));
}

bool AppFixtureLoaders::apply(const QString& name, QObject* mainWindow)
{
    const auto it = registry().constFind(name);
    if (it == registry().constEnd()) {
        return false;
    }
    it.value()(mainWindow);
    return true;
}

bool AppFixtureLoaders::isRegistered(const QString& name)
{
    return registry().contains(name);
}

}  // namespace DOCSHOTS
