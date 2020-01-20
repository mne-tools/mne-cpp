//=============================================================================================================
/**
 * @file     surfer.cpp
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
 * @brief    Definition of the Surfer class.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "surfer.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SURFEREXTENSION;
using namespace ANSHAREDLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Surfer::Surfer()
: m_pControl(Q_NULLPTR)
, m_pView(Q_NULLPTR)
{

}


//*************************************************************************************************************

Surfer::~Surfer()
{

}


//*************************************************************************************************************

QSharedPointer<IExtension> Surfer::clone() const
{
    QSharedPointer<Surfer> pSurferClone(new Surfer);
    return pSurferClone;
}


//*************************************************************************************************************

void Surfer::init()
{

}


//*************************************************************************************************************

void Surfer::unload()
{

}


//*************************************************************************************************************

QString Surfer::getName() const
{
    return "Surfer";
}


//*************************************************************************************************************

QMenu *Surfer::getMenu()
{
    return Q_NULLPTR;
}


//*************************************************************************************************************

QDockWidget *Surfer::getControl()
{
    if(!m_pControl) {
        m_pControl = new QDockWidget(tr("Surfer Control"));
        m_pControl->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        m_pControl->setMinimumWidth(180);
    }

    return m_pControl;
}


//*************************************************************************************************************

// check with owner ship and mdi area for garbage collection
QWidget *Surfer::getView()
{
    if(!m_pView) {
        //
        //Pial surface
        //
        m_pView = new View3DAnalyze(1);
        m_pView->setWindowTitle("Pial surface");
    }

    return m_pView;
}
