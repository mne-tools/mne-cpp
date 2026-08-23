//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_studio_raw_model.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     August, 2026
 * @brief    Tests the MNE Analyze Studio legacy raw browser model.
 */

#include <browsercore/Models/rawmodel.h>

#include <QTemporaryDir>
#include <QtTest>

using namespace MNEBROWSE;

class TestStudioRawModel : public QObject {
    Q_OBJECT

    private slots:
    void construction();
    void loadFailures();
    void loadAndInspect();
    void bufferLoad();
    void writeAndReopen();
};

QString rawFilePath() {
    return QCoreApplication::applicationDirPath() + QStringLiteral("/../resources/data/mne-cpp-test-data/MEG/sample/"
                                                                   "sample_audvis_trunc_raw.fif");
}

void TestStudioRawModel::construction() {
    RawModel model(nullptr);

    QVERIFY(!model.isFileLoaded());
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.columnCount(), 3);
    QVERIFY(model.fiffInfo());
    QVERIFY(!model.operators().isEmpty());

    model.clearModel();
    QVERIFY(!model.isFileLoaded());
    QCOMPARE(model.rowCount(), 0);
}

void TestStudioRawModel::loadFailures() {
    RawModel model(nullptr);
    QVERIFY(!model.loadFiffData(nullptr));

    QFile missingFile(QStringLiteral("missing-raw-file.fif"));
    QVERIFY(!model.loadFiffData(&missingFile));
    QVERIFY(!model.isFileLoaded());
}

void TestStudioRawModel::loadAndInspect() {
    const QString rawPath = rawFilePath();
    if (!QFile::exists(rawPath)) {
        QSKIP("Sample raw data not available");
    }

    QFile rawFile(rawPath);
    RawModel model(nullptr);
    QVERIFY(model.loadFiffData(&rawFile));
    QVERIFY(model.isFileLoaded());
    QVERIFY(model.fiffInfo());
    QCOMPARE(model.rowCount(), model.fiffInfo()->nchan);
    QCOMPARE(model.channelInfoList().size(), model.fiffInfo()->nchan);
    QVERIFY(model.sizeOfFiffData() > 0);
    QVERIFY(model.lastSample() > model.firstSample());
    QVERIFY(model.sizeOfPreloadedData() > 0);

    const QModelIndex nameIndex = model.index(0, 0);
    const QModelIndex dataIndex = model.index(0, 1);
    QVERIFY(!model.data(QModelIndex(), Qt::DisplayRole).isValid());
    QVERIFY(!model.data(nameIndex, Qt::ToolTipRole).isValid());
    QCOMPARE(model.data(nameIndex, Qt::DisplayRole).toString(),
             model.channelInfoList().constFirst().ch_name);
    QVERIFY(model.data(dataIndex, Qt::DisplayRole).isValid());
    QCOMPARE(model.headerData(1, Qt::Horizontal, Qt::DisplayRole).toString(),
             QStringLiteral("data plot"));
    QCOMPARE(model.headerData(0, Qt::Vertical, Qt::DisplayRole).toString(),
             model.channelInfoList().constFirst().ch_name);
    QVERIFY(!model.headerData(0, Qt::Horizontal, Qt::ToolTipRole).isValid());

    QModelIndexList channels{dataIndex};
    const QString channelName = model.channelInfoList().constFirst().ch_name;
    const bool initiallyBad = model.fiffInfo()->bads.contains(channelName);
    model.markChBad(channels, !initiallyBad);
    QCOMPARE(model.fiffInfo()->bads.contains(channelName), !initiallyBad);
    QVERIFY(model.data(dataIndex, Qt::BackgroundRole).isValid());
    model.markChBad(channels, initiallyBad);
    QCOMPARE(model.fiffInfo()->bads.contains(channelName), initiallyBad);

    model.updateProjections();
    model.updateCompensator(0);
    QVERIFY(model.isFileLoaded());

    model.clearModel();
    QVERIFY(!model.isFileLoaded());
    QCOMPARE(model.rowCount(), 0);
}

void TestStudioRawModel::bufferLoad() {
    QFile sourceFile(rawFilePath());
    if (!sourceFile.open(QIODevice::ReadOnly)) {
        QSKIP("Sample raw data not available");
    }

    QByteArray sourceData = sourceFile.readAll();
    QBuffer sourceBuffer(&sourceData);
    RawModel model(nullptr);
    QVERIFY(model.loadFiffData(&sourceBuffer));
    QVERIFY(model.isFileLoaded());
    QCOMPARE(model.rowCount(), model.fiffInfo()->nchan);
}

void TestStudioRawModel::writeAndReopen() {
    QFile sourceFile(rawFilePath());
    if (!sourceFile.exists()) {
        QSKIP("Sample raw data not available");
    }

    RawModel model(nullptr);
    QVERIFY(model.loadFiffData(&sourceFile));

    QTemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());
    const QString outputPath = temporaryDir.filePath(QStringLiteral("rawmodel-output.fif"));
    QFile outputFile(outputPath);
    QVERIFY(model.writeFiffData(&outputFile));
    QVERIFY(QFileInfo(outputFile).size() > 0);

    QFile writtenFile(outputPath);
    RawModel writtenModel(nullptr);
    QVERIFY(writtenModel.loadFiffData(&writtenFile));
    QCOMPARE(writtenModel.rowCount(), model.rowCount());
    QCOMPARE(writtenModel.firstSample(), model.firstSample());
    QCOMPARE(writtenModel.lastSample(), model.lastSample());
}

QTEST_GUILESS_MAIN(TestStudioRawModel)
#include "test_studio_raw_model.moc"