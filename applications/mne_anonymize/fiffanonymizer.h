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

    //=========================================================================================================
    /**
     * Constructs a FiffAnonymizer object.
     */
    FiffAnonymizer();

    //=========================================================================================================
    /**
    * Copy constructor for FiffAnonymizer object.
    * @param [in] A FiffAnonyzer object.
    */
    FiffAnonymizer(const FiffAnonymizer& obj);

    //=========================================================================================================
    /**
    * Move contructor
    * @param [in] a FiffAnonymizer object.
    */
    FiffAnonymizer(FiffAnonymizer &&obj);

    //=========================================================================================================
    /**
    * Calls the anonymizer routine.
    * @details This method is the main method in the class. It goes through the input file and tag by tag
    *   analyses if there might be some relevant information to anonymize and eventually does so.
    *   Finally the method would test if the input file should be deleted or renamed according to the
    *   flags set up during the object setup.
    */
    int anonymizeFile();

    //=========================================================================================================
    // PUBLIC INTERFACE
    /**
    * Specifies the input file to anonymize. This file will help to set a input Fiffstream object.
    * @param [in] String containing the input file name including its path.
    */
    void setFileIn(const QString sFilePathIn);

    //=========================================================================================================
    /**
    * Specifies the output file to anonymize. This file will help to set a output Fiffstream object.
    * @param [in] String containing the output file name including its path.
    */
    void setFileOut(const QString sFilePathOut);

    //=========================================================================================================
    /**
    * Sets the state of the FiffAnonymizer object's desired verbose mode. If set to TRUE, different messages will be
    * printed on screen during the anonymizing process. If set to false only a single line confirmation message will be
    * printed on each execution.
    * @param [in] Bool argument. [Default=FALSE]
    */
    void setVerboseMode(bool v);

    //=========================================================================================================
    /**
    * Retrieves the state of the FiffAnonyzer object's desired verbose mode.
    */
    bool getVerboseMode();

    //=========================================================================================================
    /**
    * Sets the state of the object the desired verbose mode to none. If set to TRUE, absolutely no message will be printed
    * to on screen during the anonymizing process.
    * @param [in] Bool argument.
    */
    void setQuietMode(bool q);

    /**
    * Sets the state of the FiffAnonymizer object's desired anonymization mode. If set to TRUE, apart from the default information
    * additional information will also be anonymized, like Subject's weight, height, or different project information.
    * printed on screen during the anonymizing process. [Default=FALSE].
    * @param [in] Bool argument.
    */
    void setBruteMode(bool b);

    //=========================================================================================================
    /**
    * If found in the fiff file, the measurement date information will be overwritten in the file in order to match the date specified with this function.
    * @param [in] String containing the desired measurement date.
    */
    void setMeasurementDay(QString d);

    //=========================================================================================================
    /**
    * If found in the fiff file, the specified number of days will be subtracted from the measurement date information contained in each fif file.
    * @param [in] Integer with the number of dates to subtract from the measurement date.
    */
    void setMeasurementDayOffset(int d);

    //=========================================================================================================
    /**
    * If found in the fiff file, subject's birthday information will be overwritten in the file in order to match the date specified with this function.
    * @param [in] String containing the desired measurement date.
    */
    void setSubjectBirthday(QString d);

    //=========================================================================================================
    /**
    * If found in the fiff file, the specified number of days will be subtracted from the subject's birthday date information contained in each fif file.
    * @param [in] Integer with the number of dates to subtract from the subject's birthday date.
    */
    void setSubjectBirthdayOffset(int d);

    //=========================================================================================================
    /**
    * Sets fiffanonymizer to delete the input file after anonymization finishes. This is intended to avoid duplication of disk space usage.
    * If set to true, by its own, a confirmation message will be prompted to the user. Used with the --delete_input_file_after option.
    * It can be used with the option "avoid_delete_confirmation" so that no confirmation is prompt to the user.
    * @param [in] Bool argument. [Default=FALSE]
    */
    void setDeleteInputFileAfter(bool d);

    //=========================================================================================================
    /**
    * Method to avoid the need to prompt the user for confirmation of deletion of the input file after anonymization has finished.
    * As the deletion flag has to manually be set to true and this confirmation flag has to manually be set to false, the chances of
    * a user disadvertently deleted a relevant input file are (hopefully) minimized.
    * @param [in] Bool argument. [Default=TRUE]
    */
    void setDeleteInputFileAfterConfirmation(bool dc);

    //=========================================================================================================
    /**
    * Specifies a flag so that the user can specifiy the value of the subject's id tag. If found in the fiff file, the value
    * will be changed to match the one specified with this method.
    * @param [in] Integer with the subject's id.
    */
    void setSubjectHisId(int id);

    //PUBLIC MEMBERS. note "const" type qualifier. Left public for convenience.
    const double version;               /**< Version of this FiffAnonymizer application --> mne_fiffanonymize application will be based on it*/
    const double maxValidFiffVerion;    /**< Maximum version of the Fiff file standard compatible with this application.*/
    const QString versionStr;           /**< version translated to a string.*/
    const QString name;                 /**< Name of this application. Typically MNE_ANONYMIZE.*/
    const QString description;          /**< Application description*/

private:

    //=========================================================================================================
    /**
    *
    * @param [in]
    */
    void updateBlockTypeList(FIFFLIB::FiffTag::SPtr pTag);

    //=========================================================================================================
    /**
    *
    * @param [in]
    */
    bool checkValidFiffFormatVersion(FIFFLIB::FiffTag::SPtr pTag);

    //=========================================================================================================
    /**
    *
    * @param [in]
    */
    int censorTag(FIFFLIB::FiffTag::SPtr pInTag,FIFFLIB::FiffTag::SPtr pOutTag);

    //=========================================================================================================
    /**
    *
    * @param [in]
    */
    void addEntryToDir(FIFFLIB::FiffTag::SPtr pTag,qint64 filePos);

    //=========================================================================================================
    /**
    *
    * @param [in]
    */
    void addFinalEntryToDir();

    //=========================================================================================================
    /**
    *
    * @param [in]
    */
    void dir2tag(FIFFLIB::FiffTag::SPtr pTag);

    //=========================================================================================================
    /**
    *
    * @param [in]
    */
    void writeDirectory(FIFFLIB::FiffStream* stream, FIFFLIB::fiff_long_t pos=-1);

    //=========================================================================================================
    /**
    *
    * @param [in]
    */
    void updatePointer(FIFFLIB::FiffStream* stream, FIFFLIB::fiff_int_t tagKind, FIFFLIB::fiff_long_t newPos);

    //=========================================================================================================
    /**
    *
    * @param [in]
    */
    void printIfVerbose(const QString str,bool sameLine=false);

    //=========================================================================================================
    /**
    *
    * @param [in]
    */
    QString generateRandomFileName();

    //=========================================================================================================
    /**
    *
    * @param [in]
    */
    void deleteInputFile();

    //=========================================================================================================
    /**
    *
    * @param [in]
    */
    bool checkDeleteInputFile();

    //=========================================================================================================
    /**
    *
    * @param [in]
    */
    bool checkRenameOutputFile();

    //=========================================================================================================
    /**
    *
    * @param [in]
    */
    void renameOutputFileAsInputFile();

    bool m_bVerboseMode;                /**< */
    bool m_bBruteMode;                  /**< */
    bool m_bQuietMode;                  /**< */
    bool m_bDeleteInputFileAfter;       /**< */
    bool m_bDeleteInputFileConfirmation;/**< */
    bool m_bInputFileDeleted;           /**< */
    bool m_bInOutFileNamesEqual;        /**< */
    bool m_bOutputFileRenamed;          /**< */
                                        /**< */
    QString m_sDfltString;              /**< */
    QDateTime m_dateDfltDate;           /**< */
                                        /**< */
    QDateTime m_dateMeasurmentDate;     /**< */
    bool m_bUseMeasurementDayOffset;    /**< */
    int  m_iMeasurementDayOffset;       /**< */
                                        /**< */
    QDateTime m_dateSubjectBirthday;    /**< */
    bool m_bUseSubjectBirthdayOffset;   /**< */
    int  m_iSubjectBirthdayOffset;      /**< */
                                        /**< */
    QByteArray m_BDfltMAC;              /**< */
                                        /**< */
    int m_iDfltSubjectId;               /**< */
    QString m_sDfltSubjectFirstName;    /**< */
    QString m_sDfltSubjectMidName;      /**< */
    QString m_sDfltSubjectLastName;     /**< */
    int m_iDfltSubjectWeight;           /**< */
    int m_iDfltSubjectHeight;           /**< */
    QString m_sDfltSubjectComment;      /**< */
    int m_iDfltSubjectHisId;            /**< */
                                        /**< */
    int m_iDfltProjectId;               /**< */
    QString m_sDfltProjectName;         /**< */
    QString m_sDfltProjectAim;          /**< */
    QString m_sDfltProjectPersons;      /**< */
    QString m_sDfltProjectComment;      /**< */
                                        /**< */
    QString m_sFileNameIn;              /**< */
    QString m_sFileNameOut;             /**< */
                                        /**< */
    QFile m_fFileIn;                    /**< */
    QFile m_fFileOut;                   /**< */
                                        /**< */
    QDebug m_printInSameLineHelper;     /**< */
    const bool m_bPrintInSameLine;      /**< */

    QSharedPointer<QStack<int32_t> > m_pBlockTypeList;  /**< */
    QSharedPointer<QVector<FIFFLIB::FiffDirEntry> > m_pOutDir; /**< */

};



//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

//=========================================================================================================
/**
* Helper function to allow for delayed QDebug on the same line as previous QDebug.
* @param [in]
*/
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
