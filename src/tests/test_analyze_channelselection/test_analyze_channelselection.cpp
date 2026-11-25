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

#include <apps/mne_analyze/plugins/channelselection/channelselection.h>
#include <mne/base/string_utils.h>
#include <anShared/Management/analyzedata.h>
#include <anShared/Management/communicator.h>
#include <anShared/Management/event.h>
#include <anShared/Management/eventmanager.h>
#include <anShared/Model/bemdatamodel.h>
#include <anShared/Model/fiffrawviewmodel.h>
#include <mne/disp_widgets/viewers/channelselectionview.h>
#include <mne/disp/viewers/helpers/selectionitem.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QDockWidget>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

using namespace CHANNELSELECTIONPLUGIN;
using namespace mne::analyze;

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
    std::vector<std::vector<std::string>> names;
    std::vector<std::vector<int>> indices;
    connect(&listener, &Communicator::receivedEvent, this, [&](const QSharedPointer<Event> e) {
        const auto* item = e->getData().value<mne::disp::SelectionItem*>();
        names.push_back(item->m_sChannelName);
        indices.push_back(item->m_iChannelNumber);
    });

    auto* selection = view->findChild<mne::disp::ChannelSelectionView*>();
    if (!selection) {
        for (QWidget* w : QApplication::topLevelWidgets()) {
            if (auto* v = qobject_cast<mne::disp::ChannelSelectionView*>(w))
                selection = v;
        }
    }
    QVERIFY(selection);

    // Selecting two sensors, twice: each time the same names and the indices of those channels in the recording
    const std::vector<std::string> wanted{"MEG 0113", "MEG 0112"};
    for (int pass = 0; pass < 2; ++pass) {
        selection->selectChannels(wanted);
        QTRY_VERIFY_WITH_TIMEOUT(std::ssize(names) > pass, 5000);
        std::vector<std::string> got = names.back();
        std::ranges::sort(got);
        std::vector<std::string> expected = wanted;
        std::ranges::sort(expected);
        QCOMPARE(got, expected);
        std::vector<int> idx = indices.back();
        std::sort(idx.begin(), idx.end());
        std::vector<int> expectedIdx{static_cast<int>(mne::index_of(raw->getFiffInfo()->ch_names, "MEG 0112")),
                                     static_cast<int>(mne::index_of(raw->getFiffInfo()->ch_names, "MEG 0113"))};
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
