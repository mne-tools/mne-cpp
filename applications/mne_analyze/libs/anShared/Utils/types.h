//=============================================================================================================
/**
 * @file     types.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @since    0.1.0
 * @date     March, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch, Lars Debor, Simon Heinke. All rights reserved.
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
 * @brief    Contains general application specific types
 *
 */
#ifndef ANSHARED_TYPES_H
#define ANSHARED_TYPES_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <inverse/dipoleFit/ecd.h>
#include <Eigen/Core>

//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;

//=============================================================================================================
// DEFINE NAMESPACE MNEANALYZE
//=============================================================================================================

namespace ANSHAREDLIB
{
    //=========================================================================================================
    /**
     * The following directory paths are only imaginary.
     * They should be used for models that are not stored to the file system yet.
     *
     * Convention: Imaginary paths start with '*', end with '/' and all characters are upper case.
     */
    #define ECD_SET_MODEL_DEFAULT_DIR_PATH  QStringLiteral("*ECDSETMODEL/")

    //=========================================================================================================
    /**
     * The MODEL_TYPE enum lists all available model types.
     * Naming convention: NAMESPACE_CLASSNAME_MODEL
     */
    enum MODEL_TYPE
    {
        ANSHAREDLIB_SURFACE_MODEL,
        ANSHAREDLIB_QENTITYLIST_MODEL,
        ANSHAREDLIB_ECDSET_MODEL,
        ANSHAREDLIB_FIFFRAW_MODEL,
        ANSHAREDLIB_ANNOTATION_MODEL
    };

    //=========================================================================================================
    /**
     * Public enum for all available Event types.
     */
    enum EVENT_TYPE
    {
        PING,                       // dummy event for testing and debuggin purposes
        PLUGIN_INIT_FINISHED,       // send when all plugins finished initializing
        STATUS_BAR_MSG,             // sending a message to the status bar (part of gui)
        SELECTED_MODEL_CHANGED,     // event send whenever the user changes the selection in the datamanager plugin
        NEW_ANNOTATION_ADDED,       // event send whenever the user adds a new annotation in the rawdataviewer plugin
        EVENT_GROUPS_UPDATED,       // send when plugins dependent on event groups need to be updated
        TRIGGER_REDRAW,             // send when viewer needs to be updated
        TRIGGER_ACTIVE_CHANGED,     // send when the trigger active state was toggled
        TRIGGER_VIEWER_MOVE,        // send when scroll position of viewer needs to be moved
        FILTER_CHANNEL_TYPE_CHANGED,// send when the channel type to be filtered changed
        FILTER_ACTIVE_CHANGED,      // send when the filter active state was toggled
        FILTER_DESIGN_CHANGED       // send when the designed filter changed
    };
} //NAMESPACE

#endif // TYPES_H
