//=============================================================================================================
/**
 * @file     randomdata.h
 * @author   Juan GarciaPrieto <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel B Motta <gbmotta@mgh.harvard.edu>;
 * @since    0.1.0
 * @date     February, 2023
 *
 * @section  LICENSE
 *
 * Copyright (C) 2023, Juan G Prieto, Gabriel B Motta. All rights reserved.
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
 * @brief    Contains the declaration of the Random data plugin class.
 *
 */

#ifndef RANDOMDATA_H
#define RANDOMDATA_H //============================================================================================================= INCLUDES =============================================================================================================

#include "randomdata/randomdata_global.h"
// #include "randomdata/randomdata_plugin_gui.h"

#include <scShared/Plugins/abstractsensor.h>
// #include <utils/generics/circularbuffer.h>
#include <memory>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATION
//=============================================================================================================

namespace SCMEASLIB {
class RealTimeMultiSampleArray;
}

namespace FIFFLIB {
class FiffInfo;
}


//=============================================================================================================
// DEFINE NAMESPACE RANDOMDATAPLUGIN
//=============================================================================================================

namespace RANDOMDATAPLUGIN {

//=============================================================================================================
// RANDOMDATAPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * The RandomData class provides a EEG connector for generating random data. 
 *
 * @brief The RandomData class.
 */
class RANDOMDATASHARED_EXPORT RandomData : public SCSHAREDLIB::AbstractSensor {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "randomdata.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::AbstractSensor)

 public:
    //=========================================================================================================
    /**
     * Constructs RandomData.
     */
    RandomData();

    //=========================================================================================================
    /**
     * Destroys RandomData.
     */
    ~RandomData();

    //=========================================================================================================
    /**
     * Virtual Interface
     * */
    virtual QSharedPointer<SCSHAREDLIB::AbstractPlugin> clone() const;
    virtual void init();
    virtual void unload();
    virtual bool start();
    virtual bool stop();
    virtual AbstractPlugin::PluginType getType() const;
    virtual QString getName() const;
    virtual QWidget* setupWidget();
    virtual QString getBuildInfo();

 protected:
    //=========================================================================================================
    /**
     * the starting point for the thread. after calling start(), the newly created thread calls this function.
     * returning from this method will end the execution of the thread.
     * pure virtual method inherited by qthread.
     */
  virtual void run();

  // std::unique_ptr<RandomDataPluginGUI> guiWidget;

  QSharedPointer<SCSHAREDLIB::PluginOutputData<
    SCMEASLIB::RealTimeMultiSampleArray>> m_pRTMSA_RandomData;
  QSharedPointer<FIFFLIB::FiffInfo> m_pFiffInfo;
  Eigen::MatrixXd matData;

};  // namespace RANDOMDATAPLUGIN

}

#endif  // RANDOMDATA_H
