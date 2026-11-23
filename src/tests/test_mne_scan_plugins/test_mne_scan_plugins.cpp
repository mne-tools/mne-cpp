//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_mne_scan_plugins.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October, 2026
 * @brief    Streams the sample recording through MNE Scan algorithm plugins, as a connected sensor would.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <averaging/averaging.h>
#include <mne/base/log.h>
#include "../common/scoped_log_capture.h"
#include <babymeg/babymeg.h>
#include <brainamp/brainamp.h>
#include <brainflowboard/brainflowboard.h>
#include <covariance/covariance.h>
#include <eegosports/eegosports.h>
#include <eegosports/eegosportselectrodeitem.h>
#include <fiffsimulator/fiffsimulator.h>
#include <ftbuffer/ftbuffer.h>
#include <gusbamp/gusbamp.h>
#include <hpi/hpi.h>
#include <lsladapter/lsladapter.h>
#include <natus/natus.h>
#include <neuromag/neuromag.h>
#include <neuronalconnectivity/neuronalconnectivity.h>
#include <noisereduction/noisereduction.h>
#include <rtclustmne/rtclustmne.h>
#include <rtfwd/rtfwd.h>
#include <tmsi/tmsi.h>
#include <tmsi/tmsielectrodeitem.h>
#include <mainwindow.h>
#include <mne/connectivity/network/network.h>
#include <scMeas/realtimeconnectivityestimate.h>
#include <scMeas/realtimecov.h>
#include <scMeas/realtimeevokedset.h>
#include <scMeas/realtimefwdsolution.h>
#include <scMeas/realtimemultisamplearray.h>
#include <scMeas/realtimesourceestimate.h>
#include <scShared/Management/plugininputconnector.h>
#include <scShared/Management/pluginoutputconnector.h>
#include <mne/fiff/fiff_cov.h>
#include <mne/fiff/fiff_evoked_set.h>
#include <mne/fiff/fiff_raw_data.h>
#include <mne/inv/inv_source_estimate.h>
#include <mne/mne/mne_forward_solution.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QGraphicsView>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QSettings>
#include <QStandardPaths>
#include <QtTest>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================

using namespace mne::scan;
using namespace mne;

class TestMneScanPlugins : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void initialisesWithoutSampleData_data();
    void initialisesWithoutSampleData();
    void stopsWhenStartedWithoutData_data();
    void stopsWhenStartedWithoutData();
    void rtFwdRefusesMissingInputFiles_data();
    void rtFwdRefusesMissingInputFiles();
    void rtClustMneKeepsTransformItCannotRead();
    void rtClustMneLocalizesTheSources_data();
    void rtClustMneLocalizesTheSources();
    void connectivityFollowsTheStream();
    void averagesTheStimulusEvents();
    void estimatesTheCovariance();
    void passesTheStreamThrough();
    void brainAmpStreamsTheSimulatedAmplifier();
    void tmsiStreamsTheSimulatedAmplifier();
    void gusbampStreamsTheSimulatedAmplifiers();
    void eegoStreamsTheSimulatedAmplifier();
    void neuromagStreamsTheSimulatedAcquisition();
    void ftbufferStreamsTheSimulatedRelay();
    void fiffSimulatorStreamsTheSimulatedServer();
    void natusStreamsTheSimulatedAmplifier();
    void babyMegStreamsTheSimulatedSystem();
    void brainFlowStreamsTheBoard();
    void eegoImpedanceViewShowsTheMeasuredElectrodes();
    void tmsiImpedanceViewShowsTheMeasuredElectrodes();
    void mainWindowSwitchesStyle();
    void lslAdapterUnloadsAndCloses();

private:
    static void addPluginRows();
    std::unique_ptr<AbstractPlugin> makePlugin(const QString& name) const;
    QList<Measurement::SPtr> stream(AbstractPlugin& plugin, int outputs = 1);

    QString m_rawPath;
    std::shared_ptr<FiffInfo> m_info;
    Eigen::MatrixXd m_data;
};

//=============================================================================================================

void TestMneScanPlugins::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QSettings("MNECPP").clear();
    m_rawPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(m_rawPath))
        return;
    FiffRawData raw(m_rawPath.toStdString());
    Eigen::MatrixXd times;
    QVERIFY(raw.read_raw_segment(m_data, times));
    m_info = std::make_shared<FiffInfo>(raw.info);
}

//=============================================================================================================

std::unique_ptr<AbstractPlugin> TestMneScanPlugins::makePlugin(const QString& name) const
{
    std::unique_ptr<AbstractPlugin> plugin;
    if (name == "Averaging")
        plugin = std::make_unique<AVERAGINGPLUGIN::Averaging>();
    else if (name == "Covariance")
        plugin = std::make_unique<COVARIANCEPLUGIN::Covariance>();
    else if (name == "NoiseReduction")
        plugin = std::make_unique<NOISEREDUCTIONPLUGIN::NoiseReduction>();
    else if (name == "Hpi")
        plugin = std::make_unique<HPIPLUGIN::Hpi>();
    else if (name == "NeuronalConnectivity")
        plugin = std::make_unique<NEURONALCONNECTIVITYPLUGIN::NeuronalConnectivity>();
    else if (name == "RtClustMne")
        plugin = std::make_unique<RTCLUSTMNEPLUGIN::RtClustMne>();
    else if (name == "RtFwd") {
        plugin = std::make_unique<RTFWDPLUGIN::RtFwd>();
        const QString data = QFileInfo(m_rawPath).absolutePath() + "/../../";
        plugin->setAttributes({{"bemname", data + "subjects/sample/bem/sample-5120-bem.fif"},
                               {"srcname", data + "subjects/sample/bem/sample-oct-6-src.fif"},
                               {"mriname", data + "MEG/sample/all-trans.fif"},
                               {"measname", m_rawPath}});
    }
    if (plugin) {
        plugin->init();
        delete plugin->setupWidget();
    }
    return plugin;
}

//=============================================================================================================

QList<Measurement::SPtr> TestMneScanPlugins::stream(AbstractPlugin& plugin, int outputs)
{
    QList<Measurement::SPtr> received;
    QMutex mutex;
    for (const auto& output : plugin.getOutputConnectors()) {
        connect(output.data(), &PluginOutputConnector::notify, this, [&received, &mutex](Measurement::SPtr pMeasurement) {
            QMutexLocker locker(&mutex);
            received.append(pMeasurement); }, Qt::DirectConnection);
    }
    plugin.start();

    // Blocks of 200 samples, the way the FIFF simulator delivers them
    auto block = QSharedPointer<RealTimeMultiSampleArray>::create();
    block->initFromFiffInfo(m_info);
    block->setMultiArraySize(1);
    // Like a plugin connection: the input sees the block while it holds the samples
    PluginInputConnector* input = plugin.getInputConnectors().first().data();
    RealTimeMultiSampleArray* raw = block.data();
    connect(raw, &Measurement::notify, input, [input, &block]() { emit input->notify(block); }, Qt::DirectConnection);
    const int blockSize = 200;
    for (int from = 0; from + blockSize <= m_data.cols(); from += blockSize)
        block->setValue(m_data.middleCols(from, blockSize));
    disconnect(raw, nullptr, input, nullptr);
    static_cast<void>(QTest::qWaitFor([&]() { QMutexLocker locker(&mutex); return received.size() >= outputs; }, 20000));

    plugin.stop();
    plugin.wait(5000);
    for (const auto& output : plugin.getOutputConnectors())
        disconnect(output.data(), nullptr, this, nullptr);
    QMutexLocker locker(&mutex);
    return received;
}

//=============================================================================================================

void TestMneScanPlugins::addPluginRows()
{
    QTest::addColumn<QString>("name");
    for (const char* name : {"Averaging", "Covariance", "Hpi", "NeuronalConnectivity", "NoiseReduction", "RtClustMne", "RtFwd"})
        QTest::newRow(name) << QString(name);
}

//=============================================================================================================

void TestMneScanPlugins::initialisesWithoutSampleData_data()
{
    addPluginRows();
}

void TestMneScanPlugins::initialisesWithoutSampleData()
{
    // The defaults point at MNE-sample-data, which a fresh install does not have
    QFETCH(QString, name);
    std::unique_ptr<AbstractPlugin> plugin = makePlugin(name);
    QVERIFY(plugin);
    QVERIFY(!plugin->getInputConnectors().isEmpty());
    QVERIFY(!plugin->getOutputConnectors().isEmpty());
}

//=============================================================================================================

void TestMneScanPlugins::stopsWhenStartedWithoutData_data()
{
    addPluginRows();
}

void TestMneScanPlugins::stopsWhenStartedWithoutData()
{
    QFETCH(QString, name);
    std::unique_ptr<AbstractPlugin> plugin = makePlugin(name);
    QVERIFY(plugin);
    QVERIFY(plugin->start());
    QVERIFY(plugin->stop());
    if (!plugin->wait(5000)) {
        // Destroying a running QThread aborts, so leave it to the process
        static QList<AbstractPlugin*> stillRunning;
        stillRunning.append(plugin.release());
        QFAIL("The processing thread is still running after stop()");
    }
}

//=============================================================================================================

void TestMneScanPlugins::rtFwdRefusesMissingInputFiles_data()
{
    QTest::addColumn<QString>("attribute");
    for (const char* attribute : {"bemname", "srcname", "mriname", "measname"})
        QTest::newRow(attribute) << QString(attribute);
}

void TestMneScanPlugins::rtFwdRefusesMissingInputFiles()
{
    QFETCH(QString, attribute);
    if (!m_info)
        QSKIP("Sample test data not found");

    std::unique_ptr<AbstractPlugin> plugin = makePlugin("RtFwd");
    plugin->setAttributes({{attribute, QDir::temp().filePath("no-such-mne-scan-input.fif")}});
    QString message;
    QTimer dismiss;
    connect(&dismiss, &QTimer::timeout, [&message]() {
        if (auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget())) {
            message = box->text();
            box->accept();
        }
    });
    dismiss.start(10);
    const bool started = plugin->start();
    dismiss.stop();
    if (started) {
        plugin->stop();
        QVERIFY(plugin->wait(5000));
    }
    QVERIFY2(!started, qPrintable(attribute));
    QVERIFY(message.contains("cannot be op"));
}

//=============================================================================================================

void TestMneScanPlugins::rtClustMneKeepsTransformItCannotRead()
{
    if (!m_info)
        QSKIP("Sample test data not found");

    std::unique_ptr<AbstractPlugin> plugin = makePlugin("RtClustMne");
    std::unique_ptr<QWidget> setup(plugin->setupWidget());
    auto* button = setup->findChild<QPushButton*>("m_qPushButton_MriHeadTrans");
    auto* path = setup->findChild<QLineEdit*>("m_qLineEdit_MriHeadTrans");
    QVERIFY(button && path);

    // Cancelling the dialog, a FIFF file without a transform, then a real one
    const QString sample = QFileInfo(m_rawPath).absolutePath();
    for (const QString& choice : {QString(), sample + "/sample_audvis-cov.fif", sample + "/all-trans.fif"}) {
        const QString before = path->text();
        QTimer::singleShot(10, qApp, [choice]() {
            auto* dialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget());
            QVERIFY(dialog);
            if (choice.isEmpty()) {
                dialog->reject();
                return;
            }
            dialog->setDirectory(QFileInfo(choice).absolutePath());
            dialog->findChild<QLineEdit*>("fileNameEdit")->setText(QFileInfo(choice).fileName());
            static_cast<QDialog*>(dialog)->accept();
        });
        QTest::mouseClick(button, Qt::LeftButton);
        QCOMPARE(path->text(), choice.endsWith("all-trans.fif") ? choice : before);
    }
}

//=============================================================================================================

void TestMneScanPlugins::rtClustMneLocalizesTheSources_data()
{
    QTest::addColumn<bool>("evoked");
    QTest::newRow("raw") << false;
    QTest::newRow("evoked") << true;
}

void TestMneScanPlugins::rtClustMneLocalizesTheSources()
{
    QFETCH(bool, evoked);
    if (!m_info)
        QSKIP("Sample test data not found");

    const QString sample = QFileInfo(m_rawPath).absolutePath();
    auto fwd = std::make_shared<MNEForwardSolution>((sample + "/../../Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif").toStdU16String());
    std::unique_ptr<AbstractPlugin> plugin = makePlugin("RtClustMne");
    const auto inputs = plugin->getInputConnectors();
    std::shared_ptr<InvSourceEstimate> estimate;
    QMutex mutex;
    connect(plugin->getOutputConnectors().first().data(), &PluginOutputConnector::notify, this, [&estimate, &mutex](Measurement::SPtr pMeasurement) {
        if (auto rtse = pMeasurement.dynamicCast<RealTimeSourceEstimate>(); rtse && !rtse->getValue().isEmpty()) {
            QMutexLocker locker(&mutex);
            estimate = rtse->getValue().last();
        } }, Qt::DirectConnection);
    QVERIFY(plugin->start());

    auto rtfs = QSharedPointer<RealTimeFwdSolution>::create();
    rtfs->setValue(fwd);
    emit inputs[3]->notify(rtfs);
    auto block = QSharedPointer<RealTimeMultiSampleArray>::create();
    block->initFromFiffInfo(m_info);
    block->setMultiArraySize(1);
    // The block holds its samples only while it notifies
    connect(block.data(), &Measurement::notify, inputs[0].data(), [input = inputs[0].data(), &block]() { emit input->notify(block); }, Qt::DirectConnection);
    // An average of four trials of stimulus type 3, the plugin's default
    FiffEvokedSet averages;
    FiffEvoked average;
    average.info = *m_info;
    average.data = m_data.leftCols(200);
    average.nave = 4;
    average.comment = "3";
    average.times = Eigen::RowVectorXf::LinSpaced(200, 0.0f, 199.0f / static_cast<float>(m_info->sfreq));
    averages.evoked.push_back(average);
    auto rtes = QSharedPointer<RealTimeEvokedSet>::create();
    const auto send = [&]() {
        if (evoked) {
            rtes->setValue(averages, m_info, {"3"});
            emit inputs[1]->notify(rtes);
        } else {
            block->setValue(m_data.leftCols(200));
        }
    };
    send();
    auto rtc = QSharedPointer<RealTimeCov>::create();
    rtc->setValue(FiffCov((sample + "/sample_audvis-cov.fif").toStdU16String()));

    // Like the covariance plugin, send an estimate and a block each second; the buffer holds 40 blocks
    QElapsedTimer sinceLast;
    const bool localized = QTest::qWaitFor([&]() {
        if (!sinceLast.isValid() || sinceLast.elapsed() >= 1000) {
            sinceLast.start();
            emit inputs[2]->notify(rtc);
            send();
        }
        QMutexLocker locker(&mutex);
        return estimate != nullptr; }, 30000);
    plugin->stop();
    QVERIFY(plugin->wait(5000));
    QVERIFY(localized);
    // The normal component of each source for raw data, the magnitude for an average
    QCOMPARE(estimate->data.rows(), fwd->nsource);
    QVERIFY(estimate->data.allFinite());
    QCOMPARE(estimate->data.minCoeff() >= 0.0, evoked);
}

//=============================================================================================================

void TestMneScanPlugins::connectivityFollowsTheStream()
{
    if (!m_info)
        QSKIP("Sample test data not found");

    // The default correlation has no frequency bins, so no frequency band applies to it
    ScopedLogCapture capture(true);
    std::unique_ptr<AbstractPlugin> plugin = makePlugin("NeuronalConnectivity");
    QList<QSharedPointer<Network>> networks;
    QMutex mutex;
    connect(plugin->getOutputConnectors().first().data(), &PluginOutputConnector::notify, this, [&networks, &mutex](Measurement::SPtr pMeasurement) {
        if (auto estimate = pMeasurement.dynamicCast<RealTimeConnectivityEstimate>(); estimate && estimate->getValue()) {
            QMutexLocker locker(&mutex);
            networks.append(QSharedPointer<Network>::create(*estimate->getValue()));
        } }, Qt::DirectConnection);
    QVERIFY(plugin->start());

    // One-second blocks into the sensor input; each gives a network over the newest blocks
    auto block = QSharedPointer<RealTimeMultiSampleArray>::create();
    block->initFromFiffInfo(m_info);
    block->setMultiArraySize(1);
    PluginInputConnector* input = plugin->getInputConnectors().at(1).data();
    connect(block.data(), &Measurement::notify, input, [input, &block]() { emit input->notify(block); }, Qt::DirectConnection);
    const int blockSize = static_cast<int>(m_info->sfreq);
    for (int from = 0; from + blockSize <= m_data.cols() && from < 12 * blockSize; from += blockSize)
        block->setValue(m_data.middleCols(from, blockSize));
    const bool received = QTest::qWaitFor([&]() { QMutexLocker locker(&mutex); return networks.size() >= 12; }, 20000);
    plugin->stop();
    QVERIFY(plugin->wait(5000));
    QVERIFY(received);
    QVERIFY2(!capture.contains(mne::Log::Level::Warning, "frequency bins has not been set"), capture.dump().c_str());

    // One node per gradiometer pair (the first of each, unless bad); normalized weights reach 1 at most
    const Eigen::MatrixXd matrix = networks.last()->connectivity_matrix();
    QCOMPARE(matrix.rows(), 102);
    QVERIFY(matrix.allFinite());
    QVERIFY(std::abs(matrix.cwiseAbs().maxCoeff() - 1.0) < 1e-12);
}

//=============================================================================================================

void TestMneScanPlugins::averagesTheStimulusEvents()
{
    if (!m_info)
        QSKIP("Sample test data not found");

    std::unique_ptr<AbstractPlugin> plugin = makePlugin("Averaging");
    const QList<Measurement::SPtr> received = stream(*plugin);
    QVERIFY(!received.isEmpty());
    auto evoked = received.last().dynamicCast<RealTimeEvokedSet>();
    QVERIFY(evoked);
    const FiffEvokedSet& set = *evoked->getValue();
    QVERIFY(!set.evoked.empty());

    // Defaults: STI014, -100..400 ms around each trigger
    const int pre = static_cast<int>(0.1f * m_info->sfreq);
    const int post = static_cast<int>(0.4f * m_info->sfreq);
    for (const FiffEvoked& average : set.evoked) {
        QCOMPARE(average.data.rows(), m_info->nchan);
        QCOMPARE(average.data.cols(), pre + post);
        QVERIFY(average.nave >= 1);
        QVERIFY(average.data.allFinite());
    }
}

//=============================================================================================================

void TestMneScanPlugins::estimatesTheCovariance()
{
    if (!m_info)
        QSKIP("Sample test data not found");

    std::unique_ptr<AbstractPlugin> plugin = makePlugin("Covariance");
    const QList<Measurement::SPtr> received = stream(*plugin);
    QVERIFY(!received.isEmpty());
    auto cov = received.last().dynamicCast<RealTimeCov>();
    QVERIFY(cov);
    const FiffCov& estimate = *cov->getValue();
    // MEG and EEG channels, like mne.compute_raw_covariance(raw.pick(["meg", "eeg"]))
    int dataChannels = 0;
    for (const FiffChInfo& ch : m_info->chs)
        dataChannels += (ch.kind == FIFFV_MEG_CH || ch.kind == FIFFV_EEG_CH) ? 1 : 0;
    QCOMPARE(estimate.data.rows(), dataChannels);
    QCOMPARE(estimate.names.size(), dataChannels);
    QVERIFY(estimate.data.isApprox(estimate.data.transpose()));
    QVERIFY(estimate.data.diagonal().minCoeff() >= 0.0);
}

//=============================================================================================================

void TestMneScanPlugins::passesTheStreamThrough()
{
    if (!m_info)
        QSKIP("Sample test data not found");

    // Without projectors, compensators or filters switched on, the stream leaves as it came in
    std::unique_ptr<AbstractPlugin> plugin = makePlugin("NoiseReduction");
    QList<Eigen::MatrixXd> blocks;
    QMutex mutex;
    connect(plugin->getOutputConnectors().first().data(), &PluginOutputConnector::notify, this, [&blocks, &mutex](Measurement::SPtr pMeasurement) {
        // The samples are only held while the notification runs
        QMutexLocker locker(&mutex);
        blocks.append(pMeasurement.dynamicCast<RealTimeMultiSampleArray>()->getMultiSampleArray()); }, Qt::DirectConnection);
    QVERIFY(!stream(*plugin).isEmpty());
    QMutexLocker locker(&mutex);
    QVERIFY(!blocks.isEmpty());
    QVERIFY(blocks.first().isApprox(m_data.middleCols(0, 200)));
}

//=============================================================================================================

void TestMneScanPlugins::brainAmpStreamsTheSimulatedAmplifier()
{
    QSettings settings("MNECPP");
    settings.setValue("BRAINAMP/channels", 8);
    settings.setValue("BRAINAMP/simulate", true);
    settings.setValue("BRAINAMP/samplesPerBlock", 250);
    BRAINAMPPLUGIN::BrainAMP plugin;
    plugin.init();
    delete plugin.setupWidget();

    QList<Eigen::MatrixXd> blocks;
    QMutex mutex;
    connect(plugin.getOutputConnectors().first().data(), &PluginOutputConnector::notify, this, [&blocks, &mutex](Measurement::SPtr pMeasurement) {
        QMutexLocker locker(&mutex);
        blocks.append(pMeasurement.dynamicCast<RealTimeMultiSampleArray>()->getMultiSampleArray()); }, Qt::DirectConnection);
    QVERIFY(plugin.start());
    QVERIFY(QTest::qWaitFor([&]() { QMutexLocker locker(&mutex); return blocks.size() >= 4; }, 10000));
    QVERIFY(plugin.stop());
    QVERIFY(plugin.wait(5000));
    settings.remove("BRAINAMP");

    // 8 EEG channels and the marker channel at the amplifier's native 5 kHz
    QMutexLocker locker(&mutex);
    for (const Eigen::MatrixXd& block : blocks) {
        QCOMPARE(block.rows(), 9);
        QCOMPARE(block.cols(), 250);
        QVERIFY(block.topRows(8).cwiseAbs().maxCoeff() < 1e-3);
        QVERIFY(block.topRows(8).cwiseAbs().maxCoeff() > 0.0);
        for (const double marker : block.row(8))
            QVERIFY(marker == 0.0 || marker == 1.0 || marker == 2.0);
    }
}

//=============================================================================================================

void TestMneScanPlugins::gusbampStreamsTheSimulatedAmplifiers()
{
    GUSBAMPPLUGIN::GUSBAmp plugin;
    plugin.init();
    {
        std::unique_ptr<QWidget> setup(plugin.setupWidget());
        auto* serials = setup->findChild<QLineEdit*>("m_lineEdit_Serials");
        auto* inputs = setup->findChild<QLineEdit*>("m_lineEdit_Inputs");
        auto* rate = setup->findChild<QComboBox*>("m_comboBox_SampleRate");
        auto* block = setup->findChild<QSpinBox*>("m_spinBox_SamplesPerBlock");
        auto* trigger = setup->findChild<QCheckBox*>("m_checkBox_Trigger");
        auto* simulate = setup->findChild<QCheckBox*>("m_checkBox_Simulate");
        QVERIFY(serials && inputs && rate && block && trigger && simulate);
        serials->setText("UB-1, UB-2");
        inputs->setText("1-4");
        rate->setCurrentText("512");
        block->setValue(32);
        trigger->click();
        simulate->click();
    }

    QList<Eigen::MatrixXd> blocks;
    QMutex mutex;
    connect(plugin.getOutputConnectors().first().data(), &PluginOutputConnector::notify, this, [&blocks, &mutex](Measurement::SPtr pMeasurement) {
        QMutexLocker locker(&mutex);
        blocks.append(pMeasurement.dynamicCast<RealTimeMultiSampleArray>()->getMultiSampleArray()); }, Qt::DirectConnection);
    QVERIFY(plugin.start());
    QVERIFY(QTest::qWaitFor([&]() { QMutexLocker locker(&mutex); return blocks.size() >= 4; }, 10000));
    QVERIFY(plugin.stop());
    QVERIFY(plugin.wait(5000));

    // Four inputs of the master, four of the slave, the master's trigger line; in volts at 512 Hz
    const auto info = plugin.getOutputConnectors().first().dynamicCast<PluginOutputData<RealTimeMultiSampleArray>>()->measurementData()->info();
    QCOMPARE(info->chs.size(), std::size_t(9));
    QCOMPARE(info->sfreq, 512.0);
    QCOMPARE(info->chs[7].ch_name, std::string("EEG 008"));
    QCOMPARE(info->chs[8].kind, FIFFV_STIM_CH);
    QMutexLocker locker(&mutex);
    for (const Eigen::MatrixXd& data : blocks) {
        QCOMPARE(data.rows(), 9);
        QCOMPARE(data.cols(), 32);
        QVERIFY(data.topRows(8).cwiseAbs().maxCoeff() < 1e-3);
        QVERIFY(data.topRows(8).cwiseAbs().maxCoeff() > 0.0);
    }
}

//=============================================================================================================

void TestMneScanPlugins::ftbufferStreamsTheSimulatedRelay()
{
    FTBUFFERPLUGIN::FtBuffer plugin;
    plugin.init();
    {
        std::unique_ptr<QWidget> setup(plugin.setupWidget());
        auto* host = setup->findChild<QLineEdit*>("m_lineEdit_Host");
        auto* port = setup->findChild<QSpinBox*>("m_spinBox_Port");
        auto* block = setup->findChild<QSpinBox*>("m_spinBox_SamplesPerBlock");
        auto* simulate = setup->findChild<QCheckBox*>("m_checkBox_Simulate");
        QVERIFY(host && port && block && simulate);
        QCOMPARE(host->text(), QString("localhost"));
        QCOMPARE(port->value(), 1972);
        block->setValue(40);
        simulate->click();
    }

    QList<Eigen::MatrixXd> blocks;
    QMutex mutex;
    connect(plugin.getOutputConnectors().first().data(), &PluginOutputConnector::notify, this, [&blocks, &mutex](Measurement::SPtr pMeasurement) {
        QMutexLocker locker(&mutex);
        blocks.append(pMeasurement.dynamicCast<RealTimeMultiSampleArray>()->getMultiSampleArray()); }, Qt::DirectConnection);
    QVERIFY(plugin.start());
    QVERIFY(QTest::qWaitFor([&]() { QMutexLocker locker(&mutex); return blocks.size() >= 4; }, 10000));
    QVERIFY(plugin.stop());
    QVERIFY(plugin.wait(5000));

    // neuromag2ft relays the measurement info: 306 MEG channels in the device frame and the trigger channel, in tesla
    const auto info = plugin.getOutputConnectors().first().dynamicCast<PluginOutputData<RealTimeMultiSampleArray>>()->measurementData()->info();
    QCOMPARE(info->chs.size(), std::size_t(307));
    QCOMPARE(info->sfreq, 1000.0);
    QCOMPARE(info->chs[0].kind, FIFFV_MEG_CH);
    QCOMPARE(info->chs[0].chpos.coil_type, FIFFV_COIL_VV_MAG_T3);
    QCOMPARE(info->chs[306].ch_name, std::string("STI 014"));
    QMutexLocker locker(&mutex);
    for (const Eigen::MatrixXd& data : blocks) {
        QCOMPARE(data.rows(), 307);
        QCOMPARE(data.cols(), 40);
        QVERIFY(data.topRows(306).cwiseAbs().maxCoeff() < 1e-9);
        QVERIFY(data.topRows(306).cwiseAbs().maxCoeff() > 0.0);
    }
}

//=============================================================================================================

void TestMneScanPlugins::fiffSimulatorStreamsTheSimulatedServer()
{
    FIFFSIMULATORPLUGIN::FiffSimulator plugin;
    plugin.init();
    {
        std::unique_ptr<QWidget> setup(plugin.setupWidget());
        auto* host = setup->findChild<QLineEdit*>("m_lineEdit_Host");
        auto* port = setup->findChild<QSpinBox*>("m_spinBox_Port");
        auto* file = setup->findChild<QLineEdit*>("m_lineEdit_SimFile");
        auto* block = setup->findChild<QSpinBox*>("m_spinBox_SamplesPerBlock");
        auto* simulate = setup->findChild<QCheckBox*>("m_checkBox_Simulate");
        QVERIFY(host && port && file && block && simulate);
        QCOMPARE(host->text(), QString("127.0.0.1"));
        QCOMPARE(port->value(), 4217);
        block->setValue(40);
        simulate->click();
    }

    QList<Eigen::MatrixXd> blocks;
    QMutex mutex;
    connect(plugin.getOutputConnectors().first().data(), &PluginOutputConnector::notify, this, [&blocks, &mutex](Measurement::SPtr pMeasurement) {
        QMutexLocker locker(&mutex);
        blocks.append(pMeasurement.dynamicCast<RealTimeMultiSampleArray>()->getMultiSampleArray()); }, Qt::DirectConnection);
    QVERIFY(plugin.start());
    QVERIFY(QTest::qWaitFor([&]() { QMutexLocker locker(&mutex); return blocks.size() >= 4; }, 10000));
    QVERIFY(plugin.stop());
    QVERIFY(plugin.wait(5000));

    // The server sends the recording's measurement info: 306 MEG channels and the trigger channel, in tesla
    const auto info = plugin.getOutputConnectors().first().dynamicCast<PluginOutputData<RealTimeMultiSampleArray>>()->measurementData()->info();
    QCOMPARE(info->chs.size(), std::size_t(307));
    QCOMPARE(info->sfreq, 1000.0);
    QCOMPARE(info->chs[0].kind, FIFFV_MEG_CH);
    QCOMPARE(info->chs[306].ch_name, std::string("STI 014"));
    QMutexLocker locker(&mutex);
    for (const Eigen::MatrixXd& data : blocks) {
        QCOMPARE(data.rows(), 307);
        QCOMPARE(data.cols(), 40);
        QVERIFY(data.topRows(306).cwiseAbs().maxCoeff() < 1e-9);
        QVERIFY(data.topRows(306).cwiseAbs().maxCoeff() > 0.0);
    }
}

//=============================================================================================================

void TestMneScanPlugins::natusStreamsTheSimulatedAmplifier()
{
    NATUSPLUGIN::Natus plugin;
    plugin.init();
    {
        std::unique_ptr<QWidget> setup(plugin.setupWidget());
        auto* address = setup->findChild<QLineEdit*>("m_lineEdit_Address");
        auto* port = setup->findChild<QSpinBox*>("m_spinBox_Port");
        auto* channels = setup->findChild<QSpinBox*>("m_spinBox_NumberChannels");
        auto* rate = setup->findChild<QSpinBox*>("m_spinBox_SamplingFreq");
        auto* block = setup->findChild<QSpinBox*>("m_spinBox_SamplesPerBlock");
        auto* simulate = setup->findChild<QCheckBox*>("m_checkBox_Simulate");
        QVERIFY(address && port && channels && rate && block && simulate);
        QCOMPARE(address->text(), QString("0.0.0.0"));
        QCOMPARE(port->value(), 50000);
        channels->setValue(8);
        rate->setValue(1024);
        block->setValue(64);
        simulate->click();
    }

    QList<Eigen::MatrixXd> blocks;
    QMutex mutex;
    connect(plugin.getOutputConnectors().first().data(), &PluginOutputConnector::notify, this, [&blocks, &mutex](Measurement::SPtr pMeasurement) {
        QMutexLocker locker(&mutex);
        blocks.append(pMeasurement.dynamicCast<RealTimeMultiSampleArray>()->getMultiSampleArray()); }, Qt::DirectConnection);
    QVERIFY(plugin.start());
    QVERIFY(QTest::qWaitFor([&]() { QMutexLocker locker(&mutex); return blocks.size() >= 4; }, 10000));
    QVERIFY(plugin.stop());
    QVERIFY(plugin.wait(5000));

    // Eight EEG channels in volts at 1024 Hz
    const auto info = plugin.getOutputConnectors().first().dynamicCast<PluginOutputData<RealTimeMultiSampleArray>>()->measurementData()->info();
    QCOMPARE(info->chs.size(), std::size_t(8));
    QCOMPARE(info->sfreq, 1024.0);
    QCOMPARE(info->chs[0].kind, FIFFV_EEG_CH);
    QCOMPARE(info->chs[0].ch_name, std::string("EEG 001"));
    QMutexLocker locker(&mutex);
    for (const Eigen::MatrixXd& data : blocks) {
        QCOMPARE(data.rows(), 8);
        QCOMPARE(data.cols(), 64);
        QVERIFY(data.cwiseAbs().maxCoeff() < 1e-3);
        QVERIFY(data.cwiseAbs().maxCoeff() > 0.0);
    }
}

//=============================================================================================================

void TestMneScanPlugins::brainFlowStreamsTheBoard()
{
#ifndef BRAINFLOW_STUB
    QSKIP("needs the stand-in BrainFlow library of test_acq_devices");
#else
    BRAINFLOWBOARDPLUGIN::BrainFlowBoard plugin;
    plugin.init();
    {
        std::unique_ptr<QWidget> setup(plugin.setupWidget());
        auto* board = setup->findChild<QLineEdit*>("m_lineEdit_Board");
        auto* serialPort = setup->findChild<QLineEdit*>("m_lineEdit_SerialPort");
        auto* library = setup->findChild<QLineEdit*>("m_lineEdit_Library");
        auto* ipPort = setup->findChild<QSpinBox*>("m_spinBox_IpPort");
        auto* ipProtocol = setup->findChild<QComboBox*>("m_comboBox_IpProtocol");
        auto* block = setup->findChild<QSpinBox*>("m_spinBox_SamplesPerBlock");
        QVERIFY(board && serialPort && library && ipPort && ipProtocol && block);
        QCOMPARE(board->text(), QString("synthetic"));
        library->setText(BRAINFLOW_STUB);
        // The stand-in's Cyton opens only on its serial port
        board->setText("cyton");
        QVERIFY(!plugin.start());
        serialPort->setText("COM-STUB");
        block->setValue(25);
    }

    QList<Eigen::MatrixXd> blocks;
    QMutex mutex;
    connect(plugin.getOutputConnectors().first().data(), &PluginOutputConnector::notify, this, [&blocks, &mutex](Measurement::SPtr pMeasurement) {
        QMutexLocker locker(&mutex);
        blocks.append(pMeasurement.dynamicCast<RealTimeMultiSampleArray>()->getMultiSampleArray()); }, Qt::DirectConnection);
    QVERIFY(plugin.start());
    QVERIFY(QTest::qWaitFor([&]() { QMutexLocker locker(&mutex); return blocks.size() >= 4; }, 10000));
    QVERIFY(plugin.stop());
    QVERIFY(plugin.wait(5000));

    // The board's rows: EEG in volts, the marker row as a trigger channel
    const auto info = plugin.getOutputConnectors().first().dynamicCast<PluginOutputData<RealTimeMultiSampleArray>>()->measurementData()->info();
    QCOMPARE(info->chs.size(), std::size_t(12));
    QCOMPARE(info->sfreq, 250.0);
    QCOMPARE(info->chs[1].ch_name, std::string("Fz"));
    QCOMPARE(info->chs[1].kind, FIFFV_EEG_CH);
    QCOMPARE(info->chs[11].ch_name, std::string("MARKER"));
    QCOMPARE(info->chs[11].kind, FIFFV_STIM_CH);
    QMutexLocker locker(&mutex);
    for (const Eigen::MatrixXd& data : blocks) {
        QCOMPARE(data.rows(), 12);
        QCOMPARE(data.cols(), 25);
    }
    // Sample k of Fz is 1 + k / 1000 microvolts
    QVERIFY(std::abs(blocks.front()(1, 0) - 1e-6) < 1e-12);
    QVERIFY(std::abs(blocks[1](1, 0) - 1.025e-6) < 1e-12);
    QCOMPARE(blocks[1](11, 5), 7.0); // the marker at sample 30
#endif
}

//=============================================================================================================

void TestMneScanPlugins::babyMegStreamsTheSimulatedSystem()
{
    BABYMEGPLUGIN::BabyMEG plugin;
    plugin.init();
    {
        std::unique_ptr<QWidget> setup(plugin.setupWidget());
        auto* host = setup->findChild<QLineEdit*>("m_lineEdit_Host");
        auto* port = setup->findChild<QSpinBox*>("m_spinBox_Port");
        auto* commandPort = setup->findChild<QSpinBox*>("m_spinBox_CommandPort");
        auto* block = setup->findChild<QSpinBox*>("m_spinBox_SamplesPerBlock");
        auto* simulate = setup->findChild<QCheckBox*>("m_checkBox_Simulate");
        QVERIFY(host && port && commandPort && block && simulate);
        QCOMPARE(host->text(), QString("localhost"));
        QCOMPARE(port->value(), 6340);
        QCOMPARE(commandPort->value(), 6341);
        block->setValue(100);
        simulate->click();
    }

    QList<Eigen::MatrixXd> blocks;
    QMutex mutex;
    connect(plugin.getOutputConnectors().first().data(), &PluginOutputConnector::notify, this, [&blocks, &mutex](Measurement::SPtr pMeasurement) {
        QMutexLocker locker(&mutex);
        blocks.append(pMeasurement.dynamicCast<RealTimeMultiSampleArray>()->getMultiSampleArray()); }, Qt::DirectConnection);
    QVERIFY(plugin.start());
    QVERIFY(QTest::qWaitFor([&]() { QMutexLocker locker(&mutex); return blocks.size() >= 4; }, 10000));
    QVERIFY(plugin.stop());
    QVERIFY(plugin.wait(5000));

    // 384 magnetometers in the device frame, eight trigger lines and the trigger word, in tesla and volts
    const auto info = plugin.getOutputConnectors().first().dynamicCast<PluginOutputData<RealTimeMultiSampleArray>>()->measurementData()->info();
    QCOMPARE(info->chs.size(), std::size_t(393));
    QCOMPARE(info->sfreq, 1000.0);
    QCOMPARE(info->chs[0].kind, FIFFV_MEG_CH);
    QCOMPARE(info->chs[0].chpos.coil_type, FIFFV_COIL_BABY_MAG);
    QCOMPARE(info->chs[300].kind, FIFFV_REF_MEG_CH);
    QCOMPARE(info->chs[392].ch_name, std::string("DTRG01"));
    QVERIFY(!info->projs.empty()); // from the plugin's header.fif
    QMutexLocker locker(&mutex);
    for (const Eigen::MatrixXd& data : blocks) {
        QCOMPARE(data.rows(), 393);
        QCOMPARE(data.cols(), 100);
        QVERIFY(data.topRows(384).cwiseAbs().maxCoeff() < 1e-9);
        QVERIFY(data.topRows(384).cwiseAbs().maxCoeff() > 0.0);
    }
}

//=============================================================================================================

void TestMneScanPlugins::neuromagStreamsTheSimulatedAcquisition()
{
    NEUROMAGPLUGIN::Neuromag plugin;
    plugin.init();
    {
        std::unique_ptr<QWidget> setup(plugin.setupWidget());
        auto* root = setup->findChild<QLineEdit*>("m_lineEdit_Root");
        auto* buffer = setup->findChild<QSpinBox*>("m_spinBox_BufferSize");
        auto* block = setup->findChild<QSpinBox*>("m_spinBox_SamplesPerBlock");
        auto* autostart = setup->findChild<QCheckBox*>("m_checkBox_Autostart");
        auto* simulate = setup->findChild<QCheckBox*>("m_checkBox_Simulate");
        QVERIFY(root && buffer && block && autostart && simulate);
        root->setText("/no such dir & more"); // must reach the source encoded, and is unused when simulating
        buffer->setValue(50);
        block->setValue(25);
        simulate->click();
    }

    QList<Eigen::MatrixXd> blocks;
    QMutex mutex;
    connect(plugin.getOutputConnectors().first().data(), &PluginOutputConnector::notify, this, [&blocks, &mutex](Measurement::SPtr pMeasurement) {
        QMutexLocker locker(&mutex);
        blocks.append(pMeasurement.dynamicCast<RealTimeMultiSampleArray>()->getMultiSampleArray()); }, Qt::DirectConnection);
    QVERIFY(plugin.start());
    QVERIFY(QTest::qWaitFor([&]() { QMutexLocker locker(&mutex); return blocks.size() >= 4; }, 10000));
    QVERIFY(plugin.stop());
    QVERIFY(plugin.wait(5000));

    // 306 MEG channels in the device frame and the trigger channel, in tesla at 1 kHz
    const auto info = plugin.getOutputConnectors().first().dynamicCast<PluginOutputData<RealTimeMultiSampleArray>>()->measurementData()->info();
    QCOMPARE(info->chs.size(), std::size_t(307));
    QCOMPARE(info->sfreq, 1000.0);
    QCOMPARE(info->chs[0].kind, FIFFV_MEG_CH);
    QCOMPARE(info->chs[0].chpos.coil_type, FIFFV_COIL_VV_MAG_T3);
    QCOMPARE(info->chs[0].coord_frame, FIFFV_COORD_DEVICE);
    QCOMPARE(info->chs[306].ch_name, std::string("STI 014"));
    QCOMPARE(info->chs[306].kind, FIFFV_STIM_CH);
    QMutexLocker locker(&mutex);
    for (const Eigen::MatrixXd& data : blocks) {
        QCOMPARE(data.rows(), 307);
        QCOMPARE(data.cols(), 25);
        QVERIFY(data.topRows(306).cwiseAbs().maxCoeff() < 1e-9);
        QVERIFY(data.topRows(306).cwiseAbs().maxCoeff() > 0.0);
    }
}

//=============================================================================================================

void TestMneScanPlugins::eegoStreamsTheSimulatedAmplifier()
{
    EEGOSPORTSPLUGIN::EEGoSports plugin;
    plugin.init();
    {
        std::unique_ptr<QWidget> setup(plugin.setupWidget());
        auto* simulate = setup->findChild<QCheckBox*>("m_checkBox_Simulate");
        auto* rate = setup->findChild<QComboBox*>("m_comboBox_SamplingFreq");
        auto* block = setup->findChild<QSpinBox*>("m_spinBox_BlockSize");
        QVERIFY(simulate && rate && block);
        simulate->setChecked(true);
        rate->setCurrentText("1000");
        block->setValue(100);
    }

    QList<Eigen::MatrixXd> blocks;
    QMutex mutex;
    connect(plugin.getOutputConnectors().first().data(), &PluginOutputConnector::notify, this, [&blocks, &mutex](Measurement::SPtr pMeasurement) {
        QMutexLocker locker(&mutex);
        blocks.append(pMeasurement.dynamicCast<RealTimeMultiSampleArray>()->getMultiSampleArray()); }, Qt::DirectConnection);
    QVERIFY(plugin.start());
    QVERIFY(QTest::qWaitFor([&]() { QMutexLocker locker(&mutex); return blocks.size() >= 4; }, 10000));
    QVERIFY(plugin.stop());
    QVERIFY(plugin.wait(5000));
    QSettings("MNECPP").remove("EEGOSPORTS");

    // 64 referential inputs named and placed after the cap, 24 bipolar inputs, STI 014, the sample counter
    const auto info = plugin.getOutputConnectors().first().dynamicCast<PluginOutputData<RealTimeMultiSampleArray>>()->measurementData()->info();
    QCOMPARE(info->sfreq, 1000.0);
    QCOMPARE(info->chs.size(), std::size_t(90));
    QCOMPARE(info->chs[0].kind, FIFFV_EEG_CH);
    QVERIFY(info->chs[0].ch_name != "EEG 001");
    QVERIFY(info->chs[0].chpos.r0.norm() > 0.05); // on the head, in metres
    QCOMPARE(info->chs[64].ch_name, std::string("BIP 1"));
    QCOMPARE(info->chs[88].kind, FIFFV_STIM_CH);
    QMutexLocker locker(&mutex);
    for (const Eigen::MatrixXd& data : blocks) {
        QCOMPARE(data.rows(), 90);
        QCOMPARE(data.cols(), 100);
        QVERIFY(data.topRows(64).cwiseAbs().maxCoeff() < 1e-3);
        QVERIFY(data.topRows(64).cwiseAbs().maxCoeff() > 0.0);
    }
}

//=============================================================================================================

void TestMneScanPlugins::eegoImpedanceViewShowsTheMeasuredElectrodes()
{
    EEGOSPORTSPLUGIN::EEGoSports plugin;
    plugin.init();
    EEGOSPORTSPLUGIN::EEGoSportsImpedanceWidget widget(&plugin);
    widget.initGraphicScene();
    auto* view = widget.findChild<QGraphicsView*>("m_graphicsView_impedanceView");
    QVERIFY(view && view->scene());
    QCOMPARE(view->scene()->items().size(), 66); // the 66-electrode cap, wherever MNE Scan was started

    // A 32-input amplifier measures the first 32 electrodes of the cap
    Eigen::VectorXd ohms = Eigen::VectorXd::LinSpaced(32, 1e3, 4e3);
    widget.updateGraphicScene(ohms);
    int measured = 0;
    for (QGraphicsItem* graphicsItem : view->scene()->items()) {
        auto* item = static_cast<EEGOSPORTSPLUGIN::EEGoSportsElectrodeItem*>(graphicsItem);
        if (item->getChannelIndex() < 32) {
            QCOMPARE(item->getImpedanceValue(), ohms[item->getChannelIndex()]);
            ++measured;
        }
    }
    QCOMPARE(measured, 32);
}

//=============================================================================================================

void TestMneScanPlugins::tmsiStreamsTheSimulatedAmplifier()
{
    TMSIPLUGIN::TMSI plugin;
    plugin.init();
    {
        std::unique_ptr<QWidget> setup(plugin.setupWidget());
        auto* simulate = setup->findChild<QCheckBox*>("m_checkBox_Simulate");
        auto* keyboard = setup->findChild<QCheckBox*>("m_checkBox_EnableKeyboardTrigger");
        auto* rate = setup->findChild<QSpinBox*>("m_spinBox_SamplingFreq");
        auto* block = setup->findChild<QSpinBox*>("m_spinBox_SamplesPerBlock");
        QVERIFY(simulate && keyboard && rate && block);
        simulate->click();
        keyboard->click();
        rate->setValue(1024);
        block->setValue(64);
    }

    QList<Eigen::MatrixXd> blocks;
    QMutex mutex;
    connect(plugin.getOutputConnectors().first().data(), &PluginOutputConnector::notify, this, [&blocks, &mutex](Measurement::SPtr pMeasurement) {
        QMutexLocker locker(&mutex);
        blocks.append(pMeasurement.dynamicCast<RealTimeMultiSampleArray>()->getMultiSampleArray()); }, Qt::DirectConnection);
    QVERIFY(plugin.start());
    plugin.setKeyboardTriggerType(253);
    QVERIFY(QTest::qWaitFor([&]() { QMutexLocker locker(&mutex); return blocks.size() >= 4; }, 10000));
    QVERIFY(plugin.stop());
    QVERIFY(plugin.wait(5000));

    // 64 ExG inputs named after the cap's electrodes, 4 bipolar and 4 auxiliary inputs, STI 014, the test ramp
    const auto info = plugin.getOutputConnectors().first().dynamicCast<PluginOutputData<RealTimeMultiSampleArray>>()->measurementData()->info();
    QCOMPARE(info->chs.size(), std::size_t(74));
    QCOMPARE(info->chs[0].ch_name, std::string("EEG LM"));
    QCOMPARE(info->chs[0].kind, FIFFV_EEG_CH);
    QCOMPARE(info->chs[72].ch_name, std::string("STI 014"));
    QCOMPARE(info->chs[72].kind, FIFFV_STIM_CH);
    QVERIFY(info->chs[0].chpos.r0.norm() > 0.05); // on the head, in metres
    QCOMPARE(std::count_if(info->dig.begin(), info->dig.end(), [](const FiffDigPoint& p) { return p.kind == FIFFV_POINT_CARDINAL; }), 3);
    QMutexLocker locker(&mutex);
    bool keyed = false;
    for (const Eigen::MatrixXd& data : blocks) {
        QCOMPARE(data.rows(), 74);
        QCOMPARE(data.cols(), 64);
        QVERIFY(data.topRows(64).cwiseAbs().maxCoeff() < 1e-3);
        QVERIFY(data.topRows(64).cwiseAbs().maxCoeff() > 0.0);
        keyed |= data(72, 63) == 253.0;
    }
    QVERIFY(keyed); // the keyboard trigger lands on the last sample of a block
}

//=============================================================================================================

void TestMneScanPlugins::tmsiImpedanceViewShowsTheMeasuredElectrodes()
{
    TMSIPLUGIN::TMSI plugin;
    plugin.init();
    TMSIPLUGIN::TMSIImpedanceWidget widget(&plugin);
    widget.initGraphicScene();
    auto* view = widget.findChild<QGraphicsView*>("m_graphicsView_impedanceView");
    QVERIFY(view && view->scene());
    QCOMPARE(view->scene()->items().size(), 128); // the 128-electrode cap, wherever MNE Scan was started

    // A 64-input amplifier measures the first 64 electrodes of the cap
    Eigen::VectorXd ohms = Eigen::VectorXd::LinSpaced(64, 1e3, 64e3);
    widget.updateGraphicScene(ohms);
    int measured = 0;
    for (QGraphicsItem* graphicsItem : view->scene()->items()) {
        auto* item = static_cast<TMSIPLUGIN::TMSIElectrodeItem*>(graphicsItem);
        if (item->getChannelIndex() < 64) {
            QCOMPARE(item->getImpedanceValue(), ohms[item->getChannelIndex()]);
            ++measured;
        } else {
            QCOMPARE(item->getImpedanceValue(), 0.0);
        }
    }
    QCOMPARE(measured, 64);
}

//=============================================================================================================

void TestMneScanPlugins::mainWindowSwitchesStyle()
{
    QTemporaryDir noPlugins;
    qputenv("MNE_SCAN_PLUGIN_DIR", noPlugins.path().toLocal8Bit());
    MNESCAN::MainWindow window;
    qunsetenv("MNE_SCAN_PLUGIN_DIR");

    auto action = [&window](const QString& text) -> QAction* {
        for (QAction* candidate : window.findChildren<QAction*>()) {
            if (candidate->text() == text)
                return candidate;
        }
        return nullptr;
    };
    for (const char* text : {"Default", "Dark", "Research", "Clinical"})
        QVERIFY2(action(text), text);

    action("Dark")->trigger();
    QVERIFY2(!qApp->styleSheet().isEmpty(), "the dark stylesheet is not bundled");
    QCOMPARE(qApp->font().family(), QStringLiteral("Roboto"));
    action("Default")->trigger();
    QVERIFY(qApp->styleSheet().isEmpty());
}

//=============================================================================================================

void TestMneScanPlugins::lslAdapterUnloadsAndCloses()
{
    // MNE Scan unloads a plugin and then destroys it, with the event loop running in between
    auto adapter = std::make_unique<LSLADAPTERPLUGIN::LSLAdapter>();
    adapter->init();
    adapter->unload();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
    adapter.reset();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

//=============================================================================================================

QTEST_MAIN(TestMneScanPlugins)

#include "test_mne_scan_plugins.moc"
