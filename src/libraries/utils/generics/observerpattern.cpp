//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     observerpattern.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     September 2017
 * @brief    Out-of-line definitions of @ref UTILSLIB::Subject — attach/detach observer bookkeeping and the @c notify() fan-out.
 *
 * Owns the storage for the static @c notifyEnabled flag and the
 * non-inline @c notify() loop. Observer destruction is the
 * caller's responsibility — @ref Subject only holds raw pointers
 * because many concrete observers live on the stack or are
 * owned by Qt parent chains that do not fit a smart-pointer
 * lifetime model.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "observerpattern.h"

using namespace UTILSLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Subject::~Subject()
{
}

//=============================================================================================================

void Subject::attach(IObserver* pObserver)
{
    m_Observers.insert(pObserver);
}

//=============================================================================================================

void Subject::detach(IObserver* pObserver)
{
    m_Observers.erase(m_Observers.find(pObserver));
    //m_Observers.erase(observer); //C++ <set> STL implementation
}

//=============================================================================================================

void Subject::notify()
{
    if(notifyEnabled)
    {
        t_Observers::const_iterator it = m_Observers.begin();
        for( ; it != m_Observers.end(); ++it)
            (*it)->update(this);
//        for(auto observer : m_Observers){
//            observer->update(this);
//        }
    }
}

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

bool Subject::notifyEnabled = true;
