//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_viz_qt_viewers.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Tests for the disp viewer widget classes: construction, getter/setter round-trips,
 *           settings persistence, GuiMode/ProcessingMode switching, and clearView.
 *           Covers: ScalingView, PanelWidget (compensators, projectors), ModalitySelectionView,
 *           MinimumNormSettingsView, FilterSettingsView, FilterDesignView,
 *           SpectrumSettingsView, AveragingSettingsView, CovarianceSettingsView,
 *           ArtifactSettingsView, SettingsPanel, SpharaSettingsView, TfSettingsView, ConnectivitySettingsView,
 *           TriggerDetectionView, ProgressView, QuickControlView, and MultiView.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/viz/backends/qt/viewers/abstractview.h>
#include <mne/viz/backends/qt/viewers/scalingview.h>
#include <mne/viz/backends/qt/kit/panelwidget.h>
#include <mne/viz/panels/compensator_panel.h>
#include <mne/viz/panels/forward_panel.h>
#include <mne/viz/figures/theme.h>
#include <mne/viz/panels/projectors_panel.h>
#include <mne/viz/backends/qt/viewers/modalityselectionview.h>
#include <mne/viz/backends/qt/viewers/minimumnormsettingsview.h>
#include <mne/viz/backends/qt/viewers/filtersettingsview.h>
#include <mne/viz/backends/qt/viewers/filterdesignview.h>
#include <mne/viz/backends/qt/viewers/spectrumsettingsview.h>
#include <mne/viz/backends/qt/viewers/averagingsettingsview.h>
#include <mne/viz/backends/qt/viewers/artifactsettingsview.h>
#include <mne/viz/backends/qt/viewers/covariancesettingsview.h>
#include <mne/viz/backends/qt/viewers/spharasettingsview.h>
#include <mne/viz/backends/qt/viewers/tfsettingsview.h>
#include <mne/viz/backends/qt/viewers/connectivitysettingsview.h>
#include <mne/viz/backends/qt/viewers/triggerdetectionview.h>
#include <mne/viz/backends/qt/viewers/progressview.h>
#include <mne/viz/backends/qt/viewers/quickcontrolview.h>
#include <mne/viz/backends/qt/viewers/multiview.h>
#include <mne/viz/backends/qt/kit/settingspanel.h>

#include <mne/fiff/fiff_ch_info.h>
#include <mne/fiff/fiff_proj.h>
#include <mne/fiff/fiff_ctf_comp.h>
#include <mne/fiff/fiff_evoked_set.h>
#include <mne/fiff/fiff_constants.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QApplication>
#include <QStringList>
#include <QMap>
#include <QCheckBox>
#include <QFontInfo>
#include <QToolButton>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QSignalSpy>
#include <QSlider>
#include <QSpinBox>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;
using namespace mne::viz::qt;
using namespace mne;

//=============================================================================================================
/**
 * DECLARE CLASS TestVizQtViewers
 *
 * @brief The TestVizQtViewers class tests the disp viewer widget classes under an offscreen platform.
 *
 */
class TestVizQtViewers : public QObject
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
     * Verifies that PanelWidget shows the compensator panel's check boxes once the compensators arrive, that a
     * click chooses one grade exclusively, and that the choice is restored from QSettings by a new widget.
     */
    void panelWidget_compensators();

    //=========================================================================================================
    /**
     * Verifies that PanelWidget shows one check box per projector and the "Enable all" box, which switches all.
     */
    void panelWidget_projectors();

    //=========================================================================================================
    /**
     * Verifies that PanelWidget gives its panel the look of viz::FormView: theme style sheet, font and status
     * chips in the tone colours.
     */
    void panelWidget_theme();

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
    void settingsPanel_buildsAndSyncsWidgets();

    //=========================================================================================================
    /**
     * Verifies that SettingsPanel builds no widget for flags, hides group boxes without a shown field and stores
     * only persistent fields.
     */
    void settingsPanel_flagsGroupsAndPersistence();

    //=========================================================================================================
    /**
     * Verifies the Info, Directory and Color widgets of SettingsPanel and that added and removed fields rebuild
     * the rows.
     */
    void settingsPanel_infoColorDirectoryAndRebuild();

    //=========================================================================================================
    /**
     * Verifies the form-built AveragingSettingsView: widgets per field, values both ways, signals, mode
     * visibility and the QSettings keys.
     */
    void averagingSettingsView_formPanel();

    //=========================================================================================================
    /**
     * Verifies the form-built FilterSettingsView: widgets per field, the design view in sync, the activation
     * signal, the clinical mode and the QSettings keys.
     */
    void filterSettingsView_formPanel();

    //=========================================================================================================
    /**
     * Verifies the form-built ArtifactSettingsView: rows per present channel type, composed thresholds, the
     * threshold signal and the QSettings keys.
     */
    void artifactSettingsView_formPanel();

    //=========================================================================================================
    /**
     * Verifies the form-built TfSettingsView: spin boxes for trial and row number and their signal.
     */
    void tfSettingsView_formPanel();

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
     * Verifies the form-built SpharaSettingsView: activation and option signals and the relabelled fields per system.
     */
    void spharaSettingsView_formPanel();

    //=========================================================================================================
    /**
     * Verifies the form-built MinimumNormSettingsView: method, trigger type, time point and CMNE checkpoint.
     */
    void minimumNormSettingsView_formPanel();

    //=========================================================================================================
    /**
     * Verifies the form-built ConnectivitySettingsView: its signals and the persisted values.
     */
    void connectivitySettingsView_formPanel();

    //=========================================================================================================
    /**
     * Verifies the form-built TriggerDetectionView: its fields, custom widgets and that it reacts only after init().
     */
    void triggerDetectionView_formPanel();

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

void TestVizQtViewers::initTestCase()
{
    QApplication::processEvents();
}

//=============================================================================================================

void TestVizQtViewers::cleanupTestCase()
{
}

//=============================================================================================================

void TestVizQtViewers::scalingView_constructAndGetSet()
{
    // Free function: getDefaultScalingValue(kind, unit)
    float megMagDefault = mne::viz::qt::getDefaultScalingValue(FIFFV_MEG_CH, FIFF_UNIT_T);
    float megGradDefault = mne::viz::qt::getDefaultScalingValue(FIFFV_MEG_CH, FIFF_UNIT_T_M);
    float eegDefault = mne::viz::qt::getDefaultScalingValue(FIFFV_EEG_CH, FIFF_UNIT_V);
    QVERIFY(megMagDefault > 0.0f);
    QVERIFY(megGradDefault > 0.0f);
    QVERIFY(eegDefault > 0.0f);

    ScalingView view("test_viz_qt_viewers");

    // getScaleMap may be empty until channels are added — call must not crash
    std::map<int, float> scaleMap = view.getScaleMap();
    Q_UNUSED(scaleMap);

    // setScaleMap round-trip
    std::map<int, float> customMap;
    customMap[FIFFV_MEG_CH] = 1e-12f;
    customMap[FIFFV_EEG_CH] = 1e-6f;
    view.setScaleMap(customMap);
    std::map<int, float> returned = view.getScaleMap();
    QVERIFY(qFuzzyCompare(returned.at(FIFFV_MEG_CH), 1e-12f));
    QVERIFY(qFuzzyCompare(returned.at(FIFFV_EEG_CH), 1e-6f));

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

void TestVizQtViewers::panelWidget_compensators()
{
    const QString path = QStringLiteral("test_viz_qt_viewers_compensators_%1").arg(QCoreApplication::applicationPid());
    std::vector<FiffCtfComp> comps(2);
    comps[0].kind = 1;
    comps[1].kind = 3;
    std::vector<int> chosen;
    {
        auto panel = std::make_shared<CompensatorPanel>();
        panel->set_selection_callback([&chosen](int to) {
            chosen.push_back(to);
        });
        PanelWidget widget(panel, path);
        QCOMPARE(widget.windowTitle(), QStringLiteral("Compensators"));
        QCOMPARE(widget.width(), 330);
        QVERIFY(!widget.findChild<QCheckBox*>(QString::fromStdString(CompensatorPanel::field_key(1))));

        // The check boxes appear with the data; checking one unchecks the other
        panel->set_compensators(comps);
        auto* first = widget.findChild<QCheckBox*>(QString::fromStdString(CompensatorPanel::field_key(1)));
        auto* third = widget.findChild<QCheckBox*>(QString::fromStdString(CompensatorPanel::field_key(3)));
        QVERIFY(first && third);
        first->click();
        third->click();
        QVERIFY(!first->isChecked() && third->isChecked());
        QCOMPARE(chosen, std::vector<int>({1, 3}));
        third->click();
        QCOMPARE(chosen.back(), 0);
        third->click();
        widget.setGuiMode(AbstractView::GuiMode::Clinical);
        QCOMPARE(panel->gui_mode(), Panel::GuiMode::Clinical);
    }

    // A new widget restores the chosen grade and reports it with the data
    auto restored = std::make_shared<CompensatorPanel>();
    PanelWidget widget(restored, path);
    chosen.clear();
    restored->set_selection_callback([&chosen](int to) {
        chosen.push_back(to);
    });
    restored->set_compensators(comps);
    QCOMPARE(restored->selected(), 3);
    QCOMPARE(chosen, std::vector<int>({3}));
    QSettings(QStringLiteral("MNECPP")).remove(path);
}

//=============================================================================================================

void TestVizQtViewers::panelWidget_projectors()
{
    auto panel = std::make_shared<ProjectorsPanel>();
    std::vector<std::vector<bool>> reported;
    panel->set_selection_callback([&reported](const std::vector<FiffProj>& projs) {
        std::vector<bool> active;
        active.reserve(projs.size());
        for (const FiffProj& proj : projs) {
            active.push_back(proj.active);
        }
        reported.push_back(active);
    });
    PanelWidget widget(panel);

    FiffProj eeg;
    eeg.kind = FIFFV_PROJ_ITEM_EEG_AVREF;
    eeg.active = true;
    eeg.desc = "Average EEG reference";
    FiffProj ecg = eeg;
    ecg.active = false;
    ecg.desc = "ECG";
    panel->set_projectors({eeg, ecg});
    QCOMPARE(reported.back(), std::vector<bool>({true, false}));

    auto* box = widget.findChild<QCheckBox*>(QString::fromStdString(ProjectorsPanel::field_key(1)));
    auto* all = widget.findChild<QCheckBox*>(QString::fromStdString(ProjectorsPanel::kAllKey));
    QVERIFY(box && all);
    QCOMPARE(box->text(), QStringLiteral("ECG"));
    QVERIFY(!all->isChecked());
    box->click();
    QVERIFY(all->isChecked());
    QCOMPARE(reported.back(), std::vector<bool>({true, true}));
    all->click();
    QCOMPARE(reported.back(), std::vector<bool>({false, false}));
    QVERIFY(!box->isChecked());
}

//=============================================================================================================

void TestVizQtViewers::panelWidget_theme()
{
    auto panel = std::make_shared<ForwardPanel>();
    PanelWidget widget(panel);
    const viz::Theme theme = viz::Theme::light();
    QVERIFY(widget.styleSheet().contains(QColor(theme.accent.r, theme.accent.g, theme.accent.b).name()));
    QCOMPARE(widget.font().pixelSize(), static_cast<int>(theme.font_size));
    // The bundled typeface reaches the children, status chips included
    QCOMPARE(QFontInfo(widget.findChild<QCheckBox*>()->font()).family(), QString::fromStdString(theme.font_family));
    panel->set_status(static_cast<int>(ForwardPanel::Status::Finished));
    auto* status = qobject_cast<QLabel*>(widget.formPanel()->editor("status"));
    QVERIFY(status && status->styleSheet().contains(QStringLiteral("rgb(%1, %2, %3)").arg(theme.positive.r).arg(theme.positive.g).arg(theme.positive.b)));
    QCOMPARE(QFontInfo(status->font()).family(), QString::fromStdString(theme.font_family));

    // A dark theme recolours the chips
    widget.setTheme(viz::Theme::dark());
    const viz::Theme dark = viz::Theme::dark();
    QVERIFY(status->styleSheet().contains(QStringLiteral("rgb(%1, %2, %3)").arg(dark.positive.r).arg(dark.positive.g).arg(dark.positive.b)));
    if (const QByteArray shot = qgetenv("MNE_PANEL_SHOT"); !shot.isEmpty()) {
        widget.setTheme(theme);
        widget.resize(360, 640);
        widget.grab().save(QString::fromLocal8Bit(shot));
    }
}

//=============================================================================================================

void TestVizQtViewers::modalitySelectionView_setAndGet()
{
    FiffChInfo megCh;
    megCh.kind = FIFFV_MEG_CH;
    megCh.ch_name = "MEG 0001";
    megCh.unit = FIFF_UNIT_T;

    FiffChInfo eegCh;
    eegCh.kind = FIFFV_EEG_CH;
    eegCh.ch_name = "EEG 001";
    eegCh.unit = FIFF_UNIT_V;

    std::vector<FiffChInfo> chList;
    chList.push_back(megCh);
    chList.push_back(eegCh);

    ModalitySelectionView view(chList, "test_viz_qt_viewers");

    std::map<std::string, bool> modalMap = view.getModalityMap();
    QVERIFY(!modalMap.empty());

    // Flip all to false and round-trip
    std::map<std::string, bool> allOff;
    for (auto it = modalMap.cbegin(); it != modalMap.cend(); ++it)
        allOff[it->first] = false;
    view.setModalityMap(allOff);

    std::map<std::string, bool> returned = view.getModalityMap();
    for (auto it = returned.cbegin(); it != returned.cend(); ++it)
        QVERIFY(!it->second);

    view.setGuiMode(AbstractView::GuiMode::Research);
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    view.saveSettings();
    view.loadSettings();
    view.clearView();

    QApplication::processEvents();
}

//=============================================================================================================

void TestVizQtViewers::minimumNormSettingsView_lifecycle()
{
    MinimumNormSettingsView view("test_viz_qt_viewers");

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

void TestVizQtViewers::filterSettingsView_lifecycle()
{
    FilterSettingsView view("test_viz_qt_viewers");

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

void TestVizQtViewers::filterDesignView_setAndGet()
{
    FilterDesignView view("test_viz_qt_viewers");

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

void TestVizQtViewers::spectrumSettingsView_boundaries()
{
    SpectrumSettingsView view("test_viz_qt_viewers");

    // Typical MEG/EEG frequency range at 600 Hz sfreq
    view.setBoundaries(600.0f, 0.1f, 100.0f);
    QCOMPARE(view.getLowerBound(), 0.1f);
    QCOMPARE(view.getUpperBound(), 100.0f);
    // The sliders run in steps of 1 mHz up to the Nyquist frequency; the lower bound stays below the upper one
    auto* lower = view.findChild<QSlider*>(QStringLiteral("lower"));
    auto* upper = view.findChild<QSlider*>(QStringLiteral("upper"));
    QVERIFY(lower && upper);
    QCOMPARE(upper->maximum(), 300000);
    QSignalSpy changed(&view, &SpectrumSettingsView::settingsChanged);
    upper->setValue(50000);
    QCOMPARE(view.getUpperBound(), 50.0f);
    QVERIFY(changed.count() >= 1);
    lower->setValue(80000);
    QCOMPARE(view.getLowerBound(), 50.0f);

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

void TestVizQtViewers::averagingSettingsView_setAndGet()
{
    std::map<std::string, int> stimMap;
    stimMap["STI 014"] = 0;
    stimMap["STI 001"] = 1;

    AveragingSettingsView view("test_viz_qt_viewers", stimMap);

    QVERIFY(view.getNumAverages() > 0);
    QVERIFY(view.getPreStimMSeconds() >= 0);
    QVERIFY(view.getPostStimMSeconds() >= 0);
    QVERIFY(view.getStimChannelIdx() >= 0);
    Q_UNUSED(view.getCurrentStimCh());
    Q_UNUSED(view.getCurrentSelectGroup());
    Q_UNUSED(view.getBaselineFromSeconds());
    Q_UNUSED(view.getBaselineToSeconds());

    // Update stim channels
    std::map<std::string, int> newStimMap;
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

void TestVizQtViewers::covarianceSettingsView_lifecycle()
{
    CovarianceSettingsView view("test_viz_qt_viewers");
    auto* samples = view.findChild<QSpinBox*>(QStringLiteral("samples"));
    QVERIFY(samples);
    QCOMPARE(samples->minimum(), 600);
    QCOMPARE(samples->maximum(), 36000);
    QSignalSpy changed(&view, &CovarianceSettingsView::samplesChanged);
    samples->setValue(1200);
    QCOMPARE(changed.count(), 1);
    QCOMPARE(changed.at(0).at(0).toInt(), 1200);
    view.setMinSamples(100);
    QCOMPARE(samples->maximum(), 6000);
    view.setCurrentSamples(9000);
    QCOMPARE(samples->value(), 6000);

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

void TestVizQtViewers::settingsPanel_buildsAndSyncsWidgets()
{
    auto form = std::make_shared<viz::SettingsForm>("Test panel");
    form->add_int("trials", "Trials", 10, 1, 100, 1, " trials", "General");
    form->add_double("prestim", "Pre-stimulus", 0.1, 0.0, 1.0, 0.05, 2, " s", "General");
    form->add_bool("baseline", "Baseline", false, "Baseline");
    viz::SettingField from;
    from.key = "from";
    from.kind = viz::SettingField::Kind::Int;
    from.label = "From";
    from.group = "Baseline";
    from.enabled_if = "baseline";
    form->add(from);
    form->add_choice("mode", "Mode", {{"mean", "Mean"}, {"median", "Median"}}, "mean");
    form->add_text("name", "Name", "avg");
    viz::SettingField late;
    late.key = "late";
    late.kind = viz::SettingField::Kind::Int;
    late.label = "Late";
    late.maximum = 50;
    late.commit = viz::SettingField::Commit::WhenFinished;
    form->add(late);
    form->add_action("reset", "Reset");
    form->add_custom("extra", "Extra");

    SettingsPanel panel(form);
    QCOMPARE(panel.windowTitle(), QStringLiteral("Test panel"));
    auto* trials = qobject_cast<QSpinBox*>(panel.editor("trials"));
    auto* prestim = qobject_cast<QDoubleSpinBox*>(panel.editor("prestim"));
    auto* baseline = qobject_cast<QCheckBox*>(panel.editor("baseline"));
    auto* mode = qobject_cast<QComboBox*>(panel.editor("mode"));
    auto* name = qobject_cast<QLineEdit*>(panel.editor("name"));
    auto* reset = qobject_cast<QPushButton*>(panel.editor("reset"));
    QVERIFY(trials && prestim && baseline && mode && name && reset);
    QVERIFY(panel.customWidget("extra") && panel.customWidget("extra")->layout());
    QVERIFY(!panel.customWidget("trials"));

    // The widgets show the form's values and ranges
    QCOMPARE(trials->value(), 10);
    QCOMPARE(trials->suffix(), QStringLiteral(" trials"));
    QCOMPARE(prestim->decimals(), 2);
    QCOMPARE(mode->count(), 2);
    QCOMPARE(name->text(), QStringLiteral("avg"));

    // Widget edits reach the form, form changes reach the widgets
    trials->setValue(42);
    QCOMPARE(form->get<int>("trials"), 42);
    mode->setCurrentIndex(1);
    QCOMPARE(form->get<std::string>("mode"), std::string("median"));
    // A field that commits when editing finishes waits for it
    auto* lateSpin = qobject_cast<QSpinBox*>(panel.editor("late"));
    lateSpin->setValue(7);
    QCOMPARE(form->get<int>("late"), 0);
    emit lateSpin->editingFinished();
    QCOMPARE(form->get<int>("late"), 7);
    form->set_value("prestim", 0.5);
    QCOMPARE(prestim->value(), 0.5);
    form->set_range("trials", 1, 20);
    QCOMPARE(trials->maximum(), 20);
    QCOMPARE(trials->value(), 20);
    form->set_choices("mode", {{"max", "Maximum"}});
    QCOMPARE(mode->itemText(0), QStringLiteral("Maximum"));

    // Conditions enable fields; actions trigger listeners
    QVERIFY(!panel.editor("from")->isEnabled());
    baseline->setChecked(true);
    QVERIFY(panel.editor("from")->isEnabled());
    bool triggered = false;
    form->add_listener([&triggered](const std::string& key, const std::optional<viz::SettingValue>& value) {
        triggered = triggered || (key == "reset" && !value);
    });
    reset->click();
    QVERIFY(triggered);

    // Values round-trip through QSettings
    const QString path = QStringLiteral("test_viz_qt_viewers/settingsPanel");
    panel.saveSettings(path);
    form->set_value("trials", 3);
    form->set_value("baseline", false);
    panel.loadSettings(path);
    QCOMPARE(form->get<int>("trials"), 20);
    QVERIFY(form->get<bool>("baseline"));
    QSettings(QStringLiteral("MNECPP")).remove(path);
}

//=============================================================================================================

void TestVizQtViewers::settingsPanel_infoColorDirectoryAndRebuild()
{
    auto form = std::make_shared<SettingsForm>("Forward");
    form->add_info("status", "Status", "idle");
    form->add_directory("subjects", "Subjects", "/data");
    form->add_color("color", "Colour", "#112233");
    SettingsPanel panel(form);
    QSignalSpy rebuilt(&panel, &SettingsPanel::rebuilt);

    auto* status = qobject_cast<QLabel*>(panel.editor("status"));
    QVERIFY(status);
    QCOMPARE(status->text(), QStringLiteral("idle"));
    form->set_value("status", std::string("failed"));
    form->set_tone("status", SettingField::Tone::Negative);
    QCOMPARE(status->text(), QStringLiteral("failed"));
    QVERIFY(status->styleSheet().contains(QStringLiteral("color")));

    auto* directory = panel.editor("subjects")->findChild<QLineEdit*>();
    QVERIFY(directory);
    QCOMPARE(directory->text(), QStringLiteral("/data"));
    directory->setText(QStringLiteral("/other"));
    emit directory->textEdited(directory->text());
    QCOMPARE(form->get<std::string>("subjects"), std::string("/other"));

    auto* color = qobject_cast<QToolButton*>(panel.editor("color"));
    QVERIFY(color && color->styleSheet().contains(QStringLiteral("#112233")));
    form->set_value("color", std::string("#445566"));
    QVERIFY(color->styleSheet().contains(QStringLiteral("#445566")));

    // Added and removed fields rebuild the rows
    form->add_bool("extra", "Extra", true);
    QCOMPARE(rebuilt.count(), 1);
    QVERIFY(qobject_cast<QCheckBox*>(panel.editor("extra")));
    QCOMPARE(panel.findChildren<QCheckBox*>(QStringLiteral("extra")).size(), 1);
    form->remove("extra");
    QCOMPARE(rebuilt.count(), 2);
    QVERIFY(!panel.editor("extra"));
    QCOMPARE(qobject_cast<QLabel*>(panel.editor("status"))->text(), QStringLiteral("failed"));
}

//=============================================================================================================

void TestVizQtViewers::settingsPanel_flagsGroupsAndPersistence()
{
    auto form = std::make_shared<viz::SettingsForm>();
    form->add_flag("offline");
    viz::SettingField compute;
    compute.key = "compute";
    compute.kind = viz::SettingField::Kind::Action;
    compute.label = "Compute";
    compute.group = "Offline";
    compute.visible_if = "offline";
    form->add(compute);
    viz::SettingField temporary;
    temporary.key = "temporary";
    temporary.kind = viz::SettingField::Kind::Int;
    temporary.label = "Temporary";
    temporary.maximum = 10;
    temporary.persistent = false;
    form->add(temporary);
    form->add_int("kept", "Kept", 1, 0, 10);

    SettingsPanel panel(form);
    panel.show();
    // Flags get no widget; a group box shows while one of its fields does
    QVERIFY(!panel.editor("offline"));
    QWidget* box = panel.editor("compute")->parentWidget();
    while (box && !qobject_cast<QGroupBox*>(box)) {
        box = box->parentWidget();
    }
    QVERIFY(box);
    QVERIFY(box->isHidden());
    form->set_value("offline", true);
    QVERIFY(!box->isHidden() && !panel.editor("compute")->isHidden());
    form->set_value("offline", false);
    QVERIFY(box->isHidden());

    // Only persistent fields are stored and read
    const QString path = QStringLiteral("test_viz_qt_viewers/settingsPanelPersistence");
    QSettings(QStringLiteral("MNECPP")).remove(path);
    form->set_value("temporary", 5);
    form->set_value("kept", 7);
    form->set_value("offline", true);
    panel.saveSettings(path);
    {
        QSettings settings(QStringLiteral("MNECPP"));
        QVERIFY(settings.contains(path + QStringLiteral("/kept")));
        QVERIFY(!settings.contains(path + QStringLiteral("/temporary")));
        QVERIFY(!settings.contains(path + QStringLiteral("/offline")));
        settings.setValue(path + QStringLiteral("/temporary"), 9);
    }
    form->set_value("kept", 2);
    panel.loadSettings(path);
    QCOMPARE(form->get<int>("kept"), 7);
    QCOMPARE(form->get<int>("temporary"), 5);
    QSettings(QStringLiteral("MNECPP")).remove(path);
}

//=============================================================================================================

void TestVizQtViewers::averagingSettingsView_formPanel()
{
    const QString path = QStringLiteral("test_viz_qt_viewers_averaging_%1").arg(QCoreApplication::applicationPid());
    QSettings(QStringLiteral("MNECPP")).remove(path);
    const std::map<std::string, int> stimChannels = {{"STI 014", 306}, {"STI 101", 312}};
    {
        AveragingSettingsView view(path, stimChannels);
        view.show();
        auto* panel = view.findChild<SettingsPanel*>();
        QVERIFY(panel);
        auto* channels = qobject_cast<QComboBox*>(panel->editor("currentStimChannel"));
        auto* pre = qobject_cast<QSpinBox*>(panel->editor("preStimSeconds"));
        auto* post = qobject_cast<QSpinBox*>(panel->editor("postStimSeconds"));
        auto* from = qobject_cast<QSpinBox*>(panel->editor("baselineFromSeconds"));
        auto* num = qobject_cast<QSpinBox*>(panel->editor("numAverages"));
        auto* baseline = qobject_cast<QCheckBox*>(panel->editor("doBaselineCorrection"));
        QVERIFY(channels && pre && post && from && num && baseline);

        // Defaults reach the widgets; the epoch window bounds the baseline
        QCOMPARE(channels->count(), 2);
        QCOMPARE(channels->currentText(), QStringLiteral("STI 014"));
        QCOMPARE(view.getStimChannelIdx(), 306);
        QCOMPARE(pre->value(), 100);
        QCOMPARE(pre->prefix(), QStringLiteral("-"));
        QCOMPARE(post->value(), 400);
        QCOMPARE(num->value(), 10);
        QCOMPARE(from->minimum(), -100);
        QCOMPARE(from->maximum(), 400);

        // Spin boxes report when editing finishes
        QSignalSpy preSpy(&view, &AveragingSettingsView::changePreStim);
        pre->setValue(200);
        QCOMPARE(preSpy.count(), 0);
        emit pre->editingFinished();
        QCOMPARE(preSpy.count(), 1);
        QCOMPARE(preSpy.last().at(0).toInt(), 200);
        QCOMPARE(view.getPreStimMSeconds(), 200);
        QCOMPARE(from->minimum(), -200);
        from->setValue(-150);
        emit from->editingFinished();
        QCOMPARE(view.getBaselineFromSeconds(), -150);
        QSignalSpy baselineSpy(&view, &AveragingSettingsView::changeBaselineActive);
        baseline->click();
        QCOMPARE(baselineSpy.count(), 1);
        QVERIFY(view.getDoBaselineCorrection());
        QSignalSpy stimSpy(&view, &AveragingSettingsView::changeStimChannel);
        channels->setCurrentText(QStringLiteral("STI 101"));
        QCOMPARE(stimSpy.count(), 1);
        QCOMPARE(view.getCurrentStimCh(), std::string("STI 101"));
        QCOMPARE(view.getStimChannelIdx(), 312);

        // Realtime and offline controls follow the processing mode
        auto* reset = panel->editor("reset");
        auto* compute = panel->editor("compute");
        auto* autoCompute = qobject_cast<QCheckBox*>(panel->editor("autoCompute"));
        QVERIFY(reset && compute && autoCompute);
        view.setProcessingMode(AbstractView::ProcessingMode::Offline);
        QVERIFY(reset->isHidden() && channels->isHidden() && num->isHidden());
        QVERIFY(!compute->isHidden() && !autoCompute->isHidden());
        QSignalSpy autoSpy(&view, &AveragingSettingsView::setAutoCompute);
        autoCompute->click();
        QCOMPARE(autoSpy.count(), 1);
        QVERIFY(autoSpy.last().at(0).toBool());
        QVERIFY(view.getAutoComputeStatus());
        view.setProcessingMode(AbstractView::ProcessingMode::RealTime);
        QVERIFY(!reset->isHidden() && compute->isHidden());

        // The detected trials show in their own group while there are some
        auto* trials = panel->customWidget("detectedTrials");
        QVERIFY(trials && trials->isHidden());
        FiffEvokedSet evokedSet;
        FiffEvoked evoked;
        evoked.comment = "3";
        evoked.nave = 12;
        evokedSet.evoked.push_back(evoked);
        view.setDetectedEpochs(evokedSet);
        QVERIFY(!trials->isHidden());
        QStringList texts;
        for (QLabel* label : trials->findChildren<QLabel*>()) {
            texts << label->text();
        }
        QVERIFY(texts.contains(QStringLiteral("12")));
        view.setDetectedEpochs(FiffEvokedSet());
        QVERIFY(trials->isHidden());
    }

    // The view stored its values under the original keys; a new view reads them
    {
        QSettings settings(QStringLiteral("MNECPP"));
        const QString group = path + QStringLiteral("/AveragingSettingsView/");
        QCOMPARE(settings.value(group + QStringLiteral("preStimSeconds")).toInt(), 200);
        QCOMPARE(settings.value(group + QStringLiteral("baselineFromSeconds")).toInt(), -150);
        QCOMPARE(settings.value(group + QStringLiteral("currentStimChannel")).toString(), QStringLiteral("STI 101"));
        QVERIFY(settings.value(group + QStringLiteral("doBaselineCorrection")).toBool());
        QVERIFY(!settings.contains(group + QStringLiteral("autoCompute")));
        QVERIFY(!settings.contains(group + QStringLiteral("offline")));
        settings.setValue(group + QStringLiteral("numAverages"), 33);
    }
    {
        AveragingSettingsView view(path, stimChannels);
        QCOMPARE(view.getPreStimMSeconds(), 200);
        QCOMPARE(view.getBaselineFromSeconds(), -150);
        QCOMPARE(view.getNumAverages(), 33);
        QVERIFY(view.getDoBaselineCorrection());
        QCOMPARE(view.getCurrentStimCh(), std::string("STI 101"));
        QCOMPARE(qobject_cast<QSpinBox*>(view.findChild<SettingsPanel*>()->editor("numAverages"))->value(), 33);
    }
    QSettings(QStringLiteral("MNECPP")).remove(path);
}

//=============================================================================================================

void TestVizQtViewers::filterSettingsView_formPanel()
{
    const QString path = QStringLiteral("test_viz_qt_viewers_filter_%1").arg(QCoreApplication::applicationPid());
    QSettings(QStringLiteral("MNECPP")).remove(path);
    {
        FilterSettingsView view(path);
        view.show();
        auto* panel = view.findChild<SettingsPanel*>();
        QVERIFY(panel);
        auto* active = qobject_cast<QCheckBox*>(panel->editor("filterActivated"));
        auto* type = qobject_cast<QComboBox*>(panel->editor("filterChannelType"));
        auto* from = qobject_cast<QDoubleSpinBox*>(panel->editor("filterFrom"));
        auto* to = qobject_cast<QDoubleSpinBox*>(panel->editor("filterTo"));
        auto* advanced = panel->editor("showFilterOptions");
        QVERIFY(active && type && from && to && advanced);
        QCOMPARE(type->count(), 5);
        QCOMPARE(from->suffix(), QStringLiteral("Hz"));

        // The pass band follows the design view and is handed to it when editing finishes
        QCOMPARE(from->value(), view.getFilterView()->getFrom());
        view.setSamplingRate(600.0);
        QCOMPARE(to->maximum(), 300.0);
        to->setValue(40.0);
        emit to->editingFinished();
        QCOMPARE(view.getFilterView()->getTo(), 40.0);
        from->setValue(5.0);
        emit from->editingFinished();
        QCOMPARE(view.getFilterView()->getFrom(), 5.0);
        type->setCurrentText(QStringLiteral("EEG"));
        QCOMPARE(view.getFilterView()->getChannelType(), QStringLiteral("EEG"));

        QSignalSpy activeSpy(&view, &FilterSettingsView::filterActivationChanged);
        active->click();
        QCOMPARE(activeSpy.count(), 1);
        QVERIFY(activeSpy.last().at(0).toBool());
        QVERIFY(view.getFilterActive());

        // The clinical mode hides the advanced design
        view.setGuiMode(AbstractView::GuiMode::Clinical);
        QVERIFY(advanced->isHidden());
        view.setGuiMode(AbstractView::GuiMode::Research);
        QVERIFY(!advanced->isHidden());
    }

    {
        QSettings settings(QStringLiteral("MNECPP"));
        const QString group = path + QStringLiteral("/FilterSettingsView/");
        QVERIFY(settings.value(group + QStringLiteral("filterActivated")).toBool());
        QCOMPARE(settings.value(group + QStringLiteral("filterFrom")).toDouble(), 5.0);
        QCOMPARE(settings.value(group + QStringLiteral("filterTo")).toDouble(), 40.0);
        QCOMPARE(settings.value(group + QStringLiteral("filterChannelType")).toString(), QStringLiteral("EEG"));
        QVERIFY(!settings.contains(group + QStringLiteral("clinical")));
    }
    {
        FilterSettingsView view(path);
        QVERIFY(view.getFilterActive());
        auto* type = qobject_cast<QComboBox*>(view.findChild<SettingsPanel*>()->editor("filterChannelType"));
        QCOMPARE(type->currentText(), QStringLiteral("EEG"));
    }
    QSettings(QStringLiteral("MNECPP")).remove(path);
}

//=============================================================================================================

void TestVizQtViewers::artifactSettingsView_formPanel()
{
    auto channel = [](int kind, int unit) {
        FiffChInfo ch;
        ch.kind = kind;
        ch.unit = unit;
        return ch;
    };
    const QString path = QStringLiteral("test_viz_qt_viewers_artifact_%1").arg(QCoreApplication::applicationPid());
    QSettings(QStringLiteral("MNECPP")).remove(path);
    {
        ArtifactSettingsView view(path);
        view.show();
        auto* panel = view.findChild<SettingsPanel*>();
        QVERIFY(panel);
        auto* active = qobject_cast<QCheckBox*>(panel->editor("doArtifactThresholdReduction"));
        auto* gradMantissa = qobject_cast<QDoubleSpinBox*>(panel->editor("artifactThresholdsFirst/grad"));
        auto* gradExponent = qobject_cast<QSpinBox*>(panel->editor("artifactThresholdsSecond/grad"));
        auto* eog = panel->editor("artifactThresholdsFirst/eog");
        QVERIFY(active);
        QVERIFY(gradMantissa);
        QVERIFY(gradExponent);
        QVERIFY(eog);
        // Nothing shows before channels arrive
        QVERIFY(active->isHidden() && gradMantissa->isHidden());
        QVERIFY(!view.getDoArtifactThresholdRejection());

        QSignalSpy thresholdSpy(&view, &ArtifactSettingsView::changeArtifactThreshold);
        view.setChInfo({channel(FIFFV_MEG_CH, FIFF_UNIT_T_M), channel(FIFFV_EEG_CH, FIFF_UNIT_V)});
        QCOMPARE(thresholdSpy.count(), 1);
        QVERIFY(!active->isHidden() && !gradMantissa->isHidden() && !gradExponent->isHidden());
        QVERIFY(eog->isHidden());
        QCOMPARE(gradMantissa->prefix(), QStringLiteral("+/-"));
        QCOMPARE(gradExponent->prefix(), QStringLiteral("e"));

        // Mantissa and exponent compose the threshold
        gradMantissa->setValue(4.0);
        gradExponent->setValue(-11);
        QCOMPARE(thresholdSpy.count(), 3);
        const auto map = thresholdSpy.last().at(0).value<std::map<std::string, double>>();
        QCOMPARE(map.size(), std::size_t(3));
        QVERIFY(qAbs(map.at("grad") - 4e-11) < 1e-20);
        QCOMPARE(map.at("eeg"), 0.1);
        active->click();
        QVERIFY(view.getDoArtifactThresholdRejection());
        QCOMPARE(view.getThresholdMap().at("Active"), 1.0);
    }

    {
        QSettings settings(QStringLiteral("MNECPP"));
        const QString group = path + QStringLiteral("/ArtifactSettingsView/");
        QVERIFY(settings.value(group + QStringLiteral("doArtifactThresholdReduction")).toBool());
        QCOMPARE(settings.value(group + QStringLiteral("artifactThresholdsFirst/grad")).toDouble(), 4.0);
        QCOMPARE(settings.value(group + QStringLiteral("artifactThresholdsSecond/grad")).toInt(), -11);
        QVERIFY(!settings.contains(group + QStringLiteral("present/grad")));
    }
    {
        ArtifactSettingsView view(path, {channel(FIFFV_MEG_CH, FIFF_UNIT_T_M)});
        QVERIFY(view.getDoArtifactThresholdRejection());
        QVERIFY(qAbs(view.getThresholdMap().at("grad") - 4e-11) < 1e-20);
    }
    QSettings(QStringLiteral("MNECPP")).remove(path);
}

//=============================================================================================================

void TestVizQtViewers::tfSettingsView_formPanel()
{
    TfSettingsView view;
    auto* panel = view.findChild<SettingsPanel*>();
    QVERIFY(panel);
    auto* trials = qobject_cast<QSpinBox*>(panel->editor("trialNumber"));
    auto* rows = qobject_cast<QSpinBox*>(panel->editor("rowNumber"));
    QVERIFY(trials && rows);
    QCOMPARE(trials->maximum(), 10000);

    QSignalSpy changed(&view, &TfSettingsView::numberTrialRowChanged);
    trials->setValue(5);
    rows->setValue(3);
    QCOMPARE(changed.count(), 2);
    QCOMPARE(changed.last().at(0).toInt(), 5);
    QCOMPARE(changed.last().at(1).toInt(), 3);
    panel->form().set_value("trialNumber", 8);
    QCOMPARE(trials->value(), 8);
    QCOMPARE(changed.last().at(0).toInt(), 8);
}

//=============================================================================================================

void TestVizQtViewers::spharaSettingsView_lifecycle()
{
    SpharaSettingsView view("test_viz_qt_viewers");

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

void TestVizQtViewers::tfSettingsView_lifecycle()
{
    TfSettingsView view("test_viz_qt_viewers");

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

void TestVizQtViewers::connectivitySettingsView_setAndGet()
{
    ConnectivitySettingsView view("test_viz_qt_viewers");

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

void TestVizQtViewers::triggerDetectionView_lifecycle()
{
    TriggerDetectionView view("test_viz_qt_viewers");

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

namespace
{

/** The text of the label in front of a panel editor. */
QString fieldLabel(QWidget* editor)
{
    // The editor sits in a line widget that is the field of the form layout
    QWidget* field = editor;
    while (field && field->parentWidget() && !qobject_cast<QFormLayout*>(field->parentWidget()->layout())) {
        field = field->parentWidget();
    }
    auto* layout = field && field->parentWidget() ? qobject_cast<QFormLayout*>(field->parentWidget()->layout()) : nullptr;
    auto* label = layout ? qobject_cast<QLabel*>(layout->labelForField(field)) : nullptr;
    return label ? label->text() : QString();
}

} // namespace

//=============================================================================================================

void TestVizQtViewers::spharaSettingsView_formPanel()
{
    SpharaSettingsView view;
    auto* panel = view.findChild<SettingsPanel*>();
    QVERIFY(panel);
    auto* activate = qobject_cast<QCheckBox*>(panel->editor("activateSphara"));
    auto* system = qobject_cast<QComboBox*>(panel->editor("system"));
    auto* first = qobject_cast<QSpinBox*>(panel->editor("first"));
    auto* second = qobject_cast<QSpinBox*>(panel->editor("second"));
    QVERIFY(activate && system && first && second);
    QCOMPARE(view.windowTitle(), QStringLiteral("SPHARA Settings"));
    QCOMPARE(system->count(), 3);
    QCOMPARE(system->currentText(), QStringLiteral("BabyMEG"));
    QCOMPARE(first->maximum(), 270);
    QCOMPARE(first->value(), 270);
    QCOMPARE(second->value(), 105);
    QCOMPARE(fieldLabel(first), QStringLiteral("Inner layer:"));

    QSignalSpy activation(&view, &SpharaSettingsView::spharaActivationChanged);
    activate->setChecked(true);
    QCOMPARE(activation.count(), 1);
    QCOMPARE(activation.last().at(0).toBool(), true);

    // VectorView: Mag and Grad, at most 102 each; one signal with the clamped values
    QSignalSpy options(&view, &SpharaSettingsView::spharaOptionsChanged);
    system->setCurrentIndex(system->findText(QStringLiteral("VectorView")));
    QCOMPARE(options.count(), 1);
    QCOMPARE(options.last().at(0).toString(), QStringLiteral("VectorView"));
    QCOMPARE(options.last().at(1).toInt(), 102);
    QCOMPARE(options.last().at(2).toInt(), 102);
    QCOMPARE(fieldLabel(first), QStringLiteral("Mag"));
    QCOMPARE(fieldLabel(second), QStringLiteral("Grad"));
    QCOMPARE(first->maximum(), 102);
    QCOMPARE(second->maximum(), 102);

    // Spin boxes report when editing finishes
    first->setValue(50);
    QCOMPARE(options.count(), 1);
    emit first->editingFinished();
    QCOMPARE(options.count(), 2);
    QCOMPARE(options.last().at(1).toInt(), 50);

    // EEG: one field of at most 256, the second one hidden
    system->setCurrentIndex(system->findText(QStringLiteral("EEG")));
    QCOMPARE(options.count(), 3);
    QCOMPARE(options.last().at(0).toString(), QStringLiteral("EEG"));
    QCOMPARE(fieldLabel(first), QStringLiteral("EEG"));
    QCOMPARE(first->maximum(), 256);
    QVERIFY(second->isHidden());

    system->setCurrentIndex(system->findText(QStringLiteral("BabyMEG")));
    QCOMPARE(fieldLabel(first), QStringLiteral("Inner layer"));
    QCOMPARE(fieldLabel(second), QStringLiteral("Outer layer"));
    QCOMPARE(second->maximum(), 105);
    QVERIFY(!second->isHidden());
}

//=============================================================================================================

void TestVizQtViewers::minimumNormSettingsView_formPanel()
{
    {
        MinimumNormSettingsView fixed(QString{}, QStringLiteral("dSPM"));
        auto* method = qobject_cast<QComboBox*>(fixed.findChild<SettingsPanel*>()->editor("method"));
        QVERIFY(method);
        QCOMPARE(method->currentText(), QStringLiteral("dSPM"));
        QVERIFY(!method->isEnabled());
    }

    MinimumNormSettingsView view;
    auto* panel = view.findChild<SettingsPanel*>();
    QVERIFY(panel);
    auto* method = qobject_cast<QComboBox*>(panel->editor("method"));
    auto* trigger = qobject_cast<QComboBox*>(panel->editor("triggerType"));
    auto* timePoint = qobject_cast<QSpinBox*>(panel->editor("timePoint"));
    QWidget* checkpoint = panel->editor("modelCheckpoint");
    QVERIFY(method && trigger && timePoint && checkpoint);
    QCOMPARE(view.windowTitle(), QStringLiteral("InvMinimumNorm Settings"));
    QVERIFY(method->isEnabled());
    QCOMPARE(method->count(), 4);
    QCOMPARE(timePoint->maximum(), 10000);
    QVERIFY(checkpoint->isHidden());

    // The first trigger type is selected and reported; known types are not added twice
    QSignalSpy triggerSpy(&view, &MinimumNormSettingsView::triggerTypeChanged);
    view.setTriggerTypes(QStringList() << "1" << "2");
    view.setTriggerTypes(QStringList() << "2" << "face");
    QCOMPARE(trigger->count(), 3);
    QCOMPARE(triggerSpy.count(), 1);
    QCOMPARE(triggerSpy.last().at(0).toString(), QStringLiteral("1"));
    trigger->setCurrentIndex(2);
    QCOMPARE(triggerSpy.last().at(0).toString(), QStringLiteral("face"));

    QSignalSpy timeSpy(&view, &MinimumNormSettingsView::timePointChanged);
    timePoint->setValue(120);
    QCOMPARE(timeSpy.count(), 0);
    emit timePoint->editingFinished();
    QCOMPARE(timeSpy.count(), 1);
    QCOMPARE(timeSpy.last().at(0).toInt(), 120);

    // CMNE shows the model checkpoint
    QSignalSpy methodSpy(&view, &MinimumNormSettingsView::methodChanged);
    method->setCurrentIndex(method->findText(QStringLiteral("CMNE")));
    QCOMPARE(methodSpy.count(), 1);
    QCOMPARE(methodSpy.last().at(0).toString(), QStringLiteral("CMNE"));
    QVERIFY(!checkpoint->isHidden());

    QSignalSpy checkpointSpy(&view, &MinimumNormSettingsView::modelCheckpointChanged);
    view.setModelCheckpoint("model.onnx");
    QCOMPARE(checkpointSpy.count(), 1);
    QCOMPARE(view.getModelCheckpoint(), std::filesystem::path("model.onnx"));
    auto* edit = checkpoint->findChild<QLineEdit*>();
    QVERIFY(edit);
    QCOMPARE(edit->text(), QStringLiteral("model.onnx"));
    view.setModelCheckpoint("model.onnx");
    QCOMPARE(checkpointSpy.count(), 1);
}

//=============================================================================================================

void TestVizQtViewers::connectivitySettingsView_formPanel()
{
    const QString path = QStringLiteral("test_viz_qt_viewers_connectivity_form");
    QSettings("MNECPP").remove(path);
    {
        ConnectivitySettingsView view(path);
        auto* panel = view.findChild<SettingsPanel*>();
        QVERIFY(panel);
        auto* metric = qobject_cast<QComboBox*>(panel->editor("connMethod"));
        auto* window = qobject_cast<QComboBox*>(panel->editor("connWindowType"));
        auto* trials = qobject_cast<QSpinBox*>(panel->editor("connNrTrials"));
        auto* trigger = qobject_cast<QComboBox*>(panel->editor("connTriggerType"));
        auto* low = qobject_cast<QDoubleSpinBox*>(panel->editor("connFreqLow"));
        auto* high = qobject_cast<QDoubleSpinBox*>(panel->editor("connFreqHigh"));
        QVERIFY(metric && window && trials && trigger && low && high);
        QCOMPARE(view.windowTitle(), QStringLiteral("Connectivity Settings"));
        QCOMPARE(metric->count(), 9);
        QCOMPARE(view.getConnectivityMetric(), QStringLiteral("COR"));
        QCOMPARE(view.getWindowType(), QStringLiteral("Hanning"));
        QCOMPARE(view.getNumberTrials(), 10);
        QCOMPARE(view.getLowerFreq(), 7.0);
        QCOMPARE(view.getUpperFreq(), 13.0);

        QSignalSpy metricSpy(&view, &ConnectivitySettingsView::connectivityMetricChanged);
        metric->setCurrentIndex(metric->findText(QStringLiteral("PLI")));
        QCOMPARE(metricSpy.count(), 1);
        QCOMPARE(metricSpy.last().at(0).toString(), QStringLiteral("PLI"));

        QSignalSpy windowSpy(&view, &ConnectivitySettingsView::windowTypeChanged);
        window->setCurrentIndex(1);
        QCOMPARE(windowSpy.last().at(0).toString(), QStringLiteral("Ones"));

        // Only a finished edit to another number of trials is reported; setNumberTrials() is silent
        QSignalSpy trialsSpy(&view, &ConnectivitySettingsView::numberTrialsChanged);
        view.setNumberTrials(20);
        QCOMPARE(trials->value(), 20);
        QCOMPARE(trialsSpy.count(), 0);
        trials->setValue(30);
        emit trials->editingFinished();
        QCOMPARE(trialsSpy.count(), 1);
        QCOMPARE(trialsSpy.last().at(0).toInt(), 30);

        QSignalSpy triggerSpy(&view, &ConnectivitySettingsView::triggerTypeChanged);
        view.setTriggerTypes(QStringList() << "1" << "2");
        QCOMPARE(trigger->count(), 2);
        QCOMPARE(triggerSpy.count(), 1);
        trigger->setCurrentIndex(1);
        QCOMPARE(triggerSpy.last().at(0).toString(), QStringLiteral("2"));

        QSignalSpy bandSpy(&view, &ConnectivitySettingsView::freqBandChanged);
        high->setValue(30.0);
        QCOMPARE(bandSpy.count(), 1);
        QCOMPARE(bandSpy.last().at(0).toFloat(), 7.0f);
        QCOMPARE(bandSpy.last().at(1).toFloat(), 30.0f);
        view.setFrequencyBand(8.0, 12.0);
        QCOMPARE(low->value(), 8.0);
        QCOMPARE(bandSpy.last().at(1).toFloat(), 12.0f);
    }

    // The values persist under the original keys and load without signals
    QSettings settings("MNECPP");
    QCOMPARE(settings.value(path + "/ConnectivitySettingsView/connMethod").toString(), QStringLiteral("PLI"));
    QCOMPARE(settings.value(path + "/ConnectivitySettingsView/connNrTrials").toInt(), 30);
    QCOMPARE(settings.value(path + "/ConnectivitySettingsView/connFreqHigh").toDouble(), 12.0);
    {
        ConnectivitySettingsView view(path);
        QCOMPARE(view.getConnectivityMetric(), QStringLiteral("PLI"));
        QCOMPARE(view.getWindowType(), QStringLiteral("Ones"));
        QCOMPARE(view.getNumberTrials(), 30);
        QCOMPARE(view.getLowerFreq(), 8.0);
        QCOMPARE(view.getUpperFreq(), 12.0);
        // The loaded number of trials is the reference for changes
        QSignalSpy trialsSpy(&view, &ConnectivitySettingsView::numberTrialsChanged);
        auto* trials = qobject_cast<QSpinBox*>(view.findChild<SettingsPanel*>()->editor("connNrTrials"));
        emit trials->editingFinished();
        QCOMPARE(trialsSpy.count(), 0);
    }
    settings.remove(path);
}

//=============================================================================================================

void TestVizQtViewers::triggerDetectionView_formPanel()
{
    TriggerDetectionView view;
    auto* panel = view.findChild<SettingsPanel*>();
    QVERIFY(panel);
    QCOMPARE(view.windowTitle(), QStringLiteral("Trigger Detection Settings"));
    auto* first = qobject_cast<QDoubleSpinBox*>(panel->editor("detectionThresholdFirst"));
    auto* second = qobject_cast<QSpinBox*>(panel->editor("detectionThresholdSecond"));
    QVERIFY(first && second);
    QCOMPARE(first->value(), 0.1);
    QCOMPARE(second->value(), -1);
    QCOMPARE(second->prefix(), QStringLiteral("e^"));
    QVERIFY(qobject_cast<QCheckBox*>(panel->editor("activateTriggerDetection")));
    QVERIFY(qobject_cast<QComboBox*>(panel->editor("triggerChannel")));
    QVERIFY(qobject_cast<QPushButton*>(panel->editor("resetNumberTriggers")));
    QVERIFY(panel->customWidget("triggerColor")->findChild<QPushButton*>(QStringLiteral("triggerColorButton")));
    auto* count = panel->customWidget("numberDetectedTriggers")->findChild<QLabel*>(QStringLiteral("numberDetectedTriggersLabel"));
    QVERIFY(count);
    QCOMPARE(count->text(), QStringLiteral("0"));
    QVERIFY(panel->editor("detectTriggers")->isHidden());

    // Before init() the controls do not report
    QSignalSpy infoSpy(&view, &TriggerDetectionView::triggerInfoChanged);
    first->setValue(0.5);
    QCOMPARE(infoSpy.count(), 0);
    view.setNumberDetectedTriggersAndTypes(2, {{1, {{5, 3.0}}}});
    QCOMPARE(count->text(), QStringLiteral("2"));
    QCOMPARE(qobject_cast<QComboBox*>(panel->editor("triggerColorType"))->count(), 1);
}

//=============================================================================================================

void TestVizQtViewers::progressView_updateProgress()
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

void TestVizQtViewers::quickControlView_addGroupBox()
{
    QuickControlView view("test_viz_qt_viewers", "Test Controls");

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

void TestVizQtViewers::multiView_addWidget()
{
    MultiView view("test_viz_qt_viewers");

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

QTEST_MAIN(TestVizQtViewers)
#include "test_viz_qt_viewers.moc"

#include <vector>
