//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2019-2026 MNE-CPP Authors
 *
 * @file     settingscontrollercl.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Wayne Mead <wayne.mead@uth.tmc.edu>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           John C. Mosher <John.C.Mosher@uth.tmc.edu>
 * @since    0.1.0
 * @date     September, 2019
 * @brief    SettingsControllerCl class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "settingscontrollercl.h"
#include "fiffanonymizer.h"
#include <mne/base/base_global.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <print>
#include <random>
#include <string_view>
#include <system_error>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEANONYMIZE;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

const char* const kDescription =
    "\nMNE-CPP Project. UT-Health (McGovern Medical School) Houston, Tx."
    "\nMNE_ANONYMIZE"
    "\nMain authors: Juan Garcia-Prieto <juangpc@gmail.com>"
    "\n              Wayne Mead <wayne.mead@uth.tmc.edu>"
    "\n              Lorenz Esch <lesch@mgh.harvard.edu>"
    "\n"
    "\nMNE_ANONYMIZE removes or modifies Personal Health Information and Personal Identifiable information from a FIFF file."
    "\n\nIf the information exists in the input file, the following fields will be anonymized in the output file:"
    "\n"
    "\n - Measurement Date (can be altered by some number of days)"
    "\n - MAC address of the acquisition computer"
    "\n - Text description of the acquisition system"
    "\n - Experimenter"
    "\n - Subject Id"
    "\n - Subject First Name"
    "\n - Subject's Middle Name"
    "\n - Subject's Last Name"
    "\n - Subject's Birthday date (can be altered by some number of days)"
    "\n - Subject's Sex (only with brute option)"
    "\n - Subject's Handedness (only with brute option)"
    "\n - Subject's Weight (only with brute option)"
    "\n - Subject's Height (only with brute option)"
    "\n - Subject's Text Comment"
    "\n - Subject's Hospital Id"
    "\n - Project's Id (only with brute option)"
    "\n - Project's Name (only with brute option)"
    "\n - Project's Aim (only with brute option)"
    "\n - Project's Persons"
    "\n - Project's Comment (only with brute option)"
    "\n - MNE Toolbox info: Working Directory"
    "\n - MNE Toolbox info: Command line used"
    "\n "
    "\n - Additionally if there is MRI data present in the file a warning message will appear.\n"
    "\nmne_anonymize works on the command line; mne_anonymize_gui is its graphical user interface.\n";

const char* const kGuiHint = "The graphical user interface of mne_anonymize is mne_anonymize_gui.";

std::vector<mne::CommandLineOption> options()
{
    return {
        {{"no-gui"}, "Accepted for compatibility; mne_anonymize always runs on the command line (the GUI is mne_anonymize_gui).", {}, {}},
        {{"version"}, "Show the version of this application.", {}, {}},
        {{"i", "in"}, "File to anonymize.", "infile", {}},
        {{"o", "out"}, "Output file <outfile>. Default \"_anonymized.fif\" will be attached to the input file name.", "outfile", {}},
        {{"v", "verbose"}, "Prints out more information, about each specific anonymized field. Default: false", {}, {}},
        {{"s", "silent"}, "Prints no output to the terminal, other than execution or configuration errors. Default: false", {}, {}},
        {{"d", "delete_input_file_after"},
         "Delete input fiff file after anonymization. A confirmation message will be prompted to the user."
         "Default: false",
         {},
         {}},
        {{"f", "avoid_delete_confirmation"}, "Avoid confirming the deletion of the input fiff file. Default: false", {}, {}},
        {{"b", "brute"},
         "Anonymize additional subject information like weight, height, sex and handedness, and project data,"
         " subject's data. See help above. Default: false",
         {},
         {}},
        {{"md", "measurement_date"}, "Specify the measurement date. Only when anonymizing a single file. Format: DDMMYYYY. Default: 01012000", "date", {}},
        {{"mdo", "measurement_date_offset"}, "Specify the number of days to subtract from the measurement date. Default: 0", "days", {}},
        {{"sb", "subject_birthday"}, "Specify the subject birthday date. Format: DDMMYYYY. Default: 01012000", "date", {}},
        {{"sbo", "subject_birthday_offset"}, "Specify the number of days to subtract from the subject's birthday. Default: 0", "days", {}},
        {{"his"}, "Specify the subject ID within the Hospital information system. Default: \"mne_anonymize\"", "id#", {}},
        {{"mne_environment"},
         "Anonymize information related to the MNE environment. "
         "If found in the file, Working Directory or command line tags will be anonymized.",
         {},
         {}},
    };
}

// The option named @p name, or nullptr
const mne::CommandLineOption* findOption(const std::vector<mne::CommandLineOption>& all, std::string_view name)
{
    for (const mne::CommandLineOption& option : all) {
        for (const std::string& optionName : option.names) {
            if (optionName == name) {
                return &option;
            }
        }
    }
    return nullptr;
}

// Splits compacted one-letter options as QCommandLineParser does: -vb is -v -b and -ifile.fif is -i file.fif.
// A single dash before a longer option name (-md) stays that option.
std::vector<std::string> expandCompactedOptions(const std::vector<std::string>& arguments)
{
    std::vector<mne::CommandLineOption> all = options();
    all.push_back({{"h", "help"}, {}, {}, {}});

    std::vector<std::string> expanded;
    bool optionsEnded = false;
    for (std::size_t i = 0; i < arguments.size(); ++i) {
        const std::string& argument = arguments[i];
        optionsEnded = optionsEnded || argument == "--";
        const bool compacted = i > 0 && !optionsEnded && argument.size() > 2 && argument[0] == '-' && argument[1] != '-' && argument.find('=') == std::string::npos;
        const std::string_view body = std::string_view(argument).substr(1);
        if (!compacted || findOption(all, body) != nullptr || findOption(all, body.substr(0, 1)) == nullptr) {
            expanded.push_back(argument);
            continue;
        }
        for (std::size_t k = 0; k < body.size(); ++k) {
            expanded.push_back("-" + std::string(1, body[k]));
            const mne::CommandLineOption* option = findOption(all, body.substr(k, 1));
            if (option != nullptr && !option->valueName.empty()) {
                if (k + 1 < body.size()) {
                    expanded.emplace_back(body.substr(k + 1));
                }
                break;
            }
        }
    }
    return expanded;
}

std::filesystem::path absolutePath(const std::filesystem::path& path)
{
    std::error_code error;
    return std::filesystem::absolute(path, error).lexically_normal();
}

std::string fileName(const std::filesystem::path& path)
{
    return mne::utf8_from_path(path.filename());
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SettingsControllerCl::SettingsControllerCl()
: m_pAnonymizer(std::make_shared<FiffAnonymizer>())
, m_sAppName(APPLICATION_NAME)
, m_sAppVer(APPLICATION_VERSION)
, m_sBuildDate(mne::build_date_time())
, m_sBuildHash(mne::build_hash())
, m_sProgramName("mne_anonymize")
, m_bGuiMode(false)
, m_bDeleteInputFileAfter(false)
, m_bDeleteInputFileConfirmation(true)
, m_bHisIdSpecified(false)
, m_bVerboseMode(false)
, m_bSilentMode(false)
, m_bInOutFileNamesEqual(false)
, m_bInputFileDeleted(false)
, m_bOutFileRenamed(false)
{
}

//=============================================================================================================

SettingsControllerCl::SettingsControllerCl(const std::vector<std::string>& arguments)
: SettingsControllerCl()
{
    initParser();
    if (parseInputs(arguments)) {
        mne::Log::critical("Something went wrong during the parsing of input options.");
        return;
    }

    printHeaderIfVerbose();
    printIfVerbose("Executing command: " + mne::join(arguments, " "));
}

//=============================================================================================================

void SettingsControllerCl::initParser()
{
    m_parser = mne::CommandLineParser(kDescription);
    m_parser.add_help_option();
    for (mne::CommandLineOption& option : options()) {
        m_parser.add_option(std::move(option));
    }
}

//=============================================================================================================

void SettingsControllerCl::showHelp() const
{
    std::print("{}", m_parser.help_text(m_sProgramName));
    std::exit(0);
}

//=============================================================================================================

int SettingsControllerCl::parseInputs(const std::vector<std::string>& arguments)
{
    if (!arguments.empty()) {
        m_sProgramName = fileName(mne::path_from_utf8(arguments.front()));
    }
    if (!m_parser.parse(expandCompactedOptions(arguments))) {
        std::println(stderr, "{}: {}", m_sAppName, m_parser.error_text());
        std::exit(1);
    }

    if (m_parser.is_set("help")) {
        showHelp();
    }

    if (m_parser.is_set("version")) {
        std::println("{} {}", m_sAppName, m_sAppVer);
        std::exit(0);
    }

    if (parseInOutFiles()) {
        return 1;
    }

    if (m_parser.is_set("verbose")) {
        m_bVerboseMode = true;
        m_pAnonymizer->setVerboseMode(true);
    }

    if (m_parser.is_set("silent")) {
        m_bSilentMode = true;
        m_bVerboseMode = false;
        m_pAnonymizer->setVerboseMode(false);
    }

    if (m_parser.is_set("brute")) {
        m_pAnonymizer->setBruteMode(true);
    }

    if (m_parser.is_set("delete_input_file_after")) {
        m_bDeleteInputFileAfter = true;
    }

    if (m_parser.is_set("avoid_delete_confirmation")) {
        m_bDeleteInputFileConfirmation = false;
    }

    if (m_parser.is_set("measurement_date")) {
        if (m_parser.is_set("measurement_date_offset")) {
            mne::Log::critical("You cannot specify the measurement date and the measurement date offset at the same time.");
            showHelp();
        }
        m_pAnonymizer->setMeasurementDate(m_parser.value("measurement_date"));
    }

    if (m_parser.is_set("measurement_date_offset")) {
        m_pAnonymizer->setUseMeasurementDateOffset(true);
        m_pAnonymizer->setMeasurementDateOffset(mne::to_int(m_parser.value("measurement_date_offset")).value_or(0));
    }

    if (m_parser.is_set("subject_birthday")) {
        if (m_parser.is_set("subject_birthday_offset")) {
            mne::Log::critical("You cannot specify the subject's birthday and subject's birthday offsetthe same time.");
            showHelp();
        }
        m_pAnonymizer->setSubjectBirthday(m_parser.value("subject_birthday"));
    }

    if (m_parser.is_set("subject_birthday_offset")) {
        m_pAnonymizer->setUseSubjectBirthdayOffset(true);
        m_pAnonymizer->setSubjectBirthdayOffset(mne::to_int(m_parser.value("subject_birthday_offset")).value_or(0));
    }

    if (m_parser.is_set("his")) {
        m_bHisIdSpecified = true;
        m_pAnonymizer->setSubjectHisId(m_parser.value("his"));
    }

    if (m_parser.is_set("mne_environment")) {
        m_pAnonymizer->setMNEEnvironmentMode(true);
    }

    return 0;
}

//=============================================================================================================

int SettingsControllerCl::parseInOutFiles()
{
    if (m_parser.is_set("in")) {
        m_fiInFile = mne::path_from_utf8(m_parser.value("in"));
        std::error_code error;
        if (std::filesystem::is_regular_file(m_fiInFile, error)) {
            if (m_pAnonymizer->setInFile(mne::utf8_from_path(absolutePath(m_fiInFile)))) {
                mne::Log::critical("Error while setting the input file.");
                return 1;
            }
        } else {
            mne::Log::critical("Input file is not a file.");
            return 1;
        }
    } else {
        if (!m_bGuiMode) {
            mne::Log::critical("No valid input file specified.");
            if (!m_parser.is_set("no-gui")) {
                // Without --no-gui, mne_anonymize used to open its graphical user interface
                mne::Log::critical("{}", kGuiHint);
                std::print("{}", m_parser.help_text(m_sProgramName));
                std::exit(1);
            }
            showHelp();
        }
    }

    if (m_parser.is_set("out")) {
        m_fiOutFile = mne::path_from_utf8(m_parser.value("out"));
        std::error_code error;
        if (std::filesystem::is_directory(m_fiOutFile, error)) {
            mne::Log::critical("Error. Output file is infact a folder.");
            return 1;
        } else {
            if (absolutePath(m_fiInFile) == absolutePath(m_fiOutFile)) {
                m_bInOutFileNamesEqual = true;
                m_fiOutFile = absolutePath(m_fiInFile).parent_path() / mne::path_from_utf8(generateRandomFileName());
            }
            if (m_pAnonymizer->setOutFile(mne::utf8_from_path(absolutePath(m_fiOutFile)))) {
                mne::Log::critical("Error while setting the output file.");
                return 1;
            }
        }
    } else {
        if (!m_bGuiMode) {
            generateDefaultOutputFileName();
            if (m_pAnonymizer->setOutFile(mne::utf8_from_path(absolutePath(m_fiOutFile)))) {
                mne::Log::critical("Error while setting the output file.");
                return 1;
            }
        }
    }
    return 0;
}

//=============================================================================================================

int SettingsControllerCl::run()
{
    if (m_pAnonymizer->anonymizeFile()) {
        mne::Log::critical("Error. Program ends now.");
        return 1;
    }

    if (checkDeleteInputFile()) {
        deleteInputFile();
    }

    if (checkRenameOutputFile()) {
        renameOutputFileAsInputFile();
    }

    if (!m_bSilentMode) {
        mne::Log::info("\n{}", "MNE Anonymize finished correctly: " + fileName(m_fiInFile) + " -> " + fileName(m_fiOutFile));
    }

    printFooterIfVerbose();

    return 0;
}

//=============================================================================================================

bool SettingsControllerCl::checkDeleteInputFile()
{
    if (m_bDeleteInputFileAfter) //false by default
    {
        if (!m_bSilentMode) {
            mne::Log::info("You have requested to delete the input file: {}", fileName(m_fiInFile));
        }

        if (m_bDeleteInputFileConfirmation) //true by default
        {
            mne::Log::info("You can avoid this confirmation by using the delete_confirmation [-f] option.");
            // A prompt is not a log message: it must show regardless of the log level.
            std::print("Are you sure you want to delete the input file? [Y/n] ");
            std::fflush(stdout);
            std::string confirmation;
            std::cin >> confirmation;

            if (confirmation == "Y") {
                return true;
            }
        } else {
            return true;
        }
    }
    return false;
}

//=============================================================================================================

void SettingsControllerCl::deleteInputFile()
{
    const std::filesystem::path inFile = absolutePath(m_fiInFile);
    std::error_code error;
    if ((m_bInputFileDeleted = std::filesystem::remove(inFile, error))) {
        printIfVerbose("Input file deleted.");
    } else {
        mne::Log::critical("Unable to delete the input file: {}", mne::utf8_from_path(inFile));
    }
}

//=============================================================================================================

bool SettingsControllerCl::checkRenameOutputFile()
{
    //if both files in and out have the same name, this controller class would already know and a temporary
    //random filename will be in use, during the anonymizing process, for the output file.
    //When this function is called we will check if this needs to be reverted:
    // -if the infile has been deleted already there is no conflict->outfile name = infile name.
    // -if the infile has not been deleted but the user has never been asked. They is asked.
    // -if the infile has not been deleted but the user was already asked, it means they answered NO.
    //      Thus, a warning is shown.
    if (m_bInOutFileNamesEqual) {
        if (m_bDeleteInputFileAfter) {
            if (m_bInputFileDeleted) {
                return true;
            }
        } else {
            m_bDeleteInputFileAfter = true;
            if (checkDeleteInputFile()) {
                deleteInputFile();
                return true;
            } else {
                if (!m_bSilentMode) {
                    mne::Log::warning("You have requested to save the output file with the same name as the input file.");
                    mne::Log::warning("This cannot be done without deleting or modifying the input file.");
                }
            }
        }
    }

    return false;
}

//=============================================================================================================

void SettingsControllerCl::renameOutputFileAsInputFile()
{
    const std::filesystem::path auxFile = absolutePath(m_fiOutFile);
    const std::filesystem::path inFile = absolutePath(m_fiInFile);
    std::error_code error;
    // Like QFile::rename, which does not overwrite an existing file
    if (!std::filesystem::exists(inFile, error)) {
        std::filesystem::rename(auxFile, inFile, error);
        m_bOutFileRenamed = !error;
    } else {
        m_bOutFileRenamed = false;
    }
    if (m_bOutFileRenamed) {
        if (m_bVerboseMode) {
            mne::Log::info("Output file named: {} --> renamed as: {}", fileName(m_fiOutFile), fileName(m_fiInFile));
        }
        m_fiOutFile = inFile;
    } else {
        mne::Log::critical("Error while renaming the output file: {} as {}", mne::utf8_from_path(auxFile), fileName(m_fiInFile));
    }
}

//=============================================================================================================

void SettingsControllerCl::printHeaderIfVerbose()
{
    printIfVerbose(" ");
    printIfVerbose("=============================================================================================");
    printIfVerbose(" ");
    printIfVerbose(m_sAppName + "    Version: " + m_sAppVer);
    printIfVerbose("Build Date: " + m_sBuildDate);
    printIfVerbose("Build Hash: " + m_sBuildHash);
    printIfVerbose(" ");
    printIfVerbose(std::string("Fiff Lib Build Date: ") + mne::build_date_time());
    printIfVerbose(std::string("Fiff Lib Build Hash: ") + mne::build_hash());
    printIfVerbose(" ");
}

//=============================================================================================================

void SettingsControllerCl::printFooterIfVerbose()
{
    printIfVerbose("=============================================================================================");
    printIfVerbose(" ");
}

//=============================================================================================================

void SettingsControllerCl::printIfVerbose(const std::string& str) const
{
    if (m_bVerboseMode) {
        mne::Log::info("{}", str);
    }
}

//=============================================================================================================

std::string SettingsControllerCl::generateRandomFileName()
{
    std::string randomFileName("mne_anonymize_");
    const std::string_view charPool("abcdefghijklmnopqrstuvwxyz1234567890");
    const int randomLength(12);

    // Like the QRandomGenerator::bounded(randomLength) it replaces, only the first randomLength characters are drawn
    static std::mt19937 generator{std::random_device{}()};
    std::uniform_int_distribution<int> distribution(0, randomLength - 1);
    for (int i = 0; i < randomLength; ++i) {
        randomFileName.push_back(charPool[static_cast<std::size_t>(distribution(generator))]);
    }

    return randomFileName.append(".fif");
}

//=============================================================================================================

std::string SettingsControllerCl::generateDefaultOutputFileName()
{
    // QFileInfo's baseName() and completeSuffix(): the name up to and after its first dot
    const std::string name = fileName(m_fiInFile);
    const std::size_t dot = name.find('.');
    const std::string baseName = name.substr(0, dot);
    const std::string completeSuffix = dot == std::string::npos ? std::string() : name.substr(dot + 1);
    m_fiOutFile = absolutePath(m_fiInFile).parent_path() / mne::path_from_utf8(baseName + "_anonymized." + completeSuffix);
    return mne::utf8_from_path(m_fiOutFile);
}

//=============================================================================================================

std::filesystem::path SettingsControllerCl::getFiInFile() const
{
    return m_fiInFile;
}

//=============================================================================================================

std::filesystem::path SettingsControllerCl::getFiOutFile() const
{
    return m_fiOutFile;
}
