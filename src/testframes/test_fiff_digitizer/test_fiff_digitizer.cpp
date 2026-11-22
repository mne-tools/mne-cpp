//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     test_fiff_digitizer.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     April, 2017
 * @brief    Test for I/O of a FiffDigitizerData
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/log.h>
#include <mne/fiff/fiff_dig_point.h>
#include <mne/fiff/fiff_digitizer_data.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QFile>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <stdexcept>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
/**
 * DECLARE CLASS TestFiffDigitizer
 *
 * @brief The TestFiffDigitizer class provides digitizer data reading verification tests
 *
 */
class TestFiffDigitizer : public QObject
{
    Q_OBJECT

public:
    TestFiffDigitizer();

private slots:
    void initTestCase();
    void comparePoints();
    void compareCoordFrame();
    void compareNPoint();
    void cleanupTestCase();

private:
    double m_dEpsilon;
    double m_dSumPointsDigDataResult;
    int m_iCoordFrameDigDataResult;
    int m_iNPointDigDataResult;

    FiffDigitizerData digDataLoaded;
};

//=============================================================================================================

TestFiffDigitizer::TestFiffDigitizer()
: m_dEpsilon(1.0e-04)
{
}

//=============================================================================================================

void TestFiffDigitizer::initTestCase()
{
    //Read the results produced with MNE-CPP
    const QString dataDir = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/";
    digDataLoaded = FiffDigitizerData((dataDir + "sample_audvis_trunc_raw.fif").toStdString());
    QVERIFY_THROWS_EXCEPTION(mne::Error, FiffDigitizerData((dataDir + "missing.fif").toStdString()));
    QCOMPARE(FiffDigitizerData::read((dataDir + "sample_audvis-cov.fif").toStdString()).error().code, mne::ErrorCode::NotFound);
    // Copies keep the source file and the transforms
    digDataLoaded.head_mri_t_adj = FiffCoordTrans((dataDir + "all-trans.fif").toStdU16String(), FIFFV_COORD_HEAD, FIFFV_COORD_MRI);
    const FiffDigitizerData copy = digDataLoaded;
    QCOMPARE(copy.filename, digDataLoaded.filename);
    QVERIFY(!copy.filename.empty());
    QVERIFY(copy.head_mri_t_adj.has_value());
    //Prepare reference result
    m_iCoordFrameDigDataResult = FIFFV_COORD_HEAD;
    m_iNPointDigDataResult = 146;
    m_dSumPointsDigDataResult = 13.6212;
}

//=============================================================================================================

void TestFiffDigitizer::comparePoints()
{
    double dSum = 0.0;
    for (int i = 0; i < std::ssize(digDataLoaded.points); ++i) {
        dSum += digDataLoaded.points[i].r[0];
        dSum += digDataLoaded.points[i].r[1];
        dSum += digDataLoaded.points[i].r[2];
    }

    double dDiff = dSum - m_dSumPointsDigDataResult;

    QVERIFY(dDiff < m_dEpsilon);
}

//=============================================================================================================

void TestFiffDigitizer::compareCoordFrame()
{
    QVERIFY(m_iCoordFrameDigDataResult == digDataLoaded.coord_frame);
}

//=============================================================================================================

void TestFiffDigitizer::compareNPoint()
{
    QCOMPARE(static_cast<int>(digDataLoaded.points.size()), m_iNPointDigDataResult);
}

//=============================================================================================================

void TestFiffDigitizer::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestFiffDigitizer)
#include "test_fiff_digitizer.moc"
