//=============================================================================================================
/**
* @file     fiffanonymizer.h
* @author   Juan Garcia-Prieto <Juan.GarciaPrieto@uth.tmc.edu> <juangpc@gmail.com>;
*           Wayne Mead <wayne.mead@uth.tmc.edu> <wayne.isk@gmail.com>;
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*           John C. Mosher <John.C.Mosher@uth.tmc.edu> <jcmosher@gmail.com>;
* @version  1.0
* @date     August, 2019
*
* @section  LICENSE
*
* Copyright (C) 2019, Juan Garcia-Prieto and Matti Hamalainen. All rights reserved.
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
* @brief     fiffanonymizer class declaration.
*
*/

#ifndef FIFFANONYMIZER_H
#define FIFFANONYMIZER_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>
#include <fiff/fiff_types.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QStack>
#include <QDateTime>
#include <QDebug>
#include <QPointer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE
//=============================================================================================================

namespace MNEANONYMIZE
{


//=============================================================================================================
/**
* Description of what this class is intended to do (in detail).
*
* @brief FiffAnonymizer class declaration.
*/

class FiffAnonymizer
{
public:
    typedef QSharedPointer<FiffAnonymizer> SPtr;            /**< Shared pointer type for FiffAnonymizer. */
    typedef QSharedPointer<const FiffAnonymizer> ConstSPtr; /**< Const shared pointer type for FiffAnonymizer. */

    /**
     * Constructs a FiffAnonymizer object.
     */
    FiffAnonymizer();
    FiffAnonymizer(const FiffAnonymizer& obj);
    FiffAnonymizer(FiffAnonymizer &&obj);

    int anonymizeFile();
    void setFileIn(const QString sFilePathIn);
    void setFileOut(const QString sFilePathOut);
    void setVerboseMode(bool v);
    bool getVerboseMode();
    void setBruteMode(bool b);
    void setQuietMode(bool q);
    void setMeasurementDay(QString d);
    void setMeasurementDayOffset(int d);
    void setMeasurementDayOffset(bool b);
    void setSubjectBirthday(QString d);
    void setSubjectBirthdayOffset(int d);
    void setSubjectBirthdayOffset(bool b);
    void setDeleteInputFileAfter(bool d);
    void setDeleteInputFileAfterConfirmation(bool dc);

    const double version;
    const double maxValidFiffVerion;
    const QString versionStr;
    const QString name;
    const QString description;

private:
    void updateBlockTypeList(FIFFLIB::FiffTag::SPtr pTag);
    bool checkValidFiffFormatVersion(FIFFLIB::FiffTag::SPtr pTag);
    int censorTag(FIFFLIB::FiffTag::SPtr pInTag,FIFFLIB::FiffTag::SPtr pOutTag);
    void addEntryToDir(FIFFLIB::FiffTag::SPtr pTag,qint64 filePos);
    void addFinalEntryToDir();
    void dir2tag(FIFFLIB::FiffTag::SPtr pTag);
    void writeDirectory(QPointer<FIFFLIB::FiffStream> stream, FIFFLIB::fiff_long_t pos=-1);
    void updatePointer(QPointer<FIFFLIB::FiffStream> stream, FIFFLIB::fiff_int_t tagKind, FIFFLIB::fiff_long_t newPos);
    void printIfVerbose(const QString str,bool sameLine=false);
    QString generateRandomFileName();
    void deleteInputFile();
    bool checkDeleteInputFile();
    bool checkRenameOutputFile();
    void renameOutputFile();

    bool m_bVerboseMode;
    bool m_bBruteMode;
    bool m_bQuietMode;
    bool m_bDeleteInputFileAfter;
    bool m_bDeleteInputFileConfirmation;
    bool m_bInputFileDeleted;
    bool m_bInOutFileNamesEqual;
    bool m_bOutputFileRenamed;

    QString m_sDfltString;
    QDateTime m_dateDfltDate;
    int  m_iMeasurementDayOffset;
    bool m_bUseMeasurementDayOffset;
    int  m_iSubjectBirthdayOffset;
    bool m_bUseSubjectBirthdayOffset;

    QDateTime m_dateMeasurmentDate;
    QDateTime m_date_IdDate;
    QByteArray m_BDfltMAC;

    int m_iDfltSubjectId;
    QString m_sDfltSubjectFirstName;
    QString m_sDfltSubjectMidName;
    QString m_sDfltSubjectLastName;
    QDateTime m_dateSubjectBirthDay;
    int m_iDfltSubjectWeight;
    int m_iDfltSubjectHeight;
    QString m_sDfltSubjectComment;
    QString m_sDfltSubjectHisId;

    int m_iDfltProjectId;
    QString m_sDfltProjectName;
    QString m_sDfltProjectAim;
    QString m_sDfltProjectPersons;
    QString m_sDfltProjectComment;

    QString m_sFileNameIn;
    QString m_sFileNameOut;

    QFile m_fFileIn;
    QFile m_fFileOut;

    QDebug m_printInSameLineHelper;
    const bool m_bPrintInSameLine;

    QSharedPointer<QStack<int32_t> > m_pBlockTypeList;
    QSharedPointer<QVector<FIFFLIB::FiffDirEntry> > m_pOutDir;

};



//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline void FiffAnonymizer::printIfVerbose(const QString str, bool sameLine)
{
    if(m_bVerboseMode)
    {
        QDebug * dbg;
        if(!sameLine)
        {
            m_printInSameLineHelper = qDebug();
        }
        dbg = &m_printInSameLineHelper;
        *dbg << str;
    }
}

}

#endif // MNEANONYMIZE
