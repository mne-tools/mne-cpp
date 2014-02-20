//=============================================================================================================
/**
* @file     rawsettings.h
* @author   Florian Schlembach <florian.schlembach@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*           Jens Haueisen <jens.haueisen@tu-ilmenau.de>
* @version  1.0
* @date     January, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Florian Schlembach, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Contains all application settings.
*
*/

#ifndef RAWSETTINGS_H
#define RAWSETTINGS_H

//=============================================================================================================
// INCLUDES

#include "info.h"
#include "types.h"

//Qt
#include <QObject>
#include <QSettings>
#include <QSize>

//=============================================================================================================
// CONSTANTS

//MainWindow
#define MAINWINDOW_WINDOW_SIZE_W 1200 //width of MainWindow
#define MAINWINDOW_WINDOW_SIZE_H 800 //width of MainWindow

//RawModel
#define MODEL_WINDOW_SIZE 4016 //this value+MODEL_NUM_FILTER_TAPS must be a multiple integer of 2^x (e.g. 4016 or 8112 for 80 filter taps), length of data window to preload [in samples]
#define MODEL_RELOAD_POS 2000 //Distance that the current window needs to be off the ends of m_data[i] [in samples]
#define MODEL_MAX_WINDOWS 3 //number of windows that are at maximum remained in m_data
#define MODEL_NUM_FILTER_TAPS 80 //number of filter taps, required to take into account because of FFT convolution (zero padding)

//RawDelegate
//Look
#define DELEGATE_PLOT_HEIGHT 40 //height of a single plot (row)
#define DELEGATE_DX 1 //each DX pixel a sample is plot -> plot resolution
#define DELEGATE_NHLINES 6 //number of horizontal lines within a single plot (row)

//maximum values for different channels types according to FiffChInfo
#define DELEGATE_MAX_MEG_GRAD 1e-10 // kind=FIFFV_MEG_CH && unit=FIFF_UNIT_T_M
#define DELEGATE_MAX_MEG_MAG 1e-11 // kind=FIFFV_MEG_CH && unit=FIFF_UNIT_T
#define DELEGATE_MAX_EEG 1e-4 // kind=FIFFV_EEG_CH
#define DELEGATE_MAX_EOG 1e-3 // kind=FIFFV_EOG_CH
#define DELEGATE_MAX_STIM 5 // kind=FIFFV_STIM_CH

//=============================================================================================================
// NAMESPACE

namespace MNE_BROWSE_RAW_QT {

//*************************************************************************************************************

class RawSettings : public QObject
{
    Q_OBJECT
public:
    RawSettings(QObject *parent = 0);
    ~RawSettings();

signals:

public slots:


private:
    /**
     * init initializes all the application settings values from the macros
     */
    void init();

    QSettings m_qSettings; /**< QSettings object that initializes all the RawSettings */
};

} //end namespace MNE_BROWSE_RAW_QT

#endif // RAWSETTINGS_H
