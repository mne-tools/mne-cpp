//=============================================================================================================
/**
* @file		IRTRecord.h
* @author	Christoph Dinh <christoph.dinh@live.de>;
* @version	1.0
* @date		October, 2010
*
* @section    LICENSE
*
* Copyright (C) 2010 Christoph Dinh. All rights reserved.
*
* No part of this program may be photocopied, reproduced,
* or translated to another program language without the
* prior written consent of the author.
*
*
* @brief	Contains declaration of IRTReord interface class.
*
*/

#ifndef IRTRecord_H
#define IRTRecord_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "IModule.h"
#include <rtMeas/Nomenclature/nomenclature.h>
#include <rtMeas/IOBuffer/circularbuffer.h>

#include <rtMeas/Measurement/IMeasurementacceptor.h>


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <QMap>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEX
//=============================================================================================================

namespace MNEX
{


//=============================================================================================================
/**
* DECLARE CLASS IRTRecord
*
* @brief The IRTRecord class provides an interface for a real-time record module.
*/
class IRTRecord : public IModule, public IMeasurementAcceptor
{
//ToDo virtual methods of IMeasurementAcceptor
public:

    //=========================================================================================================
    /**
    * Destroys the IRTRecord.
    */
    virtual ~IRTRecord() {};

    //=========================================================================================================
    /**
    * Starts the IRTRecord.
    * Pure virtual method inherited by IModule.
    *
    * @return true if success, false otherwise
    */
    virtual bool start() = 0;

    //=========================================================================================================
    /**
    * Stops the IRTRecord.
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
    * @return type of the IRTRecord
    */
    virtual Type getType() const = 0;

    //=========================================================================================================
    /**
    * Returns the module name.
    * Pure virtual method inherited by IModule.
    *
    * @return the name of the IRTRecord.
    */
    virtual const char* getName() const = 0;

    //=========================================================================================================
    /**
    * Returns the set up widget for configuration of IRTRecord.
    * Pure virtual method inherited by IModule.
    *
    * @return the setup widget.
    */
    virtual QWidget* setupWidget() const = 0; //setup();

    //=========================================================================================================
    /**
    * Returns the widget which is shown under configuration tab while running mode.
    * Pure virtual method inherited by IModule.
    *
    * @return the run widget.
    */
    virtual QWidget* runWidget() const = 0;

    //=========================================================================================================
    /**
    * Is called when new data are available.
    * Pure virtual method inherited by IObserver.
    *
    * @param [in] pSubject pointer to Subject, should be up-cast-able to Measurement and even further.
    */
    virtual void update(Subject* pSubject) = 0;

    //=========================================================================================================
    /**
    * Sets the name of the RTRecord directory.
    *
    * @param [in] dirName name of the RTRecord directory
    */
    inline void setRTRecordDirName(const QString& dirName);

protected:

    //=========================================================================================================
    /**
    * The starting point for the thread. After calling start(), the newly created thread calls this function.
    * Returning from this method will end the execution of the thread.
    * Pure virtual method inherited by QThread
    */
    virtual void run() = 0;

    QString                         m_RTRecordDirName;		/**< Holds the real-time record sub directory name. */
    typedef QMap<S16, QFile*>       t_FileMap;				/**< Defines a new file mapping type. */
    t_FileMap                       m_mapFiles;				/**< Holds the file map. */
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline void IRTRecord::setRTRecordDirName(const QString& dirName)
{
    m_RTRecordDirName = dirName;
}

} // NAMESPACE

Q_DECLARE_INTERFACE(MNEX::IRTRecord, "cs_art/1.0")

#endif // IRTRecord_H
