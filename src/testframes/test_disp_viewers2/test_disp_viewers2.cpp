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
#include <disp/viewers/filterdesignview.h>
#include <disp/viewers/triggerdetectionview.h>
#include <disp/viewers/averagingsettingsview.h>
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
#include <disp/viewers/helpers/selectionsceneitem.h>
#include <disp/viewers/helpers/averagesceneitem.h>
#include <disp/viewers/helpers/channeldatamodel.h>
#include <disp/viewers/helpers/layoutscene.h>

#include <fiff/fiff_info.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_dig_point.h>
#include <fiff/fiff_named_matrix.h>
#include <fiff/fiff_proj.h>
#include <dsp/filterkernel.h>
#include <dsp/filterio.h>

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
#include <QDir>
#include <QHeaderView>
#include <QMenu>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsSceneMouseEvent>
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
#include <QInputDialog>
#include <QMessageBox>
#include <QColorDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QLocale>
#include <QRadioButton>
#include <QVector3D>
#include <QTimer>

#include <functional>
#include <QPointer>
#include <QTreeView>
#include <QStyledItemDelegate>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QKeyEvent>

#include <Eigen/Core>

#include <rhi/qrhi.h>

#include <memory>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace FIFFLIB;
using UTILSLIB::FilterKernel;
using UTILSLIB::FilterIO;
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

/** Selects sPath in the next modal QFileDialog and accepts it. */
void chooseFileInNextDialog(const QString& sPath)
{
    answerNextModal([sPath](QWidget* pModal) {
        auto* pDialog = qobject_cast<QFileDialog*>(pModal);
        QVERIFY(pDialog);
        // Typing the name works for open dialogs too, whose selectFile() is dropped for an existing file
        auto* pName = pDialog->findChild<QLineEdit*>(QStringLiteral("fileNameEdit"));
        QVERIFY(pName);
        pDialog->setDirectory(QFileInfo(sPath).absolutePath());
        pName->setText(QFileInfo(sPath).fileName());
        static_cast<QDialog*>(pDialog)->accept();
    });
}

/** Enters sText into the next modal QInputDialog and accepts it. */
void answerNextInputDialog(const QString& sText)
{
    answerNextModal([sText](QWidget* pModal) {
        auto* pDialog = qobject_cast<QInputDialog*>(pModal);
        QVERIFY(pDialog);
        pDialog->setTextValue(sText);
        pDialog->accept();
    });
}

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
    void hpiSettingsView_coilTablesPresetsAndFeedback();

    //=========================================================================================================
    /**
     * Verifies that CoregSettingsView constructs, exposes getter defaults, and survives
     * lifecycle transitions.
     */
    void coregSettingsView_lifecycle();

    //=========================================================================================================
    /**
     * Verifies CoregSettingsView fiducial picking, transformation parameters, scaling modes and signals.
     */
    void coregSettingsView_fiducialsAndParams();

    //=========================================================================================================
    /**
     * Verifies FilterDesignView filter design from the controls, band clamping, channel type, plot and filter file
     * export/import.
     */
    void filterDesignView_designExportLoad();

    //=========================================================================================================
    /**
     * Verifies that ArtifactSettingsView constructs and survives all lifecycle transitions.
     */
    void artifactSettingsView_lifecycle();

    //=========================================================================================================
    /**
     * Verifies ArtifactSettingsView rows per channel type, threshold composition, activation and rebuilds.
     */
    void artifactSettingsView_thresholds();

    //=========================================================================================================
    /**
     * Verifies TriggerDetectionView channel list, threshold composition, detected types, colors, reset and the
     * offline detect button, also after a re-init.
     */
    void triggerDetectionView_settingsAndSignals();

    //=========================================================================================================
    /**
     * Verifies AveragingSettingsView defaults, stim channel selection, epoch window and baseline controls, saved
     * settings and the detected-epoch table.
     */
    void averagingSettingsView_controlsAndSettings();

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
    void butterflyView_drawsAveragesAcrossTheFullWidth();

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
    void averageLayoutView_drawsEachChannelsAverage();
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
     * Verifies LayoutScene wheel zoom, double-click fit, right-drag panning and rubber-band selection.
     */
    void layoutScene_zoomPanAndSelect();
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
     * Verifies that DipoleFitView constructs headlessly, exposes getters,
     * and survives lifecycle transitions.
     */
    void dipoleFitView_lifecycle();

    //=========================================================================================================
    /**
     * Verifies DipoleFitView parameter signals, the requested parameter set, model lists and the fit name.
     */
    void dipoleFitView_paramsModelsAndFit();

    //=========================================================================================================
    /**
     * Verifies AverageSceneItem drawing: stim and zero line, scaling and clamping, per-average color and
     * activation, bad-channel opacity and the press/release label size.
     */
    void averageSceneItem_paintsScaledClampedCurves();

    //=========================================================================================================
    /**
     * Verifies FrequencySpectrumDelegate drawing: the spectrum curve rises with power, spans the frequency
     * bounds, frequency grid labels, and the mouse read-out of frequency and value.
     */
    void frequencySpectrumDelegate_paintsSpectrumAndReadout();

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
    void spectrumView_scalesBoundsAndPaints();
    void frequencySpectrumDelegate_basics();

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
     * Verifies ProjectSettingsView project/subject scanning, adding, deleting, file naming and timer.
     */
    void projectSettingsView_projectsSubjectsAndTimer();

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

void TestDispViewers2::hpiSettingsView_coilTablesPresetsAndFeedback()
{
    // An empty settings path keeps the user's stored coil frequencies out of the test
    HpiSettingsView view(QString{});
    QSignalSpy freqSpy(&view, &HpiSettingsView::coilFrequenciesChanged);
    auto* freqTable = view.findChild<QTableWidget*>(QStringLiteral("m_tableWidget_Frequencies"));
    auto* resultTable = view.findChild<QTableWidget*>(QStringLiteral("m_tableWidget_results"));
    QVERIFY(freqTable && resultTable);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile presets(dir.filePath(QStringLiteral("presets.json")));
    QVERIFY(presets.open(QIODevice::WriteOnly));
    presets.write(R"({"4": [{"name": "VectorView", "coils": [154, 158, 162, 166]}]})");
    presets.close();
    view.loadCoilPresets(presets.fileName());

    // 4 HPI coils, 3 fiducials, 2 EEG and 1 extra point
    QList<FiffDigPoint> points;
    const QVector<int> kinds = {FIFFV_POINT_CARDINAL, FIFFV_POINT_CARDINAL, FIFFV_POINT_CARDINAL, FIFFV_POINT_HPI,
                                FIFFV_POINT_HPI, FIFFV_POINT_HPI, FIFFV_POINT_HPI, FIFFV_POINT_EEG,
                                FIFFV_POINT_EEG, FIFFV_POINT_EXTRA};
    for (int i = 0; i < kinds.size(); ++i) {
        FiffDigPoint p;
        p.kind = kinds[i];
        p.ident = i + 1;
        points.append(p);
    }
    view.newDigitizerList(points);
    auto label = [&view](const char* name) {
        return view.findChild<QLabel*>(QLatin1String(name))->text();
    };
    QCOMPARE(label("m_label_numberLoadedCoils"), QStringLiteral("4"));
    QCOMPARE(label("m_label_numberLoadedFiducials"), QStringLiteral("3"));
    QCOMPARE(label("m_label_numberLoadedEEG"), QStringLiteral("2"));
    QCOMPARE(label("m_label_numberLoadedAdditional"), QStringLiteral("1"));
    QCOMPARE(freqTable->rowCount(), 4);
    QCOMPARE(resultTable->rowCount(), 4);
    QCOMPARE(freqSpy.last().at(0).value<QVector<int>>(), (QVector<int>{293, 307, 314, 321}));

    // Choosing the preset for 4 coils fills the table
    auto* preset = view.findChild<QComboBox*>(QStringLiteral("comboBox_coilPreset"));
    QCOMPARE(preset->count(), 2);
    preset->setCurrentIndex(1);
    QCOMPARE(freqSpy.last().at(0).value<QVector<int>>(), (QVector<int>{154, 158, 162, 166}));
    QCOMPARE(freqTable->item(3, 1)->text(), QStringLiteral("166"));

    // Editing a frequency cell, and "none" for an unused coil
    freqTable->item(0, 1)->setText(QStringLiteral("300"));
    freqTable->item(2, 1)->setText(QStringLiteral("none"));
    QCOMPARE(freqSpy.last().at(0).value<QVector<int>>(), (QVector<int>{300, 158, -1, 166}));

    // Removing coil 2 renumbers both tables; adding one back appends an unset frequency
    freqTable->setCurrentCell(1, 1);
    QTest::mouseClick(view.findChild<QPushButton*>(QStringLiteral("m_pushButton_removeCoil")), Qt::LeftButton);
    QCOMPARE(freqSpy.last().at(0).value<QVector<int>>(), (QVector<int>{300, -1, 166}));
    QCOMPARE(freqTable->rowCount(), 3);
    QCOMPARE(freqTable->item(2, 0)->text(), QStringLiteral("3"));
    QCOMPARE(resultTable->item(2, 0)->text(), QStringLiteral("3"));
    QTest::mouseClick(view.findChild<QPushButton*>(QStringLiteral("m_pushButton_addCoil")), Qt::LeftButton);
    QCOMPARE(freqTable->rowCount(), 4);
    QCOMPARE(freqTable->item(3, 1)->text(), QStringLiteral("none"));
    QCOMPARE(freqSpy.last().at(0).value<QVector<int>>(), (QVector<int>{300, -1, 166, -1}));

    // Fit results: errors in mm, GoF in %, good/bad against the allowed mean error
    view.findChild<QDoubleSpinBox*>(QStringLiteral("m_doubleSpinBox_maxHPIContinousDist"))->setValue(3.0);
    view.setErrorLabels({0.0011, 0.0024, 0.0005, 0.0039}, 0.002);
    QCOMPARE(resultTable->item(1, 1)->text(), QStringLiteral("2.40 mm"));
    QCOMPARE(label("m_label_averagedFitError"), QStringLiteral("2.00 mm"));
    QCOMPARE(label("m_label_fitFeedback"), QStringLiteral("Last fit: Good"));
    view.setErrorLabels({0.004}, 0.0045);
    QCOMPARE(label("m_label_fitFeedback"), QStringLiteral("Last fit: Bad"));
    Eigen::VectorXd gof(4);
    gof << 0.991, 0.95, 0.9, 0.875;
    view.setGoFLabels(gof, 0.929);
    QCOMPARE(resultTable->item(3, 2)->text(), QStringLiteral("87.50 %"));
    QCOMPARE(label("m_average_gof_set"), QStringLiteral("92.90 %"));

    view.findChild<QDoubleSpinBox*>(QStringLiteral("m_doubleSpinBox_moveThreshold"))->setValue(3.0);
    view.findChild<QDoubleSpinBox*>(QStringLiteral("m_doubleSpinBox_rotThreshold"))->setValue(5.0);
    QCOMPARE(view.getAllowedMovementChanged(), 3.0);
    QCOMPARE(view.getAllowedRotationChanged(), 5.0);
    view.setMovementResults(0.002, 4.0);
    QCOMPARE(view.findChild<QLineEdit*>(QStringLiteral("m_qLineEdit_moveResult"))->text(), QStringLiteral("2.00 mm"));
    QCOMPARE(label("m_label_movementFeedback"), QStringLiteral("Small"));
    view.setMovementResults(0.002, 6.0);
    QCOMPARE(label("m_label_movementFeedback"), QStringLiteral("Big"));
    view.setMovementResults(0.0031, 1.0);
    QCOMPARE(label("m_label_movementFeedback"), QStringLiteral("Big"));

    // Check boxes and spin boxes report through their signals
    QSignalSpy sspSpy(&view, &HpiSettingsView::sspStatusChanged);
    QSignalSpy contSpy(&view, &HpiSettingsView::contHpiStatusChanged);
    QSignalSpy windowSpy(&view, &HpiSettingsView::fittingWindowSizeChanged);
    QTest::mouseClick(view.findChild<QCheckBox*>(QStringLiteral("m_checkBox_useSSP")), Qt::LeftButton);
    QTest::mouseClick(view.findChild<QCheckBox*>(QStringLiteral("m_checkBox_continousHPI")), Qt::LeftButton);
    view.findChild<QSpinBox*>(QStringLiteral("m_spinBox_samplesToFit"))->setValue(450);
    QCOMPARE(sspSpy.size(), 1);
    QVERIFY(view.getSspStatusChanged());
    QVERIFY(view.continuousHPIChecked());
    QCOMPARE(contSpy.size(), 1);
    QCOMPARE(windowSpy.last().at(0).toInt(), 450);
    QCOMPARE(view.getFittingWindowSize(), 450);
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

void TestDispViewers2::filterDesignView_designExportLoad()
{
    FilterDesignView view(QString{});
    view.resize(600, 400);
    view.show();
    auto* pFrom = view.findChild<QDoubleSpinBox*>(QStringLiteral("m_doubleSpinBox_from"));
    auto* pTo = view.findChild<QDoubleSpinBox*>(QStringLiteral("m_doubleSpinBox_to"));
    auto* pTrans = view.findChild<QDoubleSpinBox*>(QStringLiteral("m_doubleSpinBox_transitionband"));
    auto* pTaps = view.findChild<QSpinBox*>(QStringLiteral("m_spinBox_filterTaps"));
    auto* pMethod = view.findChild<QComboBox*>(QStringLiteral("m_comboBox_designMethod"));
    auto* pApply = view.findChild<QComboBox*>(QStringLiteral("m_comboBox_filterApplyTo"));
    QVERIFY(pFrom);
    QVERIFY(pTo);
    QVERIFY(pTrans);
    QVERIFY(pTaps);
    QVERIFY(pMethod);
    QVERIFY(pApply);

    // A band-pass from the controls, with the taps forced even and the band edges at the requested frequencies
    QSignalSpy filterSpy(&view, &FilterDesignView::filterChanged);
    QSignalSpy fromSpy(&view, &FilterDesignView::updateFilterFrom);
    QSignalSpy toSpy(&view, &FilterDesignView::updateFilterTo);
    view.setSamplingRate(1000.0);
    view.setMaxAllowedFilterTaps(257);
    QCOMPARE(pTaps->maximum(), 256);
    pTaps->setValue(128);
    view.setFrom(2.0);
    view.setTo(45.0);
    QCOMPARE(view.getFrom(), 2.0);
    QCOMPARE(view.getTo(), 45.0);
    QCOMPARE(fromSpy.last().at(0).toDouble(), 2.0);
    QCOMPARE(toSpy.last().at(0).toDouble(), 45.0);
    FilterKernel kernel = view.getCurrentFilter();
    QCOMPARE(filterSpy.last().at(0).value<FilterKernel>().getName(), kernel.getName());
    QCOMPARE(kernel.getFilterType().getName(), QStringLiteral("BPF"));
    QCOMPARE(kernel.getFilterOrder(), 128);
    QCOMPARE(kernel.getSamplingFrequency(), 1000.0);
    QVERIFY(qAbs(kernel.getHighpassFreq() - 2.0) < 1e-9);
    QVERIFY(qAbs(kernel.getLowpassFreq() - 45.0) < 1e-9);
    QCOMPARE(kernel.getCoefficients().cols(), 128);

    // Band edges are clamped to Nyquist and kept ordered
    view.setSamplingRate(60.0);
    QCOMPARE(pTo->maximum(), 30.0);
    QCOMPARE(view.getTo(), 30.0);
    QVERIFY(qAbs(view.getCurrentFilter().getLowpassFreq() - 30.0) < 1e-9);
    view.setSamplingRate(1000.0);
    view.setTo(40.0);
    view.setFrom(10.0);
    QCOMPARE(pTo->minimum(), 10.0);
    QCOMPARE(pFrom->maximum(), 40.0);

    // Editing a control and finishing the edit redesigns the filter
    const int iBefore = filterSpy.count();
    pTrans->setValue(2.0);
    emit pTrans->editingFinished();
    QCOMPARE(filterSpy.count(), iBefore + 1);
    QVERIFY(qAbs(view.getCurrentFilter().getParksWidth() * 500.0 - 2.0) < 1e-9);
    pMethod->setCurrentIndex(pMethod->count() - 1);
    QCOMPARE(view.getCurrentFilter().getDesignMethod().getName(), pMethod->currentText());
    pMethod->setCurrentIndex(0);

    // Channel type: selection, Enter and Delete report it
    QSignalSpy typeSpy(&view, &FilterDesignView::filterChannelTypeChanged);
    view.setChannelType(QStringLiteral("EEG"));
    QCOMPARE(view.getChannelType(), QStringLiteral("EEG"));
    QCOMPARE(typeSpy.last().at(0).toString(), QStringLiteral("EEG"));
    QTest::keyClick(&view, Qt::Key_Return);
    QTest::keyClick(&view, Qt::Key_Delete);
    QTest::keyClick(&view, Qt::Key_A);
    QCOMPARE(typeSpy.count(), 3);

    // The response plot is drawn
    auto* pPlot = view.findChild<QGraphicsView*>(QStringLiteral("m_graphicsView_filterPlot"));
    QVERIFY(pPlot && pPlot->scene());
    QVERIFY(!pPlot->scene()->items().isEmpty());
    view.guiStyleChanged(AbstractView::StyleMode::Dark);
    view.resize(700, 500);

    // Export the coefficients, then load them back through the load button
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString sFile = dir.filePath(QStringLiteral("designed.txt"));
    view.setFrom(4.0);
    view.setTo(30.0);
    kernel = view.getCurrentFilter();
    chooseFileInNextDialog(sFile);
    QTest::mouseClick(view.findChild<QPushButton*>(QStringLiteral("m_pushButton_exportFilter")), Qt::LeftButton);
    QVERIFY(QFile::exists(sFile));
    // The file is named after type, band and sampling rate, like the suggested file name
    FilterKernel exported;
    QVERIFY(FilterIO::readFilter(sFile, exported));
    QCOMPARE(exported.getName(), QStringLiteral("%1_4_30_Fs%2").arg(kernel.getFilterType().getName()).arg(static_cast<int>(kernel.getSamplingFrequency())));

    view.setFrom(1.0);
    view.setTo(20.0);
    pTaps->setValue(64);
    view.setFrom(1.0);
    chooseFileInNextDialog(sFile);
    QTest::mouseClick(view.findChild<QPushButton*>(QStringLiteral("m_pushButton_loadFilter")), Qt::LeftButton);
    QCOMPARE(view.getFrom(), 4.0);
    QCOMPARE(view.getTo(), 30.0);
    QCOMPARE(pTaps->value(), 128);
    QVERIFY(qAbs(view.getCurrentFilter().getHighpassFreq() - 4.0) < 1e-9);
    QVERIFY(qAbs(view.getCurrentFilter().getLowpassFreq() - 30.0) < 1e-9);
    QVERIFY((view.getCurrentFilter().getCoefficients() - kernel.getCoefficients()).cwiseAbs().maxCoeff() < 1e-6);

    // Plot export as PNG
    const QString sPng = dir.filePath(QStringLiteral("plot.png"));
    chooseFileInNextDialog(sPng);
    QTest::mouseClick(view.findChild<QPushButton*>(QStringLiteral("m_pushButton_exportPlot")), Qt::LeftButton);
    QVERIFY(!QImage(sPng).isNull());
}

//=============================================================================================================

void TestDispViewers2::coregSettingsView_fiducialsAndParams()
{
    CoregSettingsView view(QString{});
    // A decimal-comma locale makes number parsing of displayed text observable.
    view.setLocale(QLocale(QLocale::German, QLocale::Germany));
    view.show();
    auto* pPick = view.findChild<QCheckBox*>(QStringLiteral("m_qCheckBox_PickFiducials"));
    auto* pResult = view.findChild<QWidget*>(QStringLiteral("m_qWidget_ResultFiducials"));
    auto* pFidX = view.findChild<QLineEdit*>(QStringLiteral("m_qLineEdit_FidX"));
    auto* pFidZ = view.findChild<QLineEdit*>(QStringLiteral("m_qLineEdit_FidZ"));
    QVERIFY(pPick);
    QVERIFY(pResult);
    QVERIFY(pFidX);
    QVERIFY(pFidZ);

    // Picking toggles the result widget and reports the state.
    QSignalSpy pickSpy(&view, &CoregSettingsView::pickFiducials);
    pPick->setChecked(true);
    QCOMPARE(pickSpy.last().at(0).toBool(), true);
    QVERIFY(pResult->isEnabled());

    // Each picked position is stored and the selection steps LPA -> NAS -> RPA -> LPA.
    QSignalSpy fidSpy(&view, &CoregSettingsView::fiducialChanged);
    view.findChild<QRadioButton*>(QStringLiteral("m_qRadioButton_LPA"))->setChecked(true);
    QCOMPARE(view.getCurrentFiducial(), FIFFV_POINT_LPA);
    view.setFiducials(QVector3D(-0.07f, 0.0f, 0.0f));
    QCOMPARE(view.getCurrentFiducial(), FIFFV_POINT_NASION);
    view.setFiducials(QVector3D(0.0f, 0.09f, 0.01f));
    QCOMPARE(view.getCurrentFiducial(), FIFFV_POINT_RPA);
    view.setFiducials(QVector3D(0.07f, 0.0f, 0.0f));
    QCOMPARE(view.getCurrentFiducial(), FIFFV_POINT_LPA);
    QVERIFY(fidSpy.count() >= 3);
    QCOMPARE(fidSpy.last().at(0).toInt(), FIFFV_POINT_LPA);
    // Back at LPA the stored LPA position is displayed (cm-truncated, in mm).
    QCOMPARE(pFidX->text(), QStringLiteral("-70 mm"));
    view.findChild<QRadioButton*>(QStringLiteral("m_qRadioButton_NAS"))->setChecked(true);
    QCOMPARE(fidSpy.last().at(0).toInt(), FIFFV_POINT_NASION);
    QCOMPARE(pFidZ->text(), QStringLiteral("10 mm"));

    // Fitting ends picking.
    QSignalSpy fitFidSpy(&view, &CoregSettingsView::fitFiducials);
    QSignalSpy fitIcpSpy(&view, &CoregSettingsView::fitICP);
    QTest::mouseClick(view.findChild<QPushButton*>(QStringLiteral("m_qPushButton_FitFiducials")), Qt::LeftButton);
    QCOMPARE(fitFidSpy.count(), 1);
    QVERIFY(!pPick->isChecked());
    QVERIFY(!pResult->isEnabled());
    QCOMPARE(pickSpy.last().at(0).toBool(), false);
    pPick->setChecked(true);
    QTest::mouseClick(view.findChild<QPushButton*>(QStringLiteral("m_qPushButton_FitICP")), Qt::LeftButton);
    QCOMPARE(fitIcpSpy.count(), 1);
    QVERIFY(!pPick->isChecked());

    // Scaling mode enables the matching spin boxes and selects how the scale is read back.
    auto* pMode = view.findChild<QComboBox*>(QStringLiteral("m_qComboBox_ScalingMode"));
    auto* pScaleX = view.findChild<QDoubleSpinBox*>(QStringLiteral("m_qDoubleSpinBox_ScalingX"));
    auto* pScaleY = view.findChild<QDoubleSpinBox*>(QStringLiteral("m_qDoubleSpinBox_ScalingY"));
    auto* pScaleZ = view.findChild<QDoubleSpinBox*>(QStringLiteral("m_qDoubleSpinBox_ScalingZ"));
    QVERIFY(pMode && pScaleX && pScaleY && pScaleZ);

    QSignalSpy paramSpy(&view, &CoregSettingsView::transParamChanged);
    const Eigen::Vector3f vecTransIn(0.001f, -0.002f, 0.003f);
    const Eigen::Vector3f vecRotIn(0.1f, 0.2f, 0.3f);
    const Eigen::Vector3f vecScaleIn(1.0f, 1.0f, 1.0f);
    view.setTransParams(vecTransIn, vecRotIn, vecScaleIn);
    QCOMPARE(paramSpy.count(), 0); // programmatic update must not echo back

    Eigen::Vector3f vecRot, vecTrans, vecScale;
    pMode->setCurrentText(QStringLiteral("None"));
    QVERIFY(!pScaleX->isEnabled() && !pScaleY->isEnabled() && !pScaleZ->isEnabled());
    view.getTransParams(vecRot, vecTrans, vecScale);
    QVERIFY(vecTrans.isApprox(vecTransIn, 1e-3f));
    QVERIFY(vecRot.isApprox(vecRotIn, 1e-3f));
    QVERIFY(vecScale.isApprox(Eigen::Vector3f::Ones()));

    pMode->setCurrentText(QStringLiteral("Uniform"));
    QVERIFY(pScaleX->isEnabled() && !pScaleY->isEnabled() && !pScaleZ->isEnabled());
    pScaleX->setValue(1.25);
    QVERIFY(paramSpy.count() >= 1);
    view.getTransParams(vecRot, vecTrans, vecScale);
    QVERIFY(vecScale.isApprox(Eigen::Vector3f::Constant(1.25f)));

    pMode->setCurrentText(QStringLiteral("3-Axis"));
    QVERIFY(pScaleX->isEnabled() && pScaleY->isEnabled() && pScaleZ->isEnabled());
    pScaleY->setValue(0.9);
    pScaleZ->setValue(1.1);
    view.getTransParams(vecRot, vecTrans, vecScale);
    QVERIFY(vecScale.isApprox(Eigen::Vector3f(1.25f, 0.9f, 1.1f), 1e-4f));

    // A non-uniform scale survives the set/get round trip (index 0 is x).
    view.setTransParams(vecTransIn, vecRotIn, Eigen::Vector3f(1.1f, 1.2f, 1.3f));
    QCOMPARE(pScaleX->value(), 1.1);
    view.getTransParams(vecRot, vecTrans, vecScale);
    QVERIFY(vecScale.isApprox(Eigen::Vector3f(1.1f, 1.2f, 1.3f), 1e-4f));
    QVERIFY(vecRot.isApprox(vecRotIn, 1e-3f));

    // Fit settings and result labels.
    view.findChild<QSpinBox*>(QStringLiteral("m_qSpinBox_MaxIter"))->setValue(42);
    QCOMPARE(view.getMaxIter(), 42);
    view.findChild<QDoubleSpinBox*>(QStringLiteral("m_qDoubleSpinBox_Converge"))->setValue(0.5);
    QVERIFY(qAbs(view.getConvergence() - 0.0005f) < 1e-7f);
    view.findChild<QSpinBox*>(QStringLiteral("m_qSpinBox_MaxDist"))->setValue(7);
    QVERIFY(qAbs(view.getOmmitDistance() - 0.007f) < 1e-7f);
    view.findChild<QCheckBox*>(QStringLiteral("m_qCheckBox_AutoScale"))->setChecked(true);
    QVERIFY(view.getAutoScale());
    view.findChild<QDoubleSpinBox*>(QStringLiteral("m_qDoubleSpinBox_WeightLpa"))->setValue(2.0);
    view.findChild<QDoubleSpinBox*>(QStringLiteral("m_qDoubleSpinBox_WeightHSP"))->setValue(3.0);
    QCOMPARE(view.getWeightLPA(), 2.0f);
    QCOMPARE(view.getWeightHSP(), 3.0f);
    QCOMPARE(view.getWeightRPA(), 1.0f);
    QCOMPARE(view.getWeightNAS(), 10.0f);

    // Digitizer picks follow the check boxes; cardinal points are always used.
    auto* pEeg = view.findChild<QCheckBox*>(QStringLiteral("m_qCheckBox_EEG"));
    auto* pHpi = view.findChild<QCheckBox*>(QStringLiteral("m_qCheckBox_HPI"));
    auto* pHsp = view.findChild<QCheckBox*>(QStringLiteral("m_qCheckBox_HSP"));
    pEeg->setChecked(true);
    pHpi->setChecked(true);
    pHsp->setChecked(true);
    QCOMPARE(view.getDigitizerCheckState(), QList<int>({FIFFV_POINT_CARDINAL, FIFFV_POINT_EEG, FIFFV_POINT_HPI, FIFFV_POINT_EXTRA}));
    pEeg->setChecked(false);
    pHsp->setChecked(false);
    QCOMPARE(view.getDigitizerCheckState(), QList<int>({FIFFV_POINT_CARDINAL, FIFFV_POINT_HPI}));
    view.setOmittedPoints(5);
    QCOMPARE(view.findChild<QLabel*>(QStringLiteral("m_qLabel_NOmitted"))->text(), QStringLiteral("5"));
    view.setRMSE(0.0025f);
    QCOMPARE(view.findChild<QLabel*>(QStringLiteral("m_qLabel_RMSE"))->text(), QStringLiteral("2.5 mm"));

    // BEM selection: clearing (the placeholder) is silent, the first added BEM becomes current.
    QSignalSpy bemSpy(&view, &CoregSettingsView::changeSelectedBem);
    QCOMPARE(view.getCurrentSelectedBem(), QStringLiteral("Select BEM"));
    view.clearSelectionBem();
    QCOMPARE(bemSpy.count(), 0);
    view.addSelectionBem(QStringLiteral("sample-head.fif"));
    view.addSelectionBem(QStringLiteral("sample-5120-bem.fif"));
    QCOMPARE(bemSpy.count(), 1);
    QCOMPARE(view.getCurrentSelectedBem(), QStringLiteral("sample-head.fif"));
    view.findChild<QComboBox*>(QStringLiteral("m_qComboBox_BemItems"))->setCurrentIndex(1);
    QCOMPARE(bemSpy.last().at(0).toString(), QStringLiteral("sample-5120-bem.fif"));
    view.clearSelectionBem();
    QCOMPARE(bemSpy.count(), 2);
    QVERIFY(view.getCurrentSelectedBem().isEmpty());

    // File buttons emit the chosen path and nothing when cancelled.
    QSignalSpy digSpy(&view, &CoregSettingsView::digFileChanged);
    answerNextModal([](QWidget* pModal) {
        auto* pDialog = qobject_cast<QFileDialog*>(pModal);
        QVERIFY(pDialog);
        pDialog->reject();
    });
    QTest::mouseClick(view.findChild<QPushButton*>(QStringLiteral("m_qPushButton_LoadDig")), Qt::LeftButton);
    QCOMPARE(digSpy.count(), 0);
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

void TestDispViewers2::averagingSettingsView_controlsAndSettings()
{
    // Defaults without stored settings
    {
        AveragingSettingsView view(QString{});
        QCOMPARE(view.getNumAverages(), 10);
        QCOMPARE(view.getPreStimMSeconds(), 100);
        QCOMPARE(view.getPostStimMSeconds(), 400);
        QCOMPARE(view.getBaselineFromSeconds(), 0);
        QCOMPARE(view.getBaselineToSeconds(), 0);
        QVERIFY(!view.getDoBaselineCorrection());
        QCOMPARE(view.getCurrentStimCh(), QStringLiteral("STI014"));
    }

    // Stored settings, including a baseline that ends before the stimulus
    const QString sPath = QStringLiteral("test_disp_viewers2_averaging_%1").arg(QCoreApplication::applicationPid());
    {
        QSettings settings("MNECPP");
        settings.setValue(sPath + QStringLiteral("/AveragingSettingsView/preStimSeconds"), 200);
        settings.setValue(sPath + QStringLiteral("/AveragingSettingsView/postStimSeconds"), 500);
        settings.setValue(sPath + QStringLiteral("/AveragingSettingsView/baselineFromSeconds"), -150);
        settings.setValue(sPath + QStringLiteral("/AveragingSettingsView/baselineToSeconds"), -50);
        settings.setValue(sPath + QStringLiteral("/AveragingSettingsView/numAverages"), 25);
        settings.setValue(sPath + QStringLiteral("/AveragingSettingsView/currentStimChannel"), QStringLiteral("STI101"));
        settings.setValue(sPath + QStringLiteral("/AveragingSettingsView/doBaselineCorrection"), true);
    }
    const auto removeSettings = qScopeGuard([&sPath]() { QSettings("MNECPP").remove(sPath); });

    const QMap<QString, int> stimChannels = {{QStringLiteral("STI014"), 306}, {QStringLiteral("STI101"), 312}};
    AveragingSettingsView view(sPath, stimChannels);
    view.show();
    auto* pChannels = view.findChild<QComboBox*>(QStringLiteral("m_pComboBoxChSelection"));
    auto* pPre = view.findChild<QSpinBox*>(QStringLiteral("m_pSpinBoxPreStimMSeconds"));
    auto* pPost = view.findChild<QSpinBox*>(QStringLiteral("m_pSpinBoxPostStimMSeconds"));
    auto* pFrom = view.findChild<QSpinBox*>(QStringLiteral("m_pSpinBoxBaselineFromMSeconds"));
    auto* pTo = view.findChild<QSpinBox*>(QStringLiteral("m_pSpinBoxBaselineToMSeconds"));
    auto* pNum = view.findChild<QSpinBox*>(QStringLiteral("m_pSpinBoxNumAverages"));
    QVERIFY(pChannels);
    QVERIFY(pPre);
    QVERIFY(pPost);
    QVERIFY(pFrom);
    QVERIFY(pTo);
    QVERIFY(pNum);

    QCOMPARE(view.getNumAverages(), 25);
    QCOMPARE(pNum->value(), 25);
    QCOMPARE(view.getPreStimMSeconds(), 200);
    QCOMPARE(view.getPostStimMSeconds(), 500);
    QCOMPARE(view.getBaselineFromSeconds(), -150);
    QCOMPARE(view.getBaselineToSeconds(), -50);
    QCOMPARE(pTo->value(), -50);
    QVERIFY(view.getDoBaselineCorrection());

    // Stim channel: the stored one is selected and its channel index is reported
    QCOMPARE(pChannels->currentText(), QStringLiteral("STI101"));
    QCOMPARE(view.getStimChannelIdx(), 312);
    QSignalSpy stimSpy(&view, &AveragingSettingsView::changeStimChannel);
    pChannels->setCurrentText(QStringLiteral("STI014"));
    QCOMPARE(stimSpy.count(), 1);
    QCOMPARE(stimSpy.last().at(0).toString(), QStringLiteral("STI014"));
    QCOMPARE(view.getCurrentStimCh(), QStringLiteral("STI014"));
    QCOMPARE(view.getStimChannelIdx(), 306);

    // New stim channels keep the selection and do not duplicate the reaction
    view.setStimChannels({{QStringLiteral("STI014"), 6}, {QStringLiteral("STI201"), 7}});
    QCOMPARE(pChannels->count(), 2);
    QCOMPARE(view.getStimChannelIdx(), 6);
    stimSpy.clear();
    pChannels->setCurrentText(QStringLiteral("STI201"));
    QCOMPARE(stimSpy.count(), 1);
    QCOMPARE(view.getCurrentStimCh(), QStringLiteral("STI201"));
    QCOMPARE(view.getStimChannelIdx(), 7);

    // Epoch window and baseline: finished edits emit and constrain the baseline range
    QSignalSpy preSpy(&view, &AveragingSettingsView::changePreStim);
    QSignalSpy postSpy(&view, &AveragingSettingsView::changePostStim);
    QSignalSpy fromSpy(&view, &AveragingSettingsView::changeBaselineFrom);
    QSignalSpy toSpy(&view, &AveragingSettingsView::changeBaselineTo);
    QSignalSpy numSpy(&view, &AveragingSettingsView::changeNumAverages);
    pPre->setValue(300);
    emit pPre->editingFinished();
    QCOMPARE(preSpy.last().at(0).toInt(), 300);
    QCOMPARE(pFrom->minimum(), -300);
    QCOMPARE(pTo->minimum(), -300);
    pPost->setValue(600);
    emit pPost->editingFinished();
    QCOMPARE(postSpy.last().at(0).toInt(), 600);
    QCOMPARE(pFrom->maximum(), 600);
    pFrom->setValue(-250);
    emit pFrom->editingFinished();
    QCOMPARE(fromSpy.last().at(0).toInt(), -250);
    QCOMPARE(pTo->minimum(), -250);
    pTo->setValue(-20);
    emit pTo->editingFinished();
    QCOMPARE(toSpy.last().at(0).toInt(), -20);
    QCOMPARE(pFrom->maximum(), -20);
    pNum->setValue(40);
    emit pNum->editingFinished();
    QCOMPARE(numSpy.last().at(0).toInt(), 40);
    QCOMPARE(view.getNumAverages(), 40);

    QSignalSpy baselineSpy(&view, &AveragingSettingsView::changeBaselineActive);
    auto* pBaseline = view.findChild<QCheckBox*>(QStringLiteral("m_pcheckBoxBaselineCorrection"));
    pBaseline->click();
    QCOMPARE(baselineSpy.count(), 1);
    QCOMPARE(baselineSpy.last().at(0).toBool(), pBaseline->isChecked());
    QCOMPARE(view.getDoBaselineCorrection(), pBaseline->isChecked());
    QSignalSpy resetSpy(&view, &AveragingSettingsView::resetAverage);
    QTest::mouseClick(view.findChild<QPushButton*>(QStringLiteral("m_pushButton_reset")), Qt::LeftButton);
    QCOMPARE(resetSpy.count(), 1);

    // Detected epochs: one row per type, at most ten
    auto* pTrials = view.findChild<QGroupBox*>(QStringLiteral("m_groupBox_detectedTrials"));
    QVERIFY(pTrials);
    FiffEvokedSet evokedSet;
    for (int i = 0; i < 12; ++i) {
        FiffEvoked evoked;
        evoked.comment = QString::number(i + 1);
        evoked.nave = 3 * i;
        evokedSet.evoked.append(evoked);
    }
    view.setDetectedEpochs(evokedSet);
    QVERIFY(!pTrials->isHidden());
    auto* pGrid = qobject_cast<QGridLayout*>(pTrials->layout());
    QVERIFY(pGrid);
    QCOMPARE(pGrid->count(), 2 * 11);
    QCOMPARE(qobject_cast<QLabel*>(pGrid->itemAtPosition(3, 1)->widget())->text(), QStringLiteral("6"));
    view.setDetectedEpochs(evokedSet);
    QCOMPARE(pGrid->count(), 2 * 11);
    view.setDetectedEpochs(FiffEvokedSet());
    QVERIFY(pTrials->isHidden());

    // Offline mode offers the compute controls
    QSignalSpy computeSpy(&view, &AveragingSettingsView::calculateAverage);
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    auto* pCompute = view.findChild<QPushButton*>(QStringLiteral("m_pushButton_compute"));
    QVERIFY(!pCompute->isHidden());
    QVERIFY(pChannels->isHidden());
    QTest::mouseClick(pCompute, Qt::LeftButton);
    QCOMPARE(computeSpy.count(), 1);
    auto* pAuto = view.findChild<QCheckBox*>(QStringLiteral("checkBox_autoCompute"));
    QSignalSpy autoSpy(&view, &AveragingSettingsView::setAutoCompute);
    pAuto->click();
    QCOMPARE(autoSpy.count(), 1);
    QCOMPARE(view.getAutoComputeStatus(), pAuto->isChecked());
    view.setProcessingMode(AbstractView::ProcessingMode::RealTime);
    QVERIFY(pCompute->isHidden());
}

//=============================================================================================================

void TestDispViewers2::triggerDetectionView_settingsAndSignals()
{
    auto info = createBrowserTestInfo();
    info->chs[2].kind = FIFFV_STIM_CH;
    info->chs[2].ch_name = QStringLiteral("STI101");

    TriggerDetectionView view(QString{});
    view.show();
    auto* pChannels = view.findChild<QComboBox*>(QStringLiteral("m_comboBox_triggerChannels"));
    auto* pTypes = view.findChild<QComboBox*>(QStringLiteral("m_comboBox_triggerColorType"));
    auto* pActive = view.findChild<QCheckBox*>(QStringLiteral("m_checkBox_activateTriggerDetection"));
    auto* pFirst = view.findChild<QDoubleSpinBox*>(QStringLiteral("m_doubleSpinBox_detectionThresholdFirst"));
    auto* pSecond = view.findChild<QSpinBox*>(QStringLiteral("m_spinBox_detectionThresholdSecond"));
    auto* pCount = view.findChild<QLabel*>(QStringLiteral("m_label_numberDetectedTriggers"));
    auto* pDetect = view.findChild<QPushButton*>(QStringLiteral("m_pushButton_DetectTriggers"));
    QVERIFY(pChannels);
    QVERIFY(pTypes);
    QVERIFY(pActive);
    QVERIFY(pFirst);
    QVERIFY(pSecond);
    QVERIFY(pCount);
    QVERIFY(pDetect);

    // Re-initialising (new measurement info) must not duplicate the reactions
    view.init(info);
    view.init(info);
    QCOMPARE(pChannels->count(), 2);
    QCOMPARE(pChannels->itemText(0), QStringLiteral("STI101"));
    QCOMPARE(view.getSelectedStimChannel(), QStringLiteral("STI101"));

    QSignalSpy infoSpy(&view, &TriggerDetectionView::triggerInfoChanged);
    pFirst->setValue(2.5);
    QCOMPARE(infoSpy.count(), 1);
    pSecond->setValue(-3);
    QCOMPARE(infoSpy.count(), 2);
    QVERIFY(qAbs(infoSpy.last().at(3).toDouble() - 2.5e-3) < 1e-15);
    pActive->setChecked(true);
    QCOMPARE(infoSpy.last().at(1).toBool(), true);
    pChannels->setCurrentIndex(1);
    QCOMPARE(infoSpy.last().at(2).toString(), QStringLiteral("STI014"));
    QCOMPARE(infoSpy.count(), 4);

    // Detected triggers fill the count and the type list once per value
    view.setNumberDetectedTriggersAndTypes(3, {{3, {{10, 1.0}, {20, 4.0}, {30, 1.0}}}});
    QCOMPARE(pCount->text(), QStringLiteral("3"));
    QCOMPARE(pTypes->count(), 2);

    // The color button shows the color of the selected type; picking a color stores it for that type
    auto* pColor = view.findChild<QPushButton*>(QStringLiteral("m_pushButton_triggerColor"));
    QVERIFY(pColor);
    pTypes->setCurrentIndex(1);
    answerNextModal([](QWidget* pModal) {
        auto* pDialog = qobject_cast<QColorDialog*>(pModal);
        QVERIFY(pDialog);
        pDialog->setCurrentColor(QColor(10, 200, 30));
        static_cast<QDialog*>(pDialog)->accept();
    });
    QTest::mouseClick(pColor, Qt::LeftButton);
    QCOMPARE(pColor->palette().color(QPalette::Button), QColor(10, 200, 30));
    using ColorMap = QMap<double, QColor>;
    QCOMPARE(infoSpy.last().at(0).value<ColorMap>().value(4.0), QColor(10, 200, 30));
    pTypes->setCurrentIndex(0);
    QVERIFY(pColor->palette().color(QPalette::Button) != QColor(10, 200, 30));
    pTypes->setCurrentIndex(1);
    QCOMPARE(pColor->palette().color(QPalette::Button), QColor(10, 200, 30));
    // Cancelling the colour dialog keeps the colour
    const qsizetype infoCount = infoSpy.size();
    answerNextModal([](QWidget* pModal) {
        auto* pDialog = qobject_cast<QColorDialog*>(pModal);
        QVERIFY(pDialog);
        static_cast<QDialog*>(pDialog)->reject();
    });
    QTest::mouseClick(pColor, Qt::LeftButton);
    QCOMPARE(pColor->palette().color(QPalette::Button), QColor(10, 200, 30));
    QCOMPARE(infoSpy.size(), infoCount);

    QSignalSpy resetSpy(&view, &TriggerDetectionView::resetTriggerCounter);
    QTest::mouseClick(view.findChild<QPushButton*>(QStringLiteral("m_pushButton_resetNumberTriggers")), Qt::LeftButton);
    QCOMPARE(resetSpy.count(), 1);
    QCOMPARE(pCount->text(), QStringLiteral("0"));
    QCOMPARE(pTypes->count(), 0);

    // Offline mode swaps the live controls for the detect button
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    QVERIFY(!pDetect->isHidden());
    QVERIFY(pActive->isHidden());
    QSignalSpy detectSpy(&view, &TriggerDetectionView::detectTriggers);
    QTest::mouseClick(pDetect, Qt::LeftButton);
    QCOMPARE(detectSpy.count(), 1);
    QCOMPARE(detectSpy.last().at(0).toString(), QStringLiteral("STI014"));
    QVERIFY(qAbs(detectSpy.last().at(1).toDouble() - 2.5e-3) < 1e-15);
    view.setProcessingMode(AbstractView::ProcessingMode::RealTime);
    QVERIFY(pDetect->isHidden());
}

//=============================================================================================================

void TestDispViewers2::artifactSettingsView_thresholds()
{
    auto channel = [](int kind, int unit) {
        FiffChInfo ch;
        ch.kind = kind;
        ch.unit = unit;
        return ch;
    };
    const QList<FiffChInfo> megEeg = {channel(FIFFV_MEG_CH, FIFF_UNIT_T_M), channel(FIFFV_MEG_CH, FIFF_UNIT_T),
                                      channel(FIFFV_MEG_CH, FIFF_UNIT_T_M), channel(FIFFV_EEG_CH, FIFF_UNIT_V),
                                      channel(FIFFV_STIM_CH, FIFF_UNIT_NONE)};

    ArtifactSettingsView view(QString{});
    view.show();
    QVERIFY(view.findChildren<QDoubleSpinBox*>().isEmpty());
    QVERIFY(!view.getDoArtifactThresholdRejection());

    // One row per present type; mantissa and exponent compose the threshold
    QSignalSpy thresholdSpy(&view, &ArtifactSettingsView::changeArtifactThreshold);
    view.setChInfo(megEeg);
    auto visible = [&view]() {
        QApplication::processEvents();
        QStringList labels;
        for (QLabel* pLabel : view.findChildren<QLabel*>()) {
            if (!pLabel->isHidden()) {
                labels << pLabel->text();
            }
        }
        labels.sort();
        return labels;
    };
    QCOMPARE(visible(), QStringList({"eeg", "grad", "mag"}));
    QCOMPARE(thresholdSpy.count(), 1);
    QMap<QString, double> map = view.getThresholdMap();
    QCOMPARE(map.value(QStringLiteral("Active")), 0.0);
    QCOMPARE(map.keys(), QStringList({"Active", "eeg", "grad", "mag"}));
    QCOMPARE(map.value(QStringLiteral("grad")), 0.1);

    auto spin = [&view](int iRow, int iColumn) {
        auto* pLayout = qobject_cast<QGridLayout*>(view.layout());
        return pLayout->itemAtPosition(iRow, iColumn)->widget();
    };
    auto* pLayout = qobject_cast<QGridLayout*>(view.layout());
    QVERIFY(pLayout);
    int iGradRow = -1;
    for (int r = 1; r < pLayout->rowCount(); ++r) {
        if (qobject_cast<QLabel*>(spin(r, 0))->text() == QStringLiteral("grad")) {
            iGradRow = r;
        }
    }
    QVERIFY(iGradRow > 0);
    qobject_cast<QDoubleSpinBox*>(spin(iGradRow, 1))->setValue(4.0);
    qobject_cast<QSpinBox*>(spin(iGradRow, 2))->setValue(-11);
    QVERIFY(qAbs(view.getThresholdMap().value(QStringLiteral("grad")) - 4e-11) < 1e-20);
    QVERIFY(qAbs(thresholdSpy.last().at(0).value<QMap<QString, double>>().value(QStringLiteral("grad")) - 4e-11) < 1e-20);

    // Activation is reported in the map
    auto* pActive = qobject_cast<QCheckBox*>(spin(0, 0));
    QVERIFY(pActive);
    QTest::mouseClick(pActive, Qt::LeftButton);
    QVERIFY(view.getDoArtifactThresholdRejection());
    QCOMPARE(view.getThresholdMap().value(QStringLiteral("Active")), 1.0);

    // A new channel set replaces the rows and keeps the active state and the edited threshold
    view.setChInfo({channel(FIFFV_MEG_CH, FIFF_UNIT_T_M), channel(FIFFV_EOG_CH, FIFF_UNIT_V)});
    QCOMPARE(visible(), QStringList({"eog", "grad"}));
    QCOMPARE(view.findChildren<QCheckBox*>().size(), 1);
    QVERIFY(view.getDoArtifactThresholdRejection());
    map = view.getThresholdMap();
    QCOMPARE(map.keys(), QStringList({"Active", "eog", "grad"}));
    QVERIFY(qAbs(map.value(QStringLiteral("grad")) - 4e-11) < 1e-20);

    view.saveSettings();
    view.loadSettings();
    view.clearView();
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

    // Status feedback for each computation state
    auto* feedback = view.findChild<QLabel*>(QStringLiteral("m_label_recomputationFeedback"));
    QVERIFY(feedback);
    const QStringList states = {QStringLiteral("Initializing"), QStringLiteral("Computing"),
                                QStringLiteral("Recomputing"), QStringLiteral("Clustering"),
                                QStringLiteral("Not Computed"), QStringLiteral("Not Clustered"),
                                QStringLiteral("Finished")};
    for (int status = 0; status < states.size(); ++status) {
        view.setRecomputationStatus(status);
        QCOMPARE(feedback->text(), states.at(status));
    }

    // Solution information
    const auto text = [&view](const char* name) {
        return view.findChild<QLineEdit*>(QLatin1String(name))->text();
    };
    for (const auto& [frame, frameName] : {std::pair{FIFFV_COORD_HEAD, "Head Space"},
                                           std::pair{FIFFV_COORD_MRI, "MRI Space"},
                                           std::pair{FIFFV_COORD_DEVICE, "Unknown"}}) {
        view.setSolutionInformation(frame == FIFFV_COORD_MRI ? 1 : 0, frame, 7498, 306, 2);
        QCOMPARE(text("m_lineEdit_sCoordFrame"), QLatin1String(frameName));
    }
    QCOMPARE(text("m_lineEdit_sSourceOri"), QStringLiteral("fixed"));
    QCOMPARE(text("m_lineEdit_iNDipole"), QStringLiteral("7498"));
    QCOMPARE(text("m_lineEdit_iNChan"), QStringLiteral("306"));
    QCOMPARE(text("m_lineEdit_iNSourceSpace"), QStringLiteral("2"));
    view.setClusteredInformation(200);
    QCOMPARE(text("m_lineEdit_iNDipoleClustered"), QStringLiteral("200"));

    // Recomputation and cluster number
    QSignalSpy recompSpy(&view, &FwdSettingsView::recompStatusChanged);
    view.findChild<QCheckBox*>(QStringLiteral("m_checkBox_bDoRecomputation"))->click();
    QVERIFY(view.getRecomputationStatusChanged());
    QCOMPARE(recompSpy.takeFirst().at(0).toBool(), true);
    QSignalSpy clusterNumberSpy(&view, &FwdSettingsView::clusterNumberChanged);
    view.findChild<QSpinBox*>(QStringLiteral("m_spinBox_iNDipoleClustered"))->setValue(150);
    QCOMPARE(view.getClusterNumber(), 150);
    QCOMPARE(clusterNumberSpy.takeFirst().at(0).toInt(), 150);

    // Atlas directory: a directory without the a2009s annotations is not loaded
    auto* atlasStat = view.findChild<QLabel*>(QStringLiteral("m_qLabel_atlasStat"));
    QTemporaryDir atlasDir;
    QVERIFY(atlasDir.isValid());
    QSignalSpy atlasSpy(&view, &FwdSettingsView::atlasDirChanged);
    chooseFileInNextDialog(atlasDir.path());
    QMetaObject::invokeMethod(view.findChild<QPushButton*>(QStringLiteral("m_qPushButton_AtlasDirDialog")), "released");
    QCOMPARE(atlasStat->text(), QStringLiteral("not loaded"));
    QVERIFY(atlasSpy.isEmpty());

    // Clustering needs an annotation set: without one the box is unchecked again after a warning
    QSignalSpy clusteringSpy(&view, &FwdSettingsView::clusteringStatusChanged);
    auto* clustering = view.findChild<QCheckBox*>(QStringLiteral("m_checkBox_bDoClustering"));
    clustering->setChecked(false);
    clickNextMessageBoxButton(QStringLiteral("OK"));
    clustering->click();
    QVERIFY(!view.getClusteringStatusChanged());
    QVERIFY(clusteringSpy.isEmpty());

    // With both hemispheres' a2009s annotations the atlas loads and clustering is allowed
    const QString label = QCoreApplication::applicationDirPath() + QStringLiteral("/../resources/data/mne-cpp-test-data/subjects/sample/label/");
    for (const QString& hemi : {QStringLiteral("lh"), QStringLiteral("rh")}) {
        QVERIFY(QFile::copy(label + hemi + QStringLiteral(".aparc.annot"),
                            atlasDir.filePath(hemi + QStringLiteral(".aparc.a2009s.annot"))));
    }
    chooseFileInNextDialog(atlasDir.path());
    QMetaObject::invokeMethod(view.findChild<QPushButton*>(QStringLiteral("m_qPushButton_AtlasDirDialog")), "released");
    QCOMPARE(atlasStat->text(), QStringLiteral("loaded"));
    QCOMPARE(atlasSpy.size(), 1);
    QCOMPARE(atlasSpy.at(0).at(0).toString(), atlasDir.path());
    clustering->click();
    QVERIFY(view.getClusteringStatusChanged());
    QCOMPARE(clusteringSpy.takeFirst().at(0).toBool(), true);

    QSignalSpy computeSpy(&view, &FwdSettingsView::doForwardComputation);
    view.findChild<QPushButton*>(QStringLiteral("m_qPushButton_ComputeForward"))->click();
    QCOMPARE(computeSpy.size(), 1);

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
    const QString settingsPath = QStringLiteral("test_disp_viewers2_avgsel");
    QSettings(QStringLiteral("MNECPP")).remove(settingsPath);
    auto view = std::make_unique<AverageSelectionView>(settingsPath);

    view->setGuiMode(AbstractView::GuiMode::Research);
    view->setProcessingMode(AbstractView::ProcessingMode::Offline);
    view->clearView();

    // One checkbox and one colour button per average
    auto colors = QSharedPointer<QMap<QString, QColor>>::create();
    auto activation = QSharedPointer<QMap<QString, bool>>::create();
    colors->insert(QStringLiteral("aud"), Qt::red);
    colors->insert(QStringLiteral("vis"), Qt::blue);
    activation->insert(QStringLiteral("aud"), true);
    activation->insert(QStringLiteral("vis"), false);
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
    QVERIFY(activation->value(QStringLiteral("vis")));

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
    QCOMPARE(colors->value(QStringLiteral("vis")), QColor(10, 200, 30));
    QCOMPARE(colorSpy.size(), 1);
    answerNextModal([](QWidget* pModal) {
        auto* pDialog = qobject_cast<QColorDialog*>(pModal);
        QVERIFY(pDialog);
        static_cast<QDialog*>(pDialog)->reject();
    });
    visButton->click();
    QCOMPARE(colors->value(QStringLiteral("vis")), QColor(10, 200, 30));
    QCOMPARE(colorSpy.size(), 1);

    // Colours and activation are restored by a new view with the same settings path
    view.reset();
    view = std::make_unique<AverageSelectionView>(settingsPath);
    QCOMPARE(view->getAverageColor()->value(QStringLiteral("vis")), QColor(10, 200, 30));
    QVERIFY(view->getAverageActivation()->value(QStringLiteral("vis")));
    QCOMPARE(view->findChildren<QCheckBox*>().size(), 2);

    // At most 10 averages are listed; maps of different size are not drawn
    for (int i = 0; i < 12; ++i) {
        colors->insert(QStringLiteral("avg%1").arg(i, 2, 10, QLatin1Char('0')), Qt::green);
        activation->insert(QStringLiteral("avg%1").arg(i, 2, 10, QLatin1Char('0')), true);
    }
    view->setAverageActivation(activation);
    view->setAverageColor(colors);
    QApplication::processEvents();
    QCOMPARE(view->findChildren<QCheckBox*>().size(), 10);
    activation->remove(QStringLiteral("aud"));
    view->setAverageActivation(activation);
    QApplication::processEvents();
    QCOMPARE(view->findChildren<QCheckBox*>().size(), 10);

    view.reset();
    QSettings(QStringLiteral("MNECPP")).remove(settingsPath);
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

void TestDispViewers2::butterflyView_drawsAveragesAcrossTheFullWidth()
{
    // One magnetometer average of 3 * width samples: the curve is decimated, and must still span the width
    auto set = QSharedPointer<FiffEvokedSet>::create();
    set->info = *createBrowserTestInfo();
    set->info.bads.clear();
    FiffEvoked evoked;
    evoked.comment = QStringLiteral("aud");
    evoked.baseline = qMakePair(-0.1f, 0.0f);
    evoked.times = Eigen::RowVectorXf::LinSpaced(1200, -0.2f, 0.9992f);
    evoked.times(200) = 0.0f;
    evoked.data = Eigen::MatrixXd::Zero(4, 1200);
    evoked.data.row(0).setConstant(5e-13);
    set->evoked.append(evoked);

    auto model = QSharedPointer<EvokedSetModel>::create();
    model->setEvokedSet(set);
    ButterflyView view(QStringLiteral("test_disp_viewers2_butterfly"));
    view.resize(400, 200);
    view.setEvokedSetModel(model);
    view.setScaleMap({{FIFF_UNIT_T, 1e-12f}});
    view.setModalityMap({{QStringLiteral("MAG"), true}});
    view.setSelectedChannels({0});
    view.setBackgroundColor(Qt::white);
    view.setAverageActivation(QSharedPointer<QMap<QString, bool>>::create(QMap<QString, bool>{{QStringLiteral("aud"), true}}));
    view.setAverageActivation(QSharedPointer<QMap<QString, bool>>::create(QMap<QString, bool>{{QStringLiteral("aud"), true}}));
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
    QCOMPARE(model.getCurrentSampleIndex(), 20);
    QCOMPARE(model.getLastBlock(), secondBlock);
    QCOMPARE(model.getMaxValueFromRawViewModel(0), static_cast<double>(1e-11f));
    QCOMPARE(dataSpy.size(), 2);

    model.addData({Eigen::MatrixXd::Zero(3, 10)});
    QCOMPARE(model.getCurrentSampleIndex(), 20);
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

void TestDispViewers2::rtFiffRawViewModel_ringBufferWrap()
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
            model.addData({block});
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
        const FilterKernel kernel(QStringLiteral("lp"), 0, 50, 0.2, 0.0, 0.05, 100.0, 0);
        const Eigen::RowVectorXd h = kernel.getCoefficients();
        const int L = static_cast<int>(h.cols());
        const int iDelay = L / 2;
        model.setFilterChannelType(QStringLiteral("MEG"));
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
            model.addData({stream.middleCols(b * nBlock, nBlock)});
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
    }

    // Triggers: positions are buffer columns of the sweep they belong to.
    {
        RtFiffRawViewModel model;
        auto info = createBrowserTestInfo();
        model.setFiffInfo(info);
        model.setSamplingInfo(100.0f, 1, true);
        model.triggerInfoChanged({{2.0, Qt::red}}, true, QStringLiteral("STI014"), 1.0);
        QSignalSpy triggerSpy(&model, &RtFiffRawViewModel::triggerDetected);

        // Pulses at stream samples 90 (before the wrap inside block 60..119) and 215 (after the wrap inside 180..239)
        Eigen::MatrixXd stream = Eigen::MatrixXd::Zero(4, 240);
        stream(3, 90) = 2.0;
        stream(3, 215) = 2.0;
        model.addData({stream.middleCols(0, 60)});
        model.addData({stream.middleCols(60, 60)});
        QCOMPARE(model.getDetectedTriggersOld(), (QList<QPair<int, double>>{{90, 2.0}}));
        QVERIFY(model.getDetectedTriggers().isEmpty());
        model.addData({stream.middleCols(120, 60)});
        model.addData({stream.middleCols(180, 60)});
        QCOMPARE(model.getDetectedTriggers(), (QList<QPair<int, double>>{{15, 2.0}}));
        QVERIFY(model.getDetectedTriggersOld().isEmpty());
        QCOMPARE(triggerSpy.size(), 2);
    }
}

//=============================================================================================================

void TestDispViewers2::rtFiffRawViewModel_projectionWrapFilterAndRoles()
{
    RtFiffRawViewModel model;
    auto info = createBrowserTestInfo();
    info->bads.clear();
    model.setFiffInfo(info);
    model.setSamplingInfo(100.0f, 1, true);

    // An active projector removing the common mode of the three MEG channels
    Eigen::MatrixXd vec(1, 3);
    vec.setConstant(1.0 / std::sqrt(3.0));
    FiffNamedMatrix vectors(1, 3, QStringList(), info->ch_names.mid(0, 3), vec);
    model.updateProjection({FiffProj(FIFFV_PROJ_ITEM_FIELD, true, QStringLiteral("common"), vectors)});
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
        model.addData({block});
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
    QCOMPARE(model.data(model.index(3, 0)).toString(), QStringLiteral("STI014"));
    const auto row = model.data(model.index(0, 1)).value<RowVectorPair>();
    QCOMPARE(row.second, 100);
    QVERIFY(std::fabs(row.first[10] - std::sin(0.1 * 110)) < 1e-12);
    QCOMPARE(model.data(model.index(1, 2)).toBool(), false);
    QCOMPARE(model.data(model.index(0, 0), Qt::BackgroundRole).value<QBrush>().color(), QColor(Qt::darkGray));
    QVERIFY(!model.data(model.index(0, 0), Qt::ToolTipRole).isValid());
    QCOMPARE(model.headerData(1, Qt::Horizontal).toString(), QStringLiteral("data plot"));
    QCOMPARE(model.headerData(1, Qt::Horizontal, Qt::TextAlignmentRole).toInt(), static_cast<int>(Qt::AlignLeft));
    QCOMPARE(model.headerData(2, Qt::Vertical).toString(), QStringLiteral("MEG0113"));
    QVERIFY(!model.headerData(0, Qt::Horizontal, Qt::ToolTipRole).isValid());

    // Freezing keeps serving the snapshot while new data arrives
    model.toggleFreeze(QModelIndex());
    model.addData({block * 0.0});
    const auto frozen = model.data(model.index(0, 1)).value<RowVectorPair>();
    QVERIFY(std::fabs(frozen.first[10] - std::sin(0.1 * 110)) < 1e-12);
    model.toggleFreeze(QModelIndex());

    // Real-time filtering: a 50-tap low-pass on the MEG channels only, a pass-through on STIM
    model.setFilterChannelType(QStringLiteral("MEG"));
    model.setFilter({FilterKernel(QStringLiteral("lp"), 0, 50, 0.2, 0.0, 0.05, 100.0, 0)});
    model.setFilterActive(true);
    Eigen::MatrixXd tone(4, 50);
    for (int s = 0; s < 50; ++s) {
        tone.col(s) << std::sin(0.2 * s), 0.0, 0.0, s % 5;
    }
    for (int b = 0; b < 4; ++b) {
        model.addData({tone});
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

void TestDispViewers2::averageLayoutView_drawsEachChannelsAverage()
{
    // A positive step on MEG0111 and a negative one on MEG0113, both from the stimulus onset
    auto set = QSharedPointer<FiffEvokedSet>::create();
    set->info = *createBrowserTestInfo();
    FiffEvoked evoked;
    evoked.comment = QStringLiteral("aud");
    evoked.baseline = qMakePair(-0.05f, 0.0f);
    evoked.times = Eigen::RowVectorXf::LinSpaced(120, -0.06f, 0.059f);
    evoked.times(60) = 0.0f;
    evoked.data = Eigen::MatrixXd::Zero(4, 120);
    evoked.data.row(0).tail(60).setConstant(8e-13);
    evoked.data.row(2).tail(60).setConstant(-8e-13);
    set->evoked.append(evoked);
    auto model = QSharedPointer<EvokedSetModel>::create();
    model->setEvokedSet(set);
    auto pInfo = QSharedPointer<FiffInfo>::create(set->info);

    AverageLayoutView view(QStringLiteral("test_disp_viewers2_average_layout"));
    view.setFiffInfo(pInfo);
    view.setEvokedSetModel(model);
    view.setBackgroundColor(Qt::white);
    QCOMPARE(view.getBackgroundColor(), QColor(Qt::white));
    QCOMPARE(view.getEvokedSetModel(), model);

    // Two channels, one bad, laid out left and right
    SelectionItem selection;
    selection.m_sChannelName = {QStringLiteral("MEG0111"), QStringLiteral("MEG0113")};
    selection.m_iChannelNumber = {0, 2};
    selection.m_iChannelKind = {FIFFV_MEG_CH, FIFFV_MEG_CH};
    selection.m_iChannelUnit = {FIFF_UNIT_T, FIFF_UNIT_T};
    selection.m_qpChannelPosition = {QPointF(-1.0, 0.0), QPointF(1.0, 0.0)};
    selection.m_bShowAll = false;
    view.channelSelectionChanged(QVariant::fromValue(&selection));
    view.setScaleMap({{FIFF_UNIT_T, 1e-12f}});
    view.setAverageColor(QSharedPointer<QMap<QString, QColor>>::create(QMap<QString, QColor>{{QStringLiteral("aud"), Qt::red}}));
    view.setAverageActivation(QSharedPointer<QMap<QString, bool>>::create(QMap<QString, bool>{{QStringLiteral("aud"), true}}));
    view.setSingleAverageColor(Qt::black);
    QCOMPARE(view.getAverageColor()->value(QStringLiteral("aud")), QColor(Qt::black));
    QVERIFY(view.getAverageActivation()->value(QStringLiteral("aud")));

    auto* scene = view.findChild<QGraphicsScene*>();
    QVERIFY(scene != nullptr);
    QCOMPARE(scene->items().size(), 2);
    // Recolouring keeps the channel items (name, number and layout position) intact
    QStringList names;
    for (QGraphicsItem* item : scene->items()) {
        auto* averageItem = static_cast<AverageSceneItem*>(item);
        names << averageItem->m_sChannelName;
        QCOMPARE(std::abs(averageItem->pos().x()), 160.0);
    }
    names.sort();
    QCOMPARE(names, (QStringList{QStringLiteral("MEG0111"), QStringLiteral("MEG0113")}));

    // Item at x = -160 (MEG0111) and x = +160 (MEG0113), 120 x 60 each: 0.8 of full scale is 24 px off centre
    QImage image(480, 120, QImage::Format_RGB32);
    image.fill(Qt::white);
    {
        QPainter painter(&image);
        scene->render(&painter, QRectF(0, 0, 480, 120), QRectF(-240, -60, 480, 120));
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

    const QString shot = QDir::temp().filePath(QStringLiteral("test_disp_viewers2_average_layout.svg"));
    view.takeScreenshot(shot);
    QVERIFY(QFile::exists(shot));
    QFile::remove(shot);
    view.clearView();
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

void TestDispViewers2::evokedSetModel_projectionRolesAndAverageMaps()
{
    auto set = QSharedPointer<FiffEvokedSet>::create();
    set->info = *createBrowserTestInfo();
    set->info.bads.clear();
    for (const QString& comment : {QStringLiteral("aud"), QStringLiteral("vis")}) {
        FiffEvoked evoked;
        evoked.comment = comment;
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
        set->evoked.append(evoked);
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
    QCOMPARE(model.getBaselineInfo().first.toFloat(), -0.1f);
    QCOMPARE(colorSpy.size(), 1);
    QCOMPARE(activationSpy.size(), 1);
    QCOMPARE(model.getAverageColor()->value(QStringLiteral("vis")), QColor(Qt::yellow));
    QVERIFY(model.getAverageActivation()->value(QStringLiteral("aud")));

    // Column 1: one row per average type for the butterfly; column 2: whole matrices for the 2D layout
    auto rows = model.data(2, 1).value<QList<AvrTypeRowVector>>();
    QCOMPARE(rows.size(), 2);
    QCOMPARE(rows.at(1).first, QStringLiteral("vis"));
    QCOMPARE(rows.at(1).second, set->evoked.at(1).data.row(2));
    const auto matrices = model.data(0, 2, EvokedSetModelRoles::GetAverageData).value<QList<AvrTypeRowVectorPair>>();
    QCOMPARE(matrices.size(), 2);
    QCOMPARE(matrices.at(0).second.second, 30);
    // Column-major (nchan x nsamples), the layout AverageSceneItem indexes with sample * nchan + channel
    QCOMPARE(matrices.at(0).second.first[5 * 4 + 3], 5.0);
    QVERIFY(!model.data(0, 1, Qt::BackgroundRole).isValid());
    QVERIFY(!model.data(0, 1, Qt::ToolTipRole).isValid());
    QCOMPARE(model.data(0, 0).toStringList(), set->info.ch_names);
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
    set->info.bads = {QStringLiteral("MEG0111")};
    QVERIFY(model.getIsChannelBad(1));
    QVERIFY(!model.getIsChannelBad(0));
    QVERIFY(!model.getIsChannelBad(7));
    set->info.bads.clear();
    model.resetSelection();

    // An active common-mode projector applies to every average once the data is updated
    Eigen::MatrixXd vec(1, 3);
    vec.setConstant(1.0 / std::sqrt(3.0));
    FiffNamedMatrix vectors(1, 3, QStringList(), set->info.ch_names.mid(0, 3), vec);
    model.updateProjection({FiffProj(FIFFV_PROJ_ITEM_FIELD, true, QStringLiteral("common"), vectors)});
    model.updateCompensator(0);
    model.setEvokedSet(set);
    rows = model.data(0, 1).value<QList<AvrTypeRowVector>>();
    for (int s = 0; s < 30; ++s) {
        QVERIFY(std::fabs(rows.at(1).second(s) - 2.0 * std::sin(0.3 * s)) < 1e-12);
    }
    QCOMPARE(model.data(3, 1).value<QList<AvrTypeRowVector>>().at(0).second, set->evoked.at(0).data.row(3));

    // Freezing keeps the projected data; dropping an average keeps its colour for when it returns
    model.toggleFreeze();
    QVERIFY(model.isFreezed());
    model.getAverageColor()->insert(QStringLiteral("vis"), Qt::cyan);
    const FiffEvoked vis = set->evoked.takeLast();
    model.setEvokedSet(set);
    QCOMPARE(model.getNumAverages(), 1);
    const auto frozenRows = model.data(0, 1).value<QList<AvrTypeRowVector>>();
    QCOMPARE(frozenRows.size(), 2);
    QCOMPARE(frozenRows.at(1).first, QStringLiteral("vis"));
    const auto frozenMatrices = model.data(0, 2, EvokedSetModelRoles::GetAverageData).value<QList<AvrTypeRowVectorPair>>();
    QCOMPARE(frozenMatrices.size(), 2);
    QCOMPARE(frozenMatrices.at(1).first, QStringLiteral("vis"));
    QCOMPARE(colorSpy.size(), 2);
    QVERIFY(!model.getAverageColor()->contains(QStringLiteral("vis")));
    model.toggleFreeze();
    QCOMPARE(model.data(0, 1).value<QList<AvrTypeRowVector>>().size(), 1);
    set->evoked.append(vis);
    model.setEvokedSet(set);
    QCOMPARE(model.getAverageColor()->value(QStringLiteral("vis")), QColor(Qt::cyan));
    QCOMPARE(model.getEvokedSet(), set);

    model.setAverageColor(QSharedPointer<QMap<QString, QColor>>::create());
    model.setAverageActivation(QSharedPointer<QMap<QString, bool>>::create());
    QVERIFY(model.getAverageColor()->isEmpty());
    QVERIFY(model.getAverageActivation()->isEmpty());
}

//=============================================================================================================

void TestDispViewers2::channelInfoModel_columnsRolesAndLayoutMapping()
{
    auto info = createBrowserTestInfo();
    info->chs[1].unit = FIFF_UNIT_T_M;
    info->chs[1].chpos.coil_type = FIFFV_COIL_VV_PLANAR_T1;
    info->chs[0].chpos.r0 = Eigen::Vector3f(0.01f, -0.02f, 0.05f);
    FiffChInfo eeg;
    eeg.ch_name = QStringLiteral("EEG-007");
    eeg.kind = FIFFV_EEG_CH;
    eeg.unit = FIFF_UNIT_V;
    info->chs.append(eeg);
    info->ch_names.append(eeg.ch_name);
    info->nchan = 5;

    ChannelInfoModel model(info);
    QSignalSpy mappedSpy(&model, &ChannelInfoModel::channelsMappedToLayout);
    QCOMPARE(model.rowCount(), 5);
    QCOMPARE(model.columnCount(), 13);

    // Layout names drop the kind prefix and separators and re-prefix MEG / EEG; other kinds keep their name
    const QStringList mapped = {"MEG 0111", "MEG 0112", "MEG 0113", "STI014", "EEG 007"};
    QCOMPARE(model.getMappedChannelsList(), mapped);
    QCOMPARE(model.getIndexFromMappedChName(QStringLiteral("EEG 007")), 4);
    QCOMPARE(model.getIndexFromOrigChName(QStringLiteral("MEG0113")), 2);
    QCOMPARE(model.getBadChannelList(), QStringList{QStringLiteral("MEG0112")});

    model.layoutChanged({{QStringLiteral("MEG 0111"), QPointF(1.5, -2.0)}, {QStringLiteral("EEG 007"), QPointF(3, 4)}});
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
    QVERIFY(model.getMappedChannelsList().isEmpty());
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

void TestDispViewers2::channelRhiView_renderToImage()
{
    ChannelDataView view(QStringLiteral("test_disp_viewers2_rhi_image"));
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

void TestDispViewers2::channelRhiView_gpuFrame()
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
    view.setEvents({ChannelRhiView::EventMarker{150, 1, Qt::red, QStringLiteral("event")}});
    view.setAnnotations({ChannelRhiView::AnnotationSpan{160, 170, Qt::blue, QStringLiteral("blink")}});
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
        for (int y : cpuRows) {
            if (!near(gpuRows, y)) {
                qWarning() << mode << "CPU row" << y << "missing on the GPU" << gpuRows;
                return false;
            }
        }
        for (int y : gpuRows) {
            if (!near(cpuRows, y)) {
                qWarning() << mode << "GPU row" << y << "missing in the image" << cpuRows;
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

void TestDispViewers2::layoutScene_zoomPanAndSelect()
{
    QGraphicsView view;
    LayoutScene scene(&view);
    view.setScene(&scene);
    view.resize(200, 200);
    scene.addRect(0, 0, 1000, 1000);
    view.show();
    QCoreApplication::processEvents();

    // Wheel: one notch in zooms by 1.15, one notch out undoes it
    const auto wheel = [&scene](int delta) {
        QGraphicsSceneWheelEvent event(QEvent::GraphicsSceneWheel);
        event.setDelta(delta);
        QApplication::sendEvent(&scene, &event);
    };
    wheel(120);
    QVERIFY(std::fabs(view.transform().m11() - 1.15) < 1e-9);
    wheel(-120);
    QVERIFY(std::fabs(view.transform().m11() - 1.0) < 1e-9);

    // Double-click fits all items into the view
    QGraphicsSceneMouseEvent doubleClick(QEvent::GraphicsSceneMouseDoubleClick);
    doubleClick.setButton(Qt::LeftButton);
    QApplication::sendEvent(&scene, &doubleClick);
    QVERIFY(view.transform().m11() < 0.25);

    // Right-drag pans the scroll bars against the mouse movement
    view.resetTransform();
    view.horizontalScrollBar()->setValue(300);
    view.verticalScrollBar()->setValue(300);
    const auto mouse = [&scene](QEvent::Type type, Qt::MouseButton button, const QPoint& screenPos) {
        QGraphicsSceneMouseEvent event(type);
        event.setButton(button);
        event.setButtons(type == QEvent::GraphicsSceneMouseRelease ? Qt::NoButton : Qt::MouseButtons(button));
        event.setScreenPos(screenPos);
        QApplication::sendEvent(&scene, &event);
    };
    mouse(QEvent::GraphicsSceneMousePress, Qt::RightButton, QPoint(100, 100));
    QCOMPARE(view.dragMode(), QGraphicsView::NoDrag);
    mouse(QEvent::GraphicsSceneMouseMove, Qt::RightButton, QPoint(130, 80));
    QCOMPARE(view.horizontalScrollBar()->value(), 270);
    QCOMPARE(view.verticalScrollBar()->value(), 320);
    mouse(QEvent::GraphicsSceneMouseRelease, Qt::RightButton, QPoint(130, 80));
    mouse(QEvent::GraphicsSceneMouseMove, Qt::NoButton, QPoint(10, 10));
    QCOMPARE(view.horizontalScrollBar()->value(), 270);

    // Left press switches to rubber-band selection
    mouse(QEvent::GraphicsSceneMousePress, Qt::LeftButton, QPoint(10, 10));
    QCOMPARE(view.dragMode(), QGraphicsView::RubberBandDrag);
    mouse(QEvent::GraphicsSceneMouseRelease, Qt::LeftButton, QPoint(10, 10));
}

//=============================================================================================================

void TestDispViewers2::channelDataView_keyboardShortcuts()
{
    QSettings(QStringLiteral("MNECPP")).remove(QStringLiteral("test_disp_viewers2_keys"));
    ChannelDataView view(QStringLiteral("test_disp_viewers2_keys"));
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

void TestDispViewers2::channelRhiView_overlayValues()
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
    const QList<QWidget*> children = rhiView->findChildren<QWidget*>(Qt::FindDirectChildrenOnly);
    QCOMPARE(children.size(), 1);
    QWidget* overlay = children.first();
    overlay->resize(rhiView->size());

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
    const auto paintOverlay = [overlay]() {
        TextRecordingDevice recorder(overlay->size());
        {
            QPainter painter(&recorder);
            overlay->render(&painter);
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
    QCOMPARE(textStartingWith(rec.texts, QStringLiteral("MEG0111")),
             QStringLiteral("MEG0111  0.020 s  %1 mT").arg(QString::number(std::sin(1.0) * 1e3, 'f', 1)));
    rhiView->setClockTimeFormat(true);
    QCOMPARE(textStartingWith(paintOverlay().texts, QStringLiteral("MEG0111")),
             QStringLiteral("MEG0111  00:00.020  %1 mT").arg(QString::number(std::sin(1.0) * 1e3, 'f', 1)));
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

void TestDispViewers2::rtFiffRawView_paintsHidesRowsAndAddsEvents()
{
    RtFiffRawView view(QStringLiteral("test_disp_viewers2_rawview"));
    view.resize(600, 400);
    auto info = createBrowserTestInfo();
    view.init(info);
    QCOMPARE(view.getSamplingFreq(), 1000.0f);
    auto* table = view.findChild<QTableView*>();
    QVERIFY(table != nullptr);
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
    QCOMPARE(view.getScalingMap().value(FIFF_UNIT_T), 2.0f);
    view.setDistanceTimeSpacer(200);
    QCOMPARE(view.getDistanceTimeSpacer(), 200);
    view.triggerInfoChanged({{3.0, Qt::red}}, true, QStringLiteral("STI014"), 1.0);

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
        view.addData({block});
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

    view.showSelectedChannelsOnly({QStringLiteral("MEG0111"), QStringLiteral("STI014")});
    QVERIFY(!table->isRowHidden(0));
    QVERIFY(table->isRowHidden(2));
    QVERIFY(!table->isRowHidden(3));
    view.showSelectedChannelsOnly(info->ch_names);

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
    QVERIFY(info->bads.contains(QStringLiteral("MEG0113")));
    contextAction(table->visualRect(model->index(2, 1)).center(), QStringLiteral("Mark as good"));
    QVERIFY(!info->bads.contains(QStringLiteral("MEG0113")));
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
        QCOMPARE(crosshairSpy.last().at(2).toString(), QStringLiteral("MEG0111"));
        QCOMPARE(crosshairSpy.last().at(0).toFloat(), static_cast<float>(iSample / 1000.0));
        QCOMPARE(crosshairSpy.last().at(1).toFloat(), static_cast<float>(std::sin(0.02 * iSample)));
        QCOMPARE(crosshairSpy.last().at(3).toString(), QStringLiteral("T"));
    }
    const int iBefore = crosshairSpy.size();
    hover(QPoint(xNew, table->visualRect(model->index(2, 1)).center().y()));
    QCOMPARE(crosshairSpy.size(), iBefore + 1);
    QCOMPARE(crosshairSpy.last().at(2).toString(), QStringLiteral("MEG0113"));
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

    const QString shot = QDir::temp().filePath(QStringLiteral("test_disp_viewers2_rawview.png"));
    view.takeScreenshot(shot);
    QVERIFY(QFile::exists(shot));
    QFile::remove(shot);
}

//=============================================================================================================

void TestDispViewers2::frequencySpectrumDelegate_paintsSpectrumAndReadout()
{
    // 4 channels, 101 bins over 0..500 Hz (5 Hz per bin); channel 0 is a peak at 100 Hz (bin 20)
    auto info = createBrowserTestInfo();
    FrequencySpectrumModel model;
    model.setInfo(info);
    // Rows exist before the first spectrum: no data yet
    QCOMPARE(model.rowCount(), 4);
    QVERIFY(!model.data(model.index(0, 1)).isValid());
    model.toggleFreeze(QModelIndex());
    QVERIFY(!model.data(model.index(0, 1)).isValid());
    model.toggleFreeze(QModelIndex());
    Eigen::MatrixXd spectrum = Eigen::MatrixXd::Constant(4, 101, 1.0);
    for (int b = 0; b < 101; ++b) {
        spectrum(0, b) = 1.0 + 9.0 * std::exp(-0.5 * std::pow((b - 20) / 2.0, 2));
    }
    model.addData(spectrum);

    QTableView table;
    table.setModel(&model);
    FrequencySpectrumDelegate delegate(&table);
    table.setItemDelegate(&delegate);

    // Paint row 0 into a 400 x 100 cell; SpectrumView hides the name column, so the plot cell starts at x = 0
    const QRect cell(0, 20, 400, 100);
    auto render = [&]() {
        QImage image(cell.right() + 20, cell.bottom() + 20, QImage::Format_RGB32);
        image.fill(Qt::white);
        QPainter painter(&image);
        QStyleOptionViewItem option;
        option.rect = cell;
        delegate.paint(&painter, option, model.index(0, 1));
        return image;
    };
    auto curveRow = [](const QImage& image, int x) {
        for (int y = 0; y < image.height(); ++y) {
            const QRgb c = image.pixel(x, y);
            if (qBlue(c) > qRed(c) + 60 && qBlue(c) > qGreen(c) + 60) {
                return y;
            }
        }
        return -1;
    };

    QImage image = render();
    // The curve spans the cell width and deviates from the flat level only around the 100 Hz peak (x = 0.2 * 400)
    const int yFlat = curveRow(image, cell.left() + 300);
    QVERIFY2(yFlat >= cell.top() && yFlat <= cell.bottom(), qPrintable(QString::number(yFlat)));
    QCOMPARE(curveRow(image, cell.left() + 2), yFlat);
    QCOMPARE(curveRow(image, cell.right() - 2), yFlat);
    QVERIFY(curveRow(image, cell.left() + 80) != yFlat);
    // More power is drawn higher up, and the peak stays inside the cell
    const int yPeak = curveRow(image, cell.left() + 80);
    QVERIFY2(yPeak >= cell.top() && yPeak < yFlat - 20, qPrintable(QStringLiteral("peak %1 flat %2").arg(yPeak).arg(yFlat)));
    for (int x = cell.left(); x < cell.right(); ++x) {
        for (int y = cell.bottom() + 1; y < image.height(); ++y) {
            QVERIFY2(qBlue(image.pixel(x, y)) <= qRed(image.pixel(x, y)) + 60, qPrintable(QStringLiteral("curve below the cell at %1,%2").arg(x).arg(y)));
        }
    }

    // Grid: 5 linear divisions labelled at 100..400 Hz
    auto hasDarkPixelNear = [](const QImage& image, int x, int y) {
        for (int dx = -2; dx <= 20; ++dx) {
            for (int dy = -12; dy <= 2; ++dy) {
                if (qGray(image.pixel(x + dx, y + dy)) < 100) {
                    return true;
                }
            }
        }
        return false;
    };
    for (int f = 100; f <= 400; f += 100) {
        QVERIFY2(hasDarkPixelNear(image, cell.left() + f * 400 / 500, cell.top()), qPrintable(QString::number(f)));
    }

    // Narrowing the bounds to 50..150 Hz stretches the band over the whole width: the peak moves to the centre
    model.setBoundaries(50.0f, 150.0f);
    QCOMPARE(model.getLowerFrqBound(), 9);
    QCOMPARE(model.getUpperFrqBound(), 31);
    image = render();
    QVERIFY(curveRow(image, cell.left() + 200) != curveRow(image, cell.left() + 2));

    // Mouse read-out over the cell: a vertical marker and the frequency/value text for the hovered row
    model.setBoundaries(0.0f, 500.0f);
    table.resize(500, 300);
    delegate.rcvMouseLoc(0, cell.left() + 80, cell.top() + 50, cell);
    image = render();
    int iMarker = 0;
    for (int y = cell.top(); y < cell.bottom(); ++y) {
        const QRgb c = image.pixel(cell.left() + 80, y);
        if (qGray(c) < 200 && std::abs(qRed(c) - qBlue(c)) < 20) {
            ++iMarker;
        }
    }
    QVERIFY2(iMarker > 30, qPrintable(QString::number(iMarker)));
    QVERIFY(hasDarkPixelNear(image, cell.left() + 88, cell.top() + 42));
    // At exactly 100 Hz the read-out names that bin
    TextRecordingDevice recorder(image.size());
    {
        QPainter painter(&recorder);
        QStyleOptionViewItem option;
        option.rect = cell;
        delegate.paint(&painter, option, model.index(0, 1));
    }
    QVERIFY2(recorder.texts().contains(QStringLiteral("10 [DB], 100 [Hz]")), qPrintable(recorder.texts().join(u'|')));

    // Other rows show no read-out; the name column is rotated text
    QStyleOptionViewItem option;
    option.rect = cell;
    QImage other(cell.right() + 20, cell.bottom() + 20, QImage::Format_RGB32);
    other.fill(Qt::white);
    {
        QPainter painter(&other);
        delegate.paint(&painter, option, model.index(1, 1));
        delegate.paint(&painter, option, model.index(1, 0));
    }
    QVERIFY(!hasDarkPixelNear(other, cell.left() + 88, cell.top() + 42));
    QCOMPARE(delegate.sizeHint(option, model.index(0, 0)), QSize(20, cell.height()));

    // Log scale: decade grid lines
    delegate.setScaleType(1);
    image = render();
    QVERIFY(curveRow(image, cell.left() + 300) >= 0);
}

//=============================================================================================================

void TestDispViewers2::averageSceneItem_paintsScaledClampedCurves()
{
    // Channel 1 of 2, 120 samples, stimulus at sample 30; "aud" ramps to 2x full scale, "vis" is inactive
    Eigen::MatrixXd aud = Eigen::MatrixXd::Zero(2, 120);
    Eigen::MatrixXd vis = Eigen::MatrixXd::Zero(2, 120);
    for (int s = 0; s < 120; ++s) {
        aud(1, s) = (s < 60) ? 0.5e-12 : 2e-12;
        vis(1, s) = -0.5e-12;
    }
    AverageSceneItem item(QStringLiteral("MEG0112"), 1, QPointF(0, 0), FIFFV_MEG_CH, FIFF_UNIT_T, Qt::darkGray);
    item.m_iTotalNumberChannels = 2;
    item.m_firstLastSample = qMakePair(-30, 90);
    item.m_scaleMap = {{FIFF_UNIT_T, 1e-12f}};
    item.m_lAverageData = {{QStringLiteral("aud"), RowVectorPair(aud.data(), 120)},
                           {QStringLiteral("vis"), RowVectorPair(vis.data(), 120)}};
    item.m_qMapAverageColor = {{QStringLiteral("aud"), Qt::blue}};
    item.m_qMapAverageActivation = {{QStringLiteral("aud"), true}, {QStringLiteral("vis"), false}};
    QCOMPARE(item.boundingRect(), QRectF(-60, -30, 120, 60));

    // Render the 120 x 60 item 1:1 into an image whose origin is the item's top-left corner
    auto render = [&item]() {
        QImage image(120, 60, QImage::Format_RGB32);
        image.fill(Qt::white);
        QPainter painter(&image);
        painter.translate(60, 30);
        item.paint(&painter, nullptr, nullptr);
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
    item.m_qMapAverageActivation[QStringLiteral("vis")] = true;
    image = render();
    const QList<int> gray = rowsWhere(image, 45, isGray);
    QVERIFY2(!gray.isEmpty() && std::abs(gray.first() - 45) <= 1, qPrintable(QString::number(gray.value(0, -1))));
    item.setDefaultColor(Qt::green);
    image = render();
    QVERIFY(!rowsWhere(image, 45, [](QRgb c) { return qGreen(c) > 100 && qRed(c) < qGreen(c) - 50 && qBlue(c) < qGreen(c) - 50; }).isEmpty());

    // A bad channel is drawn faded
    item.m_bIsBad = true;
    image = render();
    blue = rowsWhere(image, 45, [](QRgb c) { return qBlue(c) > qRed(c) + 20; });
    QVERIFY(!blue.isEmpty());
    QVERIFY(qRed(image.pixel(45, blue.first())) > 150);
    item.m_bIsBad = false;

    // Press enlarges the label, release restores it, both ask for a scene update
    QSignalSpy updateSpy(&item, &AverageSceneItem::sceneUpdateRequested);
    QGraphicsSceneMouseEvent press(QEvent::GraphicsSceneMousePress);
    item.mousePressEvent(&press);
    QCOMPARE(item.m_iFontTextSize, 6);
    item.mouseReleaseEvent(&press);
    QCOMPARE(item.m_iFontTextSize, 3);
    QCOMPARE(updateSpy.count(), 2);

    // Without data nothing but the label is drawn
    item.m_lAverageData.clear();
    image = render();
    QVERIFY(rowsWhere(image, 90, isRed).isEmpty());
}

//=============================================================================================================

void TestDispViewers2::dipoleFitView_paramsModelsAndFit()
{
    DipoleFitView view;
    auto spin = [&view](const char* name) {
        return view.findChild<QSpinBox*>(QLatin1String(name));
    };
    auto dspin = [&view](const char* name) {
        return view.findChild<QDoubleSpinBox*>(QLatin1String(name));
    };

    // The sphere starts at MNE-C's defaults: origin (0, 0, 40) mm, EEG sphere radius 90 mm
    {
        QSignalSpy defaults(&view, &DipoleFitView::sphereChanged);
        view.requestParams();
        QCOMPARE(defaults.last(), (QList<QVariant>{0.0, 0.0, 40.0, 90.0}));
    }

    // Each control reports its whole parameter group
    QSignalSpy timeSpy(&view, &DipoleFitView::timeChanged);
    QSignalSpy baselineSpy(&view, &DipoleFitView::baselineChanged);
    QSignalSpy modalitySpy(&view, &DipoleFitView::modalityChanged);
    QSignalSpy fittingSpy(&view, &DipoleFitView::fittingChanged);
    QSignalSpy noiseSpy(&view, &DipoleFitView::noiseChanged);
    QSignalSpy regSpy(&view, &DipoleFitView::regChanged);
    QSignalSpy sphereSpy(&view, &DipoleFitView::sphereChanged);
    spin("spinBox_tmin")->setValue(10);
    spin("spinBox_tmax")->setValue(120);
    spin("spinBox_tstep")->setValue(5);
    spin("spinBox_tint")->setValue(2);
    QCOMPARE(timeSpy.count(), 4);
    QCOMPARE(timeSpy.last(), (QList<QVariant>{10, 120, 5, 2}));

    spin("spinBox_bmin")->setValue(-100);
    spin("spinBox_bmax")->setValue(-20);
    QCOMPARE(baselineSpy.last(), (QList<QVariant>{-100, -20}));
    spin("spinBox_bmax")->setValue(-100);
    QCOMPARE(baselineSpy.last(), (QList<QVariant>{1000000, 1000000}));
    spin("spinBox_bmax")->setValue(-20);

    view.findChild<QCheckBox*>(QStringLiteral("checkBox_EEG"))->setChecked(false);
    view.findChild<QCheckBox*>(QStringLiteral("checkBox_MEG"))->setChecked(true);
    QCOMPARE(modalitySpy.last(), (QList<QVariant>{false, true}));

    dspin("doubleSpinBox_dist")->setValue(7.0);
    dspin("doubleSpinBox_grid")->setValue(9.0);
    QCOMPARE(fittingSpy.last(), (QList<QVariant>{7.0, 9.0}));
    dspin("doubleSpinBox_gradnoise")->setValue(4.0);
    dspin("doubleSpinBox_magnoise")->setValue(30.0);
    dspin("doubleSpinBox_eegnoise")->setValue(0.4);
    QCOMPARE(noiseSpy.last(), (QList<QVariant>{4.0, 30.0, 0.4}));
    dspin("doubleSpinBox_gradreg")->setValue(0.2);
    dspin("doubleSpinBox_magreg")->setValue(0.3);
    dspin("doubleSpinBox_eegreg")->setValue(0.4);
    QCOMPARE(regSpy.last(), (QList<QVariant>{0.2, 0.3, 0.4}));
    dspin("doubleSpinBox_orgx")->setValue(1.0);
    dspin("doubleSpinBox_orgy")->setValue(2.0);
    dspin("doubleSpinBox_orgz")->setValue(40.0);
    dspin("doubleSpinBox_rad")->setValue(80.0);
    QCOMPARE(sphereSpy.last(), (QList<QVariant>{1.0, 2.0, 40.0, 80.0}));

    // Requesting the parameters repeats exactly what the controls reported
    const QList<QSignalSpy*> spies = {&timeSpy, &baselineSpy, &modalitySpy, &fittingSpy, &noiseSpy, &regSpy, &sphereSpy};
    QList<QList<QVariant>> lastReported;
    for (QSignalSpy* pSpy : spies) {
        lastReported << pSpy->last();
        pSpy->clear();
    }
    view.requestParams();
    for (int i = 0; i < spies.size(); ++i) {
        QCOMPARE(spies.at(i)->count(), 1);
        QCOMPARE(spies.at(i)->last(), lastReported.at(i));
    }

    // Model lists; the measurement selection derives the fit name
    QSignalSpy measSpy(&view, &DipoleFitView::selectedMeas);
    QSignalSpy bemSpy(&view, &DipoleFitView::selectedBem);
    QSignalSpy fitSpy(&view, &DipoleFitView::performDipoleFit);
    auto* pMeas = view.findChild<QComboBox*>(QStringLiteral("comboBox_meas"));
    auto* pBem = view.findChild<QComboBox*>(QStringLiteral("comboBox_bem"));
    auto* pName = view.findChild<QLineEdit*>(QStringLiteral("lineEdit_name"));
    view.addMeas(QStringLiteral("sample_audvis-ave.fif"));
    view.addBem(QStringLiteral("sample-5120-bem-sol.fif"));
    view.addMri(QStringLiteral("all-trans.fif"));
    view.addNoise(QStringLiteral("sample_audvis-cov.fif"));
    pMeas->setCurrentIndex(1);
    QCOMPARE(measSpy.last().at(0).toString(), QStringLiteral("sample_audvis-ave.fif"));
    QVERIFY(pName->text().startsWith(QStringLiteral("Dipole Fit - sample_audvis - ")));
    pBem->setCurrentIndex(1);
    QCOMPARE(bemSpy.last().at(0).toString(), QStringLiteral("sample-5120-bem-sol.fif"));
    view.findChild<QPushButton*>(QStringLiteral("pushButton_fit"))->click();
    QCOMPARE(fitSpy.last().at(0).toString(), pName->text());

    view.removeModel(QStringLiteral("sample-5120-bem-sol.fif"), 2);
    QCOMPARE(pBem->findText(QStringLiteral("sample-5120-bem-sol.fif")), -1);
    view.removeModel(QStringLiteral("all-trans.fif"), 3);
    view.removeModel(QStringLiteral("sample_audvis-cov.fif"), 4);
    view.removeModel(QStringLiteral("sample_audvis-ave.fif"), 1);
    QCOMPARE(pMeas->count(), 1);
    view.removeModel(QStringLiteral("x"), 9);
    view.clearView();
    QCOMPARE(pMeas->count(), 0);
    QVERIFY(pName->text().isEmpty());
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

void TestDispViewers2::control3dView_flagsSignalsAndTree()
{
    Control3DView view(QString{}, nullptr, {QStringLiteral("Data")});
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
    view.setFlags({QStringLiteral("View"), QStringLiteral("Light")});
    view.setFlags({QStringLiteral("View"), QStringLiteral("Light")});
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
    view.setFlags({QStringLiteral("Data")});
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

void TestDispViewers2::rtFiffRawViewDelegate_paintsSignalCursorAndThreshold()
{
    // 100-sample buffer; MEG0111 holds +0.5 of full scale, the stim channel steps to 4
    RtFiffRawViewModel model;
    auto info = createBrowserTestInfo();
    info->bads.clear();
    model.setFiffInfo(info);
    model.setSamplingInfo(100.0f, 1, true);
    model.setScaling({{FIFF_UNIT_T, 1e-12f}, {FIFFV_STIM_CH, 5.0f}});
    model.triggerInfoChanged({{4.0, Qt::red}}, true, QStringLiteral("STI014"), 2.5);
    Eigen::MatrixXd block = Eigen::MatrixXd::Zero(4, 60);
    block.row(0).setConstant(0.5e-12);
    block.row(3).tail(30).setConstant(4.0);
    model.addData({block});
    QCOMPARE(model.getDetectedTriggers(), (QList<QPair<int, double>>{{30, 4.0}}));

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
    info->bads = {QStringLiteral("MEG0111")};
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

void TestDispViewers2::spectrumView_scalesBoundsAndPaints()
{
    // 4 channels at 1000 Hz, 100 frequency bins of 5 Hz up to Nyquist (500 Hz)
    QSharedPointer<FiffInfo> info = createBrowserTestInfo();
    Eigen::MatrixXd psd(4, 100);
    for (int b = 0; b < 100; ++b) {
        psd.col(b).setConstant(1.0 + 0.1 * b);
    }
    psd(1, 20) = 50.0;

    for (const int scaleType : {0, 1}) {
        SpectrumView view(QStringLiteral("test_disp_viewers2_spectrum"));
        view.resize(500, 600);
        view.setBoundaries(1, 40);
        view.addData(psd);
        view.init(info, scaleType);
        view.addData(psd);
        auto* table = view.findChild<QTableView*>();
        QVERIFY(table != nullptr);
        auto* model = qobject_cast<FrequencySpectrumModel*>(table->model());
        QVERIFY(model != nullptr);
        QCOMPARE(model->rowCount(), 4);
        QCOMPARE(model->getNumStems(), 100);
        QCOMPARE(model->getInfo(), info);
        // Linear: bin k at 5k Hz, normalised by the last bin (495 Hz); log: log10(f + 1) / log10(496)
        const Eigen::RowVectorXd scale = model->getFreqScale();
        const double expected20 = scaleType ? std::log10(101.0) / std::log10(496.0) : 100.0 / 495.0;
        QVERIFY(std::fabs(scale[20] - expected20) < 1e-12);
        QCOMPARE(scale[99], 1.0);

        // 20..200 Hz: the last bin below 20 Hz and the first above 200 Hz bound the scale
        view.setBoundaries(20, 200);
        const double nf = 500.0;
        QVERIFY(scale[model->getLowerFrqBound()] * nf < 20.0);
        QVERIFY(scale[model->getLowerFrqBound() + 1] * nf >= 20.0);
        QVERIFY(scale[model->getUpperFrqBound()] * nf > 200.0);
        QVERIFY(scale[model->getUpperFrqBound() - 1] * nf <= 200.0);
        const Eigen::RowVectorXd bound = model->getFreqScaleBound();
        QCOMPARE(bound[model->getLowerFrqBound()], 0.0);
        QCOMPARE(bound[model->getUpperFrqBound()], 1.0);

        QCOMPARE(model->data(model->index(2, 0)).toString(), QStringLiteral("MEG0113"));
        QCOMPARE(model->data(model->index(1, 1)).value<Eigen::RowVectorXd>(), psd.row(1));
        QCOMPARE(model->headerData(1, Qt::Horizontal).toString(), QStringLiteral("data plot"));
        QCOMPARE(model->headerData(3, Qt::Vertical).toString(), QStringLiteral("STI014"));

        // Freeze keeps the old spectrum while new data arrives
        model->toggleFreeze(QModelIndex());
        view.addData(psd * 2.0);
        QCOMPARE(model->data(model->index(1, 1)).value<Eigen::RowVectorXd>(), psd.row(1));
        model->toggleFreeze(QModelIndex());
        QCOMPARE(model->data(model->index(1, 1)).value<Eigen::RowVectorXd>(), Eigen::RowVectorXd(psd.row(1) * 2.0));

        model->selectRows({1, 3, 7});
        QCOMPARE(model->rowCount(), 2);
        QCOMPARE(model->data(model->index(1, 0)).toString(), QStringLiteral("STI014"));
        model->resetSelection();

        // Paint the plot rows and move the mouse over the first one to draw the read-out
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        QVERIFY(!view.grab().isNull());
        QMouseEvent move(QEvent::MouseMove, QPointF(250, 70), table->viewport()->mapToGlobal(QPointF(250, 70)),
                         Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(table->viewport(), &move);
        QVERIFY(!view.grab().isNull());
        QCOMPARE(table->currentIndex().row(), 0);
    }
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

void TestDispViewers2::bidsView_treeMovesAndSelection()
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
        return view.findChildren<QMenu*>().last();
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

void TestDispViewers2::projectSettingsView_projectsSubjectsAndTimer()
{
    QTemporaryDir root;
    QVERIFY(root.isValid());
    const QString sRoot = root.path();
    for (const QString& sSub : {QStringLiteral("ProjA/Sub1"), QStringLiteral("ProjA/Sub2"), QStringLiteral("ProjB/SubX")}) {
        QVERIFY(QDir().mkpath(sRoot + QLatin1Char('/') + sSub));
    }

    ProjectSettingsView view(QString(), sRoot, QStringLiteral("ProjA"), QStringLiteral("Sub2"), QString());
    view.show();
    auto* pProjects = view.findChild<QComboBox*>(QStringLiteral("m_qComboBox_ProjectSelection"));
    auto* pSubjects = view.findChild<QComboBox*>(QStringLiteral("m_qComboBox_SubjectSelection"));
    auto* pParadigm = view.findChild<QLineEdit*>(QStringLiteral("m_qLineEditParadigm"));
    auto* pFileName = view.findChild<QLineEdit*>(QStringLiteral("m_qLineEditFileName"));
    QVERIFY(pProjects && pSubjects && pParadigm && pFileName);

    auto items = [](QComboBox* pBox) {
        QStringList list;
        for (int i = 0; i < pBox->count(); ++i) {
            list << pBox->itemText(i);
        }
        return list;
    };

    // Initial scan picks up both projects and keeps the requested subject.
    QCOMPARE(items(pProjects), QStringList({"ProjA", "ProjB"}));
    QCOMPARE(pProjects->currentText(), QStringLiteral("ProjA"));
    QCOMPARE(items(pSubjects), QStringList({"Sub1", "Sub2"}));
    QCOMPARE(pSubjects->currentText(), QStringLiteral("Sub2"));

    // File name: <root>/<project>/<subject>/<stamp>_<subject>[_<paradigm>]_raw.fif
    QString sFile = view.getCurrentFileName();
    QVERIFY(sFile.startsWith(sRoot + QStringLiteral("/ProjA/Sub2/")));
    QVERIFY(sFile.endsWith(QStringLiteral("_Sub2_raw.fif")));
    QCOMPARE(pFileName->text(), sFile);

    QSignalSpy paradigmSpy(&view, &ProjectSettingsView::newParadigm);
    QSignalSpy fileSpy(&view, &ProjectSettingsView::fileNameChanged);
    pParadigm->setText(QStringLiteral("Rest"));
    QCOMPARE(paradigmSpy.last().at(0).toString(), QStringLiteral("Rest"));
    QVERIFY(fileSpy.last().at(0).toString().endsWith(QStringLiteral("_Sub2_Rest_raw.fif")));
    view.triggerFileNameUpdate();
    QVERIFY(pFileName->text().endsWith(QStringLiteral("_Sub2_Rest_raw.fif")));

    // Switching project rescans subjects and falls back to the first one.
    QSignalSpy projectSpy(&view, &ProjectSettingsView::newProject);
    QSignalSpy subjectSpy(&view, &ProjectSettingsView::newSubject);
    pProjects->setCurrentText(QStringLiteral("ProjB"));
    QCOMPARE(projectSpy.last().at(0).toString(), QStringLiteral("ProjB"));
    QCOMPARE(items(pSubjects), QStringList({"SubX"}));
    QCOMPARE(subjectSpy.last().at(0).toString(), QStringLiteral("SubX"));
    QVERIFY(view.getCurrentFileName().startsWith(sRoot + QStringLiteral("/ProjB/SubX/")));

    // Adding a project creates its folder and selects it.
    answerNextInputDialog(QStringLiteral("ProjC"));
    view.findChild<QPushButton*>(QStringLiteral("m_qPushButtonNewProject"))->click();
    QVERIFY(QDir(sRoot + QStringLiteral("/ProjC")).exists());
    QCOMPARE(items(pProjects), QStringList({"ProjA", "ProjB", "ProjC"}));
    QCOMPARE(pProjects->currentText(), QStringLiteral("ProjC"));
    QCOMPARE(projectSpy.last().at(0).toString(), QStringLiteral("ProjC"));
    QCOMPARE(pSubjects->count(), 0);
    QVERIFY(pFileName->text().startsWith(sRoot + QStringLiteral("/ProjC/")));

    // Adding a subject creates its folder below the current project and selects it.
    answerNextInputDialog(QStringLiteral("SubNew"));
    view.findChild<QPushButton*>(QStringLiteral("m_qPushButtonNewSubject"))->click();
    QVERIFY(QDir(sRoot + QStringLiteral("/ProjC/SubNew")).exists());
    QCOMPARE(items(pSubjects), QStringList({"SubNew"}));
    QCOMPARE(pSubjects->currentText(), QStringLiteral("SubNew"));
    QVERIFY(view.getCurrentFileName().startsWith(sRoot + QStringLiteral("/ProjC/SubNew/")));

    // ... also when the project already has subjects sorting before the new one.
    pProjects->setCurrentText(QStringLiteral("ProjA"));
    answerNextInputDialog(QStringLiteral("Sub3"));
    view.findChild<QPushButton*>(QStringLiteral("m_qPushButtonNewSubject"))->click();
    QVERIFY(QDir(sRoot + QStringLiteral("/ProjA/Sub3")).exists());
    QCOMPARE(items(pSubjects), QStringList({"Sub1", "Sub2", "Sub3"}));
    QCOMPARE(pSubjects->currentText(), QStringLiteral("Sub3"));
    QCOMPARE(subjectSpy.last().at(0).toString(), QStringLiteral("Sub3"));
    QVERIFY(view.getCurrentFileName().startsWith(sRoot + QStringLiteral("/ProjA/Sub3/")));

    // Cancelled add dialog changes nothing.
    answerNextModal([](QWidget* pModal) { qobject_cast<QDialog*>(pModal)->reject(); });
    view.findChild<QPushButton*>(QStringLiteral("m_qPushButtonNewProject"))->click();
    QCOMPARE(pProjects->count(), 3);

    // Delete subject: "Keep data" and a declined confirmation keep the folder, confirming removes it.
    pProjects->setCurrentText(QStringLiteral("ProjA"));
    pSubjects->setCurrentText(QStringLiteral("Sub1"));
    auto* pDeleteSubject = view.findChild<QPushButton*>(QStringLiteral("m_qPushButtonDeleteSubject"));
    clickNextMessageBoxButton(QStringLiteral("Keep data"));
    pDeleteSubject->click();
    QVERIFY(QDir(sRoot + QStringLiteral("/ProjA/Sub1")).exists());

    answerNextModal([](QWidget* pModal) {
        clickNextMessageBoxButton(QStringLiteral("No"));
        auto* pBox = qobject_cast<QMessageBox*>(pModal);
        for (QAbstractButton* pButton : pBox->buttons()) {
            if (pButton->text() == QStringLiteral("Delete data")) {
                pButton->click();
            }
        }
    });
    pDeleteSubject->click();
    QVERIFY(QDir(sRoot + QStringLiteral("/ProjA/Sub1")).exists());

    answerNextModal([](QWidget* pModal) {
        clickNextMessageBoxButton(QStringLiteral("Yes"));
        auto* pBox = qobject_cast<QMessageBox*>(pModal);
        for (QAbstractButton* pButton : pBox->buttons()) {
            if (pButton->text() == QStringLiteral("Delete data")) {
                pButton->click();
            }
        }
    });
    pDeleteSubject->click();
    QVERIFY(!QDir(sRoot + QStringLiteral("/ProjA/Sub1")).exists());
    QVERIFY(QDir(sRoot + QStringLiteral("/ProjA/Sub2")).exists());
    QCOMPARE(items(pSubjects), QStringList({"Sub2", "Sub3"}));

    // Delete project removes the whole tree.
    pProjects->setCurrentText(QStringLiteral("ProjB"));
    answerNextModal([](QWidget* pModal) {
        clickNextMessageBoxButton(QStringLiteral("Yes"));
        auto* pBox = qobject_cast<QMessageBox*>(pModal);
        for (QAbstractButton* pButton : pBox->buttons()) {
            if (pButton->text() == QStringLiteral("Delete data")) {
                pButton->click();
            }
        }
    });
    view.findChild<QPushButton*>(QStringLiteral("m_qPushButtonDeleteProject"))->click();
    QVERIFY(!QDir(sRoot + QStringLiteral("/ProjB")).exists());
    QCOMPARE(items(pProjects), QStringList({"ProjA", "ProjC"}));

    // Recording timer: spin boxes set the total, elapsed time counts up and down.
    QSignalSpy timerSpy(&view, &ProjectSettingsView::timerChanged);
    view.findChild<QSpinBox*>(QStringLiteral("m_spinBox_hours"))->setValue(1);
    view.findChild<QSpinBox*>(QStringLiteral("m_spinBox_min"))->setValue(2);
    view.findChild<QSpinBox*>(QStringLiteral("m_spinBox_sec"))->setValue(3);
    QCOMPARE(timerSpy.last().at(0).toInt(), 3723000);
    auto* pToGo = view.findChild<QLabel*>(QStringLiteral("m_label_timeToGo"));
    auto* pPassed = view.findChild<QLabel*>(QStringLiteral("m_label_timePassed"));
    QCOMPARE(pToGo->text(), QStringLiteral("01:02:03"));
    view.setRecordingElapsedTime(1000);
    QCOMPARE(pToGo->text(), QStringLiteral("01:02:02"));
    QCOMPARE(pPassed->text(), QStringLiteral("00:00:01"));
    view.setRecordingElapsedTime(3723000 - 200);
    QCOMPARE(pPassed->text(), QStringLiteral("01:02:03"));

    QSignalSpy timerStateSpy(&view, &ProjectSettingsView::recordingTimerStateChanged);
    auto* pUseTimer = view.findChild<QCheckBox*>(QStringLiteral("m_checkBox_useRecordingTimer"));
    pUseTimer->setChecked(!pUseTimer->isChecked());
    QCOMPARE(timerStateSpy.count(), 1);
    QCOMPARE(timerStateSpy.last().at(0).toBool(), pUseTimer->isChecked());

    // Optional UI parts can be hidden and shown again.
    view.hideFileNameUi();
    view.hideParadigmUi();
    QVERIFY(pFileName->isHidden());
    QVERIFY(pParadigm->isHidden());
    view.showFileNameUi();
    view.showParadigmUi();
    QVERIFY(!pFileName->isHidden());
    QVERIFY(!pParadigm->isHidden());
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
