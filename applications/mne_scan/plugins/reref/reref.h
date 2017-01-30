//=============================================================================================================
/**
* @file     reref.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the reref class.
*
*/

#ifndef REREF_H
#define REREF_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "reref_global.h"

#include <utils/ioutils.h>
#include <utils/eegref.h>
#include <iostream>

#include <scShared/Interfaces/IAlgorithm.h>
#include <generics/circularmatrixbuffer.h>
#include <scMeas/newrealtimemultisamplearray.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/SparseCore>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE ReRefPlugin
//=============================================================================================================

namespace REREFPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCSHAREDLIB;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* DECLARE CLASS ReRef
*
* @brief The ReRef class provides a reref algorithm structure.
*/
class REREFSHARED_EXPORT ReRef : public IAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "reref.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::IAlgorithm)

public:
    //=========================================================================================================
    /**
    * Constructs a reref.
    */
    ReRef();

    //=========================================================================================================
    /**
    * Destroys the reref.
    */
    ~ReRef();

    //=========================================================================================================
    /**
    * IAlgorithm functions
    */
    virtual QSharedPointer<IPlugin> clone() const;
    virtual void init();
    virtual void unload();
    virtual bool start();
    virtual bool stop();
    virtual IPlugin::PluginType getType() const;
    virtual QString getName() const;
    virtual QWidget* setupWidget();

    //=========================================================================================================
    /**
    * Udates the pugin with new (incoming) data.
    *
    * @param[in] pMeasurement    The incoming data in form of a generalized NewMeasurement.
    */
    void update(SCMEASLIB::NewMeasurement::SPtr pMeasurement);


protected:
    //=========================================================================================================
    /**
    * IAlgorithm function
    */
    virtual void run();

private:
    QMutex                          m_mutex;                    /**< The threads mutex.*/

    bool                            m_bIsRunning;               /**< Flag whether thread is running.*/
    bool                            m_bDisp;                    /**< Flag for displaying. */
    FIFFLIB::FiffInfo::SPtr         m_pFiffInfo;                /**< Fiff measurement info.*/

    IOBUFFER::CircularMatrixBuffer<double>::SPtr    m_pReRefBuffer;    /**< Holds incoming data.*/
    SCMEASLIB::NewRealTimeMultiSampleArray::SPtr    m_pRTMSA;        /**< the real time multi sample array object. */

    PluginInputData<SCMEASLIB::NewRealTimeMultiSampleArray>::SPtr      m_pReRefInput;      /**< The NewRealTimeMultiSampleArray of the reref input.*/
    PluginOutputData<SCMEASLIB::NewRealTimeMultiSampleArray>::SPtr     m_pReRefOutput;     /**< The NewRealTimeMultiSampleArray of the reref output.*/

};

} // NAMESPACE

#endif // REREF_H
