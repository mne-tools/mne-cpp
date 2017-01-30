//=============================================================================================================
/**
* @file     MainPage.qml
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     November, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Implements the QML MainPage code behind component.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// IMPORTS
//=============================================================================================================

import QtQuick 2.7

MainPageForm {
    button_close.onClicked: {
        Qt.quit();
    }

    // MNE Scan
    button_mne_scan.onClicked: {
        launchControl.invokeScan();
    }

    button_mne_scan.onHoveredChanged: {
        if ( button_mne_scan.hovered )
            state = "SCAN"
        else
            state = "DEFAULT"
    }

    // MNE Browse
    button_mne_browse.onClicked: {
        launchControl.invokeBrowse();
    }

    button_mne_browse.onHoveredChanged: {
        if ( button_mne_browse.hovered )
            state = "BROWSE"
        else
            state = "DEFAULT"
    }

    // MNE Analyze
    button_mne_analyze.onClicked: {
        launchControl.invokeAnalyze();
    }

    button_mne_analyze.onHoveredChanged: {
        if ( button_mne_analyze.hovered )
            state = "ANALYZE"
        else
            state = "DEFAULT"
    }

    states: [
        State {
            name: "DEFAULT"
        },
        State {
            name: "SCAN"
        },
        State {
            name: "BROWSE"
        },
        State {
            name: "ANALYZE"
        }
    ]
}
