//=============================================================================================================
/**
* @file     deepcntk.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017 Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the DeepCNTK class.
*
*/

#ifndef DEEPCNTK_H
#define DEEPCNTK_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "deepcntk_global.h"

#include <anShared/Interfaces/IExtension.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QtCore/QtPlugin>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace DISPLIB
{
    class DeepViewerWidget;
    class Controls;
}

namespace DEEPLIB
{
    class Deep;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DEEPCNTKEXTENSION
//=============================================================================================================

namespace DEEPCNTKEXTENSION
{

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* DeepCNTK Extension
*
* @brief The DeepCNTK class provides a Machine Learning Capbilities.
*/
class DEEPCNTKSHARED_EXPORT DeepCNTK : public ANSHAREDLIB::IExtension
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "ansharedlib/1.0" FILE "deepcntk.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(ANSHAREDLIB::IExtension)

public:
    //=========================================================================================================
    /**
    * Constructs a DeepCNTK.
    */
    DeepCNTK();

    //=========================================================================================================
    /**
    * Destroys the DeepCNTK.
    */
    ~DeepCNTK();

    //=========================================================================================================
    /**
    * IExtension functions
    */
    virtual QSharedPointer<IExtension> clone() const;
    virtual void init();
    virtual void unload();
    virtual QString getName() const;

    virtual bool hasMenu() const;
    virtual QMenu* getMenu();

    virtual bool hasControl() const;
    virtual QDockWidget* getControl();

    virtual bool hasView() const;
    virtual QWidget* getView();

private:
    //=========================================================================================================
    /**
    * Setup the model
    */
    void setupModel();

    //=========================================================================================================
    /**
    * Start the model training
    */
    void trainModel();

private:
    // Deep Model
    QSharedPointer<DEEPLIB::Deep>   m_pDeep;            /**< CNTK Wrapper */

    DISPLIB::Controls*              m_pControlPanel;    /**< View Control Panel */

    // Control
    QDockWidget*                    m_pControl;         /**< Control Widget */

    // View
    DISPLIB::DeepViewerWidget*      m_pDeepViewer;      /**< Viewer */
};

} // NAMESPACE

#endif // DEEPCNTK_H
