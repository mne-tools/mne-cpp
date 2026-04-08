//=============================================================================================================
/**
 * @file     test_tool_info_helpers.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     June, 2026
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
