//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_scdisp_widgets.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     July, 2026
 * @brief    Construction and update checks for the mne_scan real time display widgets.
 *
 * These widgets are built by mne_scan when a plugin produces data and are
 * destroyed when the scene changes, so both ends of that lifecycle happen
 * repeatedly during a session while none of it was covered.
 *
 * This is shallow, in the same way the disp viewer tests are: it builds each
 * widget, hands it an empty measurement of the type it displays, and tears it
 * down. It does not check that anything is drawn correctly. It does check the
 * ordering that actually bites, which is a widget receiving an update before
 * any real data has arrived, since that is the state between a plugin starting
 * and its first block being produced.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <scDisp/realtimeevokedsetwidget.h>
#include <scDisp/realtimemultisamplearraywidget.h>
#include <scDisp/realtimespectrumwidget.h>

#include <scMeas/realtimeevokedset.h>
#include <scMeas/realtimemultisamplearray.h>
#include <scMeas/realtimespectrum.h>
#include <scMeas/measurement.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QWidget>
#include <QTime>
#include <QSharedPointer>
#include <QScopedPointer>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCDISPLIB;
using namespace SCMEASLIB;

//=============================================================================================================
/**
 * DECLARE CLASS TestScDispWidgets
 *
 * @brief Construction and update checks for the mne_scan real time display widgets.
 */
class TestScDispWidgets: public QObject
{
    Q_OBJECT

public:
    TestScDispWidgets() = default;

private:
    QScopedPointer<QWidget> m_pHolder;
    QSharedPointer<QTime> m_pTime;

private slots:
    void initTestCase();
    void evokedSetWidget_buildsAndTearsDown();
    void multiSampleArrayWidget_buildsAndTearsDown();
    void spectrumWidget_buildsAndTearsDown();
    void widgets_surviveInitBeforeAnyData();
    void cleanupTestCase();
};

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

void TestScDispWidgets::initTestCase()
{
    m_pHolder.reset(new QWidget());
    m_pTime = QSharedPointer<QTime>(new QTime(0, 0, 0));
}

//=============================================================================================================

void TestScDispWidgets::evokedSetWidget_buildsAndTearsDown()
{
    RealTimeEvokedSetWidget* pWidget = new RealTimeEvokedSetWidget(m_pTime, m_pHolder.data());
    QVERIFY(pWidget != nullptr);

    pWidget->resize(640, 480);
    QVERIFY2(pWidget->size().isValid() && !pWidget->size().isEmpty(),
             "widget laid out to an empty geometry");

    delete pWidget;
}

//=============================================================================================================

void TestScDispWidgets::multiSampleArrayWidget_buildsAndTearsDown()
{
    RealTimeMultiSampleArrayWidget* pWidget = new RealTimeMultiSampleArrayWidget(m_pTime, m_pHolder.data());
    QVERIFY(pWidget != nullptr);

    pWidget->resize(640, 480);
    QVERIFY2(pWidget->size().isValid() && !pWidget->size().isEmpty(),
             "widget laid out to an empty geometry");

    delete pWidget;
}

//=============================================================================================================

void TestScDispWidgets::spectrumWidget_buildsAndTearsDown()
{
    QSharedPointer<RealTimeSpectrum> pSpectrum(new RealTimeSpectrum());

    RealTimeSpectrumWidget* pWidget = new RealTimeSpectrumWidget(pSpectrum, m_pTime, m_pHolder.data());
    QVERIFY(pWidget != nullptr);

    pWidget->resize(640, 480);
    QVERIFY2(pWidget->size().isValid() && !pWidget->size().isEmpty(),
             "widget laid out to an empty geometry");

    delete pWidget;
}

//=============================================================================================================

void TestScDispWidgets::widgets_surviveInitBeforeAnyData()
{
    // This is the ordering that bites in practice. mne_scan constructs the
    // widget when a plugin starts and pushes the measurement through update
    // before the plugin has produced a single block, so anything that assumes
    // populated data here faults on the first run rather than on odd input.
    //
    // Each widget is handed a measurement of the type it displays, empty, which
    // is what it really receives at that moment. A null measurement is passed
    // too, since a plugin that fails to start delivers exactly that.
    {
        RealTimeEvokedSetWidget widget(m_pTime, m_pHolder.data());
        widget.update(QSharedPointer<RealTimeEvokedSet>(new RealTimeEvokedSet()));
        widget.update(Measurement::SPtr());
    }
    {
        RealTimeMultiSampleArrayWidget widget(m_pTime, m_pHolder.data());
        widget.update(QSharedPointer<RealTimeMultiSampleArray>(new RealTimeMultiSampleArray()));
        widget.update(Measurement::SPtr());
    }
    {
        QSharedPointer<RealTimeSpectrum> pSpectrum(new RealTimeSpectrum());
        RealTimeSpectrumWidget widget(pSpectrum, m_pTime, m_pHolder.data());
        widget.update(pSpectrum);
        widget.update(Measurement::SPtr());
    }

    // Reaching here means none of the three faulted on an empty measurement,
    // which is the whole claim of this case.
    QVERIFY(true);
}

//=============================================================================================================

void TestScDispWidgets::cleanupTestCase()
{
    m_pHolder.reset();
    m_pTime.clear();
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_MAIN(TestScDispWidgets)
#include "test_scdisp_widgets.moc"
