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
#include <rawdatabrowserwidget.h>

#include <fiff/fiff_events.h>

#include <QPushButton>
#include <QTableView>
#include <QTemporaryDir>
#include <QtTest>

using namespace MNEBROWSE;

class TestStudioRawModel : public QObject
{
    Q_OBJECT

private slots:
    void construction();
    void loadFailures();
    void loadAndInspect();
    void bufferLoad();
    void writeAndReopen();
    void browserWidgetNavigatesTheRecording();
};

QString rawFilePath()
{
    return QCoreApplication::applicationDirPath() + QStringLiteral("/../resources/data/mne-cpp-test-data/MEG/sample/"
                                                                   "sample_audvis_trunc_raw.fif");
}

void TestStudioRawModel::construction()
{
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

void TestStudioRawModel::loadFailures()
{
    RawModel model(nullptr);
    QVERIFY(!model.loadFiffData(nullptr));

    QFile missingFile(QStringLiteral("missing-raw-file.fif"));
    QVERIFY(!model.loadFiffData(&missingFile));
    QVERIFY(!model.isFileLoaded());
}

void TestStudioRawModel::loadAndInspect()
{
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

void TestStudioRawModel::bufferLoad()
{
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

void TestStudioRawModel::writeAndReopen()
{
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
    QFile unwritable(temporaryDir.filePath(QStringLiteral("no/such/dir/out.fif")));
    QVERIFY(!model.writeFiffData(&unwritable));

    QFile writtenFile(outputPath);
    RawModel writtenModel(nullptr);
    QVERIFY(writtenModel.loadFiffData(&writtenFile));
    QCOMPARE(writtenModel.rowCount(), model.rowCount());
    QCOMPARE(writtenModel.firstSample(), model.firstSample());
    QCOMPARE(writtenModel.lastSample(), model.lastSample());
}

void TestStudioRawModel::browserWidgetNavigatesTheRecording()
{
    if (!QFile::exists(rawFilePath())) {
        QSKIP("Sample raw data not available");
    }

    // A companion <name>-eve.fif next to the recording is picked up with it
    QTemporaryDir dir;
    const QString rawCopy = dir.filePath("run_raw.fif");
    QVERIFY(QFile::copy(rawFilePath(), rawCopy));
    FIFFLIB::FiffEvents events;
    events.events.resize(2, 3);
    events.events << 15000, 0, 1, 16000, 0, 2;
    QFile eveFile(dir.filePath("run_raw-eve.fif"));
    QVERIFY(events.write_to_fif(eveFile));
    eveFile.close();

    MNEANALYZESTUDIO::RawDataBrowserWidget widget;
    widget.resize(900, 600);
    widget.show();
    QSignalSpy output(&widget, &MNEANALYZESTUDIO::RawDataBrowserWidget::outputMessage);
    QSignalSpy status(&widget, &MNEANALYZESTUDIO::RawDataBrowserWidget::statusMessage);

    QCOMPARE(widget.summaryText(), QStringLiteral("No raw browser loaded."));
    QCOMPARE(widget.stateText().left(25), QStringLiteral("Visible range unavailable"));
    QVERIFY(!widget.gotoSample(100));
    QVERIFY(!widget.loadFile(dir.filePath("missing.fif")));

    QVERIFY(widget.loadFile(rawCopy));
    QCOMPARE(widget.filePath(), rawCopy);
    QVERIFY(widget.summaryText().contains("Channels: 376"));
    QVERIFY(std::any_of(output.cbegin(), output.cend(), [](const QList<QVariant>& args) {
        return args.first().toString().contains("(2 events)");
    }));

    // Going to a sample moves the cursor there; out-of-range samples are clamped
    QVERIFY(widget.gotoSample(16000));
    QCOMPARE(widget.cursorSample(), 16000);
    QVERIFY(status.last().first().toString().contains("Cursor: 16000"));
    QVERIFY(widget.gotoSample(1));
    QCOMPARE(widget.cursorSample(), 12900);

    QVERIFY(!widget.setZoomPixelsPerSample(0.0));
    QVERIFY(widget.setZoomPixelsPerSample(2.0));
    QCOMPARE(widget.pixelsPerSample(), 2.0);
    auto button = [&widget](const QString& text) {
        for (QPushButton* candidate : widget.findChildren<QPushButton*>()) {
            if (candidate->text() == text)
                return candidate;
        }
        return static_cast<QPushButton*>(nullptr);
    };
    button("+")->click();
    QCOMPARE(widget.pixelsPerSample(), 2.5);
    button("-")->click();
    QCOMPARE(widget.pixelsPerSample(), 2.0);
    button("1:1")->click();
    QCOMPARE(widget.pixelsPerSample(), 1.0);
    QVERIFY(widget.stateText().endsWith("Zoom: 1.00 px/sample"));

    // Keyboard: d toggles DC removal, +/- scale the amplitude
    auto* table = widget.findChild<QTableView*>();
    QVERIFY(table);
    QTest::keyClick(table, Qt::Key_D);
    QVERIFY(button("DC")->isChecked());
    QTest::keyClick(table, Qt::Key_Plus);
    QTest::keyClick(table, Qt::Key_Minus);

    // Clicking a channel label toggles it bad; double-clicking the data moves the cursor
    const QString firstChannel = table->model()->index(0, 0).data().toString();
    const QPoint label(table->columnViewportPosition(0) + 5, table->rowViewportPosition(0) + 5);
    QTest::mouseClick(table->viewport(), Qt::LeftButton, {}, label);
    QVERIFY(table->model()->index(0, 1).data(Qt::BackgroundRole).isValid());
    QTest::mouseClick(table->viewport(), Qt::LeftButton, {}, label);
    QCOMPARE(table->model()->index(0, 0).data().toString(), firstChannel);
    widget.gotoSample(15000);
    const int dataX = table->columnViewportPosition(1);
    QTest::mouseDClick(table->viewport(), Qt::LeftButton, {}, QPoint(dataX + 100, table->rowViewportPosition(0) + 5));
    QVERIFY(widget.cursorSample() > 15000);
}

QTEST_MAIN(TestStudioRawModel)
#include "test_studio_raw_model.moc"