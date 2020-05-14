//=============================================================================================================
/**
 * @file     fiffanonymizer.cpp
 * @author   Wayne Mead <wayne.mead@uth.tmc.edu>;
 *           Juan Garcia-Prieto <juangpc@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           John C. Mosher <John.C.Mosher@uth.tmc.edu>
 * @since    0.1.0
 * @date     August, 2019
 *
 * @section  LICENSE
 *
 * Copyright (C) 2019, Wayne Mead, Juan Garcia-Prieto, Lorenz Esch, Matti Hamalainen, John C. Mosher. All rights reserved.
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
 * @brief    FiffAnonymizer class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffanonymizer.h"
#include <fiff/fiff_dir_entry.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QStack>
#include <QFileInfo>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEANONYMIZE;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffAnonymizer::FiffAnonymizer()
: m_dMaxValidFiffVerion(1.3)
, m_sDefaultString("mne_anonymize")
, m_sFiffComment(m_sDefaultString)
, m_sFiffExperimenter(m_sDefaultString)
, m_sSubjectFirstName(m_sDefaultString)
, m_sSubjectMidName("mne")
, m_sSubjectLastName(m_sDefaultString)
, m_sSubjectComment(m_sDefaultString)
, m_sSubjectHisId(m_sDefaultString)
, m_iSubjectBirthdayOffset(0)
, m_iDfltSubjectSex(0)
, m_iDfltSubjectId(0)
, m_iDfltSubjectHand(0)
, m_fSubjectWeight(0.0)
, m_fSubjectHeight(0.0)
, m_iProjectId(0)
, m_sProjectName(m_sDefaultString)
, m_sProjectAim(m_sDefaultString)
, m_sProjectPersons(m_sDefaultString)
, m_sProjectComment(m_sDefaultString)
, m_bFileInSet(false)
, m_bFileOutSet(false)
, m_dDefaultDate(QDateTime(QDate(2000,1,1), QTime(1, 1, 0), Qt::UTC))
, m_dMeasurementDate(m_dDefaultDate)
, m_dSubjectBirthday(m_dDefaultDate)
, m_iMeasurementDayOffset(0)
, m_bUseMeasurementDayOffset(false)
, m_bUseSubjectBirthdayOffset(false)
, m_bVerboseMode(false)
, m_bBruteMode(false)
, m_pTag(FIFFLIB::FiffTag::SPtr::create())
{
    //MAC addresses have 6 bytes. We use 2 more here to complete 2 int32 (2bytes) reads.
    //check->sometimes MAC address is stored in the 0-5 bytes some other times it
    //is stored in the 2-7 bytes. To do -> Check why!!!
    m_BDfltMAC[0] = 0;
    m_BDfltMAC[1] = 0;

    m_pBlockTypeList = QSharedPointer<QStack<int32_t> >(new QStack<int32_t>);
    m_pBlockTypeList->clear();
}

//=============================================================================================================

FiffAnonymizer::FiffAnonymizer(const FiffAnonymizer& obj)
: m_dMaxValidFiffVerion(obj.m_dMaxValidFiffVerion)
, m_sDefaultString(obj.m_sDefaultString)
, m_sFiffComment(m_sDefaultString)
, m_sSubjectFirstName(obj.m_sSubjectFirstName)
, m_sSubjectMidName(obj.m_sSubjectMidName)
, m_sSubjectLastName(obj.m_sSubjectLastName)
, m_sSubjectComment(obj.m_sSubjectComment)
, m_sSubjectHisId(obj.m_sSubjectHisId)
, m_iSubjectBirthdayOffset(obj.m_iSubjectBirthdayOffset)
, m_iDfltSubjectSex(obj.m_iDfltSubjectSex)
, m_iDfltSubjectId(obj.m_iDfltSubjectId)
, m_iDfltSubjectHand(obj.m_iDfltSubjectHand)
, m_fSubjectWeight(obj.m_fSubjectWeight)
, m_fSubjectHeight(obj.m_fSubjectHeight)
, m_iProjectId(obj.m_iProjectId)
, m_sProjectName(obj.m_sProjectName)
, m_sProjectAim(obj.m_sProjectAim)
, m_sProjectPersons(obj.m_sProjectPersons)
, m_sProjectComment(obj.m_sProjectComment)
, m_bFileInSet(obj.m_bFileInSet)
, m_bFileOutSet(obj.m_bFileOutSet)
, m_dDefaultDate(QDateTime(QDate(2000,1,1), QTime(1, 1, 0), Qt::UTC))
, m_dMeasurementDate(obj.m_dMeasurementDate)
, m_dSubjectBirthday(obj.m_dSubjectBirthday)
, m_iMeasurementDayOffset(obj.m_iMeasurementDayOffset)
, m_bUseMeasurementDayOffset(obj.m_bUseMeasurementDayOffset)
, m_bUseSubjectBirthdayOffset(obj.m_bUseSubjectBirthdayOffset)
, m_bVerboseMode(obj.m_bVerboseMode)
, m_bBruteMode(obj.m_bBruteMode)
{
    m_BDfltMAC[0] = obj.m_BDfltMAC[0];
    m_BDfltMAC[1] = obj.m_BDfltMAC[1];

    m_pBlockTypeList = QSharedPointer<QStack<int32_t> >(new QStack<int32_t>(*obj.m_pBlockTypeList));
}

//=============================================================================================================

FiffAnonymizer::FiffAnonymizer(FiffAnonymizer &&obj)
: m_dMaxValidFiffVerion(obj.m_dMaxValidFiffVerion)
, m_sDefaultString(obj.m_sDefaultString)
, m_sFiffComment(m_sDefaultString)
, m_sSubjectFirstName(obj.m_sSubjectFirstName)
, m_sSubjectMidName(obj.m_sSubjectMidName)
, m_sSubjectLastName(obj.m_sSubjectLastName)
, m_sSubjectComment(obj.m_sSubjectComment)
, m_sSubjectHisId(obj.m_sSubjectHisId)
, m_iSubjectBirthdayOffset(obj.m_iSubjectBirthdayOffset)
, m_iDfltSubjectSex(obj.m_iDfltSubjectSex)
, m_iDfltSubjectId(obj.m_iDfltSubjectId)
, m_iDfltSubjectHand(obj.m_iDfltSubjectHand)
, m_fSubjectWeight(obj.m_fSubjectWeight)
, m_fSubjectHeight(obj.m_fSubjectHeight)
, m_iProjectId(obj.m_iProjectId)
, m_sProjectName(obj.m_sProjectName)
, m_sProjectAim(obj.m_sProjectAim)
, m_sProjectPersons(obj.m_sProjectPersons)
, m_sProjectComment(obj.m_sProjectComment)
, m_bFileInSet(obj.m_bFileInSet)
, m_bFileOutSet(obj.m_bFileOutSet)
, m_dDefaultDate(QDateTime(QDate(2000,1,1), QTime(1, 1, 0), Qt::UTC))
, m_dMeasurementDate(obj.m_dMeasurementDate)
, m_dSubjectBirthday(obj.m_dSubjectBirthday)
, m_iMeasurementDayOffset(obj.m_iMeasurementDayOffset)
, m_bUseMeasurementDayOffset(obj.m_bUseMeasurementDayOffset)
, m_bUseSubjectBirthdayOffset(obj.m_bUseSubjectBirthdayOffset)
, m_bVerboseMode(obj.m_bVerboseMode)
, m_bBruteMode(obj.m_bBruteMode)
{
    m_BDfltMAC[0] = obj.m_BDfltMAC[0];
    m_BDfltMAC[1] = obj.m_BDfltMAC[1];

    m_pBlockTypeList.swap(obj.m_pBlockTypeList);
}

//=============================================================================================================

int FiffAnonymizer::anonymizeFile()
{
    printIfVerbose("Max. Valid Fiff version: " + QString::number(m_dMaxValidFiffVerion));
    printIfVerbose("Current date: " + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss.zzz t"));
    printIfVerbose(" ");

    openInOutStreams();

    printIfVerbose("Reading info in the file.");
    processHeaderTags();


    while( (m_pTag->next != -1) && (!m_pInStream->device()->atEnd()))
    {
        readTag();
        censorTag();
        writeTag();
    }

    closeInOutStreams();

    return 0;
}

//=============================================================================================================

void FiffAnonymizer::censorTag() const
{
    switch (m_pTag->kind)
    {
    //all these 'kinds' of tags contain a fileID struct, which contains info related to
    //measurement date
    case FIFF_FILE_ID:
    case FIFF_BLOCK_ID:
    case FIFF_PARENT_FILE_ID:
    case FIFF_PARENT_BLOCK_ID:
    case FIFF_REF_FILE_ID:
    case FIFF_REF_BLOCK_ID:
    {
        FIFFLIB::FiffId inId = m_pTag->toFiffID();
        QDateTime inMeasDate = QDateTime::fromSecsSinceEpoch(inId.time.secs, Qt::LocalTime);
        QDateTime outMeasDate;

        if(m_bUseMeasurementDayOffset)
        {
            outMeasDate = inMeasDate.addDays(-m_iMeasurementDayOffset);
        } else {
            outMeasDate = m_dMeasurementDate;
        }

        FIFFLIB::FiffId outId(inId);
        outId.machid[0] = m_BDfltMAC[0];
        outId.machid[1] = m_BDfltMAC[1];
        outId.time.secs = static_cast<int32_t>(outMeasDate.toSecsSinceEpoch());
        outId.time.usecs = 0;

        const int fiffIdSize(sizeof(inId)/sizeof(FIFFLIB::fiff_int_t));
        FIFFLIB::fiff_int_t outData[fiffIdSize];
        outData[0] = outId.version;
        outData[1] = outId.machid[0];
        outData[2] = outId.machid[1];
        outData[3] = outId.time.secs;
        outData[4] = outId.time.usecs;

        m_pTag->resize(fiffIdSize*sizeof(FIFFLIB::fiff_int_t));
        memcpy(m_pTag->data(),reinterpret_cast<char*>(outData),fiffIdSize*sizeof(FIFFLIB::fiff_int_t));
        printIfVerbose("MAC address in ID tag changed: " + inId.toMachidString() + " -> "  + outId.toMachidString());
        printIfVerbose("Measurement date in ID tag changed: " + inMeasDate.toString("dd.MM.yyyy hh:mm:ss.zzz t") + " -> " + outMeasDate.toString("dd.MM.yyyy hh:mm:ss.zzz t"));
        break;
    }
    case FIFF_MEAS_DATE:
    {
        QDateTime inMeasDate(QDateTime::fromSecsSinceEpoch(*m_pTag->toInt(), Qt::LocalTime));
        QDateTime outMeasDate;

        if(m_bUseMeasurementDayOffset)
        {
            outMeasDate = inMeasDate.addDays(-m_iMeasurementDayOffset);
        } else {
            outMeasDate = m_dMeasurementDate;
        }

        FIFFLIB::fiff_int_t outData[1];
        outData[0] = static_cast<int32_t>(outMeasDate.toSecsSinceEpoch());
        memcpy(m_pTag->data(),reinterpret_cast<char*>(outData),sizeof(FIFFLIB::fiff_int_t));
        printIfVerbose("Measurement date changed: " + inMeasDate.toString("dd.MM.yyyy hh:mm:ss.zzz t") + " -> " + outMeasDate.toString("dd.MM.yyyy hh:mm:ss.zzz t"));
        break;
    }
    case FIFF_COMMENT:
    {
        if(m_pBlockTypeList->top() == FIFFB_MEAS_INFO)
        {
            QString inStr(m_pTag->data());
            QString outStr(m_sDefaultString);
            m_pTag->resize(outStr.size());
            memcpy(m_pTag->data(),outStr.toUtf8(),static_cast<size_t>(outStr.size()));
            printIfVerbose("Description of the measurement block changed: " + inStr + " -> " + outStr);
        }
        break;
    }
    case FIFF_EXPERIMENTER:
    {
        QString inStr(m_pTag->data());
        QString outStr(m_sDefaultString);
        m_pTag->resize(outStr.size());
        memcpy(m_pTag->data(),outStr.toUtf8(),static_cast<size_t>(outStr.size()));
        printIfVerbose("Experimenter changed: " + inStr + " -> " + outStr);
        break;
    }
    case FIFF_SUBJ_ID:
    {
        qint32 oldSubjID(*m_pTag->toInt());
        qint32 newSubjID(m_iDfltSubjectId);
        memcpy(m_pTag->data(),&newSubjID, sizeof(qint32));
        printIfVerbose("Subject ID changed: " + QString::number(oldSubjID) + " -> " + QString::number(newSubjID));
        break;
    }
    case FIFF_SUBJ_FIRST_NAME:
    {
        QString inStr(m_pTag->data());
        QString outStr(m_sSubjectFirstName);
        m_pTag->resize(outStr.size());
        memcpy(m_pTag->data(),outStr.toUtf8(),static_cast<size_t>(outStr.size()));
        printIfVerbose("Subject first name changed: " + inStr + " -> " + outStr);
        break;
    }
    case FIFF_SUBJ_MIDDLE_NAME:
    {
        QString inStr(m_pTag->data());
        QString outStr(m_sSubjectMidName);
        m_pTag->resize(outStr.size());
        memcpy(m_pTag->data(),outStr.toUtf8(),static_cast<size_t>(outStr.size()));
        printIfVerbose("Subject middle name changed: " + inStr + " -> " + outStr);
        break;
    }
    case FIFF_SUBJ_LAST_NAME:
    {
        QString inStr(m_pTag->data());
        QString outStr(m_sSubjectLastName);
        m_pTag->resize(outStr.size());
        memcpy(m_pTag->data(),outStr.toUtf8(),static_cast<size_t>(outStr.size()));
        printIfVerbose("Subject last name changed: " + inStr + " -> " + outStr);
        break;
    }
    case FIFF_SUBJ_BIRTH_DAY:
    {
        QDateTime inBirthday(QDate::fromJulianDay(*m_pTag->toJulian()));
        QDateTime outBirthday;

        if(m_bUseSubjectBirthdayOffset)
        {
            outBirthday = inBirthday.addDays(-m_iSubjectBirthdayOffset);
        } else {
            outBirthday = m_dSubjectBirthday;
        }

        FIFFLIB::fiff_int_t outData[1];
        outData[0] = static_cast<int32_t> (outBirthday.toSecsSinceEpoch());
        memcpy(m_pTag->data(),reinterpret_cast<char*>(outData),sizeof(FIFFLIB::fiff_int_t));
        printIfVerbose("Subject birthday date changed: " + inBirthday.toString("dd.MM.yyyy hh:mm:ss.zzz t") + " -> " + outBirthday.toString("dd.MM.yyyy hh:mm:ss.zzz t"));
        break;
    }
    case FIFF_SUBJ_SEX:
    {
        if(m_bBruteMode)
        {
            qint32 inSubjSex(*m_pTag->toInt());
            qint32 outSubjSex(m_iDfltSubjectSex);
            memcpy(m_pTag->data(),&outSubjSex, sizeof(qint32));
            printIfVerbose("Subject sex changed: " + subjectSexToString(inSubjSex) + " -> " + subjectSexToString(outSubjSex));
        }
        break;
    }
    case FIFF_SUBJ_HAND:
    {
        if(m_bBruteMode)
        {
            qint32 inSubjHand(*m_pTag->toInt());
            qint32 newSubjHand(m_iDfltSubjectHand);
            memcpy(m_pTag->data(),&newSubjHand, sizeof(qint32));
            printIfVerbose("Subject handedness changed: " + subjectHandToString(inSubjHand) + " -> " + subjectHandToString(newSubjHand));
        }
        break;
    }
    case FIFF_SUBJ_WEIGHT:
    {
        if(m_bBruteMode)
        {
            float inWeight(*m_pTag->toFloat());
            float outWeight(m_fSubjectWeight);
            memcpy(m_pTag->data(),&outWeight,sizeof(float));
            printIfVerbose("Subject weight changed: " + QString::number(static_cast<double>(inWeight)) + " -> " + QString::number(static_cast<double>(outWeight)));
        }
        break;
    }
    case FIFF_SUBJ_HEIGHT:
    {
        if(m_bBruteMode)
        {
            float inHeight(*m_pTag->toFloat());
            float outHeight(m_fSubjectHeight);
            memcpy(m_pTag->data(),&outHeight,sizeof(float));
            printIfVerbose("Subject height changed: " + QString::number(static_cast<double>(inHeight)) + " -> " + QString::number(static_cast<double>(outHeight)));
        }
        break;
    }
    case FIFF_SUBJ_COMMENT:
    {
        QString inStr(m_pTag->data());
        QString outStr(m_sSubjectComment);
        m_pTag->resize(outStr.size());
        memcpy(m_pTag->data(),outStr.toUtf8(),static_cast<size_t>(outStr.size()));
        printIfVerbose("Subject comment changed: " + QString(m_pTag->data()) + " -> " + outStr);
        break;
    }
    case FIFF_SUBJ_HIS_ID:
    {
        QString inSubjectHisId(m_pTag->data());
        QString outSubjectHisId(m_sSubjectHisId);
        m_pTag->resize(outSubjectHisId.size());
        memcpy(m_pTag->data(),outSubjectHisId.toUtf8(),static_cast<size_t>(outSubjectHisId.size()));
        printIfVerbose("Subject Hospital-ID(His Id) changed: " + inSubjectHisId + " -> " + outSubjectHisId);
        break;
    }
    case FIFF_PROJ_ID:
    {
        if(m_bBruteMode)
        {
            qint32 inProjID(*m_pTag->toInt());
            qint32 newProjID(m_iProjectId);
            memcpy(m_pTag->data(),&newProjID,sizeof(qint32));
            printIfVerbose("ProjectID changed: " + QString::number(inProjID) + " -> " + QString::number(newProjID));
        }
        break;
    }
    case FIFF_PROJ_NAME:
    {
        if(m_bBruteMode)
        {
            QString inStr(m_pTag->data());
            QString outStr(m_sProjectName);
            m_pTag->resize(outStr.size());
            memcpy(m_pTag->data(),outStr.toUtf8(),static_cast<size_t>(outStr.size()));
            printIfVerbose("Project name changed: " + inStr + " -> " + outStr);
        }
        break;
    }
    case FIFF_PROJ_AIM:
    {
        if(m_bBruteMode)
        {
            QString inStr(m_pTag->data());
            QString outStr(m_sProjectAim);
            m_pTag->resize(outStr.size());
            memcpy(m_pTag->data(),outStr.toUtf8(),static_cast<size_t>(outStr.size()));
            printIfVerbose("Project aim changed: " + inStr + " -> " + outStr);
        }
        break;
    }
    case FIFF_PROJ_PERSONS:
    {
        QString inStr(m_pTag->data());
        QString outStr(m_sProjectPersons);
        m_pTag->resize(outStr.size());
        memcpy(m_pTag->data(),outStr.toUtf8(),static_cast<size_t>(outStr.size()));
        printIfVerbose("Project persons changed: " + inStr + " -> " + outStr);
        break;
    }
    case FIFF_PROJ_COMMENT:
    {
        if(m_bBruteMode)
        {
            QString inStr(m_pTag->data());
            QString outStr(m_sProjectComment);
            m_pTag->resize(outStr.size());
            memcpy(m_pTag->data(),outStr.toUtf8(),static_cast<size_t>(outStr.size()));
            printIfVerbose("Project comment changed: " + inStr + " -> " + outStr);
        }
        break;
    }
    case FIFF_MRI_PIXEL_DATA:
    {
        printIfVerbose("  ");
        printIfVerbose("Warning: The input fif file contains MRI data.");
        printIfVerbose("Warning: Beware that a subject''s face can be reconstructed from it");
        printIfVerbose("Warning: This software can not anonymize MRI data, at the moment.");
        printIfVerbose("  ");
        break;
    }
    default:
    {
    }//default
    }//switch
}

//=============================================================================================================

void FiffAnonymizer::readTag()
{
    m_pInStream->read_tag(m_pTag,-1);
    updateBlockTypeList();
}

//=============================================================================================================

void FiffAnonymizer::writeTag()
{
    //make output tag list linear
    if(m_pTag->next > 0)
    {
        m_pTag->next = FIFFV_NEXT_SEQ;
    }

    FIFFLIB::FiffTag::convert_tag_data(m_pTag,FIFFV_NATIVE_ENDIAN,FIFFV_BIG_ENDIAN);
    m_pOutStream->write_tag(m_pTag, -1);
}

//=============================================================================================================

void FiffAnonymizer::processHeaderTags()
{
    readTag();

    if(checkValidFiffFormatVersion())
    {
        printIfVerbose("Input file compatible with this version.");
    } else {
        qCritical() << "This file may not be compatible with this application. First tag is not a valid ID tag.";
    }

    censorTag();
    writeTag();

    // pointer to tag directory
    readTag();

    if(m_pTag->kind != FIFF_DIR_POINTER)
    {
        qCritical() << "This file may not be compatible with this application. Second tag is not a valid Tag directory pointer tag.";
    }
    qint32 newTagDirLoc(-1);
    memcpy(m_pTag->data(),&newTagDirLoc,sizeof(qint32));

    censorTag();
    writeTag();

    //free list
    readTag();

    if( (m_pTag->kind == FIFF_FREE_LIST) && (*m_pTag->toInt() > 0) )
    {
        qWarning() << "This file contains a Free List of tags. It will not be copied to the output file.";
    } else {
        // output this tag, whatever kind it is, to the oupput file.
        censorTag();
        writeTag();
    }
}

//=============================================================================================================

void FiffAnonymizer::updateBlockTypeList()
{
    if(m_pTag->kind == FIFF_BLOCK_START)
    {
        m_pBlockTypeList->push(*m_pTag->toInt());
    }

    if(m_pTag->kind == FIFF_BLOCK_END)
    {
        m_pBlockTypeList->pop();
    }
}

//=============================================================================================================

bool FiffAnonymizer::checkValidFiffFormatVersion() const
{
    if(m_pTag->kind == FIFF_FILE_ID)
    {
        FIFFLIB::FiffId fileId = m_pTag->toFiffID();
        int inMayorVersion = (static_cast<uint32_t>(fileId.version) & 0xFFFF0000) >> 16;
        int inMinorVersion = (static_cast<uint32_t>(fileId.version) & 0x0000FFFF);
        double inVersion = inMayorVersion + inMinorVersion/10.0;

        if(inVersion > m_dMaxValidFiffVerion) {
            return false;
        }
        return true;
    }
    return false;
}

//=============================================================================================================

int FiffAnonymizer::setFileIn(const QString &sFileIn)
{
    QFileInfo fiIn(sFileIn);
    if(fiIn.exists())
    {
        m_fFileIn.setFileName(fiIn.absoluteFilePath());
        m_bFileInSet = true;
        return 0;
    } else {
        qCritical() << "File doesn't exist.";
        return 1;
    }
}

//=============================================================================================================

QString FiffAnonymizer::getFileNameIn() const
{
    return m_fFileIn.fileName();
}

//=============================================================================================================

int FiffAnonymizer::setFileOut(const QString &sFileOut)
{
    if(m_bFileInSet)
    {
        QFileInfo fiIn(m_fFileIn);
        QFileInfo fiOut(sFileOut);
        if(fiOut.absoluteFilePath() == fiIn.absoluteFilePath())
        {
            qCritical() << "Both input and output file names are the same.";
            return 1;
        } else {
            m_fFileOut.setFileName(fiOut.absoluteFilePath());
        }
        m_bFileOutSet = true;
        return 0;
    } else {
        qCritical() << "You need to specify the input file first.";
        return 1;
    }
}

//=============================================================================================================

QString FiffAnonymizer::getFileNameOut() const
{
    return m_fFileOut.fileName();
}

//=============================================================================================================

void FiffAnonymizer::setVerboseMode(bool bFlag)
{
    m_bVerboseMode = bFlag;
}

//=============================================================================================================

bool FiffAnonymizer::getVerboseMode() const
{
    return m_bVerboseMode;
}

//=============================================================================================================

void FiffAnonymizer::setBruteMode(bool bFlag)
{
    m_bBruteMode = bFlag;
}

//=============================================================================================================

bool FiffAnonymizer::getBruteMode() const
{
    return m_bBruteMode;
}

//=============================================================================================================

void FiffAnonymizer::setMeasurementDay(const QDateTime& d)
{
    m_dMeasurementDate = QDateTime(d.date(),QTime(1,1,0));
}

//=============================================================================================================

void FiffAnonymizer::setMeasurementDay(const QDate& d)
{
    m_dMeasurementDate = QDateTime(d,QTime(1,1,0));
}

//=============================================================================================================

void FiffAnonymizer::setMeasurementDay(const QString& sMeasDay)
{

    m_dMeasurementDate = QDateTime(QDate::fromString(sMeasDay,"ddMMyyyy"),QTime(1,1,0));
}

//=============================================================================================================

QDateTime FiffAnonymizer::getMeasurementDate() const
{
    return m_dMeasurementDate;
}

//=============================================================================================================

void FiffAnonymizer::setMeasurementDayOffset(const int iMeasDayOffset)
{
    m_bUseMeasurementDayOffset = true;
    m_iMeasurementDayOffset = iMeasDayOffset;
}

//=============================================================================================================

void FiffAnonymizer::setSubjectBirthday(const QString& sSubjBirthday)
{
    m_dSubjectBirthday = QDateTime(QDate::fromString(sSubjBirthday,"ddMMyyyy"),QTime(1, 1, 0));
}

//=============================================================================================================

void FiffAnonymizer::setSubjectBirthday(const QDateTime& sSubjBirthday)
{
    m_dSubjectBirthday = QDateTime(sSubjBirthday);
}

//=============================================================================================================

QDateTime FiffAnonymizer::getSubjectBirthday()
{
    return m_dSubjectBirthday;
}

//=============================================================================================================

void FiffAnonymizer::setSubjectBirthdayOffset(int iSubjBirthdayOffset)
{
    m_bUseSubjectBirthdayOffset = true;
    m_iSubjectBirthdayOffset = iSubjBirthdayOffset;
}

//=============================================================================================================

int  FiffAnonymizer::getSubjectBirthdayOffset()
{
    return m_iSubjectBirthdayOffset;
}

//=============================================================================================================

void FiffAnonymizer::setSubjectHisId(const QString& sSubjectHisId)
{
    m_sSubjectHisId = sSubjectHisId;
}

//=============================================================================================================

int FiffAnonymizer::openInOutStreams()
{
    m_pInStream = FIFFLIB::FiffStream::SPtr (new FIFFLIB::FiffStream(&m_fFileIn));
    if(m_pInStream->device()->open(QIODevice::ReadOnly))
    {
        printIfVerbose("Input file opened correctly: " + m_fFileIn.fileName());
    } else {
        qCritical() << "Problem opening the input file: " << m_fFileIn.fileName();
        return 1;
    }

    m_pOutStream = FIFFLIB::FiffStream::SPtr(new FIFFLIB::FiffStream(&m_fFileOut));
    if(m_pOutStream->device()->open(QIODevice::WriteOnly))
    {
        printIfVerbose("Output file opened correctly: " + m_fFileOut.fileName());
    } else {
        qCritical() << "Problem opening the output file: " << m_fFileOut.fileName();
        return 1;
    }
    return 0;
}

//=============================================================================================================

int FiffAnonymizer::closeInOutStreams()
{
    if(m_pInStream->close())
    {
        printIfVerbose("Input file closed.");
    } else {
        qCritical() << "Problem closing the input file: " << m_fFileIn.fileName();
        return 1;
    }

    if(m_pOutStream->close())
    {
        printIfVerbose("Output file closed.");
    } else {
        qCritical() << "Problem closing the output file: " << m_fFileOut.fileName();
        return 1;
    }
    return 0;
}
