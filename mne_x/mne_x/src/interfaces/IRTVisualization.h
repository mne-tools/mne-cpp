//=============================================================================================================
/**
* @file		IRTVisualization.h
* @author	Christoph Dinh <christoph.dinh@live.de>;
* @version	1.0
* @date		October, 2010
*
* @section	LICENSE
*
* Copyright (C) 2010 Christoph Dinh. All rights reserved.
*
* No part of this program may be photocopied, reproduced,
* or translated to another program language without the
* prior written consent of the author.
*
*
* @brief	Contains declaration of IRTVisualization interface class.
*
*/

#ifndef IRTVISUALIZATION_H
#define IRTVISUALIZATION_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../comp/rtmeas/Measurement/IMeasurementprovider.h"
#include "../../../comp/rtmeas/Measurement/IMeasurementacceptor.h"

#include "../../../comp/rtmeas/Nomenclature/nomenclature.h"


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE VSens
//=============================================================================================================

namespace CSART
{


//=============================================================================================================
/**
* DECLARE CLASS IRTVisualization
*
* @brief The IRTVisualization class provides an interface for a real-time algorithm module.
*/
class IRTVisualization :  public IMeasurementProvider, public IMeasurementAcceptor
{
//ToDo virtual methods of IMeasurementAcceptor && IMeasurementProvider
public:

    //=========================================================================================================
    /**
    * Destroys the IRTVisualization.
    */
    virtual ~IRTVisualization() {};


    //=========================================================================================================
    /**
    * Starts the IRTVisualization.
    * Pure virtual method inherited by IModule.
    *
    * @return true if success, false otherwise
    */
    virtual bool start() = 0;

    //=========================================================================================================
    /**
    * Stops the IRTVisualization.
    * Pure virtual method inherited by IModule.
    *
    * @return true if success, false otherwise
    */
    virtual bool stop() = 0;

    //=========================================================================================================
    /**
    * Returns the module type.
    * Pure virtual method inherited by IModule.
    *
    * @return type of the IRTVisualization
    */
    virtual Type getType() const = 0;

    //=========================================================================================================
    /**
    * Returns the module name.
    * Pure virtual method inherited by IModule.
    *
    * @return the name of the IRTVisualization.
    */
    virtual const char* getName() const = 0;

    //=========================================================================================================
    /**
    * Returns the set up widget for configuration of IRTVisualization.
    * Pure virtual method inherited by IModule.
    *
    * @return the setup widget.
    */
    virtual QWidget* setupWidget() = 0; //setup();

    //=========================================================================================================
    /**
    * Returns the widget which is shown under configuration tab while running mode.
    * Pure virtual method inherited by IModule.
    *
    * @return the run widget.
    */
    virtual QWidget* runWidget() = 0;

    //=========================================================================================================
    /**
    * Is called when new data are available.
    * Pure virtual method inherited by IObserver.
    *
    * @param [in] pSubject pointer to Subject, should be up-cast-able to Measurement and even further.
    */
    virtual void update(Subject* pSubject) = 0;

protected:

    //=========================================================================================================
    /**
    * The starting point for the thread. After calling start(), the newly created thread calls this function.
    * Returning from this method will end the execution of the thread.
    * Pure virtual method inherited by QThread
    */
    virtual void run() = 0;
};

} // NAMESPACE

Q_DECLARE_INTERFACE(CSART::IRTVisualization, "csa_rt/1.0")

#endif // IRTVISUALIZATION_H
