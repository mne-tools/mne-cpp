//=============================================================================================================
/**
* @file     IIO.h
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
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Contains declaration of IIO interface class.
*
*/

#ifndef IIO_H
#define IIO_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "IPlugin.h"
#include <xMeas/Nomenclature/nomenclature.h>
#include <generics/circularbuffer_old.h>

#include <xMeas/Measurement/IMeasurementSink.h>


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <QMap>

#include <QSharedPointer>


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
* DECLARE CLASS IIO
*
* @brief The IIO class provides an interface for a real-time record plugin.
*/
class IIO : public IPlugin, public XMEASLIB::IMeasurementSink
{
//ToDo virtual methods of IMeasurementSink
public:
    typedef QSharedPointer<IIO> SPtr;               /**< Shared pointer type for IIO. */
    typedef QSharedPointer<const IIO> ConstSPtr;    /**< Const shared pointer type for IIO. */

    //=========================================================================================================
    /**
    * Destroys the IIO.
    */
    virtual ~IIO() {};

    //=========================================================================================================
    /**
    * Starts the IIO.
    * Pure virtual method inherited by IPlugin.
    *
    * @return true if success, false otherwise
    */
    virtual bool start() = 0;

    //=========================================================================================================
    /**
    * Stops the IIO.
    * Pure virtual method inherited by IPlugin.
    *
    * @return true if success, false otherwise
    */
    virtual bool stop() = 0;

    //=========================================================================================================
    /**
    * Returns the plugin type.
    * Pure virtual method inherited by IPlugin.
    *
    * @return type of the IIO
    */
    virtual Type getType() const = 0;

    //=========================================================================================================
    /**
    * Returns the plugin name.
    * Pure virtual method inherited by IPlugin.
    *
    * @return the name of the IIO.
    */
    virtual const char* getName() const = 0;

    //=========================================================================================================
    /**
    * Returns the set up widget for configuration of IIO.
    * Pure virtual method inherited by IPlugin.
    *
    * @return the setup widget.
    */
    virtual QWidget* setupWidget() const = 0; //setup();

    //=========================================================================================================
    /**
    * Returns the widget which is shown under configuration tab while running mode.
    * Pure virtual method inherited by IPlugin.
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

inline void IIO::setRTRecordDirName(const QString& dirName)
{
    m_RTRecordDirName = dirName;
}

} // NAMESPACE

Q_DECLARE_INTERFACE(MNEX::IIO, "mne_x/1.0")

#endif // IIO_H
