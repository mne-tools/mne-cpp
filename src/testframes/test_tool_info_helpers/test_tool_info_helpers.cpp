//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_tool_info_helpers.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     June, 2026
 * @brief    Tests for info tool helper functions (coordinate frames, tag names, transforms).
 */

//=============================================================================================================
// Include tool sources
//=============================================================================================================

// --- mne_list_source_space ---
#define main _list_src_main_unused
#include "../../tools/info/mne_list_source_space/main.cpp"
#undef main

// --- mne_compare_fif_files ---
#define main _compare_fif_main_unused
#include "../../tools/info/mne_compare_fif_files/main.cpp"
#undef main

// --- mne_collect_transforms ---
#define main _collect_trans_main_unused
#include "../../tools/info/mne_collect_transforms/main.cpp"
#undef main

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QCoreApplication>

//=============================================================================================================

class TestToolInfoHelpers : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // --- coordFrameName tests ---
    void testCoordFrameMRI();
    void testCoordFrameHead();
    void testCoordFrameDevice();
    void testCoordFrameUnknown();
    void testCoordFrameOther();

    // --- spaceTypeName tests ---
    void testSpaceTypeSurface();
    void testSpaceTypeVolume();
    void testSpaceTypeDiscrete();
    void testSpaceTypeUnknown();

    // --- tagName tests ---
    void testTagNameFileId();
    void testTagNameDirPointer();
    void testTagNameDir();
    void testTagNameBlockId();
    void testTagNameBlockStart();
    void testTagNameBlockEnd();
    void testTagNameFreeList();
    void testTagNameNChan();
    void testTagNameSFreq();
    void testTagNameChInfo();
    void testTagNameMeasDate();
    void testTagNameCoordTrans();
    void testTagNameNave();
    void testTagNameFirstSample();
    void testTagNameLastSample();
    void testTagNameComment();
    void testTagNameUnknown();

    // --- printTransform tests ---
    void testPrintTransformIdentity();
    void testPrintTransformDoesNotCrash();

    void cleanupTestCase();

private:
    QString m_sResourcePath;
};

//=============================================================================================================

void TestToolInfoHelpers::initTestCase()
{
    QString binDir = QCoreApplication::applicationDirPath();
    m_sResourcePath = binDir + "/../resources/data/mne-cpp-test-data/";
}

//=============================================================================================================
// coordFrameName tests
//=============================================================================================================

void TestToolInfoHelpers::testCoordFrameMRI()
{
    QCOMPARE(QString(coordFrameName(FIFFV_COORD_MRI)), QString("MRI (surface RAS)"));
}

void TestToolInfoHelpers::testCoordFrameHead()
{
    QCOMPARE(QString(coordFrameName(FIFFV_COORD_HEAD)), QString("Head"));
}

void TestToolInfoHelpers::testCoordFrameDevice()
{
    QCOMPARE(QString(coordFrameName(FIFFV_COORD_DEVICE)), QString("Device"));
}

void TestToolInfoHelpers::testCoordFrameUnknown()
{
    QCOMPARE(QString(coordFrameName(FIFFV_COORD_UNKNOWN)), QString("Unknown"));
}

void TestToolInfoHelpers::testCoordFrameOther()
{
    QCOMPARE(QString(coordFrameName(12345)), QString("Other"));
}

//=============================================================================================================
// spaceTypeName tests
//=============================================================================================================

void TestToolInfoHelpers::testSpaceTypeSurface()
{
    QCOMPARE(QString(spaceTypeName(FIFFV_MNE_SPACE_SURFACE)), QString("Surface"));
}

void TestToolInfoHelpers::testSpaceTypeVolume()
{
    QCOMPARE(QString(spaceTypeName(FIFFV_MNE_SPACE_VOLUME)), QString("Volume"));
}

void TestToolInfoHelpers::testSpaceTypeDiscrete()
{
    QCOMPARE(QString(spaceTypeName(FIFFV_MNE_SPACE_DISCRETE)), QString("Discrete"));
}

void TestToolInfoHelpers::testSpaceTypeUnknown()
{
    QCOMPARE(QString(spaceTypeName(99999)), QString("Unknown"));
}

//=============================================================================================================
// tagName tests
//=============================================================================================================

void TestToolInfoHelpers::testTagNameFileId()
{
    QCOMPARE(tagName(FIFF_FILE_ID), QString("FILE_ID"));
}

void TestToolInfoHelpers::testTagNameDirPointer()
{
    QCOMPARE(tagName(FIFF_DIR_POINTER), QString("DIR_POINTER"));
}

void TestToolInfoHelpers::testTagNameDir()
{
    QCOMPARE(tagName(FIFF_DIR), QString("DIR"));
}

void TestToolInfoHelpers::testTagNameBlockId()
{
    QCOMPARE(tagName(FIFF_BLOCK_ID), QString("BLOCK_ID"));
}

void TestToolInfoHelpers::testTagNameBlockStart()
{
    QCOMPARE(tagName(FIFF_BLOCK_START), QString("BLOCK_START"));
}

void TestToolInfoHelpers::testTagNameBlockEnd()
{
    QCOMPARE(tagName(FIFF_BLOCK_END), QString("BLOCK_END"));
}

void TestToolInfoHelpers::testTagNameFreeList()
{
    QCOMPARE(tagName(FIFF_FREE_LIST), QString("FREE_LIST"));
}

void TestToolInfoHelpers::testTagNameNChan()
{
    QCOMPARE(tagName(FIFF_NCHAN), QString("NCHAN"));
}

void TestToolInfoHelpers::testTagNameSFreq()
{
    QCOMPARE(tagName(FIFF_SFREQ), QString("SFREQ"));
}

void TestToolInfoHelpers::testTagNameChInfo()
{
    QCOMPARE(tagName(FIFF_CH_INFO), QString("CH_INFO"));
}

void TestToolInfoHelpers::testTagNameMeasDate()
{
    QCOMPARE(tagName(FIFF_MEAS_DATE), QString("MEAS_DATE"));
}

void TestToolInfoHelpers::testTagNameCoordTrans()
{
    QCOMPARE(tagName(FIFF_COORD_TRANS), QString("COORD_TRANS"));
}

void TestToolInfoHelpers::testTagNameNave()
{
    QCOMPARE(tagName(FIFF_NAVE), QString("NAVE"));
}

void TestToolInfoHelpers::testTagNameFirstSample()
{
    QCOMPARE(tagName(FIFF_FIRST_SAMPLE), QString("FIRST_SAMPLE"));
}

void TestToolInfoHelpers::testTagNameLastSample()
{
    QCOMPARE(tagName(FIFF_LAST_SAMPLE), QString("LAST_SAMPLE"));
}

void TestToolInfoHelpers::testTagNameComment()
{
    QCOMPARE(tagName(FIFF_COMMENT), QString("COMMENT"));
}

void TestToolInfoHelpers::testTagNameUnknown()
{
    // Unknown tags should return TAG_<number>
    QCOMPARE(tagName(99999), QString("TAG_99999"));
}

//=============================================================================================================
// printTransform tests
//=============================================================================================================

void TestToolInfoHelpers::testPrintTransformIdentity()
{
    FiffCoordTrans t;
    t.from = FIFFV_COORD_MRI;
    t.to = FIFFV_COORD_HEAD;
    t.trans = Matrix4f::Identity();

    // Should not crash, just prints to stdout
    printTransform(t);
}

void TestToolInfoHelpers::testPrintTransformDoesNotCrash()
{
    FiffCoordTrans t;
    t.from = FIFFV_COORD_DEVICE;
    t.to = FIFFV_COORD_HEAD;
    t.trans = Matrix4f::Zero();
    t.trans(0, 0) = 0.5f;
    t.trans(1, 1) = 0.5f;
    t.trans(2, 2) = 0.5f;
    t.trans(0, 3) = 0.1f;
    t.trans(1, 3) = 0.2f;
    t.trans(2, 3) = 0.3f;
    t.trans(3, 3) = 1.0f;

    printTransform(t);
}

//=============================================================================================================

void TestToolInfoHelpers::cleanupTestCase()
{
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestToolInfoHelpers)
#include "test_tool_info_helpers.moc"
