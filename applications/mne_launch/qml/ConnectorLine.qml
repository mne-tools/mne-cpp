//=============================================================================================================
/**
* @file     ConnectorLine.qml
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     November, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Christoph Dinh. All rights reserved.
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
* @brief    Implements QML ConnectorLine component.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// IMPORTS
//=============================================================================================================

import QtQuick 2.7
import QtQuick.Controls 2.0
import QtGraphicalEffects 1.0

Item{
    id: container

    property int x_start: 0
    property int y_start: 0

    property int x_end: 100
    property int y_end: 100

    property int x_mid: x_start + (x_end - x_start) / 2
    property int y_mid: y_start

    Canvas {
        id: lines_mne_scan

        opacity: 0.4

        // canvas size
        width: parent.width; height: parent.height

        // handler to override for drawing
        onPaint: {
            // get context to draw with
            var ctx = getContext("2d")
            // setup the stroke
            ctx.lineWidth = 2
            ctx.strokeStyle = "white"
            // begin a new path to draw
            ctx.beginPath()
            // start point
            ctx.moveTo(x_start,y_start)
            // mid line
            ctx.lineTo(x_mid,y_mid)
            // end line
            ctx.lineTo(x_end,y_end)
            // stroke using line width and stroke style
            ctx.stroke()
        }
    }
}
