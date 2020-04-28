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
, m_iDirectoryPos(-1)
, m_bFileHasDirPtr(true)
, m_dDefaultDate(QDateTime(QDate(2000,1,1), QTime(1, 1, 0), Qt::UTC))
, m_dMeasurementDate(m_dDefaultDate)
, m_dSubjectBirthday(m_dDefaultDate)
, m_iMeasurementDayOffset(0)
, m_bUseMeasurementDayOffset(false)
, m_bUseSubjectBirthdayOffset(false)
, m_bVerboseMode(false)
, m_bBruteMode(false)
, m_bDeleteInputFileAfter(false)
, m_bDeleteInputFileConfirmation(true)
, m_bInputFileDeleted(false)
, m_bInOutFileNamesEqual(false)
, m_bOutFileRenamed(false)
, m_pInTag(FIFFLIB::FiffTag::SPtr::create())
, m_pOutTag(FIFFLIB::FiffTag::SPtr::create())
{
    //MAC addresses have 6 bytes. We use 2 more here to complete 2 int32 (2bytes) reads.
    //check->sometimes MAC address is stored in the 0-5 bytes some other times it
    //is stored in the 2-7 bytes. To do -> Check why!!!
    m_BDfltMAC[0] = 0;
    m_BDfltMAC[1] = 0;

    m_pBlockTypeList = QSharedPointer<QStack<int32_t> >(new QStack<int32_t>);
    m_pBlockTypeList->clear();
    m_pOutDir = QSharedPointer<QVector<FIFFLIB::FiffDirEntry> >(new QVector<FIFFLIB::FiffDirEntry>);

}

//=============================================================================================================

FiffAnonymizer::FiffAnonymizer(const FiffAnonymizer& obj)
: m_dMaxValidFiffVerion(obj.m_dMaxValidFiffVerion)
, m_sDefaultString(obj.m_sDefaultString)
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
, m_iDirectoryPos(-1)
, m_bFileHasDirPtr(obj.m_bFileHasDirPtr)
, m_dDefaultDate(QDateTime(QDate(2000,1,1), QTime(1, 1, 0), Qt::UTC))
, m_dMeasurementDate(obj.m_dMeasurementDate)
, m_dSubjectBirthday(obj.m_dSubjectBirthday)
, m_iMeasurementDayOffset(obj.m_iMeasurementDayOffset)
, m_bUseMeasurementDayOffset(obj.m_bUseMeasurementDayOffset)
, m_bUseSubjectBirthdayOffset(obj.m_bUseSubjectBirthdayOffset)
, m_bVerboseMode(obj.m_bVerboseMode)
, m_bBruteMode(obj.m_bBruteMode)
, m_bDeleteInputFileAfter(obj.m_bDeleteInputFileAfter)
, m_bDeleteInputFileConfirmation(obj.m_bDeleteInputFileConfirmation)
, m_bInputFileDeleted(obj.m_bInputFileDeleted)
, m_bInOutFileNamesEqual(obj.m_bInOutFileNamesEqual)
, m_bOutFileRenamed(obj.m_bOutFileRenamed)
, m_pInTag(FIFFLIB::FiffTag::SPtr::create())
, m_pOutTag(FIFFLIB::FiffTag::SPtr::create())
{
    m_BDfltMAC[0] = obj.m_BDfltMAC[0];
    m_BDfltMAC[1] = obj.m_BDfltMAC[1];

    m_pBlockTypeList = QSharedPointer<QStack<int32_t> >(new QStack<int32_t>(*obj.m_pBlockTypeList));
    m_pOutDir = QSharedPointer<QVector<FIFFLIB::FiffDirEntry> >(new QVector<FIFFLIB::FiffDirEntry>(*obj.m_pOutDir));
}

//=============================================================================================================

FiffAnonymizer::FiffAnonymizer(FiffAnonymizer &&obj)
: m_dMaxValidFiffVerion(obj.m_dMaxValidFiffVerion)
, m_sDefaultString(obj.m_sDefaultString)
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
, m_iDirectoryPos(-1)
, m_bFileHasDirPtr(obj.m_bFileHasDirPtr)
, m_dDefaultDate(QDateTime(QDate(2000,1,1), QTime(1, 1, 0), Qt::UTC))
, m_dMeasurementDate(obj.m_dMeasurementDate)
, m_dSubjectBirthday(obj.m_dSubjectBirthday)
, m_iMeasurementDayOffset(obj.m_iMeasurementDayOffset)
, m_bUseMeasurementDayOffset(obj.m_bUseMeasurementDayOffset)
, m_bUseSubjectBirthdayOffset(obj.m_bUseSubjectBirthdayOffset)
, m_bVerboseMode(obj.m_bVerboseMode)
, m_bBruteMode(obj.m_bBruteMode)
, m_bDeleteInputFileAfter(obj.m_bDeleteInputFileAfter)
, m_bDeleteInputFileConfirmation(obj.m_bDeleteInputFileConfirmation)
, m_bInputFileDeleted(obj.m_bInputFileDeleted)
, m_bInOutFileNamesEqual(obj.m_bInOutFileNamesEqual)
, m_bOutFileRenamed(obj.m_bOutFileRenamed)
, m_pInTag(FIFFLIB::FiffTag::SPtr::create())
, m_pOutTag(FIFFLIB::FiffTag::SPtr::create())
{
    m_BDfltMAC[0] = obj.m_BDfltMAC[0];
    m_BDfltMAC[1] = obj.m_BDfltMAC[1];

    m_pBlockTypeList.swap(obj.m_pBlockTypeList);
    m_pOutDir.swap(obj.m_pOutDir);
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


    while(m_pInTag->next != -1)
    {
        readTag();
        censorTag();
        writeTag();
    }

    if (m_bFileHasDirPtr)
    {
        addFinalEntryToDir();
        writeDirectory();
        updatePointer(m_pOutStream,FIFF_DIR_POINTER,m_iDirectoryPos);

    }

    closeInOutStreams();

    if(checkDeleteInputFile())
    {
        deleteInputFile();
    }

    if(checkRenameOutputFile())
    {
        renameOutputFileAsInputFile();
    }

    qInfo() << "MNE Anonymize finished correctly: " + QFileInfo(m_fFileIn).fileName() + " -> " + QFileInfo(m_fFileOut).fileName();

    printIfVerbose(" ");
    printIfVerbose("----------------------------------------------------------------------------");
    printIfVerbose(" ");

    return 0;
}

//=============================================================================================================

void FiffAnonymizer::censorTag() const
{
    m_pOutTag->kind = m_pInTag->kind;
    m_pOutTag->type = m_pInTag->type;
    m_pOutTag->next = m_pInTag->next;

    switch (m_pInTag->kind) {
    //all these 'kinds' of tags contain a fileID struct, which contains info related to
    //measurement date
    case FIFF_FILE_ID:
    case FIFF_BLOCK_ID:
    case FIFF_PARENT_FILE_ID:
    case FIFF_PARENT_BLOCK_ID:
    case FIFF_REF_FILE_ID:
    case FIFF_REF_BLOCK_ID:
    {
        FIFFLIB::FiffId inId = m_pInTag->toFiffID();
        QDateTime inMeasDate = QDateTime::fromSecsSinceEpoch(inId.time.secs, Qt::UTC);
        QDateTime outMeasDate;

        if(m_bUseMeasurementDayOffset) {
            outMeasDate = inMeasDate.addDays(-m_iMeasurementDayOffset);
        } else {
            outMeasDate = m_dDefaultDate;
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

        m_pOutTag->resize(fiffIdSize*sizeof(FIFFLIB::fiff_int_t));
        memcpy(m_pOutTag->data(),reinterpret_cast<char*>(outData),fiffIdSize*sizeof(FIFFLIB::fiff_int_t));
        printIfVerbose("MAC address in ID tag changed: " + inId.toMachidString() + " -> "  + outId.toMachidString());
        printIfVerbose("Measurement date in ID tag changed: " + inMeasDate.toString("dd.MM.yyyy hh:mm:ss.zzz t") + " -> " + outMeasDate.toString("dd.MM.yyyy hh:mm:ss.zzz t"));
        break;
    }
    case FIFF_MEAS_DATE:
    {
        QDateTime inMeasDate(QDateTime::fromSecsSinceEpoch(*m_pInTag->toInt(), Qt::UTC));
        QDateTime outMeasDate;

        if(m_bUseMeasurementDayOffset) {
            outMeasDate = inMeasDate.addDays(-m_iMeasurementDayOffset);
        } else {
            outMeasDate = m_dDefaultDate;
        }

        FIFFLIB::fiff_int_t outData[1];
        outData[0] = static_cast<int32_t>(outMeasDate.toSecsSinceEpoch());
        memcpy(m_pOutTag->data(),reinterpret_cast<char*>(outData),sizeof(FIFFLIB::fiff_int_t));
        printIfVerbose("Measurement date changed: " + inMeasDate.toString("dd.MM.yyyy hh:mm:ss.zzz t") + " -> " + outMeasDate.toString("dd.MM.yyyy hh:mm:ss.zzz t"));
        break;
    }
    case FIFF_COMMENT:
    {
        if(m_pBlockTypeList->top()==FIFFB_MEAS_INFO) {
            QString newStr(m_sDefaultString);
            m_pOutTag->resize(newStr.size());
            memcpy(m_pOutTag->data(),newStr.toUtf8(),static_cast<size_t>(newStr.size()));
            printIfVerbose("Description of the measurement block changed: " + QString(m_pInTag->data()) + " -> " + newStr);
        } else {
            m_pOutTag->resize(m_pInTag->size());
            memcpy(m_pOutTag->data(),m_pInTag->data(),static_cast<size_t>(m_pInTag->size()));
        }
        break;
    }
    case FIFF_EXPERIMENTER:
    {
        QString newStr(m_sDefaultString);
        m_pOutTag->resize(newStr.size());
        memcpy(m_pOutTag->data(),newStr.toUtf8(),static_cast<size_t>(newStr.size()));
        printIfVerbose("Experimenter changed: " + QString(m_pInTag->data()) + " -> " + newStr);
        break;
    }
    case FIFF_SUBJ_ID:
    {
        qint32 inSubjID(*m_pInTag->toInt());
        qint32 newSubjID(m_iDfltSubjectId);
        memcpy(m_pOutTag->data(),&newSubjID, sizeof(qint32));
        printIfVerbose("Subject's SubjectID changed: " + QString::number(inSubjID) + " -> " + QString::number(newSubjID));
        break;
    }
    case FIFF_SUBJ_FIRST_NAME:
    {
        QString newStr(m_sSubjectFirstName);
        m_pOutTag->resize(newStr.size());
        memcpy(m_pOutTag->data(),newStr.toUtf8(),static_cast<size_t>(newStr.size()));
        printIfVerbose("Experimenter changed: " + QString(m_pInTag->data()) + " -> " + newStr);
        break;
    }
    case FIFF_SUBJ_MIDDLE_NAME:
    {
        QString newStr(m_sSubjectMidName);
        m_pOutTag->resize(newStr.size());
        memcpy(m_pOutTag->data(),newStr.toUtf8(),static_cast<size_t>(newStr.size()));
        printIfVerbose("Experimenter changed: " + QString(m_pInTag->data()) + " -> " + newStr);
        break;
    }
    case FIFF_SUBJ_LAST_NAME:
    {
        QString newStr(m_sSubjectLastName);
        m_pOutTag->resize(newStr.size());
        memcpy(m_pOutTag->data(),newStr.toUtf8(),static_cast<size_t>(newStr.size()));
        printIfVerbose("Experimenter changed: " + QString(m_pInTag->data()) + " -> " + newStr);
        break;
    }
    case FIFF_SUBJ_BIRTH_DAY:
    {
        QDateTime inBirthday(QDate::fromJulianDay(*m_pInTag->toJulian()));

        QDateTime outBirthday;

        if(m_bUseSubjectBirthdayOffset) {
            outBirthday = inBirthday.addDays(-m_iSubjectBirthdayOffset);
        } else {
            outBirthday = m_dSubjectBirthday;
        }

        FIFFLIB::fiff_int_t outData[1];
        outData[0] = static_cast<int32_t> (outBirthday.toSecsSinceEpoch());
        memcpy(m_pOutTag->data(),reinterpret_cast<char*>(outData),sizeof(FIFFLIB::fiff_int_t));
        printIfVerbose("Subject birthday date changed: " + inBirthday.toString("dd.MM.yyyy hh:mm:ss.zzz t") + " -> " + outBirthday.toString("dd.MM.yyyy hh:mm:ss.zzz t"));

        break;
    }
    case FIFF_SUBJ_SEX:
    {
        qint32 inSubjSex(*m_pInTag->toInt());
        qint32 newSubjSex(m_iDfltSubjectSex);
        memcpy(m_pOutTag->data(),&newSubjSex, sizeof(qint32));
        printIfVerbose("Subject's sex changed: " + QString::number(inSubjSex) + " -> " + QString::number(newSubjSex));
        break;
    }
    case FIFF_SUBJ_HAND:
    {
        qint32 inSubjHand(*m_pInTag->toInt());
        qint32 newSubjHand(m_iDfltSubjectHand);
        memcpy(m_pOutTag->data(),&newSubjHand, sizeof(qint32));
        printIfVerbose("Subject's handedness changed: " + QString::number(inSubjHand) + " -> " + QString::number(newSubjHand));
        break;
    }
    case FIFF_SUBJ_WEIGHT:
    {
        if(m_bBruteMode) {
            float inWeight(*m_pInTag->toFloat());
            float outWeight(m_fSubjectWeight);
            memcpy(m_pOutTag->data(),&outWeight,sizeof(float));
            printIfVerbose("Subject's weight changed: " + QString::number(static_cast<double>(inWeight)) + " -> " + QString::number(static_cast<double>(outWeight)));
        }
        break;
    }
    case FIFF_SUBJ_HEIGHT:
    {
        if(m_bBruteMode) {
            float inHeight(*m_pInTag->toFloat());
            float outHeight(m_fSubjectHeight);
            memcpy(m_pOutTag->data(),&outHeight,sizeof(float));
            printIfVerbose("Subject's Height changed: " + QString::number(static_cast<double>(inHeight)) + " -> " + QString::number(static_cast<double>(outHeight)));
        }
        break;
    }
    case FIFF_SUBJ_COMMENT:
    {
        QString newStr(m_sSubjectComment);
        m_pOutTag->resize(newStr.size());
        memcpy(m_pOutTag->data(),newStr.toUtf8(),static_cast<size_t>(newStr.size()));
        printIfVerbose("Subject Comment changed: " + QString(m_pInTag->data()) + " -> " + newStr);
        break;
    }
    case FIFF_SUBJ_HIS_ID:
    {
        QString inSubjectHisId(m_pInTag->data());
        QString newSubjectHisId(m_sSubjectHisId);
        m_pOutTag->resize(newSubjectHisId.size());
        memcpy(m_pOutTag->data(),newSubjectHisId.toUtf8(),static_cast<size_t>(newSubjectHisId.size()));
        printIfVerbose("Subject Hospital-ID changed:" + inSubjectHisId + " -> " + newSubjectHisId);
        break;
    }
    case FIFF_PROJ_ID:
    {
        if(m_bBruteMode) {
            qint32 inProjID(*m_pInTag->toInt());
            qint32 newProjID(m_iProjectId);
            memcpy(m_pOutTag->data(),&newProjID,sizeof(qint32));
            printIfVerbose("ProjectID changed: " + QString::number(inProjID) + " -> " + QString::number(newProjID));
        }
        break;
    }
    case FIFF_PROJ_NAME:
    {
        if(m_bBruteMode) {
            QString newStr(m_sProjectName);
            m_pOutTag->resize(newStr.size());
            memcpy(m_pOutTag->data(),newStr.toUtf8(),static_cast<size_t>(newStr.size()));
            printIfVerbose("Project name changed: " + QString(m_pInTag->data()) + " -> " + newStr);
        }
        break;
    }
    case FIFF_PROJ_AIM:
    {
        if(m_bBruteMode) {
            QString newStr(m_sProjectAim);
            m_pOutTag->resize(newStr.size());
            memcpy(m_pOutTag->data(),newStr.toUtf8(),static_cast<size_t>(newStr.size()));
            printIfVerbose("Project Aim changed: " + QString(m_pInTag->data()) + " -> " + newStr);
        }
        break;
    }
    case FIFF_PROJ_PERSONS:
    {
        QString newStr(m_sProjectPersons);
        m_pOutTag->resize(newStr.size());
        memcpy(m_pOutTag->data(),newStr.toUtf8(),static_cast<size_t>(newStr.size()));
        printIfVerbose("Project Persons changed: " + QString(m_pInTag->data()) + " -> " + newStr);
        break;
    }
    case FIFF_PROJ_COMMENT:
    {
        if(m_bBruteMode) {
            QString newStr(m_sProjectComment);
            m_pOutTag->resize(newStr.size());
            memcpy(m_pOutTag->data(),newStr.toUtf8(),static_cast<size_t>(newStr.size()));
            printIfVerbose("Project comment changed: " + QString(m_pInTag->data()) + " -> " + newStr);
        }
        break;
    }
    case FIFF_MRI_PIXEL_DATA:
    {
        qWarning() << " ";
        qWarning() << "The input fif file contains MRI data.";
        qWarning() << "Beware that a subject''s face can be reconstructed from it";
        qWarning() << "This software can not anonymize MRI data, at the moment.";
        qWarning() << " ";
        break;
    }
    default:
    {
        m_pOutTag->resize(m_pInTag->size());
        memcpy(m_pOutTag->data(),m_pInTag->data(),static_cast<size_t>(m_pInTag->size()));
    }
    }

}

//=============================================================================================================

void FiffAnonymizer::readTag()
{
    m_pInStream->read_tag(m_pInTag,-1);
    updateBlockTypeList();
}

//=============================================================================================================

void FiffAnonymizer::writeTag()
{
    //make output tag list linear
    if(m_pOutTag->next > 0)
    {
        m_pOutTag->next = FIFFV_NEXT_SEQ;
    }

    if(m_bFileHasDirPtr)
    {
        addEntryToDir();
    }

    FIFFLIB::FiffTag::convert_tag_data(m_pOutTag,FIFFV_NATIVE_ENDIAN,FIFFV_BIG_ENDIAN);
    m_pOutStream->write_tag(m_pOutTag, 0);
}

//=============================================================================================================

void FiffAnonymizer::processHeaderTags()
{
    readTag();

    if(checkValidFiffFormatVersion()) {
        printIfVerbose("Input file compatible with this version of mne_anonymizer.");
    } else {
        qCritical() << "This file may not be compatible with this application. First tag is not a valid ID tag.";
    }

    censorTag();
    writeTag();

    // pointer to tag directory
    readTag();

    if(m_pInTag->kind != FIFF_DIR_POINTER)
    {
        qCritical() << "This file may not be compatible with this application. Second tag is not a valid Tag directory pointer tag.";
    }
    if(*m_pInTag->toInt() <= 0)
    {
        m_bFileHasDirPtr = false;
    }

    censorTag();
    writeTag();

    //free list
    readTag();

    if(m_pInTag->kind == FIFF_FREE_LIST)
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
    if(m_pInTag->kind == FIFF_BLOCK_START) {
        m_pBlockTypeList->push(*m_pInTag->toInt());
    }

    if(m_pInTag->kind == FIFF_BLOCK_END) {
        m_pBlockTypeList->pop();
    }
}

//=============================================================================================================

bool FiffAnonymizer::checkValidFiffFormatVersion() const
{
    if(m_pInTag->kind == FIFF_FILE_ID)
    {
        FIFFLIB::FiffId fileId = m_pInTag->toFiffID();
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

void FiffAnonymizer::addEntryToDir()
{
    FIFFLIB::FiffDirEntry t_dirEntry;
    t_dirEntry.kind = m_pOutTag->kind;
    t_dirEntry.type = m_pOutTag->type;
    t_dirEntry.size = m_pOutTag->size();
    t_dirEntry.pos  = static_cast<FIFFLIB::fiff_int_t>(m_pOutStream->device()->pos());
    m_pOutDir->append(t_dirEntry);
}

//=============================================================================================================

void FiffAnonymizer::addFinalEntryToDir()
{
    FIFFLIB::FiffDirEntry t_dirEntry;
    t_dirEntry.kind = -1;
    t_dirEntry.type = -1;
    t_dirEntry.size = -1;
    t_dirEntry.pos  = -1;
    m_pOutDir->append(t_dirEntry);
}

//=============================================================================================================

void FiffAnonymizer::writeDirectory()
{
    m_iDirectoryPos = m_pOutStream->device()->pos();

    QFile* file=qobject_cast<QFile*>(m_pOutStream->device());
    if(file)
        m_pOutStream->device()->seek(file->size());

    *m_pOutStream << static_cast<quint32>(FIFF_DIR);
    *m_pOutStream << static_cast<quint32>(FIFFT_DIR_ENTRY_STRUCT);
    *m_pOutStream << static_cast<quint32>
                     (static_cast<unsigned long long>(m_pOutDir->size())*sizeof(FIFFLIB::FiffDirEntry));
    *m_pOutStream << static_cast<quint32>(-1);
    for(int i=0;i<m_pOutDir->size();++i) {
        *m_pOutStream << static_cast<quint32>(m_pOutDir->at(i).kind);
        *m_pOutStream << static_cast<quint32>(m_pOutDir->at(i).type);
        *m_pOutStream << static_cast<quint32>(m_pOutDir->at(i).size);
        *m_pOutStream << static_cast<quint32>(m_pOutDir->at(i).pos);
    }
}

//=============================================================================================================

void FiffAnonymizer::updatePointer(FIFFLIB::FiffStream::SPtr stream,
                                   FIFFLIB::fiff_int_t tagKind,
                                   FIFFLIB::fiff_long_t newPos)
{
    FIFFLIB::fiff_long_t tagInfoSize = 16;

    for(int i=0; i < m_pOutDir->size(); ++i) {
        if(m_pOutDir->at(i).kind != tagKind) {
            stream->device()->seek(m_pOutDir->at(i).pos+tagInfoSize);
            *stream << static_cast<quint32>(newPos);
            break;
        }
    }
}

//=============================================================================================================

bool FiffAnonymizer::checkDeleteInputFile()
{
    if(m_bDeleteInputFileAfter) { //false by default
        qInfo() << "You have requested to delete the input file: " + m_fFileIn.fileName();

        if(m_bDeleteInputFileConfirmation) { //true by default
            QTextStream consoleOut(stdout);
            QTextStream consoleIn(stdin);
            QString confirmation;
            qInfo() << "You can avoid this confirmation by using the delete_confirmation option.";
            consoleOut << "Are you sure you want to delete this file? [Y/n] ";
            consoleIn >> confirmation;

            if(confirmation == "Y") {
                return true;
            }
        } else {
            return true;
        }
    }
    return false;
}

//=============================================================================================================

int FiffAnonymizer::setFileIn(const QString &sFileIn)
{
//    m_sFileNameIn = sFileIn;
    QFileInfo fiIn(sFileIn);
    if(fiIn.exists())
    {
        m_fFileIn.setFileName(fiIn.absoluteFilePath());
        m_bFileInSet = true;
        return 0;
    } else {
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
        QFileInfo fiOut(sFileOut);
        if(fiOut.fileName() == m_fFileIn.fileName())
        {
            m_bInOutFileNamesEqual = true;
            m_fFileOut.setFileName(fiOut.absolutePath() + "/" + generateRandomFileName()); // "/" is system dependant. Qt rocks!
        } else {
            m_bInOutFileNamesEqual = false;
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

void FiffAnonymizer::setVerboseMode(const bool bFlag)
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
    m_dMeasurementDate = QDateTime(QDate::fromString(sMeasDay),QTime(1,1,0));
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
    m_dSubjectBirthday = QDateTime(QDate::fromString(sSubjBirthday),QTime(1, 1, 0));
}

//=============================================================================================================

void FiffAnonymizer::setSubjectBirthday(const QDateTime& sSubjBirthday)
{
    m_dSubjectBirthday = QDateTime(sSubjBirthday);
}

//=============================================================================================================

QDateTime  FiffAnonymizer::getSubjectBirthday()
{
    return m_dSubjectBirthday;
}

//=============================================================================================================

void FiffAnonymizer::setSubjectBirthdayOffset(const int iSubjBirthdayOffset)
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

void FiffAnonymizer::setDeleteInputFileAfter(const bool bFlag)
{
    m_bDeleteInputFileAfter = bFlag;
}

//=============================================================================================================

void FiffAnonymizer::setDeleteInputFileConfirmation(const bool bFlag)
{
    m_bDeleteInputFileConfirmation = bFlag;
}

//=============================================================================================================

void FiffAnonymizer::setSubjectHisId(const QString& sSubjectId)
{
    m_sSubjectHisId = sSubjectId;
}

//=============================================================================================================

QString FiffAnonymizer::generateRandomFileName()
{
    QString randomFileName("mne_anonymize_");
    const QString charPool("abcdefghijklmnopqrstuvwxyz1234567890");
    const int randomLength(12);

    for(int i=0;i<randomLength;++i) {
        int p=qrand() % charPool.length();
        randomFileName.append(charPool.at(p));
    }

    return randomFileName.append(".fif");
}

//=============================================================================================================

void FiffAnonymizer::deleteInputFile()
{
    m_bInputFileDeleted = m_fFileIn.remove();
    printIfVerbose("Input file deleted.");
}

//=============================================================================================================

bool FiffAnonymizer::checkRenameOutputFile()
{
    //if both files in and out have the same name, Anonymizer class would already know and a temporary
    //random filename will be in use, during the anonymizing process, for the output file.
    //When this function is called Anonymizer will check if this needs to be reverted:
    // -if the infile has been deleted already there is no conflict->outfile name = infile name.
    // -if the infile has not been deleted but the user has never been asked. They is asked.
    // -if the infile has not been deleted but the user was already asked, it means they answered NO.
    //  Thus, a warning is shown.
    if(m_bInOutFileNamesEqual) {
        if(m_bDeleteInputFileAfter) {
            if(m_bInputFileDeleted) {
                return true;
            }
        } else {
            m_bDeleteInputFileAfter = true;
            if(checkDeleteInputFile()) {
                deleteInputFile();
                return true;
            } else {
                qWarning() << " ";
                qWarning() << "You have requested to save the output file with the same name as the input file.";
                qWarning() << "This cannot be done without deleting or modifying the input file.";
//                qWarning() << "The output file is: " << m_sFileNameOut;
                qWarning() << " ";
            }
        }
    }

    return false;
}

//=============================================================================================================

void FiffAnonymizer::renameOutputFileAsInputFile()
{
    QString oldName(m_fFileOut.fileName());
    m_fFileOut.rename(m_fFileIn.fileName());
    m_bOutFileRenamed = true;
    printIfVerbose("Output file named: " + oldName + " --> renamed as: " + m_fFileOut.fileName());
}

//=============================================================================================================

int FiffAnonymizer::openInOutStreams()
{
    m_pInStream = FIFFLIB::FiffStream::SPtr (new FIFFLIB::FiffStream(&m_fFileIn));
    if(m_pInStream->device()->open(QIODevice::ReadOnly)) {
        printIfVerbose("Input file opened correctly: " + m_fFileIn.fileName());
    } else {
        qCritical() << "Problem opening the input file: " << m_fFileIn.fileName();
        return 1;
    }

    m_pOutStream = FIFFLIB::FiffStream::SPtr(new FIFFLIB::FiffStream(&m_fFileOut));
    if(m_pOutStream->device()->open(QIODevice::WriteOnly)) {
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
    if(m_pInStream->close()) {
        printIfVerbose("Input file closed. All tags have been correctly anonymized.");
    } else {
        qCritical() << "Problem closing the input file: " << m_fFileIn.fileName();
        return 1;
    }

    if(m_pOutStream->close()) {
        printIfVerbose("Output file closed. All tags have been correctly anonymized.");
    } else {
        qCritical() << "Problem closing the output file: " << m_fFileOut.fileName();
        return 1;
    }

    return 0;

}
