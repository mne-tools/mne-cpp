//=============================================================================================================
/**
* @file		IMeasurementacceptor.cpp
* @author	Christoph Dinh <christoph.dinh@live.de>;
* @version	1.0
* @date		October, 2010
*
* @section	LICENSE
*
* Copyright (C) 2010 Christoph Dinh. All rights reserved.
*
* No part of this program may be photocopied, reproduced,
* or translated to another program language without the
* prior written consent of the author.
*
*
* @brief	Contains the implementation of the IMeasurementAcceptor interface.
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
