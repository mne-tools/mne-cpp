//=============================================================================================================
/**
* @file     music.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017 Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the Music class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "music.h"
#include "FormFiles/musiccontrol.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MUSICEXTENSION;
using namespace ANSHAREDLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Music::Music()
: m_pControl(Q_NULLPTR)
, m_pMusicControl(Q_NULLPTR)
{

}


//*************************************************************************************************************

Music::~Music()
{

}


//*************************************************************************************************************

QSharedPointer<IExtension> Music::clone() const
{
    QSharedPointer<Music> pMusicClone(new Music);
    return pMusicClone;
}


//*************************************************************************************************************

void Music::init()
{
    m_pMusicControl = new MusicControl;
}


//*************************************************************************************************************

void Music::unload()
{

}


//*************************************************************************************************************

QString Music::getName() const
{
    return "STC Browser";
}


//*************************************************************************************************************

QMenu *Music::getMenu()
{
    return Q_NULLPTR;
}


//*************************************************************************************************************

QDockWidget *Music::getControl()
{
    if(!m_pControl) {
        m_pControl = new QDockWidget(tr("MUSIC"));
        m_pControl->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        m_pControl->setMinimumWidth(180);
        m_pControl->setWidget(m_pMusicControl);
    }

    return m_pControl;
}


//*************************************************************************************************************

QWidget *Music::getView()
{
    return Q_NULLPTR;
}
