//=============================================================================================================
/**
* @file     test_edf2fiff_rwr.cpp
* @author   Simon Heinke <simon.heinke@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     August, 2019
*
* @section  LICENSE
*
* Copyright (C) 2019, Simon Heinke and Matti Hamalainen. All rights reserved.
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
* @brief    Converts the EDF file into a Fiff file and tests whether the raw values are identical.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <algorithm>

#include <fiff/fiff.h>
#include "../../applications/mne_edf2fiff/edf_raw_data.h"

#include <utils/generics/applicationlogger.h>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QProcess>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace EDF2FIFF;
using namespace Eigen;

//=============================================================================================================
/**
* DECLARE CLASS TestEDF2FIFFRWR
*
* @brief The TestEDF2FIFFRWR class performs an EDF-to-Fiff conversion and compares the converted data with
*        the original data from the EDF file.
*
*/
class TestEDF2FIFFRWR: public QObject
{
    Q_OBJECT

public:
    TestEDF2FIFFRWR();

private slots:
    void initTestCase();
    void testEDF2FiffConversion();
    void testEDFReadAndFiffWrite();
    void testFiffReadingAndValueEquality();
    void cleanupTestCase();

private:
    const float m_fTimesliceSeconds = 10.0f; //read and write in 10 sec chunks
    const float m_fEpsilon = 0.000000001f;

    // files:
    QFile* m_pFileIn;
    QFile* m_pFileOut;  // temporary outfile, to be deleted during cleanup

    // EDF / Fiff containers:
    EDFRawData* m_pEDFRaw;
    FiffRawData* m_pFiffRaw;

    // raw data:
    QVector<MatrixXd> m_vRawChunksFromOriginalEDF;
    QVector<MatrixXd> m_vRawChunksFromWrittenFIFF;
};

//*************************************************************************************************************

TestEDF2FIFFRWR::TestEDF2FIFFRWR()
{

}

//*************************************************************************************************************

void TestEDF2FIFFRWR::initTestCase()
{
    qInstallMessageHandler(UTILSLIB::ApplicationLogger::customLogWriter);

    m_pFileIn = new QFile(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/EEG/test_reduced.edf");
    m_pFileOut = new QFile(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/EEG/test_reduced_temporary.fif");

    // initialize EDF raw data
    m_pEDFRaw = new EDFRawData(m_pFileIn);

    QVERIFY(m_pEDFRaw->getInfo().getAllChannelInfos().size() != 0);
}

//*************************************************************************************************************

void TestEDF2FIFFRWR::testEDF2FiffConversion()
{
    // convert to fiff
    m_pFiffRaw = new FiffRawData(m_pEDFRaw->toFiffRawData());

    QVERIFY(m_pEDFRaw->getInfo().getMeasurementChannelInfos().size() == m_pFiffRaw->info.nchan);
    QVERIFY(std::abs(m_pEDFRaw->getInfo().getFrequency() - m_pFiffRaw->info.sfreq) <= m_fEpsilon);  // float-comparisons via '==' are unsafe
    QVERIFY(m_pEDFRaw->getInfo().getSampleCount() == m_pFiffRaw->last_samp - m_pFiffRaw->first_samp);
}

//*************************************************************************************************************

void TestEDF2FIFFRWR::testEDFReadAndFiffWrite()
{
    // set up the reading parameters
    int iTimesliceSamples = static_cast<int>(ceil(m_fTimesliceSeconds * m_pFiffRaw->info.sfreq));

    RowVectorXd cals;
    FiffStream::SPtr outfid = FiffStream::start_writing_raw(*m_pFileOut, m_pFiffRaw->info, cals);

    // write start of Fiff file
    fiff_int_t first = 0;  // EDF files start at index 0
    outfid->write_int(FIFF_FIRST_SAMPLE, &first);

    // read chunks, remember how many samples were already read
    int iSamplesRead = 0;
    while(iSamplesRead < m_pEDFRaw->getInfo().getSampleCount()) {
        int iNextChunkSize = std::min(iTimesliceSamples, m_pEDFRaw->getInfo().getSampleCount() - iSamplesRead);
        // EDF sample indexing starts at 0, simply use samplesRead as argument to read_raw_segment
        MatrixXd data = m_pEDFRaw->read_raw_segment(iSamplesRead, iSamplesRead + iNextChunkSize).cast<double>();
        iSamplesRead += iNextChunkSize;
        outfid->write_raw_buffer(data, cals);
        // copy into vector for later comparison with written Fiff file
        m_vRawChunksFromOriginalEDF.append(data);
    }

    outfid->finish_writing_raw();

    QVERIFY(iSamplesRead == m_pEDFRaw->getInfo().getSampleCount());
}

//*************************************************************************************************************

void TestEDF2FIFFRWR::testFiffReadingAndValueEquality()
{
    // close outfile in case it is still open
    m_pFileOut->close();
    // open again
    FiffRawData writtenFiff(*m_pFileOut);

    // set up the reading parameters
    int iTimesliceSamples = static_cast<int>(ceil(m_fTimesliceSeconds * writtenFiff.info.sfreq));
    // read chunks, remember which is the current sample
    int iCurrentSample = writtenFiff.first_samp;
    while(iCurrentSample < writtenFiff.last_samp) {
        // timeslice_samples - 1 because FiffRawData.read_raw_segment has inclusive index arguments
        int iNextChunkSize = std::min(iTimesliceSamples - 1, writtenFiff.last_samp - iCurrentSample);
        MatrixXd data, times;
        writtenFiff.read_raw_segment(data, times, iCurrentSample, iCurrentSample + iNextChunkSize);
        iCurrentSample += iNextChunkSize + 1;  // + 1 because chunkSize of inclusive index arguments (see above)
        // copy into vector for later comparison with original EDF file
        m_vRawChunksFromWrittenFIFF.append(data);
    }

    // compare chunk vectors
    QVERIFY(m_vRawChunksFromOriginalEDF.size() == m_vRawChunksFromWrittenFIFF.size());
    for(int i = 0; i < m_vRawChunksFromOriginalEDF.size(); ++i) {
        const MatrixXd originalEDFChunk = m_vRawChunksFromOriginalEDF[i];
        const MatrixXd writtenFiffChunk = m_vRawChunksFromWrittenFIFF[i];

        if(originalEDFChunk.cols() != writtenFiffChunk.cols() || originalEDFChunk.rows() != writtenFiffChunk.rows()) {
            QFAIL("Found a chunk with mismatching dimensions ...");
        }
        // compare every single raw value
        for(int r = 0; r < originalEDFChunk.rows(); ++r) {
            for(int c = 0; c < originalEDFChunk.cols(); ++c) {
                if(std::abs(originalEDFChunk(r, c) - writtenFiffChunk(r, c)) > static_cast<double>(m_fEpsilon)) {  // double-comparisons via '==' are unsafe
                    QFAIL("Found some non-identical raw values ...");
                }
            }
        }
    }
}

//*************************************************************************************************************

void TestEDF2FIFFRWR::cleanupTestCase()
{
    // destroy containers
    delete m_pFiffRaw;
    delete m_pEDFRaw;

    // close files
    m_pFileOut->close();
    m_pFileIn->close();

    // remove temporary outfile
    m_pFileOut->remove();

    // destroy filehandles
    delete m_pFileOut;
    delete m_pFileIn;
}

//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestEDF2FIFFRWR)
#include "test_edf2fiff_rwr.moc"
