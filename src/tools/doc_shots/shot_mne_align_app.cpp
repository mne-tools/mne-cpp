//=============================================================================================================
/**
 * @file     shot_mne_align_app.cpp
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
 * @brief    Renderer for the `mne_align_app` shot kind: builds the real
 *           mne_align `MneAlign` window, pushes in-memory demo digitisation
 *           fixtures, advances the wizard to a chosen step, then grabs the
 *           rendered window into a PNG.
 */

#include "shot_mne_align_app.h"

#include "shot_app_common.h"
#include "shot_runner.h"

#include "align_wizard.h"
#include "fixtures/align_demo_fixtures.h"
#include "mne_align.h"

#include <utils/polhemus/acquired_points.h>

#include <QApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QPixmap>
#include <QString>
#include <QVector>

using MNEALIGN::AlignStep;
using MNEALIGN::MneAlign;
using UTILSLIB::DigitizedPoint;

namespace DOCSHOTS
{

namespace {

void appendCapture(QVector<DigitizedPoint>& target, const QJsonObject& spec)
{
    const QString kind = spec.value(QStringLiteral("kind")).toString().toLower();
    const int count    = spec.value(QStringLiteral("count")).toInt(0);
    if (count <= 0) return;

    if (kind == QLatin1String("fiducial")) {
        const QVector<DigitizedPoint> fids = MNEALIGN::demoFiducials();
        const int n = qMin(count, fids.size());
        for (int i = 0; i < n; ++i) target.append(fids.at(i));
    } else if (kind == QLatin1String("eeg")) {
        target += MNEALIGN::demoEegCap(count);
    } else if (kind == QLatin1String("hsp")) {
        target += MNEALIGN::demoHeadShape(count);
    }
}

}  // namespace

bool renderMneAlignApp(const ShotSpec& spec,
                       const QString& outPath,
                       bool& skipped,
                       QString& err)
{
    skipped = false;

    if (!QApplication::instance()) {
        err = QStringLiteral("QApplication has not been constructed");
        return false;
    }

    MneAlign mw;
    mw.resize(spec.size.isValid() ? spec.size : QSize(1280, 800));

    // Switch every QRhiWidget under the window to the Null backend BEFORE
    // showing — otherwise BrainView will try to bring up Metal/Vulkan/OpenGL
    // and crash on a typical headless CI box.
    forceQRhiNullOnRhiWidgets(&mw);

    const QJsonObject setup = spec.setup;

    // Compose the demo digitisation session from the manifest setup before
    // we hand it to the live AcquiredPoints store.
    QVector<DigitizedPoint> session;

    if (setup.value(QStringLiteral("load_demo_digitisation")).toString()
        == QLatin1String("demo")) {
        session = MNEALIGN::demoFullDigitisation();
    }

    if (setup.contains(QStringLiteral("simulate_capture"))) {
        appendCapture(session, setup.value(QStringLiteral("simulate_capture")).toObject());
    }

    if (!session.isEmpty()) {
        MNEALIGN::applyTo(mw.points(), session);
    }

    if (setup.value(QStringLiteral("load_demo_bem")).toBool(false)) {
        if (auto* wiz = mw.wizard()) {
            wiz->setBemPath(QStringLiteral("demo_head_bem.fif"));
        }
    }

    // load_demo_cap is currently informational — the wizard's cap combo is
    // populated from the StandardMontage registry at construction time and
    // does not need a runtime mutation for the screenshots.
    Q_UNUSED(setup.value(QStringLiteral("load_demo_cap")));

    // Optional generic fixture hooks (for future apps that register loaders).
    const QJsonArray extras = setup.value(QStringLiteral("fixtures")).toArray();
    for (const QJsonValue& v : extras) {
        const QString name = v.toString();
        if (!name.isEmpty()) {
            AppFixtureLoaders::apply(name, &mw);
        }
    }

    if (setup.contains(QStringLiteral("wizard_step"))) {
        const int step = qBound(0, setup.value(QStringLiteral("wizard_step")).toInt(0), 6);
        if (auto* wiz = mw.wizard()) {
            wiz->goToStep(static_cast<AlignStep>(step));
        }
    }

    mw.show();
    pumpUntilIdle(&mw, 250);

    const QPixmap pm = mw.grab();
    if (pm.isNull()) {
        err = QStringLiteral("MneAlign::grab() returned a null pixmap");
        return false;
    }
    if (!pm.save(outPath, "PNG")) {
        err = QStringLiteral("Could not write PNG to %1").arg(outPath);
        return false;
    }
    return true;
}

}  // namespace DOCSHOTS
