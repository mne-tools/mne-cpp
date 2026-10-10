//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_analyze_rawdataviewer.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October, 2026
 * @brief    Drives the mne_analyze signal viewer: event marks and jumping to a selected event.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <applications/mne_analyze/plugins/rawdataviewer/rawdataviewer.h>
#include <applications/mne_analyze/plugins/rawdataviewer/fiffrawview.h>

#include <anShared/Management/analyzedata.h>
#include <anShared/Management/event.h>
#include <anShared/Model/eventmodel.h>
#include <anShared/Model/fiffrawviewmodel.h>

#include <QtTest>
#include <QRhiWidget>
#include <QScrollBar>
#include <QTableView>

using namespace RAWDATAVIEWERPLUGIN;
using namespace ANSHAREDLIB;

//=============================================================================================================

class TestAnalyzeRawDataViewer : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void jumpsToTheSelectedEvent();
};

//=============================================================================================================

void TestAnalyzeRawDataViewer::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("MNE-CPP-Test"));
    QCoreApplication::setApplicationName(QStringLiteral("test_analyze_rawdataviewer"));
}

//=============================================================================================================

void TestAnalyzeRawDataViewer::jumpsToTheSelectedEvent()
{
    const QString rawPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample test data not found");

    auto data = QSharedPointer<AnalyzeData>::create();
    RawDataViewer plugin;
    plugin.setGlobalData(data);
    plugin.init();
    auto* view = qobject_cast<FiffRawView*>(plugin.getView());
    QVERIFY(view);
    view->resize(1000, 600);
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view));

    auto raw = data->loadModel<FiffRawViewModel>(rawPath);
    plugin.handleEvent(QSharedPointer<Event>::create(SELECTED_MODEL_CHANGED, nullptr, QVariant::fromValue(raw.staticCast<AbstractModel>())));
    for (QRhiWidget* rhi : view->findChildren<QRhiWidget*>())
        rhi->setApi(QRhiWidget::Api::Null);
    auto* table = view->findChild<QTableView*>(QStringLiteral("m_pTableView"));
    QVERIFY(table);
    QCoreApplication::processEvents();

    // Jumping without a selected event leaves the view where it is
    const int start = table->horizontalScrollBar()->value();
    plugin.handleEvent(QSharedPointer<Event>::create(TRIGGER_VIEWER_MOVE, nullptr, QVariant()));
    QCOMPARE(table->horizontalScrollBar()->value(), start);

    // With the second of two events selected, the view centres on it
    auto events = QSharedPointer<EventModel>::create(raw);
    raw->setEventModel(events);
    events->addGroup(QStringLiteral("other"), QColor(Qt::blue));
    events->addEventWithCode(13000, 5);
    events->addGroup(QStringLiteral("stim"), QColor(Qt::red));
    events->addEventWithCode(14000, 1);
    events->addEventWithCode(16000, 2);
    // The table lists the selected group's events only; its second row is the event at 16000
    events->clearEventSelection();
    events->appendSelected(1);
    plugin.handleEvent(QSharedPointer<Event>::create(TRIGGER_VIEWER_MOVE, nullptr, QVariant()));
    const int expected = static_cast<int>((16000 - raw->absoluteFirstSample()) * raw->pixelDifference()) - table->width() / 2;
    QVERIFY2(std::abs(table->horizontalScrollBar()->value() - expected) <= 1,
             qPrintable(QStringLiteral("scroll %1, expected %2").arg(table->horizontalScrollBar()->value()).arg(expected)));
    view->hide();
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_MAIN(TestAnalyzeRawDataViewer)
#include "test_analyze_rawdataviewer.moc"
