//=============================================================================================================
/**
* @file     neuronalconnectivity.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     October, 2016
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
* @brief    Contains the declaration of the NeuronalConnectivity class.
*
*/

#ifndef NEURONALCONNECTIVITY_H
#define NEURONALCONNECTIVITY_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "neuronalconnectivity_global.h"

#include <scShared/Interfaces/IAlgorithm.h>

#include <utils/generics/circularmatrixbuffer.h>

#include "FormFiles/neuronalconnectivityyourwidget.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QtCore/QtPlugin>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB {
    class FiffInfo;
}

namespace SCMEASLIB {
    class RealTimeSourceEstimate;
    class NewRealTimeMultiSampleArray;
    class RealTimeConnectivityEstimate;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE NEURONALCONNECTIVITYPLUGIN
//=============================================================================================================

namespace NEURONALCONNECTIVITYPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* DECLARE CLASS NeuronalConnectivity
*
* @brief The NeuronalConnectivity class provides a NeuronalConnectivity plugin for online processing.
*/
class NEURONALCONNECTIVITYSHARED_EXPORT NeuronalConnectivity : public SCSHAREDLIB::IAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "neuronalconnectivity.json") //NEw Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::IAlgorithm)

public:
    //=========================================================================================================
    /**
    * Constructs a NeuronalConnectivity.
    */
    NeuronalConnectivity();

    //=========================================================================================================
    /**
    * Destroys the NeuronalConnectivity.
    */
    ~NeuronalConnectivity();

    //=========================================================================================================
    /**
    * IAlgorithm functions
    */
    virtual QSharedPointer<SCSHAREDLIB::IPlugin> clone() const;
    virtual void init();
    virtual void unload();
    virtual bool start();
    virtual bool stop();
    virtual SCSHAREDLIB::IPlugin::PluginType getType() const;
    virtual QString getName() const;
    virtual QWidget* setupWidget();

    //=========================================================================================================
    /**
    * Udates the pugin with new (incoming) data.
    *
    * @param[in] pMeasurement    The incoming data in form of a generalized NewMeasurement.
    */
    void updateSource(SCMEASLIB::NewMeasurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
    * Updates the real time multi sample array data
    *
    * @param[in] pMeasurement    The incoming data in form of a generalized NewMeasurement.
    */
    void updateRTMSA(SCMEASLIB::NewMeasurement::SPtr pMeasurement);

protected:
    //=========================================================================================================
    /**
    * IAlgorithm function
    */
    virtual void run();

    void showYourWidget();

private:
    bool                                                                            m_bIsRunning;                   /**< Flag whether thread is running.*/
    qint32                                                                          m_iDownSample;                  /**< Sampling rate */

    QSharedPointer<FIFFLIB::FiffInfo>                                               m_pFiffInfo;                    /**< Fiff measurement info.*/
    QSharedPointer<NeuronalConnectivityYourWidget>                                  m_pYourWidget;                  /**< flag whether thread is running.*/
    QAction*                                                                        m_pActionShowYourWidget;        /**< flag whether thread is running.*/

    QSharedPointer<IOBUFFER::CircularMatrixBuffer<double> >                         m_pNeuronalConnectivityBuffer;  /**< Holds incoming data.*/

    SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeSourceEstimate>::SPtr           m_pRTSEInput;                   /**< The RealTimeSourceEstimate input.*/
    SCSHAREDLIB::PluginInputData<SCMEASLIB::NewRealTimeMultiSampleArray>::SPtr      m_pRTMSAInput;                  /**< The RealTimeMultiSampleArray input.*/

    SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeConnectivityEstimate>::SPtr    m_pRTCEOutput;                  /**< The RealTimeSourceEstimate output.*/

    Eigen::MatrixX3f        m_matNodeVertLeft;          /**< Holds the left hemi vertex postions of the network nodes. Corresponding to the neuronal sources.*/
    Eigen::MatrixX3f        m_matNodeVertRight;         /**< Holds the right hemi vertex postions of the network nodes. Corresponding to the neuronal sources.*/
    Eigen::MatrixX3f        m_matNodeVertComb;          /**< Holds both hemi vertex postions of the network nodes. Corresponding to the neuronal sources.*/

    QVector<int>            m_chIdx;                    /**< The channel indeces to pick from the incoming data.*/

signals:
    //=========================================================================================================
    /**
    * Emitted when fiffInfo is available
    */
    void fiffInfoAvailable();
};

} // NAMESPACE

#endif // NEURONALCONNECTIVITY_H
