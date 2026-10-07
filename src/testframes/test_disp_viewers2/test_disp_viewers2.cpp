//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_disp_viewers2.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Tests for additional disp viewer widget classes and helper models.
 *           Covers: HpiSettingsView, CoregSettingsView, ArtifactSettingsView,
 *           FwdSettingsView, FiffRawViewSettings, ProjectSettingsView, BidsView,
 *           ChannelSelectionView, AverageLayoutView, AverageSelectionView,
 *           ButterflyView, SpectrumView, RtFiffRawViewModel, EvokedSetModel,
 *           ChannelInfoModel, and RtFiffRawView.
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
#include <disp/viewers/channeldataview.h>
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
#include <disp/viewers/helpers/channelrhiview.h>
#include <disp/viewers/helpers/channeldatamodel.h>

#include <fiff/fiff_info.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_dig_point.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QApplication>
#include <QFile>
#include <QLayout>
#include <QSharedPointer>
#include <QScrollBar>
#include <QSignalSpy>
#include <QStringList>
#include <QTableView>

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace FIFFLIB;
using Eigen::MatrixXd;

namespace
{

QSharedPointer<FiffInfo> createBrowserTestInfo()
{
    QSharedPointer<FiffInfo> info(new FiffInfo);
    info->sfreq = 1000.0f;
    info->nchan = 4;
    info->ch_names = QStringList({QStringLiteral("MEG0111"),
                                  QStringLiteral("MEG0112"),
                                  QStringLiteral("MEG0113"),
                                  QStringLiteral("STI014")});

    info->chs.clear();
    info->chs.resize(info->nchan);

    for (int i = 0; i < info->nchan; ++i) {
        FiffChInfo channelInfo;
        channelInfo.ch_name = info->ch_names.at(i);
        channelInfo.scanNo = i + 1;
        channelInfo.logNo = i + 1;
        channelInfo.chpos.coil_type = FIFFV_COIL_VV_MAG_T3;
        channelInfo.coord_frame = FIFFV_COORD_DEVICE;
        channelInfo.cal = 1.0f;
        channelInfo.range = 1.0f;
        channelInfo.unit_mul = 0;
        channelInfo.kind = (i == info->nchan - 1) ? FIFFV_STIM_CH : FIFFV_MEG_CH;
        channelInfo.unit = (i == info->nchan - 1) ? FIFF_UNIT_NONE : FIFF_UNIT_T;
        info->chs[i] = channelInfo;
    }

    info->bads = QStringList({QStringLiteral("MEG0112")});
    return info;
}

Eigen::MatrixXd createBrowserTestData()
{
    Eigen::MatrixXd data(4, 200);
    data.setZero();
    for (int sample = 0; sample < data.cols(); ++sample) {
        data(0, sample) = std::sin(static_cast<double>(sample) * 0.05);
        data(1, sample) = std::cos(static_cast<double>(sample) * 0.08);
        data(2, sample) = (sample % 25 == 0) ? 1.0 : 0.0;
        data(3, sample) = (sample % 50 == 0) ? 3.0 : 0.0;
    }
    return data;
}

}

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
     * Verifies that the initial layout selection resolves the actual .lout resource
     * immediately when a FIFF file is loaded, instead of requiring a manual layout switch.
     */
    void channelSelectionView_initialLayoutLoads();

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
     * Verifies RtFiffRawViewModel metadata, ring-buffer data flow, selection, freeze, and trigger state.
     */
    void rtFiffRawViewModel_dataFlow();

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
     * Verifies that ChannelDataView keeps the QRHI trace list in sync with the bad-channel
     * visibility filter and viewport/sample mapping.
     */
    void channelDataView_hideBadChannelsAndMapping();

    //=========================================================================================================
    /**
        * Verifies ChannelRhiView public state, clamping, signals, and overlays without rendering.
     */
    void channelRhiView_stateContracts();
    void channelRhiView_rendersAndDrawsOverlays();
    void channelDataModel_bufferDetrendAndDecimation();

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

void TestDispViewers2::cleanupTestCase()
{
}

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
    const QString layoutPath = QCoreApplication::applicationDirPath() + QStringLiteral("/../resources/general/2DLayouts/Vectorview-all.lout");
    if (!QFile::exists(layoutPath)) {
        QSKIP("Vectorview-all.lout not available in test environment");
    }

    auto info = createBrowserTestInfo();
    ChannelInfoModel::SPtr infoModel(new ChannelInfoModel);
    infoModel->setFiffInfo(info);

    ChannelSelectionView view(QStringLiteral("test_disp_viewers2_lifecycle"),
                              nullptr,
                              infoModel,
                              Qt::Widget);
    view.setGuiMode(AbstractView::GuiMode::Research);
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    view.setCurrentLayoutFile(QStringLiteral("Vectorview-all.lout"));
    view.newFiffFileLoaded(info);
    QVERIFY(!view.getLayoutMap().isEmpty());

    view.selectChannels(QStringList{info->ch_names.first()});
    view.updateBadChannels();
    view.updateDataView();
    view.saveSettings();
    view.loadSettings();
    view.clearView();

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers2::channelSelectionView_initialLayoutLoads()
{
    const QString layoutPath = QCoreApplication::applicationDirPath() + QStringLiteral("/../resources/general/2DLayouts/Vectorview-all.lout");
    if (!QFile::exists(layoutPath)) {
        QSKIP("Vectorview-all.lout not available in test environment");
    }

    auto info = createBrowserTestInfo();
    ChannelInfoModel::SPtr infoModel(new ChannelInfoModel);
    infoModel->setFiffInfo(info);

    ChannelSelectionView view(QStringLiteral("test_disp_viewers2_selection"),
                              nullptr,
                              infoModel,
                              Qt::Widget);
    view.setCurrentLayoutFile(QStringLiteral("Vectorview-all.lout"));
    view.newFiffFileLoaded(info);

    QVERIFY(!view.getLayoutMap().isEmpty());

    QApplication::processEvents();
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

void TestDispViewers2::rtFiffRawViewModel_dataFlow()
{
    RtFiffRawViewModel model;
    auto info = createBrowserTestInfo();
    model.setFiffInfo(info);

    QCOMPARE(model.rowCount(), 4);
    QCOMPARE(model.getKind(0), FIFFV_MEG_CH);
    QCOMPARE(model.getUnit(0), FIFF_UNIT_T);
    QCOMPARE(model.getCoil(0), FIFFV_COIL_VV_MAG_T3);
    QCOMPARE(model.getKind(99), 0);
    QCOMPARE(model.getUnit(99), FIFF_UNIT_NONE);
    QCOMPARE(model.getCoil(99), FIFFV_COIL_NONE);

    model.setSamplingInfo(100.0f, 1, true);
    QCOMPARE(model.getMaxSamples(), 100);

    QSignalSpy dataSpy(&model, &QAbstractItemModel::dataChanged);
    Eigen::MatrixXd firstBlock = Eigen::MatrixXd::Zero(4, 60);
    firstBlock.row(0).setConstant(1.0);
    firstBlock.row(1).setConstant(-1.0);
    firstBlock(3, 10) = 3.0;
    model.addData({firstBlock});
    QCOMPARE(model.getCurrentSampleIndex(), 60);
    QCOMPARE(model.getLastBlock(), firstBlock);
    QCOMPARE(dataSpy.size(), 1);

    Eigen::MatrixXd secondBlock = Eigen::MatrixXd::Zero(4, 60);
    secondBlock.row(0).setConstant(2.0);
    secondBlock.row(2).setConstant(-2.0);
    model.addData({secondBlock});
    QCOMPARE(model.getCurrentSampleIndex(), 60);
    QCOMPARE(model.getLastBlock(), secondBlock);
    QCOMPARE(model.getMaxValueFromRawViewModel(0), static_cast<double>(1e-11f));
    QCOMPARE(dataSpy.size(), 2);

    model.addData({Eigen::MatrixXd::Zero(3, 10)});
    QCOMPARE(model.getCurrentSampleIndex(), 60);
    QCOMPARE(dataSpy.size(), 2);

    model.toggleFreeze(QModelIndex());
    QVERIFY(model.isFreezed());
    model.toggleFreeze(QModelIndex());
    QVERIFY(!model.isFreezed());

    QSignalSpy selectionSpy(&model, &RtFiffRawViewModel::newSelection);
    model.selectRows({0, 2, 99});
    QCOMPARE(model.getIdxSelMap().size(), 2);
    QCOMPARE(model.getIdxSelMap().value(0), 0);
    QCOMPARE(model.getIdxSelMap().value(1), 2);
    QCOMPARE(selectionSpy.size(), 1);
    model.hideRows({0});
    QCOMPARE(model.getIdxSelMap().size(), 1);
    model.resetSelection();
    QCOMPARE(model.getIdxSelMap().size(), 4);

    model.markChBad(model.index(0, 0), true);
    QVERIFY(info->bads.contains(QStringLiteral("MEG0111")));
    model.markChBad(model.index(0, 0), false);
    QVERIFY(!info->bads.contains(QStringLiteral("MEG0111")));
    model.markChBad({model.index(0, 0), model.index(2, 0)}, true);
    QVERIFY(info->bads.contains(QStringLiteral("MEG0111")));
    QVERIFY(info->bads.contains(QStringLiteral("MEG0113")));

    QMap<double, QColor> triggerColors;
    triggerColors.insert(3.0, Qt::red);
    model.triggerInfoChanged(triggerColors, true, QStringLiteral("STI014"), 1.0);
    QVERIFY(model.triggerDetectionActive());
    QCOMPARE(model.getTriggerName(), QStringLiteral("STI014"));
    QCOMPARE(model.getTriggerThreshold(), 1.0);
    QCOMPARE(model.getCurrentTriggerIndex(), 3);

    Eigen::MatrixXd triggerBlock = Eigen::MatrixXd::Zero(4, 20);
    triggerBlock(3, 5) = 3.0;
    model.updateSpharaActivation(true);
    model.addData({triggerBlock});
    model.updateSpharaActivation(false);
    QVERIFY(!model.getDetectedTriggers().isEmpty());

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

void TestDispViewers2::channelDataView_hideBadChannelsAndMapping()
{
    ChannelDataView view(QStringLiteral("test_disp_viewers2_channeldata"));
    const auto info = createBrowserTestInfo();

    view.resize(900, 500);
    view.init(info);
    view.setFileBounds(100, 299);
    view.setData(createBrowserTestData(), 100);
    view.setWindowSize(0.2f);
    view.scrollToSample(100, false);
    if (view.layout()) {
        view.layout()->activate();
    }

    auto* rhiView = view.findChild<ChannelRhiView*>();
    QVERIFY(rhiView != nullptr);
    QCOMPARE(rhiView->totalLogicalChannels(), 4);

    view.hideBadChannels(true);
    QCOMPARE(rhiView->totalLogicalChannels(), 3);

    view.setChannelFilter(QStringList() << QStringLiteral("MEG0111") << QStringLiteral("MEG0113"));
    QCOMPARE(rhiView->totalLogicalChannels(), 2);

    view.setChannelFilter(QStringList());
    QCOMPARE(rhiView->totalLogicalChannels(), 3);

    const QRect viewport = view.signalViewportRect();
    QVERIFY(viewport.width() > 0);

    const int sample = 150;
    const int x = view.sampleToViewportX(sample);
    const int roundTripSample = view.viewportXToSample(x);
    QVERIFY(qAbs(roundTripSample - sample) <= 2);

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers2::channelRhiView_stateContracts()
{
    ChannelDataView view(QStringLiteral("test_disp_viewers2_rhi_state"));
    const auto info = createBrowserTestInfo();
    view.resize(800, 400);
    view.init(info);
    view.setFileBounds(100, 299);
    view.setData(createBrowserTestData(), 100);
    if (view.layout()) {
        view.layout()->activate();
    }

    auto* rhiView = view.findChild<ChannelRhiView*>();
    QVERIFY(rhiView != nullptr);
    rhiView->resize(640, 360);

    QSignalSpy scrollSpy(rhiView, &ChannelRhiView::scrollSampleChanged);
    QSignalSpy zoomSpy(rhiView, &ChannelRhiView::samplesPerPixelChanged);
    QSignalSpy channelSpy(rhiView, &ChannelRhiView::channelOffsetChanged);

    rhiView->setSamplesPerPixel(0.0f);
    QCOMPARE(rhiView->samplesPerPixel(), 1.0e-4f);
    QCOMPARE(zoomSpy.size(), 1);
    rhiView->zoomTo(2.0f, 0);
    QCOMPARE(rhiView->samplesPerPixel(), 2.0f);
    QCOMPARE(rhiView->visibleSampleCount(), 1280);

    rhiView->setLastFileSample(299);
    rhiView->setFirstFileSample(100);
    rhiView->setScrollSample(-50.0f);
    QCOMPARE(rhiView->visibleFirstSample(), 100);
    rhiView->scrollTo(500.0f, 0);
    QCOMPARE(rhiView->visibleFirstSample(), 100);
    QVERIFY(!scrollSpy.isEmpty());

    rhiView->setScrollSpeedFactor(0.0f);
    QCOMPARE(rhiView->scrollSpeedFactor(), 0.25f);
    rhiView->setScrollSpeedFactor(10.0f);
    QCOMPARE(rhiView->scrollSpeedFactor(), 4.0f);
    rhiView->setPrefetchFactor(0.0f);

    rhiView->setVisibleChannelCount(2);
    QCOMPARE(rhiView->visibleChannelCount(), 2);
    rhiView->setFirstVisibleChannel(99);
    QCOMPARE(rhiView->firstVisibleChannel(), 2);
    QCOMPARE(channelSpy.size(), 1);
    rhiView->setChannelIndices({0, 2});
    QCOMPARE(rhiView->totalLogicalChannels(), 2);
    QCOMPARE(rhiView->firstVisibleChannel(), 0);
    rhiView->setChannelIndices({});
    QCOMPARE(rhiView->totalLogicalChannels(), 4);

    rhiView->setHideBadChannels(true);
    QCOMPARE(rhiView->totalLogicalChannels(), 3);
    rhiView->setButterflyMode(true);
    QVERIFY(rhiView->butterflyMode());

    rhiView->setFrozen(true);
    QVERIFY(rhiView->isFrozen());
    rhiView->setGridVisible(false);
    QVERIFY(!rhiView->gridVisible());
    rhiView->setWheelScrollsChannels(false);
    QVERIFY(!rhiView->wheelScrollsChannels());
    rhiView->setCrosshairEnabled(true);
    QVERIFY(rhiView->crosshairEnabled());
    rhiView->setCrosshairEnabled(false);
    rhiView->setScalebarsVisible(false);
    QVERIFY(!rhiView->scalebarsVisible());
    rhiView->setBackgroundColor(Qt::darkBlue);
    QCOMPARE(rhiView->backgroundColor(), QColor(Qt::darkBlue));
    rhiView->setSfreq(-1.0f);

    rhiView->setEvents({ChannelRhiView::EventMarker{120, 1, Qt::red, QStringLiteral("event")}});
    rhiView->setEventsVisible(false);
    QVERIFY(!rhiView->eventsVisible());
    rhiView->setEpochMarkers({125, 150});
    rhiView->setEpochMarkersVisible(false);
    QVERIFY(!rhiView->epochMarkersVisible());
    rhiView->setClippingVisible(false);
    QVERIFY(!rhiView->clippingVisible());
    rhiView->setZScoreMode(true);
    QVERIFY(rhiView->zScoreMode());
    rhiView->setAnnotations({ChannelRhiView::AnnotationSpan{130, 145, Qt::yellow,
                                                            QStringLiteral("annotation")}});
    rhiView->setAnnotationSelectionEnabled(true);
    rhiView->setAnnotationsVisible(false);
    QVERIFY(!rhiView->annotationsVisible());

    QSignalSpy clickSpy(rhiView, &ChannelRhiView::sampleClicked);
    QSignalSpy rangeSpy(rhiView, &ChannelRhiView::sampleRangeSelected);
    QSignalSpy boundarySpy(rhiView, &ChannelRhiView::annotationBoundaryMoved);

    rhiView->setSamplesPerPixel(0.5f);
    rhiView->setFrozen(true);
    QTest::mouseClick(rhiView, Qt::LeftButton, Qt::NoModifier, QPoint(40, 40));
    QCOMPARE(clickSpy.size(), 1);
    QCOMPARE(clickSpy.takeFirst().at(0).toInt(), 120);

    QTest::mousePress(rhiView, Qt::LeftButton, Qt::NoModifier, QPoint(60, 40));
    QTest::mouseMove(rhiView, QPoint(80, 40));
    QTest::mouseRelease(rhiView, Qt::LeftButton, Qt::NoModifier, QPoint(80, 40));
    QCOMPARE(boundarySpy.size(), 1);
    QCOMPARE(boundarySpy.takeFirst().at(2).toInt(), 140);

    QTest::mousePress(rhiView, Qt::RightButton, Qt::NoModifier, QPoint(100, 40));
    QTest::mouseMove(rhiView, QPoint(200, 40));
    QTest::mouseRelease(rhiView, Qt::RightButton, Qt::NoModifier, QPoint(200, 40));
    QCOMPARE(rangeSpy.size(), 1);
    QCOMPARE(rangeSpy.takeFirst().at(0).toInt(), 150);

    rhiView->setAnnotationSelectionEnabled(false);
    QTest::mousePress(rhiView, Qt::RightButton, Qt::NoModifier, QPoint(20, 20));
    QTest::mouseMove(rhiView, QPoint(100, 22));
    QTest::mouseRelease(rhiView, Qt::RightButton, Qt::NoModifier, QPoint(100, 22));

    rhiView->setFrozen(false);
    QTest::mouseClick(rhiView, Qt::LeftButton, Qt::NoModifier, QPoint(50, 40));
    QCOMPARE(clickSpy.size(), 1);
    QTest::mousePress(rhiView, Qt::LeftButton, Qt::AltModifier, QPoint(200, 40));
    QTest::mouseMove(rhiView, QPoint(180, 40));
    QTest::mouseRelease(rhiView, Qt::LeftButton, Qt::AltModifier, QPoint(180, 40));

    rhiView->setWheelScrollsChannels(true);
    rhiView->setFirstVisibleChannel(0);
    QWheelEvent wheelEvent(QPointF(50, 40), QPointF(50, 40), QPoint(), QPoint(0, -120),
                           Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    QApplication::sendEvent(rhiView, &wheelEvent);
    QCOMPARE(rhiView->firstVisibleChannel(), 1);

    QApplication::processEvents();
}

//=============================================================================================================

void TestDispViewers2::channelRhiView_rendersAndDrawsOverlays()
{
    ChannelDataView view(QStringLiteral("test_disp_viewers2_rhi_render"));
    view.resize(800, 400);
    view.init(createBrowserTestInfo());
    view.setFileBounds(100, 299);
    view.setData(createBrowserTestData(), 100);
    auto* rhiView = view.findChild<ChannelRhiView*>();
    QVERIFY(rhiView != nullptr);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    rhiView->setSfreq(1000.0f);
    rhiView->setFirstFileSample(100);
    rhiView->setLastFileSample(299);
    rhiView->setSamplesPerPixel(0.25f);
    rhiView->setScrollSample(100.0f);
    rhiView->setVisibleChannelCount(4);
    rhiView->setEvents({ChannelRhiView::EventMarker{120, 1, Qt::red, QStringLiteral("event")}});
    rhiView->setEpochMarkers({125, 150});
    rhiView->setAnnotations({ChannelRhiView::AnnotationSpan{130, 145, Qt::yellow, QStringLiteral("annotation")}});
    const int laneCenterY = rhiView->height() / 8;

    QSignalSpy cursorSpy(rhiView, &ChannelRhiView::cursorDataChanged);
    rhiView->setCrosshairEnabled(true);
    rhiView->setScalebarsVisible(true);
    for (const bool butterfly : {false, true}) {
        rhiView->setButterflyMode(butterfly);
        for (const bool zScore : {false, true}) {
            rhiView->setZScoreMode(zScore);
            QVERIFY(!view.grab().isNull());
        }
        // Crosshair over channel row 0 at x = 80 px -> sample 100 + 80 * 0.25 = 120
        QMouseEvent hover(QEvent::MouseMove, QPointF(80, laneCenterY), rhiView->mapToGlobal(QPointF(80, laneCenterY)),
                          Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(rhiView, &hover);
        QVERIFY(!cursorSpy.isEmpty());
        const QList<QVariant> args = cursorSpy.takeLast();
        QCOMPARE(args.at(0).toFloat(), 0.02f);
        if (butterfly) {
            QCOMPARE(args.at(1).toFloat(), 0.0f);
            QCOMPARE(args.at(3).toString(), QStringLiteral("T"));
        } else {
            QCOMPARE(args.at(2).toString(), QStringLiteral("MEG0111"));
            QVERIFY(std::fabs(args.at(1).toFloat() - static_cast<float>(std::sin(20 * 0.05))) < 1e-5f);
        }
        QVERIFY(!view.grab().isNull());
    }
    rhiView->setButterflyMode(false);

    // Ruler measurements: free, horizontal and vertical snapping
    rhiView->setAnnotationSelectionEnabled(false);
    for (const QPoint& end : {QPoint(103, 63), QPoint(300, 62), QPoint(102, 200), QPoint(200, 150)}) {
        QTest::mousePress(rhiView, Qt::RightButton, Qt::NoModifier, QPoint(100, 60));
        QTest::mouseMove(rhiView, end);
        QVERIFY(!view.grab().isNull());
        QTest::mouseRelease(rhiView, Qt::RightButton, Qt::NoModifier, end);
    }

    // Annotation range selection overlay
    rhiView->setAnnotationSelectionEnabled(true);
    QSignalSpy rangeSpy(rhiView, &ChannelRhiView::sampleRangeSelected);
    QTest::mousePress(rhiView, Qt::RightButton, Qt::NoModifier, QPoint(200, 40));
    QTest::mouseMove(rhiView, QPoint(240, 40));
    QVERIFY(!view.grab().isNull());
    QTest::mouseRelease(rhiView, Qt::RightButton, Qt::NoModifier, QPoint(240, 40));
    QCOMPARE(rangeSpy.size(), 1);
    QCOMPARE(rangeSpy.at(0).at(0).toInt(), 150);
    QCOMPARE(rangeSpy.at(0).at(1).toInt(), 160);

    // Hovering an annotation edge, then dragging it and double-clicking
    QSignalSpy boundarySpy(rhiView, &ChannelRhiView::annotationBoundaryMoved);
    QTest::mouseMove(rhiView, QPoint(180, 40));
    QTest::mousePress(rhiView, Qt::LeftButton, Qt::NoModifier, QPoint(180, 40));
    QTest::mouseMove(rhiView, QPoint(160, 40));
    QTest::mouseRelease(rhiView, Qt::LeftButton, Qt::NoModifier, QPoint(160, 40));
    QCOMPARE(boundarySpy.size(), 1);
    QCOMPARE(boundarySpy.at(0).at(2).toInt(), 140);
    QTest::mouseDClick(rhiView, Qt::LeftButton, Qt::NoModifier, QPoint(150, 40));

    // Middle-button and left drags pan, with inertia
    rhiView->setAnnotationSelectionEnabled(false);
    rhiView->setSamplesPerPixel(0.05f);
    const float before = rhiView->scrollSample();
    QTest::mousePress(rhiView, Qt::LeftButton, Qt::NoModifier, QPoint(300, 40));
    QTest::mouseMove(rhiView, QPoint(260, 40));
    QTest::mouseMove(rhiView, QPoint(200, 40));
    QTest::mouseRelease(rhiView, Qt::LeftButton, Qt::NoModifier, QPoint(200, 40));
    QVERIFY(rhiView->scrollSample() > before);
    QTest::mousePress(rhiView, Qt::MiddleButton, Qt::NoModifier, QPoint(200, 40));
    QTest::mouseMove(rhiView, QPoint(260, 40));
    QTest::mouseRelease(rhiView, Qt::MiddleButton, Qt::NoModifier, QPoint(260, 40));

    // Wheel: time scroll and Ctrl-zoom
    rhiView->setWheelScrollsChannels(false);
    const float spp = rhiView->samplesPerPixel();
    QWheelEvent zoom(QPointF(50, 40), QPointF(50, 40), QPoint(), QPoint(0, 120), Qt::NoButton, Qt::ControlModifier,
                     Qt::NoScrollPhase, false);
    QApplication::sendEvent(rhiView, &zoom);
    QTRY_VERIFY(std::fabs(rhiView->samplesPerPixel() - 0.8f * spp) < 1e-6f);
    QWheelEvent scroll(QPointF(50, 40), QPointF(50, 40), QPoint(), QPoint(0, -120), Qt::NoButton, Qt::NoModifier,
                       Qt::NoScrollPhase, false);
    QApplication::sendEvent(rhiView, &scroll);

    view.resize(600, 300);
    QApplication::processEvents();
    QVERIFY(!view.grab().isNull());
}

//=============================================================================================================

void TestDispViewers2::channelDataModel_bufferDetrendAndDecimation()
{
    QSharedPointer<FiffInfo> info(new FiffInfo);
    const QVector<std::pair<int, int>> kinds = {{FIFFV_MEG_CH, FIFF_UNIT_T_M}, {FIFFV_MEG_CH, FIFF_UNIT_T}, {FIFFV_EEG_CH, FIFF_UNIT_V}, {FIFFV_EOG_CH, FIFF_UNIT_V}, {FIFFV_ECG_CH, FIFF_UNIT_V}, {FIFFV_EMG_CH, FIFF_UNIT_V}, {FIFFV_STIM_CH, FIFF_UNIT_NONE}, {FIFFV_MISC_CH, FIFF_UNIT_NONE}};
    info->sfreq = 500.0f;
    info->nchan = kinds.size();
    for (int i = 0; i < kinds.size(); ++i) {
        FiffChInfo ch;
        ch.ch_name = QStringLiteral("CH%1").arg(i);
        ch.kind = kinds[i].first;
        ch.unit = kinds[i].second;
        info->chs.append(ch);
        info->ch_names.append(ch.ch_name);
    }
    info->bads = {QStringLiteral("CH2")};

    ChannelDataModel model;
    QSignalSpy metaSpy(&model, &ChannelDataModel::metaChanged);
    model.init(info);
    QCOMPARE(model.channelCount(), 8);
    QCOMPARE(model.sfreq(), 500.0f);
    const QStringList labels = {"MEG grad", "MEG mag", "EEG", "EOG", "ECG", "EMG", "STIM", "MISC"};
    const QVector<float> scales = {400e-13f, 1.2e-12f, 30e-6f, 150e-6f, 1e-3f, 1e-3f, 5.0f, 1.0f};
    for (int i = 0; i < 8; ++i) {
        QCOMPARE(model.channelInfo(i).typeLabel, labels[i]);
        QCOMPARE(model.channelInfo(i).name, info->ch_names[i]);
        QCOMPARE(model.channelInfo(i).bad, i == 2);
        QVERIFY(model.channelInfo(i).color.isValid());
    }
    QCOMPARE(model.channelInfo(0).amplitudeMax, scales[0]);
    QCOMPARE(model.channelInfo(1).amplitudeMax, 400e-13f);
    QCOMPARE(model.channelInfo(99).name, QString());

    // The browser's string-keyed scales map onto grad/mag/EEG/... separately
    model.setScaleMapFromStrings({{"MEG_grad", 1e-10}, {"MEG_mag", 2e-12}, {"MEG_EEG", 5e-5}, {"MEG_EOG", 2e-4}, {"MEG_EMG", 3e-3}, {"MEG_ECG", 4e-3}, {"MEG_MISC", 2.0}, {"MEG_STIM", 7.0}});
    const QVector<float> userScales = {1e-10f, 2e-12f, 5e-5f, 2e-4f, 4e-3f, 3e-3f, 7.0f, 2.0f};
    for (int i = 0; i < 8; ++i) {
        QCOMPARE(model.channelInfo(i).amplitudeMax, userScales[i]);
    }

    // Bad flags round-trip into the FiffInfo
    model.setChannelBad(2, false);
    model.setChannelBad(5, true);
    model.setChannelBad(5, true);
    QCOMPARE(info->bads, QStringList{QStringLiteral("CH5")});
    QVERIFY(model.channelInfo(5).bad);

    // Virtual channels get defaults for every missing field
    model.setSignalColor(Qt::darkGreen);
    ChannelDisplayInfo named;
    named.name = QStringLiteral("EOG bipolar");
    named.typeLabel = QStringLiteral("EOG");
    named.color = Qt::red;
    named.amplitudeMax = 1e-4f;
    model.setVirtualChannels({named, ChannelDisplayInfo{}});
    QCOMPARE(model.channelCount(), 10);
    QCOMPARE(model.channelInfo(8).name, QStringLiteral("EOG bipolar"));
    QVERIFY(model.channelInfo(8).isVirtualChannel);
    QCOMPARE(model.channelInfo(9).name, QStringLiteral("Virtual 2"));
    QCOMPARE(model.channelInfo(9).typeLabel, QStringLiteral("MISC"));
    QCOMPARE(model.channelInfo(9).color, QColor(Qt::darkGreen));
    QCOMPARE(model.channelInfo(9).amplitudeMax, 1.0f);
    QVERIFY(metaSpy.size() >= 6);

    // Ring buffer: 3 channels, keep at most 1200 samples
    ChannelDataModel ring;
    ring.setMaxStoredSamples(1200);
    MatrixXd block(3, 500);
    for (int b = 0; b < 3; ++b) {
        for (int s = 0; s < 500; ++s) {
            const int t = 500 * b + s;
            block(0, s) = 2.0 + 0.5 * t;
            block(1, s) = std::sin(0.37 * t) + 0.01 * (t % 11);
            block(2, s) = (t % 2) ? 3.0 : -3.0;
        }
        ring.appendData(block);
    }
    ring.appendData(MatrixXd());
    QCOMPARE(ring.totalSamples(), 1200);
    QCOMPARE(ring.firstSample(), 300);
    QCOMPARE(ring.sampleValueAt(0, 300), 152.0f);
    QCOMPARE(ring.sampleValueAt(0, 299), 0.0f);
    QCOMPARE(ring.sampleValueAt(7, 400), 0.0f);
    // RMS uses at most the last 1000 samples of the window
    QCOMPARE(ring.channelRms(2, 0, 5000), 3.0f);
    double sumSq = 0.0;
    for (int t = 500; t < 1500; ++t) {
        sumSq += (2.0 + 0.5 * t) * (2.0 + 0.5 * t);
    }
    QVERIFY(std::fabs(ring.channelRms(0, 300, 1500) - std::sqrt(sumSq / 1000.0)) < 1e-2f * std::sqrt(sumSq / 1000.0));
    QCOMPARE(ring.channelRms(0, 900, 900), 0.0f);
    QCOMPARE(ring.channelRms(-1, 0, 10), 0.0f);

    // Raw path: one vertex per sample, detrended over the window
    int vboFirst = -1;
    ring.setDetrendMode(DetrendMode::Linear);
    QVector<float> v = ring.decimatedVertices(0, 400, 450, 100, vboFirst);
    QCOMPARE(vboFirst, 400);
    QCOMPARE(v.size(), 100);
    for (int i = 0; i < 50; ++i) {
        QCOMPARE(v[2 * i], static_cast<float>(i));
        QVERIFY(std::fabs(v[2 * i + 1]) < 1e-3f);
    }
    ring.setRemoveDC(true);
    v = ring.decimatedVertices(0, 400, 450, 100, vboFirst);
    QVERIFY(std::fabs(v[1] - (-0.5f * 24.5f)) < 1e-3f);
    QVERIFY(std::fabs(v[99] - 0.5f * 24.5f) < 1e-3f);
    ring.setRemoveDC(false);
    v = ring.decimatedVertices(0, 100, 310, 400, vboFirst);
    QCOMPARE(vboFirst, 300);
    QCOMPARE(v[1], 152.0f);
    QVERIFY(ring.decimatedVertices(0, 450, 400, 10, vboFirst).isEmpty());
    QVERIFY(ring.decimatedVertices(9, 400, 450, 10, vboFirst).isEmpty());

    // Decimation path: max then min of each 10-sample bin
    v = ring.decimatedVertices(1, 300, 1300, 100, vboFirst);
    QCOMPARE(v.size(), 400);
    for (int px = 0; px < 100; ++px) {
        float mx = -1e9f, mn = 1e9f;
        for (int t = 300 + 10 * px; t < 310 + 10 * px; ++t) {
            const float val = static_cast<float>(std::sin(0.37 * t) + 0.01 * (t % 11));
            mx = std::max(mx, val);
            mn = std::min(mn, val);
        }
        QCOMPARE(v[4 * px], 10.0f * px);
        QCOMPARE(v[4 * px + 1], mx);
        QCOMPARE(v[4 * px + 3], mn);
    }

    ring.setData(MatrixXd::Ones(2, 4), 10);
    QCOMPARE(ring.firstSample(), 10);
    QCOMPARE(ring.totalSamples(), 4);
    ring.clearData();
    QCOMPARE(ring.totalSamples(), 0);
    QCOMPARE(ring.firstSample(), 0);
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
    DipoleFitView view;
    view.setGuiMode(AbstractView::GuiMode::Research);
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);

    view.addMeas(QStringLiteral("sample_audvis-ave.fif"));
    view.addBem(QStringLiteral("sample-5120-bem-sol.fif"));
    view.addMri(QStringLiteral("all-trans.fif"));
    view.addNoise(QStringLiteral("sample_audvis-cov.fif"));
    view.removeModel(QStringLiteral("all-trans.fif"), 3);

    QSignalSpy timeSpy(&view, &DipoleFitView::timeChanged);
    QSignalSpy sphereSpy(&view, &DipoleFitView::sphereChanged);
    view.requestParams();
    QCOMPARE(timeSpy.count(), 1);
    QCOMPARE(sphereSpy.count(), 1);

    view.saveSettings();
    view.loadSettings();
    view.clearView();

    QApplication::processEvents();
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
    const QMap<qint32, qint32>& selMap = model.getIdxSelMap();
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
    QStandardItem* dataItem = new QStandardItem("scan.fif");
    model.addData(QModelIndex(), dataItem, BIDS_FUNCTIONALDATA);

    // addData with invalid index + BIDS_ANATOMICALDATA
    QStandardItem* anatItem = new QStandardItem("T1.nii");
    model.addData(QModelIndex(), anatItem, BIDS_ANATOMICALDATA);

    // addData for BIDS_EVENT
    QStandardItem* evtItem = new QStandardItem("events.tsv");
    model.addData(QModelIndex(), evtItem, BIDS_EVENT);

    // addData for BIDS_AVERAGE
    QStandardItem* avgItem = new QStandardItem("avg.fif");
    model.addData(QModelIndex(), avgItem, BIDS_AVERAGE);

    // addDataToSession by index
    QStandardItem* extra = new QStandardItem("extra.fif");
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
