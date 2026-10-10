//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_analyze_channelselection.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October, 2026
 * @brief    Drives the mne_analyze channel selection plugin: selecting sensors publishes their channel indices.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <applications/mne_analyze/plugins/channelselection/channelselection.h>

#include <anShared/Management/analyzedata.h>
#include <anShared/Management/communicator.h>
#include <anShared/Management/event.h>
#include <anShared/Management/eventmanager.h>
#include <anShared/Model/bemdatamodel.h>
#include <anShared/Model/fiffrawviewmodel.h>

#include <disp/viewers/channelselectionview.h>
#include <disp/viewers/helpers/selectionsceneitem.h>

#include <QtTest>
#include <QDockWidget>

#include <memory>

using namespace CHANNELSELECTIONPLUGIN;
using namespace ANSHAREDLIB;

//=============================================================================================================

class TestAnalyzeChannelSelection : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void publishesTheSelectedChannels();
    void cleanupTestCase();
};

//=============================================================================================================

void TestAnalyzeChannelSelection::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("MNE-CPP-Test"));
    QCoreApplication::setApplicationName(QStringLiteral("test_analyze_channelselection"));
    EventManager::startEventHandling();
}

//=============================================================================================================

void TestAnalyzeChannelSelection::publishesTheSelectedChannels()
{
    const QString dataDir = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/";
    if (!QFile::exists(dataDir + "MEG/sample/sample_audvis_trunc_raw.fif"))
        QSKIP("Sample test data not found");

    auto data = QSharedPointer<AnalyzeData>::create();
    ChannelSelection plugin;
    plugin.setGlobalData(data);
    plugin.init();
    std::unique_ptr<QDockWidget> dock(plugin.getControl());
    std::unique_ptr<QWidget> view(plugin.getView());

    // Removing a model before any recording was shown leaves the plugin alone
    auto bem = data->loadModel<BemDataModel>(dataDir + "subjects/sample/bem/sample-1280-1280-1280-bem.fif");
    plugin.handleEvent(QSharedPointer<Event>::create(MODEL_REMOVED, nullptr, QVariant::fromValue(bem.staticCast<AbstractModel>())));

    auto raw = data->loadModel<FiffRawViewModel>(dataDir + "MEG/sample/sample_audvis_trunc_raw.fif");
    plugin.handleEvent(QSharedPointer<Event>::create(SELECTED_MODEL_CHANGED, nullptr, QVariant::fromValue(raw.staticCast<AbstractModel>())));

    Communicator listener(QVector<EVENT_TYPE>{CHANNEL_SELECTION_ITEMS});
    QList<QStringList> names;
    QList<QList<int>> indices;
    connect(&listener, &Communicator::receivedEvent, this, [&](const QSharedPointer<Event> e) {
        const auto* item = e->getData().value<DISPLIB::SelectionItem*>();
        names << item->m_sChannelName;
        indices << item->m_iChannelNumber;
    });

    auto* selection = view->findChild<DISPLIB::ChannelSelectionView*>();
    if (!selection) {
        for (QWidget* w : QApplication::topLevelWidgets()) {
            if (auto* v = qobject_cast<DISPLIB::ChannelSelectionView*>(w))
                selection = v;
        }
    }
    QVERIFY(selection);

    // Selecting two sensors, twice: each time the same names and the indices of those channels in the recording
    const QStringList wanted{QStringLiteral("MEG0113"), QStringLiteral("MEG0112")};
    for (int pass = 0; pass < 2; ++pass) {
        selection->selectChannels(wanted);
        QTRY_VERIFY_WITH_TIMEOUT(names.size() > pass, 5000);
        QStringList got = names.last();
        got.sort();
        QStringList expected = wanted;
        expected.sort();
        QCOMPARE(got, expected);
        QList<int> idx = indices.last();
        std::sort(idx.begin(), idx.end());
        QList<int> expectedIdx{static_cast<int>(raw->getFiffInfo()->ch_names.indexOf("MEG0112")),
                               static_cast<int>(raw->getFiffInfo()->ch_names.indexOf("MEG0113"))};
        std::sort(expectedIdx.begin(), expectedIdx.end());
        QCOMPARE(idx, expectedIdx);
    }
}

//=============================================================================================================

void TestAnalyzeChannelSelection::cleanupTestCase()
{
    EventManager::stopEventHandling();
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_MAIN(TestAnalyzeChannelSelection)
#include "test_analyze_channelselection.moc"
