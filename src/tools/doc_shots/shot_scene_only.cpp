//=============================================================================================================
/**
 * @file     shot_scene_only.cpp
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
 * @brief    "scene_only" shot kind: renders a labelled placeholder PNG.
 *
 *           A real Metal/QRhi readback of a MultimodalScene requires a
 *           dedicated DISP3DLIB renderer which does not yet exist (the
 *           current MultimodalScene class is a pure controller).  Until
 *           that renderer lands, this implementation produces a clearly
 *           labelled gradient placeholder so the documentation pipeline
 *           stays unblocked.
 */

#include "shot_kinds.h"

#include <QFont>
#include <QImage>
#include <QJsonObject>
#include <QLinearGradient>
#include <QPainter>
#include <QPen>
#include <QString>

namespace DOCSHOTS
{

bool renderSceneOnly(const ShotSpec& spec, const QString& outPath,
                     bool& skipped, QString& err)
{
    skipped = false;

    QImage img(spec.size, QImage::Format_ARGB32_Premultiplied);
    if (img.isNull()) {
        err = QStringLiteral("Failed to allocate QImage %1x%2")
                  .arg(spec.size.width()).arg(spec.size.height());
        return false;
    }

    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, true);

    QLinearGradient grad(0, 0, spec.size.width(), spec.size.height());
    grad.setColorAt(0.0, QColor("#10131a"));
    grad.setColorAt(1.0, QColor("#2c313c"));
    p.fillRect(img.rect(), grad);

    // Subtle 3D axes hint, centred.
    const QPointF c(spec.size.width() / 2.0, spec.size.height() / 2.0);
    const qreal a = qMin(spec.size.width(), spec.size.height()) * 0.18;
    p.setPen(QPen(QColor(220, 70, 70), 2.0));
    p.drawLine(c, c + QPointF(a, 0));
    p.setPen(QPen(QColor(70, 200, 90), 2.0));
    p.drawLine(c, c + QPointF(0, -a));
    p.setPen(QPen(QColor(80, 130, 230), 2.0));
    p.drawLine(c, c + QPointF(-a * 0.6, a * 0.6));

    // Labels.
    const QString title = spec.setup.value(QStringLiteral("canvas_title")).toString(
        QStringLiteral("MultimodalScene"));
    p.setPen(QColor(225, 225, 225));
    QFont f = p.font();
    f.setBold(true);
    f.setPointSize(f.pointSize() + 4);
    p.setFont(f);
    p.drawText(QRect(0, 24, spec.size.width(), 40), Qt::AlignHCenter, title);

    f.setBold(false);
    f.setPointSize(f.pointSize() - 3);
    p.setFont(f);
    p.setPen(QColor(160, 160, 160));
    p.drawText(QRect(0, spec.size.height() - 48, spec.size.width(), 24),
               Qt::AlignHCenter,
               QStringLiteral("scene_only placeholder — %1").arg(spec.id));

    p.end();

    if (!img.save(outPath, "PNG")) {
        err = QStringLiteral("Failed to save PNG to %1").arg(outPath);
        return false;
    }
    return true;
}

}  // namespace DOCSHOTS
