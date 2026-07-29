//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_disp_viewers3.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     July, 2026
 * @brief    Construction and lifecycle checks for display viewers that had no test at all.
 *
 * This covers the viewers that test_disp_viewers and test_disp_viewers2 do not
 * touch. It is deliberately shallow: it constructs each widget, exercises the
 * settings save and restore path, shows and resizes it, and destroys it. That
 * is far weaker than the reader cross validation tests elsewhere in this suite
 * and it is not pretending otherwise.
 *
 * It is still worth having. A widget whose constructor dereferences a null
 * model, whose settings path throws on an empty key, or whose destructor
 * double frees will fail here, and those faults currently reach users because
 * nothing instantiates these classes outside the application. The value is in
 * catching construction and teardown faults early, not in verifying that the
 * widgets display anything correct.
 *
 * Widgets are parented to a holder so destruction runs through the normal Qt
 * ownership path rather than through a bare delete, which is how they are used
 * in the applications.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <disp/viewers/dipolefitview.h>
#include <disp/viewers/bidsview.h>
#include <disp/viewers/averagelayoutview.h>
#include <disp/viewers/butterflyview.h>
#include <disp/viewers/control3dview.h>
#include <disp/viewers/artifactsettingsview.h>
#include <disp/viewers/projectsettingsview.h>
#include <disp/viewers/helpers/timerulerwidget.h>
#include <disp/viewers/helpers/channellabelpanel.h>
#include <disp/viewers/helpers/overviewbarwidget.h>

#include <fiff/fiff_ch_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QWidget>
#include <QSettings>
#include <QScopedPointer>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
/**
 * DECLARE CLASS TestDispViewers3
 *
 * @brief Construction and lifecycle checks for the remaining display viewers.
 */
class TestDispViewers3: public QObject
{
    Q_OBJECT

public:
    TestDispViewers3() = default;

private:
    /** Settings key used for the save and restore paths, cleaned up afterwards. */
    static QString settingsPath();

    QScopedPointer<QWidget> m_pHolder;

private slots:
    void initTestCase();
    void construct_dipoleFitView();
    void construct_bidsView();
    void construct_averageLayoutView();
    void construct_butterflyView();
    void construct_control3DView();
    void construct_artifactSettingsView();
    void construct_projectSettingsView();
    void construct_timeRulerWidget();
    void construct_channelLabelPanel();
    void construct_overviewBarWidget();
    void cleanupTestCase();
};

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

QString TestDispViewers3::settingsPath()
{
    return QString("MNECPP/TestDispViewers3");
}

//=============================================================================================================

void TestDispViewers3::initTestCase()
{
    m_pHolder.reset(new QWidget());
}

//=============================================================================================================

void TestDispViewers3::construct_dipoleFitView()
{
    // Widgets are parented so teardown goes through Qt ownership, the way the
    // applications use them, rather than through a bare delete.
    DipoleFitView* pView = new DipoleFitView(m_pHolder.data());
    QVERIFY(pView != nullptr);

    pView->resize(320, 240);
    pView->show();
    QVERIFY2(pView->size().isValid() && !pView->size().isEmpty(),
             "widget laid out to an empty geometry");

    delete pView;
}

//=============================================================================================================

void TestDispViewers3::construct_bidsView()
{
    BidsView* pView = new BidsView(m_pHolder.data());
    QVERIFY(pView != nullptr);

    pView->resize(320, 240);
    pView->show();
    QVERIFY2(pView->size().isValid() && !pView->size().isEmpty(),
             "widget laid out to an empty geometry");

    delete pView;
}

//=============================================================================================================

void TestDispViewers3::construct_averageLayoutView()
{
    // The settings path exercises the save and restore branches, which are the
    // ones most likely to fault on a key that has never been written before.
    AverageLayoutView* pView = new AverageLayoutView(settingsPath(), m_pHolder.data());
    QVERIFY(pView != nullptr);

    pView->resize(320, 240);
    pView->show();
    pView->saveSettings();

    delete pView;
}

//=============================================================================================================

void TestDispViewers3::construct_butterflyView()
{
    ButterflyView* pView = new ButterflyView(settingsPath(), m_pHolder.data());
    QVERIFY(pView != nullptr);

    pView->resize(320, 240);
    pView->show();
    pView->saveSettings();

    delete pView;
}

//=============================================================================================================

void TestDispViewers3::construct_control3DView()
{
    Control3DView* pView = new Control3DView(settingsPath(), m_pHolder.data());
    QVERIFY(pView != nullptr);

    pView->resize(320, 240);
    pView->show();
    pView->saveSettings();

    delete pView;
}

//=============================================================================================================

void TestDispViewers3::construct_artifactSettingsView()
{
    // An empty channel list is the interesting case: it is what the view gets
    // before any data is loaded, and it is where an unguarded index would fault.
    ArtifactSettingsView* pView = new ArtifactSettingsView(settingsPath(),
                                                          QList<FIFFLIB::FiffChInfo>(),
                                                          m_pHolder.data());
    QVERIFY(pView != nullptr);

    pView->resize(320, 240);
    pView->show();
    pView->saveSettings();

    delete pView;
}

//=============================================================================================================

void TestDispViewers3::construct_projectSettingsView()
{
    ProjectSettingsView* pView = new ProjectSettingsView(settingsPath(),
                                                         "/TestData",
                                                         "TestProject",
                                                         "TestSubject",
                                                         "UnknownParadigm",
                                                         m_pHolder.data());
    QVERIFY(pView != nullptr);

    pView->resize(320, 240);
    pView->show();
    pView->saveSettings();

    delete pView;
}

//=============================================================================================================

void TestDispViewers3::construct_timeRulerWidget()
{
    TimeRulerWidget* pWidget = new TimeRulerWidget(m_pHolder.data());
    QVERIFY(pWidget != nullptr);

    pWidget->resize(400, 40);
    pWidget->show();
    QVERIFY2(pWidget->size().isValid() && !pWidget->size().isEmpty(),
             "widget laid out to an empty geometry");

    delete pWidget;
}

//=============================================================================================================

void TestDispViewers3::construct_channelLabelPanel()
{
    ChannelLabelPanel* pWidget = new ChannelLabelPanel(m_pHolder.data());
    QVERIFY(pWidget != nullptr);

    pWidget->resize(120, 400);
    pWidget->show();
    QVERIFY2(pWidget->size().isValid() && !pWidget->size().isEmpty(),
             "widget laid out to an empty geometry");

    delete pWidget;
}

//=============================================================================================================

void TestDispViewers3::construct_overviewBarWidget()
{
    OverviewBarWidget* pWidget = new OverviewBarWidget(m_pHolder.data());
    QVERIFY(pWidget != nullptr);

    pWidget->resize(400, 60);
    pWidget->show();
    QVERIFY2(pWidget->size().isValid() && !pWidget->size().isEmpty(),
             "widget laid out to an empty geometry");

    delete pWidget;
}

//=============================================================================================================

void TestDispViewers3::cleanupTestCase()
{
    m_pHolder.reset();

    // The settings writes above are test artefacts, so they are removed rather
    // than left in the developer's real QSettings store.
    QSettings settings;
    settings.remove(settingsPath());
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_MAIN(TestDispViewers3)
#include "test_disp_viewers3.moc"
