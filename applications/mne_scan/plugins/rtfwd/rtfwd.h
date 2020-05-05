//=============================================================================================================
/**
 * @file     rtfwd.h
 * @author   Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>
 * @since    0.1.0
 * @date     May, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Ruben Dörfel. All rights reserved.
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
 * @brief    Contains the declaration of the RtFwd class.
 *
 */

#ifndef RTFWD_H
#define RTFWD_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtfwd_global.h"

#include "FormFiles/rtfwdsetupwidget.h"
//#include "FormFiles/rtfwdwidget.h"

#include <fwd/computeFwd/compute_fwd.h>
#include <fwd/computeFwd/compute_fwd_settings.h>

#include <scShared/Interfaces/IAlgorithm.h>
#include <scMeas/realtimemultisamplearray.h>
#include <scMeas/realtimehpiresult.h>

#include <utils/generics/circularbuffer.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QtCore/QtPlugin>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB {
    class FiffInfo;
    class FiffCoordTrans;
}

namespace FWDLIB {
    class ComputeFwdSettings;
    class ComputeFwd;
}

namespace SCMEASLIB{
    class RealTimeMultiSampleArray;
    class RealTimeHpiResult;
}

//=============================================================================================================
// DEFINE NAMESPACE RTFWDPLUGIN
//=============================================================================================================

namespace RTFWDPLUGIN
{

//=============================================================================================================
// RTFWDPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * DECLARE CLASS RtFwd
 *
 * @brief The RtFwd class provides a dummy algorithm structure.
 */
class RTFWDSHARED_EXPORT RtFwd : public SCSHAREDLIB::IAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "rtfwd.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::IAlgorithm)

public:
    //=========================================================================================================
    /**
     * Constructs a RtFwd.
     */
    RtFwd();

    //=========================================================================================================
    /**
     * Destroys the RtFwd.
     */
    ~RtFwd();

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
     * Udates the plugin with new (incoming) data.
     *
     * @param[in] pMeasurement    The incoming data in form of a generalized Measurement.
     */
    void update(SCMEASLIB::Measurement::SPtr pMeasurement);

protected:
    //=========================================================================================================
    /**
     * Inits widgets which are used to control this plugin, then emits them in form of a QList.
     */
    virtual void initPluginControlWidgets();

    //=========================================================================================================
    /**
     * IAlgorithm function
     */
    virtual void run();

private:
    //=========================================================================================================
    /**
     * Check if incoming headmovement should trigger a recalculation
     */
    void checkHeadDisplacement();

    //=========================================================================================================
    /**
     * Call this function whenever the allowed head rotation changed.
     *
     * @param[in] dThreshRot    Allowed rotation in degree.
     */
    void onThresholdRotationChanged(double dThreshRot);

    //=========================================================================================================
    /**
     * Call this function whenever the allowed head movement changed.
     *
     * @param[in] dThreshMove   Allowed movement in mm.
     */
    void onThresholdMovementChanged(double dThreshMove);

    QMutex                              m_mutex;                /**< The threads mutex.*/

    float                               m_fThreshRot;           /**< The allowed rotation in degree.**/
    float                               m_fThreshMove;          /**< The Allowed movement in mm.**/
    bool                                m_bUpdateHeadPos;       /**< Indicates if we have to update headposition.**/

    QSharedPointer<HpiFitResult>        m_pHpiFitResult;        /**< The Hpi fitting result.**/

    FIFFLIB::FiffInfo::SPtr             m_pFiffInfo;            /**< Fiff measurement info.*/
    FIFFLIB::FiffCoordTrans             m_transDevHead;         /**< Updated meg->head transformation. */

    SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeHpiResult>::SPtr            m_pHpiInput;    /**< The incoming Hpi Data.*/
    SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeMultiSampleArray>::SPtr    m_pOutput;      /**< The outgoing data.*/

    IOBUFFER::CircularBuffer_Matrix_double::SPtr    m_pCircularBuffer;      /**< Holds incoming data.*/

public:
    FWDLIB::ComputeFwdSettings::SPtr    m_pFwdSettings;         /**< Forward Solution Settings. */

    MNELIB::MNEForwardSolution::SPtr    m_pFwdSolution;         /**< Forward Solution. */

signals:
    //=========================================================================================================
    /**
     * Emitted when forward solution is available
     */
    void fwdSolutionAvailable(const MNELIB::MNEForwardSolution& mneFwdSol);
};
} // NAMESPACE

#endif // RTFWD_H
