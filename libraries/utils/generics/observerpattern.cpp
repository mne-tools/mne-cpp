//=============================================================================================================
/**
 * @file     observerpattern.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    Contains implementations of the observer design pattern: Subject class and IObserver interface.
 *
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
