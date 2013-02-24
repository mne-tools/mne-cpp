//=============================================================================================================
/**
* @file		IRTAlgorithm.h
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
* @brief	Contains declaration of IRTAlgorithm interface class.
*
*/

#ifndef IRTALGORITHM_H
#define IRTALGORITHM_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <rtMeas/Nomenclature/nomenclature.h>

#include <rtMeas/Measurement/IMeasurementprovider.h>
#include <rtMeas/Measurement/IMeasurementacceptor.h>



//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEX
//=============================================================================================================

namespace MNEX
{


//=============================================================================================================
/**
* DECLARE CLASS IRTAlgorithm
*
* @brief The IRTAlgorithm class provides an interface for a real-time algorithm module.
*/
class IRTAlgorithm : public IMeasurementProvider, public IMeasurementAcceptor
{
//ToDo virtual methods of IMeasurementAcceptor && IMeasurementProvider
public:

    //=========================================================================================================
    /**
    * Destroys the IRTAlgorithm.
    */
    virtual ~IRTAlgorithm() {};


    //=========================================================================================================
    /**
    * Starts the IRTAlgorithm.
    * Pure virtual method inherited by IModule.
    *
    * @return true if success, false otherwise
    */
    virtual bool start() = 0;

    //=========================================================================================================
    /**
    * Stops the IRTAlgorithm.
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
    * @return type of the IRTAlgorithm
    */
    virtual Type getType() const = 0;

    //=========================================================================================================
    /**
    * Returns the module name.
    * Pure virtual method inherited by IModule.
    *
    * @return the name of the IRTAlgorithm.
    */
    virtual const char* getName() const = 0;

    //=========================================================================================================
    /**
    * Returns the set up widget for configuration of IRTAlgorithm.
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

Q_DECLARE_INTERFACE(MNEX::IRTAlgorithm, "csa_rt/1.0")

#endif // IALGORITHM_H
