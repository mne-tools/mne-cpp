//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     mne_show_fiff_settings.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @date     February, 2017
 * @version  dev
 * @brief    Definition of the MNEShowFiffSettings class.
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/log.h>
#include "mne_show_fiff_settings.h"

#include <base/base_global.h>
#include <stdio.h>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SHOWFIFF;


//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#ifndef PROGRAM_VERSION
#define PROGRAM_VERSION MNE_CPP_VERSION
#endif

constexpr auto DEFAULT_INDENT = 3;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEShowFiffSettings::MNEShowFiffSettings()
: indent(-1)
, verbose(false)
, long_strings(false)
, blocks_only(false)
{
}


//*************************************************************************************************************

MNEShowFiffSettings::MNEShowFiffSettings(int* argc, char** argv)
: indent(-1)
, verbose(false)
, long_strings(false)
, blocks_only(false)
{
    if (!check_args(argc, argv))
        return;

    mne::Log::info("{} version {} compiled at {}", argv[0], PROGRAM_VERSION, mne::build_date_time());

    checkIntegrity();
}


//*************************************************************************************************************

MNEShowFiffSettings::~MNEShowFiffSettings()
{
    //ToDo Garbage collection
}


//*************************************************************************************************************

void MNEShowFiffSettings::checkIntegrity()
{
    if (this->indent < 0)
        this->indent = this->verbose ? 0 : DEFAULT_INDENT;
}


//*************************************************************************************************************

void MNEShowFiffSettings::usage(char* name)
{
    mne::Log::info("usage: {} [options]", name);
    mne::Log::info("List contents of a fif file to stdout");
    mne::Log::info("\t--in name         The input file.");
    mne::Log::info("\t--fif name        Synonym for the above.");
    mne::Log::info("\t--blocks          Only list the blocks (the tree structure)");
    mne::Log::info("\t--verbose         Verbose output.");
    mne::Log::info("\t--indent no       Number of spaces to use in indentation (default {} in terse and 0 in verbose output)", indent);
    mne::Log::info("\t--tag no          Provide information about these tags (can have multiple of these).");
    mne::Log::info("\t--long            Print long strings in full?");
    mne::Log::info("\t--help            print this info.");
    mne::Log::info("\t--version         print version info.\n");
}


//*************************************************************************************************************

bool MNEShowFiffSettings::check_unrecognized_args(int argc, char** argv)
{
    if (argc > 1) {
        QStringList unrecognized;
        for (int k = 1; k < argc; k++)
            unrecognized << QString::fromLocal8Bit(argv[k]);
        mne::Log::critical("Unrecognized arguments : {}", unrecognized.join(' ').toStdString());
        mne::Log::critical("Check the command line.");
        return false;
    }
    return true;
}


//*************************************************************************************************************

bool MNEShowFiffSettings::check_args(int* argc, char** argv)
{
    int k;
    int p;
    int found, val;

    for (k = 0; k < *argc; k++) {
        found = 0;
        if (strcmp(argv[k], "--version") == 0) {
            mne::Log::info("{} version {} compiled at {}", argv[0], PROGRAM_VERSION, mne::build_date_time());
            exit(0);
        } else if (strcmp(argv[k], "--help") == 0) {
            usage(argv[0]);
            exit(1);
        } else if (strcmp(argv[k], "--in") == 0) {
            found = 2;
            if (k == *argc - 1) {
                mne::Log::critical("--in: argument required.");
                return false;
            }
            inname = QString(argv[k + 1]);
        } else if (strcmp(argv[k], "--fif") == 0) {
            found = 2;
            if (k == *argc - 1) {
                mne::Log::critical("--fif: argument required.");
                return false;
            }
            inname = QString(argv[k + 1]);
        } else if (strcmp(argv[k], "--tag") == 0) {
            found = 2;
            if (k == *argc - 1) {
                mne::Log::critical("--tag: argument required.");
                return false;
            }
            bool bOk = false;
            val = QString::fromUtf8(argv[k + 1]).toInt(&bOk);
            if (!bOk) {
                mne::Log::critical("Incomprehensible tag number : {}", argv[k + 1]);
                return false;
            }
            tags.append(val);
        } else if (strcmp(argv[k], "--indent") == 0) {
            found = 2;
            if (k == *argc - 1) {
                mne::Log::critical("--indent: argument required.");
                return false;
            }
            bool bOk = false;
            val = QString::fromUtf8(argv[k + 1]).toInt(&bOk);
            if (!bOk) {
                mne::Log::critical("Incomprehensible number : {}", argv[k + 1]);
                return false;
            }
            if (val >= 0)
                indent = val;
        } else if (strcmp(argv[k], "--verbose") == 0) {
            found = 1;
            verbose = true;
        } else if (strcmp(argv[k], "--long") == 0) {
            found = 1;
            long_strings = true;
        } else if (strcmp(argv[k], "--blocks") == 0) {
            found = 1;
            blocks_only = true;
            verbose = false;
        }
        if (found) {
            for (p = k; p < *argc - found; p++)
                argv[p] = argv[p + found];
            *argc = *argc - found;
            k = k - found;
        }
    }
    return check_unrecognized_args(*argc, argv);
}
