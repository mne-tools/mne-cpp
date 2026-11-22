//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_dsp_channel_derivation.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Tests for ChannelDerivation (bipolar, CAR, custom rules).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/dsp/channel_derivation.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>
#include <QTemporaryDir>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestDspChannelDerivation
 *
 * @brief The TestDspChannelDerivation class provides tests for channel derivation computations.
 */
class TestDspChannelDerivation : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // Bipolar
    void testBuildBipolar();
    void testBipolarApply();
    void testBipolarNaming();

    // Common Average Reference
    void testBuildCommonAverage();
    void testCommonAverageApply();
    void testCommonAverageZeroMean();

    // Apply
    void testApplyCustomRules();
    void testApplyOutputDimensions();

    // File I/O
    void testWriteReadDefinitionFile();

    void cleanupTestCase();

private:
    QTemporaryDir m_tempDir;
};

//=============================================================================================================

void TestDspChannelDerivation::initTestCase()
{
    QVERIFY(m_tempDir.isValid());
}

//=============================================================================================================

void TestDspChannelDerivation::testBuildBipolar()
{
    StringList channels = {"EEG01", "EEG02", "EEG03", "EEG04"};
    std::vector<DerivationRule> rules = ChannelDerivation::buildBipolar(channels);

    // Bipolar: n-1 pairs
    QCOMPARE(rules.size(), 3);

    // Each rule should have exactly 2 input weights: +1 and -1
    for (const DerivationRule& rule : rules) {
        QCOMPARE(rule.inputWeights.size(), 2);
        double sumWeights = 0;
        for (auto it = rule.inputWeights.begin(); it != rule.inputWeights.end(); ++it) {
            sumWeights += it->second;
        }
        QVERIFY(qAbs(sumWeights) < 1e-10); // Weights sum to zero
    }
}

//=============================================================================================================

void TestDspChannelDerivation::testBipolarApply()
{
    StringList channels = {"C1", "C2", "C3"};
    int nSamples = 100;

    // Create known data
    MatrixXd data(3, nSamples);
    for (int s = 0; s < nSamples; ++s) {
        data(0, s) = 1.0;
        data(1, s) = 3.0;
        data(2, s) = 6.0;
    }

    std::vector<DerivationRule> rules = ChannelDerivation::buildBipolar(channels);
    auto [derived, names] = ChannelDerivation::apply(data, channels, rules);

    QCOMPARE(derived.rows(), 2); // 2 bipolar pairs
    QCOMPARE(derived.cols(), nSamples);
    QCOMPARE(names.size(), 2);

    // C1-C2 = 1.0 - 3.0 = -2.0, C2-C3 = 3.0 - 6.0 = -3.0
    QVERIFY(qAbs(derived(0, 0) - (-2.0)) < 1e-10);
    QVERIFY(qAbs(derived(1, 0) - (-3.0)) < 1e-10);
}

//=============================================================================================================

void TestDspChannelDerivation::testBipolarNaming()
{
    StringList channels = {"Fp1", "Fp2", "F3"};
    std::vector<DerivationRule> rules = ChannelDerivation::buildBipolar(channels);

    // Output names should reflect the bipolar pair
    for (const DerivationRule& rule : rules) {
        QVERIFY(!rule.outputName.empty());
    }

    // Pairs stay within a shaft, shafts in first-seen order, channels in input order
    const std::vector<DerivationRule> shafts = ChannelDerivation::buildBipolar({"LH1", "RA1", "LH2", "A'1", "RA2", "LH3", "A'2"});
    StringList names;
    for (const DerivationRule& rule : shafts) {
        names.push_back(rule.outputName);
    }
    QCOMPARE(names, (StringList{"LH1-LH2", "LH2-LH3", "RA1-RA2", "A'1-A'2"}));
}

//=============================================================================================================

void TestDspChannelDerivation::testBuildCommonAverage()
{
    StringList channels = {"C1", "C2", "C3", "C4"};
    std::vector<DerivationRule> rules = ChannelDerivation::buildCommonAverage(channels);

    // CAR: one rule per channel
    QCOMPARE(rules.size(), 4);

    // Each rule should reference all channels
    for (const DerivationRule& rule : rules) {
        QCOMPARE(rule.inputWeights.size(), 4);
    }
}

//=============================================================================================================

void TestDspChannelDerivation::testCommonAverageApply()
{
    StringList channels = {"C1", "C2", "C3"};
    int nSamples = 50;

    MatrixXd data(3, nSamples);
    for (int s = 0; s < nSamples; ++s) {
        data(0, s) = 1.0;
        data(1, s) = 2.0;
        data(2, s) = 3.0;
    }

    std::vector<DerivationRule> rules = ChannelDerivation::buildCommonAverage(channels);
    auto [derived, names] = ChannelDerivation::apply(data, channels, rules);

    QCOMPARE(derived.rows(), 3);
    QCOMPARE(names.size(), 3);
}

//=============================================================================================================

void TestDspChannelDerivation::testCommonAverageZeroMean()
{
    StringList channels = {"C1", "C2", "C3", "C4"};
    int nSamples = 100;

    MatrixXd data = MatrixXd::Random(4, nSamples);

    std::vector<DerivationRule> rules = ChannelDerivation::buildCommonAverage(channels);
    auto [derived, names] = ChannelDerivation::apply(data, channels, rules);

    // After CAR, the mean across channels at each time point should be ~0
    for (int s = 0; s < nSamples; ++s) {
        double mean = derived.col(s).mean();
        QVERIFY(qAbs(mean) < 1e-10);
    }
}

//=============================================================================================================

void TestDspChannelDerivation::testApplyCustomRules()
{
    StringList channels = {"A", "B", "C"};
    int nSamples = 10;

    MatrixXd data(3, nSamples);
    for (int s = 0; s < nSamples; ++s) {
        data(0, s) = 1.0; // A
        data(1, s) = 2.0; // B
        data(2, s) = 3.0; // C
    }

    // Custom rule: 2*A + 0.5*B - C
    DerivationRule rule;
    rule.outputName = "custom";
    rule.inputWeights.emplace("A", 2.0);
    rule.inputWeights.emplace("B", 0.5);
    rule.inputWeights.emplace("C", -1.0);

    std::vector<DerivationRule> rules;
    rules.push_back(rule);

    auto [derived, names] = ChannelDerivation::apply(data, channels, rules);

    QCOMPARE(derived.rows(), 1);
    // 2*1 + 0.5*2 - 3 = 2 + 1 - 3 = 0
    QVERIFY(qAbs(derived(0, 0) - 0.0) < 1e-10);
}

//=============================================================================================================

void TestDspChannelDerivation::testApplyOutputDimensions()
{
    StringList channels = {"C1", "C2", "C3"};
    int nSamples = 50;
    MatrixXd data = MatrixXd::Random(3, nSamples);

    // 2 custom rules
    DerivationRule r1, r2;
    r1.outputName = "r1";
    r1.inputWeights.emplace("C1", 1.0);
    r2.outputName = "r2";
    r2.inputWeights.emplace("C2", 1.0);
    r2.inputWeights.emplace("C3", -1.0);

    std::vector<DerivationRule> rules = {r1, r2};
    auto [derived, names] = ChannelDerivation::apply(data, channels, rules);

    QCOMPARE(derived.rows(), 2);
    QCOMPARE(derived.cols(), nSamples);
    QCOMPARE(names.size(), 2);
    QCOMPARE(names[0], std::string("r1"));
    QCOMPARE(names[1], std::string("r2"));
}

//=============================================================================================================

void TestDspChannelDerivation::testWriteReadDefinitionFile()
{
    DerivationRule r1;
    r1.outputName = "Bipolar1";
    r1.inputWeights.emplace("EEG01", 1.0);
    r1.inputWeights.emplace("EEG02", -1.0);

    DerivationRule r2;
    r2.outputName = "Bipolar2";
    r2.inputWeights.emplace("EEG02", 1.0);
    r2.inputWeights.emplace("EEG03", -1.0);

    std::vector<DerivationRule> rules = {r1, r2};

    const std::filesystem::path path = m_tempDir.filePath("derivations.txt").toStdU16String();
    QVERIFY(ChannelDerivation::writeDefinitionFile(path, rules));

    std::vector<DerivationRule> loaded = ChannelDerivation::readDefinitionFile(path);
    QCOMPARE(loaded.size(), 2);
    QCOMPARE(loaded[0].outputName, std::string("Bipolar1"));
    QCOMPARE(loaded[1].outputName, std::string("Bipolar2"));
    QCOMPARE(loaded[0].inputWeights, r1.inputWeights);
    QCOMPARE(loaded[1].inputWeights, r2.inputWeights);
}

//=============================================================================================================

void TestDspChannelDerivation::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestDspChannelDerivation)
#include "test_dsp_channel_derivation.moc"
