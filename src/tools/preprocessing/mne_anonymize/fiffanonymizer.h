//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     fiffanonymizer.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Wayne Mead <wayne.mead@uth.tmc.edu>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           John C. Mosher <John.C.Mosher@uth.tmc.edu>
 * @since    0.1.0
 * @date     August, 2019
 * @brief     FiffAnonymizer class declaration.
 */

#ifndef MNEANONYMIZE_FIFFANONYMIZER_H
#define MNEANONYMIZE_FIFFANONYMIZER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/log.h>
#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_tag.h>
#include <mne/fiff/fiff_types.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE
//=============================================================================================================

namespace MNEANONYMIZE
{

//=============================================================================================================
/**
 * Functions the FiffAnonymizer calls while it reads the input file, one per kind of protected information, with
 * the value found in the file. Any of them may be empty. The GUI uses them to show the file's content.
 *
 * @brief Notifications of a FiffAnonymizer.
 *
 * @since 3.0.0
 */
struct FiffAnonymizerCallbacks
{
    std::function<void(double)> readingIdFileVersion;                 /**< FIFF standard version of the file ID tag. */
    std::function<void(std::int64_t)> readingIdMeasurementDate;       /**< Measurement date in an ID tag, seconds since the epoch. */
    std::function<void(const std::string&)> readingIdMac;             /**< MAC address in the file ID tag. */
    std::function<void(std::int64_t)> readingFileMeasurementDate;     /**< Measurement date tag, seconds since the epoch. */
    std::function<void(const std::string&)> readingFileComment;       /**< File comment. */
    std::function<void(const std::string&)> readingFileExperimenter;  /**< Experimenter. */
    std::function<void(int)> readingSubjectId;                        /**< Subject id. */
    std::function<void(const std::string&)> readingSubjectFirstName;  /**< Subject first name. */
    std::function<void(const std::string&)> readingSubjectMiddleName; /**< Subject middle name. */
    std::function<void(const std::string&)> readingSubjectLastName;   /**< Subject last name. */
    std::function<void(std::int64_t)> readingSubjectBirthday;         /**< Subject birthday as a Julian day. */
    std::function<void(int)> readingSubjectSex;                       /**< Subject sex [0 = unknown, 1 = male, 2 = female]. */
    std::function<void(int)> readingSubjectHand;                      /**< Subject handedness [0 = unknown, 1 = right, 2 = left, 3 = ambidextrous]. */
    std::function<void(float)> readingSubjectWeight;                  /**< Subject weight. */
    std::function<void(float)> readingSubjectHeight;                  /**< Subject height. */
    std::function<void(const std::string&)> readingSubjectComment;    /**< Subject comment. */
    std::function<void(const std::string&)> readingSubjectHisId;      /**< Subject id in the hospital information system. */
    std::function<void(int)> readingProjectId;                        /**< Project id. */
    std::function<void(const std::string&)> readingProjectName;       /**< Project name. */
    std::function<void(const std::string&)> readingProjectAim;        /**< Project aim. */
    std::function<void(const std::string&)> readingProjectPersons;    /**< Project persons. */
    std::function<void(const std::string&)> readingProjectComment;    /**< Project comment. */
    std::function<void(bool)> mriDataFoundInFile;                     /**< MRI data, from which a face can be reconstructed, is in the file. */
    std::function<void(const std::string&)> readingMNEWorkingDir;     /**< Working directory of the MNE toolbox that modified the file. */
    std::function<void(const std::string&)> readingMNECommandLine;    /**< Command line of the MNE toolbox that modified the file. */
    std::function<void()> outFileReady;                               /**< The anonymization finished and the output file is ready. */
};

//=============================================================================================================
/**
 * This class opens an input fif file, reads sequentially  all its tags (linked list) and saves
 * an anonymized copy of each tag into an output file.
 *
 * @details The class is intentionally concieved as "not-smart" or restricted to the actual anonymization
 * process. It has several getters and setters, and a main anonymizer member method. Several member variable help
 * keep track of how the anonymization process should take place, but the anonymization flow is intended to be
 * taken care of by a controller. Dates are local dates and times, as the measurement and birthday dates in a FIFF
 * file are shown and replaced in the local time zone.
 *
 * @brief FiffAnonymizer class declaration.
 *
 */
class FiffAnonymizer
{
public:
    using SPtr = std::shared_ptr<FiffAnonymizer>;            /**< Shared pointer type for FiffAnonymizer. */
    using ConstSPtr = std::shared_ptr<const FiffAnonymizer>; /**< Const shared pointer type for FiffAnonymizer. */

    //=========================================================================================================
    /**
     * Constructs a FiffAnonymizer object.
     */
    FiffAnonymizer();

    //=========================================================================================================
    /**
     * Copy constructor for FiffAnonymizer object. The callbacks are not copied.
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
    FiffAnonymizer& operator=(const FiffAnonymizer& t) = delete;

    //=========================================================================================================
    /**
     * Move contructor. The callbacks are not moved.
     *
     * @param[in] a FiffAnonymizer object.
     */
    FiffAnonymizer(FiffAnonymizer&& obj) noexcept;

    //=========================================================================================================
    /**
     * @brief Anonymize the input file and save resutls in the output file.
     *
     * @details This method is the main method in the class. It goes through the input file and tag by tag
     * analyses if there might be some relevant information to anonymize and eventually does so. Initially it
     * checks for valid file formating.
     *
     * @return 0 on success, 1 if the files are not set or cannot be opened.
     */
    int anonymizeFile();

    //=========================================================================================================
    /**
     * Sets the functions called while the input file is read.
     *
     * @param[in] callbacks   The functions; empty ones are not called.
     *
     * @since 3.0.0
     */
    void setCallbacks(FiffAnonymizerCallbacks callbacks);

    //=========================================================================================================
    /**
     * Configure the input file to anonymize.
     *
     * @param[in] sFilePathIn  UTF-8 path of the input file. Can be a relative or an absolute path.
     */
    int setInFile(const std::string& sFilePathIn);

    //=========================================================================================================
    /**
     * Configure the output file to anonymize.
     *
     * @param[in] sFilePathOut UTF-8 path of the output file. Can be a relative or an absolute path.
     *
     * @return 1 if it is the input file, 0 otherwise.
     */
    int setOutFile(const std::string& sFilePathOut);

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
     * with this function, at 01:01:00 local time.
     *
     * @param[in] d     The desired measurement day; a date that is not ok() writes 0 seconds.
     */
    void setMeasurementDate(const std::chrono::year_month_day& d);

    //=========================================================================================================
    /**
     * If found in the fiff file, the measurement date information will be overwritten with the date specified
     * with this function, at 01:01:00 local time.
     *
     * @param[in] s     The desired measurement day, as DDMMYYYY.
     */
    void setMeasurementDate(const std::string& s);

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
     * @param[in] sSubjBirthday String containing the desired subject birthday date, as DDMMYYYY.
     */
    void setSubjectBirthday(const std::string& sSubjBirthday);

    //=========================================================================================================
    /**
     * If found in the fiff file, subject's birthday information will be overwritten in the file in order to match the date specified with this function.
     *
     * @param[in] dSubjBirthday The desired subject birthday date.
     */
    void setSubjectBirthday(const std::chrono::year_month_day& dSubjBirthday);

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
     * @param[in] sSubjectHisId   UTF-8 string with the subject's id.
     */
    void setSubjectHisId(const std::string& sSubjectHisId);

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

    //=========================================================================================================
    /**
     * Returns the input file which will be anonymized. .
     *
     * @return The absolute UTF-8 path of the input file.
     */
    std::string getFileNameIn() const;

    //=========================================================================================================
    /**
     * Returns the output file where anonymized data will be saved.
     *
     * @return The absolute UTF-8 path of the output file.
     */
    std::string getFileNameOut() const;

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
     * Retrieve the value of Date to substitute the measuremnt date appearing in the file. The time is always
     * 01:01:00 local time.
     */
    std::chrono::year_month_day getMeasurementDate() const;

    //=========================================================================================================
    /**
     * Get the value of Number of days to subtract from the measurement date.
     */
    int getMeasurementDayOffset() const;

    //=========================================================================================================
    /**
     * Get value of Flags to use Measurement-date days offset.
     */
    bool getUseMeasurementDayOffset() const;

    //=========================================================================================================
    /**
     * Get value of Subject's birthday.
     */
    std::chrono::year_month_day getSubjectBirthday() const;

    //=========================================================================================================
    /**
     * Get value of Flags use of Subject's birthday offset.
     */
    bool getUseSubjectBirthdayOffset() const;

    //=========================================================================================================
    /**
     * Get value of Subjects's birthday offset.
     */
    int getSubjectBirthdayOffset() const;

    //=========================================================================================================
    /**
     * Get value of Subjects's His Id text.
     */
    std::string getSubjectHisID() const;

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
    bool getMNEEnvironmentMode() const;

private:
    //=========================================================================================================
    /**
     * Updates a stack (m_blockTypeList) with the type of block the input stream is in.
     * While reading an input file as a data stream, we will read tag by tag. Each tag can be inside a specific
     * block, as defined in Fiff file standard docs. FIFF_COMMENT tag (#206) will need to be censored/anonymized
     * only if it is contained inside a block of type "measurement info". Thus, we need to keep track of which
     * are we in, while reading. We do this by checking the first element in this stack (see censorTag() in case
     * FIFF_COMMENT.
     * This is a refactoring method, designed to make anonymizeFile() more readable.
     */
    void updateBlockTypeList();

    //=========================================================================================================
    /**
     * Acquisition sw stamps in the fiff file the version of the fiff standard used to code the fiff file. This
     * anonymizer class has been developed according to a specific fiff version of this standard. If in the future,
     * this standard were to change, it should be noted through this method.
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
     * Replaces the measurement date in seconds since the epoch, as configured: by the offset or by the date.
     *
     * @param[in] inSecs   The date found in the file.
     *
     * @return The date to write.
     */
    std::int64_t censorMeasurementDate(std::int64_t inSecs) const;

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
    std::string subjectSexToString(int sexCode) const;

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
    std::string subjectHandToString(int handCode) const;

    //=========================================================================================================
    /**
     * @brief print string to console if the object is set to Verbose Mode on.
     *
     * @param[in] str String to print.
     *
     */
    void printIfVerbose(const std::string& str) const;

    //=========================================================================================================
    /**
     * @return The current tag's text up to its first NUL, as UTF-8 with invalid bytes replaced by U+FFFD.
     */
    std::string tagText() const;

    //=========================================================================================================
    /**
     * Replaces the current tag's payload with @p text.
     *
     * @param[in] text   The new UTF-8 text.
     */
    void setTagText(const std::string& text);

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
     * Will read a tag and update the block type list and stores it in m_pTag.
     */
    void readTag();

    //=========================================================================================================
    /**
     * Will overwrite the 'next' field in the tag stored in m_pTag. It will store the output tag in the tag
     * directory and will finally write the tag into the output file stream.
     */
    void writeTag();

    //=========================================================================================================

    std::unique_ptr<mne::FiffStream> m_pInStream;  /**< Pointer to FiffStream object for reading.*/
    std::unique_ptr<mne::FiffStream> m_pOutStream; /**< Pointer to FiffStream object for writing the result.*/
    std::optional<mne::FiffTag> m_pTag;            /**< Pointer to FiffTag used for reading and writing each tag.*/

    mne::fiff_int_t m_BDfltMAC[2]; /**< MAC addresss substitutor.*/

    std::vector<std::int32_t> m_blockTypeList; /**< Stack storing info related to the blocks of tags in the file.*/

    FiffAnonymizerCallbacks m_callbacks; /**< Functions called while reading the input file. */

    std::filesystem::path m_fFileIn;  /**< Input file.*/
    std::filesystem::path m_fFileOut; /**< Output file.*/

    bool m_bFileInSet;  /**< Input file set.*/
    bool m_bFileOutSet; /**< Output file set.*/

    bool m_bVerboseMode;                /**< Verbosity mode enabler.*/
    bool m_bBruteMode;                  /**< Advanced anonymization. Anonymize also weight, height and some other fields.*/
    bool m_bMNEEnvironmentMode;         /**< User's request to anonymize info related to the MNE toolbox.*/
    const double m_dMaxValidFiffVerion; /**< Maximum version of the Fiff file standard compatible with this application.*/

    std::string m_sDefaultString;               /**< String to be used as substitution of other strings in a fiff file. */
    std::string m_sDefaultShortString;          /**< Short string to be used as substitution of protected short strings in a fiff file. */
    std::chrono::year_month_day m_dDefaultDate; /**< Date to be used as substitution of dates found in a fiff file, at 01:01:00. */

    std::chrono::year_month_day m_dMeasurementDate; /**< Date to substitute the measuremnt date appearing in the file, at 01:01:00.*/
    int m_iMeasurementDateOffset;                   /**< Number of days to subtract from the measurement date.*/
    bool m_bUseMeasurementDateOffset;               /**< Flags use Measurement-date days offset.*/

    std::string m_sFiffComment;      /**< Fiff comment string substitutor.*/
    std::string m_sFiffExperimenter; /**< Fiff experimenter string substitutor.*/

    int m_iSubjectId;                               /**< Subject's id substitutor.*/
    std::string m_sSubjectFirstName;                /**< Subject's first name substitutor.*/
    std::string m_sSubjectMidName;                  /**< Subject's middle name substitutor.*/
    std::string m_sSubjectLastName;                 /**< Subject's last name substitutor.*/
    std::chrono::year_month_day m_dSubjectBirthday; /**< Subject's birthday substitutor.*/
    int m_iSubjectBirthdayOffset;                   /**< Subjects's birthday offset.*/
    bool m_bUseSubjectBirthdayOffset;               /**< Flags use of Subject's birthday offset.*/
    std::string m_sSubjectComment;                  /**< Subject's comment substitutor.*/
    int m_iSubjectSex;                              /**< Subject's sex substitutor.*/
    int m_iSubjectHand;                             /**< Subject's hand substitutor.*/
    float m_fSubjectWeight;                         /**< Subject's weight substitutor.*/
    float m_fSubjectHeight;                         /**< Subject's height substitutor.*/
    std::string m_sSubjectHisId;                    /**< Subject's HIS ID substitutor.*/

    int m_iProjectId;              /**< Project's id# substitutor.*/
    std::string m_sProjectName;    /**< Project's name substitutor.*/
    std::string m_sProjectAim;     /**< Project's aim substitutor.*/
    std::string m_sProjectPersons; /**< Project's Persons substitutor.*/
    std::string m_sProjectComment; /**< Project's comment substitutor.*/
    std::string m_sMNEWorkingDir;  /**< MNE Toolbox working directory used while processing the file.*/
    std::string m_sMNECommand;     /**< MNE Toolbox command line used used while processing the file.*/
};

} // namespace MNEANONYMIZE

#endif // MNEANONYMIZE_FIFFANONYMIZER_H
