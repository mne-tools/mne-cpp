//=============================================================================================================
/**
* @file     epidetect.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the epidetect class.
*
*/

#ifndef EPIDETECT_H
#define EPIDETECT_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "epidetect_global.h"

#include <scShared/Interfaces/IAlgorithm.h>
#include <generics/circularmatrixbuffer.h>
#include <scMeas/newrealtimemultisamplearray.h>
#include "FormFiles/epidetectsetupwidget.h"
#include "FormFiles/epidetectwidget.h"
#include "calcMetric.h"
#include "fuzzyMembership.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QtCore/QtPlugin>
#include <QDebug>
#include <QList>
#include <QStringList>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE EpidetectPlugin
//=============================================================================================================

namespace EPIDETECTPLUGIN
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
* DECLARE CLASS Epidetect
*
* @brief The Epidetect class provides a Epidetect algorithm structure.
*/
class EPIDETECTSHARED_EXPORT Epidetect : public IAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "epidetect.json") //NEw Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::IAlgorithm)

public:
    //=========================================================================================================
    /**
    * Constructs a Epidetect.
    */
    Epidetect();

    //=========================================================================================================
    /**
    * Destroys the Epidetect.
    */
    ~Epidetect();

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

    void showWidget();

private:
    //=========================================================================================================
    /**
    * Removes all unused channels and creates a list of stim channel indices.
    *
    * @param[in] mat unprocessed data-set
    * @param[out] QPair containing the data-set without all unwanted channels and a QList containing the channel indices.
    */
    QPair<Eigen::MatrixXd, QList<int>> prepareData(Eigen::MatrixXd mat);

    //=========================================================================================================
    /**
    * Updates all parameters if changed by the user via the GUI.
    *
    */
    void updateValues();

    bool                                                               m_bIsRunning;        /**< Flag whether thread is running.*/

    FIFFLIB::FiffInfo::SPtr                                            m_pFiffInfo;         /**< Fiff measurement info.*/
    QSharedPointer<EpidetectWidget>                                    m_pWidget;           /**< flag whether thread is running.*/
    QAction*                                                           m_pActionShowWidget; /**< flag whether thread is running.*/

    IOBUFFER::CircularMatrixBuffer<double>::SPtr                       m_pEpidetectBuffer;  /**< Holds incoming data.*/

    PluginInputData<SCMEASLIB::NewRealTimeMultiSampleArray>::SPtr      m_pEpidetectInput;   /**< The NewRealTimeMultiSampleArray of the Epidetect input.*/
    PluginOutputData<SCMEASLIB::NewRealTimeMultiSampleArray>::SPtr     m_pEpidetectOutput;  /**< The NewRealTimeMultiSampleArray of the Epidetect output.*/

    Eigen::VectorXd                                                    m_dvecEpiHistory;    /**< Contains seizure-detection history.*/
    Eigen::VectorXd                                                    m_dvecMuP2P;         /**< Contains membership-values for peak-to-peak magnitude.*/
    Eigen::VectorXd                                                    m_dvecMuKurtosis;    /**< Contains membership-values for Kurtosis.*/
    Eigen::VectorXd                                                    m_dvecMuFuzzyEn;     /**< Contains membership-values for Fuzzy Entropy.*/
    Eigen::VectorXd                                                    m_dvecMuMin;         /**< Contains smallest membership value for each channel.*/
    Eigen::VectorXd                                                    m_dvecMuMax;         /**< Contains largest membership value for each channel.*/
    Eigen::VectorXd                                                    m_dvecMuMean;        /**< Contains mean membership value for each channel.*/
    Eigen::VectorXd                                                    m_dvecMuGesVec;      /**< Contains all membership values for each channel.*/


    int                                                                 m_iDim;             /**< FuzzyEn emmbeding dimension as set through the GUI.*/
    int                                                                 m_iN;               /**< Step of the FuzzyEn exponential function as set through the GUI.*/
    int                                                                 m_iListLength;      /**< Length of the history matrices as set through the GUI.*/
    int                                                                 m_iFuzzyEnStep;     /**< Number of channels to skip after each FuzzyEn calculation as set through the GUI.*/
    int                                                                 m_iChWeight;        /**< Weight of inividual channels as set through the GUI.*/

    double                                                              m_dMargin;          /**< Margin for the membership function as set through the GUI.*/
    double                                                              m_dThreshold1;      /**< Smallest requiered value for first stage detection.*/
    double                                                              m_dThreshold2;      /**< Smallest requiered value for second stage detection.*/
    double                                                              m_dMuGes;           /**< Overall membership.*/
    double                                                              m_dR;               /**< Width of FuzzyEn exponential function as set through the GUI.*/

signals:
    //=========================================================================================================
    /**
    * Emitted when fiffInfo is available
    */
    void fiffInfoAvailable();
};

} // NAMESPACE

#endif // EPIDETECT_H
