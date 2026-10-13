//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_analyze_controlmanager.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October, 2026
 * @brief    Drives the mne_analyze control manager: its settings widgets must reach the views as events.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <applications/mne_analyze/plugins/controlmanager/controlmanager.h>

#include <anShared/Management/analyzedata.h>
#include <anShared/Management/communicator.h>
#include <anShared/Management/event.h>
#include <anShared/Management/eventmanager.h>
#include <anShared/Model/fiffrawviewmodel.h>
#include <anShared/Utils/types.h>

#include <disp/viewers/control3dview.h>
#include <disp/viewers/fiffrawviewsettings.h>
#include <disp/viewers/scalingview.h>

#include <QtTest>
#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QSpinBox>

#include <cstring>
#include <memory>
#include <new>

using namespace CONTROLMANAGERPLUGIN;
using namespace ANSHAREDLIB;

//=============================================================================================================

class TestAnalyzeControlManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void publishesViewSettings();
    void cleanupTestCase();
};

//=============================================================================================================

void TestAnalyzeControlManager::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    EventManager::startEventHandling();
}

//=============================================================================================================

void TestAnalyzeControlManager::publishesViewSettings()
{
    const QString rawPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample test data not found");

    auto data = QSharedPointer<AnalyzeData>::create();

    // A plugin whose controls do not exist (not built yet, or closed by a plugin reload) leaves the views alone
    QList<ViewParameters> early;
    {
        Communicator earlyListener(QVector<EVENT_TYPE>{VIEW_SETTINGS_CHANGED});
        connect(&earlyListener, &Communicator::receivedEvent, this, [&](const QSharedPointer<Event> e) {
            early << e->getData().value<ViewParameters>();
        });
        auto earlyRaw = data->loadModel<FiffRawViewModel>(rawPath);
        const auto selected = QSharedPointer<Event>::create(SELECTED_MODEL_CHANGED, nullptr, QVariant::fromValue(earlyRaw.staticCast<AbstractModel>()));
        alignas(ControlManager) unsigned char storage[sizeof(ControlManager)];
        std::memset(storage, 0x7f, sizeof(storage));
        auto* unbuilt = new (storage) ControlManager();
        unbuilt->setGlobalData(data);
        unbuilt->init();
        unbuilt->handleEvent(selected);
        delete unbuilt->getControl();
        unbuilt->handleEvent(selected);
        unbuilt->~ControlManager();
        QTest::qWait(200);
    }
    QVERIFY(early.isEmpty());

    ControlManager plugin;
    plugin.setGlobalData(data);
    plugin.init();
    std::unique_ptr<QDockWidget> dock(plugin.getControl());
    dock->show();

    QList<ViewParameters> view;
    QList<ScalingParameters> scaling;
    QList<View3DParameters> view3d;
    Communicator listener(QVector<EVENT_TYPE>{VIEW_SETTINGS_CHANGED, SCALING_MAP_CHANGED, VIEW3D_SETTINGS_CHANGED});
    connect(&listener, &Communicator::receivedEvent, this, [&](const QSharedPointer<Event> e) {
        if (e->getType() == VIEW_SETTINGS_CHANGED)
            view << e->getData().value<ViewParameters>();
        else if (e->getType() == SCALING_MAP_CHANGED)
            scaling << e->getData().value<ScalingParameters>();
        else
            view3d << e->getData().value<View3DParameters>();
    });

    // Only the views ticked in "Apply to" are addressed
    auto* signalView = dock->findChild<QCheckBox*>("m_checkBoxSignaViewer");
    auto* butterfly = dock->findChild<QCheckBox*>("m_checkBoxButterfly");
    auto* layout = dock->findChild<QCheckBox*>("m_checkBoxLayout");
    QVERIFY(signalView && butterfly && layout);
    signalView->setChecked(true);
    butterfly->setChecked(false);
    layout->setChecked(false);
    const QList<QString> views{"null", "signalview"};

    auto* settings = dock->findChild<DISPLIB::FiffRawViewSettings*>();
    QVERIFY(settings);
    auto* zoom = settings->findChild<QDoubleSpinBox*>("m_doubleSpinBox_numberVisibleChannels");
    zoom->setValue(zoom->value() == 20.0 ? 21.0 : 20.0);
    QTRY_VERIFY_WITH_TIMEOUT(!view.isEmpty(), 5000);
    QCOMPARE(view.last().m_sSettingsToApply, ViewParameters::zoom);
    QCOMPARE(view.last().m_dZoomValue, zoom->value());
    QCOMPARE(view.last().m_sViewsToApply, views);

    auto* window = settings->findChild<QSpinBox*>("m_spinBox_windowSize");
    window->setValue(window->value() == 7 ? 8 : 7);
    QTRY_COMPARE_WITH_TIMEOUT(view.last().m_sSettingsToApply, ViewParameters::window, 5000);
    QCOMPARE(view.last().m_iTimeWindow, window->value());

    auto* spacer = settings->findChild<QComboBox*>("m_comboBox_distaceTimeSpacer");
    spacer->setCurrentIndex(spacer->currentIndex() == 2 ? 3 : 2);
    QTRY_COMPARE_WITH_TIMEOUT(view.last().m_sSettingsToApply, ViewParameters::spacer, 5000);
    QCOMPARE(view.last().m_iTimeSpacers, spacer->currentIndex() == 2 ? 500 : 1000);

    settings->findChild<QPushButton*>("m_pushButton_makeScreenshot")->click();
    QTRY_COMPARE_WITH_TIMEOUT(view.last().m_sSettingsToApply, ViewParameters::screenshot, 5000);
    QCOMPARE(view.last().m_sImageType, settings->findChild<QComboBox*>("m_comboBox_imageType")->currentText());

    // Scaling
    auto* scalingView = dock->findChild<DISPLIB::ScalingView*>();
    QVERIFY(scalingView);
    auto* scale = scalingView->findChild<QDoubleSpinBox*>();
    QVERIFY(scale);
    scale->setValue(scale->value() == scale->minimum() ? scale->maximum() : scale->minimum());
    QTRY_VERIFY_WITH_TIMEOUT(!scaling.isEmpty(), 5000);
    QCOMPARE(scaling.last().m_sViewsToApply, views);

    // Selecting a recording pushes the complete current settings to its views
    auto raw = data->loadModel<FiffRawViewModel>(rawPath);
    view.clear();
    scaling.clear();
    plugin.handleEvent(QSharedPointer<Event>::create(SELECTED_MODEL_CHANGED, nullptr, QVariant::fromValue(raw.staticCast<AbstractModel>())));
    QTRY_VERIFY_WITH_TIMEOUT(!view.isEmpty() && !scaling.isEmpty(), 5000);
    QCOMPARE(view.last().m_sSettingsToApply, ViewParameters::all);
    QCOMPARE(view.last().m_dZoomValue, zoom->value());
    QCOMPARE(view.last().m_iTimeWindow, window->value());
    QCOMPARE(view.last().m_iTimeSpacers, spacer->currentIndex() == 2 ? 500 : 1000);

    // 3D controls
    auto* control3d = dock->findChild<DISPLIB::Control3DView*>();
    QVERIFY(control3d);
    auto* rotate = control3d->findChild<QCheckBox*>("m_checkBox_rotate");
    QVERIFY(rotate);
    const bool rotateBefore = rotate->isChecked();
    rotate->click();
    QTRY_VERIFY_WITH_TIMEOUT(!view3d.isEmpty(), 5000);
    QCOMPARE(view3d.last().m_settingsToApply, View3DParameters::rotation);
    QCOMPARE(view3d.last().m_bToggleRotation, !rotateBefore);

    auto* coordAxis = control3d->findChild<QCheckBox*>("m_checkBox_coordAxis");
    const bool axisBefore = coordAxis->isChecked();
    coordAxis->click();
    QTRY_COMPARE_WITH_TIMEOUT(view3d.last().m_settingsToApply, View3DParameters::coordAxis, 5000);
    QCOMPARE(view3d.last().m_bToogleCoordAxis, !axisBefore);

    auto* intensity = control3d->findChild<QDoubleSpinBox*>("m_doubleSpinBox_colorIntensity");
    intensity->setValue(intensity->value() == 50.0 ? 60.0 : 50.0);
    QTRY_COMPARE_WITH_TIMEOUT(view3d.last().m_settingsToApply, View3DParameters::lightIntensity, 5000);
    QCOMPARE(view3d.last().m_dLightIntensity, intensity->value());

    control3d->findChild<QPushButton*>("m_pushButton_takeScreenshot")->click();
    QTRY_COMPARE_WITH_TIMEOUT(view3d.last().m_settingsToApply, View3DParameters::screenshot, 5000);
}

//=============================================================================================================

void TestAnalyzeControlManager::cleanupTestCase()
{
    EventManager::stopEventHandling();
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_MAIN(TestAnalyzeControlManager)
#include "test_analyze_controlmanager.moc"
