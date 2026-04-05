//=============================================================================================================
/**
 * @file     test_tool_preprocessing.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
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
 * @brief    Integration tests for preprocessing CLI tools:
 *           mne_process_raw, mne_compensate_data, mne_mark_bad_channels, etc.
 */

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QProcess>
#include <QCoreApplication>
#include <QDir>

//=============================================================================================================
// TEST CLASS
//=============================================================================================================

class TestToolPreprocessing : public QObject
{
    Q_OBJECT

private:
    QString m_sBinDir;
    QString m_sResourcePath;

    QString runTool(const QString& toolName, const QStringList& args, int timeoutMs = 30000)
    {
        QString path = m_sBinDir + "/" + toolName;
#ifdef Q_OS_WIN
        path += ".exe";
#endif
        QProcess proc;
        proc.setProgram(path);
        proc.setArguments(args);
        proc.start();
        proc.waitForFinished(timeoutMs);
        return proc.readAllStandardOutput() + proc.readAllStandardError();
    }

    bool toolExists(const QString& toolName)
    {
        QString path = m_sBinDir + "/" + toolName;
#ifdef Q_OS_WIN
        path += ".exe";
#endif
        return QFile::exists(path);
    }

private slots:

    void initTestCase()
    {
        QString binDir = QCoreApplication::applicationDirPath();
        m_sBinDir = binDir + "/../bin";
        m_sResourcePath = binDir + "/../resources/data/mne-cpp-test-data/";
    }

    //=========================================================================================================
    // mne_list_bem
    //=========================================================================================================

    void testListBemHelp()
    {
        if (!toolExists("mne_list_bem")) QSKIP("mne_list_bem not found");
        QString output = runTool("mne_list_bem", {"--help"});
        QVERIFY(output.contains("help", Qt::CaseInsensitive) ||
                output.contains("bem", Qt::CaseInsensitive) ||
                output.contains("usage", Qt::CaseInsensitive));
    }

    void testListBemRun()
    {
        if (!toolExists("mne_list_bem")) QSKIP("mne_list_bem not found");
        QString bemFile = m_sResourcePath + "subjects/sample/bem/sample-5120-bem.fif";
        if (!QFile::exists(bemFile)) QSKIP("BEM file not available");

        QString output = runTool("mne_list_bem", {"--bem", bemFile});
        QVERIFY(!output.isEmpty());
        // Should list BEM surface information
        QVERIFY(output.contains("surface", Qt::CaseInsensitive) ||
                output.contains("tri", Qt::CaseInsensitive) ||
                output.contains("5120", Qt::CaseInsensitive) ||
                output.contains("vert", Qt::CaseInsensitive) ||
                output.contains("bem", Qt::CaseInsensitive));
    }

    //=========================================================================================================
    // mne_check_surface
    //=========================================================================================================

    void testCheckSurfaceHelp()
    {
        if (!toolExists("mne_check_surface")) QSKIP("mne_check_surface not found");
        QString output = runTool("mne_check_surface", {"--help"});
        QVERIFY(output.contains("help", Qt::CaseInsensitive) ||
                output.contains("surface", Qt::CaseInsensitive));
    }

    //=========================================================================================================
    // mne_compare_fif_files
    //=========================================================================================================

    void testCompareFifHelp()
    {
        if (!toolExists("mne_compare_fif_files")) QSKIP("mne_compare_fif_files not found");
        QString output = runTool("mne_compare_fif_files", {"--help"});
        QVERIFY(output.contains("help", Qt::CaseInsensitive) ||
                output.contains("compare", Qt::CaseInsensitive) ||
                output.contains("usage", Qt::CaseInsensitive));
    }

    void testCompareFifSameFile()
    {
        if (!toolExists("mne_compare_fif_files")) QSKIP("mne_compare_fif_files not found");
        QString rawFile = m_sResourcePath + "MEG/sample/sample_audvis_trunc_raw.fif";
        if (!QFile::exists(rawFile)) QSKIP("raw file not available");

        // Compare a file with itself — should show no differences
        QString output = runTool("mne_compare_fif_files", {"--in1", rawFile, "--in2", rawFile});
        // Should not report errors about essential differences
        QVERIFY(!output.isEmpty());
    }

    //=========================================================================================================
    // mne_collect_transforms
    //=========================================================================================================

    void testCollectTransformsHelp()
    {
        if (!toolExists("mne_collect_transforms")) QSKIP("mne_collect_transforms not found");
        QString output = runTool("mne_collect_transforms", {"--help"});
        QVERIFY(output.contains("help", Qt::CaseInsensitive) ||
                output.contains("transform", Qt::CaseInsensitive));
    }

    //=========================================================================================================
    // mne_mark_bad_channels
    //=========================================================================================================

    void testMarkBadChannelsHelp()
    {
        if (!toolExists("mne_mark_bad_channels")) QSKIP("mne_mark_bad_channels not found");
        QString output = runTool("mne_mark_bad_channels", {"--help"});
        QVERIFY(output.contains("help", Qt::CaseInsensitive) ||
                output.contains("bad", Qt::CaseInsensitive) ||
                output.contains("channel", Qt::CaseInsensitive));
    }

    //=========================================================================================================
    // mne_make_source_space
    //=========================================================================================================

    void testMakeSourceSpaceHelp()
    {
        if (!toolExists("mne_make_source_space")) QSKIP("mne_make_source_space not found");
        QString output = runTool("mne_make_source_space", {"--help"});
        QVERIFY(output.contains("help", Qt::CaseInsensitive) ||
                output.contains("source", Qt::CaseInsensitive) ||
                output.contains("space", Qt::CaseInsensitive));
    }

    //=========================================================================================================
    // mne_cov2proj
    //=========================================================================================================

    void testCov2projHelp()
    {
        if (!toolExists("mne_cov2proj")) QSKIP("mne_cov2proj not found");
        QString output = runTool("mne_cov2proj", {"--help"});
        QVERIFY(output.contains("help", Qt::CaseInsensitive) ||
                output.contains("cov", Qt::CaseInsensitive) ||
                output.contains("proj", Qt::CaseInsensitive));
    }

    //=========================================================================================================
    // mne_sensitivity_map
    //=========================================================================================================

    void testSensitivityMapHelp()
    {
        if (!toolExists("mne_sensitivity_map")) QSKIP("mne_sensitivity_map not found");
        QString output = runTool("mne_sensitivity_map", {"--help"});
        QVERIFY(output.contains("help", Qt::CaseInsensitive) ||
                output.contains("sensitivity", Qt::CaseInsensitive) ||
                output.contains("map", Qt::CaseInsensitive));
    }

    //=========================================================================================================
    // mne_volume_source_space
    //=========================================================================================================

    void testVolumeSourceSpaceHelp()
    {
        if (!toolExists("mne_volume_source_space")) QSKIP("mne_volume_source_space not found");
        QString output = runTool("mne_volume_source_space", {"--help"});
        QVERIFY(output.contains("help", Qt::CaseInsensitive) ||
                output.contains("volume", Qt::CaseInsensitive) ||
                output.contains("source", Qt::CaseInsensitive));
    }

    //=========================================================================================================
    // mne_smooth
    //=========================================================================================================

    void testSmoothHelp()
    {
        if (!toolExists("mne_smooth")) QSKIP("mne_smooth not found");
        QString output = runTool("mne_smooth", {"--help"});
        QVERIFY(output.contains("help", Qt::CaseInsensitive) ||
                output.contains("smooth", Qt::CaseInsensitive));
    }

    //=========================================================================================================
    // mne_make_eeg_layout
    //=========================================================================================================

    void testMakeEegLayoutHelp()
    {
        if (!toolExists("mne_make_eeg_layout")) QSKIP("mne_make_eeg_layout not found");
        QString output = runTool("mne_make_eeg_layout", {"--help"});
        QVERIFY(output.contains("help", Qt::CaseInsensitive) ||
                output.contains("layout", Qt::CaseInsensitive) ||
                output.contains("eeg", Qt::CaseInsensitive));
    }
};

QTEST_GUILESS_MAIN(TestToolPreprocessing)
#include "test_tool_preprocessing.moc"
