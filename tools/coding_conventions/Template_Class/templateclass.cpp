//=============================================================================================================
/**
* @file     templateclass.cpp
* @author   Your name <yourname@yourdomain>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     Month, Year
*
* @section  LICENSE
*
* Copyright (C) Year, Your name and Matti Hamalainen. All rights reserved.
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
* @brief    TemplateClass class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "templateclass.h"

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

//Put all internal includes from mne-cpp here. I.e. everything from the MNE liraries or your application


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//Put all Qt includes here


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

//Put all Eigen includes here


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace YOURNAMESPACE;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//Put all globally defined functions here. These functions are not part of your class. I.e. put all functions 
//here which are called by a multithreading framework (QtFramework, etc.)


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TemplateClass::TemplateClass(OtherClass *pParent);
: m_iParamterOne(1)
, m_dParamterTwo(1.0)
, m_bParamterThree(false)
, m_pOtherClassObject(pParent)
{

}


//*************************************************************************************************************

TemplateClass::TemplateClass(const TemplateClass &pTemplateClass);
: m_iParamterOne(pTemplateClass.m_iParamterOne)
, m_dParamterTwo(pTemplateClass.m_dParamterTwo)
, m_bParamterThree(pTemplateClass.m_bParamterThree)
, m_pOtherClassObject(pTemplateClass.m_pOtherClassObject)
{

}


//*************************************************************************************************************

TemplateClass::~TemplateClass()
{

}


//*************************************************************************************************************

int TemplateClass::doSomething(int parameterOne, int &parameterTwo)
{
    paramterTwo = 6;

    return parameterOne + paramterTwo;
}


