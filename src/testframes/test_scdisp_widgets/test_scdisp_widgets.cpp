//=============================================================================================================
/**
 * @file     test_scdisp_widgets.cpp
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
