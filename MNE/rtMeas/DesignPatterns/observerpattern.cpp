//=============================================================================================================
/**
* @file		observerpattern.cpp
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
* @brief	Contains implementations of the observer design pattern: Subject class and IObserver interface.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "observerpattern.h"


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Subject::~Subject()
{

}


//*************************************************************************************************************

void Subject::attach(IObserver* pObserver)
{
    m_Observers.insert(pObserver);
}


//*************************************************************************************************************

void Subject::detach(IObserver* pObserver)
{
	m_Observers.erase(m_Observers.find(pObserver));
    //m_Observers.erase(observer); //C++ <set> STL implementation
}


//*************************************************************************************************************

void Subject::notify()
{
	if(notifyEnabled)
	{
		t_Observers::const_iterator it = m_Observers.begin();
		for( ; it != m_Observers.end(); ++it)
			(*it)->update(this);
	}
}

//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

bool Subject::notifyEnabled = true;
