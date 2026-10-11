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
#include <covariance/covariance.h>
#include <hpi/hpi.h>
#include <neuronalconnectivity/neuronalconnectivity.h>
#include <noisereduction/noisereduction.h>
#include <rtclustmne/rtclustmne.h>
#include <rtfwd/rtfwd.h>

#include <scMeas/realtimecov.h>
#include <scMeas/realtimeevokedset.h>
#include <scMeas/realtimemultisamplearray.h>
#include <scShared/Management/plugininputconnector.h>
#include <scShared/Management/pluginoutputconnector.h>

#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_raw_data.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <QtTest>

#include <memory>

//=============================================================================================================

using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace FIFFLIB;

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
    void averagesTheStimulusEvents();
    void estimatesTheCovariance();
    void passesTheStreamThrough();

private:
    static void addPluginRows();
    std::unique_ptr<AbstractPlugin> makePlugin(const QString& name) const;
    QList<Measurement::SPtr> stream(AbstractPlugin& plugin, int outputs = 1);

    QString m_rawPath;
    QSharedPointer<FiffInfo> m_info;
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
    QFile file(m_rawPath);
    FiffRawData raw(file);
    Eigen::MatrixXd times;
    QVERIFY(raw.read_raw_segment(m_data, times));
    m_info = QSharedPointer<FiffInfo>::create(raw.info);
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
    for (const char* attribute : {"bemname", "mriname", "measname"})
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
    QVERIFY(!set.evoked.isEmpty());

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

QTEST_MAIN(TestMneScanPlugins)
#include "test_mne_scan_plugins.moc"
