//=============================================================================================================
/**
* @file     templateclass.h
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
* @brief     TemplateClass class declaration.
*
*/

#ifndef TEMPLATECLASS
#define TEMPLATECLASS

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
// FORWARD DECLARATIONS
//=============================================================================================================

//Put all used forward declarations of the diferent namespaces here. 
//Try not to include too many header with #include -> Keep build time to a minimum
//Rather use forward declarations and the actual includes in the cpp file.
//Example:
//namespace Foo {
//	class FooDoSomething;
//}

//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE YOURNAMESPACE
//=============================================================================================================

namespace YOURNAMESPACE
{

//PLEAS DO NOT USE ""using namespace"" IN HEADER FILES

//*************************************************************************************************************
//=============================================================================================================
// YOURNAMESPACE FORWARD DECLARATIONS
//=============================================================================================================

//Put all used forward declarations of the YOURNAMESPACE namespace here


//=============================================================================================================
/**
* Description of what this class is intended to do (in detail).
*
* @brief Brief description of this class.
*/
class YOUREXPORT_FLAG TemplateClass //The YOUREXPORT_FLAG flag only needs to be set when this class is part of a library
{
    Q_OBJECT //Only set if you want to use qt's signal/slot feature in this class

public:
    typedef QSharedPointer<TemplateClass> SPtr;            /**< Shared pointer type for TemplateClass. */
    typedef QSharedPointer<const TemplateClass> ConstSPtr; /**< Const shared pointer type for TemplateClass. */

    //=========================================================================================================
    /**
    * Constructs a TemplateClass object.
    */
    TemplateClass(OtherClass *pParent);

    //=========================================================================================================
    /**
    * Copy constructor.
    *
    * @param[in] p_templateClass   TemplateClass which should be copied
    */
    TemplateClass(const TemplateClass &pTemplateClass);

    //=========================================================================================================
    /**
    * Destroys the TemplateClass object.
    */
    ~TemplateClass();

    //=========================================================================================================
    /**
    * The function doSomething takes parameterOne and parameterTwo as inputs, writes something to parameterTwo and returns a result as an integer value.
    *
    * @param[in]  parameterOne  Description goes here.
    * @param[out] paramterTwo   Description goes here.
    *
    * @return calculated result as an integer
    */
    int doSomething(int parameterOne, int &parameterTwo);

    //=========================================================================================================
    /**
    * The function doSomethingInline takes parameterOne as input and returns a result as an integer value. This function is defined as an inline function (see below)
    *
    * @param[in]  parameterOne  Description goes here.
    *
    * @return calculated result as an integer
    */
    inline int doSomething(int parameterOne);

    //Put your other public member functions here

    int     m_iParamterOne;         /**< m_iParamterOne description goes here. */
    double  m_dParamterTwo;         /**< m_dParamterTwo description goes here. */
    bool    m_bParamterThree;       /**< m_bParamterThree description goes here. */

    OtherClass*    m_pOtherClassObject;       /**< m_pOtherClassObject description goes here. */

    //Put your other public members here. Try NOT to define member variables as public. It is always more safe to declare them under protected or private.
	
protected:
    //Put your protected member functions here

    //Put your protected member variables here
	
private:
    //Put your private member functions here

    //Put your private member variables here
	
signals: //If you want to use signal/slot system you need to specify the Q_Object flag above
    //Put your signals which you want to emit from this class here
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline int TemplateClass::doSomething(int parameterOne);
{
    return parameterOne+1;
}

} // NAMESPACE YOURNAMESPACE

#endif // TEMPLATECLASS
