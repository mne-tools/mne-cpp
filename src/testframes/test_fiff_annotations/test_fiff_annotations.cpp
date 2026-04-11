//=============================================================================================================
/**
 * @file     test_fiff_annotations.cpp
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
 * @brief    Tests for FiffAnnotations class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_annotations.h>

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

    void cleanupTestCase();

private:
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
    QCOMPARE(annot.size(), 0);
    QVERIFY(annot.isEmpty());
}

//=============================================================================================================

void TestFiffAnnotations::testAppendAndSize()
{
    FiffAnnotations annot;
    FiffAnnotation entry;
    entry.onset = 1.0;
    entry.duration = 0.5;
    entry.description = "stimulus/1";

    annot.append(entry);
    QCOMPARE(annot.size(), 1);
    QCOMPARE(annot[0].onset, 1.0);
    QCOMPARE(annot[0].duration, 0.5);
    QCOMPARE(annot[0].description, QString("stimulus/1"));
}

//=============================================================================================================

void TestFiffAnnotations::testAppendConvenience()
{
    FiffAnnotations annot;
    annot.append(2.5, 1.0, "response/1", QStringList{"MEG0111"}, "good response");

    QCOMPARE(annot.size(), 1);
    QCOMPARE(annot[0].onset, 2.5);
    QCOMPARE(annot[0].duration, 1.0);
    QCOMPARE(annot[0].description, QString("response/1"));
    QCOMPARE(annot[0].channelNames.size(), 1);
    QCOMPARE(annot[0].comment, QString("good response"));
}

//=============================================================================================================

void TestFiffAnnotations::testOperatorIndex()
{
    FiffAnnotations annot;
    annot.append(0.0, 0.1, "event/1");
    annot.append(1.0, 0.2, "event/2");

    const FiffAnnotation& first = annot[0];
    QCOMPARE(first.description, QString("event/1"));

    // Mutable access
    annot[1].description = "event/2_modified";
    QCOMPARE(annot[1].description, QString("event/2_modified"));
}

//=============================================================================================================

void TestFiffAnnotations::testRemove()
{
    FiffAnnotations annot;
    annot.append(0.0, 0.1, "a");
    annot.append(1.0, 0.2, "b");
    annot.append(2.0, 0.3, "c");

    annot.remove(1); // Remove "b"
    QCOMPARE(annot.size(), 2);
    QCOMPARE(annot[0].description, QString("a"));
    QCOMPARE(annot[1].description, QString("c"));
}

//=============================================================================================================

void TestFiffAnnotations::testClear()
{
    FiffAnnotations annot;
    annot.append(0.0, 0.1, "a");
    annot.append(1.0, 0.2, "b");
    annot.clear();

    QCOMPARE(annot.size(), 0);
    QVERIFY(annot.isEmpty());
}

//=============================================================================================================

void TestFiffAnnotations::testIsEmpty()
{
    FiffAnnotations annot;
    QVERIFY(annot.isEmpty());
    annot.append(0.0, 0.1, "x");
    QVERIFY(!annot.isEmpty());
}

//=============================================================================================================

void TestFiffAnnotations::testOnsetToSample()
{
    FiffAnnotations annot;
    annot.append(1.5, 0.5, "test");

    double sfreq = 1000.0;
    int firstSample = 0;
    int sample = annot.onsetToSample(0, sfreq, firstSample);
    QCOMPARE(sample, 1500);
}

//=============================================================================================================

void TestFiffAnnotations::testEndToSample()
{
    FiffAnnotations annot;
    annot.append(1.5, 0.5, "test");

    double sfreq = 1000.0;
    int firstSample = 0;
    int sample = annot.endToSample(0, sfreq, firstSample);
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
    QCOMPARE(stimuli.size(), 2);
    QCOMPARE(stimuli[0].description, QString("stimulus/1"));
    QCOMPARE(stimuli[1].description, QString("stimulus/2"));
}

//=============================================================================================================

void TestFiffAnnotations::testSelectByChannel()
{
    FiffAnnotations annot;
    annot.append(0.0, 0.1, "bad", QStringList{"MEG0111", "MEG0112"});
    annot.append(1.0, 0.1, "bad", QStringList{"MEG0211"});
    annot.append(2.0, 0.1, "bad", QStringList{"MEG0111"});

    FiffAnnotations ch0111 = annot.selectByChannel("MEG0111");
    QCOMPARE(ch0111.size(), 2);
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
    QCOMPARE(cropped.size(), 2);
    QCOMPARE(cropped[0].description, QString("b"));
    QCOMPARE(cropped[1].description, QString("c"));
}

//=============================================================================================================

void TestFiffAnnotations::testWriteReadJson()
{
    FiffAnnotations original;
    original.append(0.0, 0.5, "event/1", QStringList{"MEG0111"}, "comment1");
    original.append(1.0, 0.3, "event/2");

    QString path = m_tempDir.filePath("annot_test.json");
    QVERIFY(FiffAnnotations::writeJson(path, original));

    FiffAnnotations restored = FiffAnnotations::readJson(path);
    QCOMPARE(restored.size(), 2);
    QCOMPARE(restored[0].onset, 0.0);
    QCOMPARE(restored[0].description, QString("event/1"));
    QCOMPARE(restored[1].onset, 1.0);
}

//=============================================================================================================

void TestFiffAnnotations::testWriteReadJsonMultiple()
{
    FiffAnnotations original;
    for (int i = 0; i < 100; ++i) {
        original.append(i * 0.5, 0.1, QString("event/%1").arg(i));
    }

    QString path = m_tempDir.filePath("annot_many.json");
    QVERIFY(FiffAnnotations::writeJson(path, original));

    FiffAnnotations restored = FiffAnnotations::readJson(path);
    QCOMPARE(restored.size(), 100);
    QCOMPARE(restored[99].description, QString("event/99"));
}

//=============================================================================================================

void TestFiffAnnotations::testWriteReadCsv()
{
    FiffAnnotations original;
    original.append(0.0, 0.5, "stim");
    original.append(1.0, 0.3, "resp");

    QString path = m_tempDir.filePath("annot_test.csv");
    QVERIFY(FiffAnnotations::writeCsv(path, original));

    FiffAnnotations restored = FiffAnnotations::readCsv(path);
    QCOMPARE(restored.size(), 2);
    QCOMPARE(restored[0].onset, 0.0);
    QCOMPARE(restored[0].description, QString("stim"));
}

//=============================================================================================================

void TestFiffAnnotations::testWriteReadAutoJson()
{
    FiffAnnotations original;
    original.append(0.0, 0.5, "auto_test");

    QString path = m_tempDir.filePath("annot_auto.json");
    QVERIFY(FiffAnnotations::write(path, original));

    FiffAnnotations restored = FiffAnnotations::read(path);
    QCOMPARE(restored.size(), 1);
    QCOMPARE(restored[0].description, QString("auto_test"));
}

//=============================================================================================================

void TestFiffAnnotations::testEmptyAnnotations()
{
    FiffAnnotations empty;
    QString path = m_tempDir.filePath("annot_empty.json");
    QVERIFY(FiffAnnotations::writeJson(path, empty));

    FiffAnnotations restored = FiffAnnotations::readJson(path);
    QCOMPARE(restored.size(), 0);
}

//=============================================================================================================

void TestFiffAnnotations::testSelectNoMatch()
{
    FiffAnnotations annot;
    annot.append(0.0, 0.1, "stimulus/1");
    FiffAnnotations result = annot.select("nonexistent");
    QCOMPARE(result.size(), 0);
}

//=============================================================================================================

void TestFiffAnnotations::testCropEmpty()
{
    FiffAnnotations annot;
    FiffAnnotations cropped = annot.crop(0.0, 10.0);
    QCOMPARE(cropped.size(), 0);
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
