//=============================================================================================================
/**
* @file     rawsettings.cpp
* @author   Florian Schlembach <florian.schlembach@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*           Jens Haueisen <jens.haueisen@tu-ilmenau.de>
* @version  1.0
* @date     January, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Florian Schlembach, Christoph Dinh, Matti Hamalainen and Jens Haueisen. All rights reserved.
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

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rawsettings.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNE_BROWSE_RAW_QT;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RawSettings::RawSettings(QObject *parent)
: QObject(parent)
, m_qSettings("mne-cpp","MNE_BROWSE_RAW_QT")
{
    init();
}


//*************************************************************************************************************

RawSettings::~RawSettings()
{
}


//*************************************************************************************************************

void RawSettings::init()
{
    //MainWindow
    //ToDo: ask for already stored setting in OS environment before setting them
    //e.g. if(!m_qSettings.contains("RawModel/window_size")) m_qSettings.setValue("window_size",MODEL_WINDOW_SIZE);

    //Window settings
    m_qSettings.beginGroup("MainWindow");
        m_qSettings.setValue("size",QSize(MAINWINDOW_WINDOW_SIZE_W,MAINWINDOW_WINDOW_SIZE_H));
    m_qSettings.endGroup();

    //RawModel
    m_qSettings.beginGroup("RawModel");
        m_qSettings.setValue("window_size",MODEL_WINDOW_SIZE);
        m_qSettings.setValue("reload_pos",MODEL_RELOAD_POS);
        m_qSettings.setValue("max_windows",MODEL_MAX_WINDOWS);
        m_qSettings.setValue("num_filter_taps",MODEL_NUM_FILTER_TAPS);
    m_qSettings.endGroup();

    //RawDelegate
    m_qSettings.beginGroup("RawDelegate");

        //look
        m_qSettings.setValue("plotheight",DELEGATE_PLOT_HEIGHT);
        m_qSettings.setValue("dx",DELEGATE_DX);
        m_qSettings.setValue("nhlines",DELEGATE_NHLINES);

//        //maximum values for different channels types according to FiffChInfo
        m_qSettings.setValue("max_meg_grad",DELEGATE_MAX_MEG_GRAD);
        m_qSettings.setValue("max_meg_mag",DELEGATE_MAX_MEG_MAG);
        m_qSettings.setValue("max_eeg",DELEGATE_MAX_EEG);
        m_qSettings.setValue("max_eog",DELEGATE_MAX_EOG);
        m_qSettings.setValue("max_stim",DELEGATE_MAX_STIM);

    m_qSettings.endGroup();
}
