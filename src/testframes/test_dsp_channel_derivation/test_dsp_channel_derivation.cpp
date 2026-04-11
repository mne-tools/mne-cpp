//=============================================================================================================
/**
 * @file     test_dsp_channel_derivation.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
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
 * @brief    Tests for ChannelDerivation (bipolar, CAR, custom rules).
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dsp/channel_derivation.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>
#include <QTemporaryDir>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DSPLIB;
using namespace UTILSLIB;
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
    QStringList channels = {"EEG01", "EEG02", "EEG03", "EEG04"};
    QVector<DerivationRule> rules = ChannelDerivation::buildBipolar(channels);

    // Bipolar: n-1 pairs
    QCOMPARE(rules.size(), 3);

    // Each rule should have exactly 2 input weights: +1 and -1
    for (const DerivationRule& rule : rules) {
        QCOMPARE(rule.inputWeights.size(), 2);
        double sumWeights = 0;
        for (auto it = rule.inputWeights.begin(); it != rule.inputWeights.end(); ++it) {
            sumWeights += it.value();
        }
        QVERIFY(qAbs(sumWeights) < 1e-10); // Weights sum to zero
    }
}

//=============================================================================================================

void TestDspChannelDerivation::testBipolarApply()
{
    QStringList channels = {"C1", "C2", "C3"};
    int nSamples = 100;

    // Create known data
    MatrixXd data(3, nSamples);
    for (int s = 0; s < nSamples; ++s) {
        data(0, s) = 1.0;
        data(1, s) = 3.0;
        data(2, s) = 6.0;
    }

    QVector<DerivationRule> rules = ChannelDerivation::buildBipolar(channels);
    auto [derived, names] = ChannelDerivation::apply(data, channels, rules);

    QCOMPARE(derived.rows(), 2);     // 2 bipolar pairs
    QCOMPARE(derived.cols(), nSamples);
    QCOMPARE(names.size(), 2);

    // C1-C2 = 1.0 - 3.0 = -2.0, C2-C3 = 3.0 - 6.0 = -3.0
    QVERIFY(qAbs(derived(0, 0) - (-2.0)) < 1e-10);
    QVERIFY(qAbs(derived(1, 0) - (-3.0)) < 1e-10);
}

//=============================================================================================================

void TestDspChannelDerivation::testBipolarNaming()
{
    QStringList channels = {"Fp1", "Fp2", "F3"};
    QVector<DerivationRule> rules = ChannelDerivation::buildBipolar(channels);

    // Output names should reflect the bipolar pair
    for (const DerivationRule& rule : rules) {
        QVERIFY(!rule.outputName.isEmpty());
    }
}

//=============================================================================================================

void TestDspChannelDerivation::testBuildCommonAverage()
{
    QStringList channels = {"C1", "C2", "C3", "C4"};
    QVector<DerivationRule> rules = ChannelDerivation::buildCommonAverage(channels);

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
    QStringList channels = {"C1", "C2", "C3"};
    int nSamples = 50;

    MatrixXd data(3, nSamples);
    for (int s = 0; s < nSamples; ++s) {
        data(0, s) = 1.0;
        data(1, s) = 2.0;
        data(2, s) = 3.0;
    }

    QVector<DerivationRule> rules = ChannelDerivation::buildCommonAverage(channels);
    auto [derived, names] = ChannelDerivation::apply(data, channels, rules);

    QCOMPARE(derived.rows(), 3);
    QCOMPARE(names.size(), 3);
}

//=============================================================================================================

void TestDspChannelDerivation::testCommonAverageZeroMean()
{
    QStringList channels = {"C1", "C2", "C3", "C4"};
    int nSamples = 100;

    MatrixXd data = MatrixXd::Random(4, nSamples);

    QVector<DerivationRule> rules = ChannelDerivation::buildCommonAverage(channels);
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
    QStringList channels = {"A", "B", "C"};
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
    rule.inputWeights.insert("A", 2.0);
    rule.inputWeights.insert("B", 0.5);
    rule.inputWeights.insert("C", -1.0);

    QVector<DerivationRule> rules;
    rules.append(rule);

    auto [derived, names] = ChannelDerivation::apply(data, channels, rules);

    QCOMPARE(derived.rows(), 1);
    // 2*1 + 0.5*2 - 3 = 2 + 1 - 3 = 0
    QVERIFY(qAbs(derived(0, 0) - 0.0) < 1e-10);
}

//=============================================================================================================

void TestDspChannelDerivation::testApplyOutputDimensions()
{
    QStringList channels = {"C1", "C2", "C3"};
    int nSamples = 50;
    MatrixXd data = MatrixXd::Random(3, nSamples);

    // 2 custom rules
    DerivationRule r1, r2;
    r1.outputName = "r1";
    r1.inputWeights.insert("C1", 1.0);
    r2.outputName = "r2";
    r2.inputWeights.insert("C2", 1.0);
    r2.inputWeights.insert("C3", -1.0);

    QVector<DerivationRule> rules = {r1, r2};
    auto [derived, names] = ChannelDerivation::apply(data, channels, rules);

    QCOMPARE(derived.rows(), 2);
    QCOMPARE(derived.cols(), nSamples);
    QCOMPARE(names.size(), 2);
    QCOMPARE(names[0], QString("r1"));
    QCOMPARE(names[1], QString("r2"));
}

//=============================================================================================================

void TestDspChannelDerivation::testWriteReadDefinitionFile()
{
    DerivationRule r1;
    r1.outputName = "Bipolar1";
    r1.inputWeights.insert("EEG01", 1.0);
    r1.inputWeights.insert("EEG02", -1.0);

    DerivationRule r2;
    r2.outputName = "Bipolar2";
    r2.inputWeights.insert("EEG02", 1.0);
    r2.inputWeights.insert("EEG03", -1.0);

    QVector<DerivationRule> rules = {r1, r2};

    QString path = m_tempDir.filePath("derivations.json");
    QVERIFY(ChannelDerivation::writeDefinitionFile(path, rules));

    QVector<DerivationRule> loaded = ChannelDerivation::readDefinitionFile(path);
    QCOMPARE(loaded.size(), 2);
    QCOMPARE(loaded[0].outputName, QString("Bipolar1"));
    QCOMPARE(loaded[1].outputName, QString("Bipolar2"));
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
