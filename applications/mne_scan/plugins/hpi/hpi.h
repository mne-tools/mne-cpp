//=============================================================================================================
/**
 * @file     hpi.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     February, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the Hpi class.
 *
 */

#ifndef WRITETOFILE_H
#define WRITETOFILE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "hpi_global.h"

#include <utils/generics/circularbuffer.h>
#include <scShared/Interfaces/IAlgorithm.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB{
    class FiffInfo;
}

namespace SCMEASLIB{
    class RealTimeMultiSampleArray;
}

#define MAX_DATA_LEN    2000000000L

//=============================================================================================================
// DEFINE NAMESPACE HPIPLUGIN
//=============================================================================================================

namespace HPIPLUGIN
{

//=============================================================================================================
// HPIPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * DECLARE CLASS Hpi
 *
 * @brief The Hpi class provides a tools to reduce noise of an incoming data stream. It then forwards the processed data to subsequent plugins.
 */
class HPISHARED_EXPORT Hpi : public SCSHAREDLIB::IAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "hpi.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::IAlgorithm)

public:
    //=========================================================================================================
    /**
     * Constructs a Hpi.
     */
    Hpi();

    //=========================================================================================================
    /**
     * Destroys the Hpi.
     */
    ~Hpi();

    //=========================================================================================================
    /**
     * IAlgorithm functions
     */
    virtual QSharedPointer<SCSHAREDLIB::IPlugin> clone() const;
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
     * @param[in] pMeasurement    The incoming data in form of a generalized Measurement.
     */
    void update(SCMEASLIB::Measurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
     * Inits widgets which are used to control this plugin, then emits them in form of a QList.
     */
    virtual void initPluginControlWidgets();

private:
    //=========================================================================================================
    /**
     * IAlgorithm function
     */
    virtual void run();

    QMutex          m_mutex;                                    /**< The threads mutex.*/

    QSharedPointer<FIFFLIB::FiffInfo>                                           m_pFiffInfo;            /**< Fiff measurement info.*/

    QSharedPointer<IOBUFFER::CircularBuffer_Matrix_double>                      m_pCircularBuffer;      /**< Holds incoming raw data. */

    SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeMultiSampleArray>::SPtr     m_pHpiInput;            /**< The RealTimeMultiSampleArray of the Hpi input.*/

};
} // NAMESPACE

#endif // WRITETOFILE_H
