//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2019-2026 MNE-CPP Authors
 *
 * @file     test_edf2fiff_rwr.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Simon Heinke <simon.heinke@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @date     August, 2019
 * @brief    Runs mne_edf2fiff and compares the FIFF it writes with mne.io.read_raw_edf and BIDSLIB::EDFReader.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <bids/readers/bids_edf_reader.h>

#include <fiff/fiff_constants.h>
#include <fiff/fiff_raw_data.h>

#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QProcess>
#include <QTemporaryDir>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BIDSLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * @brief Converts test_reduced.edf with mne_edf2fiff and checks the FIFF against MNE-Python and EDFReader.
 */
class TestEDF2FIFFRWR : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void convertMatchesPython();
    void refusesBadInput();

private:
    int run(const QStringList& arguments);

    QString m_sTool;
    QString m_sEdf;
    QTemporaryDir m_tempDir;
};

//=============================================================================================================

void TestEDF2FIFFRWR::initTestCase()
{
    qInstallMessageHandler(UTILSLIB::MNELogger::customLogWriter);
    const QString appDir = QCoreApplication::applicationDirPath();
    m_sEdf = appDir + "/../resources/data/mne-cpp-test-data/EEG/test_reduced.edf";
    QVERIFY(QFileInfo::exists(m_sEdf));
    QVERIFY(m_tempDir.isValid());
#ifdef Q_OS_WIN
    const QString exe = QStringLiteral("mne_edf2fiff.exe");
#else
    const QString exe = QStringLiteral("mne_edf2fiff");
#endif
    for (const QString& dir : {appDir, appDir + "/../bin", appDir + "/../apps"}) {
        const QFileInfo fi(dir + "/" + exe);
        if (fi.exists() && fi.isExecutable()) {
            m_sTool = fi.canonicalFilePath();
            break;
        }
    }
    QVERIFY2(!m_sTool.isEmpty(), "mne_edf2fiff executable not found");
}

//=============================================================================================================

int TestEDF2FIFFRWR::run(const QStringList& arguments)
{
    QProcess proc;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    // The Windows loader needs the Qt and mne-cpp DLL directories on PATH
    env.insert("PATH", QFileInfo(m_sTool).absolutePath() + ";" + QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../../../src/external/qt/dynamic/bin") + ";" + env.value("PATH"));
    proc.setProcessEnvironment(env);
    proc.setProcessChannelMode(QProcess::ForwardedChannels);
    proc.start(m_sTool, arguments);
    if (!proc.waitForFinished(60000) || proc.exitStatus() != QProcess::NormalExit) {
        return -1;
    }
    return proc.exitCode();
}

//=============================================================================================================

void TestEDF2FIFFRWR::convertMatchesPython()
{
    const QString fif = m_tempDir.filePath(QStringLiteral("test_reduced_raw.fif"));
    QCOMPARE(run({"--fileIn", m_sEdf, "--fileOut", fif}), 0);

    QFile file(fif);
    FiffRawData raw(file);
    QCOMPARE(raw.info.sfreq, 512.0f);
    QCOMPARE(raw.first_samp, 0);
    QCOMPARE(raw.last_samp, 3071);
    QCOMPARE(raw.info.nchan, 126); // the 512 Hz channels of mne's 139
    MatrixXd data, times;
    QVERIFY(raw.read_raw_segment(data, times, raw.first_samp, raw.last_samp));

    // mne.io.read_raw_edf(test_reduced.edf).get_data(): samples 0, 1000, 3071 and sum |x|
    const QList<std::tuple<QString, int, std::array<double, 4>>> expected{
        {QStringLiteral("A10"), FIFFV_EEG_CH, {-1.2e-05, 2.6e-05, -3.6e-05, 0.047375}},
        {QStringLiteral("H16"), FIFFV_EEG_CH, {-5e-06, 2.2e-05, -2.3e-05, 0.039064}},
        {QStringLiteral("Ergo-Left"), FIFFV_EEG_CH, {1.6e-05, 1.7e-05, 1.7e-05, 0.052226}},
        {QStringLiteral("Status"), FIFFV_STIM_CH, {4352.0, 0.0, 0.0, 57624.0}},
    };
    for (const auto& [name, kind, values] : expected) {
        const int k = raw.info.ch_names.indexOf(name);
        QVERIFY2(k >= 0, qPrintable(name));
        QCOMPARE(raw.info.chs[k].kind, kind);
        const double tol = 1e-6 * std::abs(values[3]);
        QVERIFY2(std::abs(data(k, 0) - values[0]) < tol, qPrintable(name));
        QVERIFY2(std::abs(data(k, 1000) - values[1]) < tol, qPrintable(name));
        QVERIFY2(std::abs(data(k, 3071) - values[2]) < tol, qPrintable(name));
        QVERIFY2(std::abs(data.row(k).cwiseAbs().sum() - values[3]) < 1e-5 * values[3], qPrintable(name));
    }

    // Every channel equals what EDFReader reads (float samples, float FIFF buffers)
    EDFReader reader;
    QVERIFY(reader.open(m_sEdf));
    QVERIFY(data.isApprox(reader.readRawSegment(0, 3072).cast<double>()));

    // Without --fileOut the output goes next to the input
    const QString copy = m_tempDir.filePath(QStringLiteral("copy.EDF"));
    QVERIFY(QFile::copy(m_sEdf, copy));
    QCOMPARE(run({"--fileIn", copy}), 0);
    QVERIFY(QFileInfo::exists(m_tempDir.filePath(QStringLiteral("copy.fif"))));
}

//=============================================================================================================

void TestEDF2FIFFRWR::refusesBadInput()
{
    QCOMPARE(run({}), 1);
    QCOMPARE(run({"--fileIn", m_tempDir.filePath(QStringLiteral("missing.edf"))}), 1);
    const QString text = m_tempDir.filePath(QStringLiteral("notes.txt"));
    QFile notes(text);
    QVERIFY(notes.open(QIODevice::WriteOnly));
    notes.close();
    QCOMPARE(run({"--fileIn", text}), 1);
    QCOMPARE(run({"--fileIn", m_sEdf, "--fileOut", m_tempDir.filePath(QStringLiteral("no/such/dir/out.fif"))}), 1);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestEDF2FIFFRWR)
#include "test_edf2fiff_rwr.moc"
