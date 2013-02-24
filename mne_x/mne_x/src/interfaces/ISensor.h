//=============================================================================================================
/**
* @file		ISensor.h
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
* @brief	Contains declaration of ISensor interface class.
*
*/

#ifndef ISENSOR_H
#define ISENSOR_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../MNE/rtMeas/Measurement/IMeasurementprovider.h"

#include "../../MNE/rtMeas/Nomenclature/nomenclature.h"


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
* @brief The ISensor class provides an interface for a sensor module.
*/
class ISensor : public IMeasurementProvider
{
//ToDo virtual methods of IMeasurementProvider
public:

    //=========================================================================================================
    /**
    * Destroys the ISensor.
    */
    virtual ~ISensor() {};

    //=========================================================================================================
    /**
    * Starts the ISensor.
    * Pure virtual method inherited by IModule.
    *
    * @return true if success, false otherwise
    */
    virtual bool start() = 0;

    //=========================================================================================================
    /**
    * Stops the ISensor.
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
    * @return type of the ISensor
    */
    virtual Type getType() const = 0;

    //=========================================================================================================
    /**
    * Returns the module name.
    * Pure virtual method inherited by IModule.
    *
    * @return the name of the ISensor.
    */
    virtual const char* getName() const = 0;

    //=========================================================================================================
    /**
    * Returns the set up widget for configuration of ISensor.
    * Pure virtual method inherited by IModule.
    *
    * @return the setup widget.
    */
    virtual QWidget* setupWidget() = 0;

    //=========================================================================================================
    /**
    * Returns the widget which is shown under configuration tab while running mode.
    * Pure virtual method inherited by IModule.
    *
    * @return the run widget.
    */
    virtual QWidget* runWidget() = 0;

protected:

    //=========================================================================================================
    /**
    * The starting point for the thread. After calling start(), the newly created thread calls this function.
    * Returning from this method will end the execution of the thread.
    * Pure virtual method inherited by QThread.
    */
    virtual void run() = 0;

};

} //NAMESPACE

Q_DECLARE_INTERFACE(MNEX::ISensor, "csa_rt/1.0")

#endif // ISENSOR_H
