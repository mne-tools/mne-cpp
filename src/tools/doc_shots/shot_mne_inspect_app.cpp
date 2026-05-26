//=============================================================================================================
/**
 * @file     shot_mne_inspect_app.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided
 * that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and
 *       the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions
 *       and the following disclaimer in the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used to endorse or
 *       promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @brief    Renderer for the `mne_inspect_app` shot kind: builds the real
 *           mne_inspect `MainWindow`, pushes in-memory demo fixtures, raises
 *           a focus dock, optionally injects a `PickResult`, then grabs the
 *           rendered window into a PNG.
 */

#include "shot_mne_inspect_app.h"

#include "shot_app_common.h"
#include "shot_runner.h"

#include "fixtures/inspect_demo_fixtures.h"
#include "mainwindow.h"
#include "plugins/electrodes/electrodes.h"
#include "plugins/mri_slices/mri_slices.h"

#include <disp3D/scene/multimodalscene.h>
#include <disp3D/scene/pickresult.h>

#include <QApplication>
#include <QDockWidget>
#include <QImage>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QPixmap>
#include <QString>
#include <QVector3D>

using DISP3DLIB::PickKind;
using DISP3DLIB::PickResult;

namespace DOCSHOTS
{

namespace {

QDockWidget* dockByName(MainWindow& mw, const QString& name)
{
    const QString lower = name.toLower();
    if (lower == QLatin1String("pick"))    return mw.pickDock();
    if (lower == QLatin1String("layers"))  return mw.layersDock();
    if (lower == QLatin1String("overlay")) return mw.overlayDock();
    return nullptr;
}

void injectPick(MainWindow& mw, const QJsonObject& spec)
{
    const QString kindStr = spec.value(QStringLiteral("kind")).toString().toLower();
    PickResult pick;

    if (kindStr == QLatin1String("contact")) {
        pick.kind = PickKind::ElectrodeContact;
        const QJsonArray target = spec.value(QStringLiteral("target")).toArray();
        if (target.size() >= 2) {
            pick.sourceId = QStringLiteral("seeg_%1").arg(target.at(0).toString());
            pick.label    = target.at(1).toString();
        } else if (target.size() == 1) {
            pick.label    = target.at(0).toString();
        }
        pick.world = QVector3D(0.0f, 0.01f, 0.02f);
    } else if (kindStr == QLatin1String("voxel")) {
        pick.kind  = PickKind::MriVoxel;
        pick.label = QStringLiteral("voxel(16,16,16)");
        pick.voxel = QVector3D(16, 16, 16);
        pick.world = QVector3D(0.0f, 0.0f, 0.0f);
        pick.sliceOrientation = 0;
    } else if (kindStr == QLatin1String("vertex")) {
        pick.kind        = PickKind::CorticalVertex;
        pick.hemisphere  = 0;
        pick.label       = QStringLiteral("LH vertex 0");
        pick.objectId    = 0;
    } else {
        return;  // unknown pick kind — silently ignored
    }

    mw.scene().reportPick(pick);
}

}  // namespace

bool renderMneInspectApp(const ShotSpec& spec,
                         const QString& outPath,
                         bool& skipped,
                         QString& err)
{
    skipped = false;

    if (!QApplication::instance()) {
        err = QStringLiteral("QApplication has not been constructed");
        return false;
    }

    MainWindow mw;
    mw.resize(spec.size.isValid() ? spec.size : QSize(1280, 800));

    // Switch every QRhiWidget under the MainWindow to the Null backend BEFORE
    // showing — otherwise BrainView will try to bring up Metal/Vulkan/OpenGL
    // and crash on a typical headless CI box.
    forceQRhiNullOnRhiWidgets(&mw);

    const QJsonObject setup = spec.setup;

    if (setup.value(QStringLiteral("load_demo_electrodes")).toBool(false)) {
        mw.electrodesPlugin().setArrays(MNEINSPECT::demoFourArrayMontage());
    }
    if (setup.value(QStringLiteral("load_demo_mri")).toBool(false)) {
        if (!mw.mriSlicesPlugin().setVolume(MNEINSPECT::demoMriSlab())) {
            err = QStringLiteral("demoMriSlab() produced an invalid MriVolData");
            return false;
        }
    }

    // Optional generic fixture hooks (for future apps that register loaders).
    const QJsonArray extras = setup.value(QStringLiteral("fixtures")).toArray();
    for (const QJsonValue& v : extras) {
        const QString name = v.toString();
        if (!name.isEmpty()) {
            AppFixtureLoaders::apply(name, &mw);
        }
    }

    mw.show();
    pumpUntilIdle(&mw, 250);

    if (setup.contains(QStringLiteral("focus_dock"))) {
        const QString focus = setup.value(QStringLiteral("focus_dock")).toString();
        if (QDockWidget* d = dockByName(mw, focus)) {
            d->show();
            d->raise();
            pumpUntilIdle(&mw, 100);
        }
    }

    if (setup.contains(QStringLiteral("simulate_pick"))) {
        injectPick(mw, setup.value(QStringLiteral("simulate_pick")).toObject());
        pumpUntilIdle(&mw, 100);
    }

    const QPixmap pm = mw.grab();
    if (pm.isNull()) {
        err = QStringLiteral("MainWindow::grab() returned a null pixmap");
        return false;
    }
    if (!pm.save(outPath, "PNG")) {
        err = QStringLiteral("Could not write PNG to %1").arg(outPath);
        return false;
    }
    return true;
}

}  // namespace DOCSHOTS
