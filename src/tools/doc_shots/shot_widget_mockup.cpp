//=============================================================================================================
/**
 * @file     shot_widget_mockup.cpp
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
 * @brief    "widget_mockup" shot kind: builds a representative QMainWindow
 *           with toolbars and docks from the manifest's setup block, pumps
 *           the event loop, and saves the grab as PNG.
 */

#include "shot_kinds.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDockWidget>
#include <QImage>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QPainter>
#include <QPixmap>
#include <QStatusBar>
#include <QString>
#include <QStringList>
#include <QTextEdit>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

namespace DOCSHOTS
{

namespace {

QString jsonString(const QJsonObject& o, const QString& key, const QString& fallback = QString())
{
    return o.value(key).toString(fallback);
}

QStringList jsonStringList(const QJsonObject& o, const QString& key)
{
    QStringList out;
    for (const QJsonValue& v : o.value(key).toArray()) {
        out.append(v.toString());
    }
    return out;
}

QWidget* buildCentralCanvas(const QJsonObject& setup, QWidget* parent)
{
    auto* w = new QWidget(parent);
    auto* lay = new QVBoxLayout(w);
    lay->setContentsMargins(12, 12, 12, 12);
    lay->setSpacing(8);

    const QString title = jsonString(setup, QStringLiteral("canvas_title"),
                                     QStringLiteral("Scene"));
    auto* titleLbl = new QLabel(title, w);
    QFont tf = titleLbl->font();
    tf.setBold(true);
    tf.setPointSize(tf.pointSize() + 2);
    titleLbl->setFont(tf);
    lay->addWidget(titleLbl);

    auto* canvas = new QLabel(w);
    canvas->setMinimumSize(640, 480);
    canvas->setAlignment(Qt::AlignCenter);
    canvas->setStyleSheet(QStringLiteral(
        "QLabel { background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        " stop:0 #1a1d24, stop:1 #2c313c);"
        " color: #d8d8d8; border: 1px solid #555; }"));
    canvas->setText(jsonString(setup, QStringLiteral("canvas_text"),
                               QStringLiteral("3D viewport")));
    lay->addWidget(canvas, /*stretch=*/1);

    const QString caption = jsonString(setup, QStringLiteral("canvas_caption"));
    if (!caption.isEmpty()) {
        auto* capLbl = new QLabel(caption, w);
        capLbl->setWordWrap(true);
        capLbl->setStyleSheet(QStringLiteral("color: #555; font-style: italic;"));
        lay->addWidget(capLbl);
    }
    return w;
}

void addDocks(QMainWindow* mw, const QJsonArray& docks)
{
    for (const QJsonValue& v : docks) {
        const QJsonObject o = v.toObject();
        const QString title = jsonString(o, QStringLiteral("title"),
                                         QStringLiteral("Dock"));
        const QString area  = jsonString(o, QStringLiteral("area"),
                                         QStringLiteral("right"));
        const QStringList items = jsonStringList(o, QStringLiteral("items"));

        auto* dock = new QDockWidget(title, mw);
        dock->setAllowedAreas(Qt::AllDockWidgetAreas);
        if (!items.isEmpty()) {
            auto* list = new QListWidget(dock);
            list->addItems(items);
            dock->setWidget(list);
        } else {
            const QString body = jsonString(o, QStringLiteral("text"));
            auto* text = new QTextEdit(dock);
            text->setReadOnly(true);
            text->setPlainText(body);
            dock->setWidget(text);
        }
        Qt::DockWidgetArea da = Qt::RightDockWidgetArea;
        if (area == QLatin1String("left"))   da = Qt::LeftDockWidgetArea;
        if (area == QLatin1String("bottom")) da = Qt::BottomDockWidgetArea;
        if (area == QLatin1String("top"))    da = Qt::TopDockWidgetArea;
        mw->addDockWidget(da, dock);
    }
}

void addToolbar(QMainWindow* mw, const QJsonObject& setup)
{
    const QStringList actions = jsonStringList(setup, QStringLiteral("toolbar"));
    if (actions.isEmpty()) return;
    auto* tb = mw->addToolBar(QStringLiteral("Main"));
    tb->setMovable(false);
    for (const QString& a : actions) {
        tb->addAction(a);
    }
}

void pumpEvents()
{
    for (int i = 0; i < 6; ++i) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 30);
    }
}

}  // namespace

bool renderWidgetMockup(const ShotSpec& spec, const QString& outPath, QString& err)
{
    QMainWindow mw;
    const QString windowTitle = jsonString(spec.setup, QStringLiteral("window_title"),
                                           spec.id);
    mw.setWindowTitle(windowTitle);

    addToolbar(&mw, spec.setup);
    mw.setCentralWidget(buildCentralCanvas(spec.setup, &mw));
    addDocks(&mw, spec.setup.value(QStringLiteral("docks")).toArray());

    const QString statusText = jsonString(spec.setup, QStringLiteral("status"));
    if (!statusText.isEmpty()) {
        mw.statusBar()->showMessage(statusText);
    }

    mw.resize(spec.size);
    mw.show();
    pumpEvents();

    const QPixmap pm = mw.grab();
    if (pm.isNull()) {
        err = QStringLiteral("QWidget::grab() returned a null pixmap");
        return false;
    }
    if (!pm.save(outPath, "PNG")) {
        err = QStringLiteral("Failed to save PNG to %1").arg(outPath);
        return false;
    }
    return true;
}

}  // namespace DOCSHOTS
