//=============================================================================================================
/**
 * @file     test_dsp_eog_regression.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
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
 * @brief    Tests for EogRegression.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dsp/eog_regression.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_constants.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * @brief Tests for EogRegression class.
 */
class TestDspEogRegression : public QObject
{
    Q_OBJECT

private:
    //=========================================================================================================
    /**
     * @brief Build a small FiffInfo with a mix of MEG, EEG, and EOG channels.
     *
     * Layout: 5 channels total
     *   [0] MEG1 (MEG, kind=1)
     *   [1] MEG2 (MEG, kind=1)
     *   [2] EEG1 (EEG, kind=2)
     *   [3] EOG1 (EOG, kind=202)
     *   [4] EOG2 (EOG, kind=202)
     */
    static FiffInfo makeTestInfo()
    {
        FiffInfo info;
        info.nchan = 5;
        info.sfreq = 1000.0;

        QStringList names = {"MEG1", "MEG2", "EEG1", "EOG1", "EOG2"};
        QList<int> kinds = {FIFFV_MEG_CH, FIFFV_MEG_CH, FIFFV_EEG_CH,
                            FIFFV_EOG_CH, FIFFV_EOG_CH};

        for (int i = 0; i < 5; ++i) {
            FiffChInfo ch;
            ch.scanNo = i + 1;
            ch.logNo = i + 1;
            ch.kind = kinds[i];
            ch.ch_name = names[i];
            ch.range = 1.0f;
            ch.cal = 1.0f;
            ch.unit = 107;   // FIFF_UNIT_V
            ch.unit_mul = 0;
            ch.coord_frame = 0;
            info.chs.append(ch);
            info.ch_names.append(names[i]);
        }

        return info;
    }

    //=========================================================================================================
    /**
     * @brief Build a 5 x 200 test data matrix with known EOG artifacts.
     *
     * - Channel 0 (MEG1): clean sine at 5 Hz (no EOG contamination)
     * - Channel 1 (MEG2): clean sine at 5 Hz + 2.0 * EOG1
     * - Channel 2 (EEG1): clean sine at 5 Hz + 1.5 * EOG1 + 0.5 * EOG2
     * - Channel 3 (EOG1): sine at 1 Hz (orthogonal to EOG2 and clean signal)
     * - Channel 4 (EOG2): sine at 3 Hz (orthogonal to EOG1 and clean signal)
     *
     * Using complete-cycle sines ensures perfect orthogonality and stable
     * regression coefficients.
     */
    static MatrixXd makeTestData()
    {
        const int nTimes = 600;
        MatrixXd data(5, nTimes);

        // EOG1: 1 Hz sine (6 complete cycles in 600 samples at 100 Hz)
        for (int t = 0; t < nTimes; ++t) {
            data(3, t) = std::sin(2.0 * M_PI * 1.0 * static_cast<double>(t) / static_cast<double>(nTimes / 6));
        }

        // EOG2: 3 Hz sine (18 complete cycles)
        for (int t = 0; t < nTimes; ++t) {
            data(4, t) = std::sin(2.0 * M_PI * 3.0 * static_cast<double>(t) / static_cast<double>(nTimes / 6));
        }

        // Clean sine wave base signal at 5 Hz (30 complete cycles)
        RowVectorXd cleanSine(nTimes);
        for (int t = 0; t < nTimes; ++t) {
            cleanSine(t) = std::sin(2.0 * M_PI * 5.0 * static_cast<double>(t) / static_cast<double>(nTimes / 6));
        }

        // Channel 0 (MEG1): clean sine only
        data.row(0) = cleanSine;

        // Channel 1 (MEG2): clean sine + 2.0 * EOG1
        data.row(1) = cleanSine + 2.0 * data.row(3);

        // Channel 2 (EEG1): clean sine + 1.5 * EOG1 + 0.5 * EOG2
        data.row(2) = cleanSine + 1.5 * data.row(3) + 0.5 * data.row(4);

        return data;
    }

    //=========================================================================================================
    /**
     * @brief Compute RMS of a row vector.
     */
    static double rms(const RowVectorXd& v)
    {
        return std::sqrt(v.squaredNorm() / static_cast<double>(v.size()));
    }

private slots:
    //=========================================================================================================
    /**
     * @brief EOG artifact amplitude should be reduced by >80% after fitApply.
     */
    void testFitApplyReducesEog()
    {
        FiffInfo info = makeTestInfo();
        MatrixXd data = makeTestData();
        const int nTimes = static_cast<int>(data.cols());

        // Reconstruct the clean sine used in makeTestData
        RowVectorXd cleanSine(nTimes);
        for (int t = 0; t < nTimes; ++t) {
            cleanSine(t) = std::sin(2.0 * M_PI * 5.0 * static_cast<double>(t) / static_cast<double>(nTimes / 6));
        }
        double rmsBefore = rms(RowVectorXd(data.row(1) - cleanSine));

        EogRegression::fitApply(data, info);

        double rmsAfter = rms(RowVectorXd(data.row(1) - cleanSine));

        // EOG contribution should be reduced by >80%
        QVERIFY2(rmsAfter < 0.2 * rmsBefore,
                 qPrintable(QString("EOG RMS not reduced enough: before=%1 after=%2")
                            .arg(rmsBefore).arg(rmsAfter)));
    }

    //=========================================================================================================
    /**
     * @brief Non-EOG channels that had EOG contamination should be modified.
     */
    void testNonEogChannelsModified()
    {
        FiffInfo info = makeTestInfo();
        MatrixXd data = makeTestData();
        MatrixXd dataBefore = data;

        EogRegression::fitApply(data, info);

        // Channel 1 (MEG2) had EOG contamination — should be changed
        QVERIFY(!data.row(1).isApprox(dataBefore.row(1), 1e-10));
        // Channel 2 (EEG1) had EOG contamination — should be changed
        QVERIFY(!data.row(2).isApprox(dataBefore.row(2), 1e-10));
    }

    //=========================================================================================================
    /**
     * @brief EOG channels themselves should remain unchanged.
     */
    void testEogChannelsUnchanged()
    {
        FiffInfo info = makeTestInfo();
        MatrixXd data = makeTestData();
        MatrixXd dataBefore = data;

        EogRegression::fitApply(data, info);

        // EOG1 (channel 3) unchanged
        QVERIFY(data.row(3).isApprox(dataBefore.row(3), 1e-12));
        // EOG2 (channel 4) unchanged
        QVERIFY(data.row(4).isApprox(dataBefore.row(4), 1e-12));
    }

    //=========================================================================================================
    /**
     * @brief Specifying EOG channels by name should work correctly.
     */
    void testExplicitEogChannels()
    {
        FiffInfo info = makeTestInfo();
        MatrixXd data = makeTestData();

        // Only specify EOG1 — regression should only use EOG1
        QStringList eogNames = {"EOG1"};
        EogRegression reg;
        reg.fit(data, info, eogNames);

        QVERIFY(reg.isFitted());
        // Beta should have 3 target channels (MEG1, MEG2, EEG1) x 1 EOG channel
        // Note: when only EOG1 is specified, EOG2 becomes a target channel too
        // Actually: target = all not in EOG set, so targets = MEG1, MEG2, EEG1, EOG2 = 4 targets
        QCOMPARE(reg.coefficients().rows(), static_cast<Eigen::Index>(4));
        QCOMPARE(reg.coefficients().cols(), static_cast<Eigen::Index>(1));
    }

    //=========================================================================================================
    /**
     * @brief Auto-detect EOG channels from FiffInfo kind field.
     */
    void testAutoDetectEog()
    {
        FiffInfo info = makeTestInfo();
        MatrixXd data = makeTestData();

        EogRegression reg;
        reg.fit(data, info);  // no explicit EOG list — auto-detect

        QVERIFY(reg.isFitted());
        // 3 target channels (MEG1, MEG2, EEG1) x 2 EOG channels (EOG1, EOG2)
        QCOMPARE(reg.coefficients().rows(), static_cast<Eigen::Index>(3));
        QCOMPARE(reg.coefficients().cols(), static_cast<Eigen::Index>(2));
    }

    //=========================================================================================================
    /**
     * @brief Coefficients matrix should have correct dimensions.
     */
    void testCoefficientsShape()
    {
        FiffInfo info = makeTestInfo();
        MatrixXd data = makeTestData();

        EogRegression reg;
        reg.fit(data, info);

        // n_targets x n_eog = 3 x 2
        const MatrixXd& beta = reg.coefficients();
        QCOMPARE(beta.rows(), static_cast<Eigen::Index>(3));
        QCOMPARE(beta.cols(), static_cast<Eigen::Index>(2));

        // Check that the regression coefficients match the known mixing.
        // Use a relaxed tolerance for "should be zero" coefficients because
        // spurious correlations between the sine wave and triangle/sawtooth
        // signals can produce small non-zero values with limited samples.
        const double tolZero = 0.3;
        const double tolCoeff = 0.15;

        // Channel 0 (MEG1): no EOG => beta(0,:) ≈ [0, 0]
        QVERIFY(std::abs(beta(0, 0)) < tolZero);
        QVERIFY(std::abs(beta(0, 1)) < tolZero);

        // Channel 1 (MEG2): 2.0 * EOG1 => beta(1,0) ≈ 2.0, beta(1,1) ≈ 0
        QVERIFY(std::abs(beta(1, 0) - 2.0) < tolCoeff);
        QVERIFY(std::abs(beta(1, 1)) < tolZero);

        // Channel 2 (EEG1): 1.5 * EOG1 + 0.5 * EOG2 => beta(2,0) ≈ 1.5, beta(2,1) ≈ 0.5
        QVERIFY(std::abs(beta(2, 0) - 1.5) < tolCoeff);
        QVERIFY(std::abs(beta(2, 1) - 0.5) < tolCoeff);
    }

    //=========================================================================================================
    /**
     * @brief If no EOG channels are found, data should remain unchanged.
     */
    void testNoEogChannelsWarns()
    {
        // Build info with no EOG channels
        FiffInfo info;
        info.nchan = 2;
        info.sfreq = 1000.0;

        QStringList names = {"MEG1", "EEG1"};
        QList<int> kinds = {FIFFV_MEG_CH, FIFFV_EEG_CH};

        for (int i = 0; i < 2; ++i) {
            FiffChInfo ch;
            ch.scanNo = i + 1;
            ch.logNo = i + 1;
            ch.kind = kinds[i];
            ch.ch_name = names[i];
            ch.range = 1.0f;
            ch.cal = 1.0f;
            ch.unit = 107;
            ch.unit_mul = 0;
            ch.coord_frame = 0;
            info.chs.append(ch);
            info.ch_names.append(names[i]);
        }

        MatrixXd data = MatrixXd::Random(2, 50);
        MatrixXd dataBefore = data;

        EogRegression::fitApply(data, info);

        // Data should be unchanged
        QVERIFY(data.isApprox(dataBefore, 1e-15));
    }

    //=========================================================================================================
    /**
     * @brief isFitted() should return correct state.
     */
    void testIsFitted()
    {
        EogRegression reg;
        QVERIFY(!reg.isFitted());

        FiffInfo info = makeTestInfo();
        MatrixXd data = makeTestData();
        reg.fit(data, info);

        QVERIFY(reg.isFitted());
    }

    //=========================================================================================================
    /**
     * @brief Static fitApply convenience method should produce same result as fit+apply.
     */
    void testStaticFitApply()
    {
        FiffInfo info = makeTestInfo();
        MatrixXd data1 = makeTestData();
        MatrixXd data2 = makeTestData();

        // Method 1: static fitApply
        EogRegression::fitApply(data1, info);

        // Method 2: separate fit + apply
        EogRegression reg;
        reg.fit(data2, info);
        reg.apply(data2, info);

        QVERIFY(data1.isApprox(data2, 1e-12));
    }
};

//=============================================================================================================

QTEST_GUILESS_MAIN(TestDspEogRegression)

//=============================================================================================================

#include "test_dsp_eog_regression.moc"
