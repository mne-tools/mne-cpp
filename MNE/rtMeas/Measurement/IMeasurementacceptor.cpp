//=============================================================================================================
/**
* @file     IMeasurementacceptor.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the IMeasurementAcceptor interface.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "IMeasurementacceptor.h"


#include <QDebug>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

IMeasurementAcceptor::IMeasurementAcceptor()
: m_pHashBuffers(new QHash<MSR_ID::Measurement_ID, Buffer*>)
{

}


//*************************************************************************************************************

IMeasurementAcceptor::~IMeasurementAcceptor()
{
//	foreach(Buffer* buf, m_pHashBuffers->values())

//    m_pBuffInp_r->clear();
//    m_pBuffInp_phi->clear();
//    m_pBuffInp_theta->clear();
//ToDo Cleanup STuff inside the buffer
    delete m_pHashBuffers;
}


//*************************************************************************************************************

void IMeasurementAcceptor::addModule(MDL_ID::Module_ID id)
{
    if(!m_qList_MDL_ID.contains(id))
        m_qList_MDL_ID.append(id);
}


//*************************************************************************************************************

void IMeasurementAcceptor::addAcceptorMeasurementBuffer(MSR_ID::Measurement_ID id, Buffer* buffer)
{
//ToDo test at the same time if measurement is accepted
    qDebug() << "inside adding Measurement";
    if(!m_pHashBuffers->contains(id))
        m_pHashBuffers->insert(id, buffer);
}


//*************************************************************************************************************

Buffer* IMeasurementAcceptor::getAcceptorMeasurementBuffer(MSR_ID::Measurement_ID id)
{
    if(m_pHashBuffers->contains(id))
        return m_pHashBuffers->value(id);

    return NULL;
}


//*************************************************************************************************************

void IMeasurementAcceptor::cleanAcceptor()
{
    qDebug()<<"IMeasurementAcceptor::clean(): Cleaning up IMeasurementAcceptor";

    m_pHashBuffers->clear();
}
