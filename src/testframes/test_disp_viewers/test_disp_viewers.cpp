//=============================================================================================================
/**
 * @file     test_disp_viewers.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Tests for the disp viewer widget classes: construction, getter/setter round-trips,
 *           settings persistence, GuiMode/ProcessingMode switching, and clearView.
 *           Covers: ScalingView, CompensatorView, ProjectorsView, ModalitySelectionView,
 *           MinimumNormSettingsView, FilterSettingsView, FilterDesignView,
 *           SpectrumSettingsView, AveragingSettingsView, CovarianceSettingsView,
 *           SpharaSettingsView, TfSettingsView, ConnectivitySettingsView,
 *           TriggerDetectionView, ProgressView, QuickControlView, and MultiView.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <disp/viewers/abstractview.h>
#include <disp/viewers/scalingview.h>
#include <disp/viewers/compensatorview.h>
#include <disp/viewers/projectorsview.h>
#include <disp/viewers/modalityselectionview.h>
#include <disp/viewers/minimumnormsettingsview.h>
#include <disp/viewers/filtersettingsview.h>
#include <disp/viewers/filterdesignview.h>
#include <disp/viewers/spectrumsettingsview.h>
#include <disp/viewers/averagingsettingsview.h>
#include <disp/viewers/covariancesettingsview.h>
#include <disp/viewers/spharasettingsview.h>
#include <disp/viewers/tfsettingsview.h>
#include <disp/viewers/connectivitysettingsview.h>
#include <disp/viewers/triggerdetectionview.h>
#include <disp/viewers/progressview.h>
#include <disp/viewers/quickcontrolview.h>
#include <disp/viewers/multiview.h>

#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_proj.h>
#include <fiff/fiff_ctf_comp.h>
#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_constants.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QApplication>
#include <QStringList>
#include <QMap>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace FIFFLIB;

//=============================================================================================================
/**
 * DECLARE CLASS TestDispViewers
 *
 * @brief The TestDispViewers class tests the disp viewer widget classes under an offscreen platform.
 *
 */
class TestDispViewers : public QObject
{
    Q_OBJECT

private slots:

    //=========================================================================================================
    /**
     * Called once before any test function.
     */
    void initTestCase();

    //=========================================================================================================
    /**
     * Called once after all test functions have run (no-op).
     */
    void cleanupTestCase();

    //=========================================================================================================
    /**
     * Verifies the getDefaultScalingValue free function and ScalingView getter/setter round-trip.
     */
    void scalingView_constructAndGetSet();

    //=========================================================================================================
    /**
     * Verifies that CompensatorView accepts an empty and populated compensator list and
     * returns consistent values.
     */
    void compensatorView_setAndGet();

    //=========================================================================================================
    /**
     * Verifies that ProjectorsView accepts projectors, returns them, and survives lifecycle calls.
     */
    void projectorsView_setAndGet();

    //=========================================================================================================
    /**
     * Verifies that ModalitySelectionView builds a modality map from a channel list and
     * round-trips setModalityMap.
     */
    void modalitySelectionView_setAndGet();

    //=========================================================================================================
    /**
     * Verifies that MinimumNormSettingsView accepts trigger type lists and survives
     * all GuiMode/ProcessingMode transitions.
     */
    void minimumNormSettingsView_lifecycle();

    //=========================================================================================================
    /**
     * Verifies that FilterSettingsView constructs and survives lifecycle transitions.
     */
    void filterSettingsView_lifecycle();

    //=========================================================================================================
    /**
     * Verifies that FilterDesignView constructs, setSamplingRate/setFrom/setTo/setChannelType
     * do not crash, getChannelType returns a non-empty string, and the view survives
     * lifecycle transitions.
     */
    void filterDesignView_setAndGet();

    //=========================================================================================================
    /**
     * Verifies that SpectrumSettingsView accepts boundaries and updateValue without crashing.
     */
    void spectrumSettingsView_boundaries();

    //=========================================================================================================
    /**
     * Verifies that AveragingSettingsView constructs with a stim-channel map, getters return
     * sensible values, and setStimChannels / setDetectedEpochs / lifecycle calls do not crash.
     */
    void averagingSettingsView_setAndGet();

    //=========================================================================================================
    /**
     * Verifies that CovarianceSettingsView survives all lifecycle transitions.
     */
    void covarianceSettingsView_lifecycle();

    //=========================================================================================================
    /**
     * Verifies that SpharaSettingsView survives all lifecycle transitions.
     */
    void spharaSettingsView_lifecycle();

    //=========================================================================================================
    /**
     * Verifies that TfSettingsView survives all lifecycle transitions.
     */
    void tfSettingsView_lifecycle();

    //=========================================================================================================
    /**
     * Verifies that ConnectivitySettingsView getters return non-empty defaults, setters do not
     * crash, and the view survives lifecycle transitions.
     */
    void connectivitySettingsView_setAndGet();

    //=========================================================================================================
    /**
     * Verifies that TriggerDetectionView survives all lifecycle transitions.
     */
    void triggerDetectionView_lifecycle();

    //=========================================================================================================
    /**
     * Verifies that ProgressView constructs in both layouts, updateProgress / setMessage /
     * setLoadingBarVisible do not crash, and clearView works.
     */
    void progressView_updateProgress();

    //=========================================================================================================
    /**
     * Verifies that QuickControlView constructs, addGroupBox and addGroupBoxWithTabs succeed,
     * and saveSettings / loadSettings do not crash.
     */
    void quickControlView_addGroupBox();

    //=========================================================================================================
    /**
     * Verifies that MultiView constructs, addWidgetTop / addWidgetBottom succeed, and
     * saveSettings / loadSettings do not crash.
     */
    void multiView_addWidget();
};

//=============================================================================================================

void TestDispViewers::initTestCase()
{
    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers::cleanupTestCase() {}

//=============================================================================================================

void TestDispViewers::scalingView_constructAndGetSet()
{
    // Free function: getDefaultScalingValue(kind, unit)
    float megMagDefault = DISPLIB::getDefaultScalingValue(FIFFV_MEG_CH, FIFF_UNIT_T);
    float megGradDefault = DISPLIB::getDefaultScalingValue(FIFFV_MEG_CH, FIFF_UNIT_T_M);
    float eegDefault = DISPLIB::getDefaultScalingValue(FIFFV_EEG_CH, FIFF_UNIT_V);
    QVERIFY(megMagDefault > 0.0f);
    QVERIFY(megGradDefault > 0.0f);
    QVERIFY(eegDefault > 0.0f);

    ScalingView view("test_disp_viewers");

    // getScaleMap may be empty until channels are added — call must not crash
    QMap<qint32, float> scaleMap = view.getScaleMap();
    Q_UNUSED(scaleMap);

    // setScaleMap round-trip
    QMap<qint32, float> customMap;
    customMap[FIFFV_MEG_CH] = 1e-12f;
    customMap[FIFFV_EEG_CH] = 1e-6f;
    view.setScaleMap(customMap);
    QMap<qint32, float> returned = view.getScaleMap();
    QCOMPARE(returned.value(FIFFV_MEG_CH, 0.0f), 1e-12f);
    QCOMPARE(returned.value(FIFFV_EEG_CH, 0.0f), 1e-6f);

    view.setGuiMode(AbstractView::GuiMode::Clinical);
    view.setGuiMode(AbstractView::GuiMode::Research);
    view.setProcessingMode(AbstractView::ProcessingMode::RealTime);
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    view.saveSettings();
    view.loadSettings();
    view.clearView();

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers::compensatorView_setAndGet()
{
    CompensatorView view("test_disp_viewers");

    // Empty list
    view.setCompensators(QList<FiffCtfComp>());
    QVERIFY(view.getCompensators().isEmpty());
    QCOMPARE(view.getLastTo(), 0);

    // Non-empty list
    FiffCtfComp comp;
    comp.ctfkind = 101;
    comp.kind    = FIFF_MNE_CTF_COMP_KIND;
    view.setCompensators(QList<FiffCtfComp>() << comp);

    view.setGuiMode(AbstractView::GuiMode::Research);
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    view.saveSettings();
    view.loadSettings();
    view.clearView();

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers::projectorsView_setAndGet()
{
    ProjectorsView view("test_disp_viewers");

    // Empty list
    view.setProjectors(QList<FiffProj>());
    QVERIFY(view.getProjectors().isEmpty());

    // Non-empty list
    FiffProj proj;
    proj.kind   = FIFFV_PROJ_ITEM_EEG_AVREF;
    proj.active = false;
    proj.desc   = "Test projector";
    view.setProjectors(QList<FiffProj>() << proj);
    QCOMPARE(view.getProjectors().size(), 1);
    QVERIFY(!view.getProjectors().first().active);

    view.redrawGUI();
    view.setGuiMode(AbstractView::GuiMode::Clinical);
    view.setProcessingMode(AbstractView::ProcessingMode::RealTime);
    view.saveSettings();
    view.loadSettings();
    view.clearView();

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers::modalitySelectionView_setAndGet()
{
    FiffChInfo megCh;
    megCh.kind    = FIFFV_MEG_CH;
    megCh.ch_name = "MEG 0001";
    megCh.unit    = FIFF_UNIT_T;

    FiffChInfo eegCh;
    eegCh.kind    = FIFFV_EEG_CH;
    eegCh.ch_name = "EEG 001";
    eegCh.unit    = FIFF_UNIT_V;

    QList<FiffChInfo> chList;
    chList << megCh << eegCh;

    ModalitySelectionView view(chList, "test_disp_viewers");

    QMap<QString, bool> modalMap = view.getModalityMap();
    QVERIFY(!modalMap.isEmpty());

    // Flip all to false and round-trip
    QMap<QString, bool> allOff;
    for(auto it = modalMap.constBegin(); it != modalMap.constEnd(); ++it)
        allOff[it.key()] = false;
    view.setModalityMap(allOff);

    QMap<QString, bool> returned = view.getModalityMap();
    for(auto it = returned.constBegin(); it != returned.constEnd(); ++it)
        QVERIFY(!it.value());

    view.setGuiMode(AbstractView::GuiMode::Research);
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    view.saveSettings();
    view.loadSettings();
    view.clearView();

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers::minimumNormSettingsView_lifecycle()
{
    MinimumNormSettingsView view("test_disp_viewers");

    view.setTriggerTypes(QStringList() << "1" << "2" << "face" << "scrambled");

    view.setGuiMode(AbstractView::GuiMode::Clinical);
    view.setGuiMode(AbstractView::GuiMode::Research);
    view.setProcessingMode(AbstractView::ProcessingMode::RealTime);
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    view.saveSettings();
    view.loadSettings();
    view.clearView();

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers::filterSettingsView_lifecycle()
{
    FilterSettingsView view("test_disp_viewers");

    view.setGuiMode(AbstractView::GuiMode::Clinical);
    view.setGuiMode(AbstractView::GuiMode::Research);
    view.setProcessingMode(AbstractView::ProcessingMode::RealTime);
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    view.saveSettings();
    view.loadSettings();
    view.clearView();

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers::filterDesignView_setAndGet()
{
    FilterDesignView view("test_disp_viewers");

    view.setSamplingRate(600.0);
    view.setMaxAllowedFilterTaps(512);
    view.setFrom(1.0);
    view.setTo(40.0);

    int taps = view.getFilterTaps();
    QVERIFY(taps > 0);

    QString chType = view.getChannelType();
    QVERIFY(!chType.isEmpty());

    view.setChannelType("MEG");

    view.setGuiMode(AbstractView::GuiMode::Research);
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    view.saveSettings();
    view.loadSettings();
    view.clearView();

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers::spectrumSettingsView_boundaries()
{
    SpectrumSettingsView view("test_disp_viewers");

    // Typical MEG/EEG frequency range at 600 Hz sfreq
    view.setBoundaries(600.0f, 0.1f, 100.0f);

    view.updateValue(50);
    view.updateValue(0);
    view.updateValue(100);

    view.setGuiMode(AbstractView::GuiMode::Research);
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    view.saveSettings();
    view.loadSettings();
    view.clearView();

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers::averagingSettingsView_setAndGet()
{
    QMap<QString, int> stimMap;
    stimMap["STI 014"] = 0;
    stimMap["STI 001"] = 1;

    AveragingSettingsView view("test_disp_viewers", stimMap);

    QVERIFY(view.getNumAverages() > 0);
    QVERIFY(view.getPreStimMSeconds() >= 0);
    QVERIFY(view.getPostStimMSeconds() >= 0);
    QVERIFY(view.getStimChannelIdx() >= 0);
    Q_UNUSED(view.getCurrentStimCh());
    Q_UNUSED(view.getCurrentSelectGroup());
    Q_UNUSED(view.getBaselineFromSeconds());
    Q_UNUSED(view.getBaselineToSeconds());

    // Update stim channels
    QMap<QString, int> newStimMap;
    newStimMap["STI 016"] = 2;
    view.setStimChannels(newStimMap);

    // setDetectedEpochs with empty set must not crash
    FiffEvokedSet emptySet;
    view.setDetectedEpochs(emptySet);

    view.clearSelectionGroup();
    view.setGuiMode(AbstractView::GuiMode::Research);
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    view.saveSettings();
    view.loadSettings();
    view.clearView();

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers::covarianceSettingsView_lifecycle()
{
    CovarianceSettingsView view("test_disp_viewers");

    view.setGuiMode(AbstractView::GuiMode::Clinical);
    view.setGuiMode(AbstractView::GuiMode::Research);
    view.setProcessingMode(AbstractView::ProcessingMode::RealTime);
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    view.saveSettings();
    view.loadSettings();
    view.clearView();

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers::spharaSettingsView_lifecycle()
{
    SpharaSettingsView view("test_disp_viewers");

    view.setGuiMode(AbstractView::GuiMode::Clinical);
    view.setGuiMode(AbstractView::GuiMode::Research);
    view.setProcessingMode(AbstractView::ProcessingMode::RealTime);
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    view.saveSettings();
    view.loadSettings();
    view.clearView();

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers::tfSettingsView_lifecycle()
{
    TfSettingsView view("test_disp_viewers");

    view.setGuiMode(AbstractView::GuiMode::Clinical);
    view.setGuiMode(AbstractView::GuiMode::Research);
    view.setProcessingMode(AbstractView::ProcessingMode::RealTime);
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    view.saveSettings();
    view.loadSettings();
    view.clearView();

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers::connectivitySettingsView_setAndGet()
{
    ConnectivitySettingsView view("test_disp_viewers");

    QString metric = view.getConnectivityMetric();
    QVERIFY(!metric.isEmpty());

    QString windowType = view.getWindowType();
    QVERIFY(!windowType.isEmpty());

    int nTrials = view.getNumberTrials();
    QVERIFY(nTrials >= 0);
    Q_UNUSED(view.getTriggerType());

    view.setTriggerTypes(QStringList() << "1" << "2");
    view.setNumberTrials(10);
    view.setFrequencyBand(1.0, 40.0);
    QCOMPARE(view.getNumberTrials(), 10);

    view.setGuiMode(AbstractView::GuiMode::Research);
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    view.saveSettings();
    view.loadSettings();
    view.clearView();

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers::triggerDetectionView_lifecycle()
{
    TriggerDetectionView view("test_disp_viewers");

    view.setGuiMode(AbstractView::GuiMode::Clinical);
    view.setGuiMode(AbstractView::GuiMode::Research);
    view.setProcessingMode(AbstractView::ProcessingMode::RealTime);
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    view.saveSettings();
    view.loadSettings();
    view.clearView();

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers::progressView_updateProgress()
{
    // Horizontal message layout
    {
        ProgressView view(true);
        view.setMessage("Processing...");
        view.updateProgress(0);
        view.updateProgress(50, "Half done");
        view.updateProgress(100, "Complete");
        view.setLoadingBarVisible(true);
        view.setLoadingBarVisible(false);
        view.setHorizontal();
        view.setVertical();
        view.setGuiMode(AbstractView::GuiMode::Research);
        view.setProcessingMode(AbstractView::ProcessingMode::Offline);
        view.saveSettings();
        view.loadSettings();
        view.clearView();
        QApplication::processEvents();
    }

    // Vertical message layout
    {
        ProgressView view(false);
        view.setMessage("Done.");
        view.updateProgress(75);
        view.clearView();
        QApplication::processEvents();
    }
}

//=============================================================================================================

void TestDispViewers::quickControlView_addGroupBox()
{
    QuickControlView view("test_disp_viewers", "Test Controls");

    // addWidget(widget, tabName)
    QWidget* child1 = new QWidget(&view);
    view.addWidget(child1, "Settings");

    // addGroupBox(widget, groupName, tabName)
    QWidget* child2 = new QWidget(&view);
    view.addGroupBox(child2, "Test Group", "Settings");

    // addGroupBoxWithTabs(widget, groupName, tabNameInsideGroup, tabName)
    QWidget* child3 = new QWidget(&view);
    view.addGroupBoxWithTabs(child3, "Tabs Group", "Sub-Tab", "Settings");

    view.setOpacityValue(80);
    view.setVisiblityHideOpacityClose(true);

    view.saveSettings();
    view.loadSettings();
    view.clear();

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers::multiView_addWidget()
{
    MultiView view("test_disp_viewers");

    // addWidgetTop
    QWidget* child1 = new QWidget();
    child1->setWindowTitle("TopWidget");
    view.addWidgetTop(child1, "Top Dock");

    // addWidgetBottom
    QWidget* child2 = new QWidget();
    child2->setWindowTitle("BottomWidget");
    view.addWidgetBottom(child2, "Bottom Dock");

    view.saveSettings();
    view.loadSettings();

    QApplication::processEvents();
}

//=============================================================================================================

QTEST_MAIN(TestDispViewers)
#include "test_disp_viewers.moc"
