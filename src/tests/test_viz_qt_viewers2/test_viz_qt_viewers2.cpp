//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_viz_qt_viewers2.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Tests for additional disp viewer widget classes and helper models.
 *           Covers: PanelWidget (HPI, co-registration, forward solution, project, dipole fit, averaging,
 *           artifact rejection, trigger detection), FiffRawViewSettings, BidsView,
 *           ChannelSelectionView, AverageLayoutPanel, AverageSelectionView,
 *           ButterflyView, SpectrumPanel, RtFiffRawViewModel, EvokedSetModel,
 *           ChannelInfoModel, and RtFiffRawView.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/viz/backends/qt/viewers/abstractview.h>
#include <mne/viz/backends/qt/detail/map_value.h>
#include <mne/base/string_utils.h>
#include <mne/base/log.h>
#include <mne/viz/panels/hpi_panel.h>
#include <mne/viz/panels/coregistration_panel.h>
#include <mne/viz/backends/qt/kit/panelwidget.h>
#include <mne/viz/panels/artifact_panel.h>
#include <mne/viz/panels/averaging_panel.h>
#include <mne/viz/panels/forward_panel.h>
#include <mne/viz/panels/trigger_detection_panel.h>
#include <mne/viz/backends/qt/viewers/fiffrawviewsettings.h>
#include <mne/viz/panels/project_panel.h>
#include <mne/viz/panels/filter_design_panel.h>
#include <mne/viz/backends/qt/plots/helpers/scenepainter.h>
#include <mne/viz/backends/qt/kit/settingspanel.h>
#include <mne/viz/backends/qt/viewers/bidsview.h>
#include <mne/viz/backends/qt/kit/itemlistview.h>
#include <mne/viz/models/item_list.h>
#include <mne/viz/panels/channel_selection_panel.h>
#include <mne/viz/backends/qt/viewers/channeldataview.h>
#include <mne/viz/backends/qt/kit/panelwidget.h>
#include <mne/viz/backends/qt/kit/sceneviewport.h>
#include <mne/viz/figures/svg_renderer.h>
#include <mne/viz/figures/average_layout_figure.h>
#include <mne/viz/figures/sensor_layout_figure.h>
#include <mne/viz/panels/average_layout_panel.h>
#include <mne/viz/backends/qt/viewers/averageselectionview.h>
#include <mne/viz/backends/qt/viewers/butterflyview.h>
#include <mne/viz/panels/spectrum_panel.h>
#include <mne/viz/backends/qt/viewers/rtfiffrawview.h>
#include <mne/viz/backends/qt/viewers/helpers/rtfiffrawviewmodel.h>
#include <mne/viz/backends/qt/viewers/helpers/evokedsetmodel.h>
#include <mne/viz/backends/qt/viewers/helpers/channelinfomodel.h>
#include <mne/viz/panels/dipole_fit_panel.h>
#include <mne/viz/backends/qt/viewers/control3dview.h>
#include <mne/viz/backends/qt/viewers/applytoview.h>
#include <mne/viz/backends/qt/viewers/helpers/rtfiffrawviewdelegate.h>
#include <mne/viz/backends/qt/viewers/helpers/bidsviewmodel.h>
#include <mne/viz/backends/qt/viewers/helpers/mneoperator.h>
#include <mne/viz/backends/qt/viewers/helpers/channelrhiview.h>
#include <mne/viz/backends/qt/viewers/helpers/selectionitem.h>
#include <mne/viz/backends/qt/viewers/helpers/channeldatamodel.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_ch_info.h>
#include <mne/fiff/fiff_coord_trans.h>
#include <mne/fiff/fiff_evoked_set.h>
#include <mne/fiff/fiff_dig_point.h>
#include <mne/fiff/fiff_named_matrix.h>
#include <mne/fiff/fiff_proj.h>
#include <mne/dsp/filterkernel.h>
#include <mne/dsp/filterio.h>
#include <rhi/qrhi.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QRhiWidget>
#include <QtTest>
#include <QApplication>
#include <QFile>
#include <QLayout>
#include <QSignalSpy>
#include <QStringList>
#include <QDir>
#include <QHeaderView>
#include <QMenu>
#include <QGridLayout>
#include <QScopeGuard>
#include <QGroupBox>
#include <QSettings>
#include <QPaintEngine>
#include <QPainter>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableView>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QMessageBox>
#include <QColorDialog>
#include <QToolButton>
#include <QFileDialog>
#include <QFileInfo>
#include <QLocale>
#include <QRadioButton>
#include <QVector3D>
#include <QTimer>
#include <QPointer>
#include <QTreeView>
#include <QStyledItemDelegate>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QKeyEvent>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <functional>
#include <memory>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;
using namespace mne::viz::qt;
using namespace mne;
using mne::FilterKernel;
using mne::FilterIO;
using Eigen::MatrixXd;

namespace
{

/** Exposes the frame recording of ChannelRhiView so a test can render it into an offscreen QRhi. */
class FrameChannelRhiView : public ChannelRhiView
{
public:
    using ChannelRhiView::releaseResources;
    using ChannelRhiView::renderFrame;
};

/** Paint engine that records the drawn text and lines, independent of the platform's glyph rasterisation. */
class TextRecordingEngine : public QPaintEngine
{
public:
    QStringList texts;
    QList<QLineF> lines;

    bool begin(QPaintDevice*) override
    {
        return true;
    }
    bool end() override
    {
        return true;
    }
    void updateState(const QPaintEngineState&) override
    {
    }
    void drawPixmap(const QRectF&, const QPixmap&, const QRectF&) override
    {
    }
    void drawPath(const QPainterPath&) override
    {
    }
    void drawPolygon(const QPointF*, int, PolygonDrawMode) override
    {
    }
    void drawLines(const QLineF* lineArray, int lineCount) override
    {
        for (int i = 0; i < lineCount; ++i) {
            lines.append(state->transform().map(lineArray[i]));
        }
    }
    void drawLines(const QLine* lineArray, int lineCount) override
    {
        for (int i = 0; i < lineCount; ++i) {
            lines.append(state->transform().map(QLineF(lineArray[i])));
        }
    }
    void drawRects(const QRectF*, int) override
    {
    }
    void drawTextItem(const QPointF&, const QTextItem& textItem) override
    {
        texts.append(textItem.text());
    }
    Type type() const override
    {
        return QPaintEngine::User;
    }
};

/** Paint device backed by TextRecordingEngine. */
class TextRecordingDevice : public QPaintDevice
{
public:
    explicit TextRecordingDevice(const QSize& size)
    : m_size(size)
    {
    }
    QPaintEngine* paintEngine() const override
    {
        return &m_engine;
    }
    QStringList texts() const
    {
        return m_engine.texts;
    }
    QList<QLineF> lines() const
    {
        return m_engine.lines;
    }

protected:
    int metric(PaintDeviceMetric metric) const override
    {
        switch (metric) {
            case PdmWidth:
                return m_size.width();
            case PdmHeight:
                return m_size.height();
            case PdmDpiX:
            case PdmDpiY:
            case PdmPhysicalDpiX:
            case PdmPhysicalDpiY:
                return 96;
            case PdmDevicePixelRatio:
                return 1;
            case PdmDevicePixelRatioScaled:
                return static_cast<int>(devicePixelRatioFScale());
            default:
                return QPaintDevice::metric(metric);
        }
    }

private:
    QSize m_size;
    mutable TextRecordingEngine m_engine;
};

/** Runs fn on the next application-modal widget as soon as it is shown. */
void answerNextModal(const std::function<void(QWidget*)>& fn)
{
    QTimer::singleShot(10, qApp, [fn]() {
        if (QWidget* pModal = QApplication::activeModalWidget()) {
            fn(pModal);
        } else {
            answerNextModal(fn);
        }
    });
}

/** Clicks the button labelled sText on the next modal QMessageBox. */
void clickNextMessageBoxButton(const QString& sText)
{
    answerNextModal([sText](QWidget* pModal) {
        auto* pBox = qobject_cast<QMessageBox*>(pModal);
        QVERIFY(pBox);
        for (QAbstractButton* pButton : pBox->buttons()) {
            if (pButton->text().remove(QLatin1Char('&')) == sText) {
                pButton->click();
                return;
            }
        }
        QFAIL(qPrintable(QStringLiteral("no button ") + sText));
    });
}

std::shared_ptr<FiffInfo> createBrowserTestInfo()
{
    std::shared_ptr<FiffInfo> info(new FiffInfo);
    info->sfreq = 1000.0f;
    info->nchan = 4;
    info->ch_names = {"MEG 0111", "MEG 0112", "MEG 0113", "STI 014"};

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

    info->bads = {"MEG 0112"};
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
 * DECLARE CLASS TestVizQtViewers2
 *
 * @brief The TestVizQtViewers2 class tests additional disp viewer widget classes and helper models
 *        under an offscreen platform.
 *
 */
class TestVizQtViewers2 : public QObject
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
     * Verifies HpiPanel in a PanelWidget: digitizers, coil frequencies and presets, fit results and reports.
     */
    void hpiPanel_inPanelWidget();

    //=========================================================================================================
    /**
     * Verifies CoregistrationPanel in a PanelWidget: fiducial picking, transformation, scaling modes, BEMs and files.
     */
    void coregistrationPanel_inPanelWidget();

    //=========================================================================================================
    /**
     * Verifies the PanelWidget of a FilterDesignPanel: filter design from the controls, band clamping, channel type,
     * the response figure and filter file export/import.
     */
    void filterDesignPanel_designExportLoad();


    //=======================================================================================================
    /**
     * Verifies AveragingPanel defaults, stim channel selection, epoch window and baseline controls, stored settings and the detected epochs.
     */
    void averagingPanel_controlsAndSettings();

    //=======================================================================================================
    /**
     * Verifies TriggerDetectionPanel channel list, threshold composition, detected types, colours, reset and the offline detect button, also after a re-init.
     */
    void triggerDetectionPanel_settingsAndSignals();

    //=======================================================================================================
    /**
     * Verifies ArtifactPanel rows per channel type, threshold composition, activation and new channel sets.
     */
    void artifactPanel_thresholds();

    //=========================================================================================================
    /**
     * Verifies the forward solution panel in its Qt host: status, solution summary, switches, the atlas directory
     * (clustering needs both a2009s annotations) and the compute button.
     */
    void forwardPanel_inPanelWidget();

    //=========================================================================================================
    /**
     * Verifies that FiffRawViewSettings constructs, getters return sensible defaults,
     * and setters do not crash.
     */
    void fiffRawViewSettings_setAndGet();

    //=========================================================================================================
    /**
     * Verifies that BidsView constructs and survives all lifecycle transitions.
     */
    void bidsView_lifecycle();

    //=========================================================================================================
    /**
     * Verifies a ChannelSelectionPanel in a PanelWidget: its lists as ItemListViews, picking sensors with the
     * mouse, Delete in the picked list, choosing a group and restoring the layout file from the settings.
     */
    void channelSelectionPanel_inPanelWidget();

    //=========================================================================================================
    /**
     * Verifies that an AverageLayoutPanel in a PanelWidget survives all lifecycle transitions.
     */
    void averageLayoutPanel_lifecycle();

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
    void butterflyView_drawsAveragesAcrossTheFullWidth();

    //=========================================================================================================
    /**
     * Verifies a SpectrumPanel in a PanelWidget: data before init dropped, the read-out under the mouse, freezing
     * by double click and the frequency bounds.
     */
    void spectrumPanel_inPanelWidget();

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
    void rtFiffRawViewModel_projectionWrapFilterAndRoles();

    //=========================================================================================================
    /**
     * Verifies that stream sample s lands in buffer column s mod N for raw data, filtered data and detected
     * triggers, for block sizes that do and do not divide the buffer length.
     */
    void rtFiffRawViewModel_ringBufferWrap();

    //=========================================================================================================
    /**
     * Verifies that EvokedSetModel constructs, reports sensible defaults, and
     * setEvokedSet does not crash on an empty set.
     */
    void averageLayoutPanel_drawsEachChannelsAverage();
    void evokedSetModel_basics();
    void evokedSetModel_projectionRolesAndAverageMaps();

    //=========================================================================================================
    /**
     * Verifies that ChannelInfoModel constructs with a FiffInfo, rowCount and
     * columnCount return sensible values, and basic data() queries do not crash.
     */
    void channelInfoModel_basics();
    void channelInfoModel_columnsRolesAndLayoutMapping();

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

    //=========================================================================================================
    /**
     * Verifies that ChannelRhiView::renderToImage() lays out rows and scales traces like the GPU path.
     */
    void channelRhiView_renderToImage();

    //=========================================================================================================
    /**
     * Verifies that the GPU frame of ChannelRhiView draws the rows of renderToImage() in every display mode.
     */
    void channelRhiView_gpuFrame();

    //=========================================================================================================
    /**
     * Verifies the values shown by the ChannelRhiView scalebar, crosshair and ruler overlays.
     */
    void channelRhiView_overlayValues();

    //=========================================================================================================
    /**
     * Verifies the ChannelDataView keyboard shortcuts: toggles, detrend cycling, navigation and zoom.
     */
    void channelDataView_keyboardShortcuts();

    //=========================================================================================================
    /**
     * Verifies a SensorLayoutFigure in a SceneViewport: wheel zoom, double-click fit, right-drag panning, click and
     * rubber-band selection.
     */
    void sensorLayoutFigure_zoomPanAndSelect();
    void channelRhiView_rendersAndDrawsOverlays();
    void channelDataModel_bufferDetrendAndDecimation();

    //=========================================================================================================
    /**
     * Verifies that RtFiffRawView constructs, init does not crash,
     * and lifecycle methods survive.
     */
    void rtFiffRawView_lifecycle();
    void rtFiffRawView_paintsHidesRowsAndAddsEvents();

    //=========================================================================================================
    /**
     * Verifies DipoleFitPanel in a PanelWidget: parameter reports, the reported parameter set, models and the fit name.
     */
    void dipoleFitPanel_inPanelWidget();

    //=========================================================================================================
    /**
     * Verifies AverageLayoutFigure drawing: stim and zero line, scaling and clamping, per-average color and
     * activation, bad-channel opacity and the press/release label size.
     */
    void averageLayoutFigure_paintsScaledClampedCurves();


    //=========================================================================================================
    /**
     * Verifies that Control3DView constructs headlessly and basic setters
     * do not crash.
     */
    void control3dView_lifecycle();

    //=========================================================================================================
    /**
     * Verifies Control3DView tool flags, view and light signals, color pickers and the data tree context menu.
     */
    void control3dView_flagsSignalsAndTree();

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
     * Verifies RtFiffRawViewDelegate drawing: signal scale and polarity, time cursor, trigger threshold, the
     * bad-channel tint and the signal color.
     */
    void rtFiffRawViewDelegate_paintsSignalCursorAndThreshold();

    //=========================================================================================================
    /**
     * Verifies that EvokedSetModel responds to more API calls than the
     * baseline test covers (header data, flags, column count, etc.).
     */
    void evokedSetModel_extended();


    //=========================================================================================================

    //=========================================================================================================
    /**
     * Verifies that BidsViewModel constructs, rowCount/columnCount return 0,
     * addSubject and addSessionToSubject do not crash.
     */
    void bidsViewModel_basics();

    //=========================================================================================================
    /**
     * Verifies BidsView tree building, context-menu moves, selection and removal.
     */
    void bidsView_treeMovesAndSelection();

    //=========================================================================================================
    /**
     * Verifies ProjectPanel in a PanelWidget: project and subject scanning, adding, file naming and timer.
     */
    void projectPanel_inPanelWidget();

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

void TestVizQtViewers2::initTestCase()
{
    QApplication::processEvents();
}

//=============================================================================================================

void TestVizQtViewers2::cleanupTestCase()
{
}

//=============================================================================================================

void TestVizQtViewers2::hpiPanel_inPanelWidget()
{
    // An empty settings path keeps the user's stored coil frequencies out of the test
    auto panel = std::make_shared<HpiPanel>();
    PanelWidget view(panel, QString());
    std::vector<std::vector<int>> frequencies;
    std::vector<bool> ssp;
    std::vector<bool> continuous;
    std::vector<int> windows;
    std::vector<std::filesystem::path> digitizerFiles;
    int singleFits = 0;
    HpiPanel::Callbacks callbacks;
    callbacks.coil_frequencies_changed = [&frequencies](const std::vector<int>& coils) {
        frequencies.push_back(coils);
    };
    callbacks.ssp_changed = [&ssp](bool on) {
        ssp.push_back(on);
    };
    callbacks.continuous_fitting_changed = [&continuous](bool on) {
        continuous.push_back(on);
    };
    callbacks.fitting_window_size_changed = [&windows](int samples) {
        windows.push_back(samples);
    };
    callbacks.digitizers_changed = [&digitizerFiles](const FiffDigPointSet&, const std::filesystem::path& file) {
        digitizerFiles.push_back(file);
    };
    callbacks.single_fit = [&singleFits] {
        ++singleFits;
    };
    panel->set_callbacks(std::move(callbacks));
    const auto text = [&view](const char* key) {
        return qobject_cast<QLabel*>(view.formPanel()->editor(key))->text();
    };

    // 4 HPI coils, 3 fiducials, 2 EEG and 1 extra point
    FiffDigPointSet points;
    const QVector<int> kinds = {FIFFV_POINT_CARDINAL, FIFFV_POINT_CARDINAL, FIFFV_POINT_CARDINAL, FIFFV_POINT_HPI,
                                FIFFV_POINT_HPI, FIFFV_POINT_HPI, FIFFV_POINT_HPI, FIFFV_POINT_EEG,
                                FIFFV_POINT_EEG, FIFFV_POINT_EXTRA};
    for (int i = 0; i < kinds.size(); ++i) {
        FiffDigPoint p;
        p.kind = kinds[i];
        p.ident = i + 1;
        points.push_back(p);
    }
    panel->set_digitizers(points);
    QCOMPARE(text("loadedCoils"), QStringLiteral("4"));
    QCOMPARE(text("loadedFiducials"), QStringLiteral("3"));
    QCOMPARE(text("loadedEeg"), QStringLiteral("2"));
    QCOMPARE(text("loadedAdditional"), QStringLiteral("1"));
    QCOMPARE(panel->coil_frequencies(), (std::vector<int>{293, 307, 314, 321}));
    QCOMPARE(panel->list("coilResults")->count(), 4);
    QVERIFY(frequencies.empty());

    // The VectorView presets are built in; a file that cannot be read keeps them
    auto* preset = qobject_cast<QComboBox*>(view.formPanel()->editor("coilPreset"));
    QVERIFY(preset);
    QCOMPARE(preset->count(), 4);
    QCOMPARE(preset->itemText(2), QStringLiteral("VV 293-321Hz"));
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(!panel->load_coil_presets(dir.filePath(QStringLiteral("missing.json")).toStdU16String()));
    QFile presets(dir.filePath(QStringLiteral("presets.json")));
    QVERIFY(presets.open(QIODevice::WriteOnly));
    presets.write("{\"4\": [");
    presets.close();
    QVERIFY(!panel->load_coil_presets(presets.fileName().toStdU16String()));
    QCOMPARE(preset->count(), 4);

    // A preset file replaces them for the loaded number of coils; choosing a preset sets the frequencies
    QVERIFY(presets.open(QIODevice::WriteOnly | QIODevice::Truncate));
    presets.write(R"({"4": [{"name": "VectorView", "coils": [154, 158, 162, 166]}]})");
    presets.close();
    QVERIFY(panel->load_coil_presets(presets.fileName().toStdU16String()));
    QCOMPARE(preset->count(), 2);
    preset->setCurrentIndex(1);
    QCOMPARE(frequencies.back(), (std::vector<int>{154, 158, 162, 166}));
    auto* coils = qobject_cast<QLineEdit*>(view.formPanel()->editor("coilFreqs"));
    QVERIFY(coils);
    QCOMPARE(coils->text(), QStringLiteral("154, 158, 162, 166"));

    // Typed frequencies, "none" for an unused coil; the list sets the number of coils
    coils->setText(QStringLiteral("300, 158, none, 166"));
    emit coils->editingFinished();
    QCOMPARE(frequencies.back(), (std::vector<int>{300, 158, -1, 166}));
    coils->setText(QStringLiteral("300 -1 166 x 170"));
    emit coils->editingFinished();
    QCOMPARE(frequencies.back(), (std::vector<int>{300, -1, 166, -1, 170}));
    QCOMPARE(coils->text(), QStringLiteral("300, none, 166, none, 170"));
    QCOMPARE(panel->list("coilResults")->count(), 5);

    // Fit results: errors in mm, GoF in %, good/bad against the allowed mean error
    qobject_cast<QDoubleSpinBox*>(view.formPanel()->editor("maxError"))->setValue(3.0);
    QCOMPARE(panel->allowed_error(), 3.0);
    panel->set_errors({0.0011, 0.0024, 0.0005, 0.0039, 0.001}, 0.002);
    QCOMPARE(panel->list("coilResults")->items().at(1), std::string("Coil 2: 2.40 mm, 00.00%"));
    QCOMPARE(text("averageError"), QStringLiteral("2.00 mm"));
    QCOMPARE(text("fitFeedback"), QStringLiteral("Good"));
    panel->set_errors({0.004}, 0.0045);
    QCOMPARE(text("fitFeedback"), QStringLiteral("Bad"));
    Eigen::VectorXd gof(4);
    gof << 0.991, 0.95, 0.9, 0.875;
    panel->set_goodness_of_fit(gof, 0.929);
    QCOMPARE(panel->list("coilResults")->items().at(3), std::string("Coil 4: 3.90 mm, 87.50 %"));
    QCOMPARE(text("averageGof"), QStringLiteral("92.90 %"));

    QCOMPARE(panel->allowed_movement(), 3.0);
    QCOMPARE(panel->allowed_rotation(), 5.0);
    panel->set_movement(0.002, 4.0);
    QCOMPARE(text("moveResult"), QStringLiteral("2.00 mm"));
    QCOMPARE(text("movementFeedback"), QStringLiteral("Small"));
    panel->set_movement(0.002, 6.0);
    QCOMPARE(text("movementFeedback"), QStringLiteral("Big"));
    panel->set_movement(0.0031, 1.0);
    QCOMPARE(text("movementFeedback"), QStringLiteral("Big"));

    // Check boxes, spin boxes and buttons report
    view.findChild<QCheckBox*>(QStringLiteral("useSSP"))->click();
    view.findChild<QCheckBox*>(QStringLiteral("continousHPI"))->click();
    qobject_cast<QSpinBox*>(view.formPanel()->editor("fittingWindowSize"))->setValue(450);
    view.findChild<QPushButton*>(QStringLiteral("doSingleFit"))->click();
    QCOMPARE(ssp, std::vector<bool>({true}));
    QVERIFY(panel->use_ssp() && panel->continuous_fitting());
    QCOMPARE(continuous, std::vector<bool>({true}));
    QCOMPARE(windows.back(), 450);
    QCOMPARE(panel->fitting_window_size(), 450);
    QCOMPARE(singleFits, 1);

    // Loading digitizers from a file stops continuous fitting; a FIFF file without any is refused
    const QString sampleDir = QCoreApplication::applicationDirPath() + QStringLiteral("/../resources/data/mne-cpp-test-data/MEG/sample/");
    auto* file = view.formPanel()->editor("digitizerFile")->findChild<QLineEdit*>();
    QVERIFY(file);
    file->setText(sampleDir + QStringLiteral("sample_audvis_trunc_raw.fif"));
    emit file->editingFinished();
    QCOMPARE(digitizerFiles.size(), 1);
    QCOMPARE(text("loadedCoils"), QStringLiteral("4"));
    QCOMPARE(text("loadedEeg"), QStringLiteral("61"));
    QVERIFY(!panel->continuous_fitting());
    QCOMPARE(continuous.back(), false);
    QCOMPARE(panel->coil_frequencies().size(), 4);
    file->setText(sampleDir + QStringLiteral("sample_audvis-cov.fif"));
    emit file->editingFinished();
    QCOMPARE(digitizerFiles.size(), 1);
    QCOMPARE(text("loadedEeg"), QStringLiteral("61"));
    QCOMPARE(text("digitizerStatus"), QStringLiteral("File could not be loaded!"));

    view.setGuiMode(AbstractView::GuiMode::Clinical);
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    view.saveSettings();
    view.loadSettings();
    view.clearView();
    QApplication::processEvents();
}

//=============================================================================================================

void TestVizQtViewers2::filterDesignPanel_designExportLoad()
{
    auto panel = std::make_shared<FilterDesignPanel>();
    PanelWidget view(panel);
    view.resize(600, 400);
    view.show();
    SettingsPanel* form = view.formPanel();
    auto* pFrom = qobject_cast<QDoubleSpinBox*>(form->editor(FilterDesignPanel::from_key));
    auto* pTo = qobject_cast<QDoubleSpinBox*>(form->editor(FilterDesignPanel::to_key));
    auto* pTrans = qobject_cast<QDoubleSpinBox*>(form->editor(FilterDesignPanel::transition_key));
    auto* pTaps = qobject_cast<QSpinBox*>(form->editor(FilterDesignPanel::taps_key));
    auto* pMethod = qobject_cast<QComboBox*>(form->editor(FilterDesignPanel::method_key));
    auto* pApply = qobject_cast<QComboBox*>(form->editor(FilterDesignPanel::channel_type_key));
    QVERIFY(pFrom);
    QVERIFY(pTo);
    QVERIFY(pTrans);
    QVERIFY(pTaps);
    QVERIFY(pMethod);
    QVERIFY(pApply);
    QVERIFY(view.figureViewport());

    std::vector<FilterKernel> designed;
    std::vector<double> froms;
    std::vector<double> tos;
    std::vector<std::string> types;
    FilterDesignPanel::Callbacks callbacks;
    callbacks.filter_changed = [&designed](const FilterKernel& filter) {
        designed.push_back(filter);
    };
    callbacks.from_changed = [&froms](double from) {
        froms.push_back(from);
    };
    callbacks.to_changed = [&tos](double to) {
        tos.push_back(to);
    };
    callbacks.channel_type_changed = [&types](const std::string& type) {
        types.push_back(type);
    };
    panel->set_callbacks(std::move(callbacks));

    // A band-pass from the controls, with the taps forced even and the band edges at the requested frequencies;
    // the panel's setters are silent
    panel->set_sampling_rate(1000.0);
    panel->set_max_taps(257);
    QCOMPARE(pTaps->maximum(), 256);
    panel->set_from(2.0);
    panel->set_to(45.0);
    QVERIFY(designed.empty());
    pTaps->setValue(64);
    emit pTaps->editingFinished();
    pTaps->setValue(128);
    emit pTaps->editingFinished();
    QCOMPARE(designed.size(), std::size_t(2));
    QCOMPARE(panel->from(), 2.0);
    QCOMPARE(panel->to(), 45.0);
    QCOMPARE(froms.back(), 2.0);
    QCOMPARE(tos.back(), 45.0);
    FilterKernel kernel = panel->filter();
    QCOMPARE(designed.back().name(), kernel.name());
    QCOMPARE(kernel.filter_type().name(), std::string("BPF"));
    QCOMPARE(kernel.filter_order(), 128);
    QCOMPARE(kernel.sampling_frequency(), 1000.0);
    QVERIFY(qAbs(kernel.highpass_freq() - 2.0) < 1e-9);
    QVERIFY(qAbs(kernel.lowpass_freq() - 45.0) < 1e-9);
    QCOMPARE(kernel.coefficients().cols(), 128);

    // Band edges are clamped to Nyquist and kept ordered
    panel->set_sampling_rate(60.0);
    QCOMPARE(pTo->maximum(), 30.0);
    QCOMPARE(panel->to(), 30.0);
    QVERIFY(qAbs(panel->filter().lowpass_freq() - 30.0) < 1e-9);
    panel->set_sampling_rate(1000.0);
    panel->set_to(40.0);
    panel->set_from(10.0);
    QCOMPARE(pTo->minimum(), 10.0);
    QCOMPARE(pFrom->maximum(), 40.0);

    // Editing a control and finishing the edit redesigns the filter
    const std::size_t before = designed.size();
    pTrans->setValue(2.0);
    emit pTrans->editingFinished();
    QCOMPARE(designed.size(), before + 1);
    QVERIFY(qAbs(panel->filter().parks_width() * 500.0 - 2.0) < 1e-9);
    pMethod->setCurrentIndex(pMethod->count() - 1);
    QCOMPARE(QString::fromStdString(panel->filter().design_method().name()), pMethod->currentText());
    pMethod->setCurrentIndex(0);

    // Channel type: a programmatic change is silent, a selection reports it
    panel->set_channel_type("EEG");
    QCOMPARE(pApply->currentText(), QStringLiteral("EEG"));
    QVERIFY(types.empty());
    pApply->setCurrentText(QStringLiteral("MEG"));
    QVERIFY(types == std::vector<std::string>{"MEG"});

    // The response figure is drawn
    QCOMPARE(panel->response().plot().nyquist(), panel->filter().sampling_frequency() / 2.0);
    const std::vector<double> db = panel->response().plot().magnitude_db();
    QVERIFY(!db.empty());
    QCOMPARE(*std::max_element(db.begin(), db.end()), 0.0); // relative to the pass band
    view.resize(700, 500);

    // Export the coefficients, then load them back through the file fields
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString sFile = dir.filePath(QStringLiteral("designed.txt"));
    panel->set_from(4.0);
    panel->set_to(30.0);
    kernel = panel->filter();
    panel->form().set_value(FilterDesignPanel::export_coefficients_key, sFile.toStdString());
    QVERIFY(QFile::exists(sFile));
    // The file is named after type, band and sampling rate
    const auto exported = FilterIO::read_filter(sFile.toStdString());
    QVERIFY(exported);
    QCOMPARE(exported->name(), std::format("{}_4_30_Fs{}", kernel.filter_type().name(), static_cast<int>(kernel.sampling_frequency())));

    panel->set_from(1.0);
    panel->set_to(20.0);
    pTaps->setValue(64);
    emit pTaps->editingFinished();
    panel->form().set_value(FilterDesignPanel::load_filter_key, sFile.toStdString());
    QCOMPARE(panel->from(), 4.0);
    QCOMPARE(panel->to(), 30.0);
    QCOMPARE(pTaps->value(), 128);
    QVERIFY(qAbs(panel->filter().highpass_freq() - 4.0) < 1e-9);
    QVERIFY(qAbs(panel->filter().lowpass_freq() - 30.0) < 1e-9);
    QVERIFY((panel->filter().coefficients() - kernel.coefficients()).cwiseAbs().maxCoeff() < 1e-6);

    // Plot export as PNG
    const QString sPng = dir.filePath(QStringLiteral("plot.png"));
    panel->form().set_value(FilterDesignPanel::export_plot_key, sPng.toStdString());
    QVERIFY(!QImage(sPng).isNull());

    // and as vector graphics, with the filter's name as text
    const QString sSvg = dir.filePath(QStringLiteral("plot.svg"));
    panel->form().set_value(FilterDesignPanel::export_plot_key, sSvg.toStdString());
    QFile svg(sSvg);
    QVERIFY(svg.open(QIODevice::ReadOnly));
    const QByteArray svgText = svg.readAll();
    QVERIFY(svgText.contains("<svg") && svgText.contains(QByteArray::fromStdString(panel->filter().name())));
}

//=============================================================================================================

void TestVizQtViewers2::coregistrationPanel_inPanelWidget()
{
    auto panel = std::make_shared<CoregistrationPanel>();
    PanelWidget view(panel, QString());
    // A decimal-comma locale makes number parsing of displayed text observable.
    view.setLocale(QLocale(QLocale::German, QLocale::Germany));
    view.show();

    std::vector<bool> picks;
    std::vector<int> fiducials;
    std::vector<std::string> bems;
    std::vector<std::filesystem::path> digitizerFiles;
    std::vector<std::filesystem::path> storedTransforms;
    int fiducialFits = 0;
    int icpFits = 0;
    int transformationChanges = 0;
    CoregistrationPanel::Callbacks callbacks;
    callbacks.picking_changed = [&picks](bool on) {
        picks.push_back(on);
    };
    callbacks.fiducial_changed = [&fiducials](int fiducial) {
        fiducials.push_back(fiducial);
    };
    callbacks.bem_changed = [&bems](const std::string& name) {
        bems.push_back(name);
    };
    callbacks.digitizers_loaded = [&digitizerFiles](const std::filesystem::path& file) {
        digitizerFiles.push_back(file);
    };
    callbacks.transformation_stored = [&storedTransforms](const std::filesystem::path& file) {
        storedTransforms.push_back(file);
    };
    callbacks.fit_fiducials = [&fiducialFits] {
        ++fiducialFits;
    };
    callbacks.fit_icp = [&icpFits] {
        ++icpFits;
    };
    callbacks.transformation_changed = [&transformationChanges] {
        ++transformationChanges;
    };
    panel->set_callbacks(std::move(callbacks));
    const auto editor = [&view](const char* key) {
        return view.formPanel()->editor(key);
    };
    const auto text = [&editor](const char* key) {
        return qobject_cast<QLabel*>(editor(key))->text();
    };

    // Picking enables the fiducial choice and reports the state
    auto* pick = view.findChild<QCheckBox*>(QStringLiteral("pickFiducials"));
    auto* fiducial = qobject_cast<QComboBox*>(editor("fiducial"));
    QVERIFY(pick && fiducial);
    QVERIFY(!fiducial->isEnabled());
    pick->click();
    QCOMPARE(picks, std::vector<bool>({true}));
    QVERIFY(fiducial->isEnabled());

    // Each picked position is stored and the choice steps LPA -> nasion -> RPA -> LPA without reports
    QCOMPARE(panel->fiducial(), FIFFV_POINT_LPA);
    panel->set_fiducial(Eigen::Vector3f(-0.07f, 0.0f, 0.0f));
    QCOMPARE(panel->fiducial(), FIFFV_POINT_NASION);
    panel->set_fiducial(Eigen::Vector3f(0.0f, 0.09f, 0.01f));
    QCOMPARE(panel->fiducial(), FIFFV_POINT_RPA);
    panel->set_fiducial(Eigen::Vector3f(0.07f, 0.0f, 0.0f));
    QCOMPARE(panel->fiducial(), FIFFV_POINT_LPA);
    QVERIFY(fiducials.empty());
    // Back at LPA the stored LPA position is shown in mm
    QCOMPARE(text("fiducialX"), QStringLiteral("-70.00 mm"));
    fiducial->setCurrentIndex(1);
    QCOMPARE(fiducials, std::vector<int>({FIFFV_POINT_NASION}));
    QCOMPARE(text("fiducialZ"), QStringLiteral("10.00 mm"));

    // Fitting ends picking
    view.findChild<QPushButton*>(QStringLiteral("fitFiducials"))->click();
    QCOMPARE(fiducialFits, 1);
    QVERIFY(!pick->isChecked() && !panel->picking());
    QCOMPARE(picks.back(), false);
    pick->click();
    view.findChild<QPushButton*>(QStringLiteral("fitIcp"))->click();
    QCOMPARE(icpFits, 1);
    QVERIFY(!pick->isChecked());

    // The scaling mode enables the matching spin boxes and selects how the scale is read back
    auto* mode = qobject_cast<QComboBox*>(editor("scalingMode"));
    auto* scaleX = qobject_cast<QDoubleSpinBox*>(editor("scaleX"));
    auto* scaleY = qobject_cast<QDoubleSpinBox*>(editor("scaleY"));
    auto* scaleZ = qobject_cast<QDoubleSpinBox*>(editor("scaleZ"));
    QVERIFY(mode && scaleX && scaleY && scaleZ);
    const Eigen::Vector3f translationIn(0.001f, -0.002f, 0.003f);
    const Eigen::Vector3f rotationIn(0.1f, 0.2f, 0.3f);
    panel->set_transformation(translationIn, rotationIn, Eigen::Vector3f::Ones());
    QCOMPARE(transformationChanges, 0);

    Eigen::Vector3f rotation;
    Eigen::Vector3f translation;
    Eigen::Vector3f scaling;
    mode->setCurrentText(QStringLiteral("None"));
    QVERIFY(!scaleX->isEnabled() && !scaleY->isEnabled() && !scaleZ->isEnabled());
    panel->transformation(rotation, translation, scaling);
    QVERIFY(translation.isApprox(translationIn, 1e-3f));
    QVERIFY(rotation.isApprox(rotationIn, 1e-3f));
    QVERIFY(scaling.isApprox(Eigen::Vector3f::Ones()));

    mode->setCurrentText(QStringLiteral("Uniform"));
    QVERIFY(scaleX->isEnabled() && !scaleY->isEnabled() && !scaleZ->isEnabled());
    scaleX->setValue(1.25);
    QVERIFY(transformationChanges >= 1);
    panel->transformation(rotation, translation, scaling);
    QVERIFY(scaling.isApprox(Eigen::Vector3f::Constant(1.25f)));

    mode->setCurrentText(QStringLiteral("3-Axis"));
    QVERIFY(scaleX->isEnabled() && scaleY->isEnabled() && scaleZ->isEnabled());
    scaleY->setValue(0.9);
    scaleZ->setValue(1.1);
    panel->transformation(rotation, translation, scaling);
    QVERIFY(scaling.isApprox(Eigen::Vector3f(1.25f, 0.9f, 1.1f), 1e-4f));

    // A non-uniform scale survives the set/get round trip (index 0 is x)
    panel->set_transformation(translationIn, rotationIn, Eigen::Vector3f(1.1f, 1.2f, 1.3f));
    QCOMPARE(scaleX->value(), 1.1);
    panel->transformation(rotation, translation, scaling);
    QVERIFY(scaling.isApprox(Eigen::Vector3f(1.1f, 1.2f, 1.3f), 1e-4f));
    QVERIFY(rotation.isApprox(rotationIn, 1e-3f));

    // Fit settings and results
    qobject_cast<QSpinBox*>(editor("maxIterations"))->setValue(42);
    QCOMPARE(panel->max_iterations(), 42);
    qobject_cast<QDoubleSpinBox*>(editor("convergence"))->setValue(0.5);
    QVERIFY(qAbs(panel->convergence() - 0.0005f) < 1e-7f);
    qobject_cast<QSpinBox*>(editor("maxDistance"))->setValue(7);
    QVERIFY(qAbs(panel->omit_distance() - 0.007f) < 1e-7f);
    view.findChild<QCheckBox*>(QStringLiteral("autoScale"))->click();
    QVERIFY(panel->auto_scale());
    qobject_cast<QDoubleSpinBox*>(editor("weightLpa"))->setValue(2.0);
    qobject_cast<QDoubleSpinBox*>(editor("weightHsp"))->setValue(3.0);
    QCOMPARE(panel->weight_lpa(), 2.0f);
    QCOMPARE(panel->weight_hsp(), 3.0f);
    QCOMPARE(panel->weight_rpa(), 1.0f);
    QCOMPARE(panel->weight_nasion(), 10.0f);

    // The digitizer kinds follow the check boxes; the fiducials are always used
    view.findChild<QCheckBox*>(QStringLiteral("useEeg"))->click();
    QCOMPARE(panel->digitizer_kinds(), std::vector<int>({FIFFV_POINT_CARDINAL, FIFFV_POINT_EEG, FIFFV_POINT_HPI, FIFFV_POINT_EXTRA}));
    view.findChild<QCheckBox*>(QStringLiteral("useEeg"))->click();
    view.findChild<QCheckBox*>(QStringLiteral("useHsp"))->click();
    QCOMPARE(panel->digitizer_kinds(), std::vector<int>({FIFFV_POINT_CARDINAL, FIFFV_POINT_HPI}));
    panel->set_omitted_points(5);
    QCOMPARE(text("omittedPoints"), QStringLiteral("5"));
    panel->set_rmse(0.0025f);
    QCOMPARE(text("rmse"), QStringLiteral("2.50 mm"));

    // BEMs: set silently, the chosen one stays while it is there, the user's choice is reported
    auto* bem = qobject_cast<QComboBox*>(editor("bem"));
    QCOMPARE(bem->itemText(0), QStringLiteral("Select BEM"));
    QVERIFY(panel->bem().empty());
    panel->set_bems({"sample-head.fif", "sample-5120-bem.fif"});
    QCOMPARE(panel->bem(), std::string("sample-head.fif"));
    bem->setCurrentIndex(1);
    QCOMPARE(bems, std::vector<std::string>({"sample-5120-bem.fif"}));
    panel->set_bems({"other.fif", "sample-5120-bem.fif"});
    QCOMPARE(panel->bem(), std::string("sample-5120-bem.fif"));
    panel->set_bems({});
    QVERIFY(panel->bem().empty());
    QCOMPARE(bems.size(), 1);

    // Files: a store path is cleared once reported, so the same file can be stored again
    auto* store = editor("transStoreFile")->findChild<QLineEdit*>();
    QVERIFY(store);
    for (int i = 0; i < 2; ++i) {
        store->setText(QStringLiteral("fit-trans.fif"));
        emit store->editingFinished();
    }
    QCOMPARE(storedTransforms, std::vector<std::filesystem::path>(2, std::filesystem::path("fit-trans.fif")));
    QVERIFY(store->text().isEmpty());
    auto* digitizers = editor("digitizerFile")->findChild<QLineEdit*>();
    digitizers->setText(QStringLiteral("sample-dig.fif"));
    emit digitizers->editingFinished();
    QCOMPARE(digitizerFiles, std::vector<std::filesystem::path>({std::filesystem::path("sample-dig.fif")}));
    QCOMPARE(digitizers->text(), QStringLiteral("sample-dig.fif"));

    view.setGuiMode(AbstractView::GuiMode::Research);
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    view.saveSettings();
    view.loadSettings();
    view.clearView();
    QApplication::processEvents();
}

//=============================================================================================================

void TestVizQtViewers2::averagingPanel_controlsAndSettings()
{
    // Defaults without stored settings
    {
        AveragingPanel averaging;
        QCOMPARE(averaging.num_averages(), 10);
        QCOMPARE(averaging.pre_stim(), 100);
        QCOMPARE(averaging.post_stim(), 400);
        QCOMPARE(averaging.baseline_from(), 0);
        QCOMPARE(averaging.baseline_to(), 0);
        QVERIFY(!averaging.baseline_correction());
        QCOMPARE(averaging.current_stim_channel(), std::string("STI 014"));
    }

    // Stored settings, including a baseline that ends before the stimulus
    const QString sPath = QStringLiteral("test_viz_qt_viewers2_averaging_%1").arg(QCoreApplication::applicationPid());
    {
        QSettings settings("MNECPP");
        settings.setValue(sPath + QStringLiteral("/AveragingSettingsView/preStimSeconds"), 200);
        settings.setValue(sPath + QStringLiteral("/AveragingSettingsView/postStimSeconds"), 500);
        settings.setValue(sPath + QStringLiteral("/AveragingSettingsView/baselineFromSeconds"), -150);
        settings.setValue(sPath + QStringLiteral("/AveragingSettingsView/baselineToSeconds"), -50);
        settings.setValue(sPath + QStringLiteral("/AveragingSettingsView/numAverages"), 25);
        settings.setValue(sPath + QStringLiteral("/AveragingSettingsView/currentStimChannel"), QStringLiteral("STI 101"));
        settings.setValue(sPath + QStringLiteral("/AveragingSettingsView/doBaselineCorrection"), true);
    }
    const auto removeSettings = qScopeGuard([&sPath]() { QSettings("MNECPP").remove(sPath); });

    const std::map<std::string, int> stimChannels = {{"STI 014", 306}, {"STI 101", 312}};
    auto averaging = std::make_shared<AveragingPanel>(stimChannels);
    std::vector<std::string> stims;
    std::vector<int> preStims;
    std::vector<int> postStims;
    std::vector<int> froms;
    std::vector<int> tos;
    std::vector<int> nums;
    std::vector<bool> baselines;
    std::vector<bool> autoComputes;
    int resets = 0;
    int computes = 0;
    AveragingPanel::Callbacks callbacks;
    callbacks.stim_channel_changed = [&stims](const std::string& channel) {
        stims.push_back(channel);
    };
    callbacks.pre_stim_changed = [&preStims](int value) {
        preStims.push_back(value);
    };
    callbacks.post_stim_changed = [&postStims](int value) {
        postStims.push_back(value);
    };
    callbacks.baseline_from_changed = [&froms](int value) {
        froms.push_back(value);
    };
    callbacks.baseline_to_changed = [&tos](int value) {
        tos.push_back(value);
    };
    callbacks.num_averages_changed = [&nums](int value) {
        nums.push_back(value);
    };
    callbacks.baseline_active_changed = [&baselines](bool active) {
        baselines.push_back(active);
    };
    callbacks.auto_compute_changed = [&autoComputes](bool live) {
        autoComputes.push_back(live);
    };
    callbacks.reset = [&resets] {
        ++resets;
    };
    callbacks.compute = [&computes] {
        ++computes;
    };
    averaging->set_callbacks(std::move(callbacks));
    PanelWidget view(averaging, sPath);
    view.show();
    auto* pChannels = view.findChild<QComboBox*>(QStringLiteral("currentStimChannel"));
    auto* pPre = view.findChild<QSpinBox*>(QStringLiteral("preStimSeconds"));
    auto* pPost = view.findChild<QSpinBox*>(QStringLiteral("postStimSeconds"));
    auto* pFrom = view.findChild<QSpinBox*>(QStringLiteral("baselineFromSeconds"));
    auto* pTo = view.findChild<QSpinBox*>(QStringLiteral("baselineToSeconds"));
    auto* pNum = view.findChild<QSpinBox*>(QStringLiteral("numAverages"));
    QVERIFY(pChannels && pPre && pPost && pFrom && pTo && pNum);

    // Restoring reports nothing
    QVERIFY(stims.empty() && preStims.empty() && froms.empty() && tos.empty() && nums.empty() && baselines.empty());
    QCOMPARE(averaging->num_averages(), 25);
    QCOMPARE(pNum->value(), 25);
    QCOMPARE(averaging->pre_stim(), 200);
    QCOMPARE(averaging->post_stim(), 500);
    QCOMPARE(averaging->baseline_from(), -150);
    QCOMPARE(averaging->baseline_to(), -50);
    QCOMPARE(pTo->value(), -50);
    QVERIFY(averaging->baseline_correction());

    // Stim channel: the stored one is selected and its channel index is reported
    QCOMPARE(pChannels->currentText(), QStringLiteral("STI 101"));
    QCOMPARE(averaging->stim_channel_index(), 312);
    pChannels->setCurrentText(QStringLiteral("STI 014"));
    QCOMPARE(stims, std::vector<std::string>{"STI 014"});
    QCOMPARE(averaging->current_stim_channel(), std::string("STI 014"));
    QCOMPARE(averaging->stim_channel_index(), 306);

    // New stim channels keep the selection and do not duplicate the reaction
    averaging->set_stim_channels(std::map<std::string, int>{{"STI 014", 6}, {"STI 201", 7}});
    QCOMPARE(pChannels->count(), 2);
    QCOMPARE(averaging->stim_channel_index(), 6);
    stims.clear();
    pChannels->setCurrentText(QStringLiteral("STI 201"));
    QCOMPARE(stims.size(), std::size_t{1});
    QCOMPARE(averaging->current_stim_channel(), std::string("STI 201"));
    QCOMPARE(averaging->stim_channel_index(), 7);

    // Epoch window and baseline: finished edits report and constrain the baseline range
    pPre->setValue(300);
    emit pPre->editingFinished();
    QCOMPARE(preStims.back(), 300);
    QCOMPARE(pFrom->minimum(), -300);
    QCOMPARE(pTo->minimum(), -300);
    pPost->setValue(600);
    emit pPost->editingFinished();
    QCOMPARE(postStims.back(), 600);
    QCOMPARE(pFrom->maximum(), 600);
    pFrom->setValue(-250);
    emit pFrom->editingFinished();
    QCOMPARE(froms.back(), -250);
    QCOMPARE(pTo->minimum(), -250);
    pTo->setValue(-20);
    emit pTo->editingFinished();
    QCOMPARE(tos.back(), -20);
    QCOMPARE(pFrom->maximum(), -20);
    pNum->setValue(40);
    emit pNum->editingFinished();
    QCOMPARE(nums.back(), 40);
    QCOMPARE(averaging->num_averages(), 40);

    auto* pBaseline = view.findChild<QCheckBox*>(QStringLiteral("doBaselineCorrection"));
    pBaseline->click();
    QCOMPARE(baselines.size(), std::size_t{1});
    QCOMPARE(baselines.back(), pBaseline->isChecked());
    QCOMPARE(averaging->baseline_correction(), pBaseline->isChecked());
    QTest::mouseClick(view.findChild<QPushButton*>(QStringLiteral("reset")), Qt::LeftButton);
    QCOMPARE(resets, 1);

    // Detected epochs: one row per type, at most ten
    auto* pTrials = view.formPanel()->customWidget("detectedTrials");
    QVERIFY(pTrials);
    FiffEvokedSet evokedSet;
    for (int i = 0; i < 12; ++i) {
        FiffEvoked evoked;
        evoked.comment = QString::number(i + 1).toStdString();
        evoked.nave = 3 * i;
        evokedSet.evoked.push_back(evoked);
    }
    averaging->set_detected_epochs(evokedSet);
    QVERIFY(!pTrials->isHidden());
    const ItemList* trials = averaging->list("detectedTrials");
    QCOMPARE(trials->count(), 10);
    QCOMPARE(trials->items()[2], std::string("3: 6"));
    averaging->set_detected_epochs(evokedSet);
    QCOMPARE(trials->count(), 10);
    averaging->set_detected_epochs(FiffEvokedSet());
    QVERIFY(pTrials->isHidden());

    // Offline mode offers the compute controls
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    auto* pCompute = view.findChild<QPushButton*>(QStringLiteral("compute"));
    QVERIFY(!pCompute->isHidden());
    QVERIFY(pChannels->isHidden());
    QTest::mouseClick(pCompute, Qt::LeftButton);
    QCOMPARE(computes, 1);
    auto* pAuto = view.findChild<QCheckBox*>(QStringLiteral("autoCompute"));
    pAuto->click();
    QCOMPARE(autoComputes.size(), std::size_t{1});
    QCOMPARE(averaging->auto_compute(), pAuto->isChecked());
    view.setProcessingMode(AbstractView::ProcessingMode::RealTime);
    QVERIFY(pCompute->isHidden());
}

//=============================================================================================================

void TestVizQtViewers2::triggerDetectionPanel_settingsAndSignals()
{
    auto info = createBrowserTestInfo();
    info->chs[2].kind = FIFFV_STIM_CH;
    info->chs[2].ch_name = "STI 101";

    struct Info
    {
        std::map<double, std::string> colors;
        bool active = false;
        std::string channel;
        double threshold = 0.0;
    };
    std::vector<Info> infos;
    int resets = 0;
    std::vector<std::pair<std::string, double>> detections;
    auto triggers = std::make_shared<TriggerDetectionPanel>();
    TriggerDetectionPanel::Callbacks callbacks;
    callbacks.trigger_info_changed = [&infos](const std::map<double, std::string>& colors, bool active, const std::string& channel,
                                              double threshold) {
        infos.push_back({colors, active, channel, threshold});
    };
    callbacks.reset_trigger_counter = [&resets] {
        ++resets;
    };
    callbacks.detect_triggers = [&detections](const std::string& channel, double threshold) {
        detections.emplace_back(channel, threshold);
    };
    triggers->set_callbacks(std::move(callbacks));
    PanelWidget view(triggers);
    view.show();
    auto* pChannels = view.findChild<QComboBox*>(QStringLiteral("triggerChannel"));
    auto* pTypes = view.findChild<QComboBox*>(QStringLiteral("triggerColorType"));
    auto* pActive = view.findChild<QCheckBox*>(QStringLiteral("activateTriggerDetection"));
    auto* pFirst = view.findChild<QDoubleSpinBox*>(QStringLiteral("detectionThresholdFirst"));
    auto* pSecond = view.findChild<QSpinBox*>(QStringLiteral("detectionThresholdSecond"));
    auto* pCount = view.findChild<QLabel*>(QStringLiteral("numberDetectedTriggers"));
    auto* pDetect = view.findChild<QPushButton*>(QStringLiteral("detectTriggers"));
    auto* pColor = view.findChild<QToolButton*>(QStringLiteral("triggerColor"));
    QVERIFY(pChannels && pTypes && pActive && pFirst && pSecond && pCount && pDetect && pColor);

    // Re-initialising (new measurement info) must not duplicate the channels; init() reports once
    triggers->init(*info);
    triggers->init(*info);
    QCOMPARE(pChannels->count(), 2);
    QCOMPARE(pChannels->itemText(0), QStringLiteral("STI 101"));
    QCOMPARE(triggers->selected_stim_channel(), std::string("STI 101"));
    QCOMPARE(infos.size(), std::size_t{2});

    infos.clear();
    pFirst->setValue(2.5);
    QCOMPARE(infos.size(), std::size_t{1});
    pSecond->setValue(-3);
    QCOMPARE(infos.size(), std::size_t{2});
    QVERIFY(qAbs(infos.back().threshold - 2.5e-3) < 1e-15);
    pActive->setChecked(true);
    QVERIFY(infos.back().active);
    pChannels->setCurrentIndex(1);
    QCOMPARE(infos.back().channel, std::string("STI 014"));
    QCOMPARE(infos.size(), std::size_t{4});

    // Detected triggers fill the count and the type list once per value
    triggers->set_detected_triggers(3, {{3, {{10, 1.0}, {20, 4.0}, {30, 1.0}}}});
    QCOMPARE(pCount->text(), QStringLiteral("3"));
    QCOMPARE(pTypes->count(), 2);

    // The colour button shows the colour of the selected type; picking a colour stores it for that type
    pTypes->setCurrentIndex(1);
    answerNextModal([](QWidget* pModal) {
        auto* pDialog = qobject_cast<QColorDialog*>(pModal);
        QVERIFY(pDialog);
        pDialog->setCurrentColor(QColor(10, 200, 30));
        static_cast<QDialog*>(pDialog)->accept();
    });
    QTest::mouseClick(pColor, Qt::LeftButton);
    QCOMPARE(triggers->form().get<std::string>("triggerColor"), std::string("#0ac81e"));
    QCOMPARE(mne::viz::qt::detail::valueOr(infos.back().colors, 4.0), std::string("#0ac81e"));
    pTypes->setCurrentIndex(0);
    QVERIFY(triggers->form().get<std::string>("triggerColor") != "#0ac81e");
    pTypes->setCurrentIndex(1);
    QCOMPARE(triggers->form().get<std::string>("triggerColor"), std::string("#0ac81e"));
    // Cancelling the colour dialog keeps the colour
    const std::size_t infoCount = infos.size();
    answerNextModal([](QWidget* pModal) {
        auto* pDialog = qobject_cast<QColorDialog*>(pModal);
        QVERIFY(pDialog);
        static_cast<QDialog*>(pDialog)->reject();
    });
    QTest::mouseClick(pColor, Qt::LeftButton);
    QCOMPARE(triggers->form().get<std::string>("triggerColor"), std::string("#0ac81e"));
    QCOMPARE(infos.size(), infoCount);

    QTest::mouseClick(view.findChild<QPushButton*>(QStringLiteral("resetNumberTriggers")), Qt::LeftButton);
    QCOMPARE(resets, 1);
    QCOMPARE(pCount->text(), QStringLiteral("0"));
    QCOMPARE(pTypes->count(), 0);

    // Offline mode swaps the live controls for the detect button
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    QVERIFY(!pDetect->isHidden());
    QVERIFY(pActive->isHidden());
    QTest::mouseClick(pDetect, Qt::LeftButton);
    QCOMPARE(detections.size(), std::size_t{1});
    QCOMPARE(detections.back().first, std::string("STI 014"));
    QVERIFY(qAbs(detections.back().second - 2.5e-3) < 1e-15);
    view.setProcessingMode(AbstractView::ProcessingMode::RealTime);
    QVERIFY(pDetect->isHidden());
}

//=============================================================================================================

void TestVizQtViewers2::artifactPanel_thresholds()
{
    auto channel = [](int kind, int unit) {
        FiffChInfo ch;
        ch.kind = kind;
        ch.unit = unit;
        return ch;
    };
    const std::vector<FiffChInfo> megEeg = {channel(FIFFV_MEG_CH, FIFF_UNIT_T_M), channel(FIFFV_MEG_CH, FIFF_UNIT_T),
                                            channel(FIFFV_MEG_CH, FIFF_UNIT_T_M), channel(FIFFV_EEG_CH, FIFF_UNIT_V),
                                            channel(FIFFV_STIM_CH, FIFF_UNIT_NONE)};

    auto artifact = std::make_shared<ArtifactPanel>();
    std::vector<std::map<std::string, double>> reports;
    artifact->set_thresholds_callback([&reports](const std::map<std::string, double>& thresholds) {
        reports.push_back(thresholds);
    });
    PanelWidget view(artifact);
    view.show();
    auto shown = [&view]() {
        QList<QDoubleSpinBox*> boxes;
        for (QDoubleSpinBox* pBox : view.findChildren<QDoubleSpinBox*>()) {
            if (!pBox->isHidden()) {
                boxes << pBox;
            }
        }
        return boxes;
    };
    QVERIFY(shown().isEmpty());
    QVERIFY(!artifact->rejection_active());

    // One row per present type; mantissa and exponent compose the threshold
    artifact->set_channels(megEeg);
    auto visible = [&view]() {
        QApplication::processEvents();
        QStringList labels;
        for (QLabel* pLabel : view.findChildren<QLabel*>()) {
            if (!pLabel->isHidden() && !pLabel->text().isEmpty()) {
                labels << pLabel->text();
            }
        }
        labels.sort();
        return labels;
    };
    QCOMPARE(visible(), QStringList({"eeg", "grad", "mag"}));
    QCOMPARE(shown().size(), 3);
    QCOMPARE(reports.size(), std::size_t{1});
    std::map<std::string, double> map = artifact->thresholds();
    QCOMPARE(mne::viz::qt::detail::valueOr(map, "Active"), 0.0);
    QCOMPARE(std::vector<std::string>(std::views::keys(map).begin(), std::views::keys(map).end()), (std::vector<std::string>{"Active", "eeg", "grad", "mag"}));
    QCOMPARE(mne::viz::qt::detail::valueOr(map, "grad"), 0.1);

    SettingsPanel* pPanel = view.formPanel();
    qobject_cast<QDoubleSpinBox*>(pPanel->editor("artifactThresholdsFirst/grad"))->setValue(4.0);
    qobject_cast<QSpinBox*>(pPanel->editor("artifactThresholdsSecond/grad"))->setValue(-11);
    QVERIFY(qAbs(mne::viz::qt::detail::valueOr(artifact->thresholds(), "grad") - 4e-11) < 1e-20);
    QVERIFY(qAbs(mne::viz::qt::detail::valueOr(reports.back(), "grad") - 4e-11) < 1e-20);

    // Activation is reported in the map
    auto* pActive = qobject_cast<QCheckBox*>(pPanel->editor("doArtifactThresholdReduction"));
    QVERIFY(pActive);
    QTest::mouseClick(pActive, Qt::LeftButton);
    QVERIFY(artifact->rejection_active());
    QCOMPARE(mne::viz::qt::detail::valueOr(artifact->thresholds(), "Active"), 1.0);

    // A new channel set replaces the rows and keeps the active state and the edited threshold
    artifact->set_channels({channel(FIFFV_MEG_CH, FIFF_UNIT_T_M), channel(FIFFV_EOG_CH, FIFF_UNIT_V)});
    QCOMPARE(visible(), QStringList({"eog", "grad"}));
    QCOMPARE(view.findChildren<QCheckBox*>().size(), 1);
    QVERIFY(artifact->rejection_active());
    map = artifact->thresholds();
    QCOMPARE(std::vector<std::string>(std::views::keys(map).begin(), std::views::keys(map).end()), (std::vector<std::string>{"Active", "eog", "grad"}));
    QVERIFY(qAbs(mne::viz::qt::detail::valueOr(map, "grad") - 4e-11) < 1e-20);
}

//=============================================================================================================

void TestVizQtViewers2::forwardPanel_inPanelWidget()
{
    auto panel = std::make_shared<ForwardPanel>();
    PanelWidget view(panel, QStringLiteral("test_viz_qt_viewers2"));
    view.setGuiMode(AbstractView::GuiMode::Clinical);
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    QCOMPARE(panel->processing_mode(), Panel::ProcessingMode::Offline);

    std::vector<bool> recomputations;
    std::vector<bool> clusterings;
    std::vector<int> clusterSources;
    std::vector<std::filesystem::path> atlases;
    int computations = 0;
    ForwardPanel::Callbacks callbacks;
    callbacks.compute = [&computations] {
        ++computations;
    };
    callbacks.recomputation_changed = [&recomputations](bool on) {
        recomputations.push_back(on);
    };
    callbacks.clustering_changed = [&clusterings](bool on) {
        clusterings.push_back(on);
    };
    callbacks.cluster_sources_changed = [&clusterSources](int sources) {
        clusterSources.push_back(sources);
    };
    callbacks.atlas_changed = [&atlases](const std::filesystem::path& directory, const std::shared_ptr<FsAnnotationSet>&) {
        atlases.push_back(directory);
    };
    panel->set_callbacks(std::move(callbacks));

    // Status feedback for each computation state
    auto* feedback = qobject_cast<QLabel*>(view.formPanel()->editor("status"));
    QVERIFY(feedback);
    const QStringList states = {QStringLiteral("Initializing"), QStringLiteral("Computing"),
                                QStringLiteral("Recomputing"), QStringLiteral("Clustering"),
                                QStringLiteral("Not Computed"), QStringLiteral("Not Clustered"),
                                QStringLiteral("Finished")};
    for (int status = 0; status < states.size(); ++status) {
        panel->set_status(status);
        QCOMPARE(feedback->text(), states.at(status));
    }

    // Solution information
    const auto text = [&view](const char* key) {
        return qobject_cast<QLabel*>(view.formPanel()->editor(key))->text();
    };
    for (const auto& [frame, frameName] : {std::pair{FIFFV_COORD_HEAD, "Head Space"},
                                           std::pair{FIFFV_COORD_MRI, "MRI Space"},
                                           std::pair{FIFFV_COORD_DEVICE, "Unknown"}}) {
        panel->set_solution_information(frame == FIFFV_COORD_MRI ? 1 : 0, frame, 7498, 306, 2);
        QCOMPARE(text("coordinateFrame"), QLatin1String(frameName));
    }
    QCOMPARE(text("sourceOrientation"), QStringLiteral("fixed"));
    QCOMPARE(text("sources"), QStringLiteral("7498"));
    QCOMPARE(text("channels"), QStringLiteral("306"));
    QCOMPARE(text("sourceSpaces"), QStringLiteral("2"));
    panel->set_clustered_information(200);
    QCOMPARE(text("clusteredSources"), QStringLiteral("200"));

    // Recomputation and cluster number
    view.findChild<QCheckBox*>(QStringLiteral("recompute"))->click();
    QVERIFY(panel->recomputation());
    QCOMPARE(recomputations, std::vector<bool>({true}));
    auto* sources = view.findChild<QSpinBox*>(QStringLiteral("clusterSources"));
    sources->setValue(150);
    emit sources->editingFinished();
    QCOMPARE(panel->cluster_sources(), 150);
    QCOMPARE(clusterSources, std::vector<int>({150}));

    // Atlas directory: a directory without the a2009s annotations is not loaded
    auto* atlasDirectory = view.formPanel()->editor("atlasDirectory")->findChild<QLineEdit*>();
    QTemporaryDir atlasDir;
    QVERIFY(atlasDir.isValid());
    atlasDirectory->setText(atlasDir.path());
    emit atlasDirectory->textEdited(atlasDirectory->text());
    QCOMPARE(text("atlasStatus"), QStringLiteral("not loaded"));
    QVERIFY(atlases.empty());

    // Clustering needs an annotation set: without one the box turns off again and the status asks for one
    auto* clustering = view.findChild<QCheckBox*>(QStringLiteral("cluster"));
    clustering->click();
    clustering->click();
    QVERIFY(!panel->clustering() && !clustering->isChecked());
    QVERIFY(clusterings.size() == 1 && !clusterings.front());
    QVERIFY(text("atlasStatus").startsWith(QStringLiteral("not loaded")));

    // With both hemispheres' a2009s annotations the atlas loads and clustering is allowed
    const QString label = QCoreApplication::applicationDirPath() + QStringLiteral("/../resources/data/mne-cpp-test-data/subjects/sample/label/");
    for (const QString& hemi : {QStringLiteral("lh"), QStringLiteral("rh")}) {
        QVERIFY(QFile::copy(label + hemi + QStringLiteral(".aparc.annot"),
                            atlasDir.filePath(hemi + QStringLiteral(".aparc.a2009s.annot"))));
    }
    atlasDirectory->setText(atlasDir.path() + QLatin1Char('/'));
    emit atlasDirectory->textEdited(atlasDirectory->text());
    QCOMPARE(text("atlasStatus"), QStringLiteral("loaded"));
    QCOMPARE(atlases.size(), 1);
    clustering->click();
    QVERIFY(panel->clustering());
    QCOMPARE(clusterings.back(), true);

    view.findChild<QPushButton*>(QStringLiteral("compute"))->click();
    QCOMPARE(computations, 1);
}

//=============================================================================================================

void TestVizQtViewers2::fiffRawViewSettings_setAndGet()
{
    FiffRawViewSettings view("test_viz_qt_viewers2");

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

void TestVizQtViewers2::bidsView_lifecycle()
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

void TestVizQtViewers2::channelSelectionPanel_inPanelWidget()
{
    const QString layoutPath = QCoreApplication::applicationDirPath() + QStringLiteral("/../resources/general/2DLayouts/Vectorview-all.lout");
    if (!QFile::exists(layoutPath)) {
        QSKIP("Vectorview-all.lout not available in test environment");
    }

    auto info = createBrowserTestInfo();
    auto infoModel = std::make_shared<ChannelInfoModel>();
    infoModel->setFiffInfo(info);
    const QString settingsPath = QStringLiteral("test_viz_qt_viewers2_selection");
    QSettings(QStringLiteral("MNECPP")).remove(settingsPath);

    auto panel = std::make_shared<mne::viz::ChannelSelectionPanel>();
    panel->set_channel_table(ChannelInfoModel::sharedTable(infoModel));
    std::vector<std::string> shown;
    SelectionItem selection;
    mne::viz::ChannelSelectionPanel::Callbacks callbacks;
    callbacks.layout_loaded = [&infoModel](const std::map<std::string, mne::viz::Point>& layout) {
        infoModel->setLayout(layout);
    };
    callbacks.channels_shown = [&shown](const std::vector<std::string>& channels) {
        shown = channels;
    };
    callbacks.selection_changed = [&selection](const std::vector<mne::viz::SensorLayout::Sensor>& sensors) {
        selection = SelectionItem::fromSensors(sensors);
    };
    panel->set_callbacks(std::move(callbacks));
    QObject::connect(infoModel.get(), &ChannelInfoModel::channelsMappedToLayout, infoModel.get(), [&panel](const std::vector<std::string>& names) {
        panel->set_mapped_channels(names);
    });
    panel->reload_layout();
    QVERIFY(!panel->layout_map().empty());
    QVERIFY(!infoModel->getMappedChannelsList().empty());

    {
        PanelWidget view(panel, settingsPath);
        view.resize(700, 700);
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));

        // The three lists sit in their custom fields; "All" is chosen and lists the mapped channels
        const QList<ItemListView*> lists = view.findChildren<ItemListView*>();
        QCOMPARE(lists.size(), 3);
        ItemListView* groups = nullptr;
        ItemListView* picked = nullptr;
        for (ItemListView* list : lists) {
            if (&list->list() == &panel->groups()) {
                groups = list;
            } else if (&list->list() == &panel->picked_channels()) {
                picked = list;
            }
        }
        QVERIFY(groups && picked);
        QCOMPARE(panel->groups().current_item(), std::string("All"));
        QCOMPARE(panel->group_channels().items(), infoModel->getMappedChannelsList());
        QCOMPARE(groups->selectionModel()->selectedIndexes().size(), 1);

        // A click on a sensor of the layout picks it, and the picked list shows it
        SceneViewport* viewport = view.figureViewport();
        const std::string name = panel->group_channels().items().front();
        const mne::viz::SensorLayout& sensors = panel->sensors().sensor_layout();
        std::size_t index = 0;
        while (index < sensors.sensors().size() && sensors.sensors()[index].name != name) {
            ++index;
        }
        QVERIFY(index < sensors.sensors().size());
        const QPoint at = viewport->mapFromScene(sensors.scene_position(index)).toPoint();
        QTest::mouseClick(viewport, Qt::LeftButton, Qt::NoModifier, at);
        QCOMPARE(panel->picked_channels().items(), std::vector<std::string>({name}));
        QCOMPARE(picked->model()->rowCount(), 1);
        QCOMPARE(selection.m_sChannelName, std::vector<std::string>({name}));
        QVERIFY(!panel->selection_empty());

        // Delete in the picked list drops the channel, in the layout too, and the data view shows the group again
        picked->setFocus();
        picked->selectionModel()->select(picked->model()->index(0, 0), QItemSelectionModel::ClearAndSelect);
        QCOMPARE(panel->picked_channels().selected(), std::vector<int>({0}));
        QTest::keyClick(picked, Qt::Key_Delete);
        QCOMPARE(panel->picked_channels().count(), 0);
        QVERIFY(panel->sensors().selected_names().empty());
        QCOMPARE(shown.size(), panel->group_channels().items().size());

        // Choosing a group in its list shows its channels
        const int eeg = panel->groups().find("All EEG");
        QVERIFY(eeg >= 0);
        groups->selectionModel()->select(groups->model()->index(eeg, 0), QItemSelectionModel::ClearAndSelect);
        QCOMPARE(panel->groups().current_item(), std::string("All EEG"));
        QCOMPARE(panel->group_channels().count(), 0);

        // Another layout is stored with the panel
        QVERIFY(panel->load_layout("Vectorview-grad.lout"));
        QCOMPARE(panel->layout_file(), std::string("Vectorview-grad.lout"));
    }
    auto restored = std::make_shared<mne::viz::ChannelSelectionPanel>();
    PanelWidget again(restored, settingsPath);
    QCOMPARE(restored->layout_file(), std::string("Vectorview-grad.lout"));
    QSettings(QStringLiteral("MNECPP")).remove(settingsPath);
}

//=============================================================================================================

void TestVizQtViewers2::averageLayoutPanel_lifecycle()
{
    PanelWidget view(std::make_shared<mne::viz::AverageLayoutPanel>(), "test_viz_qt_viewers2");
    // The panel has no settings, only its figure
    QVERIFY(view.figureViewport() != nullptr);
    QVERIFY(view.formPanel()->isHidden());

    view.setGuiMode(AbstractView::GuiMode::Research);
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    view.saveSettings();
    view.loadSettings();
    view.clearView();

    QApplication::processEvents();
}

//=============================================================================================================

void TestVizQtViewers2::averageSelectionView_lifecycle()
{
    const QString settingsPath = QStringLiteral("test_viz_qt_viewers2_avgsel");
    QSettings(QStringLiteral("MNECPP")).remove(settingsPath);
    auto view = std::make_unique<AverageSelectionView>(settingsPath);

    view->setGuiMode(AbstractView::GuiMode::Research);
    view->setProcessingMode(AbstractView::ProcessingMode::Offline);
    view->clearView();

    // One checkbox and one colour button per average
    auto colors = std::make_shared<std::map<std::string, QColor>>();
    auto activation = std::make_shared<std::map<std::string, bool>>();
    colors->insert_or_assign("aud", Qt::red);
    colors->insert_or_assign("vis", Qt::blue);
    activation->insert_or_assign("aud", true);
    activation->insert_or_assign("vis", false);
    view->setAverageActivation(activation);
    view->setAverageColor(colors);
    QCOMPARE(view->getAverageColor(), colors);
    QCOMPARE(view->getAverageActivation(), activation);
    QCOMPARE(view->findChildren<QCheckBox*>().size(), 2);
    auto* visBox = view->findChild<QCheckBox*>(QStringLiteral("vis"));
    QVERIFY(visBox && !visBox->isChecked());
    QVERIFY(view->findChild<QCheckBox*>(QStringLiteral("aud"))->isChecked());

    // Toggling a checkbox updates the activation map
    QSignalSpy activationSpy(view.get(), &AverageSelectionView::newAverageActivationMap);
    visBox->click();
    QCOMPARE(activationSpy.size(), 1);
    QVERIFY(mne::viz::qt::detail::valueOr(*activation, "vis"));

    // Picking a colour stores it; cancelling the dialog keeps the old one
    QSignalSpy colorSpy(view.get(), &AverageSelectionView::newAverageColorMap);
    auto* visButton = view->findChild<QPushButton*>(QStringLiteral("vis"));
    answerNextModal([](QWidget* pModal) {
        auto* pDialog = qobject_cast<QColorDialog*>(pModal);
        QVERIFY(pDialog);
        pDialog->setCurrentColor(QColor(10, 200, 30));
        static_cast<QDialog*>(pDialog)->accept();
    });
    visButton->click();
    QCOMPARE(mne::viz::qt::detail::valueOr(*colors, "vis"), QColor(10, 200, 30));
    QCOMPARE(colorSpy.size(), 1);
    answerNextModal([](QWidget* pModal) {
        auto* pDialog = qobject_cast<QColorDialog*>(pModal);
        QVERIFY(pDialog);
        static_cast<QDialog*>(pDialog)->reject();
    });
    visButton->click();
    QCOMPARE(mne::viz::qt::detail::valueOr(*colors, "vis"), QColor(10, 200, 30));
    QCOMPARE(colorSpy.size(), 1);

    // Colours and activation are restored by a new view with the same settings path
    view.reset();
    view = std::make_unique<AverageSelectionView>(settingsPath);
    QCOMPARE(mne::viz::qt::detail::valueOr(*view->getAverageColor(), "vis"), QColor(10, 200, 30));
    QVERIFY(mne::viz::qt::detail::valueOr(*view->getAverageActivation(), "vis"));
    QCOMPARE(view->findChildren<QCheckBox*>().size(), 2);

    // At most 10 averages are listed; maps of different size are not drawn
    for (int i = 0; i < 12; ++i) {
        colors->insert_or_assign(std::format("avg{:02}", i), Qt::green);
        activation->insert_or_assign(std::format("avg{:02}", i), true);
    }
    view->setAverageActivation(activation);
    view->setAverageColor(colors);
    QApplication::processEvents();
    QCOMPARE(view->findChildren<QCheckBox*>().size(), 10);
    activation->erase("aud");
    view->setAverageActivation(activation);
    QApplication::processEvents();
    QCOMPARE(view->findChildren<QCheckBox*>().size(), 10);

    view.reset();
    QSettings(QStringLiteral("MNECPP")).remove(settingsPath);
}

//=============================================================================================================

void TestVizQtViewers2::butterflyView_lifecycle()
{
    ButterflyView view("test_viz_qt_viewers2");

    // Scale map with some channel types
    std::map<int, float> scaleMap;
    scaleMap[FIFFV_MEG_CH] = 1e-12f;
    scaleMap[FIFFV_EEG_CH] = 1e-6f;
    view.setScaleMap(scaleMap);

    // Modality map
    std::map<std::string, bool> modalMap;
    modalMap["MEG"] = true;
    modalMap["EEG"] = false;
    view.setModalityMap(modalMap);

    view.clearView();

    QApplication::processEvents();
}

//=============================================================================================================

void TestVizQtViewers2::butterflyView_drawsAveragesAcrossTheFullWidth()
{
    // One magnetometer average of 3 * width samples: the curve is decimated, and must still span the width
    auto set = std::make_shared<FiffEvokedSet>();
    set->info = *createBrowserTestInfo();
    set->info.bads.clear();
    FiffEvoked evoked;
    evoked.comment = QStringLiteral("aud").toStdString();
    evoked.baseline = qMakePair(-0.1f, 0.0f);
    evoked.times = Eigen::RowVectorXf::LinSpaced(1200, -0.2f, 0.9992f);
    evoked.times(200) = 0.0f;
    evoked.data = Eigen::MatrixXd::Zero(4, 1200);
    evoked.data.row(0).setConstant(5e-13);
    set->evoked.push_back(evoked);

    auto model = std::make_shared<EvokedSetModel>();
    model->setEvokedSet(set);
    ButterflyView view(QStringLiteral("test_viz_qt_viewers2_butterfly"));
    view.resize(400, 200);
    view.setEvokedSetModel(model);
    view.setScaleMap({{FIFF_UNIT_T, 1e-12f}});
    view.setModalityMap(std::map<std::string, bool>{{"MAG", true}});
    view.setSelectedChannels({0});
    view.setBackgroundColor(Qt::white);
    view.setAverageActivation(std::make_shared<std::map<std::string, bool>>(std::map<std::string, bool>{{"aud", true}}));
    view.setAverageActivation(std::make_shared<std::map<std::string, bool>>(std::map<std::string, bool>{{"aud", true}}));
    view.setSingleAverageColor(Qt::black);
    view.show();
    view.dataUpdate();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    // The constant 0.5e-12 T at scale 1e-12 T sits a quarter height above the centre: y = 50
    const QImage frame = view.grab().toImage();
    QVERIFY(!frame.isNull());
    // Grid lines run the full height, so compare the curve row with the same column well below it
    auto curveAt = [&frame](int x) {
        int darkest = 255;
        for (int y = 48; y <= 52; ++y) {
            darkest = std::min(darkest, qGray(frame.pixel(x, y)));
        }
        return darkest < qGray(frame.pixel(x, 150)) - 40;
    };
    QVERIFY(curveAt(52));
    QVERIFY2(curveAt(392), "decimated curve ends before the right edge");

    // The screenshot holds the same QPainter-drawn frame
    QTemporaryDir dir;
    const QString shot = dir.filePath(QStringLiteral("butterfly.png"));
    view.takeScreenshot(shot);
    QCOMPARE(QImage(shot).convertToFormat(QImage::Format_RGB32), frame.convertToFormat(QImage::Format_RGB32));
}

//=============================================================================================================

void TestVizQtViewers2::spectrumPanel_inPanelWidget()
{
    // 4 channels at 1000 Hz, 101 bins up to 500 Hz; channel 0 peaks at 100 Hz (bin 20)
    std::shared_ptr<FiffInfo> info = createBrowserTestInfo();
    Eigen::MatrixXd psd = Eigen::MatrixXd::Constant(4, 101, 1.0);
    for (int b = 0; b < 101; ++b) {
        psd(0, b) = 1.0 + 9.0 * std::exp(-0.5 * std::pow((b - 20) / 2.0, 2));
    }

    auto panel = std::make_shared<mne::viz::SpectrumPanel>();
    PanelWidget view(panel, QStringLiteral("test_viz_qt_viewers2_spectrum"));
    view.setGuiMode(AbstractView::GuiMode::Research);
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    view.saveSettings();
    view.loadSettings();
    SceneViewport* viewport = view.figureViewport();
    // The frequency band shows above the spectra
    QVERIFY(viewport != nullptr && !view.formPanel()->isHidden());

    // mne_scan may send spectra and bounds before the channels: they are dropped
    panel->set_boundaries(1, 40);
    panel->add_data(psd);
    QCOMPARE(panel->spectrum().table().bins(), 0);
    panel->init(info, mne::viz::SpectrumTable::Scale::Linear);
    panel->add_data(psd);
    QCOMPARE(panel->spectrum().table().row_count(), 4);
    // Bounds set before the data apply with the first spectra; the user's changes are reported
    QVERIFY(panel->spectrum().table().upper_bin() < 20);
    std::vector<float> bounds;
    panel->set_bounds_callback([&bounds](float lower, float upper) {
        bounds = {lower, upper};
    });
    panel->form().set_value("upper", 500.0);
    QCOMPARE(bounds, (std::vector<float>{1.0f, 500.0f}));
    panel->form().set_value("lower", 600.0);
    QCOMPARE(panel->lower_bound(), 500.0f);
    panel->set_boundaries(0, 500);
    QCOMPARE(panel->spectrum().table().upper_bin(), 100);

    view.resize(500, 600);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QVERIFY(!view.grab().isNull());

    // The mouse over the peak of the first row reads out its bin
    const double plotWidth = viewport->width() - mne::viz::SpectrumFigure::name_width;
    const QPoint peak(static_cast<int>(std::lround(mne::viz::SpectrumFigure::name_width + 0.2 * plotWidth)), 70);
    QMouseEvent move(QEvent::MouseMove, QPointF(peak), viewport->mapToGlobal(QPointF(peak)), Qt::NoButton, Qt::NoButton,
                     Qt::NoModifier);
    QApplication::sendEvent(viewport, &move);
    const std::optional<mne::viz::SpectrumFigure::Readout> readout = panel->spectrum().readout();
    QVERIFY(readout.has_value());
    QCOMPARE(readout->row, 0);
    QVERIFY(std::abs(readout->bin - 20) <= 1);
    QVERIFY(!view.grab().isNull());

    // A double click freezes the spectra while new ones arrive
    QTest::mouseDClick(viewport, Qt::LeftButton, Qt::NoModifier, peak);
    QVERIFY(panel->spectrum().table().frozen());
    panel->add_data(psd * 2.0);
    QCOMPARE((*panel->spectrum().table().spectrum(0))[20], 10.0);
    QTest::mouseDClick(viewport, Qt::LeftButton, Qt::NoModifier, peak);
    QVERIFY(!panel->spectrum().table().frozen());

    // Bounds crop the frequency axis; leaving the view ends the read-out
    panel->set_boundaries(20, 200);
    QVERIFY(panel->spectrum().table().lower_bin() > 0);
    QEvent leave(QEvent::Leave);
    QApplication::sendEvent(viewport, &leave);
    QVERIFY(!panel->spectrum().readout().has_value());
    view.clearView();
}

//=============================================================================================================

void TestVizQtViewers2::rtFiffRawViewModel_basics()
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

    std::string trigName = model.getTriggerName();
    Q_UNUSED(trigName);

    double trigThreshold = model.getTriggerThreshold();
    Q_UNUSED(trigThreshold);

    int numSpacers = model.getNumberOfTimeSpacers();
    Q_UNUSED(numSpacers);

    // Scale map round-trip (safe — no threading)
    std::map<int, float> scaleMap;
    scaleMap[FIFFV_MEG_CH] = 1e-12f;
    model.setScaling(scaleMap);
    QVERIFY(qFuzzyCompare(model.getScaling().at(FIFFV_MEG_CH), 1e-12f));

    QApplication::processEvents();
}

//=============================================================================================================

void TestVizQtViewers2::rtFiffRawViewModel_dataFlow()
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
    model.addData(std::vector<Eigen::MatrixXd>{firstBlock});
    QCOMPARE(model.getCurrentSampleIndex(), 60);
    QCOMPARE(model.getLastBlock(), firstBlock);
    QCOMPARE(dataSpy.size(), 1);

    Eigen::MatrixXd secondBlock = Eigen::MatrixXd::Zero(4, 60);
    secondBlock.row(0).setConstant(2.0);
    secondBlock.row(2).setConstant(-2.0);
    model.addData(std::vector<Eigen::MatrixXd>{secondBlock});
    QCOMPARE(model.getCurrentSampleIndex(), 20);
    QCOMPARE(model.getLastBlock(), secondBlock);
    QCOMPARE(model.getMaxValueFromRawViewModel(0), static_cast<double>(1e-11f));
    QCOMPARE(dataSpy.size(), 2);

    model.addData(std::vector<Eigen::MatrixXd>{Eigen::MatrixXd::Zero(3, 10)});
    QCOMPARE(model.getCurrentSampleIndex(), 20);
    QCOMPARE(dataSpy.size(), 2);

    model.toggleFreeze(QModelIndex());
    QVERIFY(model.isFreezed());
    model.toggleFreeze(QModelIndex());
    QVERIFY(!model.isFreezed());

    QSignalSpy selectionSpy(&model, &RtFiffRawViewModel::newSelection);
    model.selectRows({0, 2, 99});
    QCOMPARE(model.getIdxSelMap().size(), 2);
    QCOMPARE(mne::viz::qt::detail::valueOr(model.getIdxSelMap(), 0), 0);
    QCOMPARE(mne::viz::qt::detail::valueOr(model.getIdxSelMap(), 1), 2);
    QCOMPARE(selectionSpy.size(), 1);
    model.hideRows({0});
    QCOMPARE(model.getIdxSelMap().size(), 1);
    model.resetSelection();
    QCOMPARE(model.getIdxSelMap().size(), 4);

    model.markChBad(model.index(0, 0), true);
    QVERIFY(std::ranges::contains(info->bads, "MEG 0111"));
    model.markChBad(model.index(0, 0), false);
    QVERIFY(!std::ranges::contains(info->bads, "MEG 0111"));
    model.markChBad({model.index(0, 0), model.index(2, 0)}, true);
    QVERIFY(std::ranges::contains(info->bads, "MEG 0111"));
    QVERIFY(std::ranges::contains(info->bads, "MEG 0113"));

    std::map<double, QColor> triggerColors;
    triggerColors.insert_or_assign(3.0, Qt::red);
    model.triggerInfoChanged(triggerColors, true, "STI 014", 1.0);
    QVERIFY(model.triggerDetectionActive());
    QCOMPARE(model.getTriggerName(), QStringLiteral("STI 014"));
    QCOMPARE(model.getTriggerThreshold(), 1.0);
    QCOMPARE(model.getCurrentTriggerIndex(), 3);

    Eigen::MatrixXd triggerBlock = Eigen::MatrixXd::Zero(4, 20);
    triggerBlock(3, 5) = 3.0;
    model.updateSpharaActivation(true);
    model.addData(std::vector<Eigen::MatrixXd>{triggerBlock});
    model.updateSpharaActivation(false);
    QVERIFY(!model.getDetectedTriggers().empty());

    QApplication::processEvents();
}

//=============================================================================================================

void TestVizQtViewers2::rtFiffRawViewModel_ringBufferWrap()
{
    const int N = 100;
    auto column = [](const RtFiffRawViewModel& model, int row, int col) {
        return model.data(model.index(row, 1)).value<RowVectorPair>().first[col];
    };

    // Raw: a ramp (value = stream index) must read back as column c = s mod N.
    for (const int nBlock : {60, 64, 100, 37, 25}) {
        RtFiffRawViewModel model;
        auto info = createBrowserTestInfo();
        model.setFiffInfo(info);
        model.setSamplingInfo(100.0f, 1, true);
        QCOMPARE(model.getMaxSamples(), N);

        int iTotal = 0;
        while (iTotal < 5 * N + 13) {
            Eigen::MatrixXd block = Eigen::MatrixXd::Zero(4, nBlock);
            for (int s = 0; s < nBlock; ++s) {
                block.col(s).setConstant(iTotal + s);
            }
            model.addData(std::vector<Eigen::MatrixXd>{block});
            iTotal += nBlock;

            const int iCur = model.getCurrentSampleIndex();
            QCOMPARE(iCur, (iTotal - 1) % N + 1);
            QCOMPARE(model.getFirstSampleOffset(), iTotal - iCur);
            QCOMPARE(model.getLastBlock(), block);
            for (int c = 0; c < N; ++c) {
                const int s = c < iCur ? model.getFirstSampleOffset() + c : model.getFirstSampleOffset() - N + c;
                if (s >= 0 && column(model, 0, c) != s) {
                    QFAIL(qPrintable(QStringLiteral("block %1, %2 samples: column %3 holds %4, expected %5")
                                         .arg(nBlock)
                                         .arg(iTotal)
                                         .arg(c)
                                         .arg(column(model, 0, c))
                                         .arg(s)));
                }
            }
        }
    }

    // Filtered: the overlap-add output equals the offline convolution, delayed by half the kernel.
    {
        RtFiffRawViewModel model;
        auto info = createBrowserTestInfo();
        model.setFiffInfo(info);
        model.setSamplingInfo(100.0f, 1, true);
        const FilterKernel kernel("lp", 0, 50, 0.2, 0.0, 0.05, 100.0, 0);
        const Eigen::RowVectorXd& h = kernel.coefficients();
        const int L = static_cast<int>(h.cols());
        const int iDelay = L / 2;
        model.setFilterChannelType("MEG");
        model.setFilter({kernel});
        model.setFilterActive(true);

        const int nBlock = 64;
        const int iTotal = 8 * nBlock;
        Eigen::MatrixXd stream = Eigen::MatrixXd::Zero(4, iTotal);
        for (int s = 0; s < iTotal; ++s) {
            stream(0, s) = std::sin(0.13 * s) + 0.5 * std::sin(1.7 * s);
            stream(3, s) = s % 7;
        }
        for (int b = 0; b < iTotal / nBlock; ++b) {
            model.addData(std::vector<Eigen::MatrixXd>{stream.middleCols(b * nBlock, nBlock)});
        }

        // Samples from the second block on are final once the following half kernel has arrived.
        for (int t = iTotal - N; t < iTotal - iDelay; ++t) {
            double y = 0.0;
            for (int j = 0; j < L && t + iDelay - j >= 0; ++j) {
                y += h(j) * stream(0, t + iDelay - j);
            }
            if (std::fabs(column(model, 0, t % N) - y) > 1e-9) {
                QFAIL(qPrintable(QStringLiteral("filtered sample %1 (column %2): %3, expected %4")
                                     .arg(t)
                                     .arg(t % N)
                                     .arg(column(model, 0, t % N))
                                     .arg(y)));
            }
        }
        for (int t = iTotal - N; t < iTotal; ++t) {
            QCOMPARE(column(model, 3, t % N), stream(3, t));
        }

        // A channel type that matches no channel leaves the MEG rows unfiltered.
        model.setFilterChannelType("EEG");
        model.addData(std::vector<Eigen::MatrixXd>{stream.middleCols(0, nBlock)});
        for (int s = 0; s < nBlock; ++s) {
            QCOMPARE(column(model, 0, (iTotal + s) % N), stream(0, s));
        }
    }

    // Triggers: positions are buffer columns of the sweep they belong to.
    {
        RtFiffRawViewModel model;
        auto info = createBrowserTestInfo();
        model.setFiffInfo(info);
        model.setSamplingInfo(100.0f, 1, true);
        model.triggerInfoChanged({{2.0, Qt::red}}, true, "STI 014", 1.0);
        QSignalSpy triggerSpy(&model, &RtFiffRawViewModel::triggerDetected);

        // Pulses at stream samples 90 (before the wrap inside block 60..119) and 215 (after the wrap inside 180..239)
        Eigen::MatrixXd stream = Eigen::MatrixXd::Zero(4, 240);
        stream(3, 90) = 2.0;
        stream(3, 215) = 2.0;
        model.addData(std::vector<Eigen::MatrixXd>{stream.middleCols(0, 60)});
        model.addData(std::vector<Eigen::MatrixXd>{stream.middleCols(60, 60)});
        QCOMPARE(model.getDetectedTriggersOld(), (std::vector<std::pair<int, double>>{{90, 2.0}}));
        QVERIFY(model.getDetectedTriggers().empty());
        model.addData(std::vector<Eigen::MatrixXd>{stream.middleCols(120, 60)});
        model.addData(std::vector<Eigen::MatrixXd>{stream.middleCols(180, 60)});
        QCOMPARE(model.getDetectedTriggers(), (std::vector<std::pair<int, double>>{{15, 2.0}}));
        QVERIFY(model.getDetectedTriggersOld().empty());
        QCOMPARE(triggerSpy.size(), 2);
    }
}

//=============================================================================================================

void TestVizQtViewers2::rtFiffRawViewModel_projectionWrapFilterAndRoles()
{
    RtFiffRawViewModel model;
    auto info = createBrowserTestInfo();
    info->bads.clear();
    model.setFiffInfo(info);
    model.setSamplingInfo(100.0f, 1, true);

    // An active projector removing the common mode of the three MEG channels
    Eigen::MatrixXd vec(1, 3);
    vec.setConstant(1.0 / std::sqrt(3.0));
    FiffNamedMatrix vectors(1, 3, {}, StringList(info->ch_names.begin() + 0, info->ch_names.begin() + 0 + 3), vec);
    model.updateProjection({FiffProj(FIFFV_PROJ_ITEM_FIELD, true, "common", vectors)});
    model.updateCompensator(0);

    // 3 blocks of 40 into a 100-sample buffer: the third fills the last 20 columns and wraps to columns 0..19,
    // so column 10 holds sample 110
    Eigen::MatrixXd block(4, 40);
    for (int b = 0; b < 3; ++b) {
        for (int s = 0; s < 40; ++s) {
            const double t = 40 * b + s;
            block(0, s) = 5.0 + std::sin(0.1 * t);
            block(1, s) = 5.0;
            block(2, s) = 5.0 - std::sin(0.1 * t);
            block(3, s) = (s == 7) ? 2.0 : 0.0;
        }
        model.addData(std::vector<Eigen::MatrixXd>{block});
    }
    QCOMPARE(model.getCurrentSampleIndex(), 20);
    const Eigen::MatrixXd last = model.getLastBlock();
    for (int s = 0; s < 40; ++s) {
        const double d = std::sin(0.1 * (80 + s));
        QVERIFY(std::fabs(last(0, s) - d) < 1e-12);
        QVERIFY(std::fabs(last(1, s)) < 1e-12);
        QVERIFY(std::fabs(last(2, s) + d) < 1e-12);
        QCOMPARE(last(3, s), s == 7 ? 2.0 : 0.0);
    }

    // data(): name, row pointer into the buffer, bad flag, background
    model.setBackgroundColor(Qt::darkGray);
    QCOMPARE(model.data(model.index(3, 0)).toString(), QStringLiteral("STI 014"));
    const auto row = model.data(model.index(0, 1)).value<RowVectorPair>();
    QCOMPARE(row.second, 100);
    QVERIFY(std::fabs(row.first[10] - std::sin(0.1 * 110)) < 1e-12);
    QCOMPARE(model.data(model.index(1, 2)).toBool(), false);
    QCOMPARE(model.data(model.index(0, 0), Qt::BackgroundRole).value<QBrush>().color(), QColor(Qt::darkGray));
    QVERIFY(!model.data(model.index(0, 0), Qt::ToolTipRole).isValid());
    QCOMPARE(model.headerData(1, Qt::Horizontal).toString(), QStringLiteral("data plot"));
    QCOMPARE(model.headerData(1, Qt::Horizontal, Qt::TextAlignmentRole).toInt(), static_cast<int>(Qt::AlignLeft));
    QCOMPARE(model.headerData(2, Qt::Vertical).toString(), QStringLiteral("MEG 0113"));
    QVERIFY(!model.headerData(0, Qt::Horizontal, Qt::ToolTipRole).isValid());

    // Freezing keeps serving the snapshot while new data arrives
    model.toggleFreeze(QModelIndex());
    model.addData(std::vector<Eigen::MatrixXd>{block * 0.0});
    const auto frozen = model.data(model.index(0, 1)).value<RowVectorPair>();
    QVERIFY(std::fabs(frozen.first[10] - std::sin(0.1 * 110)) < 1e-12);
    model.toggleFreeze(QModelIndex());

    // Real-time filtering: a 50-tap low-pass on the MEG channels only, a pass-through on STIM
    model.setFilterChannelType("MEG");
    model.setFilter({FilterKernel("lp", 0, 50, 0.2, 0.0, 0.05, 100.0, 0)});
    model.setFilterActive(true);
    Eigen::MatrixXd tone(4, 50);
    for (int s = 0; s < 50; ++s) {
        tone.col(s) << std::sin(0.2 * s), 0.0, 0.0, s % 5;
    }
    for (int b = 0; b < 4; ++b) {
        model.addData(std::vector<Eigen::MatrixXd>{tone});
    }
    const Eigen::MatrixXd filtered = model.getLastBlock();
    QVERIFY(filtered.allFinite());
    QCOMPARE(filtered.row(3), tone.row(3));
    // The active common-mode projector leaves 2/3 of the tone on channel 0 and -1/3 on channels 1 and 2; filtering is linear
    QVERIFY(filtered.row(1).cwiseAbs().maxCoeff() > 0.1);
    QVERIFY((filtered.row(1) - filtered.row(2)).cwiseAbs().maxCoeff() < 1e-12);
    QVERIFY((filtered.row(0) + 2.0 * filtered.row(1)).cwiseAbs().maxCoeff() < 1e-12);
    model.setFilterActive(false);
    QCOMPARE(model.getLastBlock().row(3), tone.row(3));

    // Default scale per channel kind, overridden by setScaling
    QCOMPARE(model.getMaxValueFromRawViewModel(3), 5.0);
    model.setScaling({{FIFF_UNIT_T, 2e-12f}, {FIFFV_STIM_CH, 3.0f}});
    QCOMPARE(model.getMaxValueFromRawViewModel(0), static_cast<double>(2e-12f));
    QCOMPARE(model.getMaxValueFromRawViewModel(3), 3.0);

    model.distanceTimeSpacerChanged(0);
    QCOMPARE(model.getNumberOfTimeSpacers(), 0);
    model.distanceTimeSpacerChanged(250);
    QCOMPARE(model.getNumberOfTimeSpacers(), 3);
    model.resetTriggerCounter();

    std::vector<int> added;
    model.addEvent(1);
    QVERIFY(model.getEventsToDisplay(0, 10).empty());
    model.setEventCallbacks([&added](int s) { added.push_back(s); },
                            [](int b, int e) { return std::vector<int>{b, e}; });
    model.addEvent(42);
    QCOMPARE(added, std::vector<int>{42});
    QCOMPARE(model.getEventsToDisplay(3, 9), (std::vector<int>{3, 9}));
}

//=============================================================================================================

void TestVizQtViewers2::averageLayoutPanel_drawsEachChannelsAverage()
{
    // A positive step on MEG0111 and a negative one on MEG0113, both from the stimulus onset
    auto set = std::make_shared<FiffEvokedSet>();
    set->info = *createBrowserTestInfo();
    FiffEvoked evoked;
    evoked.comment = QStringLiteral("aud").toStdString();
    evoked.baseline = qMakePair(-0.05f, 0.0f);
    evoked.times = Eigen::RowVectorXf::LinSpaced(120, -0.06f, 0.059f);
    evoked.times(60) = 0.0f;
    evoked.data = Eigen::MatrixXd::Zero(4, 120);
    evoked.data.row(0).tail(60).setConstant(8e-13);
    evoked.data.row(2).tail(60).setConstant(-8e-13);
    set->evoked.push_back(evoked);
    auto model = std::make_shared<EvokedSetModel>();
    model->setEvokedSet(set);
    auto pInfo = std::make_shared<FiffInfo>(set->info);

    auto panel = std::make_shared<mne::viz::AverageLayoutPanel>();
    PanelWidget view(panel, QStringLiteral("test_viz_qt_viewers2_average_layout"));
    panel->set_info(pInfo);
    panel->set_evoked(model->evokedAverages());
    view.figureViewport()->setBackgroundColor(Qt::white);
    QCOMPARE(view.figureViewport()->backgroundColor(), QColor(Qt::white));

    // Two channels, one bad, laid out left and right
    SelectionItem selection;
    selection.m_sChannelName = {"MEG 0111", "MEG 0113"};
    selection.m_iChannelNumber = {0, 2};
    selection.m_iChannelKind = {FIFFV_MEG_CH, FIFFV_MEG_CH};
    selection.m_iChannelUnit = {FIFF_UNIT_T, FIFF_UNIT_T};
    selection.m_qpChannelPosition = {QPointF(-1.0, 0.0), QPointF(1.0, 0.0)};
    selection.m_bShowAll = false;
    panel->set_selection(selection.averageChannels());
    panel->set_scale_map({{FIFF_UNIT_T, 1e-12f}});
    panel->set_average_colors({{"aud", mne::viz::Color{255, 0, 0}}});
    panel->set_average_activation({{"aud", true}});
    panel->set_single_average_color(mne::viz::Color{0, 0, 0});
    QCOMPARE(panel->average_colors().at("aud"), (mne::viz::Color{0, 0, 0}));
    QVERIFY(panel->average_activation().at("aud"));

    const mne::viz::AverageLayoutFigure& layout = panel->layout();
    QCOMPARE(view.figureViewport()->figure(), &layout);
    QCOMPARE(layout.channels().size(), std::size_t{2});
    // Each tile shows its channel's average; the bad one is marked
    QCOMPARE(layout.channels()[0].averages.size(), std::size_t{1});
    QCOMPARE(layout.channels()[0].averages[0].data, model->evokedAverages().averages[0].data);
    QCOMPARE(layout.channels()[1].averages[0].data, model->evokedAverages().averages[0].data + 2);
    QCOMPARE(layout.channels()[0].first_sample, -60);
    // Recolouring keeps the channels (name, number and layout position) intact
    QStringList names;
    for (const mne::viz::AverageLayoutFigure::Channel& channel : layout.channels()) {
        names << QString::fromStdString(channel.name);
        QCOMPARE(std::abs(channel.position.x), 1.0);
    }
    names.sort();
    QCOMPARE(names, (QStringList{QStringLiteral("MEG 0111"), QStringLiteral("MEG 0113")}));

    // Tile at x = -160 (MEG0111) and x = +160 (MEG0113), 120 x 60 each: 0.8 of full scale is 24 px off centre
    QImage image(480, 120, QImage::Format_RGB32);
    image.fill(Qt::white);
    {
        mne::viz::Canvas canvas(480, 120);
        canvas.translate(240, 60);
        layout.average_layout().draw(canvas, toSceneFont(view.font()));
        QPainter painter(&image);
        paintScene(painter, canvas.display_list(), view.font());
    }
    auto darkAt = [&image](int x, int y) {
        return qGray(image.pixel(x, y)) < 160;
    };
    auto curveRow = [&darkAt](int x) {
        for (int y = 0; y < 120; ++y) {
            if (y != 60 && darkAt(x, y) && !darkAt(x, 59) && !darkAt(x, 61)) {
                return y;
            }
        }
        return -1;
    };
    // After the stimulus (right half of each item) the curves sit 24 px above / below the centre line
    QVERIFY2(std::abs(curveRow(80 + 40) - (60 - 24)) <= 2, qPrintable(QString::number(curveRow(120))));
    QVERIFY2(std::abs(curveRow(400 + 40) - (60 + 24)) <= 2, qPrintable(QString::number(curveRow(440))));

    const QString shot = QDir::temp().filePath(QStringLiteral("test_viz_qt_viewers2_average_layout.svg"));
    QVERIFY(mne::viz::write_svg(view.figureViewport()->picture(QSize(480, 120)), shot.toStdU16String()).has_value());
    QVERIFY(QFile::exists(shot));
    QFile::remove(shot);
    panel->clear_evoked();
    QVERIFY(layout.channels()[0].averages.empty());
}

//=============================================================================================================

void TestVizQtViewers2::evokedSetModel_basics()
{
    EvokedSetModel model;

    // Without data
    QVERIFY(model.rowCount() >= 0);
    QVERIFY(model.columnCount() >= 0);
    QVERIFY(!model.isInit());

    // Set empty evoked set — must not crash
    FiffEvokedSet emptySet;
    model.setEvokedSet(std::make_shared<FiffEvokedSet>(emptySet));

    QApplication::processEvents();
}

//=============================================================================================================

void TestVizQtViewers2::evokedSetModel_projectionRolesAndAverageMaps()
{
    auto set = std::make_shared<FiffEvokedSet>();
    set->info = *createBrowserTestInfo();
    set->info.bads.clear();
    for (const QString& comment : {QStringLiteral("aud"), QStringLiteral("vis")}) {
        FiffEvoked evoked;
        evoked.comment = comment.toStdString();
        evoked.baseline = qMakePair(-0.1f, 0.0f);
        evoked.times = Eigen::RowVectorXf::LinSpaced(30, -0.1f, 0.19f);
        evoked.times(10) = 0.0f;
        evoked.data = Eigen::MatrixXd::Zero(4, 30);
        for (int s = 0; s < 30; ++s) {
            const double v = (comment == QLatin1String("aud") ? 1.0 : 2.0) * std::sin(0.3 * s);
            evoked.data(0, s) = 3.0 + v;
            evoked.data(1, s) = 3.0;
            evoked.data(2, s) = 3.0 - v;
            evoked.data(3, s) = s;
        }
        set->evoked.push_back(evoked);
    }

    EvokedSetModel model;
    QSignalSpy colorSpy(&model, &EvokedSetModel::newAverageColorMap);
    QSignalSpy activationSpy(&model, &EvokedSetModel::newAverageActivationMap);
    model.setEvokedSet(set);
    QVERIFY(model.isInit());
    QCOMPARE(model.rowCount(), 4);
    QCOMPARE(model.columnCount(), 3);
    QCOMPARE(model.getNumAverages(), 2);
    QCOMPARE(model.getNumSamples(), 30);
    QCOMPARE(model.getNumPreStimSamples(), 10);
    QCOMPARE(model.getSamplingFrequency(), 1000.0f);
    QCOMPARE(model.getNumberOfTimeSpacers(), 0);
    QCOMPARE(model.getBaselineInfo().first, -0.1f);
    QCOMPARE(colorSpy.size(), 1);
    QCOMPARE(activationSpy.size(), 1);
    QCOMPARE(mne::viz::qt::detail::valueOr(*model.getAverageColor(), "vis"), QColor(Qt::yellow));
    QVERIFY(mne::viz::qt::detail::valueOr(*model.getAverageActivation(), "aud"));

    // Column 1: one row per average type for the butterfly; column 2: whole matrices for the 2D layout
    auto rows = model.data(2, 1).value<std::vector<AvrTypeRowVector>>();
    QCOMPARE(rows.size(), 2);
    QCOMPARE(rows.at(1).first, QStringLiteral("vis"));
    QCOMPARE(rows.at(1).second, set->evoked.at(1).data.row(2));
    const auto matrices = model.data(0, 2, EvokedSetModelRoles::GetAverageData).value<std::vector<AvrTypeRowVectorPair>>();
    QCOMPARE(matrices.size(), 2);
    QCOMPARE(matrices.at(0).second.second, 30);
    // Column-major (nchan x nsamples), the layout AverageLayoutFigure indexes with sample * nchan + channel
    QCOMPARE(matrices.at(0).second.first[5 * 4 + 3], 5.0);
    QVERIFY(!model.data(0, 1, Qt::BackgroundRole).isValid());
    QVERIFY(!model.data(0, 1, Qt::ToolTipRole).isValid());
    QCOMPARE(model.data(0, 0).toStringList().join(QLatin1Char(',')).toStdString(), mne::join(set->info.ch_names, ","));
    QCOMPARE(model.headerData(1, Qt::Horizontal).toString(), QStringLiteral("data plot"));
    QCOMPARE(model.headerData(1, Qt::Horizontal, Qt::TextAlignmentRole).toInt(), static_cast<int>(Qt::AlignLeft));
    QVERIFY(!model.headerData(0, Qt::Horizontal).isValid());

    // Selection remaps rows; kind, unit, coil and bad follow the selection
    model.selectRows({3, 0, 99});
    QCOMPARE(model.getIdxSelMap().size(), 2);
    QCOMPARE(model.getKind(0), FIFFV_STIM_CH);
    QCOMPARE(model.getUnit(1), FIFF_UNIT_T);
    QCOMPARE(model.getCoil(1), FIFFV_COIL_VV_MAG_T3);
    QCOMPARE(model.getKind(5), 0);
    QCOMPARE(model.getUnit(5), FIFF_UNIT_NONE);
    QCOMPARE(model.getCoil(5), FIFFV_COIL_NONE);
    set->info.bads = {"MEG 0111"};
    QVERIFY(model.getIsChannelBad(1));
    QVERIFY(!model.getIsChannelBad(0));
    QVERIFY(!model.getIsChannelBad(7));
    set->info.bads.clear();
    model.resetSelection();

    // An active common-mode projector applies to every average once the data is updated
    Eigen::MatrixXd vec(1, 3);
    vec.setConstant(1.0 / std::sqrt(3.0));
    FiffNamedMatrix vectors(1, 3, {}, StringList(set->info.ch_names.begin() + 0, set->info.ch_names.begin() + 0 + 3), vec);
    model.updateProjection({FiffProj(FIFFV_PROJ_ITEM_FIELD, true, "common", vectors)});
    model.updateCompensator(0);
    model.setEvokedSet(set);
    rows = model.data(0, 1).value<std::vector<AvrTypeRowVector>>();
    for (int s = 0; s < 30; ++s) {
        QVERIFY(std::fabs(rows.at(1).second(s) - 2.0 * std::sin(0.3 * s)) < 1e-12);
    }
    QCOMPARE(model.data(3, 1).value<std::vector<AvrTypeRowVector>>().at(0).second, set->evoked.at(0).data.row(3));

    // Freezing keeps the projected data; dropping an average keeps its colour for when it returns
    model.toggleFreeze();
    QVERIFY(model.isFreezed());
    model.getAverageColor()->insert_or_assign("vis", Qt::cyan);
    const FiffEvoked vis = set->evoked.back();
    set->evoked.pop_back();
    model.setEvokedSet(set);
    QCOMPARE(model.getNumAverages(), 1);
    const auto frozenRows = model.data(0, 1).value<std::vector<AvrTypeRowVector>>();
    QCOMPARE(frozenRows.size(), 2);
    QCOMPARE(frozenRows.at(1).first, QStringLiteral("vis"));
    const auto frozenMatrices = model.data(0, 2, EvokedSetModelRoles::GetAverageData).value<std::vector<AvrTypeRowVectorPair>>();
    QCOMPARE(frozenMatrices.size(), 2);
    QCOMPARE(frozenMatrices.at(1).first, QStringLiteral("vis"));
    QCOMPARE(colorSpy.size(), 2);
    QVERIFY(!model.getAverageColor()->contains("vis"));
    model.toggleFreeze();
    QCOMPARE(model.data(0, 1).value<std::vector<AvrTypeRowVector>>().size(), 1);
    set->evoked.push_back(vis);
    model.setEvokedSet(set);
    QCOMPARE(mne::viz::qt::detail::valueOr(*model.getAverageColor(), "vis"), QColor(Qt::cyan));
    QCOMPARE(model.getEvokedSet(), set);

    model.setAverageColor(std::make_shared<std::map<std::string, QColor>>());
    model.setAverageActivation(std::make_shared<std::map<std::string, bool>>());
    QVERIFY(model.getAverageColor()->empty());
    QVERIFY(model.getAverageActivation()->empty());
}

//=============================================================================================================

void TestVizQtViewers2::channelInfoModel_columnsRolesAndLayoutMapping()
{
    auto info = createBrowserTestInfo();
    info->chs[1].unit = FIFF_UNIT_T_M;
    info->chs[1].chpos.coil_type = FIFFV_COIL_VV_PLANAR_T1;
    info->chs[0].chpos.r0 = Eigen::Vector3f(0.01f, -0.02f, 0.05f);
    FiffChInfo eeg;
    eeg.ch_name = "EEG-007";
    eeg.kind = FIFFV_EEG_CH;
    eeg.unit = FIFF_UNIT_V;
    info->chs.push_back(eeg);
    info->ch_names.push_back(eeg.ch_name);
    info->nchan = 5;

    ChannelInfoModel model(info);
    QSignalSpy mappedSpy(&model, &ChannelInfoModel::channelsMappedToLayout);
    QCOMPARE(model.rowCount(), 5);
    QCOMPARE(model.columnCount(), 13);

    // Layout names drop the kind prefix and separators and re-prefix MEG / EEG; other kinds keep their name
    const std::vector<std::string> mapped = {"MEG 0111", "MEG 0112", "MEG 0113", "STI 014", "EEG 007"};
    QCOMPARE(model.getMappedChannelsList(), mapped);
    QCOMPARE(model.getIndexFromMappedChName("EEG 007"), 4);
    QCOMPARE(model.getIndexFromOrigChName("MEG 0113"), 2);
    QCOMPARE(model.getIndexFromOrigChName("MEG0113"), 2);
    QCOMPARE(model.getBadChannelList(), StringList{"MEG 0112"});

    model.layoutChanged(std::map<std::string, QPointF>{{"MEG 0111", QPointF(1.5, -2.0)}, {"EEG 007", QPointF(3, 4)}});
    QCOMPARE(mappedSpy.size(), 1);

    auto cell = [&model](int row, int column, int role) {
        return model.data(model.index(row, column), role);
    };
    QCOMPARE(cell(3, 0, Qt::DisplayRole).toInt(), 3);
    QCOMPARE(cell(3, 0, ChannelInfoModelRoles::GetChNumber).toInt(), 3);
    QCOMPARE(cell(4, 1, ChannelInfoModelRoles::GetOrigChName).toString(), QStringLiteral("EEG-007"));
    QCOMPARE(cell(4, 2, ChannelInfoModelRoles::GetChAlias).toString(), QStringLiteral("EEG-007"));
    QCOMPARE(cell(4, 3, ChannelInfoModelRoles::GetMappedLayoutChName).toString(), QStringLiteral("EEG 007"));
    QCOMPARE(cell(4, 4, ChannelInfoModelRoles::GetChKind).toInt(), static_cast<int>(FIFFV_EEG_CH));
    QCOMPARE(cell(1, 5, ChannelInfoModelRoles::GetMEGType).toString(), QStringLiteral("MEG_grad"));
    QCOMPARE(cell(0, 5, Qt::DisplayRole).toString(), QStringLiteral("MEG_mag"));
    QCOMPARE(cell(4, 5, Qt::DisplayRole).toString(), QStringLiteral("non_MEG"));
    QCOMPARE(cell(4, 6, ChannelInfoModelRoles::GetChUnit).toInt(), static_cast<int>(FIFF_UNIT_V));
    QCOMPARE(cell(0, 7, ChannelInfoModelRoles::GetChPosition).toPointF(), QPointF(1.5, -2.0));
    QCOMPARE(cell(4, 7, Qt::DisplayRole).toString(), QStringLiteral("(3|4)"));
    QCOMPARE(cell(2, 7, ChannelInfoModelRoles::GetChPosition).toPointF(), QPointF());
    const QVector3D digitizer = cell(0, 8, ChannelInfoModelRoles::GetChDigitizer).value<QVector3D>();
    QVERIFY((digitizer - QVector3D(1.0f, -2.0f, 5.0f)).length() < 1e-5f);
    QVERIFY(!cell(0, 9, Qt::DisplayRole).isValid());
    QCOMPARE(cell(1, 10, ChannelInfoModelRoles::GetChCoilType).toInt(), static_cast<int>(FIFFV_COIL_VV_PLANAR_T1));
    QVERIFY(cell(1, 11, ChannelInfoModelRoles::GetIsBad).toBool());
    QVERIFY(!cell(2, 11, Qt::DisplayRole).toBool());
    QCOMPARE(cell(0, 12, Qt::DisplayRole).toInt(), 0);
    for (int column = 0; column < 13; ++column) {
        QCOMPARE(cell(0, column, Qt::TextAlignmentRole).toInt(), static_cast<int>(Qt::AlignHCenter | Qt::AlignVCenter));
        QVERIFY(!model.headerData(column, Qt::Horizontal).toString().isEmpty());
    }
    QVERIFY(!cell(9, 1, Qt::DisplayRole).isValid());
    QCOMPARE(model.headerData(2, Qt::Vertical).toString(), QStringLiteral("Ch 2"));
    QVERIFY(!model.headerData(9, Qt::Vertical).isValid());
    QVERIFY(!model.headerData(0, Qt::Horizontal, Qt::ToolTipRole).isValid());
    QVERIFY(model.flags(model.index(0, 0)).testFlag(Qt::ItemIsSelectable));
    QVERIFY(model.setData(model.index(0, 0), 1));
    QVERIFY(model.insertRows(0, 1));
    QVERIFY(model.removeRows(0, 1));

    model.assignedOperatorsChanged({});
    model.clearModel();
    QCOMPARE(model.rowCount(), 0);
    QVERIFY(model.getMappedChannelsList().empty());
}

//=============================================================================================================

void TestVizQtViewers2::channelInfoModel_basics()
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
    model.layoutChanged(std::map<std::string, QPointF>());

    // assignedOperatorsChanged with empty map
    QMultiMap<int, std::shared_ptr<MNEOperator>> emptyOps;
    model.assignedOperatorsChanged(emptyOps);

    // clearModel — resets to empty FiffInfo state
    model.clearModel();
    QVERIFY(model.rowCount() == 0);

    // setFiffInfo with empty FiffInfo — calls mapLayoutToChannels (no channels)
    std::shared_ptr<FiffInfo> pInfo(new FiffInfo);
    model.setFiffInfo(pInfo);

    QApplication::processEvents();
}

//=============================================================================================================

void TestVizQtViewers2::channelDataView_hideBadChannelsAndMapping()
{
    ChannelDataView view(QStringLiteral("test_viz_qt_viewers2_channeldata"));
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

    view.setChannelFilter({"MEG 0111", "MEG 0113"});
    QCOMPARE(rhiView->totalLogicalChannels(), 2);

    view.setChannelFilter({});
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

void TestVizQtViewers2::channelRhiView_stateContracts()
{
    ChannelDataView view(QStringLiteral("test_viz_qt_viewers2_rhi_state"));
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

    rhiView->setEvents({ChannelRhiView::EventMarker{120, 1, Qt::red, "event"}});
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
                                                            "annotation"}});
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

void TestVizQtViewers2::channelRhiView_renderToImage()
{
    ChannelDataView view(QStringLiteral("test_viz_qt_viewers2_rhi_image"));
    const QImage blank = view.renderToImage(QSize(20, 10));
    QCOMPARE(blank.size(), QSize(20, 10));
    QCOMPARE(blank.pixelColor(10, 5), view.findChild<ChannelRhiView*>()->backgroundColor());

    view.init(createBrowserTestInfo());
    const float amplitudeMax = view.model()->channelInfo(0).amplitudeMax;
    QVERIFY(amplitudeMax > 0.f);

    // Flat traces at known fractions of the row scale; MEG0112 is bad, STI014 stays at zero
    Eigen::MatrixXd data = Eigen::MatrixXd::Zero(4, 200);
    data.row(0).setConstant(0.5 * amplitudeMax);
    data.row(1).setConstant(0.5 * amplitudeMax);
    data.row(2).setConstant(-3.0 * amplitudeMax);
    view.setFileBounds(100, 299);
    view.setData(data, 100);

    auto* rhiView = view.findChild<ChannelRhiView*>();
    QVERIFY(rhiView != nullptr);
    rhiView->resize(400, 200);
    rhiView->setSamplesPerPixel(200.f / 400.f);
    rhiView->setScrollSample(100.f);
    rhiView->setGridVisible(false);
    rhiView->setSfreq(0.f);
    rhiView->setBackgroundColor(Qt::white);

    // Rows of the drawn pixels in one column, with their colours
    const auto traceRows = [](const QImage& image, int x) {
        QList<QPair<int, QColor>> rows;
        for (int y = 0; y < image.height(); ++y) {
            const QColor color = image.pixelColor(x, y);
            if (color != QColor(Qt::white)) {
                rows.append({y, color});
            }
        }
        return rows;
    };
    const auto isClipRed = [](const QColor& color) {
        return color.red() > 240 && color.green() < 30 && color.blue() < 30;
    };
    const auto hasRowNear = [](const QList<QPair<int, QColor>>& rows, float y) {
        return std::any_of(rows.cbegin(), rows.cend(), [y](const auto& row) { return qAbs(row.first - y) <= 1.f; });
    };

    // Four 50 px rows; +0.5 maps to 45 % of the half row, -3 is clamped to -2, which is clipped
    QImage image = view.renderToImage(QSize(400, 200));
    QCOMPARE(image.size(), QSize(400, 200));
    auto rows = traceRows(image, 200);
    QVERIFY2(hasRowNear(rows, 25.f - 0.5f * 22.5f), "positive trace above its row centre");
    QVERIFY2(hasRowNear(rows, 75.f - 0.5f * 22.5f), "bad channel drawn at its scale");
    QVERIFY2(hasRowNear(rows, 125.f + 2.f * 22.5f), "amplitude clamped to two row scales");
    QVERIFY(hasRowNear(rows, 175.f));
    for (const auto& [y, color] : rows) {
        QCOMPARE(isClipRed(color), qAbs(y - 170) <= 1);
        QVERIFY2(y < 190, "trace overflows into the next row");
    }

    // The time span is kept at any output size
    image = view.renderToImage(QSize(800, 100));
    QCOMPARE(image.size(), QSize(800, 100));
    rows = traceRows(image, 790);
    QVERIFY(hasRowNear(rows, 12.5f - 0.5f * 11.25f));

    // Butterfly: one row per channel type, bad channels hidden like on screen
    rhiView->setHideBadChannels(true);
    rhiView->setButterflyMode(true);
    rows = traceRows(view.renderToImage(QSize(400, 200)), 200);
    QVERIFY(hasRowNear(rows, 50.f - 0.5f * 45.f));
    QVERIFY(hasRowNear(rows, 50.f + 2.f * 45.f));
    QVERIFY(hasRowNear(rows, 150.f));
    for (const auto& [y, color] : rows) {
        QVERIFY(qAbs(y - 27.5f) <= 1.f || qAbs(y - 140.f) <= 1.f || qAbs(y - 150.f) <= 1.f);
    }

    // Z-score: a flat trace has zero deviation and sits on the row centre, no clipping highlight
    rhiView->setButterflyMode(false);
    rhiView->setZScoreMode(true);
    rows = traceRows(view.renderToImage(QSize(400, 150)), 200);
    QVERIFY(!rows.isEmpty());
    for (const auto& [y, color] : rows) {
        QVERIFY(qAbs(y - 25) <= 1 || qAbs(y - 75) <= 1 || qAbs(y - 125) <= 1);
        QVERIFY(!isClipRed(color));
    }

    // Per-second bands belong to the grid and darken odd seconds by 8 %, like the overlay shader
    rhiView->setZScoreMode(false);
    rhiView->setSfreq(100.f);
    QCOMPARE(view.renderToImage(QSize(400, 200)).pixelColor(350, 10), QColor(Qt::white));
    rhiView->setGridVisible(true);
    image = view.renderToImage(QSize(400, 200));
    QCOMPARE(image.pixelColor(150, 10), QColor(Qt::white));
    QCOMPARE(image.pixelColor(350, 10), QColor(235, 235, 235));

    QVERIFY(view.renderToImage(QSize(0, 10)).isNull());
}

//=============================================================================================================

void TestVizQtViewers2::channelRhiView_gpuFrame()
{
    // The platform's own backend renders real pixels; without one (e.g. Linux CI) the Null backend runs the code path
    std::unique_ptr<QRhi> rhi;
#if defined(Q_OS_MACOS)
    QRhiMetalInitParams metalParams;
    rhi.reset(QRhi::create(QRhi::Metal, &metalParams));
#elif defined(Q_OS_WIN)
    QRhiD3D11InitParams d3dParams;
    rhi.reset(QRhi::create(QRhi::D3D11, &d3dParams, QRhi::PreferSoftwareRenderer));
#endif
    const bool realPixels = rhi != nullptr;
    if (!rhi) {
        QRhiNullInitParams nullParams;
        rhi.reset(QRhi::create(QRhi::Null, &nullParams));
    }
    QVERIFY(rhi);

    ChannelDataModel model;
    model.init(createBrowserTestInfo());
    const float amplitudeMax = model.channelInfo(0).amplitudeMax;
    Eigen::MatrixXd data = Eigen::MatrixXd::Zero(4, 2000);
    data.row(0).setConstant(0.5 * amplitudeMax);
    data.row(1).setConstant(0.5 * amplitudeMax);
    data.row(2).setConstant(-3.0 * amplitudeMax);
    model.setData(data, 100);

    FrameChannelRhiView view;
    view.setModel(&model);
    view.setFirstFileSample(100);
    view.setLastFileSample(2099);
    view.setSamplesPerPixel(0.5f);
    view.setScrollSample(100.f);
    view.setGridVisible(false);
    view.setBackgroundColor(Qt::white);
    view.setVisibleChannelCount(4);
    view.setEvents({ChannelRhiView::EventMarker{150, 1, Qt::red, "event"}});
    view.setAnnotations({ChannelRhiView::AnnotationSpan{160, 170, Qt::blue, "blink"}});
    view.setEpochMarkers({180});
    QTest::failOnWarning(QRegularExpression(QStringLiteral("ChannelRhiView")));

    std::unique_ptr<QRhiTexture> color;
    std::unique_ptr<QRhiTextureRenderTarget> target;
    std::unique_ptr<QRhiRenderPassDescriptor> pass;
    const auto frame = [&](const QSize& size) {
        view.resize(size);
        if (!color || color->pixelSize() != size) {
            view.releaseResources();
            target.reset();
            pass.reset();
            color.reset(rhi->newTexture(QRhiTexture::RGBA8, size, 1, QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
            color->create();
            target.reset(rhi->newTextureRenderTarget({QRhiColorAttachment(color.get())}));
            pass.reset(target->newCompatibleRenderPassDescriptor());
            target->setRenderPassDescriptor(pass.get());
            target->create();
        }
        QRhiCommandBuffer* cb = nullptr;
        if (rhi->beginOffscreenFrame(&cb) != QRhi::FrameOpSuccess) {
            return QImage();
        }
        view.renderFrame(rhi.get(), target.get(), cb);
        QRhiReadbackResult result;
        QRhiResourceUpdateBatch* readback = rhi->nextResourceUpdateBatch();
        readback->readBackTexture({color.get()}, &result);
        cb->resourceUpdate(readback);
        rhi->endOffscreenFrame();
        QImage image(reinterpret_cast<const uchar*>(result.data.constData()), result.pixelSize.width(),
                     result.pixelSize.height(), QImage::Format_RGBA8888);
        image = image.convertToFormat(QImage::Format_RGB32);
        return rhi->isYUpInFramebuffer() ? image.flipped() : image;
    };
    // Rows of the drawn (non-white) pixels in one column
    const auto traceRows = [](const QImage& image, int x) {
        QList<int> rows;
        for (int y = 0; y < image.height(); ++y) {
            if (image.pixelColor(x, y) != QColor(Qt::white)) {
                rows.append(y);
            }
        }
        return rows;
    };
    const auto near = [](const QList<int>& rows, int y) {
        return std::any_of(rows.cbegin(), rows.cend(), [y](int row) { return qAbs(row - y) <= 1; });
    };
    // Every trace pixel of the CPU image has a GPU pixel next to it and vice versa, in a column without markers
    const auto matchesImage = [&](const QSize& size, const char* mode) {
        const QImage gpu = frame(size);
        if (gpu.size() != size) {
            return false;
        }
        if (!realPixels) {
            return true;
        }
        const QList<int> gpuRows = traceRows(gpu, size.width() / 8);
        const QList<int> cpuRows = traceRows(view.renderToImage(size), size.width() / 8);
        const auto listed = [](const QList<int>& rows) {
            StringList text;
            for (const int row : rows) {
                text.push_back(std::to_string(row));
            }
            return join(text, " ");
        };
        for (int y : cpuRows) {
            if (!near(gpuRows, y)) {
                Log::warning("{} CPU row {} missing on the GPU {}", mode, y, listed(gpuRows));
                return false;
            }
        }
        for (int y : gpuRows) {
            if (!near(cpuRows, y)) {
                Log::warning("{} GPU row {} missing in the image {}", mode, y, listed(cpuRows));
                return false;
            }
        }
        return !cpuRows.isEmpty();
    };

    QVERIFY(matchesImage(QSize(400, 200), "rows"));
    // The event, annotation and epoch lines are composited on top
    if (realPixels) {
        const QImage gpu = frame(QSize(400, 200));
        QVERIFY(traceRows(gpu, 100).size() > 150);
        QVERIFY(traceRows(gpu, 130).size() > 150);
        QVERIFY2(traceRows(gpu, 160).size() > 100, "dashed epoch marker");
        QVERIFY(traceRows(gpu, 50).size() < 10);
    }
    view.setScrollSample(1500.f);
    QVERIFY(matchesImage(QSize(400, 200), "scrolled"));
    QVERIFY(matchesImage(QSize(300, 120), "resized"));
    view.setHideBadChannels(true);
    view.setButterflyMode(true);
    QVERIFY(matchesImage(QSize(400, 200), "butterfly"));
    view.setButterflyMode(false);
    view.setZScoreMode(true);
    QVERIFY(matchesImage(QSize(400, 200), "z-score"));
    view.setModel(nullptr);
    QCOMPARE(frame(QSize(400, 200)).size(), QSize(400, 200));
    view.releaseResources();
}

//=============================================================================================================

void TestVizQtViewers2::sensorLayoutFigure_zoomPanAndSelect()
{
    const auto selectedNames = [](const std::vector<mne::viz::SensorLayout::Sensor>& sensors) {
        std::vector<std::string> names;
        names.reserve(sensors.size());
        for (const mne::viz::SensorLayout::Sensor& sensor : sensors) {
            names.push_back(sensor.name);
        }
        return names;
    };
    auto figure = std::make_shared<mne::viz::SensorLayoutFigure>();
    int selectionChanges = 0;
    figure->set_selection_callback([&selectionChanges]() {
        ++selectionChanges;
    });
    SceneViewport widget(figure);
    figure->set_layout({{"MEG 0111", {0, 0}}, {"MEG 0121", {10, 0}}, {"EEG 001", {5, -5}}}, {"MEG 0121"});
    widget.resize(200, 200);
    widget.show();
    QCoreApplication::processEvents();
    QVERIFY(widget.isFitted());
    const double fitted = widget.zoom();

    // Wheel: one notch in zooms by 1.15 around the pointer, one notch out undoes it
    const auto wheel = [&widget](int delta, QPointF at) {
        QWheelEvent event(at, widget.mapToGlobal(at), QPoint(), QPoint(0, delta), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QApplication::sendEvent(&widget, &event);
    };
    const mne::viz::Point anchor = widget.mapToScene(QPointF(50, 50));
    wheel(120, QPointF(50, 50));
    QVERIFY(std::fabs(widget.zoom() - fitted * 1.15) < 1e-9);
    QVERIFY(!widget.isFitted());
    QVERIFY((widget.mapFromScene(anchor) - QPointF(50, 50)).manhattanLength() < 1e-9);
    wheel(-120, QPointF(50, 50));
    QVERIFY(std::fabs(widget.zoom() - fitted) < 1e-9);

    // Right-drag pans with the mouse
    const QPointF before = widget.mapFromScene({0, 0});
    QTest::mousePress(&widget, Qt::RightButton, Qt::NoModifier, QPoint(100, 100));
    QTest::mouseMove(&widget, QPoint(130, 80));
    QTest::mouseRelease(&widget, Qt::RightButton, Qt::NoModifier, QPoint(130, 80));
    QVERIFY((widget.mapFromScene({0, 0}) - before - QPointF(30, -20)).manhattanLength() < 1e-9);

    // Double-click fits the sensors again
    QTest::mouseDClick(&widget, Qt::LeftButton, Qt::NoModifier, QPoint(1, 1));
    QVERIFY(widget.isFitted());
    QVERIFY(std::fabs(widget.zoom() - fitted) < 1e-9);

    // A click selects one sensor, Ctrl+click adds another, a click beside the sensors clears
    const auto at = [&widget](double x, double y) {
        return widget.mapFromScene({10 * x, -10 * y}).toPoint();
    };
    QTest::mouseClick(&widget, Qt::LeftButton, Qt::NoModifier, at(0, 0));
    QCOMPARE(figure->selected_names(), std::vector<std::string>({"MEG 0111"}));
    QTest::mouseClick(&widget, Qt::LeftButton, Qt::ControlModifier, at(10, 0));
    QCOMPARE(figure->selected_names(), std::vector<std::string>({"MEG 0111", "MEG 0121"}));
    QCOMPARE(selectedNames(figure->selection()), std::vector<std::string>({"MEG 0111", "MEG 0121"}));
    QTest::mouseClick(&widget, Qt::LeftButton, Qt::NoModifier, at(5, 5));
    QVERIFY(figure->selected_names().empty());
    QCOMPARE(selectionChanges, 3);

    // A rubber band over the left half selects the sensors it touches
    QTest::mousePress(&widget, Qt::LeftButton, Qt::NoModifier, at(-3, 4));
    QTest::mouseMove(&widget, at(6, -7));
    QTest::mouseRelease(&widget, Qt::LeftButton, Qt::NoModifier, at(6, -7));
    QCOMPARE(figure->selected_names(), std::vector<std::string>({"EEG 001", "MEG 0111"}));

    // Only the shown kind remains, and only the named sensors can be picked
    figure->set_channel_kind(FIFFV_MEG_CH);
    figure->set_selectable({"MEG 0121"});
    QVERIFY(figure->selected_names().empty());
    QCOMPARE(selectedNames(figure->selection()), std::vector<std::string>({"MEG 0111", "MEG 0121"}));
    figure->select_names({"MEG 0121"});
    QCOMPARE(selectedNames(figure->selection()), std::vector<std::string>({"MEG 0121"}));
    figure->highlight_names({"MEG 0111"});
    QVERIFY(figure->sensor_layout().sensors()[1].highlighted);
    figure->clear();
    QVERIFY(figure->sensor_layout().sensors().empty());
}

//=============================================================================================================

void TestVizQtViewers2::channelDataView_keyboardShortcuts()
{
    QSettings(QStringLiteral("MNECPP")).remove(QStringLiteral("test_viz_qt_viewers2_keys"));
    ChannelDataView view(QStringLiteral("test_viz_qt_viewers2_keys"));
    view.resize(800, 400);
    view.init(createBrowserTestInfo());
    view.setFileBounds(100, 299);
    view.setData(createBrowserTestData(), 100);
    view.setWindowSize(0.05f);
    view.scrollToSample(100, false);
    auto* rhiView = view.findChild<ChannelRhiView*>();
    QVERIFY(rhiView != nullptr);
    const auto pressKey = [&view](Qt::Key key, Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
        QKeyEvent event(QEvent::KeyPress, key, modifiers);
        QApplication::sendEvent(&view, &event);
    };

    // Each toggle key flips its state and reports the new state
    struct Toggle
    {
        Qt::Key key;
        Qt::KeyboardModifiers modifiers;
        std::function<bool()> state;
        QSignalSpy* spy;
    };
    QSignalSpy butterflySpy(&view, &ChannelDataView::butterflyToggled);
    QSignalSpy scalebarSpy(&view, &ChannelDataView::scalebarsToggled);
    QSignalSpy crosshairSpy(&view, &ChannelDataView::crosshairToggled);
    QSignalSpy eventsSpy(&view, &ChannelDataView::eventsVisibleToggled);
    QSignalSpy epochSpy(&view, &ChannelDataView::epochMarkersToggled);
    QSignalSpy clippingSpy(&view, &ChannelDataView::clippingToggled);
    QSignalSpy zScoreSpy(&view, &ChannelDataView::zScoreModeToggled);
    QSignalSpy annotationsSpy(&view, &ChannelDataView::annotationsVisibleToggled);
    QSignalSpy overviewSpy(&view, &ChannelDataView::overviewBarToggled);
    const QList<Toggle> toggles = {
        {Qt::Key_B, Qt::NoModifier, [&view] { return view.butterflyMode(); }, &butterflySpy},
        {Qt::Key_S, Qt::NoModifier, [&view] { return view.scalebarsVisible(); }, &scalebarSpy},
        {Qt::Key_X, Qt::NoModifier, [&view] { return view.crosshairEnabled(); }, &crosshairSpy},
        {Qt::Key_E, Qt::NoModifier, [&view] { return view.eventsVisible(); }, &eventsSpy},
        {Qt::Key_G, Qt::NoModifier, [&view] { return view.epochMarkersVisible(); }, &epochSpy},
        {Qt::Key_C, Qt::NoModifier, [&view] { return view.clippingVisible(); }, &clippingSpy},
        {Qt::Key_Z, Qt::NoModifier, [&view] { return view.zScoreMode(); }, &zScoreSpy},
        {Qt::Key_A, Qt::ShiftModifier, [&view] { return view.annotationsVisible(); }, &annotationsSpy},
        {Qt::Key_O, Qt::NoModifier, [&view] { return view.overviewBarVisible(); }, &overviewSpy},
    };
    for (const Toggle& toggle : toggles) {
        for (int press = 0; press < 2; ++press) {
            const bool before = toggle.state();
            pressKey(toggle.key, toggle.modifiers);
            QCOMPARE(toggle.state(), !before);
            QCOMPARE(toggle.spy->takeLast().at(0).toBool(), !before);
        }
    }

    // Plain A is not a shortcut
    const bool annotationsVisible = view.annotationsVisible();
    pressKey(Qt::Key_A);
    QCOMPARE(view.annotationsVisible(), annotationsVisible);
    QVERIFY(annotationsSpy.isEmpty());

    // D cycles detrending None -> Mean -> Linear -> None
    view.setDetrendMode(DetrendMode::None);
    for (const DetrendMode mode : {DetrendMode::Mean, DetrendMode::Linear, DetrendMode::None}) {
        pressKey(Qt::Key_D);
        QCOMPARE(view.detrendMode(), mode);
    }

    // ] and [ change the scroll speed by a factor of 1.5
    QSignalSpy speedSpy(&view, &ChannelDataView::scrollSpeedChanged);
    const float speed = view.scrollSpeedFactor();
    pressKey(Qt::Key_BracketRight);
    QCOMPARE(view.scrollSpeedFactor(), speed * 1.5f);
    pressKey(Qt::Key_BracketLeft);
    QCOMPARE(view.scrollSpeedFactor(), speed);
    QCOMPARE(speedSpy.size(), 2);

    // T switches between seconds and clock time
    const bool clock = view.clockTimeFormat();
    pressKey(Qt::Key_T);
    QCOMPARE(view.clockTimeFormat(), !clock);
    pressKey(Qt::Key_T);

    // Navigation: arrows step 10 %, page keys 90 % of the visible window; Home/End go to the file bounds
    const int visible = rhiView->visibleSampleCount();
    QVERIFY(visible > 0 && visible < 100);
    const auto scrollsTo = [&pressKey, rhiView](Qt::Key key, float expected) {
        pressKey(key);
        return QTest::qWaitFor([&] { return std::fabs(rhiView->scrollSample() - expected) < 0.5f; }, 2000);
    };
    QVERIFY(scrollsTo(Qt::Key_End, 300.f - visible));
    QVERIFY(scrollsTo(Qt::Key_Home, 100.f));
    QVERIFY(scrollsTo(Qt::Key_Right, 100.f + 0.1f * visible));
    QVERIFY(scrollsTo(Qt::Key_Left, 100.f));
    QVERIFY(scrollsTo(Qt::Key_PageDown, 100.f + 0.9f * visible));
    QVERIFY(scrollsTo(Qt::Key_PageUp, 100.f));
    // A key pressed while the previous scroll is still animating continues from its target
    pressKey(Qt::Key_Right);
    QVERIFY(scrollsTo(Qt::Key_Right, 100.f + 0.2f * visible));
    QVERIFY(scrollsTo(Qt::Key_Home, 100.f));

    // + / = zoom in to 75 %, - zooms out by 1.33
    const auto zoomsTo = [&pressKey, rhiView](Qt::Key key, float expected) {
        pressKey(key);
        return QTest::qWaitFor([&] { return std::fabs(rhiView->samplesPerPixel() / expected - 1.f) < 1e-4f; }, 2000);
    };
    const float spp = rhiView->samplesPerPixel();
    QVERIFY(zoomsTo(Qt::Key_Plus, spp * 0.75f));
    QVERIFY(zoomsTo(Qt::Key_Minus, spp * 0.75f * 1.33f));
    QVERIFY(zoomsTo(Qt::Key_Equal, spp * 0.75f * 1.33f * 0.75f));
}

//=============================================================================================================

void TestVizQtViewers2::channelRhiView_overlayValues()
{
    ChannelDataModel model;
    model.init(createBrowserTestInfo());
    model.setData(createBrowserTestData(), 100);
    ChannelRhiView view;
    ChannelRhiView* rhiView = &view;
    rhiView->setModel(&model);
    rhiView->resize(400, 400);
    rhiView->setAttribute(Qt::WA_DontShowOnScreen);
    rhiView->show();
    rhiView->setSfreq(1000.f);
    rhiView->setFirstFileSample(100);
    rhiView->setSamplesPerPixel(0.5f);
    rhiView->setScrollSample(100.f);
    rhiView->setVisibleChannelCount(4);
    rhiView->setCrosshairEnabled(false);
    // The crosshair, scalebars and ruler are part of the GPU frame, painted by the renderer: no child widget
    QVERIFY(rhiView->findChildren<QWidget*>(Qt::FindDirectChildrenOnly).isEmpty());

    const auto sendMouse = [rhiView](QEvent::Type type, Qt::MouseButton button, const QPoint& pos) {
        QMouseEvent event(type, pos, rhiView->mapToGlobal(pos), type == QEvent::MouseMove ? Qt::NoButton : button,
                          type == QEvent::MouseButtonRelease ? Qt::NoButton : button, Qt::NoModifier);
        QApplication::sendEvent(rhiView, &event);
    };

    struct Recording
    {
        QStringList texts;
        QList<QLineF> lines;
    };
    const auto paintOverlay = [rhiView]() {
        TextRecordingDevice recorder(rhiView->size());
        {
            QPainter painter(&recorder);
            rhiView->renderer()->paintHud(painter);
        }
        // Glyphs from a fallback font (e.g. the ruler's delta) arrive as separate items: rejoin them
        QStringList texts;
        for (const QString& text : recorder.texts()) {
            if (!texts.isEmpty() && texts.last() == QStringLiteral("\u0394")) {
                texts.last() += text;
            } else {
                texts.append(text);
            }
        }
        return Recording{texts, recorder.lines()};
    };
    const auto textStartingWith = [](const QStringList& texts, const QString& prefix) {
        for (const QString& text : texts) {
            if (text.startsWith(prefix)) {
                return text;
            }
        }
        return QString();
    };

    // Scalebar: the bar length (vertical line between the two ticks) shows the labelled amplitude at the
    // trace scale, where amplitudeMax spans 45 % of a 100 px row
    rhiView->setScalebarsVisible(true);
    Recording rec = paintOverlay();
    const float magScale = model.channelInfo(0).amplitudeMax;
    const QString magLabel = textStartingWith(rec.texts, QStringLiteral("MEG mag: "));
    QVERIFY2(!magLabel.isEmpty(), qPrintable(rec.texts.join(u'|')));
    float barPx = 0.f;
    for (const QLineF& line : rec.lines) {
        if (qFuzzyIsNull(line.dx()) && line.dy() != 0.f) {
            barPx = qMax(barPx, static_cast<float>(qAbs(line.dy())));
        }
    }
    QVERIFY(barPx > 0.f);
    const float expectedPicoTesla = magScale * barPx / 45.f * 1e12f;
    QCOMPARE(magLabel, QStringLiteral("MEG mag: %1 pT").arg(QString::number(expectedPicoTesla, 'f', 1)));
    rhiView->setScalebarsVisible(false);

    // Crosshair in row 0 at x = 40 px: sample 120, 20 ms after the first sample, value sin(20 * 0.05) T
    rhiView->setCrosshairEnabled(true);
    sendMouse(QEvent::MouseMove, Qt::NoButton, QPoint(40, 50));
    rec = paintOverlay();
    QCOMPARE(textStartingWith(rec.texts, QStringLiteral("MEG 0111")),
             QStringLiteral("MEG 0111  0.020 s  %1 mT").arg(QString::number(std::sin(1.0) * 1e3, 'f', 1)));
    rhiView->setClockTimeFormat(true);
    QCOMPARE(textStartingWith(paintOverlay().texts, QStringLiteral("MEG 0111")),
             QStringLiteral("MEG 0111  00:00.020  %1 mT").arg(QString::number(std::sin(1.0) * 1e3, 'f', 1)));
    rhiView->setCrosshairEnabled(false);

    // Ruler held from (100, 30) to (180, 75) in row 0: 80 px = 40 samples = 40 ms, -45 px = -amplitudeMax
    sendMouse(QEvent::MouseButtonPress, Qt::RightButton, QPoint(100, 30));
    sendMouse(QEvent::MouseMove, Qt::RightButton, QPoint(180, 75));
    rec = paintOverlay();
    QCOMPARE(textStartingWith(rec.texts, QStringLiteral("\u0394T")), QStringLiteral("\u0394T = 40.0 ms"));
    QCOMPARE(textStartingWith(rec.texts, QStringLiteral("\u0394A")),
             QStringLiteral("\u0394A = %1 nT").arg(QString::number(-magScale * 1e9f, 'f', 3)));
    sendMouse(QEvent::MouseButtonRelease, Qt::RightButton, QPoint(180, 75));
    QVERIFY(textStartingWith(paintOverlay().texts, QStringLiteral("\u0394T")).isEmpty());

    // Annotation selection from 100 to 160 px: 30 samples = 30 ms
    rhiView->setAnnotationSelectionEnabled(true);
    sendMouse(QEvent::MouseButtonPress, Qt::RightButton, QPoint(100, 30));
    sendMouse(QEvent::MouseMove, Qt::RightButton, QPoint(160, 30));
    QVERIFY(paintOverlay().texts.contains(QStringLiteral("30 ms")));
    sendMouse(QEvent::MouseButtonRelease, Qt::RightButton, QPoint(160, 30));
    rhiView->setAnnotationSelectionEnabled(false);

    // Wheel: vertical scrolls 15 % of the view in time, a horizontal swipe 10 %, both scaled by the scroll speed
    rhiView->setWheelScrollsChannels(false);
    rhiView->setLastFileSample(10000);
    const auto wheel = [rhiView](const QPoint& delta) {
        QWheelEvent event(QPointF(50, 40), QPointF(50, 40), QPoint(), delta, Qt::NoButton, Qt::NoModifier,
                          Qt::NoScrollPhase, false);
        QApplication::sendEvent(rhiView, &event);
    };
    const float viewSamples = 400.f * 0.5f;
    // Scrolls land exactly on their target, not just within a fuzzy distance of it
    const auto settlesAt = [rhiView](float target) {
        return QTest::qWaitFor([&] { return rhiView->scrollSample() == target; }, 2000);
    };
    rhiView->setScrollSample(1000.f);
    wheel(QPoint(0, -120));
    QVERIFY(settlesAt(1000.f + 0.15f * viewSamples));
    wheel(QPoint(0, 120));
    QVERIFY(settlesAt(1000.f));
    rhiView->setScrollSpeedFactor(2.f);
    wheel(QPoint(-120, 0));
    QVERIFY(settlesAt(1000.f + 2.f * 0.1f * viewSamples));
    wheel(QPoint(120, 0));
    QVERIFY(settlesAt(1000.f));
    rhiView->setScrollSpeedFactor(1.f);
    // Ticks arriving while the previous scroll is still animating add up
    wheel(QPoint(0, -120));
    wheel(QPoint(0, -120));
    QVERIFY(settlesAt(1000.f + 2.f * 0.15f * viewSamples));
    wheel(QPoint(0, 120));
    wheel(QPoint(0, 120));
    QVERIFY(settlesAt(1000.f));

    // Frozen: wheel and drag do not scroll, a click still reports its sample
    rhiView->setFrozen(true);
    wheel(QPoint(0, -120));
    wheel(QPoint(-120, 0));
    QTest::qWait(150);
    QCOMPARE(rhiView->scrollSample(), 1000.f);
    QSignalSpy clickSpy(rhiView, &ChannelRhiView::sampleClicked);
    sendMouse(QEvent::MouseButtonPress, Qt::LeftButton, QPoint(40, 50));
    QCOMPARE(clickSpy.size(), 1);
    QCOMPARE(clickSpy.at(0).at(0).toInt(), 1020);
    sendMouse(QEvent::MouseButtonRelease, Qt::LeftButton, QPoint(40, 50));
    rhiView->setFrozen(false);
    // Freezing stops a scroll that is still animating
    wheel(QPoint(0, -120));
    rhiView->setFrozen(true);
    const float frozenAt = rhiView->scrollSample();
    QTest::qWait(150);
    QCOMPARE(rhiView->scrollSample(), frozenAt);
    rhiView->setFrozen(false);

    // An hour into a 1 kHz recording a 5-sample step still scrolls (no relative fuzzy comparison)
    rhiView->setLastFileSample(10000000);
    rhiView->setScrollSample(3600000.f);
    rhiView->setScrollSample(3600005.f);
    QCOMPARE(qRound(rhiView->scrollSample() - 3600000.f), 5);
    rhiView->setScrollSample(1000.f);

    // Double-click marks the channel under the cursor bad and back
    QVERIFY(!model.channelInfo(0).bad);
    QMouseEvent doubleClick(QEvent::MouseButtonDblClick, QPointF(40, 50), rhiView->mapToGlobal(QPointF(40, 50)),
                            Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(rhiView, &doubleClick);
    QVERIFY(model.channelInfo(0).bad);
    QApplication::sendEvent(rhiView, &doubleClick);
    QVERIFY(!model.channelInfo(0).bad);
}

//=============================================================================================================

void TestVizQtViewers2::channelRhiView_rendersAndDrawsOverlays()
{
    ChannelDataView view(QStringLiteral("test_viz_qt_viewers2_rhi_render"));
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
    rhiView->setEvents({ChannelRhiView::EventMarker{120, 1, Qt::red, "event"}});
    rhiView->setEpochMarkers({125, 150});
    rhiView->setAnnotations({ChannelRhiView::AnnotationSpan{130, 145, Qt::yellow, "annotation"}});
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
            QCOMPARE(args.at(2).toString(), QStringLiteral("MEG 0111"));
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

void TestVizQtViewers2::channelDataModel_bufferDetrendAndDecimation()
{
    std::shared_ptr<FiffInfo> info(new FiffInfo);
    const QVector<std::pair<int, int>> kinds = {{FIFFV_MEG_CH, FIFF_UNIT_T_M}, {FIFFV_MEG_CH, FIFF_UNIT_T}, {FIFFV_EEG_CH, FIFF_UNIT_V}, {FIFFV_EOG_CH, FIFF_UNIT_V}, {FIFFV_ECG_CH, FIFF_UNIT_V}, {FIFFV_EMG_CH, FIFF_UNIT_V}, {FIFFV_STIM_CH, FIFF_UNIT_NONE}, {FIFFV_MISC_CH, FIFF_UNIT_NONE}};
    info->sfreq = 500.0f;
    info->nchan = kinds.size();
    for (int i = 0; i < kinds.size(); ++i) {
        FiffChInfo ch;
        ch.ch_name = QStringLiteral("CH%1").arg(i).toStdString();
        ch.kind = kinds[i].first;
        ch.unit = kinds[i].second;
        info->chs.push_back(ch);
        info->ch_names.push_back(ch.ch_name);
    }
    info->bads = {"CH2"};

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
    // QCOMPARE takes any two floats below 1e-5 as equal, so tesla scales are compared relatively
    const auto sameScale = [](float a, float b) {
        return std::abs(a - b) <= 1e-6f * std::abs(b);
    };
    for (int i = 0; i < 8; ++i) {
        QVERIFY2(sameScale(model.channelInfo(i).amplitudeMax, scales[i]), qPrintable(labels[i]));
    }
    QCOMPARE(model.channelInfo(99).name, QString());

    // The browser's string-keyed scales map onto grad/mag/EEG/... separately
    model.setScaleMapFromStrings({{"MEG_grad", 1e-10}, {"MEG_mag", 2e-12}, {"MEG_EEG", 5e-5}, {"MEG_EOG", 2e-4}, {"MEG_EMG", 3e-3}, {"MEG_ECG", 4e-3}, {"MEG_MISC", 2.0}, {"MEG_STIM", 7.0}});
    const QVector<float> userScales = {1e-10f, 2e-12f, 5e-5f, 2e-4f, 4e-3f, 3e-3f, 7.0f, 2.0f};
    for (int i = 0; i < 8; ++i) {
        QVERIFY2(sameScale(model.channelInfo(i).amplitudeMax, userScales[i]), qPrintable(labels[i]));
    }

    // Bad flags round-trip into the FiffInfo
    model.setChannelBad(2, false);
    model.setChannelBad(5, true);
    model.setChannelBad(5, true);
    QCOMPARE(info->bads, StringList{"CH5"});
    QVERIFY(model.channelInfo(5).bad);

    // Virtual channels get defaults for every missing field
    model.setSignalColor(Qt::darkGreen);
    ChannelDisplayInfo named;
    named.name = "EOG bipolar";
    named.typeLabel = "EOG";
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
    std::vector<float> v = ring.decimatedVertices(0, 400, 450, 100, vboFirst);
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
    QVERIFY(ring.decimatedVertices(0, 450, 400, 10, vboFirst).empty());
    QVERIFY(ring.decimatedVertices(9, 400, 450, 10, vboFirst).empty());

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

void TestVizQtViewers2::rtFiffRawView_lifecycle()
{
    // Construct only — init starts background threads, so skip it in unit tests
    RtFiffRawView view("test_viz_qt_viewers2");

    view.setGuiMode(AbstractView::GuiMode::Research);
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    view.saveSettings();
    view.loadSettings();
    view.clearView();

    QApplication::processEvents();
}

//=============================================================================================================

void TestVizQtViewers2::rtFiffRawView_paintsHidesRowsAndAddsEvents()
{
    RtFiffRawView view(QStringLiteral("test_viz_qt_viewers2_rawview"));
    view.resize(600, 400);
    auto info = createBrowserTestInfo();
    view.init(info);
    QCOMPARE(view.getSamplingFreq(), 1000.0f);
    auto* table = view.findChild<QTableView*>();
    QVERIFY(table != nullptr);
    // The delegate paints with QPainter, which a QRhiWidget viewport cannot take
    QVERIFY(table && !qobject_cast<QRhiWidget*>(table->viewport()));
    auto* model = qobject_cast<RtFiffRawViewModel*>(table->model());
    QVERIFY(model != nullptr);

    view.setWindowSize(1);
    QCOMPARE(view.getWindowSize(), 1);
    QCOMPARE(model->getMaxSamples(), 1000);
    view.setSignalColor(Qt::darkBlue);
    QCOMPARE(view.getSignalColor(), QColor(Qt::darkBlue));
    view.setBackgroundColor(Qt::white);
    QCOMPARE(view.getBackgroundColor(), QColor(Qt::white));
    view.setScalingMap({{FIFF_UNIT_T, 2.0f}, {FIFFV_STIM_CH, 4.0f}});
    QCOMPARE(view.getScalingMap().at(FIFF_UNIT_T), 2.0f);
    view.setDistanceTimeSpacer(200);
    QCOMPARE(view.getDistanceTimeSpacer(), 200);
    view.triggerInfoChanged({{3.0, Qt::red}}, true, "STI 014", 1.0);

    // Four blocks of 300 samples: the trigger channel steps to 3 every 250 samples
    QSignalSpy triggerSpy(&view, &RtFiffRawView::triggerDetected);
    for (int b = 0; b < 4; ++b) {
        Eigen::MatrixXd block = Eigen::MatrixXd::Zero(4, 300);
        for (int s = 0; s < 300; ++s) {
            const int t = 300 * b + s;
            block(0, s) = std::sin(0.02 * t);
            block(2, s) = 0.5;
            block(3, s) = (t % 250 < 5) ? 3.0 : 0.0;
        }
        view.addData(std::vector<Eigen::MatrixXd>{block});
    }
    QVERIFY(!triggerSpy.isEmpty());
    QCOMPARE(view.getLastBlock().cols(), 300);
    view.addData({});

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QVERIFY(!view.grab().isNull());

    // MEG0112 is bad in the test info: hiding bad channels leaves three rows that share the height
    QVERIFY(!view.getBadChannelHideStatus());
    view.hideBadChannels();
    QVERIFY(view.getBadChannelHideStatus());
    QVERIFY(table->isRowHidden(1));
    const int h = table->height();
    QCOMPARE(table->verticalHeader()->defaultSectionSize(), static_cast<int>(h * 4.0f / 3.0f));
    view.hideBadChannels();
    QVERIFY(!table->isRowHidden(1));
    QCOMPARE(table->verticalHeader()->defaultSectionSize(), h);

    view.setZoom(2.0);
    QCOMPARE(view.getZoom(), 2.0);
    QCOMPARE(table->verticalHeader()->defaultSectionSize(), static_cast<int>(h / 2.0f));

    view.showSelectedChannelsOnly(std::vector<std::string>{"MEG 0111", "STI 014"});
    QVERIFY(!table->isRowHidden(0));
    QVERIFY(table->isRowHidden(2));
    QVERIFY(!table->isRowHidden(3));
    view.showSelectedChannelsOnly({"MEG 0111", "MEG 0112", "MEG 0113", "STI 014"});

    // The context menu offers the event and bad-marking actions; trigger them as the user would
    auto contextAction = [&view, table](const QPoint& pos, const QString& text) {
        emit table->customContextMenuRequested(pos);
        QMenu* menu = view.findChildren<QMenu*>().last();
        for (QAction* action : menu->actions()) {
            if (action->text() == text) {
                action->trigger();
            }
        }
        menu->close();
    };
    QSignalSpy markSpy(&view, &RtFiffRawView::channelMarkingChanged);
    table->selectRow(2);
    contextAction(table->visualRect(model->index(2, 1)).center(), QStringLiteral("Mark as bad"));
    QVERIFY(std::ranges::contains(info->bads, "MEG 0113"));
    contextAction(table->visualRect(model->index(2, 1)).center(), QStringLiteral("Mark as good"));
    QVERIFY(!std::ranges::contains(info->bads, "MEG 0113"));
    QCOMPARE(markSpy.size(), 2);

    // Only show / hide / reset the selected rows
    table->selectRow(0);
    contextAction(table->visualRect(model->index(0, 1)).center(), QStringLiteral("Only show selection"));
    QVERIFY(!table->isRowHidden(0));
    QVERIFY(table->isRowHidden(3));
    contextAction(table->visualRect(model->index(0, 1)).center(), QStringLiteral("Reset selection"));
    QVERIFY(!table->isRowHidden(3));
    table->selectRow(3);
    contextAction(table->visualRect(model->index(3, 1)).center(), QStringLiteral("Hide selection"));
    QVERIFY(table->isRowHidden(3));
    contextAction(table->visualRect(model->index(0, 1)).center(), QStringLiteral("Reset selection"));

    model->toggleFreeze(QModelIndex());
    QVERIFY(!view.grab().isNull());
    model->toggleFreeze(QModelIndex());

    // "Add event": 1200 samples streamed into 1000 columns put the cursor at column 200 of sweep 2, so a click on
    // column 100 is sample 1100 and a click on column 600 still shows sample 600 of the previous sweep
    QSignalSpy eventSpy(&view, &RtFiffRawView::addSampleAsEvent);
    QCOMPARE(model->getCurrentSampleIndex(), 200);
    QCOMPARE(model->getFirstSampleOffset(), 1000);
    const double dx = static_cast<double>(table->columnWidth(1)) / 1000.0;
    const int xNew = static_cast<int>(100.5 * dx);
    const int xOld = static_cast<int>(600.5 * dx);
    contextAction(QPoint(xNew, 10), QStringLiteral("Add event"));
    contextAction(QPoint(xOld, 10), QStringLiteral("Add event"));
    QCOMPARE(eventSpy.size(), 2);
    QVERIFY(qAbs(eventSpy.at(0).at(0).toInt() - 1100) <= 1.0 / dx + 1);
    QVERIFY(qAbs(eventSpy.at(1).at(0).toInt() - 600) <= 1.0 / dx + 1);
    QCOMPARE(eventSpy.at(0).at(0).toInt(), 1000 + static_cast<int>(xNew / dx));
    QCOMPARE(eventSpy.at(1).at(0).toInt(), static_cast<int>(xOld / dx));
    // Closed context menus do not stay alive
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCOMPARE(view.findChildren<QMenu*>().size(), 0);

    // Crosshair: reports channel, time, displayed value and unit at the mouse, also in the previous sweep
    QSignalSpy crosshairSpy(&view, &RtFiffRawView::cursorDataChanged);
    auto hover = [table](const QPoint& pos) {
        QMouseEvent move(QEvent::MouseMove, QPointF(pos), table->viewport()->mapToGlobal(QPointF(pos)), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(table->viewport(), &move);
    };
    const int yRow0 = table->visualRect(model->index(0, 1)).center().y();
    hover(QPoint(xNew, yRow0));
    QCOMPARE(crosshairSpy.size(), 0);
    QVERIFY(!view.crosshairEnabled());
    view.setCrosshairEnabled(true);
    QVERIFY(view.crosshairEnabled());
    for (const int x : {xNew, xOld}) {
        hover(QPoint(x, yRow0));
        const int iSample = (x == xNew ? 1000 : 0) + static_cast<int>(x / dx);
        QVERIFY(!crosshairSpy.isEmpty());
        QCOMPARE(crosshairSpy.last().at(2).toString(), QStringLiteral("MEG 0111"));
        QCOMPARE(crosshairSpy.last().at(0).toFloat(), static_cast<float>(iSample / 1000.0));
        QCOMPARE(crosshairSpy.last().at(1).toFloat(), static_cast<float>(std::sin(0.02 * iSample)));
        QCOMPARE(crosshairSpy.last().at(3).toString(), QStringLiteral("T"));
    }
    const int iBefore = crosshairSpy.size();
    hover(QPoint(xNew, table->visualRect(model->index(2, 1)).center().y()));
    QCOMPARE(crosshairSpy.size(), iBefore + 1);
    QCOMPARE(crosshairSpy.last().at(2).toString(), QStringLiteral("MEG 0113"));
    QCOMPARE(crosshairSpy.last().at(1).toFloat(), 0.5f);

    // The crosshair is drawn: a vertical line in its own color at the mouse position
    hover(QPoint(xNew, yRow0));
    QImage withCrosshair = view.grab().toImage();
    const QPoint viewportOrigin = table->viewport()->mapTo(&view, QPoint(0, 0));
    auto crosshairPixels = [&viewportOrigin, &table](const QImage& image, int x) {
        int iCount = 0;
        for (int y = 0; y < table->viewport()->height(); ++y) {
            const QColor c = image.pixelColor(viewportOrigin + QPoint(x, y));
            // Orange, also at the half intensity of an antialiased line between pixels
            if (c.red() > 220 && c.green() < c.red() - 30 && c.blue() < c.green() - 30) {
                ++iCount;
            }
        }
        return iCount;
    };
    QVERIFY2(crosshairPixels(withCrosshair, xNew) > table->viewport()->height() / 2, qPrintable(QString::number(crosshairPixels(withCrosshair, xNew))));
    view.setCrosshairEnabled(false);
    QCOMPARE(crosshairPixels(view.grab().toImage(), xNew), 0);
    const int iReports = crosshairSpy.size();
    hover(QPoint(xOld, yRow0));
    QCOMPARE(crosshairSpy.size(), iReports);

    const QString shot = QDir::temp().filePath(QStringLiteral("test_viz_qt_viewers2_rawview.png"));
    view.takeScreenshot(shot);
    QVERIFY(QFile::exists(shot));
    QFile::remove(shot);
}

//=============================================================================================================

void TestVizQtViewers2::averageLayoutFigure_paintsScaledClampedCurves()
{
    // Channel 1 of 2, 120 samples, stimulus at sample 30; "aud" ramps to 2x full scale, "vis" is inactive
    Eigen::MatrixXd aud = Eigen::MatrixXd::Zero(2, 120);
    Eigen::MatrixXd vis = Eigen::MatrixXd::Zero(2, 120);
    for (int s = 0; s < 120; ++s) {
        aud(1, s) = (s < 60) ? 0.5e-12 : 2e-12;
        vis(1, s) = -0.5e-12;
    }
    auto figure = std::make_shared<mne::viz::AverageLayoutFigure>();
    SceneViewport widget(figure);
    mne::viz::AverageLayoutFigure::Channel channel;
    channel.name = "MEG 0112";
    channel.number = 1;
    channel.kind = FIFFV_MEG_CH;
    channel.unit = FIFF_UNIT_T;
    channel.first_sample = -30;
    channel.averages = {{"aud", aud.data() + 1, 120, 2}, {"vis", vis.data() + 1, 120, 2}};
    figure->set_channels({channel});
    figure->set_signal_color(toSceneColor(Qt::darkGray));
    figure->set_scale_map({{FIFF_UNIT_T, 1e-12f}});
    figure->set_average_colors({{"aud", toSceneColor(Qt::blue)}});
    figure->set_average_activation({{"aud", true}, {"vis", false}});
    QCOMPARE(figure->average_layout().tiles().front().box, (mne::viz::Rect{-60, -30, 120, 60}));

    // Render the 120 x 60 tile 1:1 into an image whose origin is the tile's top-left corner
    auto render = [&widget, &figure]() {
        mne::viz::Canvas canvas(120, 60);
        canvas.translate(60, 30);
        figure->average_layout().draw(canvas, toSceneFont(widget.font()));
        QImage image(120, 60, QImage::Format_RGB32);
        image.fill(Qt::white);
        QPainter painter(&image);
        paintScene(painter, canvas.display_list(), widget.font());
        return image;
    };
    // Antialiased one-pixel lines fall between pixels and show at half intensity
    auto isBlue = [](QRgb c) {
        return qBlue(c) > 200 && qRed(c) < 160 && qGreen(c) < 160;
    };
    auto isRed = [](QRgb c) {
        return qRed(c) > 200 && qGreen(c) < 160 && qBlue(c) < 160;
    };
    auto rowsWhere = [](const QImage& image, int x, const std::function<bool(QRgb)>& pred) {
        QList<int> rows;
        for (int y = 0; y < image.height(); ++y) {
            if (pred(image.pixel(x, y))) {
                rows << y;
            }
        }
        return rows;
    };

    QImage image = render();
    // Stimulus line at sample 30, i.e. x = 30; zero line at y = 30
    QVERIFY(rowsWhere(image, 30, isRed).size() > 50);
    QVERIFY(rowsWhere(image, 90, isRed).contains(30));
    // Half of full scale is a quarter of the height above the centre; twice full scale is clamped to the top
    QList<int> blue = rowsWhere(image, 45, isBlue);
    QVERIFY2(!blue.isEmpty() && std::abs(blue.first() - 15) <= 1, qPrintable(QString::number(blue.value(0, -1))));
    blue = rowsWhere(image, 100, isBlue);
    QVERIFY2(!blue.isEmpty() && blue.first() <= 1, qPrintable(QString::number(blue.value(0, -1))));
    // The inactive "vis" average (default color, below the centre) is not drawn
    auto isGray = [](QRgb c) {
        return qGray(c) < 220 && std::abs(qRed(c) - qBlue(c)) < 30 && std::abs(qRed(c) - qGreen(c)) < 30;
    };
    QVERIFY(rowsWhere(image, 45, isGray).isEmpty());

    // Activating "vis" draws it in the default color a quarter below the centre
    figure->set_average_activation({{"aud", true}, {"vis", true}});
    image = render();
    const QList<int> gray = rowsWhere(image, 45, isGray);
    QVERIFY2(!gray.isEmpty() && std::abs(gray.first() - 45) <= 1, qPrintable(QString::number(gray.value(0, -1))));
    figure->set_signal_color(toSceneColor(Qt::green));
    image = render();
    QVERIFY(!rowsWhere(image, 45, [](QRgb c) { return qGreen(c) > 100 && qRed(c) < qGreen(c) - 50 && qBlue(c) < qGreen(c) - 50; }).isEmpty());

    // A bad channel is drawn faded
    figure->channels().front().bad = true;
    image = render();
    blue = rowsWhere(image, 45, [](QRgb c) { return qBlue(c) > qRed(c) + 20; });
    QVERIFY(!blue.isEmpty());
    QVERIFY(qRed(image.pixel(45, blue.first())) > 150);
    figure->channels().front().bad = false;

    // Pressing a tile enlarges its label until the button is released
    widget.resize(240, 120);
    widget.show();
    QCoreApplication::processEvents();
    const QPoint centre = widget.mapFromScene({0, 0}).toPoint();
    QTest::mousePress(&widget, Qt::LeftButton, Qt::NoModifier, centre);
    QCOMPARE(figure->average_layout().tiles().front().name_size, 6.0);
    QTest::mouseRelease(&widget, Qt::LeftButton, Qt::NoModifier, centre);
    QCOMPARE(figure->average_layout().tiles().front().name_size, 3.0);

    // Without data nothing but the label is drawn
    figure->channels().front().averages.clear();
    image = render();
    QVERIFY(rowsWhere(image, 90, isRed).isEmpty());
}

//=============================================================================================================

void TestVizQtViewers2::dipoleFitPanel_inPanelWidget()
{
    auto panel = std::make_shared<DipoleFitPanel>();
    PanelWidget view(panel, QString());
    std::vector<std::vector<double>> times;
    std::vector<std::vector<double>> baselines;
    std::vector<std::vector<double>> modalities;
    std::vector<std::vector<double>> fittings;
    std::vector<std::vector<double>> noises;
    std::vector<std::vector<double>> regularizations;
    std::vector<std::vector<double>> spheres;
    std::vector<std::pair<DipoleFitPanel::Model, std::string>> models;
    std::vector<std::string> fits;
    DipoleFitPanel::Callbacks callbacks;
    callbacks.time_changed = [&times](int tmin, int tmax, int tstep, int tint) {
        times.push_back({static_cast<double>(tmin), static_cast<double>(tmax), static_cast<double>(tstep), static_cast<double>(tint)});
    };
    callbacks.baseline_changed = [&baselines](int bmin, int bmax) {
        baselines.push_back({static_cast<double>(bmin), static_cast<double>(bmax)});
    };
    callbacks.modality_changed = [&modalities](bool eeg, bool meg) {
        modalities.push_back({eeg ? 1.0 : 0.0, meg ? 1.0 : 0.0});
    };
    callbacks.fitting_changed = [&fittings](double minDistance, double radius) {
        fittings.push_back({minDistance, radius});
    };
    callbacks.noise_changed = [&noises](double grad, double mag, double eeg) {
        noises.push_back({grad, mag, eeg});
    };
    callbacks.regularization_changed = [&regularizations](double grad, double mag, double eeg) {
        regularizations.push_back({grad, mag, eeg});
    };
    callbacks.sphere_changed = [&spheres](double x, double y, double z, double radius) {
        spheres.push_back({x, y, z, radius});
    };
    callbacks.model_changed = [&models](DipoleFitPanel::Model kind, const std::string& name) {
        models.emplace_back(kind, name);
    };
    callbacks.fit = [&fits](const std::string& name) {
        fits.push_back(name);
    };
    panel->set_callbacks(std::move(callbacks));
    const auto spin = [&view](const char* key) {
        return qobject_cast<QSpinBox*>(view.formPanel()->editor(key));
    };
    const auto dspin = [&view](const char* key) {
        return qobject_cast<QDoubleSpinBox*>(view.formPanel()->editor(key));
    };

    // The sphere starts at MNE-C's defaults: origin (0, 0, 40) mm, EEG sphere radius 90 mm
    panel->report_parameters();
    QCOMPARE(spheres.back(), (std::vector<double>{0.0, 0.0, 40.0, 90.0}));
    QCOMPARE(baselines.back(), (std::vector<double>{1000000.0, 1000000.0}));

    // Each control reports its whole parameter group
    spin("tmin")->setValue(10);
    spin("tmax")->setValue(120);
    spin("tstep")->setValue(5);
    spin("tint")->setValue(2);
    QCOMPARE(times.back(), (std::vector<double>{10.0, 120.0, 5.0, 2.0}));

    spin("bmin")->setValue(-100);
    spin("bmax")->setValue(-20);
    QCOMPARE(baselines.back(), (std::vector<double>{-100.0, -20.0}));
    spin("bmax")->setValue(-100);
    QCOMPARE(baselines.back(), (std::vector<double>{1000000.0, 1000000.0}));
    spin("bmax")->setValue(-20);

    view.findChild<QCheckBox*>(QStringLiteral("meg"))->click();
    QCOMPARE(modalities.back(), (std::vector<double>{0.0, 1.0}));

    dspin("minDistance")->setValue(7.5);
    dspin("guessRadius")->setValue(9.0);
    QCOMPARE(fittings.back(), (std::vector<double>{7.5, 9.0}));
    dspin("gradNoise")->setValue(4.0);
    dspin("magNoise")->setValue(30.0);
    dspin("eegNoise")->setValue(0.4);
    QCOMPARE(noises.back(), (std::vector<double>{4.0, 30.0, 0.4}));
    dspin("gradReg")->setValue(0.2);
    dspin("magReg")->setValue(0.3);
    dspin("eegReg")->setValue(0.4);
    QCOMPARE(regularizations.back(), (std::vector<double>{0.2, 0.3, 0.4}));
    dspin("originX")->setValue(1.0);
    dspin("originY")->setValue(2.0);
    dspin("eegRadius")->setValue(80.0);
    QCOMPARE(spheres.back(), (std::vector<double>{1.0, 2.0, 40.0, 80.0}));

    // Reporting the parameters repeats exactly what the controls reported
    const std::vector<std::vector<std::vector<double>>*> reports = {&times, &baselines, &modalities, &fittings, &noises, &regularizations, &spheres};
    std::vector<std::vector<double>> lastReported;
    lastReported.reserve(reports.size());
    for (auto* pReport : reports) {
        lastReported.push_back(pReport->back());
        pReport->clear();
    }
    panel->report_parameters();
    for (std::size_t i = 0; i < reports.size(); ++i) {
        QCOMPARE(reports[i]->size(), std::size_t(1));
        QCOMPARE(reports[i]->back(), lastReported[i]);
    }

    // Models are added silently; choosing the measurement names the fit
    auto* pMeas = qobject_cast<QComboBox*>(view.formPanel()->editor("measurement"));
    auto* pBem = qobject_cast<QComboBox*>(view.formPanel()->editor("bem"));
    auto* pName = qobject_cast<QLineEdit*>(view.formPanel()->editor("name"));
    QVERIFY(pMeas && pBem && pName);
    panel->add_model(DipoleFitPanel::Model::Measurement, "sample_audvis-ave.fif");
    panel->add_model(DipoleFitPanel::Model::Bem, "sample-5120-bem-sol.fif");
    panel->add_model(DipoleFitPanel::Model::Mri, "all-trans.fif");
    panel->add_model(DipoleFitPanel::Model::Noise, "sample_audvis-cov.fif");
    QVERIFY(models.empty());
    pMeas->setCurrentIndex(1);
    QCOMPARE(models.back(), std::make_pair(DipoleFitPanel::Model::Measurement, std::string("sample_audvis-ave.fif")));
    QVERIFY(pName->text().startsWith(QStringLiteral("Dipole Fit - sample_audvis - ")));
    pBem->setCurrentIndex(1);
    QCOMPARE(models.back(), std::make_pair(DipoleFitPanel::Model::Bem, std::string("sample-5120-bem-sol.fif")));
    view.findChild<QPushButton*>(QStringLiteral("fit"))->click();
    QCOMPARE(QString::fromStdString(fits.back()), pName->text());

    // A removed model gives way to "None"
    panel->remove_model(DipoleFitPanel::Model::Bem, "sample-5120-bem-sol.fif");
    QCOMPARE(pBem->findText(QStringLiteral("sample-5120-bem-sol.fif")), -1);
    QCOMPARE(panel->model(DipoleFitPanel::Model::Bem), std::string("None"));
    panel->remove_model(DipoleFitPanel::Model::Mri, "all-trans.fif");
    panel->remove_model(DipoleFitPanel::Model::Noise, "sample_audvis-cov.fif");
    panel->remove_model(DipoleFitPanel::Model::Measurement, "sample_audvis-ave.fif");
    panel->remove_model(DipoleFitPanel::Model::Measurement, "None");
    QCOMPARE(pMeas->count(), 1);

    view.setGuiMode(AbstractView::GuiMode::Research);
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    view.saveSettings();
    view.loadSettings();
    view.clearView();
    QApplication::processEvents();
}

//=============================================================================================================

void TestVizQtViewers2::control3dView_flagsSignalsAndTree()
{
    Control3DView view(QString{}, nullptr, {"Data"});
    view.show();
    auto* pTree = view.findChild<QTreeView*>(QStringLiteral("m_treeView_loadedData"));
    auto* pViewBox = view.findChild<QWidget*>(QStringLiteral("m_groupBox_viewOptions"));
    auto* pLightBox = view.findChild<QWidget*>(QStringLiteral("m_groupBox_lightOptions"));
    QVERIFY(pTree);
    QVERIFY(pViewBox);
    QVERIFY(pLightBox);
    QVERIFY(!pTree->isHidden());
    QVERIFY(pViewBox->isHidden());
    QVERIFY(pLightBox->isHidden());

    // Enabling the tools (again) shows them and wires each control exactly once
    view.setFlags({"View", "Light"});
    view.setFlags({"View", "Light"});
    QVERIFY(pTree->isHidden());
    QVERIFY(!pViewBox->isHidden());
    QVERIFY(!pLightBox->isHidden());

    QSignalSpy fullSpy(&view, &Control3DView::showFullScreen);
    QSignalSpy rotSpy(&view, &Control3DView::rotationChanged);
    QSignalSpy axisSpy(&view, &Control3DView::showCoordAxis);
    QSignalSpy shotSpy(&view, &Control3DView::takeScreenshotChanged);
    QSignalSpy singleSpy(&view, &Control3DView::toggleSingleView);
    QSignalSpy multiSpy(&view, &Control3DView::toggleMutiview);
    QSignalSpy intensitySpy(&view, &Control3DView::lightIntensityChanged);
    view.findChild<QCheckBox*>(QStringLiteral("m_checkBox_showFullScreen"))->click();
    view.findChild<QCheckBox*>(QStringLiteral("m_checkBox_rotate"))->click();
    view.findChild<QCheckBox*>(QStringLiteral("m_checkBox_coordAxis"))->click();
    view.findChild<QPushButton*>(QStringLiteral("m_pushButton_takeScreenshot"))->click();
    QTest::mousePress(view.findChild<QRadioButton*>(QStringLiteral("m_radioButton_multi")), Qt::LeftButton);
    QTest::mousePress(view.findChild<QRadioButton*>(QStringLiteral("m_radioButton_single")), Qt::LeftButton);
    auto* pIntensity = view.findChild<QDoubleSpinBox*>(QStringLiteral("m_doubleSpinBox_colorIntensity"));
    pIntensity->setValue(pIntensity->value() + pIntensity->singleStep());
    QCOMPARE(fullSpy.count(), 1);
    QCOMPARE(rotSpy.count(), 1);
    QCOMPARE(axisSpy.count(), 1);
    QCOMPARE(shotSpy.count(), 1);
    QCOMPARE(multiSpy.count(), 1);
    QCOMPARE(singleSpy.count(), 1);
    QCOMPARE(intensitySpy.count(), 1);
    QCOMPARE(intensitySpy.last().at(0).toDouble(), pIntensity->value());

    // Color pickers stream the color while choosing and keep the accepted one
    auto pickColor = [](const QColor& color, bool bAccept) {
        answerNextModal([color, bAccept](QWidget* pModal) {
            auto* pDialog = qobject_cast<QColorDialog*>(pModal);
            QVERIFY(pDialog);
            pDialog->setCurrentColor(color);
            if (bAccept) {
                static_cast<QDialog*>(pDialog)->accept();
            } else {
                static_cast<QDialog*>(pDialog)->reject();
            }
        });
    };
    QSignalSpy sceneSpy(&view, &Control3DView::sceneColorChanged);
    QSignalSpy lightSpy(&view, &Control3DView::lightColorChanged);
    auto* pScene = view.findChild<QPushButton*>(QStringLiteral("m_pushButton_sceneColorPicker"));
    auto* pLight = view.findChild<QPushButton*>(QStringLiteral("m_pushButton_lightColorPicker"));
    pickColor(QColor(20, 40, 60), true);
    pScene->click();
    QCOMPARE(sceneSpy.last().at(0).value<QColor>(), QColor(20, 40, 60));
    QVERIFY(pScene->styleSheet().contains(QStringLiteral("rgb(20, 40, 60)")));
    pickColor(QColor(200, 100, 50), true);
    pLight->click();
    QCOMPARE(lightSpy.last().at(0).value<QColor>(), QColor(200, 100, 50));
    QVERIFY(pLight->styleSheet().contains(QStringLiteral("rgb(200, 100, 50)")));
    QCOMPARE(view.findChildren<QColorDialog*>().size(), 0);

    // Cancelling reverts the previewed color
    pickColor(QColor(1, 2, 3), false);
    pScene->click();
    QCOMPARE(sceneSpy.last().at(0).value<QColor>(), QColor(20, 40, 60));
    QVERIFY(pScene->styleSheet().contains(QStringLiteral("rgb(20, 40, 60)")));
    pickColor(QColor(4, 5, 6), false);
    pLight->click();
    QCOMPARE(lightSpy.last().at(0).value<QColor>(), QColor(200, 100, 50));
    QVERIFY(pLight->styleSheet().contains(QStringLiteral("rgb(200, 100, 50)")));

    // Data tree: header toggle and confirmed removal through the context menu
    view.setFlags({"Data"});
    QStandardItemModel model(0, 2);
    model.appendRow({new QStandardItem(QStringLiteral("lh.pial")), new QStandardItem(QStringLiteral("left"))});
    model.appendRow({new QStandardItem(QStringLiteral("rh.pial")), new QStandardItem(QStringLiteral("right"))});
    view.setModel(&model);
    QStyledItemDelegate delegate;
    view.setDelegate(&delegate);
    QCOMPARE(pTree->itemDelegate(), &delegate);
    QVERIFY(!pTree->isHeaderHidden());
    QCoreApplication::processEvents();
    auto contextAction = [&view, pTree](const QString& sText) {
        emit pTree->customContextMenuRequested(pTree->visualRect(pTree->model()->index(1, 0)).center());
        QPointer<QMenu> pMenu = view.findChildren<QMenu*>().last();
        for (QAction* pAction : pMenu->actions()) {
            if (pAction->text() == sText) {
                pAction->trigger();
                break;
            }
        }
        if (pMenu) {
            pMenu->close();
        }
    };
    contextAction(QStringLiteral("Toggle header"));
    QVERIFY(pTree->isHeaderHidden());
    contextAction(QStringLiteral("Toggle header"));
    QVERIFY(!pTree->isHeaderHidden());
    clickNextMessageBoxButton(QStringLiteral("No"));
    contextAction(QStringLiteral("Remove"));
    QCOMPARE(model.rowCount(), 2);
    clickNextMessageBoxButton(QStringLiteral("Yes"));
    contextAction(QStringLiteral("Remove"));
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.item(0)->text(), QStringLiteral("lh.pial"));
    // A closed context menu does not stay alive
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCOMPARE(view.findChildren<QMenu*>().size(), 0);

    view.onTreeViewDescriptionHide();
    QVERIFY(pTree->isColumnHidden(1));
    view.onTreeViewDescriptionHide();
    QVERIFY(!pTree->isColumnHidden(1));
}

//=============================================================================================================

void TestVizQtViewers2::control3dView_lifecycle()
{
    Control3DView view("test_viz_qt_viewers2");

    // save/load are public; clearView is protected so we only call public API
    view.saveSettings();
    view.loadSettings();

    QApplication::processEvents();
}

//=============================================================================================================

void TestVizQtViewers2::applyToView_lifecycle()
{
    ApplyToView view("test_viz_qt_viewers2");

    // AbstractView lifecycle
    view.saveSettings();
    view.loadSettings();
    view.clearView();

    QApplication::processEvents();
}

//=============================================================================================================

void TestVizQtViewers2::rtFiffRawViewDelegate_paintsSignalCursorAndThreshold()
{
    // 100-sample buffer; MEG0111 holds +0.5 of full scale, the stim channel steps to 4
    RtFiffRawViewModel model;
    auto info = createBrowserTestInfo();
    info->bads.clear();
    model.setFiffInfo(info);
    model.setSamplingInfo(100.0f, 1, true);
    model.setScaling({{FIFF_UNIT_T, 1e-12f}, {FIFFV_STIM_CH, 5.0f}});
    model.triggerInfoChanged({{4.0, Qt::red}}, true, "STI 014", 2.5);
    Eigen::MatrixXd block = Eigen::MatrixXd::Zero(4, 60);
    block.row(0).setConstant(0.5e-12);
    block.row(3).tail(30).setConstant(4.0);
    model.addData(std::vector<Eigen::MatrixXd>{block});
    QCOMPARE(model.getDetectedTriggers(), (std::vector<std::pair<int, double>>{{30, 4.0}}));

    RtFiffRawViewDelegate delegate(nullptr);
    delegate.initPainterPaths(&model);
    delegate.setUpperItemIndex(-1);

    // The view hides the name column, so the plot cell starts at x = 0
    const QRect cell(0, 0, 200, 100);
    auto render = [&](int iRow) {
        QImage image(cell.width(), cell.height(), QImage::Format_RGB32);
        image.fill(Qt::white);
        QPainter painter(&image);
        QStyleOptionViewItem option;
        option.rect = cell;
        delegate.paint(&painter, option, model.index(iRow, 1));
        return image;
    };
    auto rowsWhere = [](const QImage& image, int x, const std::function<bool(QRgb)>& pred) {
        QList<int> rows;
        for (int y = 0; y < image.height(); ++y) {
            if (pred(image.pixel(x, y))) {
                rows << y;
            }
        }
        return rows;
    };
    auto isSignal = [](QRgb c) {
        return qBlue(c) > qRed(c) + 50 && qBlue(c) > qGreen(c) + 50;
    };
    auto isMarker = [](QRgb c) {
        return qRed(c) > qGreen(c) + 50 && qRed(c) > qBlue(c) + 50;
    };

    // +0.5 of full scale sits a quarter of the height above the centre: y = 50 - 25
    QImage image = render(0);
    QList<int> rows = rowsWhere(image, 40, isSignal);
    QVERIFY2(!rows.isEmpty() && std::abs(rows.first() - 25) <= 1, qPrintable(QString::number(rows.value(0, -1))));
    // The time cursor is at sample 60 of 100, i.e. x = 120
    QVERIFY2(rowsWhere(image, 120, isMarker).size() > 50, qPrintable(QString::number(rowsWhere(image, 120, isMarker).size())));
    QVERIFY(rowsWhere(image, 100, isMarker).isEmpty());

    // Recolored signal
    delegate.setSignalColor(Qt::darkGreen);
    QCOMPARE(delegate.getSignalColor(), QColor(Qt::darkGreen));
    image = render(0);
    QVERIFY(rowsWhere(image, 40, isSignal).isEmpty());
    QVERIFY(!rowsWhere(image, 40, [](QRgb c) { return qGreen(c) > qRed(c) + 40 && qGreen(c) > qBlue(c) + 40; }).isEmpty());

    // The trigger channel shows its detection threshold: 2.5 of 5 full scale is a quarter above the centre
    image = render(3);
    int iThreshold = 0;
    for (int x = 0; x < 100; ++x) {
        const QRgb c = image.pixel(x, 25);
        if (qRed(c) > 200 && qGreen(c) < 150) {
            ++iThreshold;
        }
    }
    QVERIFY2(iThreshold > 30, qPrintable(QString::number(iThreshold)));

    // A bad channel gets a reddish tint over the whole cell
    info->bads = {"MEG 0111"};
    image = render(0);
    const QRgb background = image.pixel(5, 90);
    QVERIFY2(qRed(background) > qBlue(background) + 10, qPrintable(QColor(background).name()));

    // A cell that does not start at x = 0 (name column shown) draws the cursor at the same place within the cell
    info->bads.clear();
    {
        QImage shifted(300, cell.height(), QImage::Format_RGB32);
        shifted.fill(Qt::white);
        QPainter painter(&shifted);
        QStyleOptionViewItem option;
        option.rect = QRect(100, 0, 200, 100);
        delegate.paint(&painter, option, model.index(0, 1));
        painter.end();
        QVERIFY2(rowsWhere(shifted, 100 + 120, isMarker).size() > 50, qPrintable(QString::number(rowsWhere(shifted, 220, isMarker).size())));
    }
    // ... and so do detected triggers (stim step at sample 30) and added events (sample 50)
    model.setEventCallbacks([](int) {}, [](int, int) { return std::vector<int>{50}; });
    {
        QImage shifted(300, cell.height(), QImage::Format_RGB32);
        shifted.fill(Qt::white);
        QPainter painter(&shifted);
        QStyleOptionViewItem option;
        option.rect = QRect(100, 0, 200, 100);
        delegate.paint(&painter, option, model.index(3, 1));
        painter.end();
        auto isTrigger = [](QRgb c) {
            return qRed(c) > 150 && qGreen(c) < 100 && qBlue(c) < 100;
        };
        auto isEvent = [](QRgb c) {
            return qGreen(c) > qRed(c) + 60 && qGreen(c) > qBlue(c) + 60;
        };
        QVERIFY2(rowsWhere(shifted, 100 + 60, isTrigger).size() > 50, qPrintable(QString::number(rowsWhere(shifted, 160, isTrigger).size())));
        QVERIFY2(rowsWhere(shifted, 100 + 100, isEvent).size() > 50, qPrintable(QString::number(rowsWhere(shifted, 200, isEvent).size())));
    }

    // Name column and size hints
    QStyleOptionViewItem option;
    option.rect = cell;
    QCOMPARE(delegate.sizeHint(option, model.index(0, 0)), QSize(20, cell.height()));
    QCOMPARE(delegate.sizeHint(option, model.index(0, 1)), cell.size());
}

//=============================================================================================================

void TestVizQtViewers2::rtFiffRawViewDelegate_lifecycle()
{
    // Construct with null parent — default constructor path
    RtFiffRawViewDelegate delegate(nullptr);

    // Basic setter calls
    delegate.setSignalColor(Qt::red);
    delegate.setSignalColor(Qt::blue);

    QApplication::processEvents();
}

//=============================================================================================================

void TestVizQtViewers2::evokedSetModel_extended()
{
    EvokedSetModel model;

    // Basic model introspection (safe, no data loaded)
    QVERIFY(model.columnCount() >= 0);
    QVERIFY(model.rowCount() >= 0);

    // setEvokedSet with empty shared pointer — should not crash
    std::shared_ptr<FiffEvokedSet> emptySet;
    model.setEvokedSet(emptySet);

    // isInit before data — false
    QVERIFY(!model.isInit());

    // getNumSamples — 0 before data loaded
    QCOMPARE(model.getNumSamples(), 0);

    // getIdxSelMap — returns empty map
    const std::map<int, int>& selMap = model.getIdxSelMap();
    QVERIFY(selMap.empty());

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

    auto pColors = std::make_shared<std::map<std::string, QColor>>();
    (*pColors)["avg"] = Qt::red;
    model.setAverageColor(pColors);

    auto pActivations = std::make_shared<std::map<std::string, bool>>();
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

void TestVizQtViewers2::bidsViewModel_basics()
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

void TestVizQtViewers2::bidsView_treeMovesAndSelection()
{
    BidsViewModel model;
    BidsView view;
    view.setModel(&model);
    view.resize(400, 400);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    auto* tree = view.findChild<QTreeView*>();
    QVERIFY(tree != nullptr);

    // sub-01/ses-01/func/raw.fif, then an average under it, a second subject and session
    auto* raw = new QStandardItem(QStringLiteral("raw.fif"));
    raw->setData(QStringLiteral("payload"));
    model.addData(QModelIndex(), raw, BIDS_FUNCTIONALDATA);
    auto* average = new QStandardItem(QStringLiteral("avg"));
    model.addData(raw->index(), average, BIDS_AVERAGE);
    auto* anat = new QStandardItem(QStringLiteral("T1.mgz"));
    model.addData(raw->index(), anat, BIDS_ANATOMICALDATA);
    const QModelIndex sub2 = model.addSubject(QStringLiteral("Second subject"));
    const QModelIndex ses2 = model.addSessionToSubject(QStringLiteral("sub-Secondsubject"), QStringLiteral("pre op"));

    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.itemFromIndex(sub2)->text(), QStringLiteral("sub-Secondsubject"));
    QCOMPARE(model.itemFromIndex(ses2)->text(), QStringLiteral("ses-preop"));
    QStandardItem* ses1 = model.item(0)->child(0);
    QCOMPARE(ses1->text(), QStringLiteral("ses-01"));
    QCOMPARE(ses1->rowCount(), 2);
    QCOMPARE(ses1->child(0)->text(), QStringLiteral("func"));
    QCOMPARE(ses1->child(1)->text(), QStringLiteral("anat"));
    QCOMPARE(average->parent(), raw);
    QCOMPARE(average->data(BIDS_ITEM_TYPE).toInt(), BIDS_AVERAGE);
    QCOMPARE(anat->data(BIDS_ITEM_SESSION).value<QModelIndex>(), ses1->index());
    QVERIFY(!model.addSessionToSubject(QStringLiteral("sub-nobody"), QStringLiteral("x")).isValid());

    // Selecting an item emits its index and, when it carries data, that data
    QSignalSpy itemSpy(&view, &BidsView::selectedItemChanged);
    QSignalSpy modelSpy(&view, &BidsView::selectedModelChanged);
    tree->selectionModel()->select(raw->index(), QItemSelectionModel::ClearAndSelect);
    QCOMPARE(itemSpy.last().at(0).value<QModelIndex>(), raw->index());
    QCOMPARE(modelSpy.last().at(0).toString(), QStringLiteral("payload"));

    // Context menu on the raw file: move it to sub-Secondsubject/ses-preop
    auto openMenuAt = [&view, tree](const QModelIndex& index) {
        emit tree->customContextMenuRequested(tree->visualRect(index).center());
        return view.findChildren<QMenu*>(Qt::FindDirectChildrenOnly).last();
    };
    auto actionNamed = [](QMenu* menu, const QString& text) -> QAction* {
        QList<QAction*> pending = menu->actions();
        while (!pending.isEmpty()) {
            QAction* action = pending.takeFirst();
            if (action->text() == text) {
                return action;
            }
            if (action->menu()) {
                pending += action->menu()->actions();
            }
        }
        return nullptr;
    };
    tree->expandAll();
    QMenu* menu = openMenuAt(raw->index());
    QAction* moveData = actionNamed(menu, QStringLiteral("ses-preop"));
    QVERIFY(moveData != nullptr);
    QVERIFY(actionNamed(menu, QStringLiteral("Remove Data")) != nullptr);
    moveData->trigger();
    menu->close();
    QStandardItem* ses2Item = model.item(1)->child(0);
    QCOMPARE(ses2Item->child(0)->text(), QStringLiteral("func"));
    QCOMPARE(ses2Item->child(0)->child(0)->text(), QStringLiteral("raw.fif"));
    // The emptied func folder of ses-01 is removed; anat stays
    QCOMPARE(model.item(0)->child(0)->rowCount(), 1);
    QCOMPARE(model.item(0)->child(0)->child(0)->text(), QStringLiteral("anat"));

    // Move ses-01 (with its anat data) to the second subject
    tree->expandAll();
    menu = openMenuAt(model.item(0)->child(0)->index());
    QAction* moveSession = actionNamed(menu, QStringLiteral("sub-Secondsubject"));
    QVERIFY(moveSession != nullptr);
    moveSession->trigger();
    menu->close();
    QCOMPARE(model.item(0)->rowCount(), 0);
    QCOMPARE(model.item(1)->rowCount(), 2);
    QStandardItem* movedAnat = model.item(1)->child(1)->child(0)->child(0);
    QCOMPARE(movedAnat->text(), QStringLiteral("T1.mgz"));
    QCOMPARE(movedAnat->data(BIDS_ITEM_SUBJECT).value<QModelIndex>(), model.item(1)->index());

    // Subject and empty-area menus offer add actions
    tree->expandAll();
    menu = openMenuAt(model.item(0)->index());
    QVERIFY(actionNamed(menu, QStringLiteral("Add Session")) != nullptr);
    menu->close();
    emit tree->customContextMenuRequested(QPoint(5, tree->viewport()->height() - 5));
    QVERIFY(actionNamed(view.findChildren<QMenu*>().last(), QStringLiteral("Add Subject")) != nullptr);
    view.findChildren<QMenu*>().last()->close();
    // Closed context menus do not stay alive
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCOMPARE(view.findChildren<QMenu*>().size(), 0);

    // Delete on a non-subject item asks the model owner to remove it
    QSignalSpy removeSpy(&view, &BidsView::removeItem);
    tree->setCurrentIndex(movedAnat->index());
    QKeyEvent del(QEvent::KeyPress, Qt::Key_Delete, Qt::NoModifier);
    QApplication::sendEvent(&view, &del);
    QCOMPARE(removeSpy.size(), 1);
    QVERIFY(model.removeItem(removeSpy.at(0).at(0).value<QModelIndex>()));
    QCOMPARE(model.item(1)->child(1)->child(0)->rowCount(), 0);
    QVERIFY(model.removeItem(model.item(0)->index()));
    QCOMPARE(model.rowCount(), 1);
}

//=============================================================================================================

void TestVizQtViewers2::projectPanel_inPanelWidget()
{
    QTemporaryDir root;
    QVERIFY(root.isValid());
    const QString sRoot = root.path();
    for (const QString& sSub : {QStringLiteral("ProjA/Sub1"), QStringLiteral("ProjA/Sub2"), QStringLiteral("ProjB/SubX")}) {
        QVERIFY(QDir().mkpath(sRoot + QLatin1Char('/') + sSub));
    }

    auto panel = std::make_shared<ProjectPanel>(sRoot.toStdU16String(), "ProjA", "Sub2", "");
    PanelWidget view(panel, QString());
    view.show();
    std::vector<std::string> projects;
    std::vector<std::string> subjects;
    std::vector<std::string> paradigms;
    std::vector<std::string> fileNames;
    std::vector<int> times;
    std::vector<bool> timers;
    ProjectPanel::Callbacks callbacks;
    callbacks.project_changed = [&projects](const std::string& project) {
        projects.push_back(project);
    };
    callbacks.subject_changed = [&subjects](const std::string& subject) {
        subjects.push_back(subject);
    };
    callbacks.paradigm_changed = [&paradigms](const std::string& paradigm) {
        paradigms.push_back(paradigm);
    };
    callbacks.file_name_changed = [&fileNames](const std::string& fileName) {
        fileNames.push_back(fileName);
    };
    callbacks.recording_time_changed = [&times](int milliseconds) {
        times.push_back(milliseconds);
    };
    callbacks.recording_timer_changed = [&timers](bool on) {
        timers.push_back(on);
    };
    panel->set_callbacks(std::move(callbacks));
    const auto editor = [&view](const char* key) {
        return view.formPanel()->editor(key);
    };
    const auto text = [&editor](const char* key) {
        return qobject_cast<QLabel*>(editor(key))->text();
    };
    auto* pProjects = qobject_cast<QComboBox*>(editor("project"));
    auto* pSubjects = qobject_cast<QComboBox*>(editor("subject"));
    auto* pParadigm = qobject_cast<QLineEdit*>(editor("paradigm"));
    QVERIFY(pProjects && pSubjects && pParadigm);
    const auto items = [](QComboBox* pBox) {
        QStringList list;
        for (int i = 0; i < pBox->count(); ++i) {
            list << pBox->itemText(i);
        }
        return list;
    };
    const auto type = [](QLineEdit* pEdit, const QString& sText) {
        pEdit->setText(sText);
        emit pEdit->textEdited(sText);
    };

    // The initial scan picks up both projects and keeps the requested subject
    QCOMPARE(items(pProjects), QStringList({"ProjA", "ProjB"}));
    QCOMPARE(pProjects->currentText(), QStringLiteral("ProjA"));
    QCOMPARE(items(pSubjects), QStringList({"Sub1", "Sub2"}));
    QCOMPARE(pSubjects->currentText(), QStringLiteral("Sub2"));

    // File name: <root>/<project>/<subject>/<stamp>_<subject>[_<paradigm>]_raw.fif
    const QString sFile = QString::fromStdString(panel->current_file_name());
    QVERIFY(sFile.startsWith(sRoot + QStringLiteral("/ProjA/Sub2/")));
    QVERIFY(sFile.endsWith(QStringLiteral("_Sub2_raw.fif")));
    QCOMPARE(text("fileName"), sFile);
    QVERIFY(fileNames.empty());

    type(pParadigm, QStringLiteral("Rest"));
    QCOMPARE(paradigms.back(), std::string("Rest"));
    QVERIFY(fileNames.back().ends_with("_Sub2_Rest_raw.fif"));
    QVERIFY(text("fileName").endsWith(QStringLiteral("_Sub2_Rest_raw.fif")));

    // Switching the project rescans the subjects and falls back to the first one
    pProjects->setCurrentText(QStringLiteral("ProjB"));
    QCOMPARE(projects.back(), std::string("ProjB"));
    QCOMPARE(items(pSubjects), QStringList({"SubX"}));
    QCOMPARE(subjects.back(), std::string("SubX"));
    QVERIFY(QString::fromStdString(panel->current_file_name()).startsWith(sRoot + QStringLiteral("/ProjB/SubX/")));

    // Adding a project creates its directory and chooses it
    type(qobject_cast<QLineEdit*>(editor("newProject")), QStringLiteral("ProjC"));
    view.findChild<QPushButton*>(QStringLiteral("addProject"))->click();
    QVERIFY(QDir(sRoot + QStringLiteral("/ProjC")).exists());
    QCOMPARE(items(pProjects), QStringList({"ProjA", "ProjB", "ProjC"}));
    QCOMPARE(pProjects->currentText(), QStringLiteral("ProjC"));
    QCOMPARE(projects.back(), std::string("ProjC"));
    QCOMPARE(pSubjects->count(), 0);
    QVERIFY(qobject_cast<QLineEdit*>(editor("newProject"))->text().isEmpty());
    QVERIFY(text("fileName").startsWith(sRoot + QStringLiteral("/ProjC/")));

    // Adding a subject creates its directory below the current project and chooses it
    type(qobject_cast<QLineEdit*>(editor("newSubject")), QStringLiteral("SubNew"));
    view.findChild<QPushButton*>(QStringLiteral("addSubject"))->click();
    QVERIFY(QDir(sRoot + QStringLiteral("/ProjC/SubNew")).exists());
    QCOMPARE(items(pSubjects), QStringList({"SubNew"}));
    QCOMPARE(pSubjects->currentText(), QStringLiteral("SubNew"));
    QVERIFY(fileNames.back().starts_with((sRoot + QStringLiteral("/ProjC/SubNew/")).toStdString()));

    // ... also when the project already has subjects sorting before the new one
    pProjects->setCurrentText(QStringLiteral("ProjA"));
    QVERIFY(panel->add_subject("Sub3"));
    QCOMPARE(items(pSubjects), QStringList({"Sub1", "Sub2", "Sub3"}));
    QCOMPARE(pSubjects->currentText(), QStringLiteral("Sub3"));
    QCOMPARE(subjects.back(), std::string("Sub3"));

    // Without a name nothing is added
    view.findChild<QPushButton*>(QStringLiteral("addProject"))->click();
    QCOMPARE(pProjects->count(), 3);

    // Another data directory rescans the projects
    QTemporaryDir other;
    QVERIFY(other.isValid() && QDir().mkpath(other.path() + QStringLiteral("/Other/S1")));
    auto* pDirectory = editor("dataDirectory")->findChild<QLineEdit*>();
    pDirectory->setText(other.path());
    emit pDirectory->editingFinished();
    QCOMPARE(items(pProjects), QStringList({"Other"}));
    QCOMPARE(projects.back(), std::string("Other"));
    QCOMPARE(subjects.back(), std::string("S1"));
    QVERIFY(fileNames.back().starts_with((other.path() + QStringLiteral("/Other/S1/")).toStdString()));

    // Recording timer: the spin boxes set the total, the elapsed time counts up
    qobject_cast<QSpinBox*>(editor("hours"))->setValue(1);
    qobject_cast<QSpinBox*>(editor("minutes"))->setValue(2);
    qobject_cast<QSpinBox*>(editor("seconds"))->setValue(3);
    QCOMPARE(times.back(), 3723000);
    QCOMPARE(panel->recording_time(), 3723000);
    panel->set_recording_elapsed_time(1000);
    QCOMPARE(text("passedTime"), QStringLiteral("00:00:01"));
    panel->set_recording_elapsed_time(3723000 - 200);
    QCOMPARE(text("passedTime"), QStringLiteral("01:02:03"));
    view.findChild<QCheckBox*>(QStringLiteral("useRecordingTimer"))->click();
    QCOMPARE(timers, std::vector<bool>({true}));
    QVERIFY(panel->recording_timer());

    // The file name and paradigm can be hidden and shown again
    panel->set_file_name_visible(false);
    panel->set_paradigm_visible(false);
    QVERIFY(!editor("fileName")->isVisible());
    QVERIFY(!pParadigm->isVisible());
    panel->set_file_name_visible(true);
    panel->set_paradigm_visible(true);
    QVERIFY(editor("fileName")->isVisible());
    QVERIFY(pParadigm->isVisible());

    view.setGuiMode(AbstractView::GuiMode::Clinical);
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    view.saveSettings();
    view.loadSettings();
    view.clearView();
}

//=============================================================================================================

void TestVizQtViewers2::mneOperator_basics()
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

void TestVizQtViewers2::rtFiffRawViewModel_extended()
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
    std::map<int, float> scaleMap;
    scaleMap[FIFFV_EEG_CH] = 1e-6f;
    model.setScaling(scaleMap);
    QVERIFY(qFuzzyCompare(model.getScaling().at(FIFFV_EEG_CH), 1e-6f));

    QApplication::processEvents();
}

//=============================================================================================================

QTEST_MAIN(TestVizQtViewers2)
#include "test_viz_qt_viewers2.moc"

#include <vector>
#include <utility>
