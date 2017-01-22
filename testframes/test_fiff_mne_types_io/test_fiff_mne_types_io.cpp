//=============================================================================================================
/**
* @file     main.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     December, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Christoph Dinh and Matti Hamalainen. All rights reserved.
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


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>


//*************************************************************************************************************
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
    double epsilon;

    QString rawName;
    QString evokedName;
};


//*************************************************************************************************************

TestFiffMneTypesIO::TestFiffMneTypesIO()
: epsilon(0.000001)
, rawName("./mne-cpp-test-data/MEG/sample/sample_audvis_raw_short.fif")
, evokedName("./mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif")
{
}


//*************************************************************************************************************

void TestFiffMneTypesIO::initTestCase()
{
    qDebug() << "Epsilon" << epsilon;
    qDebug() << "Raw File Name" << rawName;
    qDebug() << "Evoked File Name" << evokedName;
}

//*************************************************************************************************************

void TestFiffMneTypesIO::checkFiffCoordTrans()
{
    QFile file(evokedName);
    FiffStream::SPtr stream(new FiffStream(&file));
    if(!stream->open())
        QFAIL("Failed to open data file.");

    FiffTag::SPtr t_pTag;
    fiffCoordTransRec reference_trans;
    FiffCoordTrans trans;

    bool trans_found = false;

    for (int k = 0; k < stream->dir().size(); k++) {
        if(stream->dir()[k]->kind == FIFF_COORD_TRANS) {
            if(!FiffTag::read_tag(stream, t_pTag, stream->dir()[k]->pos))
                QFAIL("Failed to read FIFF_COORD_TRANS tag.");
            reference_trans = *(fiffCoordTrans)t_pTag->data();
            trans = t_pTag->toCoordTrans();

            //CHECKS
            QVERIFY(reference_trans.from == trans.from);
            QVERIFY(reference_trans.to == trans.to);

            qDebug() << "Some more checks required";

            trans_found = true;
        }
    }

    if(!trans_found)
        QFAIL("No FIFF_COORD_TRANS found.");

    stream->device()->close();
}

//*************************************************************************************************************

void TestFiffMneTypesIO::cleanupTestCase()
{
}


//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_APPLESS_MAIN(TestFiffMneTypesIO)
#include "test_fiff_mne_types_io.moc"
