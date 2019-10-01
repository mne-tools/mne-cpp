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
//========================================================= ====================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffAnonymizer::FiffAnonymizer()
    : version(1.0)
    , maxValidFiffVerion(1.3)
    , versionStr(QString::number(version,'f',1))
    , name("MNE Anonymize")
    , description("Application that removes or modifies Personal Health Information or Personal Identifiable information.")
    , m_bVerboseMode(false)
    , m_bBruteMode(false)
    , m_bQuietMode(false)
    , m_bDeleteInputFileAfter(false)
    , m_bDeleteInputFileConfirmation(true)
    , m_bInputFileDeleted(false)
    , m_bInOutFileNamesEqual(false)
    , m_bOutputFileRenamed(false)
    , m_sDfltString("mne_anonymize")
    , m_dateDfltDate(QDateTime(QDate(2000,1,1), QTime(1, 1, 0)))
    , m_dateMeasurmentDate(m_dateDfltDate)
    , m_bUseMeasurementDayOffset(false)
    , m_iMeasurementDayOffset(0)
    , m_dateSubjectBirthday(m_dateDfltDate)
    , m_bUseSubjectBirthdayOffset(false)
    , m_iSubjectBirthdayOffset(0)
    , m_iDfltSubjectId(0)
    , m_sDfltSubjectFirstName(m_sDfltString)
    , m_sDfltSubjectMidName("x")
    , m_sDfltSubjectLastName(m_sDfltString)
    , m_iDfltSubjectWeight(0)
    , m_iDfltSubjectHeight(0)
    , m_sDfltSubjectComment(m_sDfltString)
    , m_iDfltSubjectHisId(0)
    , m_iDfltProjectId(0)
    , m_sDfltProjectName(m_sDfltString)
    , m_sDfltProjectAim(m_sDfltString)
    , m_sDfltProjectPersons(m_sDfltString)
    , m_sDfltProjectComment(m_sDfltString)
    , m_sFileNameIn("")
    , m_sFileNameOut("")
    , m_printInSameLineHelper(qDebug())
    , m_bPrintInSameLine(true)
{
    //MAC addresses have 6 bytes. We use 2 more here to complete 2 int32 (2bytes) reads.
    //check->sometimes MAC address is stored in the 0-5 bytes some other times it
    //is stored in the 2-7 bytes. To do -> Check why!!!
    m_BDfltMAC.resize(8);
    m_BDfltMAC[0] = 0x00;
    m_BDfltMAC[1] = 0x00;
    m_BDfltMAC[2] = 0x00;
    m_BDfltMAC[3] = 0x00;
    m_BDfltMAC[4] = 0x00;
    m_BDfltMAC[5] = 0x00;
    m_BDfltMAC[6] = 0x00;
    m_BDfltMAC[7] = 0x00;

    m_pBlockTypeList = QSharedPointer<QStack<int32_t> >(new QStack<int32_t>);
    m_pOutDir = QSharedPointer<QVector<FiffDirEntry> >(new QVector<FiffDirEntry>);
}


//*************************************************************************************************************

FiffAnonymizer::FiffAnonymizer(const FiffAnonymizer& obj)
    : version(obj.version)
    , maxValidFiffVerion(obj.maxValidFiffVerion)
    , versionStr(QString::number(obj.version))
    , name(obj.name)
    , description(obj.description)
    , m_bVerboseMode(obj.m_bVerboseMode)
    , m_bBruteMode(obj.m_bBruteMode)
    , m_bQuietMode(obj.m_bQuietMode)
    , m_bDeleteInputFileAfter(obj.m_bDeleteInputFileAfter)
    , m_bDeleteInputFileConfirmation(obj.m_bDeleteInputFileConfirmation)
    , m_bInputFileDeleted(obj.m_bInputFileDeleted)
    , m_bInOutFileNamesEqual(obj.m_bInOutFileNamesEqual)
    , m_bOutputFileRenamed(obj.m_bOutputFileRenamed)
    , m_sDfltString(obj.m_sDfltString)
    , m_dateDfltDate(obj.m_dateDfltDate)
    , m_dateMeasurmentDate(obj.m_dateMeasurmentDate)
    , m_bUseMeasurementDayOffset(obj.m_bUseMeasurementDayOffset)
    , m_iMeasurementDayOffset(obj.m_iMeasurementDayOffset)
    , m_dateSubjectBirthday(obj.m_dateSubjectBirthday)
    , m_bUseSubjectBirthdayOffset(obj.m_bUseSubjectBirthdayOffset)
    , m_iSubjectBirthdayOffset(obj.m_iSubjectBirthdayOffset)
    , m_BDfltMAC(obj.m_BDfltMAC)
    , m_iDfltSubjectId(obj.m_iDfltSubjectId)
    , m_sDfltSubjectFirstName(obj.m_sDfltSubjectFirstName)
    , m_sDfltSubjectMidName(obj.m_sDfltSubjectMidName)
    , m_sDfltSubjectLastName(obj.m_sDfltSubjectLastName)
    , m_iDfltSubjectWeight(obj.m_iDfltSubjectWeight)
    , m_iDfltSubjectHeight(obj.m_iDfltSubjectHeight)
    , m_sDfltSubjectComment(obj.m_sDfltSubjectComment)
    , m_iDfltSubjectHisId(obj.m_iDfltSubjectHisId)
    , m_iDfltProjectId(obj.m_iDfltProjectId)
    , m_sDfltProjectName(obj.m_sDfltProjectName)
    , m_sDfltProjectAim(obj.m_sDfltProjectAim)
    , m_sDfltProjectPersons(obj.m_sDfltProjectPersons)
    , m_sDfltProjectComment(obj.m_sDfltProjectComment)
    , m_sFileNameIn(obj.m_sFileNameIn)
    , m_sFileNameOut(obj.m_sFileNameOut)
    , m_printInSameLineHelper(qDebug())
    , m_bPrintInSameLine(true)
{
    
    //    m_BDfltMAC.resize(8);
    //    memcpy(m_BDfltMAC.data(),obj.m_BDfltMAC.data(),8);
    
    m_pBlockTypeList = QSharedPointer<QStack<int32_t> >(new QStack<int32_t>(*obj.m_pBlockTypeList));
    //    m_pBlockTypeList->resize(obj.m_pBlockTypeList->size());
    //    memcpy(m_pBlockTypeList->data(),obj.m_pBlockTypeList.data(),
    //           static_cast<size_t>(obj.m_pBlockTypeList->size()));
    
    m_pOutDir = QSharedPointer<QVector<FiffDirEntry> >(new QVector<FiffDirEntry>);
    //    m_pOutDir->resize(obj.m_pOutDir->size());
    //    memcpy(m_pOutDir->data(),obj.m_pOutDir->data(),
    //           static_cast<size_t>(obj.m_pOutDir->size()));
}


//*************************************************************************************************************

FiffAnonymizer::FiffAnonymizer(FiffAnonymizer &&obj)
    : version(obj.version)
    , maxValidFiffVerion(obj.maxValidFiffVerion)
    , versionStr(QString::number(obj.version))
    , name(obj.name)
    , description(obj.description)
    , m_bVerboseMode(obj.m_bVerboseMode)
    , m_bBruteMode(obj.m_bBruteMode)
    , m_bQuietMode(obj.m_bQuietMode)
    , m_bDeleteInputFileAfter(obj.m_bDeleteInputFileAfter)
    , m_bDeleteInputFileConfirmation(obj.m_bDeleteInputFileConfirmation)
    , m_bInputFileDeleted(obj.m_bInputFileDeleted)
    , m_bInOutFileNamesEqual(obj.m_bInOutFileNamesEqual)
    , m_bOutputFileRenamed(obj.m_bOutputFileRenamed)
    , m_sDfltString(obj.m_sDfltString)
    , m_dateDfltDate(obj.m_dateDfltDate)
    , m_dateMeasurmentDate(obj.m_dateMeasurmentDate)
    , m_bUseMeasurementDayOffset(obj.m_bUseMeasurementDayOffset)
    , m_iMeasurementDayOffset(obj.m_iMeasurementDayOffset)
    , m_dateSubjectBirthday(obj.m_dateSubjectBirthday)
    , m_bUseSubjectBirthdayOffset(obj.m_bUseSubjectBirthdayOffset)
    , m_iSubjectBirthdayOffset(obj.m_iSubjectBirthdayOffset)
    , m_BDfltMAC(obj.m_BDfltMAC)
    , m_iDfltSubjectId(obj.m_iDfltSubjectId)
    , m_sDfltSubjectFirstName(obj.m_sDfltSubjectFirstName)
    , m_sDfltSubjectMidName(obj.m_sDfltSubjectMidName)
    , m_sDfltSubjectLastName(obj.m_sDfltSubjectLastName)
    , m_iDfltSubjectWeight(obj.m_iDfltSubjectWeight)
    , m_iDfltSubjectHeight(obj.m_iDfltSubjectHeight)
    , m_sDfltSubjectComment(obj.m_sDfltSubjectComment)
    , m_iDfltSubjectHisId(obj.m_iDfltSubjectHisId)
    , m_iDfltProjectId(obj.m_iDfltProjectId)
    , m_sDfltProjectName(obj.m_sDfltProjectName)
    , m_sDfltProjectAim(obj.m_sDfltProjectAim)
    , m_sDfltProjectPersons(obj.m_sDfltProjectPersons)
    , m_sDfltProjectComment(obj.m_sDfltProjectComment)
    , m_sFileNameIn(obj.m_sFileNameIn)
    , m_sFileNameOut(obj.m_sFileNameOut)
    , m_printInSameLineHelper(qDebug())
    , m_bPrintInSameLine(true)
{
    //    m_BDfltMAC.resize(8);
    //    memcpy(m_BDfltMAC.data(),obj.m_BDfltMAC.data(),8);
    
    m_pBlockTypeList.swap(obj.m_pBlockTypeList);
    m_pOutDir.swap(obj.m_pOutDir);
}


//*************************************************************************************************************

int FiffAnonymizer::anonymizeFile()
{
    printIfVerbose(" ");
    printIfVerbose("----------------------------------------------------------------------------");
    printIfVerbose(" ");
    printIfVerbose(name);
    printIfVerbose(description);
    printIfVerbose(" ");
    printIfVerbose("Version: " + versionStr);
    printIfVerbose("Current date: " + QDateTime::currentDateTime().toString("ddd MMMM d yyyy hh:mm:ss"));
    printIfVerbose(" ");
    
    FiffStream inStream(&m_fFileIn);
    printIfVerbose("Input file: " + m_fFileIn.fileName());
    if(inStream.open(QIODevice::ReadOnly))
    {
        printIfVerbose(" opened correctly.",m_bPrintInSameLine);
    } else {
        qCritical() << "FiffAnonymizer::run - Problem opening the input file: " << m_fFileIn.fileName();
        return 1;
    }
    
    FiffStream outStream(&m_fFileOut);
    printIfVerbose("Output file: " + m_fFileOut.fileName());
    if(outStream.device()->open(QIODevice::ReadWrite))
    {
        printIfVerbose(" opened correctly",m_bPrintInSameLine);
    } else {
        qCritical() << "FiffAnonymizer::run - Problem opening the output file: " << m_fFileOut.fileName();
        return 1;
    }
    
    FiffTag::SPtr pInTag = FiffTag::SPtr::create();
    FiffTag::SPtr pOutTag = FiffTag::SPtr::create();
    
    inStream.read_tag(pInTag,0);
    //    FiffTag::convert_tag_data(pInTag,FIFFV_BIG_ENDIAN,FIFFV_NATIVE_ENDIAN);
    
    //info in a tag FIFF_COMMENT (206) depends on the type of block it is in. Therefore, in order to
    //anonymize it we not only have to know the kind of the current tag, but also which type of block
    //we're currently in. BlockTypeList is a stack container used to keep track of this.
    m_pBlockTypeList->clear();
    updateBlockTypeList(pInTag);
    
    printIfVerbose("Reading info in the file.");
    if(checkValidFiffFormatVersion(pInTag))
    {
        printIfVerbose("Input file compatible with this version of mne_fiffAnonymizer.");
    } else {
        printIfVerbose("***");
        printIfVerbose("***   Warning: This file may not be compatible with this application.");
        printIfVerbose("***");
    }
    censorTag(pOutTag,pInTag);
    pOutTag->next = 0;
    
    //we build the tag directory on the go
    addEntryToDir(pOutTag,outStream.device()->pos());
    outStream.write_tag(pOutTag);
    
    while(pInTag->next != -1)
    {
        inStream.read_tag(pInTag);
        updateBlockTypeList(pInTag);
        censorTag(pOutTag,pInTag);

        //the order of the tags in the output file is sequential. No jumps in the output file.
        if(pOutTag->next > 0)
        {
            pOutTag->next = 0;
        }
        addEntryToDir(pOutTag,outStream.device()->pos());
        outStream.write_tag(pOutTag);
    }
    
    if(inStream.close())
    {
        printIfVerbose("Input file finished. All tags have been correctly anonymized.");
    } else {
        qCritical() << "FiffAnonymizer::run - Problem closeing the input file: " << m_fFileIn.fileName();
    }
    
    addFinalEntryToDir();
    fiff_long_t posOfDirectory(outStream.device()->pos());
    writeDirectory(&outStream);
    updatePointer(&outStream,FIFF_DIR_POINTER,posOfDirectory);
    updatePointer(&outStream,FIFF_FREE_LIST,-1);
    
    if(outStream.close())
    {
        printIfVerbose("Input file finished. All tags have been correctly anonymized.");
    } else {
        qCritical() << "FiffAnonymizer::run - Problem closing the output file: " << m_fFileOut.fileName();
    }
    
    if(checkDeleteInputFile())
    {
        deleteInputFile();
    }
    
    if(checkRenameOutputFile())
    {
        renameOutputFileAsInputFile();
    }
    
    printIfVerbose(" ");
    printIfVerbose("----------------------------------------------------------------------------");
    printIfVerbose(" ");
    printIfVerbose(" "); //Note that this line will actually never be printed.

    return 0;
}


//*************************************************************************************************************

void FiffAnonymizer::updateBlockTypeList(FiffTag::SPtr pTag)
{
    if(pTag->kind == FIFF_BLOCK_START)
    {
        m_pBlockTypeList->push(*pTag->toInt());
    }
    if(pTag->kind == FIFF_BLOCK_END)
    {
        m_pBlockTypeList->pop();
    }
}


//*************************************************************************************************************

bool FiffAnonymizer::checkValidFiffFormatVersion(FiffTag::SPtr pTag)
{
    if(pTag->kind == FIFF_FILE_ID)
    {
        FiffId fileId = pTag->toFiffID();
        int inMayorVersion = (fileId.version & 0xFFFF0000) >> 16;
        int inMinorVersion = (fileId.version & 0x0000FFFF);
        double inVersion = inMayorVersion + inMinorVersion/10.;
        if(inVersion > maxValidFiffVerion)
            return false;
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
    
    if(m_pOutDir->size() > 0)
    {
        QByteArray pInt8(sizeof(fiff_int_t),0);
        for(int i=0;i<m_pOutDir->size();++i)
        {
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
    for(int i=0;i<m_pOutDir->size();++i)
    {
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
    
    if(m_bDeleteInputFileAfter) //false by default
    {
        qDebug() << "You have requested to delete the input file: " + m_fFileIn.fileName();
        if(m_bDeleteInputFileConfirmation) //true by default
        {
            QTextStream consoleOut(stdout);
            QTextStream consoleIn(stdin);
            QString confirmation;
            qDebug() << "You can avoid this confirmation by using the delete_confirmation option.";
            consoleOut << "Are you sure you want to delete this file? [Y/n] ";
            consoleIn >> confirmation;
            if(confirmation == "Y") {
                return true;
            }
        } else
        {
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
    
    switch (inTag->kind)
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
        FiffId inId = inTag->toFiffID();

//        QList<QByteArray> inMAC({QByteArray::fromHex(QString::number(inId.machid[0]).toUtf8()),
//                                 QByteArray::fromHex(QString::number(inId.machid[1]).toUtf8())});
//        QList<QByteArray> outMAC(m_BDfltMAC.split(3));
        QDateTime inMeasDate(QDateTime::fromSecsSinceEpoch(inId.time.secs).date());
        QDateTime outMeasDate;
        
        if(m_bUseMeasurementDayOffset)
        {
            outMeasDate = inMeasDate.addDays(-m_iMeasurementDayOffset);
        } else {
            outMeasDate = m_dateMeasurmentDate;
        }
        const int fiffIdSize(sizeof(inId));//5
        fiff_int_t outData[fiffIdSize];
        outData[0] = inId.version;
        outData[1] = -1; //inFileId.machid[0];
        outData[2] = -1; //inFileId.machid[1];
        outData[3] = static_cast<int32_t>(outMeasDate.toSecsSinceEpoch());
        outData[4] = 0; //inId.time.usecs;

        outTag->resize(fiffIdSize*sizeof(fiff_int_t));
        memcpy(outTag->data(),reinterpret_cast<char*>(outData),fiffIdSize*sizeof(fiff_int_t));
        //printIfVerbose("MAC address changed: " + inMAC.at(0) + inMAC.at(1) + " -> "  + outMAC.at(0) + outMAC.at(1));
        printIfVerbose("Measurement date changed: " + inMeasDate.toString() + " -> " + outMeasDate.toString());
        break;
    }
    case FIFF_MEAS_DATE:
    {
        QDateTime inMeasDate(QDateTime::fromSecsSinceEpoch(*inTag->toInt()).date());
        QDateTime outMeasDate;
        
        if(m_bUseMeasurementDayOffset)
        {
            outMeasDate = inMeasDate.addDays(-m_iMeasurementDayOffset);
        } else {
            outMeasDate = m_dateMeasurmentDate;
        }
        
        fiff_int_t outData[1];
        outData[0]=static_cast<int32_t>(outMeasDate.toSecsSinceEpoch());
        memcpy(outTag->data(),reinterpret_cast<char*>(outData),1);
        printIfVerbose("Measurement date changed: " + inMeasDate.toString() + " -> " + outMeasDate.toString());
        break;
    }
    case FIFF_COMMENT:
    {
        if(m_pBlockTypeList->first()==FIFFB_MEAS_INFO)
        {
            QString inStr = inTag->toString();
            QString newStr(m_sDfltString);
            outTag->resize(newStr.toUtf8().size());
            memcpy(outTag->data(),newStr.data(),newStr.toUtf8().size());
            printIfVerbose("Description of the measurement block changed: " +
                           QString(inTag->data()) + " -> " + newStr);
        }
        break;
    }
    case FIFF_EXPERIMENTER:
    {
        QString inStr = inTag->toString();
        QString newStr(m_sDfltString);
        outTag->resize(newStr.toUtf8().size());
        memcpy(outTag->data(),newStr.data(),newStr.toUtf8().size());
        printIfVerbose("Experimenter changed: " +
                       QString(inTag->data()) + " -> " + newStr);
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
        QString inStr = inTag->toString();
        QString newStr(m_sDfltSubjectFirstName);
        outTag->resize(newStr.toUtf8().size());
        memcpy(outTag->data(),newStr.data(),newStr.toUtf8().size());
        printIfVerbose("Experimenter changed: " +
                       QString(inTag->data()) + " -> " + newStr);
        break;
    }
    case FIFF_SUBJ_MIDDLE_NAME:
    {
        QString inStr = inTag->toString();
        QString newStr(m_sDfltSubjectMidName);
        outTag->resize(newStr.toUtf8().size());
        memcpy(outTag->data(),newStr.data(),newStr.toUtf8().size());
        printIfVerbose("Experimenter changed: " +
                       QString(inTag->data()) + " -> " + newStr);
        break;
    }
    case FIFF_SUBJ_LAST_NAME:
    {
        QString inStr = inTag->toString();
        QString newStr(m_sDfltSubjectLastName);
        outTag->resize(newStr.toUtf8().size());
        memcpy(outTag->data(),newStr.data(),newStr.toUtf8().size());
        printIfVerbose("Experimenter changed: " +
                       QString(inTag->data()) + " -> " + newStr);
        break;
    }
    case FIFF_SUBJ_BIRTH_DAY:
    {
        QDateTime inBirthday(QDate::toJulianDay(*inTag->toJulian));
        QDateTime outBirthday;
        
        if(m_bUseSubjectBirthdayOffset)
        {
            outBirthday = inBirthday.addDays(-m_iMeasurementDayOffset);
        } else {
            outBirthday = m_dateSubjectBirthday;
        }
        
        fiff_int_t outData[1];
        outData[0] = static_cast<int32_t> (outBirthday.toSecsSinceEpoch());
        memcpy(outTag->data(),reinterpret_cast<char*>(outData),1);
        break;
    }
    case FIFF_SUBJ_WEIGHT:
    {
        break;
    }
    case FIFF_SUBJ_HEIGHT:
    {
        break;
    }
    case FIFF_SUBJ_COMMENT:
    {
        break;
    }
    case FIFF_SUBJ_HIS_ID:
    {
        QString inSubjectHisId(inTag->data());
        memcpy(outTag->data(),reinterpret_cast<char*>(&m_iDfltSubjectHisId),sizeof(int));
        printIfVerbose("Subject Hospital-ID changed:" + inSubjectHisId + " -> " + QString::number(m_iDfltSubjectId));
        break;
    }
    case FIFF_PROJ_ID:
    {
        break;
    }
    case FIFF_PROJ_NAME:
    {
        break;
    }
    case FIFF_PROJ_AIM:
    {
        break;
    }
    case FIFF_PROJ_PERSONS:
    {
        break;
    }
    case FIFF_PROJ_COMMENT:
    {
        break;
    }
    case FIFF_MRI_PIXEL_DATA:
    {
        //        disp(' ');
        //        disp('WARNING. The input fif file contains MRI data.');
        //        disp('Beware that a subject''s face can be reconstructed from it');
        //        disp('This software can not anonymize MRI data, at the moment.');
        //        disp('Contanct the authors for more information.');
        //        disp(' ');
        break;
    }
    default:
    {
        outTag->resize(inTag->size());
        memcpy(outTag->data(),inTag->data(),inTag->size());
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
    if(m_fFileOut.fileName().isEmpty())
    {
        m_sFileNameOut = sFileOut;
    } else
    {
        if(m_fFileIn.fileName().compare(sFileOut,Qt::CaseInsensitive) == 0)
        {
            m_bInOutFileNamesEqual = true;
            QFileInfo outFileInfo(sFileOut);
            outFileInfo.makeAbsolute();
            m_sFileNameOut = outFileInfo.absolutePath() + generateRandomFileName();
        } else
        {
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
    if(q)
    {
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
    if(m_bInOutFileNamesEqual)
    {
        if(m_bDeleteInputFileAfter)
        {
            if(m_bInputFileDeleted)
            {
                return true;
            }
        } else
        {
            m_bDeleteInputFileAfter = true;
            if(checkDeleteInputFile())
            {
                deleteInputFile();
                return true;
            } else {
                
                qCritical() << " ";
                qCritical() << "You have requested to save the output file with the same name as the input file.";
                qCritical() << "This cannot be done without deleting or modifying the input file.";
                qCritical() << "The output file is: " << m_sFileNameOut;
                qCritical() << " ";
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

void FiffAnonymizer::setSubjectHisId(int id)
{
    m_iDfltSubjectHisId = id;
}

