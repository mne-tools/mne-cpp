//=============================================================================================================
/**
* @file     ecd_set.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     December, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Implementation of the Electric Current Dipole Set (ECDSet) Class.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ecd_set.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVERSELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ECDSet::ECDSet()
{
}


//*************************************************************************************************************

ECDSet::ECDSet(const ECDSet &p_ECDSet)
: dataname(p_ECDSet.dataname)
, m_qListDips(p_ECDSet.m_qListDips)
{

}


//*************************************************************************************************************

ECDSet::~ECDSet()
{

}


//*************************************************************************************************************

void ECDSet::addEcd(const ECD& p_ecd)
{
    m_qListDips.append(p_ecd);
}


//*************************************************************************************************************

const ECD& ECDSet::operator[] (qint32 idx) const
{
    if (idx>=m_qListDips.length())
    {
        qWarning("Warning: Required ECD doesn't exist! Returning ECD '0'.");
        idx=0;
    }
    return m_qListDips[idx];
}


//*************************************************************************************************************

ECD& ECDSet::operator[] (qint32 idx)
{
    if (idx >= m_qListDips.length())
    {
        qWarning("Warning: Required ECD doesn't exist! Returning ECD '0'.");
        idx = 0;
    }
    return m_qListDips[idx];
}


//*************************************************************************************************************

ECDSet &ECDSet::operator<<(const ECD &p_ecd)
{
    this->m_qListDips.append(p_ecd);
    return *this;
}
