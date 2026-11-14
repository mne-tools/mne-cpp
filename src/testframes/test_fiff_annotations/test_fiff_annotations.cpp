//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_fiff_annotations.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Tests for FiffAnnotations class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_annotations.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>
#include <QTemporaryDir>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;

//=============================================================================================================
/**
 * DECLARE CLASS TestFiffAnnotations
 *
 * @brief The TestFiffAnnotations class provides tests for FiffAnnotations.
 */
class TestFiffAnnotations : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // Construction and basic operations
    void testDefaultConstruction();
    void testAppendAndSize();
    void testAppendConvenience();
    void testOperatorIndex();
    void testRemove();
    void testClear();
    void testIsEmpty();

    // Time-sample conversion
    void testOnsetToSample();
    void testEndToSample();

    // Filtering and selection
    void testSelectByDescription();
    void testSelectByChannel();
    void testCrop();

    // JSON I/O
    void testWriteReadJson();
    void testWriteReadJsonMultiple();

    // CSV I/O
    void testWriteReadCsv();

    // Generic read/write (format auto-detection)
    void testWriteReadAutoJson();

    // Edge cases
    void testEmptyAnnotations();
    void testSelectNoMatch();
    void testCropEmpty();
    void testReadersReportWhyTheyFail();

    void cleanupTestCase();

private:
    std::filesystem::path file(const char* name) const
    {
        return m_tempDir.filePath(QString::fromLatin1(name)).toStdString();
    }

    QTemporaryDir m_tempDir;
};

//=============================================================================================================

void TestFiffAnnotations::initTestCase()
{
    QVERIFY(m_tempDir.isValid());
}

//=============================================================================================================

void TestFiffAnnotations::testDefaultConstruction()
{
    FiffAnnotations annot;
    QCOMPARE(annot.size(), std::size_t{0});
    QVERIFY(annot.empty());
}

//=============================================================================================================

void TestFiffAnnotations::testAppendAndSize()
{
    FiffAnnotations annot;
    FiffAnnotation entry;
    entry.onset = 1.0;
    entry.duration = 0.5;
    entry.description = "stimulus/1";

    annot.push_back(entry);
    QCOMPARE(annot.size(), std::size_t{1});
    QCOMPARE(annot[0].onset, 1.0);
    QCOMPARE(annot[0].duration, 0.5);
    QCOMPARE(annot[0].description, std::string("stimulus/1"));
}

//=============================================================================================================

void TestFiffAnnotations::testAppendConvenience()
{
    FiffAnnotations annot;
    annot.append(2.5, 1.0, "response/1", {"MEG 0111"});
    annot[0].comment = "good response";

    QCOMPARE(annot.size(), std::size_t{1});
    QCOMPARE(annot[0].onset, 2.5);
    QCOMPARE(annot[0].duration, 1.0);
    QCOMPARE(annot[0].description, std::string("response/1"));
    QCOMPARE(annot[0].ch_names, std::vector<std::string>{"MEG 0111"});
    QCOMPARE(annot[0].comment, std::string("good response"));
}

//=============================================================================================================

void TestFiffAnnotations::testOperatorIndex()
{
    FiffAnnotations annot;
    annot.append(0.0, 0.1, "event/1");
    annot.append(1.0, 0.2, "event/2");

    const FiffAnnotation& first = annot[0];
    QCOMPARE(first.description, std::string("event/1"));

    // Mutable access
    annot[1].description = "event/2_modified";
    QCOMPARE(annot[1].description, std::string("event/2_modified"));
}

//=============================================================================================================

void TestFiffAnnotations::testRemove()
{
    FiffAnnotations annot;
    annot.append(0.0, 0.1, "a");
    annot.append(1.0, 0.2, "b");
    annot.append(2.0, 0.3, "c");

    annot.erase(annot.begin() + 1); // Remove "b"
    QCOMPARE(annot.size(), std::size_t{2});
    QCOMPARE(annot[0].description, std::string("a"));
    QCOMPARE(annot[1].description, std::string("c"));
}

//=============================================================================================================

void TestFiffAnnotations::testClear()
{
    FiffAnnotations annot;
    annot.append(0.0, 0.1, "a");
    annot.append(1.0, 0.2, "b");
    annot.clear();

    QCOMPARE(annot.size(), std::size_t{0});
    QVERIFY(annot.empty());
}

//=============================================================================================================

void TestFiffAnnotations::testIsEmpty()
{
    FiffAnnotations annot;
    QVERIFY(annot.empty());
    annot.append(0.0, 0.1, "x");
    QVERIFY(!annot.empty());
}

//=============================================================================================================

void TestFiffAnnotations::testOnsetToSample()
{
    FiffAnnotations annot;
    annot.append(1.5, 0.5, "test");

    double sfreq = 1000.0;
    int firstSample = 0;
    int sample = annot.onset_to_sample(0, sfreq, firstSample);
    QCOMPARE(sample, 1500);
}

//=============================================================================================================

void TestFiffAnnotations::testEndToSample()
{
    FiffAnnotations annot;
    annot.append(1.5, 0.5, "test");

    double sfreq = 1000.0;
    int firstSample = 0;
    int sample = annot.end_to_sample(0, sfreq, firstSample);
    QCOMPARE(sample, 2000);
}

//=============================================================================================================

void TestFiffAnnotations::testSelectByDescription()
{
    FiffAnnotations annot;
    annot.append(0.0, 0.1, "stimulus/1");
    annot.append(1.0, 0.1, "response/1");
    annot.append(2.0, 0.1, "stimulus/2");
    annot.append(3.0, 0.1, "response/2");

    FiffAnnotations stimuli = annot.select("stimulus");
    QCOMPARE(stimuli.size(), std::size_t{2});
    QCOMPARE(stimuli[0].description, std::string("stimulus/1"));
    QCOMPARE(stimuli[1].description, std::string("stimulus/2"));
}

//=============================================================================================================

void TestFiffAnnotations::testSelectByChannel()
{
    FiffAnnotations annot;
    annot.append(0.0, 0.1, "bad", {"MEG 0111", "MEG 0112"});
    annot.append(1.0, 0.1, "bad", {"MEG 0211"});
    annot.append(2.0, 0.1, "bad", {"MEG 0111"});

    FiffAnnotations ch0111 = annot.select_by_channel("MEG 0111");
    QCOMPARE(ch0111.size(), std::size_t{2});
}

//=============================================================================================================

void TestFiffAnnotations::testCrop()
{
    FiffAnnotations annot;
    annot.append(0.5, 0.1, "a");
    annot.append(1.5, 0.1, "b");
    annot.append(2.5, 0.1, "c");
    annot.append(3.5, 0.1, "d");

    FiffAnnotations cropped = annot.crop(1.0, 3.0);
    QCOMPARE(cropped.size(), std::size_t{2});
    QCOMPARE(cropped[0].description, std::string("b"));
    QCOMPARE(cropped[1].description, std::string("c"));
}

//=============================================================================================================

void TestFiffAnnotations::testWriteReadJson()
{
    FiffAnnotations original;
    original.append(0.0, 0.5, "event/1", {"MEG 0111"});
    original[0].comment = "comment1";
    original[0].extras = {{"hed", std::string("Sensory-event")}, {"trial", std::int64_t{3}}, {"sample", std::int64_t{9007199254740993}}, {"score", 0.25}, {"valid", true}, {"note", std::monostate{}}};
    original.append(1.0, 0.3, "event/2");

    const std::filesystem::path path = file("annot_test.json");
    QVERIFY(original.write_json(path).has_value());

    const FiffAnnotations restored = FiffAnnotations::read_json(path).value();
    QCOMPARE(restored.size(), std::size_t{2});
    QCOMPARE(restored[0].onset, 0.0);
    QCOMPARE(restored[0].description, std::string("event/1"));
    QCOMPARE(restored[1].onset, 1.0);
    QCOMPARE(restored[0], original[0]); // channels, comment and every extras type survive
    QCOMPARE(restored[1], original[1]);
}

//=============================================================================================================

void TestFiffAnnotations::testWriteReadJsonMultiple()
{
    FiffAnnotations original;
    for (int i = 0; i < 100; ++i) {
        original.append(i * 0.5, 0.1, "event/" + std::to_string(i));
    }

    const std::filesystem::path path = file("annot_many.json");
    QVERIFY(original.write_json(path).has_value());

    const FiffAnnotations restored = FiffAnnotations::read_json(path).value();
    QCOMPARE(restored.size(), std::size_t{100});
    QCOMPARE(restored[99].description, std::string("event/99"));
}

//=============================================================================================================

void TestFiffAnnotations::testWriteReadCsv()
{
    FiffAnnotations original;
    original.append(0.0, 0.5, "stim");
    original.append(3600.123456, 0.3, "resp");

    const std::filesystem::path path = file("annot_test.csv");
    QVERIFY(original.write_csv(path).has_value());

    const FiffAnnotations restored = FiffAnnotations::read_csv(path).value();
    QCOMPARE(restored.size(), std::size_t{2});
    QCOMPARE(restored[0].onset, 0.0);
    QCOMPARE(restored[1].onset, 3600.123456);
    QCOMPARE(restored[0].description, std::string("stim"));
}

//=============================================================================================================

void TestFiffAnnotations::testWriteReadAutoJson()
{
    FiffAnnotations original;
    original.append(0.0, 0.5, "auto_test");

    const std::filesystem::path path = file("annot_auto.json");
    QVERIFY(original.write(path).has_value());

    const FiffAnnotations restored = FiffAnnotations::read(path).value();
    QCOMPARE(restored.size(), std::size_t{1});
    QCOMPARE(restored[0].description, std::string("auto_test"));
}

//=============================================================================================================

void TestFiffAnnotations::testEmptyAnnotations()
{
    FiffAnnotations empty;
    const std::filesystem::path path = file("annot_empty.json");
    QVERIFY(empty.write_json(path).has_value());

    const FiffAnnotations restored = FiffAnnotations::read_json(path).value();
    QCOMPARE(restored.size(), std::size_t{0});
}

//=============================================================================================================

void TestFiffAnnotations::testSelectNoMatch()
{
    FiffAnnotations annot;
    annot.append(0.0, 0.1, "stimulus/1");
    FiffAnnotations result = annot.select("nonexistent");
    QCOMPARE(result.size(), std::size_t{0});
}

//=============================================================================================================

void TestFiffAnnotations::testCropEmpty()
{
    FiffAnnotations annot;
    FiffAnnotations cropped = annot.crop(0.0, 10.0);
    QCOMPARE(cropped.size(), std::size_t{0});
}

//=============================================================================================================

void TestFiffAnnotations::testReadersReportWhyTheyFail()
{
    QCOMPARE(FiffAnnotations::read(file("missing.json")).error().code, BASELIB::ErrorCode::Io);
    QCOMPARE(FiffAnnotations::read_csv(file("missing.csv")).error().code, BASELIB::ErrorCode::Io);

    const std::filesystem::path broken = file("broken.json");
    std::ofstream(broken) << "{\"annotations\": [";
    QCOMPARE(FiffAnnotations::read_json(broken).error().code, BASELIB::ErrorCode::Format);

    FiffAnnotations annot;
    annot.append(0.0, 1.0, "x");
    QCOMPARE(annot.write(file("no/such/dir/annot.json")).error().code, BASELIB::ErrorCode::Io);

    // An unknown extension is read as JSON when it is JSON, otherwise as CSV
    const std::filesystem::path text = file("annot.txt");
    std::ofstream(text) << "onset,duration,description\n1.5,0.5,BAD_x, with comma\n";
    const FiffAnnotations csv = FiffAnnotations::read(text).value();
    QCOMPARE(csv.size(), std::size_t{1});
    QCOMPARE(csv[0].description, std::string("BAD_x, with comma"));
}

//=============================================================================================================

void TestFiffAnnotations::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestFiffAnnotations)
#include "test_fiff_annotations.moc"
