//=============================================================================================================
/**
 * @file     test_utils_layout_selection.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     March, 2026
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
 * @brief    Tests for LayoutLoader, LayoutMaker, and SelectionIO — file I/O
 *           for 2D/3D electrode layouts and channel selection groups.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/ioutils.h>
#include <utils/layoutloader.h>
#include <utils/layoutmaker.h>
#include <utils/selectionio.h>
#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QObject>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QDebug>
#include <QtTest>
#include <QFile>
#include <QTextStream>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * @brief Tests for LayoutLoader, LayoutMaker, and SelectionIO.
 */
class TestUtilsLayoutSelection : public QObject
{
    Q_OBJECT

public:
    TestUtilsLayoutSelection();

private slots:
    void initTestCase();

    // ── LayoutLoader ──────────────────────────────────────────────────
    void testReadAsaElcFileQString();
    void testReadAsaElcFileStdString();
    void testReadAsaElcFileNonExistent();
    void testReadAsaElcFileWrongExtension();
    void testReadMNELoutFileQString();
    void testReadMNELoutFileStdString();
    void testReadMNELoutFileNonExistent();

    // ── LayoutMaker ───────────────────────────────────────────────────
    void testMakeLayoutNoFit();
    void testMakeLayoutWithFit();
    void testMakeLayoutMirrorX();
    void testMakeLayoutMirrorY();
    // std::vector overload
    void testMakeLayoutStdVectorNoFit();
    void testMakeLayoutStdVectorWithFit();
    void testMakeLayoutStdVectorMirrorX();
    void testMakeLayoutStdVectorWriteFile();

    // ── SelectionIO ───────────────────────────────────────────────────
    void testWriteReadMNESelFileQString();
    void testWriteReadMNESelFileStdString();
    void testReadMNESelFileNonExistent();
    void testWriteReadBrainstormMonQString();
    void testWriteReadBrainstormMonStdString();

    void cleanupTestCase();

private:
    QTemporaryDir m_tempDir;
    QString resourcePath() const;
};

//=============================================================================================================

TestUtilsLayoutSelection::TestUtilsLayoutSelection()
{
}

//=============================================================================================================

void TestUtilsLayoutSelection::initTestCase()
{
    qInstallMessageHandler(UTILSLIB::MNELogger::customLogWriter);
    QVERIFY(m_tempDir.isValid());
}

//=============================================================================================================

QString TestUtilsLayoutSelection::resourcePath() const
{
    return QCoreApplication::applicationDirPath() + "/../resources/";
}

//=============================================================================================================
// LayoutLoader
//=============================================================================================================

void TestUtilsLayoutSelection::testReadAsaElcFileQString()
{
    QString elcPath = resourcePath() + "general/3DLayouts/standard_waveguard64.elc";
    if (!QFile::exists(elcPath)) {
        QSKIP("ELC file not available");
    }

    QStringList channelNames;
    QList<QVector<float>> location3D, location2D;
    QString unit;

    bool ok = LayoutLoader::readAsaElcFile(elcPath, channelNames, location3D, location2D, unit);
    QVERIFY(ok);
    QVERIFY(!channelNames.isEmpty());
    QVERIFY(!location3D.isEmpty());
    // Each 3D location has 3 coordinates
    QCOMPARE(location3D.first().size(), 3);
}

//=============================================================================================================

void TestUtilsLayoutSelection::testReadAsaElcFileStdString()
{
    QString elcPath = resourcePath() + "general/3DLayouts/standard_waveguard64.elc";
    if (!QFile::exists(elcPath)) {
        QSKIP("ELC file not available");
    }

    std::vector<std::string> channelNames;
    std::vector<std::vector<float>> location3D, location2D;
    std::string unit;

    bool ok = LayoutLoader::readAsaElcFile(elcPath.toStdString(), channelNames, location3D, location2D, unit);
    QVERIFY(ok);
    QVERIFY(!channelNames.empty());
    QVERIFY(!location3D.empty());
    QCOMPARE((int)location3D.front().size(), 3);
}

//=============================================================================================================

void TestUtilsLayoutSelection::testReadAsaElcFileNonExistent()
{
    QStringList channelNames;
    QList<QVector<float>> location3D, location2D;
    QString unit;

    bool ok = LayoutLoader::readAsaElcFile(QString("/nonexistent.elc"), channelNames, location3D, location2D, unit);
    QVERIFY(!ok);
}

//=============================================================================================================

void TestUtilsLayoutSelection::testReadAsaElcFileWrongExtension()
{
    QStringList channelNames;
    QList<QVector<float>> location3D, location2D;
    QString unit;

    bool ok = LayoutLoader::readAsaElcFile(QString("/some/file.txt"), channelNames, location3D, location2D, unit);
    QVERIFY(!ok);
}

//=============================================================================================================

void TestUtilsLayoutSelection::testReadMNELoutFileQString()
{
    QString loutPath = resourcePath() + "general/2DLayouts/Vectorview-all.lout";
    if (!QFile::exists(loutPath)) {
        QSKIP("LOUT file not available");
    }

    QMap<QString, QPointF> channelData;
    bool ok = LayoutLoader::readMNELoutFile(loutPath, channelData);
    QVERIFY(ok);
    QVERIFY(!channelData.isEmpty());
}

//=============================================================================================================

void TestUtilsLayoutSelection::testReadMNELoutFileStdString()
{
    QString loutPath = resourcePath() + "general/2DLayouts/Vectorview-all.lout";
    if (!QFile::exists(loutPath)) {
        QSKIP("LOUT file not available");
    }

    QMap<std::string, QPointF> channelData;
    bool ok = LayoutLoader::readMNELoutFile(loutPath.toStdString(), channelData);
    QVERIFY(ok);
    QVERIFY(!channelData.isEmpty());
}

//=============================================================================================================

void TestUtilsLayoutSelection::testReadMNELoutFileNonExistent()
{
    QMap<QString, QPointF> channelData;
    bool ok = LayoutLoader::readMNELoutFile(QString("/nonexistent.lout"), channelData);
    QVERIFY(!ok);
}

//=============================================================================================================
// LayoutMaker
//=============================================================================================================

void TestUtilsLayoutSelection::testMakeLayoutNoFit()
{
    QList<QVector<float>> inputPoints;
    inputPoints.append({0.1f, 0.0f, 0.0f});
    inputPoints.append({0.0f, 0.1f, 0.0f});
    inputPoints.append({-0.1f, 0.0f, 0.0f});
    inputPoints.append({0.0f, -0.1f, 0.0f});

    QList<QVector<float>> outputPoints;
    QStringList names = {"Ch1", "Ch2", "Ch3", "Ch4"};

    QString outPath = m_tempDir.path() + "/layout_nofit.lout";
    QFile outFile(outPath);

    bool ok = LayoutMaker::makeLayout(inputPoints, outputPoints, names, outFile,
                                      false, 0.2f, 0.5f, 0.5f, false);
    QVERIFY(ok);
    QCOMPARE(outputPoints.size(), 4);

    // Each output point should have at least 2 coordinates (x, y)
    for (const auto& pt : outputPoints) {
        QVERIFY(pt.size() >= 2);
    }
}

//=============================================================================================================

void TestUtilsLayoutSelection::testMakeLayoutWithFit()
{
    QList<QVector<float>> inputPoints;
    inputPoints.append({0.1f, 0.0f, 0.05f});
    inputPoints.append({0.0f, 0.1f, 0.05f});
    inputPoints.append({-0.1f, 0.0f, 0.05f});
    inputPoints.append({0.0f, -0.1f, 0.05f});
    inputPoints.append({0.05f, 0.05f, 0.07f});

    QList<QVector<float>> outputPoints;
    QStringList names = {"Ch1", "Ch2", "Ch3", "Ch4", "Ch5"};

    QString outPath = m_tempDir.path() + "/layout_fit.lout";
    QFile outFile(outPath);

    bool ok = LayoutMaker::makeLayout(inputPoints, outputPoints, names, outFile,
                                      true, 0.3f, 0.5f, 0.5f, false);
    QVERIFY(ok);
    QCOMPARE(outputPoints.size(), 5);
}

//=============================================================================================================

void TestUtilsLayoutSelection::testMakeLayoutMirrorX()
{
    QList<QVector<float>> inputPoints;
    inputPoints.append({0.1f, 0.0f, 0.0f});
    inputPoints.append({0.0f, 0.1f, 0.0f});
    inputPoints.append({-0.1f, 0.0f, 0.0f});

    QList<QVector<float>> outputNormal, outputMirrored;
    QStringList names = {"Ch1", "Ch2", "Ch3"};

    QFile out1(m_tempDir.path() + "/layout_normal.lout");
    QFile out2(m_tempDir.path() + "/layout_mirrorx.lout");

    LayoutMaker::makeLayout(inputPoints, outputNormal, names, out1,
                            false, 0.2f, 0.5f, 0.5f, false, false, false);
    LayoutMaker::makeLayout(inputPoints, outputMirrored, names, out2,
                            false, 0.2f, 0.5f, 0.5f, false, true, false);

    QCOMPARE(outputMirrored.size(), outputNormal.size());
    // X coordinates should be negated
    for (int i = 0; i < outputNormal.size(); ++i) {
        QVERIFY(qAbs(outputMirrored[i][0] + outputNormal[i][0]) < 1e-5f);
    }
}

//=============================================================================================================

void TestUtilsLayoutSelection::testMakeLayoutMirrorY()
{
    QList<QVector<float>> inputPoints;
    inputPoints.append({0.1f, 0.0f, 0.0f});
    inputPoints.append({0.0f, 0.1f, 0.0f});
    inputPoints.append({-0.1f, 0.0f, 0.0f});

    QList<QVector<float>> outputNormal, outputMirrored;
    QStringList names = {"Ch1", "Ch2", "Ch3"};

    QFile out1(m_tempDir.path() + "/layout_normal2.lout");
    QFile out2(m_tempDir.path() + "/layout_mirrory.lout");

    LayoutMaker::makeLayout(inputPoints, outputNormal, names, out1,
                            false, 0.2f, 0.5f, 0.5f, false, false, false);
    LayoutMaker::makeLayout(inputPoints, outputMirrored, names, out2,
                            false, 0.2f, 0.5f, 0.5f, false, false, true);

    QCOMPARE(outputMirrored.size(), outputNormal.size());
    // Y coordinates should be negated
    for (int i = 0; i < outputNormal.size(); ++i) {
        QVERIFY(qAbs(outputMirrored[i][1] + outputNormal[i][1]) < 1e-5f);
    }
}

//=============================================================================================================
// LayoutMaker — std::vector overload
//=============================================================================================================

void TestUtilsLayoutSelection::testMakeLayoutStdVectorNoFit()
{
    std::vector<std::vector<float>> inputPoints = {
        {0.1f, 0.0f, 0.0f},
        {0.0f, 0.1f, 0.0f},
        {-0.1f, 0.0f, 0.0f},
        {0.0f, -0.1f, 0.0f}
    };

    std::vector<std::vector<float>> outputPoints;
    std::vector<std::string> names = {"Ch1", "Ch2", "Ch3", "Ch4"};

    std::string outPath = (m_tempDir.path() + "/layout_stdvec_nofit.lout").toStdString();

    bool ok = LayoutMaker::makeLayout(inputPoints, outputPoints, names, outPath,
                                      false, 0.2f, 0.5f, 0.5f, false);
    QVERIFY(ok);
    QCOMPARE(static_cast<int>(outputPoints.size()), 4);
    for (const auto& pt : outputPoints) {
        QVERIFY(pt.size() >= 2);
    }
}

//=============================================================================================================

void TestUtilsLayoutSelection::testMakeLayoutStdVectorWithFit()
{
    std::vector<std::vector<float>> inputPoints = {
        {0.1f, 0.0f, 0.05f},
        {0.0f, 0.1f, 0.05f},
        {-0.1f, 0.0f, 0.05f},
        {0.0f, -0.1f, 0.05f},
        {0.05f, 0.05f, 0.07f}
    };

    std::vector<std::vector<float>> outputPoints;
    std::vector<std::string> names = {"Ch1", "Ch2", "Ch3", "Ch4", "Ch5"};

    std::string outPath = (m_tempDir.path() + "/layout_stdvec_fit.lout").toStdString();

    bool ok = LayoutMaker::makeLayout(inputPoints, outputPoints, names, outPath,
                                      true, 0.3f, 0.5f, 0.5f, false);
    QVERIFY(ok);
    QCOMPARE(static_cast<int>(outputPoints.size()), 5);
}

//=============================================================================================================

void TestUtilsLayoutSelection::testMakeLayoutStdVectorMirrorX()
{
    std::vector<std::vector<float>> inputPoints = {
        {0.1f, 0.0f, 0.0f},
        {0.0f, 0.1f, 0.0f},
        {-0.1f, 0.0f, 0.0f}
    };

    std::vector<std::vector<float>> outputNormal, outputMirrored;
    std::vector<std::string> names = {"Ch1", "Ch2", "Ch3"};

    std::string out1 = (m_tempDir.path() + "/layout_stdvec_normal.lout").toStdString();
    std::string out2 = (m_tempDir.path() + "/layout_stdvec_mirrorx.lout").toStdString();

    LayoutMaker::makeLayout(inputPoints, outputNormal, names, out1,
                            false, 0.2f, 0.5f, 0.5f, false, false, false);
    LayoutMaker::makeLayout(inputPoints, outputMirrored, names, out2,
                            false, 0.2f, 0.5f, 0.5f, false, true, false);

    QCOMPARE(outputMirrored.size(), outputNormal.size());
    for (size_t i = 0; i < outputNormal.size(); ++i) {
        QVERIFY(std::abs(outputMirrored[i][0] + outputNormal[i][0]) < 1e-5f);
    }
}

//=============================================================================================================

void TestUtilsLayoutSelection::testMakeLayoutStdVectorWriteFile()
{
    std::vector<std::vector<float>> inputPoints = {
        {0.1f, 0.0f, 0.0f},
        {0.0f, 0.1f, 0.0f},
        {-0.1f, 0.0f, 0.0f}
    };

    std::vector<std::vector<float>> outputPoints;
    std::vector<std::string> names = {"Ch1", "Ch2", "Ch3"};

    std::string outPath = (m_tempDir.path() + "/layout_stdvec_written.lout").toStdString();

    bool ok = LayoutMaker::makeLayout(inputPoints, outputPoints, names, outPath,
                                      false, 0.2f, 0.5f, 0.5f, true);
    QVERIFY(ok);
    QCOMPARE(static_cast<int>(outputPoints.size()), 3);

    // Verify file was written
    QFile written(QString::fromStdString(outPath));
    QVERIFY(written.exists());
    QVERIFY(written.size() > 0);
}

//=============================================================================================================
// SelectionIO
//=============================================================================================================

void TestUtilsLayoutSelection::testWriteReadMNESelFileQString()
{
    QString path = m_tempDir.path() + "/test_write.sel";

    QMultiMap<QString, QStringList> selMap;
    selMap.insert("GroupA", QStringList({"MEG 0111", "MEG 0121", "MEG 0131"}));
    selMap.insert("GroupB", QStringList({"MEG 0211", "MEG 0221"}));

    QVERIFY(SelectionIO::writeMNESelFile(path, selMap));

    QMultiMap<QString, QStringList> readMap;
    QVERIFY(SelectionIO::readMNESelFile(path, readMap));
    QVERIFY(!readMap.isEmpty());
    QVERIFY(readMap.contains("GroupA"));
    QVERIFY(readMap.contains("GroupB"));
}

//=============================================================================================================

void TestUtilsLayoutSelection::testWriteReadMNESelFileStdString()
{
    std::string path = m_tempDir.path().toStdString() + "/test_write_std.sel";

    std::map<std::string, std::vector<std::string>> selMap;
    selMap["GroupA"] = {"MEG 0111", "MEG 0121"};
    selMap["GroupB"] = {"MEG 0211"};

    QVERIFY(SelectionIO::writeMNESelFile(path, selMap));

    std::multimap<std::string, std::vector<std::string>> readMap;
    QVERIFY(SelectionIO::readMNESelFile(path, readMap));
    QVERIFY(!readMap.empty());
    QVERIFY(readMap.count("GroupA") > 0);
}

//=============================================================================================================

void TestUtilsLayoutSelection::testReadMNESelFileNonExistent()
{
    QMultiMap<QString, QStringList> readMap;
    bool ok = SelectionIO::readMNESelFile(QString("/nonexistent.sel"), readMap);
    QVERIFY(!ok);
}

//=============================================================================================================

void TestUtilsLayoutSelection::testWriteReadBrainstormMonQString()
{
    // Write a group to its own .mon file, then read it back
    QString basePath = m_tempDir.path() + "/brainstorm";

    QMultiMap<QString, QStringList> selMap;
    selMap.insert("TestGroup", QStringList({"Chan1", "Chan2", "Chan3"}));

    QVERIFY(SelectionIO::writeBrainstormMonFiles(basePath, selMap));

    // writeBrainstormMonFiles uses QFileInfo(path).absolutePath() + "/" + key + ".mon"
    QFileInfo fi(basePath);
    QString monPath = fi.absolutePath() + "/TestGroup.mon";
    QVERIFY2(QFile::exists(monPath), qPrintable("Mon file not found at: " + monPath));

    QMultiMap<QString, QStringList> readMap;
    QVERIFY(SelectionIO::readBrainstormMonFile(monPath, readMap));
    QVERIFY(!readMap.isEmpty());
}

//=============================================================================================================

void TestUtilsLayoutSelection::testWriteReadBrainstormMonStdString()
{
    std::string basePath = m_tempDir.path().toStdString() + "/brainstorm_std";

    std::map<std::string, std::vector<std::string>> selMap;
    selMap["StdTestGroup"] = {"Chan1", "Chan2"};

    QVERIFY(SelectionIO::writeBrainstormMonFiles(basePath, selMap));

    // Read back using std::string overload
    std::string monPath = basePath.substr(0, basePath.find_last_of("/") + 1) + "StdTestGroup.mon";
    std::multimap<std::string, std::vector<std::string>> readMap;
    QVERIFY(SelectionIO::readBrainstormMonFile(monPath, readMap));
    QVERIFY(!readMap.empty());
}


//=============================================================================================================

void TestUtilsLayoutSelection::cleanupTestCase()
{
    qInfo() << "TestUtilsLayoutSelection: All tests completed";
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestUtilsLayoutSelection)
#include "test_utils_layout_selection.moc"
