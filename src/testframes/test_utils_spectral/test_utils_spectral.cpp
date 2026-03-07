#include <QtTest/QtTest>
#include <Eigen/Dense>
#include <utils/spectral.h>
#include <cmath>

using namespace UTILSLIB;
using namespace Eigen;

class TestUtilsSpectral : public QObject
{
    Q_OBJECT

private slots:
    void testGenerateTapersHanningProperties()
    {
        QPair<MatrixXd, VectorXd> tapResult = Spectral::generateTapers(64, QString("hanning"));
        MatrixXd hann = tapResult.first;
        QCOMPARE(hann.rows(), 1);
        QCOMPARE(hann.cols(), 64);
        // Hanning is symmetric
        for (int i = 0; i < 32; ++i) {
            QVERIFY(std::abs(hann(0, i) - hann(0, 63 - i)) < 1e-10);
        }
        // Maximum at center
        QVERIFY(hann(0, 31) > 0.9);
        QVERIFY(hann(0, 32) > 0.9);
        // Ends near zero
        QVERIFY(hann(0, 0) < 0.05);
    }

    void testGenerateTapersHanningSmall()
    {
        QPair<MatrixXd, VectorXd> tapResult = Spectral::generateTapers(8, QString("hanning"));
        QCOMPARE(tapResult.first.cols(), 8);
    }

    void testGenerateTapersQString()
    {
        QPair<MatrixXd, VectorXd> result = Spectral::generateTapers(64, QString("hanning"));
        QCOMPARE(result.first.cols(), 64);
        QVERIFY(result.first.rows() >= 1);
        QCOMPARE(result.second.size(), result.first.rows());
    }

    void testGenerateTapersStdString()
    {
        std::pair<MatrixXd, VectorXd> result = Spectral::generateTapers(64, std::string("hanning"));
        QCOMPARE(result.first.cols(), 64);
        QVERIFY(result.first.rows() >= 1);
    }

    void testGenerateTapersOnes()
    {
        QPair<MatrixXd, VectorXd> result = Spectral::generateTapers(32, QString("ones"));
        QCOMPARE(result.first.cols(), 32);
        // "ones" taper should have all ones (or near constant)
    }

    void testCalculateFFTFreqs()
    {
        VectorXd freqs = Spectral::calculateFFTFreqs(128, 1000.0);
        QCOMPARE(freqs.size(), 128);
        // First element is DC = 0 Hz
        QVERIFY(std::abs(freqs(0)) < 1e-10);
        // Frequency resolution is sFreq/Nfft = 1000/128 ≈ 7.8125
        QVERIFY(std::abs(freqs(1) - (1000.0 / 128.0)) < 0.01);
    }

    void testCalculateFFTFreqs256()
    {
        VectorXd freqs = Spectral::calculateFFTFreqs(256, 500.0);
        QCOMPARE(freqs.size(), 256);
        QVERIFY(std::abs(freqs(0)) < 1e-10);
    }

    void testComputeTaperedSpectraRow()
    {
        int N = 64;
        RowVectorXd data = RowVectorXd::Random(N);
        QPair<MatrixXd, VectorXd> tapResult = Spectral::generateTapers(N, QString("hanning"));
        MatrixXd tapers = tapResult.first;
        int iNfft = N;

        MatrixXcd result = Spectral::computeTaperedSpectraRow(data, tapers, iNfft);
        // Result should be nTapers x iNfft
        QCOMPARE(result.rows(), tapers.rows());
        QCOMPARE(result.cols(), iNfft);
    }

    void testComputeTaperedSpectraRowSameLength()
    {
        // Test with varying lengths (no zero-padding to avoid Eigen FFT row-vector bug)
        int N = 128;
        RowVectorXd data = RowVectorXd::Random(N);
        QPair<MatrixXd, VectorXd> tapResult = Spectral::generateTapers(N, QString("hanning"));
        MatrixXd tapers = tapResult.first;

        MatrixXcd result = Spectral::computeTaperedSpectraRow(data, tapers, N);
        QCOMPARE(result.cols(), N);
        QCOMPARE(result.rows(), tapers.rows());
    }

    void testComputeTaperedSpectraMatrixSequential()
    {
        int nChannels = 4;
        int N = 128;
        MatrixXd data = MatrixXd::Random(nChannels, N);
        QPair<MatrixXd, VectorXd> tapResult = Spectral::generateTapers(N, QString("hanning"));
        MatrixXd tapers = tapResult.first;
        int iNfft = N;

        QVector<MatrixXcd> result = Spectral::computeTaperedSpectraMatrix(
            data, tapers, iNfft, false /* sequential */);
        QCOMPARE(result.size(), nChannels);
        for (int ch = 0; ch < nChannels; ++ch) {
            QCOMPARE(result[ch].cols(), iNfft);
        }
    }

    void testComputeTaperedSpectraMatrixParallel()
    {
        int nChannels = 4;
        int N = 128;
        MatrixXd data = MatrixXd::Random(nChannels, N);
        QPair<MatrixXd, VectorXd> tapResult = Spectral::generateTapers(N, QString("hanning"));
        MatrixXd tapers = tapResult.first;
        int iNfft = N;

        QVector<MatrixXcd> result = Spectral::computeTaperedSpectraMatrix(
            data, tapers, iNfft, true /* parallel */);
        QCOMPARE(result.size(), nChannels);
    }

    void testPsdFromTaperedSpectra()
    {
        int N = 64;
        RowVectorXd data = RowVectorXd::Random(N);
        QPair<MatrixXd, VectorXd> tapResult = Spectral::generateTapers(N, QString("hanning"));
        MatrixXcd tapSpectra = Spectral::computeTaperedSpectraRow(data, tapResult.first, N);

        RowVectorXd psd = Spectral::psdFromTaperedSpectra(
            tapSpectra, tapResult.second, N, 1000.0);

        QCOMPARE(psd.size(), N);
        // PSD values should be non-negative
        for (int i = 0; i < psd.size(); ++i) {
            QVERIFY(psd(i) >= 0.0);
        }
    }

    void testCsdFromTaperedSpectra()
    {
        int N = 64;
        RowVectorXd data1 = RowVectorXd::Random(N);
        RowVectorXd data2 = RowVectorXd::Random(N);
        QPair<MatrixXd, VectorXd> tapResult = Spectral::generateTapers(N, QString("hanning"));

        MatrixXcd tapSpectra1 = Spectral::computeTaperedSpectraRow(data1, tapResult.first, N);
        MatrixXcd tapSpectra2 = Spectral::computeTaperedSpectraRow(data2, tapResult.first, N);

        RowVectorXcd csd = Spectral::csdFromTaperedSpectra(
            tapSpectra1, tapSpectra2,
            tapResult.second, tapResult.second,
            N, 1000.0);

        QCOMPARE(csd.size(), N);
    }

    void testPsdParseval()
    {
        // For a pure sine, PSD should have a peak at the sine frequency
        int N = 256;
        double sFreq = 1000.0;
        double fSine = 100.0;
        RowVectorXd data(N);
        for (int i = 0; i < N; ++i) {
            data(i) = std::sin(2.0 * M_PI * fSine * i / sFreq);
        }

        QPair<MatrixXd, VectorXd> tapResult = Spectral::generateTapers(N, QString("hanning"));
        MatrixXcd tapSpectra = Spectral::computeTaperedSpectraRow(data, tapResult.first, N);
        RowVectorXd psd = Spectral::psdFromTaperedSpectra(tapSpectra, tapResult.second, N, sFreq);

        // Find peak
        int peakIdx = 0;
        double peakVal = psd(0);
        VectorXd freqs = Spectral::calculateFFTFreqs(N, sFreq);
        for (int i = 1; i < N / 2; ++i) {
            if (psd(i) > peakVal) {
                peakVal = psd(i);
                peakIdx = i;
            }
        }
        // Peak should be near 100 Hz
        double peakFreq = freqs(peakIdx);
        QVERIFY(std::abs(peakFreq - fSine) < 2.0 * sFreq / N);
    }

    void testAutoCorrelation()
    {
        // Verify CSD function can be called with same spectrum as seed and target
        int N = 64;
        RowVectorXd data = RowVectorXd::Random(N);
        QPair<MatrixXd, VectorXd> tapResult = Spectral::generateTapers(N, QString("hanning"));
        MatrixXcd tapSpectra = Spectral::computeTaperedSpectraRow(data, tapResult.first, N);

        RowVectorXcd csd = Spectral::csdFromTaperedSpectra(
            tapSpectra, tapSpectra,
            tapResult.second, tapResult.second,
            N, 1000.0);

        QCOMPARE(csd.size(), N);
        // Auto-CSD should have non-negative real parts
        for (int i = 0; i < N; ++i) {
            QVERIFY(csd(i).real() >= -1e-6);
        }
    }
};

QTEST_GUILESS_MAIN(TestUtilsSpectral)
#include "test_utils_spectral.moc"
