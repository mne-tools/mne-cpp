//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     test_mne_msh_display_surface_set.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     April, 2017
 * @brief    Test for I/O of a MNEMshDisplaySurfaceSet
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/mne_logger.h>

#include <mne/mne_msh_display_surface_set.h>
#include <mne/mne_msh_display_surface.h>
#include <mne/mne_surface.h>

#include <fiff/fiff_file.h>

#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;

//=============================================================================================================
/**
 * DECLARE CLASS TestMneMshDisplaySurfaceSet
 *
 * @brief The TestMneMshDisplaySurfaceSet class provides display surface set reading verification tests
 *
 */
class TestMneMshDisplaySurfaceSet: public QObject
{
    Q_OBJECT

public:
    TestMneMshDisplaySurfaceSet();

private slots:
    void initTestCase();
    void compareSurface();
    void cleanupTestCase();

private:
    double dEpsilon;

    MNEMshDisplaySurfaceSet::SPtr m_pSurfSetBemLoaded;
};

//=============================================================================================================

TestMneMshDisplaySurfaceSet::TestMneMshDisplaySurfaceSet()
: dEpsilon(0.000001)
{
}

//=============================================================================================================

void TestMneMshDisplaySurfaceSet::initTestCase()
{
    qInstallMessageHandler(UTILSLIB::MNELogger::customLogWriter);
    //qDebug() << "dEpsilon" << dEpsilon;

    //Read the results produced with MNE-CPP
    //Calculate the alignment of the fiducials
    m_pSurfSetBemLoaded = MNEMshDisplaySurfaceSet::SPtr(new MNEMshDisplaySurfaceSet());
    m_pSurfSetBemLoaded->add_bem_surface(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/subjects/sample/bem/sample-5120-bem.fif",
                                             FIFFV_BEM_SURF_ID_BRAIN,
                                             "5120",
                                             1,
                                             1);

    QVERIFY( m_pSurfSetBemLoaded->nsurf == 1 );
}

//=============================================================================================================

void TestMneMshDisplaySurfaceSet::compareSurface()
{
    if(m_pSurfSetBemLoaded->nsurf >= 1) {
        QVERIFY( m_pSurfSetBemLoaded->surfs[0]->np == 2562 );
        QVERIFY( m_pSurfSetBemLoaded->surfs[0]->ntri == 5120 );
    }
}

//=============================================================================================================

void TestMneMshDisplaySurfaceSet::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestMneMshDisplaySurfaceSet)
#include "test_mne_msh_display_surface_set.moc"
