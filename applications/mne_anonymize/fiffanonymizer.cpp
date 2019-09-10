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
, versionStr(QString::number(version))
, name("MNE Anonymize")
, description("Application that removes or modifies Personal Health Information or Personal Identifiable information.")
, m_bVerboseMode(false)
, m_bBruteMode(false)
, m_bQuietMode(false)
, m_bDeleteInputFileAfter(false)
, m_bDeleteInputFileConfirmation(true)
, m_sDfltString("mne_anonymize")
, m_dateDfltDate(QDateTime(QDate(2000,1,1), QTime(1, 1, 0)))
, m_iMeasurementDayOffset(0)
, m_bUseMeasurementDayOffset(false)
, m_iSubjectBirthdayOffset(0)
, m_bUseSubjectBirthdayOffset(false)
, m_dateMeasurmentDate(m_dateDfltDate)
, m_date_IdDate(m_dateDfltDate)
, m_iDfltSubjectId(0)
, m_sDfltSubjectFirstName(m_sDfltString)
, m_sDfltSubjectMidName("x")
, m_sDfltSubjectLastName(m_sDfltString)
, m_dateSubjectBirthDay(m_dateDfltDate)
, m_iDfltSubjectWeight(0)
, m_iDfltSubjectHeight(0)
, m_sDfltSubjectComment(m_sDfltString)
, m_sDfltSubjectHisId(m_sDfltString)
, m_iDfltProjectId(0)
, m_sDfltProjectName(m_sDfltString)
, m_sDfltProjectAim(m_sDfltString)
, m_sDfltProjectPersons(m_sDfltString)
, m_sDfltProjectComment(m_sDfltString)
, m_sFileNameIn("")
, m_sFfileNameOut("")
, m_printInSameLineHelper(qDebug())
, m_bPrintInSameLine(true)
{
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
, m_bVerboseMode(false)
, m_bBruteMode(false)
, m_bQuietMode(false)
, m_bDeleteInputFileAfter(false)
, m_bDeleteInputFileConfirmation(true)
, m_sDfltString("mne_anonymize")
, m_dateDfltDate(QDateTime(QDate(2000,1,1), QTime(1, 1, 0)))
, m_iMeasurementDayOffset(0)
, m_bUseMeasurementDayOffset(false)
, m_iSubjectBirthdayOffset(0)
, m_bUseSubjectBirthdayOffset(false)
, m_dateMeasurmentDate(m_dateDfltDate)
, m_date_IdDate(m_dateDfltDate)
, m_iDfltSubjectId(0)
, m_sDfltSubjectFirstName(m_sDfltString)
, m_sDfltSubjectMidName("x")
, m_sDfltSubjectLastName(m_sDfltString)
, m_dateSubjectBirthDay(m_dateDfltDate)
, m_iDfltSubjectWeight(0)
, m_iDfltSubjectHeight(0)
, m_sDfltSubjectComment(m_sDfltString)
, m_sDfltSubjectHisId(m_sDfltString)
, m_iDfltProjectId(0)
, m_sDfltProjectName(m_sDfltString)
, m_sDfltProjectAim(m_sDfltString)
, m_sDfltProjectPersons(m_sDfltString)
, m_sDfltProjectComment(m_sDfltString)
, m_sFileNameIn("")
, m_sFfileNameOut("")
, m_printInSameLineHelper(qDebug())
, m_bPrintInSameLine(true)
{
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

int FiffAnonymizer::anonymizeFile()
{
    printIfVerbose(" ");
    printIfVerbose("----------------------------------------------------------------------------");
    printIfVerbose(" ");
    printIfVerbose(name);
    printIfVerbose(description);
    printIfVerbose(" ");
    printIfVerbose("Version: " + versionStr);
    printIfVerbose("Current date: " + m_date_IdDate.currentDateTime().toString("ddd MMMM d yyyy hh:mm:ss"));
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
    printIfVerbose("Input file: " + m_fFileOut.fileName());
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
    censorTag(pInTag,pOutTag);
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
        qCritical() << "FiffAnonymizer::run - Problem closeing the output file: " << m_fFileOut.fileName();
    }

    //delete file
    if(checkDeleteInputFile())
    {
        m_fFileIn.remove();
        printIfVerbose("Input file deleted.");
    }
    printIfVerbose(" ");
    printIfVerbose("----------------------------------------------------------------------------");
    printIfVerbose(" ");
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
        int inMayorVersion = (fileId.version & 0xFF00) >> 16;
        int inMinorVersion = (fileId.version & 0x00FF);
        double inVersion = inMayorVersion + inMinorVersion/10.;
        if(inVersion > version)
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

    if(m_bDeleteInputFileAfter)
    {
        qDebug() << "You have requested to delete the input file: " + m_fFileIn.fileName();
        if(m_bDeleteInputFileConfirmation)
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
        }
    }
    return false;
}


//*************************************************************************************************************

int FiffAnonymizer::censorTag(FiffTag::SPtr outTag,FiffTag::SPtr inTag)
{
    int sizeDiff(0);
    outTag = QSharedPointer<FiffTag>(new FiffTag(inTag.data()));

    //This is not copying the data and leaves the data ptr to the QByteArray dangling -> memory leak
//    outTag->kind = inTag->kind;
//    outTag->type = inTag->type;
//    outTag->next = inTag->next;

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
        const int fiffIdSize = 5;
        fiff_int_t outData[fiffIdSize];
        outData[0] = inId.version;
        outData[1] = 0; //inFileId.machid[0];
        outData[2] = 0; //inFileId.machid[1];
        outData[3] = inId.time.secs - 24 * 60 * 60 * m_iMeasurementDayOffset;
        outData[4] = 0; //inId.time.usecs;
        //FiffTag::convert_tag_data(p_pTag,FIFFV_BIG_ENDIAN,FIFFV_NATIVE_ENDIAN);
        memcpy(outTag->data(),reinterpret_cast<char*>(outData),fiffIdSize*sizeof(fiff_int_t));
        break;
    }
    case FIFF_MEAS_DATE:
    {
        qint32 inMeasDate = *inTag->toInt();
        qint32 newMeasDate = inMeasDate - 24 * 60 * 60 * m_iMeasurementDayOffset;
        memcpy(outTag->data(),reinterpret_cast<char*>(&newMeasDate),1*sizeof(newMeasDate));

        //        outTag->data()[0] = static_cast<char>((static_cast<uint>(newMeasDate) & 0xff000000) >> (8*3));
        //        outTag->data()[1] = static_cast<char>((static_cast<uint>(newMeasDate) & 0x00ff0000) >> (8*2));
        //        outTag->data()[2] = static_cast<char>((static_cast<uint>(newMeasDate) & 0x0000ff00) >>  8);
        //        outTag->data()[3] = static_cast<char>((static_cast<uint>(newMeasDate) & 0x000000ff) );
        break;
    }
    case FIFF_COMMENT:
    {
        QString inStr = inTag->toString();
        QString newStr("This is a new String");
        //We do not need to clear since we resize later and then overwrite the data
        //outTag->clear();
        outTag->resize(newStr.toUtf8().size());
        outTag->append(newStr);
        printIfVerbose("Description of the measurement block changed: " +
                       QString(inTag->data()) + " -> " + newStr);
        break;
    }
    case FIFF_EXPERIMENTER:
    {
        QString inStr = inTag->toString();
        QString newStr("This is a new String");
        //We do not need to clear since we resize later and then overwrite the data
        //outTag->clear();
        outTag->resize(newStr.toUtf8().size());
        outTag->append(newStr);
        printIfVerbose("Description of the measurement block changed: " +
                       QString(inTag->data()) + " -> " + newStr);
        break;
    }
    case FIFF_SUBJ_ID:
    {
        break;
    }
    case FIFF_SUBJ_FIRST_NAME:
    {
        break;
    }
    case FIFF_SUBJ_MIDDLE_NAME:
    {
        break;
    }
    case FIFF_SUBJ_LAST_NAME:
    {
        break;
    }
    case FIFF_SUBJ_BIRTH_DAY:
    {
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
        // outTag.data()=inTag.data();
    }

    }

    sizeDiff = outTag->size() - inTag->size();

    return sizeDiff;
}


//*************************************************************************************************************

void FiffAnonymizer::setFileIn(const QString& sFilePathIn)
{
    m_fFileIn.setFileName(sFilePathIn);
}


//*************************************************************************************************************

void FiffAnonymizer::setFileOut(const QString& sFilePathOut)
{
    m_fFileOut.setFileName(sFilePathOut);
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
    m_date_IdDate = m_dateMeasurmentDate;
}


//*************************************************************************************************************

void FiffAnonymizer::setMeasurementDayOffset(int d)
{
    m_bUseMeasurementDayOffset = true;
    m_iMeasurementDayOffset = d;
}


//*************************************************************************************************************

void FiffAnonymizer::setMeasurementDayOffset(bool b)
{
    m_bUseMeasurementDayOffset = b;
    m_iMeasurementDayOffset = b? m_iMeasurementDayOffset:0;
}


//*************************************************************************************************************

void FiffAnonymizer::setSubjectBirthday(QString d)
{
    m_dateSubjectBirthDay = QDateTime(QDate::fromString(d),QTime(1, 1, 0));
}


//*************************************************************************************************************

void FiffAnonymizer::setSubjectBirthdayOffset(int d)
{
    m_bUseSubjectBirthdayOffset= true;
    m_iSubjectBirthdayOffset= d;
}


//*************************************************************************************************************

void FiffAnonymizer::setSubjectBirthdayOffset(bool b)
{
    m_bUseSubjectBirthdayOffset = b;
    m_iSubjectBirthdayOffset = b? m_iSubjectBirthdayOffset:0;
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
