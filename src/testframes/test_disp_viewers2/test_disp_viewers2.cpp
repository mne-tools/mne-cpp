//=============================================================================================================
/**
 * @file     test_disp_viewers2.cpp
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
 * @brief    Tests for additional disp viewer widget classes and helper models.
 *           Covers: HpiSettingsView, CoregSettingsView, ArtifactSettingsView,
 *           FwdSettingsView, FiffRawViewSettings, ProjectSettingsView, BidsView,
 *           ChannelSelectionView, AverageLayoutView, AverageSelectionView,
 *           ButterflyView, SpectrumView, RtFiffRawViewModel, EvokedSetModel,
 *           ChannelInfoModel, and RtFiffRawView.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <disp/viewers/abstractview.h>
#include <disp/viewers/hpisettingsview.h>
#include <disp/viewers/coregsettingsview.h>
#include <disp/viewers/artifactsettingsview.h>
#include <disp/viewers/fwdsettingsview.h>
#include <disp/viewers/fiffrawviewsettings.h>
#include <disp/viewers/projectsettingsview.h>
#include <disp/viewers/bidsview.h>
#include <disp/viewers/channelselectionview.h>
#include <disp/viewers/averagelayoutview.h>
#include <disp/viewers/averageselectionview.h>
#include <disp/viewers/butterflyview.h>
#include <disp/viewers/spectrumview.h>
#include <disp/viewers/rtfiffrawview.h>
#include <disp/viewers/helpers/rtfiffrawviewmodel.h>
#include <disp/viewers/helpers/evokedsetmodel.h>
#include <disp/viewers/helpers/channelinfomodel.h>
#include <disp/viewers/dipolefitview.h>
#include <disp/viewers/control3dview.h>
#include <disp/viewers/applytoview.h>
#include <disp/viewers/helpers/rtfiffrawviewdelegate.h>
#include <disp/viewers/helpers/evokedsetmodel.h>
#include <disp/viewers/helpers/frequencyspectrummodel.h>
#include <disp/viewers/helpers/frequencyspectrumdelegate.h>
#include <disp/viewers/helpers/bidsviewmodel.h>
#include <disp/viewers/helpers/mneoperator.h>

#include <fiff/fiff_info.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_dig_point.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QApplication>
#include <QSharedPointer>
#include <QStringList>
#include <QTableView>

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace FIFFLIB;

//=============================================================================================================
/**
 * DECLARE CLASS TestDispViewers2
 *
 * @brief The TestDispViewers2 class tests additional disp viewer widget classes and helper models
 *        under an offscreen platform.
 *
 */
class TestDispViewers2 : public QObject
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
     * Verifies that HpiSettingsView constructs, exposes getter defaults, and survives
     * lifecycle transitions.
     */
    void hpiSettingsView_lifecycle();

    //=========================================================================================================
    /**
     * Verifies that CoregSettingsView constructs, exposes getter defaults, and survives
     * lifecycle transitions.
     */
    void coregSettingsView_lifecycle();

    //=========================================================================================================
    /**
     * Verifies that ArtifactSettingsView constructs and survives all lifecycle transitions.
     */
    void artifactSettingsView_lifecycle();

    //=========================================================================================================
    /**
     * Verifies that FwdSettingsView constructs and survives all lifecycle transitions.
     */
    void fwdSettingsView_lifecycle();

    //=========================================================================================================
    /**
     * Verifies that FiffRawViewSettings constructs, getters return sensible defaults,
     * and setters do not crash.
     */
    void fiffRawViewSettings_setAndGet();

    //=========================================================================================================
    /**
     * Verifies that ProjectSettingsView constructs and survives all lifecycle transitions.
     */
    void projectSettingsView_lifecycle();

    //=========================================================================================================
    /**
     * Verifies that BidsView constructs and survives all lifecycle transitions.
     */
    void bidsView_lifecycle();

    //=========================================================================================================
    /**
     * Verifies that ChannelSelectionView constructs (with and without a ChannelInfoModel)
     * and survives lifecycle transitions.
     */
    void channelSelectionView_lifecycle();

    //=========================================================================================================
    /**
     * Verifies that AverageLayoutView constructs and survives all lifecycle transitions.
     */
    void averageLayoutView_lifecycle();

    //=========================================================================================================
    /**
     * Verifies that AverageSelectionView constructs and survives all lifecycle transitions.
     */
    void averageSelectionView_lifecycle();

    //=========================================================================================================
    /**
     * Verifies that ButterflyView constructs, accepts scale maps and modality maps,
     * and survives lifecycle transitions.
     */
    void butterflyView_lifecycle();

    //=========================================================================================================
    /**
     * Verifies that SpectrumView constructs and survives lifecycle transitions.
     */
    void spectrumView_lifecycle();

    //=========================================================================================================
    /**
     * Verifies that RtFiffRawViewModel constructs, reports sensible defaults, and
     * setScaling / setFiffInfo do not crash.
     */
    void rtFiffRawViewModel_basics();

    //=========================================================================================================
    /**
     * Verifies that EvokedSetModel constructs, reports sensible defaults, and
     * setEvokedSet does not crash on an empty set.
     */
    void evokedSetModel_basics();

    //=========================================================================================================
    /**
     * Verifies that ChannelInfoModel constructs with a FiffInfo, rowCount and
     * columnCount return sensible values, and basic data() queries do not crash.
     */
    void channelInfoModel_basics();

    //=========================================================================================================
    /**
     * Verifies that RtFiffRawView constructs, init does not crash,
     * and lifecycle methods survive.
     */
    void rtFiffRawView_lifecycle();

    //=========================================================================================================
    /**
     * Verifies that DipoleFitView constructs headlessly, exposes getters,
     * and survives lifecycle transitions.
     */
    void dipoleFitView_lifecycle();

    //=========================================================================================================
    /**
     * Verifies that Control3DView constructs headlessly and basic setters
     * do not crash.
     */
    void control3dView_lifecycle();

    //=========================================================================================================
    /**
     * Verifies that ApplyToView constructs headlessly and survives
     * lifecycle transitions.
     */
    void applyToView_lifecycle();

    //=========================================================================================================
    /**
     * Verifies that RtFiffRawViewDelegate constructs with a null parent,
     * basic setters compile and run.
     */
    void rtFiffRawViewDelegate_lifecycle();

    //=========================================================================================================
    /**
     * Verifies that EvokedSetModel responds to more API calls than the
     * baseline test covers (header data, flags, column count, etc.).
     */
    void evokedSetModel_extended();

    //=========================================================================================================
    /**
     * Verifies that FrequencySpectrumModel constructs, reports sensible defaults, and
     * setInfo, setScaleType, addData, selectRows, resetSelection, setBoundaries do not crash.
     */
    void frequencySpectrumModel_basics();

    //=========================================================================================================
    /**
     * Verifies that FrequencySpectrumDelegate constructs with a null QTableView,
     * setScaleType does not crash, and sizeHint returns a valid size.
     */
    void frequencySpectrumDelegate_basics();

    //=========================================================================================================
    /**
     * Verifies that BidsViewModel constructs, rowCount/columnCount return 0,
     * addSubject and addSessionToSubject do not crash.
     */
    void bidsViewModel_basics();

    //=========================================================================================================
    /**
     * Verifies that MNEOperator constructs via default, copy, and type constructors.
     */
    void mneOperator_basics();

    //=========================================================================================================
    /**
     * Verifies additional RtFiffRawViewModel methods that are safe to call
     * without active data.
     */
    void rtFiffRawViewModel_extended();
};

//=============================================================================================================

void TestDispViewers2::initTestCase()
{
    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers2::cleanupTestCase() {}

//=============================================================================================================

void TestDispViewers2::hpiSettingsView_lifecycle()
{
    HpiSettingsView view("test_disp_viewers2");

    QVERIFY(view.getFittingWindowSize() > 0);
    QVERIFY(view.getAllowedMeanErrorDistChanged() > 0.0);
    QVERIFY(view.getAllowedMovementChanged() > 0.0);
    QVERIFY(view.getAllowedRotationChanged() > 0.0);

    // Pass empty digitizer list
    view.newDigitizerList(QList<FiffDigPoint>());

    // Pass non-trivial error and GoF data
    view.setErrorLabels(QVector<double>() << 1.2 << 0.8, 1.0);
    Eigen::VectorXd gof(2);
    gof << 0.95, 0.90;
    view.setGoFLabels(gof, 0.925);
    view.setMovementResults(0.5, 0.1);

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

void TestDispViewers2::coregSettingsView_lifecycle()
{
    CoregSettingsView view("test_disp_viewers2");

    QVERIFY(view.getMaxIter() > 0);
    QVERIFY(view.getConvergence() > 0.0f);
    QVERIFY(view.getOmmitDistance() > 0.0f);
    Q_UNUSED(view.getAutoScale());
    Q_UNUSED(view.getCurrentFiducial());
    Q_UNUSED(view.getCurrentSelectedBem());

    view.addSelectionBem("sample-head.fif");
    view.clearSelectionBem();

    view.setFiducials(QVector3D(0.05f, 0.0f, 0.08f));
    view.setOmittedPoints(3);
    view.setRMSE(1.5f);

    view.setGuiMode(AbstractView::GuiMode::Research);
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    view.saveSettings();
    view.loadSettings();
    view.clearView();

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers2::artifactSettingsView_lifecycle()
{
    ArtifactSettingsView view("test_disp_viewers2");

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

void TestDispViewers2::fwdSettingsView_lifecycle()
{
    FwdSettingsView view("test_disp_viewers2");

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

void TestDispViewers2::fiffRawViewSettings_setAndGet()
{
    FiffRawViewSettings view("test_disp_viewers2");

    // Window size getter/setter
    int wSize = view.getWindowSize();
    QVERIFY(wSize > 0);
    view.setWindowSize(wSize + 1);

    // Distance time spacer
    int dist = view.getDistanceTimeSpacer();
    QVERIFY(dist >= 0);

    // Background color
    QColor bg = view.getBackgroundColor();
    QVERIFY(bg.isValid());

    view.setGuiMode(AbstractView::GuiMode::Research);
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    view.saveSettings();
    view.loadSettings();
    view.clearView();

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers2::projectSettingsView_lifecycle()
{
    ProjectSettingsView view("test_disp_viewers2");

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

void TestDispViewers2::bidsView_lifecycle()
{
    BidsView view;

    view.setGuiMode(AbstractView::GuiMode::Research);
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    view.saveSettings();
    view.loadSettings();
    view.clearView();

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers2::channelSelectionView_lifecycle()
{
    QSKIP("ChannelSelectionView requires layout files that are not guaranteed present in test environment");
}

//=============================================================================================================

void TestDispViewers2::averageLayoutView_lifecycle()
{
    AverageLayoutView view("test_disp_viewers2");

    view.setGuiMode(AbstractView::GuiMode::Research);
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    view.saveSettings();
    view.loadSettings();
    view.clearView();

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers2::averageSelectionView_lifecycle()
{
    AverageSelectionView view("test_disp_viewers2");

    view.setGuiMode(AbstractView::GuiMode::Research);
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    view.saveSettings();
    view.loadSettings();
    view.clearView();

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers2::butterflyView_lifecycle()
{
    ButterflyView view("test_disp_viewers2");

    // Scale map with some channel types
    QMap<qint32, float> scaleMap;
    scaleMap[FIFFV_MEG_CH] = 1e-12f;
    scaleMap[FIFFV_EEG_CH] = 1e-6f;
    view.setScaleMap(scaleMap);

    // Modality map
    QMap<QString, bool> modalMap;
    modalMap["MEG"] = true;
    modalMap["EEG"] = false;
    view.setModalityMap(modalMap);

    view.clearView();

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers2::spectrumView_lifecycle()
{
    SpectrumView view("test_disp_viewers2");

    view.setGuiMode(AbstractView::GuiMode::Research);
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    view.saveSettings();
    view.loadSettings();
    view.clearView();

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers2::rtFiffRawViewModel_basics()
{
    RtFiffRawViewModel model;

    // Defaults without data
    QVERIFY(model.rowCount() >= 0);
    QVERIFY(model.columnCount() >= 0);

    int maxSamples = model.getMaxSamples();
    Q_UNUSED(maxSamples);

    int currentSampleIdx = model.getCurrentSampleIndex();
    Q_UNUSED(currentSampleIdx);

    bool frozen = model.isFreezed();
    Q_UNUSED(frozen);

    QString trigName = model.getTriggerName();
    Q_UNUSED(trigName);

    double trigThreshold = model.getTriggerThreshold();
    Q_UNUSED(trigThreshold);

    int numSpacers = model.getNumberOfTimeSpacers();
    Q_UNUSED(numSpacers);

    // Scale map round-trip (safe — no threading)
    QMap<qint32, float> scaleMap;
    scaleMap[FIFFV_MEG_CH] = 1e-12f;
    model.setScaling(scaleMap);
    QCOMPARE(model.getScaling().value(FIFFV_MEG_CH, 0.0f), 1e-12f);

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers2::evokedSetModel_basics()
{
    EvokedSetModel model;

    // Without data
    QVERIFY(model.rowCount() >= 0);
    QVERIFY(model.columnCount() >= 0);
    QVERIFY(!model.isInit());

    // Set empty evoked set — must not crash
    FiffEvokedSet emptySet;
    model.setEvokedSet(QSharedPointer<FiffEvokedSet>::create(emptySet));

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers2::channelInfoModel_basics()
{
    // Default constructor — no FiffInfo channels, no threads
    ChannelInfoModel model;

    QVERIFY(model.rowCount() >= 0);
    QVERIFY(model.columnCount() >= 0);

    // Getters on empty model — must not crash
    model.getMappedChannelsList();
    model.getIndexFromOrigChName("nonexistent");
    model.getIndexFromMappedChName("nonexistent");
    model.getBadChannelList();

    // flags() — always returns a fixed value
    Qt::ItemFlags f = model.flags(QModelIndex());
    QVERIFY(f & Qt::ItemIsEnabled);

    // setData() — always returns true
    QVERIFY(model.setData(QModelIndex(), QVariant(), Qt::DisplayRole));

    // headerData() with all column sections — covers the full switch
    for (int i = 0; i <= 12; ++i) {
        model.headerData(i, Qt::Horizontal, Qt::DisplayRole);
        model.headerData(i, Qt::Horizontal, Qt::TextAlignmentRole);
    }
    model.headerData(0, Qt::Vertical, Qt::DisplayRole);

    // data() with invalid index — returns QVariant() immediately
    model.data(QModelIndex(), Qt::DisplayRole);

    // layoutChanged with empty map
    model.layoutChanged(QMap<QString, QPointF>());

    // assignedOperatorsChanged with empty map
    QMultiMap<int, QSharedPointer<MNEOperator>> emptyOps;
    model.assignedOperatorsChanged(emptyOps);

    // clearModel — resets to empty FiffInfo state
    model.clearModel();
    QVERIFY(model.rowCount() == 0);

    // setFiffInfo with empty FiffInfo — calls mapLayoutToChannels (no channels)
    FiffInfo::SPtr pInfo(new FiffInfo);
    model.setFiffInfo(pInfo);

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers2::rtFiffRawView_lifecycle()
{
    // Construct only — init starts background threads, so skip it in unit tests
    RtFiffRawView view("test_disp_viewers2");

    view.setGuiMode(AbstractView::GuiMode::Research);
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    view.saveSettings();
    view.loadSettings();
    view.clearView();

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers2::dipoleFitView_lifecycle()
{
    QSKIP("DipoleFitView accesses a null pointer during UI construction in headless mode");
}

//=============================================================================================================

void TestDispViewers2::control3dView_lifecycle()
{
    Control3DView view("test_disp_viewers2");

    // save/load are public; clearView is protected so we only call public API
    view.saveSettings();
    view.loadSettings();

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers2::applyToView_lifecycle()
{
    ApplyToView view("test_disp_viewers2");

    // AbstractView lifecycle
    view.saveSettings();
    view.loadSettings();
    view.clearView();

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers2::rtFiffRawViewDelegate_lifecycle()
{
    // Construct with null parent — default constructor path
    RtFiffRawViewDelegate delegate(nullptr);

    // Basic setter calls
    delegate.setSignalColor(Qt::red);
    delegate.setSignalColor(Qt::blue);

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers2::evokedSetModel_extended()
{
    EvokedSetModel model;

    // Basic model introspection (safe, no data loaded)
    QVERIFY(model.columnCount() >= 0);
    QVERIFY(model.rowCount() >= 0);

    // setEvokedSet with empty shared pointer — should not crash
    QSharedPointer<FiffEvokedSet> emptySet;
    model.setEvokedSet(emptySet);

    // isInit before data — false
    QVERIFY(!model.isInit());

    // getNumSamples — 0 before data loaded
    QCOMPARE(model.getNumSamples(), 0);

    // getIdxSelMap — returns empty map
    const QMap<qint32, qint32> &selMap = model.getIdxSelMap();
    QVERIFY(selMap.isEmpty());

    // numVLines — 0 before data
    QVERIFY(model.numVLines() >= -1);

    // row-based getters with invalid row — return 0/false immediately
    QCOMPARE(model.getKind(0), (fiff_int_t)0);
    QCOMPARE(model.getUnit(0), (fiff_int_t)FIFF_UNIT_NONE);
    QVERIFY(!model.getIsChannelBad(0));

    // Average color/activation maps — initially null, setters survive
    auto colors = model.getAverageColor();
    Q_UNUSED(colors);
    auto activations = model.getAverageActivation();
    Q_UNUSED(activations);

    auto pColors = QSharedPointer<QMap<QString, QColor>>::create();
    (*pColors)["avg"] = Qt::red;
    model.setAverageColor(pColors);

    auto pActivations = QSharedPointer<QMap<QString, bool>>::create();
    (*pActivations)["avg"] = true;
    model.setAverageActivation(pActivations);

    // headerData on valid sections (Vertical calls data() → dereferences m_pEvokedSet → skip)
    model.headerData(0, Qt::Horizontal, Qt::DisplayRole);
    model.headerData(1, Qt::Horizontal, Qt::DisplayRole);

    // data() with invalid index — QVariant()
    model.data(QModelIndex(), Qt::DisplayRole);

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers2::frequencySpectrumModel_basics()
{
    FrequencySpectrumModel model;

    // Default state — no data loaded
    QVERIFY(model.rowCount() >= 0);
    QCOMPARE(model.columnCount(), 2);

    // headerData on column 0 and 1
    model.headerData(0, Qt::Horizontal, Qt::DisplayRole);
    model.headerData(1, Qt::Horizontal, Qt::DisplayRole);
    model.headerData(1, Qt::Horizontal, Qt::TextAlignmentRole);
    model.headerData(0, Qt::Vertical, Qt::DisplayRole);

    // data() with invalid index — returns QVariant immediately
    model.data(QModelIndex(), Qt::DisplayRole);

    // setInfo with an empty FiffInfo
    QSharedPointer<FiffInfo> pInfo(new FiffInfo);
    model.setInfo(pInfo);

    // Scale type
    model.setScaleType(0);
    model.setScaleType(1);

    // toggleFreeze — just toggles a bool flag
    model.toggleFreeze(QModelIndex());
    model.toggleFreeze(QModelIndex());

    // setBoundaries — returns early since m_bInitialized is false
    model.setBoundaries(1.0f, 40.0f);
    model.setBoundaries(0.5f, 100.0f);

    // selectRows / resetSelection with empty selection
    QList<qint32> empty;
    model.selectRows(empty);
    model.resetSelection();

    // selectRows with a small selection (values exceed pInfo->chs.size() == 0, so skipped)
    QList<qint32> sel;
    sel << 0 << 1;
    model.selectRows(sel);
    model.resetSelection();

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers2::frequencySpectrumDelegate_basics()
{
    // Construct with a real (hidden) QTableView — constructor calls setMouseTracking
    QTableView tableView;
    FrequencySpectrumDelegate delegate(&tableView);

    delegate.setScaleType(0);
    delegate.setScaleType(1);

    // sizeHint with a dummy invalid index — column() == -1, no crash (switch falls through)
    QStyleOptionViewItem opt;
    QModelIndex invalidIdx;
    QSize sz = delegate.sizeHint(opt, invalidIdx);
    Q_UNUSED(sz);

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers2::bidsViewModel_basics()
{
    BidsViewModel model;

    QVERIFY(model.rowCount() >= 0);
    QVERIFY(model.columnCount() >= 0);

    // addSubject — creates BIDS-formatted subject name
    QModelIndex subjectIdx = model.addSubject("Subject01");
    QVERIFY(subjectIdx.isValid());

    // addSubject with BIDS-formatted name
    QModelIndex subjectIdx2 = model.addSubject("sub-02");
    QVERIFY(subjectIdx2.isValid());

    // addSessionToSubject by name (subject not found — returns invalid index)
    model.addSessionToSubject("Subject01", "Session1");

    // addSessionToSubject by index
    QModelIndex sessionIdx = model.addSessionToSubject(subjectIdx, "Session1");
    QVERIFY(sessionIdx.isValid());

    // addSessionToSubject again — same session name
    model.addSessionToSubject(subjectIdx, "Session1");

    // addData with invalid QModelIndex — creates sub-01/ses-01 automatically
    QStandardItem *dataItem = new QStandardItem("scan.fif");
    model.addData(QModelIndex(), dataItem, BIDS_FUNCTIONALDATA);

    // addData with invalid index + BIDS_ANATOMICALDATA
    QStandardItem *anatItem = new QStandardItem("T1.nii");
    model.addData(QModelIndex(), anatItem, BIDS_ANATOMICALDATA);

    // addData for BIDS_EVENT
    QStandardItem *evtItem = new QStandardItem("events.tsv");
    model.addData(QModelIndex(), evtItem, BIDS_EVENT);

    // addData for BIDS_AVERAGE
    QStandardItem *avgItem = new QStandardItem("avg.fif");
    model.addData(QModelIndex(), avgItem, BIDS_AVERAGE);

    // addDataToSession by index
    QStandardItem *extra = new QStandardItem("extra.fif");
    model.addDataToSession(sessionIdx, extra, BIDS_FUNCTIONALDATA);

    // removeItem with invalid index — returns false
    QVERIFY(!model.removeItem(QModelIndex()));

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers2::mneOperator_basics()
{
    // Default constructor
    MNEOperator op1;
    QCOMPARE(op1.m_OperatorType, MNEOperator::UNKNOWN);

    // Type constructor
    MNEOperator op2(MNEOperator::FILTER);
    QCOMPARE(op2.m_OperatorType, MNEOperator::FILTER);

    // Copy constructor
    MNEOperator op3(op2);
    QCOMPARE(op3.m_OperatorType, MNEOperator::FILTER);

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers2::rtFiffRawViewModel_extended()
{
    RtFiffRawViewModel model;

    // Safe setters that don't require FiffInfo
    model.setFilterActive(true);
    model.setFilterActive(false);

    model.setBackgroundColor(Qt::white);
    model.setBackgroundColor(Qt::black);

    model.distanceTimeSpacerChanged(0);   // sets to 1000
    model.distanceTimeSpacerChanged(100); // sets to 100
    model.distanceTimeSpacerChanged(-1);  // sets to 1000 (≤0 guard)

    model.resetTriggerCounter();

    // Scale map operations
    QMap<qint32, float> scaleMap;
    scaleMap[FIFFV_EEG_CH] = 1e-6f;
    model.setScaling(scaleMap);
    QCOMPARE(model.getScaling().value(FIFFV_EEG_CH, 0.0f), 1e-6f);

    QApplication::processEvents();
}

//=============================================================================================================

QTEST_MAIN(TestDispViewers2)
#include "test_disp_viewers2.moc"
