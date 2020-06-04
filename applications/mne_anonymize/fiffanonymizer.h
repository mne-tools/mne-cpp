//=============================================================================================================
/**
 * @file     fiffanonymizer.h
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
 * @brief     FiffAnonymizer class declaration.
 *
 */

#ifndef MNEANONYMIZE_FIFFANONYMIZER_H
#define MNEANONYMIZE_FIFFANONYMIZER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>
#include <fiff/fiff_types.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QDateTime>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================
class fiffData;
//=============================================================================================================
// DEFINE NAMESPACE
//=============================================================================================================

namespace MNEANONYMIZE
{

//=============================================================================================================
// MNEANONYMIZE FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Description of what this class is intended to do (in detail).
 *
 * @brief FiffAnonymizer class declaration.
 */
class FiffAnonymizer : public QObject
{
    Q_OBJECT

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
     *
     * @param [in] A FiffAnonyzer object.
     */
    FiffAnonymizer(const FiffAnonymizer& obj);

    //=========================================================================================================
    /**
     * Assignment operator for FiffAnonymizer object.
     *
     * @param [in] A FiffAnonyzer object.
     */
    FiffAnonymizer& operator = (const FiffAnonymizer &t)
    {
        Q_UNUSED(t)
        return *this;
    }

    //=========================================================================================================
    /**
     * Move contructor
     *
     * @param [in] a FiffAnonymizer object.
     */
    FiffAnonymizer(FiffAnonymizer &&obj);

    //=========================================================================================================
    /**
     * This method is the main method in the class. It goes through the input file and tag by tag
     * analyses if there might be some relevant information to anonymize and eventually does so. Initially it
     * checks for valid file formating. Finally the method would test if the input file should be deleted or
     * renamed according to the flags configured in the class.
     */
    int anonymizeFile();

    //=========================================================================================================
signals:
    void readingIdMeasurementDate(QDateTime d);
    void readingFileMeasurementDate(QDateTime d);
    void readingFileComment(QString s);
    void readingFileExperimenter(QString e);
    void readingSubjectId(int i);
    void readingSubjectFirstName(QString fn);
    void readingSubjectMiddleName(QString mn);
    void readingSubjectLastName(QString ln);
    void readingSubjectBirthday(QDateTime b);
    void readingSubjectSex(int s);
    void readingSubjectHand(int h);
    void readingSubjectWeight(float w);
    void readingSubjectHeight(float h);
    void readingSubjectComment(QString c);
    void readingSubjectHisId(QString);

    void readingProjectId(int);
    void readingProjectName(QString);
    void readingProjectAim(QString);
    void readingProjectPersons(QString);
    void readingProjectComment(QString);

    void mriDataFoundInFile(bool);

    //=========================================================================================================

public slots:
    /**
     * Configure the input file to anonymize.
     *
     * @param [in] sFilePathIn  String containing the input file name including its path. Can be a relative or
     * an absolute path.
     */
    int setFileIn(const QString &sFilePathIn);

    //=========================================================================================================
    /**
     * Configure the output file to anonymize.
     *
     * @param [in] sFilePathOut String containing the output file name. Can be a relative or
     * an absolute path.
     */
    int setFileOut(const QString &sFilePathOut);

    //=========================================================================================================
    /**
     * Configure the state of the FiffAnonymizer object's desired anonymization mode. If set to TRUE, apart from the
     * default information additional information will also be anonymized, like Subject's weight, height, sex or
     * handedness and the project's information too.
     *
     * @param [in] bFlag    Bool argument whether to use the brute mode.
     *
     */
    void setBruteMode(bool bFlag);

    //=========================================================================================================
    /**
     * If found in the fiff file, the measurement date information will be overwritten with the date specified
     * with this function. Format: YYYMMDD. Default: 20000101
     *
     * @param [in] sMeasDay     String containing the desired measurement day.
     */
    void setMeasurementDate(const QString& sMeasDay);

    //=========================================================================================================
    /**
     * If found in the fiff file, the measurement date information will be overwritten with the date specified
     * with this function.
     *
     * @param [in] d     QDateTime object containing the desired measurement day.
     */
    void setMeasurementDate(const QDateTime& d);

    //=========================================================================================================
    /**
     * If found in the fiff file, the measurement date information will be overwritten with the date specified
     * with this function.
     *
     * @param [in] d     QDate object containing the desired measurement day.
     */
    void setMeasurementDate(const QDate& d);

    //=========================================================================================================
    /**
     * If found in the fiff file, the specified number of days will be subtracted from the measurement date information contained in each fif file.
     *
     * @param [in] iMeasDayOffset   Integer with the number of dates to subtract from the measurement date.
     */
    void setMeasurementDateOffset(int iMeasDayOffset);

    //=========================================================================================================
    /**
     * If found in the fiff file, the specified number of days will be subtracted from the measurement date information contained in each fif file.
     *
     * @param [in] iMeasDayOffset   Integer with the number of dates to subtract from the measurement date.
     */
    void setUseMeasurementDateOffset(bool);

    //=========================================================================================================
    /**
     * If found in the fiff file, subject's birthday information will be overwritten in the file in order to match the date specified with this function.
     *
     * @param [in] sSubjBirthday String containing the desired subject birthday date.
     */
    void setSubjectBirthday(const QString& sSubjBirthday);

    //=========================================================================================================
    /**
     * If found in the fiff file, subject's birthday information will be overwritten in the file in order to match the date specified with this function.
     *
     * @param [in] sSubjBirtday String containing the desired subject birthday date.
     */
    void setSubjectBirthday(const QDateTime& sSubjBirthday);

    //=========================================================================================================
    /**
     * If found in the fiff file, the specified number of days will be subtracted from the subject's birthday date information contained in each fif file.
     *
     * @param [in] iSubjBirthdayOffset  Integer with the number of dates to subtract from the subject's birthday date.
     */
    void setSubjectBirthdayOffset(int iSubjBirthdayOffset);

    //=========================================================================================================
    /**
     * If found in the fiff file, the specified number of days will be subtracted from the subject's birthday date information contained in each fif file.
     *
     * @param [in] iSubjBirthdayOffset  Integer with the number of dates to subtract from the subject's birthday date.
     */
    void setUseSubjectBirthdayOffset(bool);

    //=========================================================================================================
    /**
     * Specifies the subject's id text information you want the output file to have. If a hisID tag is found in the fiff file,
     * then, the subject's hisID information will be changed to match the one specified with this method.
     *
     * @param [in] sSubjectHisId   String with the subject's id.
     */
    void setSubjectHisId(const QString& sSubjectHisId);

    //=========================================================================================================
//    /**
//     * Sets fiffanonymizer to delete the input file after anonymization finishes. This is intended to avoid duplication of disk space usage.
//     * If set to true, by its own, a confirmation message will be prompted to the user. Used with the --delete_input_file_after option.
//     * It can be used with the option "avoid_delete_confirmation" so that no confirmation is prompt to the user.
//     *
//     * @param [in] bFlag    Bool argument whether to delete the input file afterwards.
//     */
//    void setDeleteInputFileAfter(bool bFlag);

    //=========================================================================================================
//    /**
//     * Method to avoid the need to prompt the user for confirmation of deletion of the input file after anonymization has finished.
//     * As the deletion flag has to manually be set to true and this confirmation flag has to manually be set to false, the chances of
//     * a user disadvertently deleted a relevant input file are (hopefully) minimized.
//     *
//     * @param [in] bFlag    Bool argument whether to ask for confirmation when deleting a file.
//     */
//    void setDeleteInputFileConfirmation(bool bFlag);

    //=========================================================================================================
    /**
     * Sets the state of the FiffAnonymizer object's desired verbose mode. If set to TRUE, different messages will be
     * printed on screen during the anonymizing process. If set to false only a single line confirmation message will be
     * printed on each execution.
     *
     * @param [in] bFlag    Bool argument whether to use the verbose mode.
     */
    void setVerboseMode(bool bFlag);

public:
    //=========================================================================================================
    /**
     * Returns the input file which will be anonymized. .
     *
     * @param [out] Returns a string containing the input file name including its path.
     */
    QString getFileNameIn() const;

    //=========================================================================================================
    /**
     * Returns the output file where anonymized data will be saved.
     *
     * @param [out] Returns a string containing the output file name including its path.
     */
    QString getFileNameOut() const;

    //=========================================================================================================
    /**
     * Retrieves the state of the FiffAnonyzer object's desired verbose mode.
     *
     * @param [out] Bool value with the actual verbose mode.
     *
     */
    bool getVerboseMode() const;

    //=========================================================================================================
    /**
     * Retrieve the value of the anonymization level brute mode. If set to true, the anonymization will delete
     * normal data but additionally it will also delete info related to the subject's weight, height, sex and
     * handedness.
     */
    bool getBruteMode() const;

    //=========================================================================================================
    /**
     * Retrieve the value of Date to substitute the measuremnt date appearing in the file.
     */
    QDateTime getMeasurementDate() const;

    //=========================================================================================================
    /**
     * Get the value of Number of days to subtract from the measurement date.
     */
    int getMeasurementDayOffset();

    //=========================================================================================================
    /**
     * Get value of Flags to use Measurement-date days offset.
     */
    bool getUseMeasurementDayOffset();

    //=========================================================================================================
    /**
     * Get value of Subject's birthday.
     */
    QDateTime getSubjectBirthday();

    //=========================================================================================================
    /**
     * Get value of Flags use of Subject's birthday offset.
     */
    bool getUseSubjectBirthdayOffset();

    //=========================================================================================================
    /**
     * Get value of Subjects's birthday offset.
     */
    int  getSubjectBirthdayOffset();

    //=========================================================================================================
    /**
     * Get value of Subjects's His Id text.
     */
    QString getSubjectHisID();

private:
    //=========================================================================================================
    /**
     * Updates a stack (m_pBlockTyeList points to it) with the type of block the input stream is in.
     * While reading an input file as a data stream, we will read tag by tag. Each tag can be inside a specific
     * block, as defined in Fiff file standard docs. FIFF_COMMENT tag (#206) will need to be censored/anonymized
     * only if it is contained inside a block of type "measurement info". Thus, we need to keep track of which
     * are we in, while reading. We do this by checking the first element in this stack (see censorTag() in case
     * FIFF_COMMENT.
     * This is a refactoring method, designed to make anonymizeFile() more readable.
     *
     * @param [in] pTag Pointer to the tag being read. Normally the input Tag.
     */
    void updateBlockTypeList();

    //=========================================================================================================
    /**
     * Acquisition sw stamps in the fiff file the version of the fiff standard used to code the fiff file. This
     * anonymizer class has been developed according to a specific fiff version of this standard. If in the future,
     * this standard were to change, it should be noted through this method.
     *
     * @param [in] pTag Pointer to the tag being read. Normally, the input Tag.
     */
    bool checkValidFiffFormatVersion() const;

    //=========================================================================================================
    /**
     * For a specific input tag, check if that tag belongs to a set of tags where relevant information should be
     * censored/anonymized. If so, perform such anonymization while copying the new tag into an output Tag.
     * This is the core method of the class where the actual anonymization takes place.
     */
    void censorTag();

    //=========================================================================================================
    /**
     * This transforms the data inside a subject sex tag (male, female or unknown) into a string object
     * containing that information.
     */
    inline QString subjectSexToString(int sexCode) const;

    //=========================================================================================================
    /**
     * This transforms the data inside a subject handedness tag (right, left or unknown) into a string object
     * containing that information.
     */
    inline QString subjectHandToString(int handCode) const;

    //=========================================================================================================
    /**
     * Helper function that prints messages to the command line only if the object has been set to a verbose mode.
     * This wraps QDebug functionality inline. Specified here in header file. If the obj is not set to be in a
     * verbose mode, it does nothing. Messages can be printed to a single line (followed by an eol character) or
     * can be printed in the same line as previous one. Note that to achieve this, messages will be "retained" one
     * call, in order to check if next call requests to print in the same line.
     *
     * @param [in] str  String to print.
     */
    inline void printIfVerbose(const QString &str) const;

    //=========================================================================================================
    /**
     * Configure and setup the in and out stream (Fiffstreams) to read from and write to.
     */
    int openInOutStreams();

    //=========================================================================================================
    /**
     * Closes the streams (Fiffstream) for the input and output files.
     */
    int closeInOutStreams();

    //=========================================================================================================
    /**
     * The first 2-4 tags have to be a specific set, although not all are mandatory. First tag must be a FiffID tag,
     * second must be a Tag Directory pointer tag. 3 and 4 are optional and must be a Free List pointer and a
     * parent file FiffID tag. This method goes through tags #1 and #2 and verifies that they follow the fiff
     * format.
     *
     */
    void processHeaderTags();

    //=========================================================================================================
    /**
     * Will read a tag and update the block type list and stores it in m_pInTag.
     */
    void readTag();

    //=========================================================================================================
    /**
     * Will overwrite the 'next' field in the tag stored in m_pInTag. It will store the output tag in the tag
     * directory and will finally write the tag stored in m_pOutTag into the output file stream.
     */
    void writeTag();

    //=========================================================================================================

    FIFFLIB::FiffStream::SPtr m_pInStream;  /**< Pointer to FiffStream object for reading.*/
    FIFFLIB::FiffStream::SPtr m_pOutStream; /**< Pointer to FiffStream object for writing the result.*/
    FIFFLIB::FiffTag::SPtr m_pTag;          /**< Pointer to FiffTag used for reading and writing each tag.*/

    FIFFLIB::fiff_int_t m_BDfltMAC[2];  /**< MAC addresss substitutor.*/

    QSharedPointer<QStack<int32_t> > m_pBlockTypeList;          /**< Pointer to Stack storing info related to the blocks of tags in the file.*/

    QFile m_fFileIn;                    /**< Input file.*/
    QFile m_fFileOut;                   /**< Output file.*/
    bool m_bFileInSet;                  /**< Input file set.*/
    bool m_bFileOutSet;                 /**< Output file set.*/

    bool m_bVerboseMode;                /**< Verbosity mode enabler.*/
    bool m_bBruteMode;                  /**< Advanced anonymization. Anonymize also weight, height and some other fields.*/

    //fiff version of standard
    const double m_dMaxValidFiffVerion; /**< Maximum version of the Fiff file standard compatible with this application.*/

    //actual fields in fiff file to anonymize
    QString m_sDefaultString;           /**< String to be used as substitution of other strings in a fiff file */
    QString m_sDefaultShortString;      /**< Short string to be used as substitution of protected short strings in a fiff file */
    QDateTime m_dDefaultDate;           /**< Date to be used as substitution of dates found in a fiff file */

    QDateTime m_dMeasurementDate;       /**< Date to substitute the measuremnt date appearing in the file.*/
    int  m_iMeasurementDateOffset;       /**< Number of days to subtract from the measurement date.*/
    bool m_bUseMeasurementDateOffset;    /**< Flags use Measurement-date days offset.*/

    QString m_sFiffComment;             /**< Fiff comment string substitutor.*/
    QString m_sFiffExperimenter;        /**< Fiff experimenter string substitutor.*/

    int m_iSubjectId;                   /**< Subject's id substitutor.*/
    QString m_sSubjectFirstName;        /**< Subject's first name substitutor.*/
    QString m_sSubjectMidName;          /**< Subject's middle name substitutor.*/
    QString m_sSubjectLastName;         /**< Subject's last name substitutor.*/
    QDateTime m_dSubjectBirthday;       /**< Subject's birthday substitutor.*/
    int m_iSubjectBirthdayOffset;       /**< Subjects's birthday offset.*/
    bool m_bUseSubjectBirthdayOffset;   /**< Flags use of Subject's birthday offset.*/
    QString m_sSubjectComment;          /**< Subject's comment substitutor.*/
    int m_iSubjectSex;                  /**< Subject's sex substitutor.*/
    int m_iSubjectHand;                 /**< Subject's hand substitutor.*/
    float m_fSubjectWeight;             /**< Subject's weight substitutor.*/
    float m_fSubjectHeight;             /**< Subject's height substitutor.*/
    QString m_sSubjectHisId;            /**< Subject's HIS ID substitutor.*/

    int m_iProjectId;                   /**< Project's id# substitutor.*/
    QString m_sProjectName;             /**< Project's name substitutor.*/
    QString m_sProjectAim;              /**< Project's aim substitutor.*/
    QString m_sProjectPersons;          /**< Project's Persons substitutor.*/
    QString m_sProjectComment;          /**< Project's comment substitutor.*/
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline QString FiffAnonymizer::subjectSexToString(int sexCode) const
{
    static QStringList subjectSexRefList =
    {
        "unknown" ,
        "male" ,
        "female"
    };

    if (sexCode > -1 && sexCode < subjectSexRefList.size())
    {
        return subjectSexRefList.at(sexCode);
    } else {
        qCritical() << "Invalid subject sex code. [0 = unknown, 1 = male, 2 = female]. The code of the subject is: " << QString::number(sexCode);
        return QString("invalid-code");
    }
}

//=============================================================================================================

inline QString FiffAnonymizer::subjectHandToString(int handCode) const
{
    static QStringList subjectHandRefList =
    {
        "unknown",
        "right",
        "left",
        "ambidextrous"
    };

    if ((handCode > -1) && (handCode < subjectHandRefList.size()))
    {
        return subjectHandRefList.at(handCode);
    } else {
        qCritical() << "Invalid subject handedness code. [0 = unknown, 1 = right, 2 = left, 3 = ambidextrous]. The code of the subject is: " << QString::number(handCode);
        return QString("invalid-code");
    }
}

//=============================================================================================================

inline void FiffAnonymizer::printIfVerbose(const QString& str) const
{
    if(m_bVerboseMode)
    {
        std::printf("\n%s", str.toUtf8().data());
    }
}

} // namespace MNEANONYMIZE

#endif // MNEANONYMIZE_FIFFANONYMIZER_H
