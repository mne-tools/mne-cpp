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
 * Copyright (C) 2020, Wayne Mead, Juan Garcia-Prieto, Lorenz Esch, Matti Hamalainen, John C. Mosher. All rights reserved.
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
 * This class opens an input fif file, reads sequentially  all its tags (linked list) and saves
 * an anonymized copy of each tag into an output file.
 *
 * @details The class is intentionally concieved as "not-smart" or restricted to the actual anonymization
 * process. It has several getters and setters, and a main anonymizer member method. Several member variable help
 * keep track of how the anonymization process should take place, but the anonymization flow is intended to be
 * taken care of by a controller.
 *
 * @brief FiffAnonymizer class declaration.
 *
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
     * @param[in] A FiffAnonyzer object.
     */
    FiffAnonymizer(const FiffAnonymizer& obj);

    //=========================================================================================================
    /**
     * Assignment operator for FiffAnonymizer object.
     *
     * @param[in] A FiffAnonyzer object.
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
     * @param[in] a FiffAnonymizer object.
     */
    FiffAnonymizer(FiffAnonymizer &&obj);

    //=========================================================================================================
    /**
     * @brief Anonymize the input file and save resutls in the output file.
     *
     * @details This method is the main method in the class. It goes through the input file and tag by tag
     * analyses if there might be some relevant information to anonymize and eventually does so. Initially it
     * checks for valid file formating. Finally the method would test if the input file should be deleted or
     * renamed according to the flags configured in the class.
     */
    int anonymizeFile();

signals:
    //=========================================================================================================
    /**
     *  @brief Send the version of FIFF standard of the current file.
     *
     *  @details This signal is emitted whenever runing <censorTag>"()" and an ID tag is in <m_pTag> (generally through
     * <readTag>"()"). Inside, the ID tag, the version of the FIFF standard used to encode the current file is stored
     * as a floating point number.
     *
     * @param[in] v Version of the FIFF standard used to encode the current file being censored.
     *
     */
    void readingIdFileVersion(double v);

    //=========================================================================================================
    /**
     *  @brief Send the measurement date of the current file.
     *
     *  @details This signal is emitted whenever runing <censorTag>"()" and an ID tag is in <m_pTag> (generally through
     * <readTag>"()"). Inside, the ID tag, the measurement date is stored.
     *
     * @param[in] d Measurement date of the file being censored.
     *
     */
    void readingIdMeasurementDate(QDateTime d);

    //=========================================================================================================
    /**
     *  @brief Send the MAC address of the current file.
     *
     *  @details This signal is emitted whenever runing <censorTag>"()" and an ID tag is in <m_pTag> (generally through
     * <readTag>"()"). Inside, the MAC address of the network card of the computer used to register the measurement is stored.
     *
     * @param[in] mac MAC address of the file being censored.
     *
     */
    void readingIdMac(QString mac);

    //=========================================================================================================
    /**
     *  @brief Send the measurement date of the current file, as defined in a "measurement date" tag.
     *
     *  @details This signal is emitted whenever runing <censorTag>"()" and a "measurement date" tag is in <m_pTag> (generally through
     * <readTag>"()").
     *
     * @param[in] d Measurement date of the file being censored.
     *
     */
    void readingFileMeasurementDate(QDateTime d);

    //=========================================================================================================
    /**
     *  @brief Send the "file comment" field of the current file, as defined in a "file comment" tag.
     *
     *  @details This signal is emitted whenever runing <censorTag>"()" and a "file comment" tag is in <m_pTag> (generally through
     * <readTag>"()"). The file comment tag stores the actual comment as a string, which is sent through this signal.
     *
     * @param[in] s Comment in the current "file comment" tag being censored.
     *
     */
    void readingFileComment(QString s);

    //=========================================================================================================
    /**
     *  @brief Send the "file experimeneter" field of the current file, as defined in a "file experimenter" tag.
     *
     *  @details This signal is emitted whenever runing <censorTag>"()" and a "file comment" tag is in <m_pTag> (generally through
     * <readTag>"()"). The file experimenter tag stores the actual experimenter as a string, which is sent through this signal.
     *
     * @param[in] e Experimenter in the current "file experimenter" tag being censored.
     *
     */
    void readingFileExperimenter(QString e);

    //=========================================================================================================
    /**
     *  @brief Send the "subject id" field of the current file, as defined in a "subject id" tag.
     *
     *  @details This signal is emitted whenever runing <censorTag>"()" and a "subject id" tag is in <m_pTag> (generally through
     * <readTag>"()"). The subject id tag stores the actual id as an integer value, which is sent through this signal.
     *
     * @param[in] i Subject id, in the current file.
     *
     */
    void readingSubjectId(int i);

    //=========================================================================================================
    /**
     *  @brief Send the "subject first name" field of the current file, as defined in a "subject first name" tag.
     *
     *  @details This signal is emitted whenever runing <censorTag>"()" and a "subject first name" tag is in <m_pTag> (generally through
     * <readTag>"()"). The subject first name tag stores the actual first name as a string value, which is sent through this signal.
     *
     * @param[in] fn Subject first name, in the current file.
     *
     */
    void readingSubjectFirstName(QString fn);

    //=========================================================================================================
    /**
     *  @brief Send the "subject middle name" field of the current file, as defined in a "subject middle name" tag.
     *
     *  @details This signal is emitted whenever runing <censorTag>"()" and a "subject middle name" tag is in <m_pTag> (generally through
     * <readTag>"()"). The subject middle name tag stores the actual middle name as a string value, which is sent through this signal.
     *
     * @param[in] mn Subject middle name, in the current file.
     *
     */
    void readingSubjectMiddleName(QString mn);

    //=========================================================================================================
    /**
     *  @brief Send the "subject last name" field of the current file, as defined in a "subject last name" tag.
     *
     *  @details This signal is emitted whenever runing <censorTag>"()" and a "subject last name" tag is in <m_pTag> (generally through
     * <readTag>"()"). The subject last name tag stores the actual last name as a string value, which is sent through this signal.
     *
     * @param[in] ln Subject last name, in the current file.
     *
     */
    void readingSubjectLastName(QString ln);

    //=========================================================================================================
    /**
     *  @brief Send the "subject birthday" field of the current file, as defined in a "subject birthday" tag.
     *
     *  @details This signal is emitted whenever runing <censorTag>"()" and a "subject birthday" tag is in <m_pTag> (generally through
     * <readTag>"()"). The subject birthday tag stores the actual birthday as a julian date value, which is transformed into a QDateTime
     * object and sent through this signal.
     *
     * @param[in] b Subject birthday, in the current file.
     *
     */
    void readingSubjectBirthday(QDate b);

    //=========================================================================================================
    /**
     *  @brief Send the "subject sex" field of the current file, as defined in a "subject sex" tag.
     *
     *  @details This signal is emitted whenever runing <censorTag>"()" and a "subject sex" tag is in <m_pTag> (generally through
     * <readTag>"()"). The subject sex tag stores the actual sex as a codified integer value: [0 = unknown, 1 = male, 2 = female]. This value can
     * be transformed to a meaningful string with the method <subjectSexToString>"()".
     *
     * @param[in] s Subject sex, in the current file.
     *
     */
    void readingSubjectSex(int s);

    //=========================================================================================================
    /**
     *  @brief Send the "subject hand" field of the current file, as defined in a "subject hand" tag.
     *
     *  @details This signal is emitted whenever runing <censorTag>"()" and a "subject handedness" tag is in <m_pTag> (generally through
     * <readTag>"()"). The subject hand tag stores the actual handedness as a codified integer value: [0 = unknown, 1 = right, 2 = left]. This value can
     * be transformed to a meaningful string with the method <subjectHandToString>"()".
     *
     * @param[in] h Subject hand, in the current file.
     *
     */
    void readingSubjectHand(int h);

    //=========================================================================================================
    /**
     *  @brief Send the "subject weight" field of the current file, as defined in a "subject weight" tag.
     *
     *  @details This signal is emitted whenever runing <censorTag>"()" and a "subject weight" tag is in <m_pTag> (generally through
     * <readTag>"()"). The subject weight tag stores the actual weight of the subject as a floating point value. This value is sent through this signal.
     *
     * @param[in] w Subject weight, in the current file.
     *
     */
    void readingSubjectWeight(float w);

    //=========================================================================================================
    /**
     *  @brief Send the "subject height" field of the current file, as defined in a "subject height" tag.
     *
     *  @details This signal is emitted whenever runing <censorTag>"()" and a "subject height" tag is in <m_pTag> (generally through
     * <readTag>"()"). The subject height tag stores the actual height of the subject as a floating point value. This value is sent through this signal.
     *
     * @param[in] h Subject height, in the current file.
     *
     */
    void readingSubjectHeight(float h);

    /**
     *  @brief Send the "subject comment" field of the current file, as defined in a "subject comment" tag.
     *
     *  @details This signal is emitted whenever runing <censorTag>"()" and a "subject comment" tag is in <m_pTag> (generally through
     * <readTag>"()"). The subject comment tag stores the actual comment of the subject as a string value. This value is sent through this signal.
     *
     * @param[in] c Subject comment, in the current file.
     *
     */
    void readingSubjectComment(QString c);

    //=========================================================================================================
    /**
     *  @brief Send the "subject his id" field of the current file, as defined in a "subject his id" tag.
     *
     *  @details This signal is emitted whenever runing <censorTag>"()" and a "subject his id" tag is in <m_pTag> (generally through
     * <readTag>"()"). The subject his id tag stores the actual his id of the subject as a string value. This value is sent through this signal.
     *
     * @param[in] hisId Subject his id, in the current file.
     *
     */
    void readingSubjectHisId(QString hisId);

    //=========================================================================================================
    /**
     *  @brief Send the "project id" field of the current file, as defined in a "project id" tag.
     *
     *  @details This signal is emitted whenever runing <censorTag>"()" and a "project id" tag is in <m_pTag> (generally through
     * <readTag>"()"). The project id tag stores the actual his id of the subject as an integer value. This value is sent through this signal.
     *
     * @param[in] pId Project id, in the current file.
     *
     */
    void readingProjectId(int pId);

    //=========================================================================================================
    /**
     *  @brief Send the "project name" field of the current file, as defined in a "project name" tag.
     *
     *  @details This signal is emitted whenever runing <censorTag>"()" and a "project name" tag is in <m_pTag> (generally through
     * <readTag>"()"). The project name tag stores the actual name of the project as a string value. This value is sent through this signal.
     *
     * @param[in] pName Project name in the current file.
     *
     */
    void readingProjectName(QString pName);

    //=========================================================================================================
    /**
     *  @brief Send the "project name" field of the current file, as defined in a "project name" tag.
     *
     *  @details This signal is emitted whenever runing <censorTag>"()" and a "project name" tag is in <m_pTag> (generally through
     * <readTag>"()"). The project name tag stores the actual name of the project as a string value. This value is sent through this signal.
     *
     * @param[in] pName Project name in the current file.
     *
     */
    void readingProjectAim(QString pAim);

    //=========================================================================================================
    /**
     *  @brief Send the "project aim" field of the current file, as defined in a "project aim" tag.
     *
     *  @details This signal is emitted whenever runing <censorTag>"()" and a "project aim" tag is in <m_pTag> (generally through
     * <readTag>"()"). The project aim tag stores the actual aim of the project as a string value. This value is sent through this signal.
     *
     * @param[in] pAim Project aim in the current file.
     *
     */
    void readingProjectPersons(QString pPersons);

    //=========================================================================================================
    /**
     *  @brief Send the "project comment" field of the current file, as defined in a "project comment" tag.
     *
     *  @details This signal is emitted whenever runing <censorTag>"()" and a "project comment" tag is in <m_pTag> (generally through
     * <readTag>"()"). The project comment tag stores the actual comment of the project as a string value. This value is sent through this signal.
     *
     * @param[in] pComment Project comment in the current file.
     *
     */
    void readingProjectComment(QString pComment);

    //=========================================================================================================
    /**
     *  @brief This signal is emitted whenever MRI volume data is found in the current file.
     *
     *  @details This signal is emitted whenever runing <censorTag>"()" and a mri data is in <m_pTag> (generally through
     * <readTag>"()"). MRI data can be used to reconstruct the subjects face and thus could be considered protected information.
     *
     * @param[in] f MRi information present in the current file.
     *
     */
    void mriDataFoundInFile(bool f);

    //=========================================================================================================
    /**
     *  @brief Send the "working directory" field of the MNE toolbox instance used to modify the current file, as defined in a "MNE Working Directory" tag.
     *
     *  @details This signal is emitted whenever runing <censorTag>"()" and a "MNE Working Directory" tag is in <m_pTag> (generally through
     * <readTag>"()"). The working directory tag stores the actual directorys of the project as a string value. This value is sent through this signal.
     *
     * @param[in] wdir MNE working directory in the current file.
     *
     */
    void readingMNEWorkingDir(QString wdir);

    //=========================================================================================================
    /**
     *  @brief Send the "command line" field of the MNE toolbox instance used to modify the current file, as defined in a "MNE Command Line" tag.
     *
     *  @details This signal is emitted whenever runing <censorTag>"()" and a "MNE Command Line" tag is in <m_pTag> (generally through
     * <readTag>"()"). The command line tag stores the actual MNE toolbox command used to modify the file, as a string value. This value is sent through this signal.
     *
     * @param[in] cl MNE working directory in the current file.
     *
     */
    void readingMNECommandLine(QString cl);

    //=========================================================================================================
    /**
     *  @brief Send the a signal to alert that the anonymization has finished and the output file is ready.
     *
     */
    void outFileReady();

public slots:
    //=========================================================================================================
    /**
     * Configure the input file to anonymize.
     *
     * @param[in] sFilePathIn  String containing the input file name including its path. Can be a relative or.
     * an absolute path.
     */
    int setInFile(const QString &sFilePathIn);

    //=========================================================================================================
    /**
     * Configure the output file to anonymize.
     *
     * @param[in] sFilePathOut String containing the output file name. Can be a relative or.
     * an absolute path.
     */
    int setOutFile(const QString &sFilePathOut);

    //=========================================================================================================
    /**
     * Configure the state of the FiffAnonymizer object's desired anonymization mode. If set to TRUE, apart from the
     * default information additional information will also be anonymized, like Subject's weight, height, sex or
     * handedness and the project's information too.
     *
     * @param[in] bFlag    Bool argument whether to use the brute mode.
     *
     */
    void setBruteMode(bool bFlag);

    //=========================================================================================================
    /**
     * If found in the fiff file, the measurement date information will be overwritten with the date specified
     * with this function.
     *
     * @param[in] d     QDateTime object containing the desired measurement day.
     */
    void setMeasurementDate(const QDateTime& d);

    //=========================================================================================================
    /**
     * If found in the fiff file, the measurement date information will be overwritten with the date specified
     * with this function.
     *
     * @param[in] d     QDate object containing the desired measurement day.
     */
    void setMeasurementDate(const QString& s);

    //=========================================================================================================
    /**
     * If found in the fiff file, the specified number of days will be subtracted from the measurement date information contained in each fif file.
     *
     * @param[in] iMeasDayOffset   Integer with the number of dates to subtract from the measurement date.
     */
    void setMeasurementDateOffset(int iMeasDateDaysOffset);

    //=========================================================================================================
    /**
     * If found in the fiff file, the specified number of days will be subtracted from the measurement date information contained in each fif file.
     *
     * @param[in] b   Integer with the number of dates to subtract from the measurement date.
     */
    void setUseMeasurementDateOffset(bool b);

    //=========================================================================================================
    /**
     * If found in the fiff file, subject's birthday information will be overwritten in the file in order to match the date specified with this function.
     *
     * @param[in] sSubjBirthday String containing the desired subject birthday date.
     */
    void setSubjectBirthday(const QString& sSubjBirthday);

    //=========================================================================================================
    /**
     * If found in the fiff file, subject's birthday information will be overwritten in the file in order to match the date specified with this function.
     *
     * @param[in] sSubjBirtday String containing the desired subject birthday date.
     */
    void setSubjectBirthday(const QDate& sSubjBirthday);

    //=========================================================================================================
    /**
     * If found in the fiff file, the specified number of days will be subtracted from the subject's birthday date information contained in each fif file.
     *
     * @param[in] iSubjBirthdayOffset  Integer with the number of dates to subtract from the subject's birthday date.
     */
    void setSubjectBirthdayOffset(int iSubjBirthdayOffset);

    //=========================================================================================================
    /**
     * If found in the fiff file, the specified number of days will be subtracted from the subject's birthday date information contained in each fif file.
     *
     * @param[in] b  Flag whether to use a birthday offset.
     */
    void setUseSubjectBirthdayOffset(bool b);

    //=========================================================================================================
    /**
     * Specifies the subject's id text information you want the output file to have. If a hisID tag is found in the fiff file,
     * then, the subject's hisID information will be changed to match the one specified with this method.
     *
     * @param[in] sSubjectHisId   String with the subject's id.
     */
    void setSubjectHisId(const QString& sSubjectHisId);

    //=========================================================================================================
    /**
     * Sets the state of the FiffAnonymizer object's desired verbose mode. If set to TRUE, different messages will be
     * printed on screen during the anonymizing process. If set to false only a single line confirmation message will be
     * printed on each execution.
     *
     * @param[in] bFlag    Bool argument whether to use the verbose mode.
     */
    void setVerboseMode(bool bFlag);

    //=========================================================================================================
    /**
     * Sets the FiffAnonymizer object's mode to anonymize information related to MNE toolbox like the
     * Working Directory or the command line used to process the data.
     *
     * @param[in] bFlag    Bool argument whether to anonymize MNE related information.
     */
    void setMNEEnvironmentMode(bool bFlag);

public:
    //=========================================================================================================
    /**
     * Returns the input file which will be anonymized. .
     *
     * @param[in, out] Returns a string containing the input file name including its path.
     */
    QString getFileNameIn() const;

    //=========================================================================================================
    /**
     * Returns the output file where anonymized data will be saved.
     *
     * @param[in, out] Returns a string containing the output file name including its path.
     */
    QString getFileNameOut() const;

    //=========================================================================================================
    /**
     * Retrieves the state of the FiffAnonyzer object's desired verbose mode.
     *
     * @param[in, out] Bool value with the actual verbose mode.
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
    QDate getSubjectBirthday();

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

    //=========================================================================================================
    /**
     * Check if the input file to be anonymized has already been set up.
     */
    bool isFileInSet() const;

    //=========================================================================================================
    /**
     * Check if the output file where to save the anonymized input filee has already been defined.
     */
    bool isFileOutSet() const;

    //=========================================================================================================
    /**
     * Check if the MNE Environment option has been set.
     */
    bool getMNEEnvironmentMode();

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
     * @param[in] pTag Pointer to the tag being read. Normally the input Tag.
     */
    void updateBlockTypeList();

    //=========================================================================================================
    /**
     * Acquisition sw stamps in the fiff file the version of the fiff standard used to code the fiff file. This
     * anonymizer class has been developed according to a specific fiff version of this standard. If in the future,
     * this standard were to change, it should be noted through this method.
     *
     * @param[in] pTag Pointer to the tag being read. Normally, the input Tag.
     */
    bool checkValidFiffFormatVersion();

    //=========================================================================================================
    /**
     * For a specific input tag, check if that tag belongs to a set of tags where relevant information should be
     * censored/anonymized. If so, perform such anonymization while copying the new tag into an output Tag.
     * This is the core method of the class where the actual anonymization takes place.
     */
    void censorTag();

    //=========================================================================================================
    /**
     *
     * @brief Transfrom the sex code of the subject into a string ["unknown", "male" or "female"].
     *
     * @details Fiff standard codes the sex of the subject as an integer value. This helper method allows to easily transform it into a
     * more user friendly format.
     *
     * @param[in] sexCode the code with the sex of the subject.
     *
     */
    inline QString subjectSexToString(int sexCode) const;

    //=========================================================================================================
    /**
     *
     * @brief Transfrom the handedness code of the subject into a string ["unknown", "right" or "left"], expressing the right or left-handness of the the
     * subject.
     *
     * @details Fiff standard codes the handness (or handedness) of the subject as an integer value. This helper method allows to easily transform it into a
     * more user friendly format.
     *
     * @param[in] handCode the code with the handedness of the subject.
     *
     */
    inline QString subjectHandToString(int handCode) const;

    //=========================================================================================================
    /**
     * @brief print string to console if the object is set to Verbose Mode on. Or if Silent Mode has not been set.
     *
     * @details Helper function that prints messages to the command line only if the object has been set to a verbose mode.
     * This wraps QDebug functionality inline. Specified here in header file. If the obj is not set to be in a
     * verbose mode, it does nothing. Messages can be printed to a single line (followed by an eol character) or
     * can be printed in the same line as previous one. Note that to achieve this, messages will be "retained" one
     * call, in order to check if next call requests to print in the same line.
     *
     * @param[in] str String to print.
     *
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
     * directory and will finally write the tag into the output file stream.
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
    bool m_bMNEEnvironmentMode;         /**< User's request to anonymize info related to the MNE toolbox.*/
    const double m_dMaxValidFiffVerion; /**< Maximum version of the Fiff file standard compatible with this application.*/

    QString m_sDefaultString;           /**< String to be used as substitution of other strings in a fiff file. */
    QString m_sDefaultShortString;      /**< Short string to be used as substitution of protected short strings in a fiff file. */
    QDateTime m_dDefaultDate;           /**< Date to be used as substitution of dates found in a fiff file. */

    QDateTime m_dMeasurementDate;       /**< Date to substitute the measuremnt date appearing in the file.*/
    int  m_iMeasurementDateOffset;      /**< Number of days to subtract from the measurement date.*/
    bool m_bUseMeasurementDateOffset;   /**< Flags use Measurement-date days offset.*/

    QString m_sFiffComment;             /**< Fiff comment string substitutor.*/
    QString m_sFiffExperimenter;        /**< Fiff experimenter string substitutor.*/

    int m_iSubjectId;                   /**< Subject's id substitutor.*/
    QString m_sSubjectFirstName;        /**< Subject's first name substitutor.*/
    QString m_sSubjectMidName;          /**< Subject's middle name substitutor.*/
    QString m_sSubjectLastName;         /**< Subject's last name substitutor.*/
    QDate m_dSubjectBirthday;       /**< Subject's birthday substitutor.*/
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
    QString m_sMNEWorkingDir;           /**< MNE Toolbox working directory used while processing the file.*/
    QString m_sMNECommand;              /**< MNE Toolbox command line used used while processing the file.*/
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
