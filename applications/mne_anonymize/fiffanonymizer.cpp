//=============================================================================================================
/**
* @file     fiffanonymizer.cpp
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
* @brief    FiffAnonymizer class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffanonymizer.h"
#include <fiff/fiff_dir_entry.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEANONYMIZE;
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffAnonymizer::FiffAnonymizer()
: maxValidFiffVerion(1.3)
, m_sDefaultString("mne_anonymize")
, m_dateDefaultDate(QDateTime(QDate(2000,1,1), QTime(1, 1, 0)))
, m_dateMeasurmentDate(m_dateDefaultDate)
, m_bUseMeasurementDayOffset(false)
, m_iMeasurementDayOffset(0)
, m_dateSubjectBirthday(m_dateDefaultDate)
, m_bUseSubjectBirthdayOffset(false)
, m_iSubjectBirthdayOffset(0)
, m_iDfltSubjectId(0)
, m_sSubjectFirstName(m_sDefaultString)
, m_sSubjectMidName("x")
, m_sSubjectLastName(m_sDefaultString)
, m_iSubjectWeight(0)
, m_iSubjectHeight(0)
, m_sSubjectComment(m_sDefaultString)
, m_sSubjectHisId("mne_anonymize")
, m_iProjectId(0)
, m_sProjectName(m_sDefaultString)
, m_sProjectAim(m_sDefaultString)
, m_sProjectPersons(m_sDefaultString)
, m_sProjectComment(m_sDefaultString)
, m_bVerboseMode(false)
, m_bBruteMode(false)
, m_bQuietMode(false)
, m_bDeleteInputFileAfter(false)
, m_bDeleteInputFileConfirmation(true)
, m_bInputFileDeleted(false)
, m_bInOutFileNamesEqual(false)
, m_bOutputFileRenamed(false)
, m_sFileNameIn("")
, m_sFileNameOut("")
{
    //MAC addresses have 6 bytes. We use 2 more here to complete 2 int32 (2bytes) reads.
    //check->sometimes MAC address is stored in the 0-5 bytes some other times it
    //is stored in the 2-7 bytes. To do -> Check why!!!
    m_BDfltMAC[0] = 0;
    m_BDfltMAC[1] = 0;

    m_pBlockTypeList = QSharedPointer<QStack<int32_t> >(new QStack<int32_t>);
    m_pOutDir = QSharedPointer<QVector<FiffDirEntry> >(new QVector<FiffDirEntry>);
}


//*************************************************************************************************************

FiffAnonymizer::FiffAnonymizer(const FiffAnonymizer& obj)
: maxValidFiffVerion(obj.maxValidFiffVerion)
, m_sDefaultString(obj.m_sDefaultString)
, m_dateDefaultDate(obj.m_dateDefaultDate)
, m_dateMeasurmentDate(obj.m_dateMeasurmentDate)
, m_bUseMeasurementDayOffset(obj.m_bUseMeasurementDayOffset)
, m_iMeasurementDayOffset(obj.m_iMeasurementDayOffset)
, m_dateSubjectBirthday(obj.m_dateSubjectBirthday)
, m_bUseSubjectBirthdayOffset(obj.m_bUseSubjectBirthdayOffset)
, m_iSubjectBirthdayOffset(obj.m_iSubjectBirthdayOffset)
, m_iDfltSubjectId(obj.m_iDfltSubjectId)
, m_sSubjectFirstName(obj.m_sSubjectFirstName)
, m_sSubjectMidName(obj.m_sSubjectMidName)
, m_sSubjectLastName(obj.m_sSubjectLastName)
, m_iSubjectWeight(obj.m_iSubjectWeight)
, m_iSubjectHeight(obj.m_iSubjectHeight)
, m_sSubjectComment(obj.m_sSubjectComment)
, m_sSubjectHisId(obj.m_sSubjectHisId)
, m_iProjectId(obj.m_iProjectId)
, m_sProjectName(obj.m_sProjectName)
, m_sProjectAim(obj.m_sProjectAim)
, m_sProjectPersons(obj.m_sProjectPersons)
, m_sProjectComment(obj.m_sProjectComment)
, m_bVerboseMode(obj.m_bVerboseMode)
, m_bBruteMode(obj.m_bBruteMode)
, m_bQuietMode(obj.m_bQuietMode)
, m_bDeleteInputFileAfter(obj.m_bDeleteInputFileAfter)
, m_bDeleteInputFileConfirmation(obj.m_bDeleteInputFileConfirmation)
, m_bInputFileDeleted(obj.m_bInputFileDeleted)
, m_bInOutFileNamesEqual(obj.m_bInOutFileNamesEqual)
, m_bOutputFileRenamed(obj.m_bOutputFileRenamed)
, m_sFileNameIn(obj.m_sFileNameIn)
, m_sFileNameOut(obj.m_sFileNameOut)
{
    m_BDfltMAC[0]=obj.m_BDfltMAC[0];
    m_BDfltMAC[1]=obj.m_BDfltMAC[1];

    m_pBlockTypeList = QSharedPointer<QStack<int32_t> >(new QStack<int32_t>(*obj.m_pBlockTypeList));
    m_pOutDir = QSharedPointer<QVector<FiffDirEntry> >(new QVector<FiffDirEntry>);
}


//*************************************************************************************************************

FiffAnonymizer::FiffAnonymizer(FiffAnonymizer &&obj)
: maxValidFiffVerion(obj.maxValidFiffVerion)
, m_sDefaultString(obj.m_sDefaultString)
, m_dateDefaultDate(obj.m_dateDefaultDate)
, m_dateMeasurmentDate(obj.m_dateMeasurmentDate)
, m_bUseMeasurementDayOffset(obj.m_bUseMeasurementDayOffset)
, m_iMeasurementDayOffset(obj.m_iMeasurementDayOffset)
, m_dateSubjectBirthday(obj.m_dateSubjectBirthday)
, m_bUseSubjectBirthdayOffset(obj.m_bUseSubjectBirthdayOffset)
, m_iSubjectBirthdayOffset(obj.m_iSubjectBirthdayOffset)
, m_iDfltSubjectId(obj.m_iDfltSubjectId)
, m_sSubjectFirstName(obj.m_sSubjectFirstName)
, m_sSubjectMidName(obj.m_sSubjectMidName)
, m_sSubjectLastName(obj.m_sSubjectLastName)
, m_iSubjectWeight(obj.m_iSubjectWeight)
, m_iSubjectHeight(obj.m_iSubjectHeight)
, m_sSubjectComment(obj.m_sSubjectComment)
, m_sSubjectHisId(obj.m_sSubjectHisId)
, m_iProjectId(obj.m_iProjectId)
, m_sProjectName(obj.m_sProjectName)
, m_sProjectAim(obj.m_sProjectAim)
, m_sProjectPersons(obj.m_sProjectPersons)
, m_sProjectComment(obj.m_sProjectComment)
, m_bVerboseMode(obj.m_bVerboseMode)
, m_bBruteMode(obj.m_bBruteMode)
, m_bQuietMode(obj.m_bQuietMode)
, m_bDeleteInputFileAfter(obj.m_bDeleteInputFileAfter)
, m_bDeleteInputFileConfirmation(obj.m_bDeleteInputFileConfirmation)
, m_bInputFileDeleted(obj.m_bInputFileDeleted)
, m_bInOutFileNamesEqual(obj.m_bInOutFileNamesEqual)
, m_bOutputFileRenamed(obj.m_bOutputFileRenamed)
, m_sFileNameIn(obj.m_sFileNameIn)
, m_sFileNameOut(obj.m_sFileNameOut)
{
    m_BDfltMAC[0]=obj.m_BDfltMAC[0];
    m_BDfltMAC[1]=obj.m_BDfltMAC[1];

    m_pBlockTypeList.swap(obj.m_pBlockTypeList);
    m_pOutDir.swap(obj.m_pOutDir);
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GETTER METHODS
//=============================================================================================================

QString FiffAnonymizer::getDefaultString()
{
    return m_sDefaultString;
}

//*************************************************************************************************************

QDateTime FiffAnonymizer::getDefaultDate()
{
    return m_dateDefaultDate;
}

//*************************************************************************************************************

QDateTime FiffAnonymizer::getMeasurementDate()
{
    return m_dateMeasurmentDate;
}

//*************************************************************************************************************

bool FiffAnonymizer::getUseMeasurementDayOffset()
{
    return m_bUseMeasurementDayOffset;
}

//*************************************************************************************************************

int  FiffAnonymizer::getIntMeasurementDayOffset()
{
    return m_iMeasurementDayOffset;
}

//*************************************************************************************************************

QDateTime FiffAnonymizer::getSubjectBirthday()
{
    return m_dateSubjectBirthday;
}

//*************************************************************************************************************

bool FiffAnonymizer::getUseSubjectBirthdayOffset()
{
    return m_bUseSubjectBirthdayOffset;
}

//*************************************************************************************************************

int  FiffAnonymizer::getIntSubjectBirthdayOffset()
{
    return m_iSubjectBirthdayOffset;
}

//*************************************************************************************************************

void FiffAnonymizer::getDefaultMAC(fiff_int_t (&mac)[2])
{
    mac[0] = m_BDfltMAC[0];
    mac[1] = m_BDfltMAC[1];
}

//*************************************************************************************************************

int FiffAnonymizer::getDefaultSubjectId()
{
    return m_iDfltSubjectId;
}

//*************************************************************************************************************

QString FiffAnonymizer::getDefaultSubjectFirstName()
{
    return m_sSubjectFirstName;
}

//*************************************************************************************************************

QString FiffAnonymizer::getDefaultSubjectMidName()
{
    return m_sSubjectMidName;
}

//*************************************************************************************************************

QString FiffAnonymizer::getDefaultSubjectLastName()
{
    return m_sSubjectLastName;
}

//*************************************************************************************************************

int FiffAnonymizer::getDefaultSubjectWeight()
{
    return m_iSubjectWeight;
}

//*************************************************************************************************************

int FiffAnonymizer::getDefaultSubjectHeight()
{
    return m_iSubjectHeight;
}

//*************************************************************************************************************

QString FiffAnonymizer::getDefaultSubjectComment()
{
    return m_sSubjectComment;
}

//*************************************************************************************************************

QString FiffAnonymizer::getDefaultSubjectHisId()
{
    return m_sSubjectHisId;
}

//*************************************************************************************************************

int FiffAnonymizer::getDefaultProjectId()
{
    return m_iProjectId;
}

//*************************************************************************************************************

QString FiffAnonymizer::getDefaultProjectName()
{
    return m_sProjectName;
}

//*************************************************************************************************************

QString FiffAnonymizer::getDefaultProjectAim()
{
    return m_sProjectAim;
}

//*************************************************************************************************************

QString FiffAnonymizer::getDefaultProjectPersons()
{
    return m_sProjectPersons;
}

//*************************************************************************************************************

QString FiffAnonymizer::getDefaultProjectComment()
{
    return m_sProjectComment;
}

//*************************************************************************************************************

bool FiffAnonymizer::getBruteMode()
{
    return m_bBruteMode;
}

//*************************************************************************************************************

bool FiffAnonymizer::getQuietMode()
{
    return m_bQuietMode;
}

//*************************************************************************************************************

bool FiffAnonymizer::getDeleteInputFileAfter()
{
    return m_bDeleteInputFileAfter;
}

//*************************************************************************************************************

bool FiffAnonymizer::getDeleteInputFileConfirmation()
{
    return m_bDeleteInputFileConfirmation;
}

//*************************************************************************************************************

bool FiffAnonymizer::getInputFileDeleted()
{
    return m_bInputFileDeleted;
}

//*************************************************************************************************************

bool FiffAnonymizer::getInOutFileNamesEqual()
{
    return m_bInOutFileNamesEqual;
}

//*************************************************************************************************************

bool FiffAnonymizer::getOutputFileRenamed()
{
    return m_bOutputFileRenamed;
}

//*************************************************************************************************************

QString FiffAnonymizer::getFileNameIn()
{
    return m_sFileNameIn;
}

//*************************************************************************************************************

QString FiffAnonymizer::getsFileNameOut()
{
    return m_sFileNameOut;
}


//*************************************************************************************************************

int FiffAnonymizer::anonymizeFile()
{
    printIfVerbose("Max. Valid Fiff version: " + QString::number(maxValidFiffVerion));
    printIfVerbose("Current date: " + QDateTime::currentDateTime().toString("ddd MMMM d yyyy hh:mm:ss"));
    printIfVerbose(" ");

    FiffStream inStream(&m_fFileIn);
    if(inStream.open(QIODevice::ReadOnly))
    {
        printIfVerbose("Input file opened correctly: " + m_fFileIn.fileName());
    } else {
        qCritical() << "Problem opening the input file: " << m_fFileIn.fileName();
        return 1;
    }

    FiffStream outStream(&m_fFileOut);
    if(outStream.device()->open(QIODevice::WriteOnly)) {
        printIfVerbose("Output file opened correctly: " + m_fFileOut.fileName());
    } else {
        qCritical() << "Problem opening the output file: " << m_fFileOut.fileName();
        return 1;
    }

    FiffTag::SPtr pInTag = FiffTag::SPtr::create();
    FiffTag::SPtr pOutTag = FiffTag::SPtr::create();

    inStream.read_tag(pInTag,0);

    //info in a tag FIFF_COMMENT (206) depends on the type of block it is in. Therefore, in order to
    //anonymize it we not only have to know the kind of the current tag, but also which type of block
    //we're currently in. BlockTypeList is a stack container used to keep track of this.
    m_pBlockTypeList->clear();
    updateBlockTypeList(pInTag);

    printIfVerbose("Reading info in the file.");
    if(checkValidFiffFormatVersion(pInTag)) {
        printIfVerbose("Input file compatible with this version of mne_fiffAnonymizer.");
    } else {
        printIfVerbose("***");
        printIfVerbose("***   Warning: This file may not be compatible with this application.");
        printIfVerbose("***");
    }
    censorTag(pOutTag,pInTag);
    pOutTag->next = FIFFV_NEXT_SEQ;

    //we build the tag directory on the go
    addEntryToDir(pOutTag,outStream.device()->pos());
    FiffTag::convert_tag_data(pOutTag,FIFFV_NATIVE_ENDIAN,FIFFV_BIG_ENDIAN);
    outStream.write_tag(pOutTag, 0);

    // Set FIFF_DIR_POINTER tag to -1
    inStream.read_tag(pInTag);
    if (pInTag->kind != FIFF_DIR_POINTER) {
        qCritical() << "File does have a directory pointer: " << m_fFileOut.fileName();
        return 1;
    }

    pOutTag->kind = FIFF_DIR_POINTER;
    pOutTag->type = pInTag->type;
    pOutTag->next = FIFFV_NEXT_SEQ;
    qint32 iFiffDirPos = -1;
    pOutTag->resize(sizeof(iFiffDirPos));
    memcpy(pOutTag->data(),&iFiffDirPos,sizeof(iFiffDirPos));
    addEntryToDir(pOutTag,outStream.device()->pos());
    FiffTag::convert_tag_data(pOutTag,FIFFV_NATIVE_ENDIAN,FIFFV_BIG_ENDIAN);
    outStream.write_tag(pOutTag);

    // Set FIFF_FREE_LIST tag to -1
    inStream.read_tag(pInTag);
    if (pInTag->kind != FIFF_FREE_LIST) {
        qCritical() << "File does have a free list pointer: " << m_fFileOut.fileName();
        return 1;
    }

    pOutTag->kind = FIFF_FREE_LIST;
    pOutTag->type = pInTag->type;
    pOutTag->next = FIFFV_NEXT_SEQ;
    pOutTag->resize(sizeof(iFiffDirPos));
    memcpy(pOutTag->data(),&iFiffDirPos,sizeof(iFiffDirPos));
    addEntryToDir(pOutTag,outStream.device()->pos());
    FiffTag::convert_tag_data(pOutTag,FIFFV_NATIVE_ENDIAN,FIFFV_BIG_ENDIAN);
    outStream.write_tag(pOutTag);

    while(pInTag->next != -1) {
        inStream.read_tag(pInTag);

        updateBlockTypeList(pInTag);
        censorTag(pOutTag,pInTag);

        //the order of the tags in the output file is sequential. No jumps in the output file.
        if(pOutTag->next > 0)
        {
            pOutTag->next = FIFFV_NEXT_SEQ;
        }
        addEntryToDir(pOutTag,outStream.device()->pos());
        FiffTag::convert_tag_data(pOutTag,FIFFV_NATIVE_ENDIAN,FIFFV_BIG_ENDIAN);
        outStream.write_tag(pOutTag);
    }

    if(inStream.close()) {
        printIfVerbose("Input file closed. All tags have been correctly anonymized.");
    } else {
        qCritical() << "Problem closing the input file: " << m_fFileIn.fileName();
    }

    addFinalEntryToDir();
//    fiff_long_t posOfDirectory(outStream.device()->pos());
//    writeDirectory(&outStream);
//    updatePointer(&outStream,FIFF_DIR_POINTER,posOfDirectory);
//    updatePointer(&outStream,FIFF_FREE_LIST,-1);

    if(outStream.close()) {
        printIfVerbose("Output file closed. All tags have been correctly anonymized.");
    } else {
        qCritical() << "Problem closing the output file: " << m_fFileOut.fileName();
    }

    if(checkDeleteInputFile()) {
        deleteInputFile();
    }

    if(checkRenameOutputFile()) {
        renameOutputFileAsInputFile();
    }

    if(!m_bQuietMode) {
        qDebug() << "MNE Fiff Anonymize finished correctly: " + QFileInfo(m_fFileIn).fileName() + " -> " + QFileInfo(m_fFileOut).fileName();
    }

    printIfVerbose(" ");
    printIfVerbose("----------------------------------------------------------------------------");
    printIfVerbose(" ");

    return 0;
}


//*************************************************************************************************************

void FiffAnonymizer::updateBlockTypeList(FiffTag::SPtr pTag)
{
    if(pTag->kind == FIFF_BLOCK_START) {
        m_pBlockTypeList->push(*pTag->toInt());
    }

    if(pTag->kind == FIFF_BLOCK_END) {
        m_pBlockTypeList->pop();
    }
}


//*************************************************************************************************************

bool FiffAnonymizer::checkValidFiffFormatVersion(FiffTag::SPtr pTag)
{
    if(pTag->kind == FIFF_FILE_ID) {
        FiffId fileId = pTag->toFiffID();
        int inMayorVersion = (static_cast<uint32_t>(fileId.version) & 0xFFFF0000) >> 16;
        int inMinorVersion = (static_cast<uint32_t>(fileId.version) & 0x0000FFFF);
        double inVersion = inMayorVersion + inMinorVersion/10.0;

        if(inVersion > maxValidFiffVerion) {
            return false;
        }
    }
    return true;
}


//*************************************************************************************************************

void FiffAnonymizer::addEntryToDir(FiffTag::SPtr pTag,
                                   qint64 filePos)
{
    FiffDirEntry t_dirEntry;
    t_dirEntry.kind = pTag->kind;
    t_dirEntry.type = pTag->type;
    t_dirEntry.size = pTag->size();
    t_dirEntry.pos  = static_cast<fiff_int_t>(filePos);
    m_pOutDir->append(t_dirEntry);
}


//*************************************************************************************************************

void FiffAnonymizer::addFinalEntryToDir()
{
    FiffDirEntry t_dirEntry;
    t_dirEntry.kind = -1;
    t_dirEntry.type = -1;
    t_dirEntry.size = -1;
    t_dirEntry.pos  = -1;
    m_pOutDir->append(t_dirEntry);
}


//*************************************************************************************************************

void FiffAnonymizer::dir2tag(FiffTag::SPtr pTag)
{
    pTag->kind = FIFF_DIR;
    pTag->type = FIFFT_DIR_ENTRY_STRUCT;
    pTag->next = -1;
    pTag->resize(m_pOutDir->size() * 4  * 4);
    pTag->clear();

    if(m_pOutDir->size() > 0) {
        QByteArray pInt8(sizeof(fiff_int_t),0);
        for(int i=0;i<m_pOutDir->size();++i) {
            memcpy(&pInt8,reinterpret_cast<char*>(m_pOutDir->at(i).kind),sizeof(fiff_int_t));
            pTag->append(pInt8);
            memcpy(&pInt8,reinterpret_cast<char*>(m_pOutDir->at(i).type),sizeof(fiff_int_t));
            pTag->append(pInt8);
            memcpy(&pInt8,reinterpret_cast<char*>(m_pOutDir->at(i).size),sizeof(fiff_int_t));
            pTag->append(pInt8);
            memcpy(&pInt8,reinterpret_cast<char*>(m_pOutDir->at(i).pos),sizeof(fiff_int_t));
            pTag->append(pInt8);
        }
    }
}


//*************************************************************************************************************

void FiffAnonymizer::writeDirectory(FiffStream* stream,
                                    fiff_long_t pos)
{
    if(pos>=0)
        stream->device()->seek(pos);
    else {
        QFile* file=qobject_cast<QFile*>(stream->device());
        if(file)
            stream->device()->seek(file->size());
    }

    *stream << static_cast<quint32>(FIFF_DIR);
    *stream << static_cast<quint32>(FIFFT_DIR_ENTRY_STRUCT);
    *stream << static_cast<quint32>
               (static_cast<unsigned long long>(m_pOutDir->size())*sizeof(FiffDirEntry));
    *stream << static_cast<quint32>(-1);
    for(int i=0;i<m_pOutDir->size();++i) {
        *stream << static_cast<quint32>(m_pOutDir->at(i).kind);
        *stream << static_cast<quint32>(m_pOutDir->at(i).type);
        *stream << static_cast<quint32>(m_pOutDir->at(i).size);
        *stream << static_cast<quint32>(m_pOutDir->at(i).pos);
    }
}


//*************************************************************************************************************

void FiffAnonymizer::updatePointer(FiffStream* stream,
                                   fiff_int_t tagKind,
                                   fiff_long_t newPos)
{
    fiff_long_t tagInfoSize = 16;

    for(int i=0; i < m_pOutDir->size(); ++i) {
        if(m_pOutDir->at(i).kind != tagKind) {
            stream->device()->seek(m_pOutDir->at(i).pos+tagInfoSize);
            *stream << static_cast<quint32>(newPos);
            break;
        }
    }
}


//*************************************************************************************************************

bool FiffAnonymizer::checkDeleteInputFile()
{
    if(m_bDeleteInputFileAfter) { //false by default
        qDebug() << "You have requested to delete the input file: " + m_fFileIn.fileName();

        if(m_bDeleteInputFileConfirmation) { //true by default
            QTextStream consoleOut(stdout);
            QTextStream consoleIn(stdin);
            QString confirmation;
            qDebug() << "You can avoid this confirmation by using the delete_confirmation option.";
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


//*************************************************************************************************************

int FiffAnonymizer::censorTag(FiffTag::SPtr outTag,FiffTag::SPtr inTag)
{
    int sizeDiff(0);

    outTag->kind = inTag->kind;
    outTag->type = inTag->type;
    outTag->next = inTag->next;

    switch (inTag->kind) {
    //all these 'kinds' of tags contain a fileID struct, which contains info related to
    //measurement date
    case FIFF_FILE_ID:
    case FIFF_BLOCK_ID:
    case FIFF_PARENT_FILE_ID:
    case FIFF_PARENT_BLOCK_ID:
    case FIFF_REF_FILE_ID:
    case FIFF_REF_BLOCK_ID:
    {
        FiffId inId = inTag->toFiffID();
        QDateTime inMeasDate = QDateTime::fromSecsSinceEpoch(inId.time.secs);
        QDateTime outMeasDate;
        if(m_bUseMeasurementDayOffset) {
            outMeasDate = inMeasDate.addDays(-m_iMeasurementDayOffset);
        } else {
            outMeasDate = m_dateMeasurmentDate;
        }

        FiffId outId(inId);
        outId.machid[0] = m_BDfltMAC[0];
        outId.machid[1] = m_BDfltMAC[1];
        outId.time.secs = static_cast<int32_t>(outMeasDate.toSecsSinceEpoch());
        outId.time.usecs = 0;

        const int fiffIdSize(sizeof(inId)/sizeof(fiff_int_t));
        fiff_int_t outData[fiffIdSize];
        outData[0] = outId.version;
        outData[1] = outId.machid[0];
        outData[2] = outId.machid[1];
        outData[3] = outId.time.secs;
        outData[4] = outId.time.usecs;

        outTag->resize(fiffIdSize*sizeof(fiff_int_t));
        memcpy(outTag->data(),reinterpret_cast<char*>(outData),fiffIdSize*sizeof(fiff_int_t));
        printIfVerbose("MAC address changed: " + inId.toMachidString() + " -> "  + outId.toMachidString());
        printIfVerbose("Measurement date changed: " + inMeasDate.toString() + " -> " + outMeasDate.toString());
        break;
    }
    case FIFF_MEAS_DATE:
    {
        QDateTime inMeasDate(QDateTime::fromSecsSinceEpoch(*inTag->toInt()));
        QDateTime outMeasDate;
        if(m_bUseMeasurementDayOffset) {
            outMeasDate = QDateTime(inMeasDate.date()).addDays(-m_iMeasurementDayOffset);
        } else {
            outMeasDate = m_dateMeasurmentDate;
        }

        fiff_int_t outData[1];
        outData[0]=static_cast<int32_t>(outMeasDate.toSecsSinceEpoch());
        memcpy(outTag->data(),reinterpret_cast<char*>(outData),sizeof(fiff_int_t));
        printIfVerbose("Measurement date changed: " + inMeasDate.toString() + " -> " + outMeasDate.toString());
        break;
    }
    case FIFF_COMMENT:
    {
        if(m_pBlockTypeList->first()==FIFFB_MEAS_INFO) {
            QString newStr(m_sDefaultString);
            outTag->resize(newStr.size());
            memcpy(outTag->data(),newStr.toUtf8(),static_cast<size_t>(newStr.size()));
            printIfVerbose("Description of the measurement block changed: " +
                           QString(inTag->data()) + " -> " + newStr);
        }
        break;
    }
    case FIFF_EXPERIMENTER:
    {
        QString newStr(m_sDefaultString);
        outTag->resize(newStr.size());
        memcpy(outTag->data(),newStr.toUtf8(),static_cast<size_t>(newStr.size()));
        printIfVerbose("Experimenter changed: " + QString(inTag->data()) + " -> " + newStr);
        break;
    }
    case FIFF_SUBJ_ID:
    {
        qint32 inSubjID(*inTag->toInt());
        qint32 newSubjID(m_iDfltSubjectId);
        memcpy(outTag->data(),&newSubjID, sizeof(qint32));
        printIfVerbose("Subject's SubjectID changed: " +
                       QString::number(inSubjID) + " -> " + QString::number(newSubjID));
        break;
    }
    case FIFF_SUBJ_FIRST_NAME:
    {
        QString newStr(m_sSubjectFirstName);
        outTag->resize(newStr.size());
        memcpy(outTag->data(),newStr.toUtf8(),static_cast<size_t>(newStr.size()));
        printIfVerbose("Experimenter changed: " +
                       QString(inTag->data()) + " -> " + newStr);
        break;
    }
    case FIFF_SUBJ_MIDDLE_NAME:
    {
        QString newStr(m_sSubjectMidName);
        outTag->resize(newStr.size());
        memcpy(outTag->data(),newStr.toUtf8(),static_cast<size_t>(newStr.size()));
        printIfVerbose("Experimenter changed: " +
                       QString(inTag->data()) + " -> " + newStr);
        break;
    }
    case FIFF_SUBJ_LAST_NAME:
    {
        QString newStr(m_sSubjectLastName);
        outTag->resize(newStr.size());
        memcpy(outTag->data(),newStr.toUtf8(),static_cast<size_t>(newStr.size()));
        printIfVerbose("Experimenter changed: " +
                       QString(inTag->data()) + " -> " + newStr);
        break;
    }
    case FIFF_SUBJ_BIRTH_DAY:
    {
        QDateTime inBirthday(QDate::fromJulianDay(*inTag->toJulian()));

        qDebug() << "*inTag->toJulian()" << *inTag->toJulian();

        QDateTime outBirthday;

        if(m_bUseSubjectBirthdayOffset) {
            outBirthday = inBirthday.addDays(-m_iMeasurementDayOffset);
        } else {
            outBirthday = m_dateSubjectBirthday;
        }

        fiff_int_t outData[1];
        outData[0] = static_cast<int32_t> (outBirthday.toSecsSinceEpoch());
        memcpy(outTag->data(),reinterpret_cast<char*>(outData),sizeof(fiff_int_t));
        printIfVerbose("Subject birthday date changed: " + inBirthday.toString() + " -> " + outBirthday.toString());

        break;
    }
    case FIFF_SUBJ_WEIGHT:
    {
        if(m_bBruteMode)
        {
            float inWeight(*inTag->toFloat());
            float outWeight(m_iSubjectWeight);
            memcpy(outTag->data(),&outWeight,sizeof(float));
            printIfVerbose("Subject's weight changed from: " +
                           QString::number(static_cast<double>(inWeight)) + " -> " + QString::number(static_cast<double>(outWeight)));
        }
        break;
    }
    case FIFF_SUBJ_HEIGHT:
    {
        if(m_bBruteMode)
        {
            float inHeight(*inTag->toFloat());
            float outHeight(m_iSubjectHeight);
            memcpy(outTag->data(),&outHeight,sizeof(float));
            printIfVerbose("Subject's Height changed from: " +
                           QString::number(static_cast<double>(inHeight)) + " -> " + QString::number(static_cast<double>(outHeight)));
        }
        break;
    }
    case FIFF_SUBJ_COMMENT:
    {
        QString newStr(m_sSubjectComment);
        outTag->resize(newStr.size());
        memcpy(outTag->data(),newStr.toUtf8(),static_cast<size_t>(newStr.size()));
        printIfVerbose("Subject Comment changed: " +
                       QString(inTag->data()) + " -> " + newStr);
        break;
    }
    case FIFF_SUBJ_HIS_ID:
    {
        QString inSubjectHisId(inTag->data());
        QString newSubjectHisId(m_sSubjectHisId);
        outTag->resize(newSubjectHisId.size());
        memcpy(outTag->data(),newSubjectHisId.toUtf8(),static_cast<size_t>(newSubjectHisId.size()));
        printIfVerbose("Subject Hospital-ID changed:" + inSubjectHisId + " -> " + newSubjectHisId);
        break;
    }
    case FIFF_PROJ_ID:
    {
        if(m_bBruteMode)
        {
            qint32 inProjID(*inTag->toInt());
            qint32 newProjID(m_iProjectId);
            memcpy(outTag->data(),&newProjID,sizeof(qint32));
            printIfVerbose("ProjectID changed: " +
                           QString::number(inProjID) + " -> " + QString::number(newProjID));
        }
        break;
    }
    case FIFF_PROJ_NAME:
    {
        if(m_bBruteMode)
        {
                QString newStr(m_sProjectName);
                outTag->resize(newStr.size());
                memcpy(outTag->data(),newStr.toUtf8(),static_cast<size_t>(newStr.size()));
                printIfVerbose("Project name changed: " +
                               QString(inTag->data()) + " -> " + newStr);
        }
        break;
    }
    case FIFF_PROJ_AIM:
    {
        if(m_bBruteMode)
        {
            QString newStr(m_sProjectAim);
            outTag->resize(newStr.size());
            memcpy(outTag->data(),newStr.toUtf8(),static_cast<size_t>(newStr.size()));
            printIfVerbose("Project Aim changed: " +
                           QString(inTag->data()) + " -> " + newStr);
        }
        break;
    }
    case FIFF_PROJ_PERSONS:
    {
        QString newStr(m_sProjectPersons);
        outTag->resize(newStr.size());
        memcpy(outTag->data(),newStr.toUtf8(),static_cast<size_t>(newStr.size()));
        printIfVerbose("Project Persons changed: " +
                       QString(inTag->data()) + " -> " + newStr);
        break;
    }
    case FIFF_PROJ_COMMENT:
    {
        if(m_bBruteMode)
        {
            QString newStr(m_sProjectComment);
            outTag->resize(newStr.size());
            memcpy(outTag->data(),newStr.toUtf8(),static_cast<size_t>(newStr.size()));
            printIfVerbose("Project comment changed: " +
                           QString(inTag->data()) + " -> " + newStr);
        }
        break;
    }
    case FIFF_MRI_PIXEL_DATA:
    {
        if(!m_bQuietMode) {
            qDebug() << " ";
            qDebug() << "WARNING. The input fif file contains MRI data.";
            qDebug() << "Beware that a subject''s face can be reconstructed from it";
            qDebug() << "This software can not anonymize MRI data, at the moment.";
            qDebug() << "Contanct the authors for more information.";
            qDebug() << " ";
        }
        break;
    }
    default:
    {
        outTag->resize(inTag->size());
        memcpy(outTag->data(),inTag->data(),static_cast<size_t>(inTag->size()));
    }

    }

    sizeDiff = outTag->size() - inTag->size();

    return sizeDiff;
}


//*************************************************************************************************************

void FiffAnonymizer::setFileIn(const QString &sFileIn)
{
    m_sFileNameIn = sFileIn;
    m_fFileIn.setFileName(sFileIn);
}


//*************************************************************************************************************

void FiffAnonymizer::setFileOut(const QString &sFileOut)
{
    if(m_fFileOut.fileName().isEmpty()) {
        m_sFileNameOut = sFileOut;
    } else {
        if(m_fFileIn.fileName().compare(sFileOut,Qt::CaseInsensitive) == 0) {
            m_bInOutFileNamesEqual = true;
            QFileInfo outFileInfo(sFileOut);
            outFileInfo.makeAbsolute();
            m_sFileNameOut = outFileInfo.absolutePath() + generateRandomFileName();
        } else {
            m_bInOutFileNamesEqual = false;
            m_sFileNameOut = sFileOut;
        }
    }

    m_fFileOut.setFileName(m_sFileNameOut);
}


//*************************************************************************************************************

void FiffAnonymizer::setVerboseMode(bool v)
{
    m_bVerboseMode = v;
}


//*************************************************************************************************************

bool FiffAnonymizer::getVerboseMode()
{
    return m_bVerboseMode;
}


//*************************************************************************************************************
void FiffAnonymizer::setBruteMode(bool b)
{
    m_bBruteMode = b;
}


//*************************************************************************************************************

void FiffAnonymizer::setQuietMode(bool q)
{
    if(q) {
        m_bVerboseMode = false;
    }
    m_bQuietMode = q;
}


//*************************************************************************************************************

void FiffAnonymizer::setMeasurementDay(QString d)
{
    m_dateMeasurmentDate = QDateTime(QDate::fromString(d),QTime(1,1,0));
}


//*************************************************************************************************************

void FiffAnonymizer::setMeasurementDayOffset(int d)
{
    m_bUseMeasurementDayOffset = true;
    m_iMeasurementDayOffset = d;
}


//*************************************************************************************************************

void FiffAnonymizer::setSubjectBirthday(QString d)
{
    m_dateSubjectBirthday = QDateTime(QDate::fromString(d),QTime(1, 1, 0));
}


//*************************************************************************************************************

void FiffAnonymizer::setSubjectBirthdayOffset(int d)
{
    m_bUseSubjectBirthdayOffset= true;
    m_iSubjectBirthdayOffset= d;
}


//*************************************************************************************************************


void FiffAnonymizer::setDeleteInputFileAfter(bool d)
{
    m_bDeleteInputFileAfter = d;
}


//*************************************************************************************************************

void FiffAnonymizer::setDeleteInputFileAfterConfirmation(bool dc)
{
    m_bDeleteInputFileConfirmation = dc;
}


//*************************************************************************************************************

QString FiffAnonymizer::generateRandomFileName()
{
    QString randomFileName("mne_anonymize_");
    const QString charPool("abcdefghijklmnopqrstuvwxyz1234567890");
    const int randomLength(8);
    for(int i=0;i<randomLength;++i)
    {
        int p=qrand() % charPool.length();
        randomFileName.append(charPool.at(p));
    }
    return randomFileName.append(".fif");
}


//*************************************************************************************************************

void FiffAnonymizer::deleteInputFile()
{
    m_bInputFileDeleted = m_fFileIn.remove();
    printIfVerbose("Input file deleted.");
}


//*************************************************************************************************************

//if both files in and out have the same name, Anonymizer class would already know and a temporary
//random filename will be in use, during the anonymizing process, for the outputfile.
//When this fucn is called Anonymizer will check if this needs to be reverted:
// -if the infile has been deleted already there is no conflict->outfile name = infile name.
// -if the infile has not been deleted but the user has never been asked. They is asked.
// -if the infile has not been deleted but the user was already asked, it means they answered NO.
//  Thus, a warning is shown.
bool FiffAnonymizer::checkRenameOutputFile()
{
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
                qWarning() << "The output file is: " << m_sFileNameOut;
                qWarning() << " ";
            }
        }
    }

    return false;
}


//*************************************************************************************************************

void FiffAnonymizer::renameOutputFileAsInputFile()
{
    m_fFileOut.rename(m_sFileNameIn);
    m_bOutputFileRenamed = true;
    printIfVerbose("Output file named: " + m_sFileNameOut + " --> renamed as: " + m_sFileNameIn);
}


//*************************************************************************************************************

void FiffAnonymizer::setSubjectHisId(QString id)
{
    m_sSubjectHisId = id;
}

