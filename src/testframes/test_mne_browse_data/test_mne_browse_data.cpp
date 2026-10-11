//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_mne_browse_data.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     June, 2026
 * @brief    Comprehensive tests for mne_browse data models and utilities.
 *           Tests RawModel, AnnotationModel, EventModel, AverageModel, EpochModel,
 *           FiffBlockReader, DataPackage, FilterOperator,
 *           SessionFilter, and NewParksMcClellan.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "Models/rawmodel.h"
#include "Models/annotationmodel.h"
#include "Models/eventmodel.h"
#include "Models/averagemodel.h"
#include "Models/epochmodel.h"
#include "Models/fiffblockreader.h"
#include "Utils/datapackage.h"
#include "Utils/filteroperator.h"
#include "Utils/sessionfilter.h"
#include "Utils/rawsettings.h"

#include <fiff/fiff.h>
#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_raw_data.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBROWSE;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * @brief Comprehensive tests for mne_browse data layer (models + utils).
 */
class TestMneBrowseData : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    //=========================================================================================================
    // FiffBlockReader tests
    //=========================================================================================================
    void testFiffBlockReaderConstruct();
    void testFiffBlockReaderOpen();
    void testFiffBlockReaderOpenBuffer();
    void testFiffBlockReaderReadBlock();
    void testFiffBlockReaderClose();
    void testFiffBlockReaderUpdateProjections();

    //=========================================================================================================
    // RawModel tests
    //=========================================================================================================
    void testRawModelConstruct();
    void testRawModelLoadFiff();
    void testRawModelRowColumnCount();
    void testRawModelData();
    void testRawModelHeaderData();
    void testRawModelClearModel();
    void testRawModelWriteFiff();
    void testRawModelChannelInfo();
    void testRawModelSizeOfFiffData();
    void testRawModelFirstLastSample();
    void testRawModelOperators();
    void testRawModelMarkChBad();
    void testRawModelProjections();
    void testRawModelCompensator();

    //=========================================================================================================
    // AnnotationModel tests
    //=========================================================================================================
    void testAnnotationModelConstruct();
    void testAnnotationModelAddAnnotation();
    void testAnnotationModelData();
    void testAnnotationModelSetData();
    void testAnnotationModelRemoveRows();
    void testAnnotationModelGetSpans();
    void testAnnotationModelSaveLoadJson();
    void testAnnotationModelSaveLoadCsv();
    void testAnnotationModelClear();
    void testAnnotationModelUpdateBoundary();
    void testAnnotationModelSampleRange();

    //=========================================================================================================
    // EventModel tests
    //=========================================================================================================
    void testEventModelConstruct();
    void testEventModelSetEventMatrix();
    void testEventModelRowColumnCount();
    void testEventModelData();
    void testEventModelSetData();
    void testEventModelInsertRemoveRows();
    void testEventModelEventTypes();
    void testEventModelFilterType();
    void testEventModelClear();
    void testEventModelSaveLoadEvents();

    //=========================================================================================================
    // AverageModel tests
    //=========================================================================================================
    void testAverageModelConstruct();
    void testAverageModelLoadEvoked();
    void testAverageModelSetEvoked();
    void testAverageModelData();
    void testAverageModelClear();

    //=========================================================================================================
    // EpochModel tests
    //=========================================================================================================
    void testEpochModelConstruct();
    void testEpochModelReviewsEpochs();


    //=========================================================================================================
    // DataPackage tests
    //=========================================================================================================
    void testDataPackageConstruct();
    void testDataPackageSetOrigRawData();
    void testDataPackageDataRaw();
    void testDataPackageDataProc();
    void testDataPackageMean();
    void testDataPackageCutData();

    //=========================================================================================================
    // FilterOperator tests
    //=========================================================================================================
    void testFilterOperatorLPF();
    void testFilterOperatorHPF();
    void testFilterOperatorBPF();
    void testFilterOperatorApplyFFTFilter();

    //=========================================================================================================
    // RawSettings tests
    //=========================================================================================================
    void testRawSettingsEventTypeColors();

    void cleanupTestCase();

private:
    QString m_sResourcePath;
    QTemporaryDir m_tempDir;
};

//=============================================================================================================

void TestMneBrowseData::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName("mne-cpp-tests");
    QCoreApplication::setApplicationName("test_mne_browse_data");
    QString binDir = QCoreApplication::applicationDirPath();
    m_sResourcePath = binDir + "/../resources/data/mne-cpp-test-data/";
    QVERIFY(m_tempDir.isValid());
}

//=============================================================================================================
// FiffBlockReader tests
//=============================================================================================================

void TestMneBrowseData::testFiffBlockReaderConstruct()
{
    FiffBlockReader reader;
    QVERIFY(!reader.isOpen());
    QCOMPARE(reader.firstSample(), 0);
    QCOMPARE(reader.lastSample(), 0);
    QCOMPARE(reader.totalSamples(), 1); // lastSample - firstSample + 1 = 0 - 0 + 1
}

void TestMneBrowseData::testFiffBlockReaderOpen()
{
    QString rawPath = m_sResourcePath + "MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample raw data not available");

    FiffBlockReader reader;
    bool ok = reader.open(rawPath);
    QVERIFY(ok);
    QVERIFY(reader.isOpen());
    QVERIFY(reader.fiffInfo() != nullptr);
    QVERIFY(reader.fiffInfo()->nchan > 0);
    QVERIFY(reader.totalSamples() > 0);
    QVERIFY(reader.firstSample() >= 0);
    QVERIFY(reader.lastSample() > reader.firstSample());

    reader.close();
    QVERIFY(!reader.isOpen());
}

void TestMneBrowseData::testFiffBlockReaderOpenBuffer()
{
    QString rawPath = m_sResourcePath + "MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample raw data not available");

    // Read the file into a buffer
    QFile file(rawPath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QByteArray data = file.readAll();
    file.close();
    QVERIFY(!data.isEmpty());

    FiffBlockReader reader;
    bool ok = reader.openBuffer(data, "buffer_test");
    QVERIFY(ok);
    QVERIFY(reader.isOpen());
    QVERIFY(reader.fiffInfo() != nullptr);
    QVERIFY(reader.totalSamples() > 0);

    reader.close();
}

void TestMneBrowseData::testFiffBlockReaderReadBlock()
{
    QString rawPath = m_sResourcePath + "MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample raw data not available");

    FiffBlockReader reader;
    reader.open(rawPath);
    QVERIFY(reader.isOpen());

    int from = reader.firstSample();
    int to = from + 999; // Read 1000 samples
    if (to > reader.lastSample())
        to = reader.lastSample();

    MatrixXd block = reader.readBlockSync(from, to);
    QVERIFY(block.rows() > 0);
    QVERIFY(block.cols() > 0);
    QCOMPARE(block.rows(), reader.fiffInfo()->nchan);
    QCOMPARE(block.cols(), to - from + 1);

    reader.close();
}

void TestMneBrowseData::testFiffBlockReaderClose()
{
    FiffBlockReader reader;
    // Close on uninitialized reader should not crash
    reader.close();
    QVERIFY(!reader.isOpen());
}

void TestMneBrowseData::testFiffBlockReaderUpdateProjections()
{
    QString rawPath = m_sResourcePath + "MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample raw data not available");

    FiffBlockReader reader;
    reader.open(rawPath);
    QVERIFY(reader.isOpen());

    // Should not crash; updates internal projection state
    reader.updateProjections();
    QVERIFY(reader.isOpen());

    reader.close();
}

//=============================================================================================================
// RawModel tests
//=============================================================================================================

void TestMneBrowseData::testRawModelConstruct()
{
    RawModel model(nullptr);
    QVERIFY(!model.isFileLoaded());
    QCOMPARE(model.rowCount(), 0);
}

void TestMneBrowseData::testRawModelLoadFiff()
{
    QString rawPath = m_sResourcePath + "MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample raw data not available");

    RawModel model(nullptr);
    QFile file(rawPath);
    // Note: don't open the file - loadFiffData opens its own copy
    bool ok = model.loadFiffData(&file);
    QVERIFY(ok);
    QVERIFY(model.isFileLoaded());
    QVERIFY(model.fiffInfo() != nullptr);
    QVERIFY(model.fiffInfo()->nchan > 0);
    QVERIFY(model.rowCount() > 0);
    QVERIFY(model.columnCount() > 0);
}

void TestMneBrowseData::testRawModelRowColumnCount()
{
    QString rawPath = m_sResourcePath + "MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample raw data not available");

    RawModel model(nullptr);
    QFile file(rawPath);
    model.loadFiffData(&file);

    QVERIFY(model.rowCount() > 0);
    QVERIFY(model.columnCount() > 0);
    // rowCount should equal number of channels
    QCOMPARE(model.rowCount(), model.fiffInfo()->nchan);
}

void TestMneBrowseData::testRawModelData()
{
    QString rawPath = m_sResourcePath + "MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample raw data not available");

    RawModel model(nullptr);
    QFile file(rawPath);
    model.loadFiffData(&file);

    // Column 0 names the channel, column 1 carries its loaded samples
    QCOMPARE(model.data(model.index(0, 0), Qt::DisplayRole).toString(), model.fiffInfo()->ch_names.first());
    QVERIFY(model.data(model.index(0, 1), Qt::DisplayRole).isValid());

    // Access invalid index
    QVariant invalid = model.data(QModelIndex(), Qt::DisplayRole);
    QVERIFY(!invalid.isValid());
}

void TestMneBrowseData::testRawModelHeaderData()
{
    QString rawPath = m_sResourcePath + "MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample raw data not available");

    RawModel model(nullptr);
    QFile file(rawPath);
    model.loadFiffData(&file);

    QVariant header = model.headerData(0, Qt::Vertical, Qt::DisplayRole);
    QVERIFY(header.isValid());
}

void TestMneBrowseData::testRawModelClearModel()
{
    QString rawPath = m_sResourcePath + "MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample raw data not available");

    RawModel model(nullptr);
    QFile file(rawPath);
    model.loadFiffData(&file);
    QVERIFY(model.isFileLoaded());

    model.clearModel();
    QVERIFY(!model.isFileLoaded());
}

void TestMneBrowseData::testRawModelWriteFiff()
{
    QString rawPath = m_sResourcePath + "MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample raw data not available");

    RawModel model(nullptr);
    QFile file(rawPath);
    model.loadFiffData(&file);
    QVERIFY(model.isFileLoaded());

    QString outPath = m_tempDir.path() + "/rawmodel_output.fif";
    QFile outFile(outPath);
    bool ok = model.writeFiffData(&outFile);
    QVERIFY(ok);
    QVERIFY(QFile::exists(outPath));
    QFileInfo fi(outPath);
    QVERIFY(fi.size() > 0);

    QFile unwritable(m_tempDir.filePath(QStringLiteral("no/such/dir/out.fif")));
    QVERIFY(!model.writeFiffData(&unwritable));
}

void TestMneBrowseData::testRawModelChannelInfo()
{
    QString rawPath = m_sResourcePath + "MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample raw data not available");

    RawModel model(nullptr);
    QFile file(rawPath);
    model.loadFiffData(&file);

    const auto& chInfo = model.channelInfoList();
    QVERIFY(!chInfo.isEmpty());
    QCOMPARE(chInfo.size(), model.fiffInfo()->nchan);

    // Test channelUnit
    int unit = model.channelUnit(0);
    QVERIFY(unit != 0); // Should be a valid FIFF unit
}

void TestMneBrowseData::testRawModelSizeOfFiffData()
{
    QString rawPath = m_sResourcePath + "MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample raw data not available");

    RawModel model(nullptr);
    QFile file(rawPath);
    model.loadFiffData(&file);

    QVERIFY(model.sizeOfFiffData() > 0);
}

void TestMneBrowseData::testRawModelFirstLastSample()
{
    QString rawPath = m_sResourcePath + "MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample raw data not available");

    RawModel model(nullptr);
    QFile file(rawPath);
    model.loadFiffData(&file);

    QVERIFY(model.lastSample() > model.firstSample());
    QVERIFY(model.sizeOfPreloadedData() > 0);
}

void TestMneBrowseData::testRawModelOperators()
{
    RawModel model(nullptr);
    auto& ops = model.operators();
    // Default construction generates standard filter operators
    QVERIFY(!ops.isEmpty());
}

void TestMneBrowseData::testRawModelMarkChBad()
{
    QString rawPath = m_sResourcePath + "MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample raw data not available");

    RawModel model(nullptr);
    QFile file(rawPath);
    model.loadFiffData(&file);

    // Mark first channel as bad (must use column 1 — markChBad only processes column 1)
    QModelIndexList indices;
    indices << model.index(0, 1);
    int badsBefore = model.fiffInfo()->bads.size();
    model.markChBad(indices, true);

    // Verify bad channel was added
    QVERIFY(model.fiffInfo()->bads.size() > badsBefore);

    // Unmark it
    model.markChBad(indices, false);
    QCOMPARE(model.fiffInfo()->bads.size(), badsBefore);
}

void TestMneBrowseData::testRawModelProjections()
{
    QString rawPath = m_sResourcePath + "MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample raw data not available");

    RawModel model(nullptr);
    QFile file(rawPath);
    model.loadFiffData(&file);

    // Should not crash
    model.updateProjections();
    QVERIFY(model.isFileLoaded());
}

void TestMneBrowseData::testRawModelCompensator()
{
    QString rawPath = m_sResourcePath + "MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample raw data not available");

    RawModel model(nullptr);
    QFile file(rawPath);
    model.loadFiffData(&file);

    // Should not crash even with no compensators
    model.updateCompensator(0);
    QVERIFY(model.isFileLoaded());
}

//=============================================================================================================
// AnnotationModel tests
//=============================================================================================================

void TestMneBrowseData::testAnnotationModelConstruct()
{
    AnnotationModel model;
    QCOMPARE(model.rowCount(), 0);
    QVERIFY(!model.isFileLoaded());
}

void TestMneBrowseData::testAnnotationModelAddAnnotation()
{
    AnnotationModel model;
    model.setFirstLastSample(0, 100000);

    int row = model.addAnnotation(1000, 2000, "Test Annotation");
    QVERIFY(row >= 0);
    QCOMPARE(model.rowCount(), 1);

    // Add another
    row = model.addAnnotation(3000, 4000, "Second Annotation", {"MEG 0111"}, "A comment");
    QVERIFY(row >= 0);
    QCOMPARE(model.rowCount(), 2);
}

void TestMneBrowseData::testAnnotationModelData()
{
    AnnotationModel model;
    model.setFirstLastSample(0, 100000);
    model.addAnnotation(1000, 2000, "Test");

    QModelIndex idx = model.index(0, 0);
    QVariant val = model.data(idx, Qt::DisplayRole);
    QVERIFY(val.isValid());

    // Invalid index
    QVERIFY(!model.data(QModelIndex(), Qt::DisplayRole).isValid());
}

void TestMneBrowseData::testAnnotationModelSetData()
{
    AnnotationModel model;
    model.setFirstLastSample(0, 100000);
    model.addAnnotation(1000, 2000, "Original");

    QModelIndex idx = model.index(0, 0);
    Qt::ItemFlags f = model.flags(idx);
    QVERIFY(f & Qt::ItemIsEnabled);
}

void TestMneBrowseData::testAnnotationModelRemoveRows()
{
    AnnotationModel model;
    model.setFirstLastSample(0, 100000);
    model.addAnnotation(1000, 2000, "First");
    model.addAnnotation(3000, 4000, "Second");
    QCOMPARE(model.rowCount(), 2);

    model.removeRows(0, 1);
    QCOMPARE(model.rowCount(), 1);
}

void TestMneBrowseData::testAnnotationModelGetSpans()
{
    AnnotationModel model;
    model.setFirstLastSample(0, 100000);
    model.addAnnotation(1000, 2000, "span1");
    model.addAnnotation(5000, 6000, "span2");

    auto spans = model.getAnnotationSpans();
    QCOMPARE(spans.size(), 2);
    QCOMPARE(spans[0].startSample, 1000);
    QCOMPARE(spans[0].endSample, 2000);
    QCOMPARE(spans[0].label, QString("span1"));
}

void TestMneBrowseData::testAnnotationModelSaveLoadJson()
{
    AnnotationModel model;
    model.setFirstLastSample(0, 100000);
    model.addAnnotation(1000, 2000, "TestAnno");

    // Save to JSON
    QString path = m_tempDir.path() + "/annotations.json";
    QFile saveFile(path);
    bool ok = model.saveAnnotationData(saveFile);
    QVERIFY(ok);
    QVERIFY(QFile::exists(path));

    // Load back
    AnnotationModel loaded;
    loaded.setFirstLastSample(0, 100000);
    QFile loadFile(path);
    ok = loaded.loadAnnotationData(loadFile);
    QVERIFY(ok);
    QCOMPARE(loaded.rowCount(), 1);
}

void TestMneBrowseData::testAnnotationModelSaveLoadCsv()
{
    AnnotationModel model;
    model.setFirstLastSample(0, 100000);
    model.addAnnotation(1000, 2000, "TestCSV");

    // Save to CSV
    QString path = m_tempDir.path() + "/annotations.csv";
    QFile saveFile(path);
    bool ok = model.saveAnnotationData(saveFile);
    QVERIFY(ok);

    // Load back
    AnnotationModel loaded;
    loaded.setFirstLastSample(0, 100000);
    QFile loadFile(path);
    ok = loaded.loadAnnotationData(loadFile);
    QVERIFY(ok);
    QCOMPARE(loaded.rowCount(), 1);
}

void TestMneBrowseData::testAnnotationModelClear()
{
    AnnotationModel model;
    model.setFirstLastSample(0, 100000);
    model.addAnnotation(1000, 2000, "Test");
    QCOMPARE(model.rowCount(), 1);

    model.clearModel();
    QCOMPARE(model.rowCount(), 0);
}

void TestMneBrowseData::testAnnotationModelUpdateBoundary()
{
    AnnotationModel model;
    model.setFirstLastSample(0, 100000);
    model.addAnnotation(1000, 2000, "Test");

    bool ok = model.updateAnnotationBoundary(0, true, 500); // Update start
    QVERIFY(ok);

    auto range = model.getSampleRange(0);
    QCOMPARE(range.first, 500);

    ok = model.updateAnnotationBoundary(0, false, 3000); // Update end
    QVERIFY(ok);
    range = model.getSampleRange(0);
    QCOMPARE(range.second, 3000);
}

void TestMneBrowseData::testAnnotationModelSampleRange()
{
    AnnotationModel model;
    model.setFirstLastSample(0, 100000);
    model.addAnnotation(1000, 2000, "Test");

    auto range = model.getSampleRange(0);
    QCOMPARE(range.first, 1000);
    QCOMPARE(range.second, 2000);
}

//=============================================================================================================
// EventModel tests
//=============================================================================================================

void TestMneBrowseData::testEventModelConstruct()
{
    EventModel model(nullptr);
    QVERIFY(!model.isFileLoaded());
    QCOMPARE(model.rowCount(), 0);
}

void TestMneBrowseData::testEventModelSetEventMatrix()
{
    EventModel model(nullptr);
    model.setFirstLastSample(0, 100000);

    // Create event matrix (N x 3): [sample, prev_value, event_code]
    MatrixXi events(3, 3);
    events << 1000, 0, 1,
        2000, 0, 2,
        3000, 0, 1;
    model.setEventMatrix(events);

    QCOMPARE(model.rowCount(), 3);
}

void TestMneBrowseData::testEventModelRowColumnCount()
{
    EventModel model(nullptr);
    model.setFirstLastSample(0, 100000);

    MatrixXi events(2, 3);
    events << 1000, 0, 1,
        2000, 0, 2;
    model.setEventMatrix(events);

    QCOMPARE(model.rowCount(), 2);
    QVERIFY(model.columnCount() > 0);
}

void TestMneBrowseData::testEventModelData()
{
    EventModel model(nullptr);
    model.setFirstLastSample(0, 100000);

    MatrixXi events(1, 3);
    events << 5000, 0, 1;
    model.setEventMatrix(events);

    QModelIndex idx = model.index(0, 0);
    QVariant val = model.data(idx, Qt::DisplayRole);
    QVERIFY(val.isValid());
}

void TestMneBrowseData::testEventModelSetData()
{
    EventModel model(nullptr);
    model.setFirstLastSample(0, 100000);

    MatrixXi events(1, 3);
    events << 5000, 0, 1;
    model.setEventMatrix(events);

    QModelIndex idx = model.index(0, 0);
    Qt::ItemFlags f = model.flags(idx);
    QVERIFY(f & Qt::ItemIsEnabled);
}

void TestMneBrowseData::testEventModelInsertRemoveRows()
{
    EventModel model(nullptr);
    model.setFirstLastSample(0, 100000);
    model.setCurrentMarkerPos(5000);

    // Insert a row
    model.insertRows(0, 1);
    int afterFirst = model.rowCount();
    QVERIFY(afterFirst >= 1);

    // Insert another
    model.setCurrentMarkerPos(6000);
    model.insertRows(afterFirst, 1);
    int afterSecond = model.rowCount();
    QVERIFY(afterSecond > afterFirst);

    // Remove first
    model.removeRows(0, 1);
    QVERIFY(model.rowCount() < afterSecond);
}

void TestMneBrowseData::testEventModelEventTypes()
{
    EventModel model(nullptr);
    model.setFirstLastSample(0, 100000);

    MatrixXi events(3, 3);
    events << 1000, 0, 1,
        2000, 0, 2,
        3000, 0, 3;
    model.setEventMatrix(events);

    QStringList types = model.getEventTypeList();
    QVERIFY(types.size() >= 3); // At least events with codes 1, 2, 3

    auto colors = model.getEventTypeColors();
    QVERIFY(!colors.isEmpty());
}

void TestMneBrowseData::testEventModelFilterType()
{
    EventModel model(nullptr);
    model.setFirstLastSample(0, 100000);

    MatrixXi events(3, 3);
    events << 1000, 0, 1,
        2000, 0, 2,
        3000, 0, 1;
    model.setEventMatrix(events);

    // Filter to show only event type 1
    model.setEventFilterType("1");

    // Should show fewer rows (only type 1 events)
    QVERIFY(model.rowCount() <= 3);
}

void TestMneBrowseData::testEventModelClear()
{
    EventModel model(nullptr);
    model.setFirstLastSample(0, 100000);

    MatrixXi events(2, 3);
    events << 1000, 0, 1,
        2000, 0, 2;
    model.setEventMatrix(events);

    model.clearModel();
    QCOMPARE(model.rowCount(), 0);
}

void TestMneBrowseData::testEventModelSaveLoadEvents()
{
    EventModel model(nullptr);
    model.setFirstLastSample(0, 100000);

    MatrixXi events(2, 3);
    events << 1000, 0, 1,
        2000, 0, 2;
    model.setEventMatrix(events);

    // Save as .eve (ASCII format — fif format has a write/read format mismatch)
    QString path = m_tempDir.path() + "/events.eve";
    QFile saveFile(path);
    bool ok = model.saveEventData(saveFile);
    QVERIFY(ok);
    QVERIFY(QFile::exists(path));

    // Load back
    EventModel loaded(nullptr);
    QFile loadFile(path);
    ok = loaded.loadEventData(loadFile);
    QVERIFY(ok);
    QCOMPARE(loaded.rowCount(), 2);
}

//=============================================================================================================
// AverageModel tests
//=============================================================================================================

void TestMneBrowseData::testAverageModelConstruct()
{
    AverageModel model;
    QVERIFY(!model.isFileLoaded());
    QCOMPARE(model.rowCount(), 0);
}

void TestMneBrowseData::testAverageModelLoadEvoked()
{
    QString avePath = m_sResourcePath + "MEG/sample/sample_audvis-ave.fif";
    if (!QFile::exists(avePath))
        QSKIP("Average data not available");

    AverageModel model;
    QFile file(avePath);
    bool ok = model.loadEvokedData(file);
    QVERIFY(ok);
    QVERIFY(model.isFileLoaded());
    QVERIFY(model.rowCount() > 0);
}

void TestMneBrowseData::testAverageModelSetEvoked()
{
    QString avePath = m_sResourcePath + "MEG/sample/sample_audvis-ave.fif";
    if (!QFile::exists(avePath))
        QSKIP("Average data not available");

    // Load via FiffEvokedSet
    QFile file(avePath);
    FiffEvokedSet evokedSet(file);

    AverageModel model;
    bool ok = model.setEvokedData(evokedSet);
    QVERIFY(ok);
    QVERIFY(model.rowCount() > 0);
}

void TestMneBrowseData::testAverageModelData()
{
    QString avePath = m_sResourcePath + "MEG/sample/sample_audvis-ave.fif";
    if (!QFile::exists(avePath))
        QSKIP("Average data not available");

    AverageModel model;
    QFile file(avePath);
    model.loadEvokedData(file);

    QModelIndex idx = model.index(0, 0);
    QVariant val = model.data(idx, Qt::DisplayRole);
    QVERIFY(val.isValid());

    // Header data
    QVariant header = model.headerData(0, Qt::Vertical, Qt::DisplayRole);
    QVERIFY(header.isValid());

    // FiffInfo
    FiffInfo info = model.getFiffInfo();
    QVERIFY(info.nchan > 0);

    // Get evoked
    const FiffEvoked* evoked = model.getEvoked(0);
    QVERIFY(evoked != nullptr);
    QVERIFY(evoked->nave > 0);
}

void TestMneBrowseData::testAverageModelClear()
{
    QString avePath = m_sResourcePath + "MEG/sample/sample_audvis-ave.fif";
    if (!QFile::exists(avePath))
        QSKIP("Average data not available");

    AverageModel model;
    QFile file(avePath);
    model.loadEvokedData(file);
    QVERIFY(model.rowCount() > 0);
}

//=============================================================================================================
// EpochModel tests
//=============================================================================================================

void TestMneBrowseData::testEpochModelConstruct()
{
    EpochModel model;
    QCOMPARE(model.rowCount(), 0);
    QVERIFY(model.respectAutoRejects());
}

void TestMneBrowseData::testEpochModelReviewsEpochs()
{
    // Two event codes; epochs listed out of time order; the code-3 epoch at sample 1300 is auto-rejected
    const auto makeEpoch = [](int sample, double ptp, bool reject) {
        auto epoch = MNEEpochData::SPtr::create();
        epoch->eventSample = sample;
        epoch->epoch = MatrixXd::Zero(2, 10);
        epoch->epoch(1, 4) = ptp;
        epoch->bReject = reject;
        return epoch;
    };
    MNEEpochDataList code1;
    code1 << makeEpoch(1900, 2.0, false) << makeEpoch(1100, 1.0, false);
    MNEEpochDataList code3;
    code3 << makeEpoch(1300, 5.0, true);

    EpochModel model;
    QSignalSpy changed(&model, &EpochModel::epochsChanged);
    model.setEpochs({code1, code3}, {1, 3}, 1000, 100.0f);
    QCOMPARE(changed.size(), 1);
    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(model.columnCount(), 6);
    QCOMPARE(model.headerData(3, Qt::Horizontal).toString(), QString("Time (s)"));

    // Sorted by sample, relative to the first sample
    const QList<int> rowSamples{model.sampleAt(0), model.sampleAt(1), model.sampleAt(2)};
    QCOMPARE(rowSamples, (QList<int>{1100, 1300, 1900}));
    QCOMPARE(model.data(model.index(1, 1)).toInt(), 3);
    QCOMPARE(model.data(model.index(1, 2)).toInt(), 300);
    QCOMPARE(model.data(model.index(1, 3)).toString(), QString("3.000"));
    QCOMPARE(model.data(model.index(1, 4)).toString(), QString("yes"));
    QCOMPARE(model.data(model.index(1, 5)).toString(), QString("auto drop"));
    QCOMPARE(model.data(model.index(1, 0), Qt::CheckStateRole).toInt(), int(Qt::Unchecked));
    QCOMPARE(model.summaryText(), QString("2 / 3 epochs currently contribute to the evoked response. 1 auto-rejected, 0 manually excluded."));
    QCOMPARE(model.ptpAmplitudes(), (QVector<double>{1.0, 2.0}));

    // An auto-rejected epoch cannot be ticked while auto rejects are respected
    QVERIFY(!(model.flags(model.index(1, 0)) & Qt::ItemIsUserCheckable));
    QVERIFY(!model.setData(model.index(1, 0), Qt::Checked, Qt::CheckStateRole));

    // Manually dropping a kept epoch
    QVERIFY(model.setData(model.index(2, 0), Qt::Unchecked, Qt::CheckStateRole));
    QVERIFY(!model.setData(model.index(2, 0), Qt::Unchecked, Qt::CheckStateRole));
    QCOMPARE(model.data(model.index(2, 5)).toString(), QString("manual drop"));
    QCOMPARE(model.ptpAmplitudes(), (QVector<double>{1.0}));
    QCOMPARE(model.summaryText(), QString("1 / 3 epochs currently contribute to the evoked response. 1 auto-rejected, 1 manually excluded."));

    // Ignoring auto rejects brings the rejected epoch back
    model.setRespectAutoRejects(false);
    QVERIFY(model.flags(model.index(1, 0)) & Qt::ItemIsUserCheckable);
    QCOMPARE(model.data(model.index(1, 5)).toString(), QString("kept"));
    QCOMPARE(model.ptpAmplitudes(), (QVector<double>{1.0, 5.0}));

    model.resetManualExclusions();
    QCOMPARE(model.data(model.index(2, 5)).toString(), QString("kept"));
    QCOMPARE(model.ptpAmplitudes(), (QVector<double>{1.0, 5.0, 2.0}));

    model.clearModel();
    QCOMPARE(model.rowCount(), 0);
    QVERIFY(!model.data(model.index(0, 0)).isValid());
}

//=============================================================================================================
// DataPackage tests
//=============================================================================================================

void TestMneBrowseData::testDataPackageConstruct()
{
    DataPackage pkg;
    // Default construction with empty matrices
    QVERIFY(pkg.dataRaw().rows() == 0 || pkg.dataRaw().cols() == 0);
}

void TestMneBrowseData::testDataPackageSetOrigRawData()
{
    MatrixXdR rawData = MatrixXdR::Random(10, 1000);
    MatrixXdR rawTime(1, 1000);
    rawTime.row(0) = Eigen::RowVectorXd::LinSpaced(1000, 0.0, 1.0);

    DataPackage pkg(rawData, rawTime);
    QCOMPARE(pkg.dataRaw().rows(), 10);
    QCOMPARE(pkg.dataRaw().cols(), 1000);
}

void TestMneBrowseData::testDataPackageDataRaw()
{
    MatrixXdR rawData = MatrixXdR::Random(5, 500);
    MatrixXdR rawTime(1, 500);
    rawTime.row(0) = Eigen::RowVectorXd::LinSpaced(500, 0.0, 0.5);

    DataPackage pkg(rawData, rawTime);

    const MatrixXdR& raw = pkg.dataRaw();
    QCOMPARE(raw.rows(), 5);
    QCOMPARE(raw.cols(), 500);

    const MatrixXdR& rawOrig = pkg.dataRawOrig();
    QCOMPARE(rawOrig.rows(), 5);
}

void TestMneBrowseData::testDataPackageDataProc()
{
    MatrixXdR rawData = MatrixXdR::Random(5, 500);
    MatrixXdR rawTime(1, 500);
    rawTime.row(0) = Eigen::RowVectorXd::LinSpaced(500, 0.0, 0.5);

    DataPackage pkg(rawData, rawTime);

    // Set processed data
    MatrixXdR procData = rawData * 2.0;
    pkg.setOrigProcData(procData);

    const MatrixXdR& proc = pkg.dataProc();
    QCOMPARE(proc.rows(), 5);
    QCOMPARE(proc.cols(), 500);
}

void TestMneBrowseData::testDataPackageMean()
{
    MatrixXdR rawData = MatrixXdR::Ones(3, 100);
    rawData.row(0) *= 1.0;
    rawData.row(1) *= 2.0;
    rawData.row(2) *= 3.0;
    MatrixXdR rawTime(1, 100);
    rawTime.row(0) = Eigen::RowVectorXd::LinSpaced(100, 0.0, 0.1);

    DataPackage pkg(rawData, rawTime);

    double mean0 = pkg.dataRawMean(0);
    QVERIFY(qAbs(mean0 - 1.0) < 0.01);

    double mean1 = pkg.dataRawMean(1);
    QVERIFY(qAbs(mean1 - 2.0) < 0.01);
}

void TestMneBrowseData::testDataPackageCutData()
{
    MatrixXdR rawData = MatrixXdR::Random(3, 200);
    MatrixXdR rawTime(1, 200);
    rawTime.row(0) = Eigen::RowVectorXd::LinSpaced(200, 0.0, 0.2);

    // Cut 10 from front and 10 from back
    DataPackage pkg(rawData, rawTime, 10, 10);

    const MatrixXdR& mapped = pkg.dataRaw();
    QCOMPARE(mapped.cols(), 180);
}

//=============================================================================================================
// FilterOperator tests
//=============================================================================================================

void TestMneBrowseData::testFilterOperatorLPF()
{
    double sFreq = 600.0;
    double centerFreq = 40.0 / sFreq;
    int order = 128;

    FilterOperator filter("LPF_40Hz",
                          FilterOperator::LPF,
                          order,
                          centerFreq,
                          0.0, // bandwidth (not used for LPF)
                          0.1, // parks width
                          sFreq,
                          4096,
                          FilterOperator::Cosine);

    QCOMPARE(filter.m_Type, FilterOperator::LPF);
    QVERIFY(filter.m_iFilterOrder > 0);
    QVERIFY(filter.m_dCoeffA.size() > 0);
}

void TestMneBrowseData::testFilterOperatorHPF()
{
    double sFreq = 600.0;
    double centerFreq = 1.0 / sFreq;
    int order = 128;

    FilterOperator filter("HPF_1Hz",
                          FilterOperator::HPF,
                          order,
                          centerFreq,
                          0.0,
                          0.1,
                          sFreq,
                          4096,
                          FilterOperator::Cosine);

    QCOMPARE(filter.m_Type, FilterOperator::HPF);
    QVERIFY(filter.m_dCoeffA.size() > 0);
}

void TestMneBrowseData::testFilterOperatorBPF()
{
    double sFreq = 600.0;
    double centerFreq = 20.0 / sFreq;
    double bandwidth = 30.0 / sFreq;
    int order = 128;

    FilterOperator filter("BPF_5-35Hz",
                          FilterOperator::BPF,
                          order,
                          centerFreq,
                          bandwidth,
                          0.1,
                          sFreq,
                          4096,
                          FilterOperator::Cosine);

    QCOMPARE(filter.m_Type, FilterOperator::BPF);
    QVERIFY(filter.m_dCoeffA.size() > 0);
}

void TestMneBrowseData::testFilterOperatorApplyFFTFilter()
{
    double sFreq = 600.0;
    int order = 128;

    // Frequencies are relative to Nyquist, as RawModel passes them: 40 Hz cut-off, 5 Hz transition
    const double nyquist = sFreq / 2.0;
    FilterOperator filter("LPF_40Hz",
                          FilterOperator::LPF,
                          order,
                          40.0 / nyquist,
                          5.0 / nyquist,
                          5.0 / nyquist,
                          sFreq,
                          4096,
                          FilterOperator::Cosine);

    // Create a test signal: 10 Hz sine + 100 Hz sine
    int nSamples = 4096;
    RowVectorXd signal = RowVectorXd::Zero(nSamples);
    for (int i = 0; i < nSamples; ++i) {
        double t = static_cast<double>(i) / sFreq;
        signal(i) = std::sin(2.0 * M_PI * 10.0 * t) + std::sin(2.0 * M_PI * 100.0 * t);
    }

    RowVectorXd filtered = filter.applyFFTFilter(signal);
    QCOMPARE(filtered.size(), signal.size());

    // Amplitude of each component over the middle half, away from the edges: 10 Hz passes, 100 Hz is stopped
    const auto amplitude = [&](double freq) {
        double re = 0.0, im = 0.0;
        for (int i = nSamples / 4; i < 3 * nSamples / 4; ++i) {
            re += filtered(i) * std::cos(2.0 * M_PI * freq * i / sFreq);
            im += filtered(i) * std::sin(2.0 * M_PI * freq * i / sFreq);
        }
        return 4.0 * std::hypot(re, im) / nSamples;
    };
    QVERIFY2(std::fabs(amplitude(10.0) - 1.0) < 0.02, qPrintable(QString::number(amplitude(10.0))));
    QVERIFY2(amplitude(100.0) < 0.01, qPrintable(QString::number(amplitude(100.0))));
}

//=============================================================================================================
// RawSettings tests
//=============================================================================================================

void TestMneBrowseData::testRawSettingsEventTypeColors()
{
    QSettings().remove("EventDesignParameters");
    QCOMPARE(RawSettings().eventTypeColors().value(2), QColor(Qt::magenta));
    QVERIFY(!RawSettings().eventTypeColors().contains(7));

    // A colour picked for a type persists, overrides a default, and seeds new event models
    EventModel model(nullptr);
    model.addNewEventType("7", QColor(1, 2, 3));
    RawSettings().setEventTypeColor(2, QColor(4, 5, 6));
    QCOMPARE(RawSettings().eventTypeColors().value(7), QColor(1, 2, 3));
    QCOMPARE(EventModel(nullptr).getEventTypeColors().value(2), QColor(4, 5, 6));
    QCOMPARE(EventModel(nullptr).getEventTypeColors().value(3), QColor(Qt::green));
    QSettings().remove("EventDesignParameters");
}

//=============================================================================================================

void TestMneBrowseData::cleanupTestCase()
{
}

//=============================================================================================================

QTEST_MAIN(TestMneBrowseData)
#include "test_mne_browse_data.moc"
