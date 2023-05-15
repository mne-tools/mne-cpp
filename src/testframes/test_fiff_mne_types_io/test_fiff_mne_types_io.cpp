//=============================================================================================================
/**
 * @file     test_fiff_mne_types_io.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     December, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Christoph Dinh, Gabriel B Motta, Lorenz Esch. All rights reserved.
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
 * @brief    The fiff mne types io unit test
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/applicationlogger.h>

#include <fiff/fiff.h>
#include "fiff_types_ref.h"
#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;

//=============================================================================================================
/**
 * DECLARE CLASS TestFiffRWR
 *
 * @brief The TestFiffRWR class provides read write read fiff verification tests
 *
 */
class TestFiffMneTypesIO: public QObject
{
    Q_OBJECT

public:
    TestFiffMneTypesIO();

private slots:
    void initTestCase();
    void checkFiffCoordTrans();
    void cleanupTestCase();

private:
    double dEpsilon;

    QString sRawName;
    QString sEvokedName;
};

//=============================================================================================================

TestFiffMneTypesIO::TestFiffMneTypesIO()
: dEpsilon(0.000001)
, sRawName(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif")
, sEvokedName(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif")
{
}

//=============================================================================================================

void TestFiffMneTypesIO::initTestCase()
{
    qInstallMessageHandler(UTILSLIB::ApplicationLogger::customLogWriter);
    qDebug() << "Epsilon" << dEpsilon;
    qDebug() << "Raw File Name" << sRawName;
    qDebug() << "Evoked File Name" << sEvokedName;
}

//=============================================================================================================

void TestFiffMneTypesIO::checkFiffCoordTrans()
{
    QFile file(sEvokedName);
    FiffStream::SPtr stream(new FiffStream(&file));
    if(!stream->open())
        QFAIL("Failed to open data file.");

    FiffTag::SPtr t_pTag;
    fiffCoordTransRec_REF refTransReference;
    FiffCoordTrans transTest;

    bool bTransFound = false;

    for (int k = 0; k < stream->dir().size(); k++) {
        if(stream->dir()[k]->kind == FIFF_COORD_TRANS) {
            if(!stream->read_tag(t_pTag, stream->dir()[k]->pos))
                QFAIL("Failed to read FIFF_COORD_TRANS tag.");
            refTransReference = *(fiffCoordTrans_REF)t_pTag->data();
            transTest = t_pTag->toCoordTrans();

            //CHECKS
            QVERIFY(refTransReference.from == transTest.from);
            QVERIFY(refTransReference.to == transTest.to);
            //Check rot
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
//                    printf("rot %f == %f ", refTransReference.rot[i][j], transTest.trans(i,j));
                    QVERIFY(refTransReference.rot[i][j] == transTest.trans(i,j));
                }
            }
            //Check move
            for (int i = 0; i < 3; ++i) {
//                    printf("move %f == %f ", refTransReference.move[i], transTest.trans(i,3));
                    QVERIFY(refTransReference.move[i] == transTest.trans(i,3));
            }
            //Check invrot
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
//                    printf("invrot %f == %f ", refTransReference.invrot[i][j], transTest.invtrans(i,j));
                    QVERIFY(refTransReference.invrot[i][j] == transTest.invtrans(i,j));
                }
            }
            //Check invmove
            for (int i = 0; i < 3; ++i) {
//                    printf("invmove %f == %f ", refTransReference.invmove[i], transTest.invtrans(i,3));
                    QVERIFY(refTransReference.invmove[i] == transTest.invtrans(i,3));
            }

            bTransFound = true;
        }
    }

    if(!bTransFound)
        QFAIL("No FIFF_COORD_TRANS found.");

    stream->close();
}

//=============================================================================================================

void TestFiffMneTypesIO::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestFiffMneTypesIO)
#include "test_fiff_mne_types_io.moc"
