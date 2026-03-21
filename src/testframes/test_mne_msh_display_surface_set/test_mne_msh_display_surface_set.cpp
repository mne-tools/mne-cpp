//=============================================================================================================
/**
 * @file     test_mne_msh_display_surface_set.cpp
 * @author   Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     April, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Gabriel B Motta, Lorenz Esch. All rights reserved.
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
 * @brief    Test for I/O of a MNEMshDisplaySurfaceSet
 *
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
