//=============================================================================================================
/**
* @file     rthpi.h
* @author   Chiran Doshi <chiran.doshi@childrens.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     June, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Chiran Doshi and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the RTHPI class.
*
*/

#ifndef RTHPI_H
#define RTHPI_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rthpi_global.h"

#include <mne_x/Interfaces/IAlgorithm.h>
#include <generics/circularmatrixbuffer.h>
#include <xMeas/newrealtimemultisamplearray.h>


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff_info.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE RtHpiPlugin
//=============================================================================================================

namespace RtHpiPlugin
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEX;
using namespace XMEASLIB;
using namespace IOBuffer;


//*************************************************************************************************************
//=============================================================================================================
// Declare all structures to be used
//=============================================================================================================

struct coilParam {
    Eigen::MatrixXd pos;
    Eigen::MatrixXd mom;
};

struct dipError {
    double error;
    Eigen::MatrixXd moment;
};

struct sens {
    Eigen::MatrixXd coilpos;
    Eigen::MatrixXd coilori;
    Eigen::MatrixXd tra;
};



//=============================================================================================================
/**
* DECLARE CLASS RTHPI
*
* @brief The RtHpi class provides a RtHpi algorithm structure.
*/
//class DUMMYTOOLBOXSHARED_EXPORT DummyToolbox : public IAlgorithm
class RTHPISHARED_EXPORT RtHpi : public IAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "mne_x/1.0" FILE "rthpi.json") //NEW Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(MNEX::IAlgorithm)

public:
    //=========================================================================================================
    /**
    * Constructs a RtHpi.
    */
    RtHpi();

    //=========================================================================================================
    /**
    * Destroys the RtHpi.
    */
    ~RtHpi();

    //=========================================================================================================
    /**
    * Initialise input and output connectors.
    */
    virtual void init();

    //=========================================================================================================
    /**
    * Is called when plugin is detached of the stage. Can be used to safe settings.
    */
    virtual void unload();

    //=========================================================================================================
    /**
    * Clone the plugin
    */
    virtual QSharedPointer<IPlugin> clone() const;

    virtual bool start();
    virtual bool stop();

    virtual IPlugin::PluginType getType() const;
    virtual QString getName() const;

    virtual QWidget* setupWidget();

    void update(XMEASLIB::NewMeasurement::SPtr pMeasurement);

    dipError dipfitError (Eigen::MatrixXd, Eigen::MatrixXd, struct sens);
    Eigen::MatrixXd ft_compute_leadfield(Eigen::MatrixXd, struct sens);
    Eigen::MatrixXd magnetic_dipole(Eigen::MatrixXd, Eigen::MatrixXd, Eigen::MatrixXd);
    coilParam dipfit(struct coilParam, struct sens, Eigen::MatrixXd, int numCoils);
    Eigen::MatrixXd fminsearch(Eigen::MatrixXd,int, int, int, Eigen::MatrixXd, struct sens);
    static bool compar (int, int);
    Eigen::MatrixXd pinv(Eigen::MatrixXd);

signals:
    //=========================================================================================================
    /**
    * Emitted when fiffInfo is available
    */
    void fiffInfoAvailable();

protected:
    virtual void run();

private:
    //=========================================================================================================
    /**
    * Initialises the output connector.
    */
    void initConnector();

    PluginInputData<NewRealTimeMultiSampleArray>::SPtr   m_pRTMSAInput;      /**< The NewRealTimeMultiSampleArray of the RtHpi input.*/
    PluginOutputData<NewRealTimeMultiSampleArray>::SPtr  m_pRTMSAOutput;    /**< The NewRealTimeMultiSampleArray of the RtHpi output.*/


    FiffInfo::SPtr  m_pFiffInfo;                            /**< Fiff measurement info.*/

    CircularMatrixBuffer<double>::SPtr   m_pRtHpiBuffer;    /**< Holds incoming data.*/

    bool m_bIsRunning;      /**< If source lab is running */
    bool m_bProcessData;    /**< If data should be received for processing */

    static std::vector <double>base_arr;

};

} // NAMESPACE

#endif // RTHPI_H
