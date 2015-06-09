//=============================================================================================================
/**
* @file     mne_bem.cpp
* @author   Jana Kiesel <jana.kiesel@tu-ilmenau.de>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     June, 2015
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    MNEBem class implementation.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_bem.h"

#include <utils/mnemath.h>
#include <fs/label.h>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace FSLIB;
using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEBem::MNEBem()
{
}

//*************************************************************************************************************

MNEBem::MNEBem(const MNEBem &p_MNEBem)
//: m_qListBemSurface(p_MNEBem.m_qListBemSurface)
{

}


//*************************************************************************************************************

MNEBem::MNEBem(QIODevice &p_IODevice)   //const MNESourceSpace &p_MNESourceSpace
//    :m_qListSurface(p_MNEBem.m_qListSurface)
{
    FiffStream::SPtr t_pStream(new FiffStream(&p_IODevice));
    FiffDirTree t_Tree;
    QList<FiffDirEntry>fiffDirEntries;

    if(!t_pStream->open(t_Tree, fiffDirEntries))
    {
        qCritical() << "Could not open FIFF stream!";
//        return false;
    }


    if(!MNEBem::readFromStream(t_pStream, t_Tree))
    {
        t_pStream->device()->close();
        std::cout << "Could not read the source spaces\n"; // ToDo throw error
        //ToDo error(me,'Could not read the source spaces (%s)',mne_omit_first_line(lasterr));
//        return false;
    }

}


//*************************************************************************************************************

MNEBem::~MNEBem()
{

}

//*************************************************************************************************************

bool MNEBem::readFromStream(FiffStream::SPtr& p_pStream, FiffDirTree& p_Tree)
{
return true;
}
