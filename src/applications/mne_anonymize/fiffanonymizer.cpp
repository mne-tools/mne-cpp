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
: m_pTag(FIFFLIB::FiffTag::SPtr::create())
, m_bFileInSet(false)
, m_bFileOutSet(false)
, m_bVerboseMode(false)
, m_bBruteMode(false)
, m_bMNEEnvironmentMode(false)
, m_dMaxValidFiffVerion(1.3)
, m_sDefaultString("mne_anonymize")
, m_sDefaultShortString("mne-cpp")
, m_dDefaultDate(QDateTime(QDate(2000,1,1), QTime(1, 1, 0), Qt::LocalTime))
, m_dMeasurementDate(m_dDefaultDate)
, m_iMeasurementDateOffset(0)
, m_bUseMeasurementDateOffset(false)
, m_sFiffComment(m_sDefaultString)
, m_sFiffExperimenter(m_sDefaultString)
, m_iSubjectId(0)
, m_sSubjectFirstName(m_sDefaultString)
, m_sSubjectMidName("mne")
, m_sSubjectLastName(m_sDefaultString)
, m_dSubjectBirthday(QDate(2000,1,1))
, m_iSubjectBirthdayOffset(0)
, m_bUseSubjectBirthdayOffset(false)
, m_sSubjectComment(m_sDefaultString)
, m_iSubjectSex(0)
, m_iSubjectHand(0)
, m_fSubjectWeight(0.0)
, m_fSubjectHeight(0.0)
, m_sSubjectHisId(m_sDefaultString)
, m_iProjectId(0)
, m_sProjectName(m_sDefaultString)
, m_sProjectAim(m_sDefaultString)
, m_sProjectPersons(m_sDefaultString)
, m_sProjectComment(m_sDefaultString)
, m_sMNEWorkingDir(m_sDefaultString)
, m_sMNECommand(m_sDefaultString)
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
: m_pTag(FIFFLIB::FiffTag::SPtr::create())
, m_bFileInSet(obj.m_bFileInSet)
, m_bFileOutSet(obj.m_bFileOutSet)
, m_bVerboseMode(obj.m_bVerboseMode)
, m_bBruteMode(obj.m_bBruteMode)
, m_bMNEEnvironmentMode(obj.m_bMNEEnvironmentMode)
, m_dMaxValidFiffVerion(obj.m_dMaxValidFiffVerion)
, m_sDefaultString(obj.m_sDefaultString)
, m_sDefaultShortString(obj.m_sDefaultShortString)
, m_dDefaultDate(obj.m_dDefaultDate)
, m_dMeasurementDate(obj.m_dMeasurementDate)
, m_iMeasurementDateOffset(obj.m_iMeasurementDateOffset)
, m_bUseMeasurementDateOffset(obj.m_bUseMeasurementDateOffset)
, m_sFiffComment(obj.m_sFiffComment)
, m_sFiffExperimenter(obj.m_sFiffExperimenter)
, m_iSubjectId(obj.m_iSubjectId)
, m_sSubjectFirstName(obj.m_sSubjectFirstName)
, m_sSubjectMidName(obj.m_sSubjectMidName)
, m_sSubjectLastName(obj.m_sSubjectLastName)
, m_dSubjectBirthday(obj.m_dSubjectBirthday)
, m_iSubjectBirthdayOffset(obj.m_iSubjectBirthdayOffset)
, m_bUseSubjectBirthdayOffset(obj.m_bUseSubjectBirthdayOffset)
, m_sSubjectComment(obj.m_sSubjectComment)
, m_iSubjectSex(obj.m_iSubjectSex)
, m_iSubjectHand(obj.m_iSubjectHand)
, m_fSubjectWeight(obj.m_fSubjectWeight)
, m_fSubjectHeight(obj.m_fSubjectHeight)
, m_sSubjectHisId(obj.m_sSubjectHisId)
, m_iProjectId(obj.m_iProjectId)
, m_sProjectName(obj.m_sProjectName)
, m_sProjectAim(obj.m_sProjectAim)
, m_sProjectPersons(obj.m_sProjectPersons)
, m_sProjectComment(obj.m_sProjectComment)
, m_sMNEWorkingDir(obj.m_sDefaultString)
, m_sMNECommand(obj.m_sDefaultString)
{
    memcpy(m_pTag->data(),obj.m_pTag->data(),static_cast<size_t>(obj.m_pTag->size()));

    m_BDfltMAC[0] = obj.m_BDfltMAC[0];
    m_BDfltMAC[1] = obj.m_BDfltMAC[1];

    m_pBlockTypeList = QSharedPointer<QStack<int32_t> >(new QStack<int32_t>(*obj.m_pBlockTypeList));
}

//=============================================================================================================

FiffAnonymizer::FiffAnonymizer(FiffAnonymizer &&obj)
: m_pTag(FIFFLIB::FiffTag::SPtr::create())
, m_bFileInSet(obj.m_bFileInSet)
, m_bFileOutSet(obj.m_bFileOutSet)
, m_bVerboseMode(obj.m_bVerboseMode)
, m_bBruteMode(obj.m_bBruteMode)
, m_bMNEEnvironmentMode(obj.m_bMNEEnvironmentMode)
, m_dMaxValidFiffVerion(obj.m_dMaxValidFiffVerion)
, m_sDefaultString(obj.m_sDefaultString)
, m_sDefaultShortString(obj.m_sDefaultShortString)
, m_dDefaultDate(obj.m_dDefaultDate)
, m_dMeasurementDate(obj.m_dMeasurementDate)
, m_iMeasurementDateOffset(obj.m_iMeasurementDateOffset)
, m_bUseMeasurementDateOffset(obj.m_bUseMeasurementDateOffset)
, m_sFiffComment(obj.m_sFiffComment)
, m_sFiffExperimenter(obj.m_sFiffExperimenter)
, m_iSubjectId(obj.m_iSubjectId)
, m_sSubjectFirstName(obj.m_sSubjectFirstName)
, m_sSubjectMidName(obj.m_sSubjectMidName)
, m_sSubjectLastName(obj.m_sSubjectLastName)
, m_dSubjectBirthday(obj.m_dSubjectBirthday)
, m_iSubjectBirthdayOffset(obj.m_iSubjectBirthdayOffset)
, m_bUseSubjectBirthdayOffset(obj.m_bUseSubjectBirthdayOffset)
, m_sSubjectComment(obj.m_sSubjectComment)
, m_iSubjectSex(obj.m_iSubjectSex)
, m_iSubjectHand(obj.m_iSubjectHand)
, m_fSubjectWeight(obj.m_fSubjectWeight)
, m_fSubjectHeight(obj.m_fSubjectHeight)
, m_sSubjectHisId(obj.m_sSubjectHisId)
, m_iProjectId(obj.m_iProjectId)
, m_sProjectName(obj.m_sProjectName)
, m_sProjectAim(obj.m_sProjectAim)
, m_sProjectPersons(obj.m_sProjectPersons)
, m_sProjectComment(obj.m_sProjectComment)
, m_sMNEWorkingDir(obj.m_sDefaultString)
, m_sMNECommand(obj.m_sDefaultString)
{
    memcpy(m_pTag->data(),obj.m_pTag->data(),static_cast<size_t>(obj.m_pTag->size()));

    m_BDfltMAC[0] = obj.m_BDfltMAC[0];
    m_BDfltMAC[1] = obj.m_BDfltMAC[1];

    m_pBlockTypeList.swap(obj.m_pBlockTypeList);
}

//=============================================================================================================

int FiffAnonymizer::anonymizeFile()
{
    if(!m_bFileInSet)
    {
        qCritical() << "Input file has not been specified.";
        return 1;
    }

    if(!m_bFileOutSet)
    {
        qCritical() << "Output file has not been specified.";
        return 1;
    }

    printIfVerbose("Max. Valid Fiff version: " + QString::number(m_dMaxValidFiffVerion));
    printIfVerbose("Current date and time: " __DATE__ " " __TIME__);
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

    emit outFileReady();

    return 0;
}

//=============================================================================================================

void FiffAnonymizer::censorTag()
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
        emit readingIdMeasurementDate(inMeasDate);

        QDateTime outMeasDate;

        if(m_bUseMeasurementDateOffset)
        {
            outMeasDate = inMeasDate.addDays(-m_iMeasurementDateOffset);
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
        emit readingFileMeasurementDate(inMeasDate);
        QDateTime outMeasDate;

        if(m_bUseMeasurementDateOffset)
        {
            outMeasDate = inMeasDate.addDays(-m_iMeasurementDateOffset);
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
        QString inStr(m_pTag->data());
        emit readingFileComment(inStr);

        if(m_pBlockTypeList->top() == FIFFB_MEAS_INFO)
        {
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
        emit readingFileExperimenter(inStr);

        QString outStr(m_sDefaultString);
        m_pTag->resize(outStr.size());
        memcpy(m_pTag->data(),outStr.toUtf8(),static_cast<size_t>(outStr.size()));
        printIfVerbose("Experimenter changed: " + inStr + " -> " + outStr);
        break;
    }
    case FIFF_SUBJ_ID:
    {
        qint32 inSubjID(*m_pTag->toInt());
        emit readingSubjectId(inSubjID);
        qint32 outSubjID(m_iSubjectId);
        memcpy(m_pTag->data(),&outSubjID, sizeof(qint32));
        printIfVerbose("Subject ID changed: " + QString::number(inSubjID) + " -> " + QString::number(outSubjID));
        break;
    }
    case FIFF_SUBJ_FIRST_NAME:
    {
        QString inFirstName(m_pTag->data());
        emit readingSubjectFirstName(inFirstName);
        QString outFirstName(m_sSubjectFirstName);
        m_pTag->resize(outFirstName.size());
        memcpy(m_pTag->data(),outFirstName.toUtf8(),static_cast<size_t>(outFirstName.size()));
        printIfVerbose("Subject first name changed: " + inFirstName + " -> " + outFirstName);
        break;
    }
    case FIFF_SUBJ_MIDDLE_NAME:
    {
        QString inStr(m_pTag->data());
        emit readingSubjectMiddleName(inStr);
        QString outStr(m_sSubjectMidName);
        m_pTag->resize(outStr.size());
        memcpy(m_pTag->data(),outStr.toUtf8(),static_cast<size_t>(outStr.size()));
        printIfVerbose("Subject middle name changed: " + inStr + " -> " + outStr);
        break;
    }
    case FIFF_SUBJ_LAST_NAME:
    {
        QString inStr(m_pTag->data());
        emit readingSubjectLastName(inStr);
        QString outStr(m_sSubjectLastName);
        m_pTag->resize(outStr.size());
        memcpy(m_pTag->data(),outStr.toUtf8(),static_cast<size_t>(outStr.size()));
        printIfVerbose("Subject last name changed: " + inStr + " -> " + outStr);
        break;
    }
    case FIFF_SUBJ_BIRTH_DAY:
    {
        QDate inBirthday(QDate::fromJulianDay(*m_pTag->toJulian()));
        emit readingSubjectBirthday(inBirthday);
        QDate outBirthday;

        if(m_bUseSubjectBirthdayOffset)
        {
            outBirthday = inBirthday.addDays(-m_iSubjectBirthdayOffset);
        } else {
            outBirthday = m_dSubjectBirthday;
        }

        FIFFLIB::fiff_int_t outData[1];
        outData[0] = static_cast<int32_t> (outBirthday.toJulianDay());
        memcpy(m_pTag->data(),reinterpret_cast<char*>(outData),sizeof(FIFFLIB::fiff_int_t));
        printIfVerbose("Subject birthday date changed: " + inBirthday.toString("dd.MM.yyyy") + " -> " + outBirthday.toString("dd.MM.yyyy"));
        break;
    }
    case FIFF_SUBJ_SEX:
    {
        qint32 inSubjectSex(*m_pTag->toInt());
        emit readingSubjectSex(inSubjectSex);
        if(m_bBruteMode)
        {
            qint32 outSubjSex(m_iSubjectSex);
            memcpy(m_pTag->data(),&outSubjSex, sizeof(qint32));
            printIfVerbose("Subject sex changed: " + subjectSexToString(inSubjectSex) + " -> " + subjectSexToString(outSubjSex));
        }
        break;
    }
    case FIFF_SUBJ_HAND:
    {
        qint32 inSubjectHand(*m_pTag->toInt());
        emit readingSubjectHand(inSubjectHand);
        if(m_bBruteMode)
        {
            qint32 newSubjHand(m_iSubjectHand);
            memcpy(m_pTag->data(),&newSubjHand, sizeof(qint32));
            printIfVerbose("Subject handedness changed: " + subjectHandToString(inSubjectHand) + " -> " + subjectHandToString(newSubjHand));
        }
        break;
    }
    case FIFF_SUBJ_WEIGHT:
    {
        float inWeight(*m_pTag->toFloat());
        emit readingSubjectWeight(inWeight);
        if(m_bBruteMode)
        {
            float outWeight(m_fSubjectWeight);
            memcpy(m_pTag->data(),&outWeight,sizeof(float));
            printIfVerbose("Subject weight changed: " + QString::number(static_cast<double>(inWeight)) + " -> " + QString::number(static_cast<double>(outWeight)));
        }
        break;
    }
    case FIFF_SUBJ_HEIGHT:
    {
        float inHeight(*m_pTag->toFloat());
        emit readingSubjectHeight(inHeight);
        if(m_bBruteMode)
        {
            float outHeight(m_fSubjectHeight);
            memcpy(m_pTag->data(),&outHeight,sizeof(float));
            printIfVerbose("Subject height changed: " + QString::number(static_cast<double>(inHeight)) + " -> " + QString::number(static_cast<double>(outHeight)));
        }
        break;
    }
    case FIFF_SUBJ_COMMENT:
    {
        QString inStr(m_pTag->data());
        emit readingSubjectComment(inStr);
        QString outStr(m_sSubjectComment);
        m_pTag->resize(outStr.size());
        memcpy(m_pTag->data(),outStr.toUtf8(),static_cast<size_t>(outStr.size()));
        printIfVerbose("Subject comment changed: " + inStr + " -> " + outStr);
        break;
    }
    case FIFF_SUBJ_HIS_ID:
    {
        QString inSubjectHisId(m_pTag->data());
        emit readingSubjectHisId(inSubjectHisId);
        QString outSubjectHisId(m_sSubjectHisId);
        m_pTag->resize(outSubjectHisId.size());
        memcpy(m_pTag->data(),outSubjectHisId.toUtf8(),static_cast<size_t>(outSubjectHisId.size()));
        printIfVerbose("Subject Hospital-ID(His Id) changed: " + inSubjectHisId + " -> " + outSubjectHisId);
        break;
    }
    case FIFF_PROJ_ID:
    {
        qint32 inProjID(*m_pTag->toInt());
        emit readingProjectId(inProjID);
        if(m_bBruteMode)
        {
            qint32 newProjID(m_iProjectId);
            memcpy(m_pTag->data(),&newProjID,sizeof(qint32));
            printIfVerbose("ProjectID changed: " + QString::number(inProjID) + " -> " + QString::number(newProjID));
        }
        break;
    }
    case FIFF_PROJ_NAME:
    {
        QString inStr(m_pTag->data());
        emit readingProjectName(inStr);
        if(m_bBruteMode)
        {
            QString outStr(m_sProjectName);
            m_pTag->resize(outStr.size());
            memcpy(m_pTag->data(),outStr.toUtf8(),static_cast<size_t>(outStr.size()));
            printIfVerbose("Project name changed: " + inStr + " -> " + outStr);
        }
        break;
    }
    case FIFF_PROJ_AIM:
    {
        QString inStr(m_pTag->data());
        emit readingProjectAim(inStr);
        if(m_bBruteMode)
        {
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
        emit readingProjectPersons(inStr);
        QString outStr(m_sProjectPersons);
        m_pTag->resize(outStr.size());
        memcpy(m_pTag->data(),outStr.toUtf8(),static_cast<size_t>(outStr.size()));
        printIfVerbose("Project persons changed: " + inStr + " -> " + outStr);
        break;
    }
    case FIFF_PROJ_COMMENT:
    {
        QString inStr(m_pTag->data());
        emit readingProjectComment(inStr);
        if(m_bBruteMode)
        {
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
        emit mriDataFoundInFile(true);
        break;
    }
    case FIFF_MNE_ENV_WORKING_DIR:
    {
        QString inStr(m_pTag->data());
        emit readingMNEWorkingDir(inStr);
        if(m_bMNEEnvironmentMode || m_bBruteMode)
        {
            QString outStr(m_sMNEWorkingDir);
            m_pTag->resize(outStr.size());
            memcpy(m_pTag->data(),outStr.toUtf8(),static_cast<size_t>(outStr.size()));
            printIfVerbose("MNE working directory info changed: " + inStr + " -> " + outStr);
        }
        break;
    }
    case FIFF_MNE_ENV_COMMAND_LINE:
    {
        QString inStr(m_pTag->data());
        emit readingMNECommandLine(inStr);
        if(m_bMNEEnvironmentMode || m_bBruteMode)
        {
            QString outStr(m_sMNECommand);
            m_pTag->resize(outStr.size());
            memcpy(m_pTag->data(),outStr.toUtf8(),static_cast<size_t>(outStr.size()));
            printIfVerbose("MNE command line info changed: " + inStr + " -> " + outStr);
        }
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

bool FiffAnonymizer::checkValidFiffFormatVersion()
{
    if(m_pTag->kind == FIFF_FILE_ID)
    {
        FIFFLIB::FiffId fileId = m_pTag->toFiffID();
        int inMayorVersion = (static_cast<uint32_t>(fileId.version) & 0xFFFF0000) >> 16;
        int inMinorVersion = (static_cast<uint32_t>(fileId.version) & 0x0000FFFF);
        double inVersion = inMayorVersion + inMinorVersion/10.0;
        emit readingIdFileVersion(inVersion);
        emit readingIdMac(fileId.toMachidString());

        if(inVersion > m_dMaxValidFiffVerion) {
            return false;
        }
        return true;
    }
    return false;
}

//=============================================================================================================

int FiffAnonymizer::setInFile(const QString &sFileIn)
{
    QFileInfo fiIn(sFileIn);
    m_fFileIn.setFileName(fiIn.absoluteFilePath());
    m_bFileInSet = true;
//    qDebug() << "Input file set: " << fiIn.absoluteFilePath();
    return 0;
}

//=============================================================================================================

QString FiffAnonymizer::getFileNameIn() const
{
    return m_fFileIn.fileName();
}

//=============================================================================================================

int FiffAnonymizer::setOutFile(const QString &sFileOut)
{
    QFileInfo fiIn(m_fFileIn);
    QFileInfo fiOut(sFileOut);
    if(fiOut.absoluteFilePath() == fiIn.absoluteFilePath())
    {
        qWarning() << "Both input and output file names are the same.";
        return 1;
    } else {
        m_fFileOut.setFileName(fiOut.absoluteFilePath());
    }
    m_bFileOutSet = true;
//    qDebug() << "Output file set: " << fiOut.absoluteFilePath();
    return 0;
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

void FiffAnonymizer::setMNEEnvironmentMode(bool bFlag)
{
    m_bMNEEnvironmentMode = bFlag;
}

//=============================================================================================================

bool FiffAnonymizer::getMNEEnvironmentMode()
{
    return m_bMNEEnvironmentMode;
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

void FiffAnonymizer::setMeasurementDate(const QDateTime& d)
{
    m_dMeasurementDate = QDateTime(d.date(),QTime(1,1,0));
}

//=============================================================================================================

void FiffAnonymizer::setMeasurementDate(const QString& sMeasDay)
{

    m_dMeasurementDate = QDateTime(QDate::fromString(sMeasDay,"ddMMyyyy"),QTime(1,1,0));
}

//=============================================================================================================

QDateTime FiffAnonymizer::getMeasurementDate() const
{
    return m_dMeasurementDate;
}

//=============================================================================================================

void FiffAnonymizer::setMeasurementDateOffset(const int iMeasDayOffset)
{
    m_iMeasurementDateOffset = iMeasDayOffset;
}

//=============================================================================================================

void FiffAnonymizer::setUseMeasurementDateOffset(bool b)
{
        m_bUseMeasurementDateOffset = b;
}

//=============================================================================================================

void FiffAnonymizer::setSubjectBirthday(const QString& sSubjBirthday)
{
    m_dSubjectBirthday = QDate::fromString(sSubjBirthday,"ddMMyyyy");
}

//=============================================================================================================

void FiffAnonymizer::setSubjectBirthday(const QDate& dSubjBirthday)
{
    m_dSubjectBirthday = QDate(dSubjBirthday);
}

//=============================================================================================================

QDate FiffAnonymizer::getSubjectBirthday()
{
    return m_dSubjectBirthday;
}

//=============================================================================================================

void FiffAnonymizer::setSubjectBirthdayOffset(int iSubjBirthdayOffset)
{
    m_iSubjectBirthdayOffset = iSubjBirthdayOffset;
}

//=============================================================================================================

void FiffAnonymizer::setUseSubjectBirthdayOffset(bool b)
{
    m_bUseSubjectBirthdayOffset = b;
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

    m_pOutStream = FIFFLIB::FiffStream::SPtr(new FIFFLIB::FiffStream(&m_fFileOut));

    if(m_pInStream->device()->open(QIODevice::ReadOnly))
    {
        printIfVerbose("Input file opened correctly: " + m_fFileIn.fileName());
    } else {
        qCritical() << "Problem opening the input file: " << m_fFileIn.fileName();
        return 1;
    }

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

//=============================================================================================================

QString FiffAnonymizer::getSubjectHisID()
{
    return m_sSubjectHisId;
}

//=============================================================================================================

int FiffAnonymizer::getMeasurementDayOffset()
{
    return m_iMeasurementDateOffset;
}

//=============================================================================================================

bool FiffAnonymizer::getUseMeasurementDayOffset()
{
    return m_bUseMeasurementDateOffset;
}

//=============================================================================================================

bool FiffAnonymizer::getUseSubjectBirthdayOffset()
{
    return m_bUseSubjectBirthdayOffset;
}

//=============================================================================================================

bool FiffAnonymizer::isFileInSet() const
{
    return m_bFileInSet;
}

//=============================================================================================================

bool FiffAnonymizer::isFileOutSet() const
{
    return m_bFileOutSet;
}
