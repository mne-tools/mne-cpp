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
#include <fiff/fiff_coord_trans.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QtTest>

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
class TestFiffCoordTrans: public QObject
{
    Q_OBJECT

public:
    TestFiffCoordTrans();

private slots:
    void initTestCase();
    void compareTrans();
    // add other compareFunctions here
    void cleanupTestCase();

private:
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

    // Reference file (Read) and test file (Write)
    QFile fileTransRef(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/all-trans.fif");
    QFile fileTransTest(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/test-trans.fif");

    // read reference
    m_transRef = FiffCoordTrans(fileTransRef);

    // write to test file
    m_transRef.write(fileTransTest);

    // read test again
    m_transTest = FiffCoordTrans(fileTransTest);
}

//=============================================================================================================

void TestFiffCoordTrans::compareTrans()
{
    // compare your data here, think about usefull metrics
    QVERIFY(m_transRef == m_transTest);
}

//=============================================================================================================

void TestFiffCoordTrans::cleanupTestCase()
{
    QFile fileTransTest(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/test-trans.fif");
    fileTransTest.remove();
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestFiffCoordTrans)
#include "test_fiff_coord_trans.moc"

