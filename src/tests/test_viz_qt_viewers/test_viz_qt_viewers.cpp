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
 *           Covers: ScalingView, CompensatorView, ProjectorsView, ModalitySelectionView,
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
#include <mne/viz/backends/qt/viewers/compensatorview.h>
#include <mne/viz/backends/qt/viewers/projectorsview.h>
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
#include <mne/viz/backends/qt/viewers/settingspanel.h>

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
#include <QComboBox>
#include <QDoubleSpinBox>
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
    void settingsPanel_buildsAndSyncsWidgets();

    //=========================================================================================================
    /**
     * Verifies that SettingsPanel builds no widget for flags, hides group boxes without a shown field and stores
     * only persistent fields.
     */
    void settingsPanel_flagsGroupsAndPersistence();

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

void TestVizQtViewers::compensatorView_setAndGet()
{
    CompensatorView view("test_viz_qt_viewers");

    // Empty list
    view.setCompensators(std::vector<FiffCtfComp>());
    QVERIFY(view.getCompensators().empty());
    QCOMPARE(view.getLastTo(), 0);

    // Non-empty list
    FiffCtfComp comp;
    comp.ctfkind = 101;
    comp.kind = FIFF_MNE_CTF_COMP_KIND;
    view.setCompensators({comp});

    view.setGuiMode(AbstractView::GuiMode::Research);
    view.setProcessingMode(AbstractView::ProcessingMode::Offline);
    view.saveSettings();
    view.loadSettings();
    view.clearView();

    QApplication::processEvents();
}

//=============================================================================================================

void TestVizQtViewers::projectorsView_setAndGet()
{
    ProjectorsView view("test_viz_qt_viewers");

    // Empty list
    view.setProjectors(std::vector<FiffProj>());
    QVERIFY(view.getProjectors().empty());

    // Non-empty list
    FiffProj proj;
    proj.kind = FIFFV_PROJ_ITEM_EEG_AVREF;
    proj.active = false;
    proj.desc = "Test projector";
    view.setProjectors({proj});
    QCOMPARE(view.getProjectors().size(), 1);
    QVERIFY(!view.getProjectors().front().active);

    view.redrawGUI();
    view.setGuiMode(AbstractView::GuiMode::Clinical);
    view.setProcessingMode(AbstractView::ProcessingMode::RealTime);
    view.saveSettings();
    view.loadSettings();
    view.clearView();

    QApplication::processEvents();
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
    auto* box = panel.editor("compute")->parentWidget();
    QVERIFY(qobject_cast<QGroupBox*>(box));
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
