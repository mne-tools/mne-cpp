//=============================================================================================================
/**
 * @file     test_mne_browse_app.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     August, 2026
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @brief    Offscreen application smoke tests for mne_browse.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <Windows/mainwindow.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDockWidget>
#include <QImage>
#include <QMenuBar>
#include <QPainter>
#include <QRhiWidget>
#include <QStandardPaths>
#include <QtTest>

//=============================================================================================================

using namespace MNEBROWSE;

class TestMneBrowseApp : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void constructsAndRendersMainWindow();
};

//=============================================================================================================

void TestMneBrowseApp::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName("mne-cpp-tests");
    QCoreApplication::setApplicationName("test_mne_browse_app");
}

//=============================================================================================================

void TestMneBrowseApp::constructsAndRendersMainWindow()
{
    // The offscreen QRhi backend cannot safely destroy this widget tree, so keep it process-owned.
    auto* window = new MainWindow;
    const QList<QRhiWidget*> rhiWidgets = window->findChildren<QRhiWidget*>();
    for (QRhiWidget* rhiWidget : rhiWidgets) {
        rhiWidget->setApi(QRhiWidget::Api::Null);
    }
    window->resize(1200, 800);
    window->show();
    const bool exposed = QTest::qWaitForWindowExposed(window);
    const bool hasRawModel = window->rawModel() != nullptr;
    const qsizetype dockCount = window->findChildren<QDockWidget*>().size();
    const qsizetype menuCount = window->menuBar()->actions().size();

    QImage image(window->size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    window->render(&painter);
    painter.end();

    QImage blank(image.size(), image.format());
    blank.fill(Qt::transparent);
    const bool rendered = !image.isNull() && image.constBits() != nullptr && image != blank;

    window->hide();
    QCoreApplication::processEvents();

    QVERIFY(exposed);
    QVERIFY(hasRawModel);
    QVERIFY(dockCount >= 5);
    QVERIFY(menuCount >= 4);
    QVERIFY(rendered);
}

//=============================================================================================================

QTEST_MAIN(TestMneBrowseApp)
#include "test_mne_browse_app.moc"