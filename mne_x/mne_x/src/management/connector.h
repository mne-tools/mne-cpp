//=============================================================================================================
/**
* @file	   	connector.h
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
* @brief	Contains the declaration of the Connector class.
*
*/

#ifndef CONNECTOR_H
#define CONNECTOR_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../comp/rtmeas/Nomenclature/nomenclature.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QTime;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE CSART
//=============================================================================================================

namespace CSART
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class DisplayManager;


//=============================================================================================================
/**
* DECLARE CLASS Connector
*
* @brief The Connector class is providing static functions which care about the module runtime connection.
*/
class Connector
{
//    Q_OBJECT
//
//    friend class MainCSART;

public:

    //=========================================================================================================
    /**
    * Constructs a Connector.
    */
    Connector();

    //=========================================================================================================
    /**
    * Destroys the Connector.
    */
    virtual ~Connector();

    //=========================================================================================================
    /**
    * Initialize a Connector.
    */
    static void init();

    //=========================================================================================================
    /**
    * Connects all measurements to measurement acceptors, depending on corresponding id's.
    */
    static void connectMeasurements();

    //=========================================================================================================
    /**
    * Disconnects all measurements to measurement acceptors, depending on corresponding id's.
    */
    static void disconnectMeasurements();

    //=========================================================================================================
    /**
    * Connects all measurement widgets which are provided by modules of the module id list.
    *
    * @param [in] idList list of module id's of which the provided measurements should be connected for displayed.
    * @param [in] t time needed to initialise real time sample array widgets.
    */
    static void connectMeasurementWidgets(QList<MDL_ID::Module_ID>& idList, QTime* t);

    //=========================================================================================================
    /**
    * Disconnects all measurement widgets which are provided by modules of the module id list.
    *
    * @param [in] idList list of module id's of which the provided measurements should be disconnected from display.
    */
    static void disconnectMeasurementWidgets(QList<MDL_ID::Module_ID>& idList);

};

} // NAMESPACE

#endif // CONNECTOR_H
