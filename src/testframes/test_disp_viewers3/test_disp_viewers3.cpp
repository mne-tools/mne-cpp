//=============================================================================================================
/**
 * @file     test_disp_viewers3.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.9
 * @date     February, 2025
 *
 * @section  LICENSE
 *
 * Copyright (C) 2025, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MNE-CPP AUTHORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
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
