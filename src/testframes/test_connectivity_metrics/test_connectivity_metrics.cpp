#include <QtTest/QtTest>
#include <Eigen/Dense>
#include <connectivity/connectivity.h>
#include <connectivity/connectivitysettings.h>
#include <connectivity/network/network.h>
#include <connectivity/metrics/correlation.h>

using namespace CONNECTIVITYLIB;
using namespace Eigen;

class TestConnectivityMetrics : public QObject
{
    Q_OBJECT

private slots:
    void testConnectivityDefaultCtor()
    {
        Connectivity conn;
        Q_UNUSED(conn);
        QVERIFY(true);
    }

    void testConnectivitySettingsCtor()
    {
        ConnectivitySettings settings;
        QVERIFY(settings.isEmpty());
        QCOMPARE(settings.size(), 0);
    }

    void testConnectivitySettingsAppendMatrix()
    {
        ConnectivitySettings settings;
        // 4 channels, 100 samples
        MatrixXd trial = MatrixXd::Random(4, 100);
        settings.append(trial);
        QCOMPARE(settings.size(), 1);
        QVERIFY(!settings.isEmpty());
    }

    void testConnectivitySettingsAppendList()
    {
        ConnectivitySettings settings;
        QList<MatrixXd> trials;
        trials.append(MatrixXd::Random(4, 100));
        trials.append(MatrixXd::Random(4, 100));
        settings.append(trials);
        QCOMPARE(settings.size(), 2);
    }

    void testConnectivitySettingsSFreq()
    {
        ConnectivitySettings settings;
        settings.setSamplingFrequency(1000);
        QCOMPARE(settings.getSamplingFrequency(), 1000);
    }

    void testConnectivitySettingsFFTSize()
    {
        ConnectivitySettings settings;
        settings.setFFTSize(256);
        QCOMPARE(settings.getFFTSize(), 256);
    }

    void testConnectivitySettingsWindowType()
    {
        ConnectivitySettings settings;
        settings.setWindowType("hanning");
        QCOMPARE(settings.getWindowType(), QString("hanning"));
    }

    void testConnectivitySettingsMethods()
    {
        ConnectivitySettings settings;
        QStringList methods;
        methods << "COR" << "XCOR";
        settings.setConnectivityMethods(methods);
        QCOMPARE(settings.getConnectivityMethods().size(), 2);
        QCOMPARE(settings.getConnectivityMethods().at(0), QString("COR"));
    }

    void testConnectivitySettingsNodePositions()
    {
        ConnectivitySettings settings;
        MatrixX3f pos(4, 3);
        pos << 0,0,0, 1,0,0, 0,1,0, 0,0,1;
        settings.setNodePositions(pos);
        QCOMPARE(settings.getNodePositions().rows(), 4);
    }

    void testConnectivitySettingsClearAll()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 100));
        settings.clearAllData();
        QCOMPARE(settings.size(), 0);
    }

    void testConnectivitySettingsClearIntermediate()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 100));
        settings.clearIntermediateData();
        // Data should still exist but intermediate cleared
        QCOMPARE(settings.size(), 1);
    }

    void testConnectivitySettingsRemoveFirst()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 100));
        settings.append(MatrixXd::Random(4, 100));
        settings.append(MatrixXd::Random(4, 100));
        settings.removeFirst(1);
        QCOMPARE(settings.size(), 2);
    }

    void testConnectivitySettingsRemoveLast()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 100));
        settings.append(MatrixXd::Random(4, 100));
        settings.removeLast(1);
        QCOMPARE(settings.size(), 1);
    }

    void testCalculateCorrelation()
    {
        ConnectivitySettings settings;
        // Create synthetic data: 4 channels, 200 samples, 2 trials
        settings.append(MatrixXd::Random(4, 200));
        settings.append(MatrixXd::Random(4, 200));
        settings.setSamplingFrequency(1000);
        settings.setFFTSize(256);
        settings.setWindowType("hanning");

        QStringList methods;
        methods << "COR";
        settings.setConnectivityMethods(methods);

        MatrixX3f nodePos(4, 3);
        nodePos << 0,0,0, 1,0,0, 0,1,0, 0,0,1;
        settings.setNodePositions(nodePos);

        QList<Network> networks = Connectivity::calculate(settings);
        QCOMPARE(networks.size(), 1);
    }

    void testCalculateCrossCorrelation()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 200));
        settings.append(MatrixXd::Random(4, 200));
        settings.setSamplingFrequency(1000);
        settings.setFFTSize(256);
        settings.setWindowType("hanning");

        QStringList methods;
        methods << "XCOR";
        settings.setConnectivityMethods(methods);

        MatrixX3f nodePos(4, 3);
        nodePos << 0,0,0, 1,0,0, 0,1,0, 0,0,1;
        settings.setNodePositions(nodePos);

        QList<Network> networks = Connectivity::calculate(settings);
        QCOMPARE(networks.size(), 1);
    }

    void testCalculateCoherence()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 200));
        settings.setSamplingFrequency(1000);
        settings.setFFTSize(128);
        settings.setWindowType("hanning");

        QStringList methods;
        methods << "COH";
        settings.setConnectivityMethods(methods);

        MatrixX3f nodePos(4, 3);
        nodePos << 0,0,0, 1,0,0, 0,1,0, 0,0,1;
        settings.setNodePositions(nodePos);

        QList<Network> networks = Connectivity::calculate(settings);
        QCOMPARE(networks.size(), 1);
    }

    void testCalculateImagCoherence()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 200));
        settings.setSamplingFrequency(1000);
        settings.setFFTSize(128);
        settings.setWindowType("hanning");

        QStringList methods;
        methods << "IMAGCOH";
        settings.setConnectivityMethods(methods);

        MatrixX3f nodePos(4, 3);
        nodePos << 0,0,0, 1,0,0, 0,1,0, 0,0,1;
        settings.setNodePositions(nodePos);

        QList<Network> networks = Connectivity::calculate(settings);
        QCOMPARE(networks.size(), 1);
    }

    void testCalculatePLI()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 200));
        settings.setSamplingFrequency(1000);
        settings.setFFTSize(128);
        settings.setWindowType("hanning");

        QStringList methods;
        methods << "PLI";
        settings.setConnectivityMethods(methods);

        MatrixX3f nodePos(4, 3);
        nodePos << 0,0,0, 1,0,0, 0,1,0, 0,0,1;
        settings.setNodePositions(nodePos);

        QList<Network> networks = Connectivity::calculate(settings);
        QCOMPARE(networks.size(), 1);
    }

    void testCalculatePLV()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 200));
        settings.setSamplingFrequency(1000);
        settings.setFFTSize(128);
        settings.setWindowType("hanning");

        QStringList methods;
        methods << "PLV";
        settings.setConnectivityMethods(methods);

        MatrixX3f nodePos(4, 3);
        nodePos << 0,0,0, 1,0,0, 0,1,0, 0,0,1;
        settings.setNodePositions(nodePos);

        QList<Network> networks = Connectivity::calculate(settings);
        QCOMPARE(networks.size(), 1);
    }

    void testCalculateWPLI()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 200));
        settings.setSamplingFrequency(1000);
        settings.setFFTSize(128);
        settings.setWindowType("hanning");

        QStringList methods;
        methods << "WPLI";
        settings.setConnectivityMethods(methods);

        MatrixX3f nodePos(4, 3);
        nodePos << 0,0,0, 1,0,0, 0,1,0, 0,0,1;
        settings.setNodePositions(nodePos);

        QList<Network> networks = Connectivity::calculate(settings);
        QCOMPARE(networks.size(), 1);
    }

    void testCalculateUSPLI()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 200));
        settings.setSamplingFrequency(1000);
        settings.setFFTSize(128);
        settings.setWindowType("hanning");

        QStringList methods;
        methods << "USPLI";
        settings.setConnectivityMethods(methods);

        MatrixX3f nodePos(4, 3);
        nodePos << 0,0,0, 1,0,0, 0,1,0, 0,0,1;
        settings.setNodePositions(nodePos);

        QList<Network> networks = Connectivity::calculate(settings);
        QCOMPARE(networks.size(), 1);
    }

    void testCalculateDSWPLI()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 200));
        settings.setSamplingFrequency(1000);
        settings.setFFTSize(128);
        settings.setWindowType("hanning");

        QStringList methods;
        methods << "DSWPLI";
        settings.setConnectivityMethods(methods);

        MatrixX3f nodePos(4, 3);
        nodePos << 0,0,0, 1,0,0, 0,1,0, 0,0,1;
        settings.setNodePositions(nodePos);

        QList<Network> networks = Connectivity::calculate(settings);
        QCOMPARE(networks.size(), 1);
    }

    void testCalculateMultipleMethods()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 200));
        settings.setSamplingFrequency(1000);
        settings.setFFTSize(128);
        settings.setWindowType("hanning");

        QStringList methods;
        methods << "COR" << "COH" << "PLI";
        settings.setConnectivityMethods(methods);

        MatrixX3f nodePos(4, 3);
        nodePos << 0,0,0, 1,0,0, 0,1,0, 0,0,1;
        settings.setNodePositions(nodePos);

        QList<Network> networks = Connectivity::calculate(settings);
        QCOMPARE(networks.size(), 3);
    }
};

QTEST_GUILESS_MAIN(TestConnectivityMetrics)
#include "test_connectivity_metrics.moc"
