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
#include <disp/viewers/helpers/channeldatamodel.h>
#include <disp/viewers/helpers/channelrhiview.h>

#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QApplication>
#include <QHelpEvent>
#include <QSignalSpy>
#include <QToolTip>
#include <QWidget>

#include <memory>
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
class TestDispViewers3 : public QObject
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
    void channelLabelPanel_dragClickTooltipAndButterfly();
    void timeRulerAndOverviewBar_mapSamplesAndPaint();
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

namespace
{

// Six channels: MEG0..MEG2 (one bad), EEG0, EEG1, STI; 1000 Hz, samples 0..1999.
std::unique_ptr<ChannelDataModel> labelTestModel()
{
    QSharedPointer<FIFFLIB::FiffInfo> info(new FIFFLIB::FiffInfo);
    info->sfreq = 1000.0f;
    const QVector<std::pair<int, int>> kinds = {{FIFFV_MEG_CH, FIFF_UNIT_T}, {FIFFV_MEG_CH, FIFF_UNIT_T}, {FIFFV_MEG_CH, FIFF_UNIT_T}, {FIFFV_EEG_CH, FIFF_UNIT_V}, {FIFFV_EEG_CH, FIFF_UNIT_V}, {FIFFV_STIM_CH, FIFF_UNIT_NONE}};
    const QStringList names = {"MEG0", "MEG1", "MEG2", "EEG0", "EEG1", "STI"};
    for (int i = 0; i < kinds.size(); ++i) {
        FIFFLIB::FiffChInfo ch;
        ch.ch_name = names[i];
        ch.kind = kinds[i].first;
        ch.unit = kinds[i].second;
        info->chs.append(ch);
    }
    info->ch_names = names;
    info->nchan = names.size();
    info->bads = {QStringLiteral("MEG1")};

    auto model = std::make_unique<ChannelDataModel>();
    model->init(info);
    Eigen::MatrixXd data(6, 2000);
    for (int s = 0; s < 2000; ++s) {
        data.col(s) << 1e-12 * std::sin(0.01 * s), 0.0, 2e-12, 1e-5 * std::cos(0.02 * s), 0.0, (s % 500 == 0) ? 1.0 : 0.0;
    }
    model->setData(data, 0);
    return model;
}

} // namespace

//=============================================================================================================

void TestDispViewers3::channelLabelPanel_dragClickTooltipAndButterfly()
{
    auto model = labelTestModel();
    ChannelLabelPanel panel(m_pHolder.data());
    panel.resize(120, 300);
    panel.setModel(model.get());
    panel.setVisibleChannelCount(3);
    panel.setFirstVisibleChannel(1);
    panel.setVisibleSampleRange(0, 2000);
    panel.show();
    QVERIFY(!panel.grab().isNull());

    // Clicking a lane toggles that channel's bad flag; lane height is 300 / 3 = 100 px
    QSignalSpy badSpy(&panel, &ChannelLabelPanel::channelBadToggled);
    QTest::mouseClick(&panel, Qt::LeftButton, Qt::NoModifier, QPoint(50, 150));
    QCOMPARE(badSpy.size(), 1);
    QCOMPARE(badSpy.at(0).at(0).toInt(), 2);
    QVERIFY(badSpy.at(0).at(1).toBool());
    QVERIFY(model->channelInfo(2).bad);
    QTest::mouseClick(&panel, Qt::LeftButton, Qt::NoModifier, QPoint(50, 50));
    QVERIFY(!model->channelInfo(1).bad);

    // Dragging scrolls channels: up by 150 px = 1.5 lanes -> first channel 1 + 1 = 2, clamped at 6 - 3 = 3
    QSignalSpy scrollSpy(&panel, &ChannelLabelPanel::channelScrollRequested);
    QTest::mousePress(&panel, Qt::LeftButton, Qt::NoModifier, QPoint(50, 200));
    QTest::mouseMove(&panel, QPoint(50, 198));
    QCOMPARE(scrollSpy.size(), 0);
    QTest::mouseMove(&panel, QPoint(50, 50));
    QCOMPARE(scrollSpy.last().at(0).toInt(), 2);
    QTest::mouseMove(&panel, QPoint(50, -400));
    QCOMPARE(scrollSpy.last().at(0).toInt(), 3);
    QTest::mouseRelease(&panel, Qt::LeftButton, Qt::NoModifier, QPoint(50, -400));
    QCOMPARE(badSpy.size(), 2);

    // Hiding bad channels removes MEG2 from the lanes: lane 1 of {MEG1, EEG0, EEG1} is EEG0
    panel.setHideBadChannels(true);
    QTest::mouseClick(&panel, Qt::LeftButton, Qt::NoModifier, QPoint(50, 150));
    QCOMPARE(badSpy.last().at(0).toInt(), 3);
    panel.setHideBadChannels(false);
    model->setChannelBad(2, false);
    model->setChannelBad(3, false);

    // An explicit channel subset, then the tooltip for its middle lane
    panel.setChannelIndices({5, 3, 0});
    panel.setFirstVisibleChannel(0);
    QHelpEvent tip(QEvent::ToolTip, QPoint(50, 150), panel.mapToGlobal(QPoint(50, 150)));
    QVERIFY(QApplication::sendEvent(&panel, &tip));
    QTRY_VERIFY(QToolTip::isVisible());
    QVERIFY(QToolTip::text().startsWith(QStringLiteral("<b>EEG0</b>")));
    QVERIFY(QToolTip::text().contains(QStringLiteral("Type: EEG")));
    QToolTip::hideText();
    QVERIFY(!panel.grab().isNull());

    panel.setButterflyMode(true);
    QVERIFY(!panel.grab().isNull());
    QVERIFY(panel.sizeHint().isValid());
    QVERIFY(panel.minimumSizeHint().isValid());
}

//=============================================================================================================

void TestDispViewers3::timeRulerAndOverviewBar_mapSamplesAndPaint()
{
    TimeRulerWidget ruler(m_pHolder.data());
    ruler.resize(800, TimeRulerWidget::kTotalH);
    ruler.setSfreq(1000.0);
    ruler.setFirstFileSample(100);
    ruler.setEvents({TimeRulerEventMark{300, Qt::red, QStringLiteral("1")},
                     TimeRulerEventMark{302, Qt::blue, QString()},
                     TimeRulerEventMark{900, Qt::green, QStringLiteral("2")}});
    ruler.setReferenceMarkers({TimeRulerReferenceMark{400, Qt::magenta, QStringLiteral("M1")}});
    ruler.show();
    // Every zoom level from 50 ms to minutes per tick, with seconds and with clock labels
    for (const float spp : {0.05f, 0.5f, 5.0f, 50.0f, 500.0f}) {
        ruler.setSamplesPerPixel(spp);
        ruler.setScrollSample(100.0f + 200.0f * spp);
        for (const bool clock : {false, true}) {
            ruler.setClockTimeFormat(clock);
            QCOMPARE(ruler.clockTimeFormat(), clock);
            QVERIFY(!ruler.grab().isNull());
        }
    }
    ruler.toggleTimeFormat();
    QVERIFY(!ruler.clockTimeFormat());

    // An hour into the recording a 5-sample scroll still moves the event mark 10 px
    ruler.setSamplesPerPixel(0.5f);
    ruler.setEvents({TimeRulerEventMark{3600200, Qt::red, QStringLiteral("1")}});
    ruler.setScrollSample(3600000.0f);
    const QImage before = ruler.grab().toImage();
    ruler.setScrollSample(3600005.0f);
    QVERIFY(ruler.grab().toImage() != before);

    OverviewBarWidget bar(m_pHolder.data());
    QSignalSpy scrollSpy(&bar, &OverviewBarWidget::scrollRequested);
    bar.resize(400, bar.sizeHint().height());
    QTest::mouseClick(&bar, Qt::LeftButton, Qt::NoModifier, QPoint(200, 5));
    QCOMPARE(scrollSpy.takeLast().at(0).toFloat(), 0.0f);

    auto model = labelTestModel();
    bar.setModel(model.get());
    bar.setSfreq(1000.0f);
    bar.setFirstFileSample(0);
    bar.setLastFileSample(2000);
    bar.setViewport(500.0f, 200.0f);
    bar.setEvents({ChannelRhiView::EventMarker{500, 1, Qt::red, QStringLiteral("1")}});
    bar.setAnnotations({ChannelRhiView::AnnotationSpan{800, 1000, Qt::yellow, QStringLiteral("bad")}});
    bar.show();
    QVERIFY(!bar.grab().isNull());

    // x maps linearly onto [first, last]; the request centres the viewport there
    QTest::mousePress(&bar, Qt::LeftButton, Qt::NoModifier, QPoint(100, 5));
    QCOMPARE(scrollSpy.takeLast().at(0).toFloat(), 500.0f - 100.0f);
    QTest::mouseMove(&bar, QPoint(300, 5));
    QCOMPARE(scrollSpy.takeLast().at(0).toFloat(), 1500.0f - 100.0f);
    QTest::mouseMove(&bar, QPoint(900, 5));
    QCOMPARE(scrollSpy.takeLast().at(0).toFloat(), 2000.0f - 100.0f);
    QTest::mouseRelease(&bar, Qt::LeftButton, Qt::NoModifier, QPoint(900, 5));
    QTest::mouseMove(&bar, QPoint(10, 5));
    QVERIFY(scrollSpy.isEmpty());

    bar.resize(600, bar.sizeHint().height());
    QVERIFY(!bar.grab().isNull());
    QVERIFY(bar.minimumSizeHint().isValid());
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
