//=============================================================================================================
/**
 * @file     mne_show_fiff_settings.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  1.0
 * @date     February, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the MneShowFiffSettings class.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_show_fiff_settings.h"
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
#define PROGRAM_VERSION     "1.9"
#endif

#define DEFAULT_INDENT 3


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MneShowFiffSettings::MneShowFiffSettings()
: indent(-1)
, verbose(false)
, long_strings(false)
, blocks_only(false)
{

}


//*************************************************************************************************************

MneShowFiffSettings::MneShowFiffSettings(int *argc,char **argv)
: indent(-1)
, verbose(false)
, long_strings(false)
, blocks_only(false)
{
    if (!check_args(argc,argv))
        return;

    fprintf(stderr,"%s version %s compiled at %s %s\n",argv[0],PROGRAM_VERSION,__DATE__,__TIME__);

    checkIntegrity();
}


//*************************************************************************************************************

MneShowFiffSettings::~MneShowFiffSettings()
{
    //ToDo Garbage collection
}


//*************************************************************************************************************

void MneShowFiffSettings::checkIntegrity()
{
    if (this->indent < 0)
        this->indent = this->verbose ? 0 : DEFAULT_INDENT;
}


//*************************************************************************************************************

void MneShowFiffSettings::usage(char *name)
{
    fprintf(stderr,"usage: %s [options]\n",name);
    fprintf(stderr,"List contents of a fif file to stdout\n");
    fprintf(stderr,"\t--in name         The input file.\n");
    fprintf(stderr,"\t--fif name        Synonym for the above.\n");
    fprintf(stderr,"\t--blocks          Only list the blocks (the tree structure)\n");
    fprintf(stderr,"\t--verbose         Verbose output.\n");
    fprintf(stderr,"\t--indent no       Number of spaces to use in indentation (default %d in terse and 0 in verbose output)\n",indent);
    fprintf(stderr,"\t--tag no          Provide information about these tags (can have multiple of these).\n");
    fprintf(stderr,"\t--long            Print long strings in full?\n");
    fprintf(stderr,"\t--help            print this info.\n");
    fprintf(stderr,"\t--version         print version info.\n\n");
}


//*************************************************************************************************************

bool MneShowFiffSettings::check_unrecognized_args(int argc, char **argv)
{
    int k;

    if (argc > 1) {
        fprintf(stderr,"Unrecognized arguments : ");
        for (k = 1; k < argc; k++)
            fprintf(stderr,"%s ",argv[k]);
        fprintf(stderr,"\n");
        qCritical("Check the command line.");
        return false;
    }
    return true;
}


//*************************************************************************************************************

bool MneShowFiffSettings::check_args (int *argc,char **argv)
{
    int k;
    int p;
    int found,val;

    for (k = 0; k < *argc; k++) {
        found = 0;
        if (strcmp(argv[k],"--version") == 0) {
            fprintf(stderr,"%s version %s compiled at %s %s\n", argv[0],PROGRAM_VERSION,__DATE__,__TIME__);
            exit(0);
        }
        else if (strcmp(argv[k],"--help") == 0) {
            usage(argv[0]);
            exit(1);
        }
        else if (strcmp(argv[k],"--in") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical("--in: argument required.");
                return false;
            }
            inname = QString(argv[k+1]);
        }
        else if (strcmp(argv[k],"--fif") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical("--fif: argument required.");
                return false;
            }
            inname = QString(argv[k+1]);
        }
        else if (strcmp(argv[k],"--tag") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical("--tag: argument required.");
                return false;
            }
            if (sscanf(argv[k+1],"%d",&val) != 1) {
                qCritical("Incomprehensible tag number : %s",argv[k+1]);
                return false;
            }
            tags.append(val);
        }
        else if (strcmp(argv[k],"--indent") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical("--indent: argument required.");
                return false;
            }
            if (sscanf(argv[k+1],"%d",&val) != 1) {
                qCritical("Incomprehensible number : %s",argv[k+1]);
                return false;
            }
            if (val >= 0)
                indent = val;
        }
        else if (strcmp(argv[k],"--verbose") == 0) {
            found = 1;
            verbose = true;
        }
        else if (strcmp(argv[k],"--long") == 0) {
            found = 1;
            long_strings = true;
        }
        else if (strcmp(argv[k],"--blocks") == 0) {
            found       = 1;
            blocks_only = true;
            verbose     = false;
        }
        if (found) {
            for (p = k; p < *argc-found; p++)
                argv[p] = argv[p+found];
            *argc = *argc - found;
            k = k - found;
        }
    }
    return check_unrecognized_args(*argc,argv);
}
