//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     test_fiff_coord_trans.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.6
 * @date     September, 2020
 * @brief     Testframe for FiffCoordTrans.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/mne_logger.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_dig_point.h>
#include <fiff/fiff_dig_point_set.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QtTest>
#include <QTemporaryDir>

//=============================================================================================================
// Eigen
//=============================================================================================================

#include <Eigen/Dense>

//=============================================================================================================
// Used Namespaces
//=============================================================================================================

using namespace FIFFLIB;

//=============================================================================================================
/**
 * DECLARE CLASS TestFiffCoordTrans
 *
 * @brief The TestFiffCoordTrans class provides read write read fiff verification tests
 *
 */
class TestFiffCoordTrans : public QObject
{
    Q_OBJECT

public:
    TestFiffCoordTrans();

private slots:
    void initTestCase();
    void compareTrans();
    void transformsMatchMnePython();
    void transformToHeadMatchesMnePython();

private:
    QString transPath() const;

    QTemporaryDir m_tempDir;
    FiffCoordTrans m_transRef;
    FiffCoordTrans m_transTest;
};

//=============================================================================================================

TestFiffCoordTrans::TestFiffCoordTrans()
{
}

//=============================================================================================================

void TestFiffCoordTrans::initTestCase()
{
    qInstallMessageHandler(UTILSLIB::MNELogger::customLogWriter);

    QVERIFY(m_tempDir.isValid());
    QFile fileTransRef(transPath());
    QFile fileTransTest(m_tempDir.filePath("test-trans.fif"));

    // read reference
    m_transRef = FiffCoordTrans(fileTransRef);

    // write to test file
    m_transRef.write(fileTransTest);

    // read test again
    m_transTest = FiffCoordTrans(fileTransTest);
}

//=============================================================================================================

QString TestFiffCoordTrans::transPath() const
{
    return QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/all-trans.fif";
}

//=============================================================================================================

void TestFiffCoordTrans::compareTrans()
{
    // write_trans: what FiffCoordTrans::write stores reads back unchanged.
    QVERIFY(m_transRef == m_transTest);
}

//=============================================================================================================

void TestFiffCoordTrans::transformsMatchMnePython()
{
    // Reference values produced by mne.read_trans(all-trans.fif) (meg -> head, the first transform),
    // mne.transforms.invert_transform of it, and mne.head_to_mri(pos, "sample", mri_head_t, kind="mri")
    // with the mri -> head transform of the same file (mne 1.11).
    const double readTrans[12] = {9.9141997098922729e-01, -3.9936363697052002e-02, -1.2446729093790054e-01, -6.1293090693652630e-03,
                                  6.0661152005195618e-02, 9.8401170969009399e-01, 1.6745623946189880e-01, 6.3605606555938721e-05,
                                  1.1578965932130814e-01, -1.7356973886489868e-01, 9.7799116373062134e-01, 6.4741522073745728e-02};
    const double invTrans[12] = {0.9914193955587981, 0.06066112332867531, 0.11578959703722973, -0.00142353724729645,
                                 -0.03993634163644685, 0.9840113043178268, -0.17356966725036418, 0.01092979362676358,
                                 -0.12446723340628951, 0.167456169748572, 0.9779907586670634, -0.06409015958396738};
    QCOMPARE(m_transRef.from, FIFFV_COORD_DEVICE);
    QCOMPARE(m_transRef.to, FIFFV_COORD_HEAD);
    const FiffCoordTrans inv = m_transRef.inverted();
    QCOMPARE(inv.from, FIFFV_COORD_HEAD);
    QCOMPARE(inv.to, FIFFV_COORD_DEVICE);
    for (int k = 0; k < 12; ++k) {
        QVERIFY(std::abs(m_transRef.trans(k / 4, k % 4) - readTrans[k]) < 1e-7);
        QVERIFY(std::abs(inv.trans(k / 4, k % 4) - invTrans[k]) < 1e-6);
    }

    const FiffCoordTrans mriHead = FiffCoordTrans::readTransform(transPath(), FIFFV_COORD_MRI, FIFFV_COORD_HEAD);
    QCOMPARE(mriHead.from, FIFFV_COORD_MRI);
    Eigen::MatrixX3f pos(3, 3);
    pos << 0.01f, -0.02f, 0.05f, -0.07f, 0.03f, 0.04f, 0.0f, 0.09f, -0.01f;
    // head_to_mri = head -> MRI in mm
    const Eigen::MatrixX3f mri = mriHead.apply_inverse_trans(pos) * 1000.0f;
    const double expected[9] = {13.55253029986492, -33.996522836108035, 1.0136428961476027, -66.10321186244052, 11.655015359808171,
                                24.907160211429286, 2.8693068357474707, 90.24865002689394, 16.793511554342192};
    for (int k = 0; k < 9; ++k) {
        QVERIFY2(std::abs(mri(k / 3, k % 3) - expected[k]) < 1e-3, qPrintable(QString("head_to_mri %1: %2").arg(k).arg(mri(k / 3, k % 3))));
    }
}

//=============================================================================================================

void TestFiffCoordTrans::transformToHeadMatchesMnePython()
{
    // Reference values produced by mne.channels.transform_to_head(make_dig_montage(ch_pos={E1, E2},
    // nasion, lpa, rpa, coord_frame="unknown")) (mne 1.11).
    auto point = [](int kind, int ident, float x, float y, float z) {
        FiffDigPoint p;
        p.kind = kind;
        p.ident = ident;
        p.coord_frame = FIFFV_COORD_UNKNOWN;
        p.r[0] = x;
        p.r[1] = y;
        p.r[2] = z;
        return p;
    };
    FiffDigPointSet dig;
    dig << point(FIFFV_POINT_CARDINAL, FIFFV_POINT_LPA, -0.071f, 0.008f, -0.013f)
        << point(FIFFV_POINT_CARDINAL, FIFFV_POINT_NASION, 0.012f, 0.105f, 0.021f)
        << point(FIFFV_POINT_CARDINAL, FIFFV_POINT_RPA, 0.077f, 0.011f, -0.004f)
        << point(FIFFV_POINT_EEG, 1, 0.02f, 0.05f, 0.09f)
        << point(FIFFV_POINT_EEG, 2, -0.06f, -0.03f, 0.07f);
    QVERIFY(dig.transformToHead());

    const double expected[5][3] = {{-8.6855529027317899e-02, 0.0, 0.0},
                                   {0.0, 9.9549570955302175e-02, 0.0},
                                   {6.1448213339488196e-02, 0.0, 0.0},
                                   {0.01105838580892796, 0.06654609421407334, 0.08147688399964782},
                                   {-0.07160979103098487, -0.01280946393687563, 0.08976349540095378}};
    for (int i = 0; i < 5; ++i) {
        QCOMPARE(dig[i].coord_frame, FIFFV_COORD_HEAD);
        for (int c = 0; c < 3; ++c) {
            QVERIFY2(std::abs(dig[i].r[c] - expected[i][c]) < 1e-6, qPrintable(QString("point %1 axis %2: %3").arg(i).arg(c).arg(dig[i].r[c])));
        }
    }

    FiffDigPointSet noFiducials;
    noFiducials << point(FIFFV_POINT_EEG, 1, 0.02f, 0.05f, 0.09f);
    QVERIFY(!noFiducials.transformToHead());
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestFiffCoordTrans)
#include "test_fiff_coord_trans.moc"
