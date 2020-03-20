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
#include <rtprocessing/rthpis.h>

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

namespace FIFFLIB {
    class FiffInfo;
    class FiffDigPoint;
}

namespace RTPROCESSINGLIB {
    class RtHpi;
}

namespace SCMEASLIB{
    class RealTimeMultiSampleArray;
    class RealTimeHpiResult;
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
     * Update the projectors for SSP and Comps.
     */
    void updateProjections();

    //=========================================================================================================
    /**
     * Call this function whenever the allowed error changed.
     *
     * @param[in] dAllowedMeanErrorDist    Allowed mean error in mm.
     */
    void onAllowedMeanErrorDistChanged(double dAllowedMeanErrorDist);

    //=========================================================================================================
    /**
     * Call this funciton whenever new digitzers were loaded.
     *
     * @param[in] lDigitzers    The new digitzers.
     */
    void onDigitizersChanged(const QList<FIFFLIB::FiffDigPoint>& lDigitzers);

    //=========================================================================================================
    /**
     * Call this funciton whenever when a single HPI fit based on the last data block was requested.
     */
    void onDoSingleHpiFit();

    //=========================================================================================================
    /**
     * Call this funciton whenever the coil frequencies changed.
     *
     * @param[in] vCoilFreqs    The new coil frequencies.
     */
    void onCoilFrequenciesChanged(const QVector<int>& vCoilFreqs);

    //=========================================================================================================
    /**
     * Call this function whenever SSP checkbox changed.
     *
     * @param[in] bChecked    Whether the SSP check box is checked.
     */
    void onSspStatusChanged(bool bChecked);

    //=========================================================================================================
    /**
     * Call this function whenever compensator checkbox changed.
     *
     * @param[in] bChecked    Whether the compensator check box is checked.
     */
    void onCompStatusChanged(bool bChecked);

    //=========================================================================================================
    /**
     * Call this function whenever continous HPI checkbox changed.
     *
     * @param[in] bChecked    Whether the continous HPI check box is checked.
     */
    void onContHpiStatusChanged(bool bChecked);

    //=========================================================================================================
    /**
     * IAlgorithm function
     */
    virtual void run();    

    QMutex                      m_mutex;                    /**< The threads mutex.*/

    QVector<int>                m_vCoilFreqs;               /**< Vector contains the HPI coil frequencies. */

    qint16                      m_iNumberBadChannels;       /**< The number of bad channels.*/

    double                      m_dAllowedMeanErrorDist;    /**< The allowed error distance in order for the last fit to be counted as a good fit.*/

    bool                        m_bDoSingleHpi;             /**< Do a single HPI fit.*/
    bool                        m_bDoContinousHpi;          /**< Do continous HPI fitting.*/
    bool                        m_bUseSSP;                  /**< Use SSP's.*/
    bool                        m_bUseComp;                 /**< Use Comps's.*/

    Eigen::MatrixXd             m_matData;                  /**< The last data block.*/
    Eigen::MatrixXd             m_matProjectors;            /**< Holds the matrix with the SSP and compensator projectors.*/
    Eigen::MatrixXd             m_matCompProjectors;        /**< Holds the matrix with the SSP and compensator projectors.*/

    QSharedPointer<FIFFLIB::FiffInfo>                                           m_pFiffInfo;            /**< Fiff measurement info.*/
    QSharedPointer<IOBUFFER::CircularBuffer_Matrix_double>                      m_pCircularBuffer;      /**< Holds incoming raw data. */

    SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeMultiSampleArray>::SPtr     m_pHpiInput;            /**< The RealTimeMultiSampleArray of the Hpi input.*/
    SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeHpiResult>::SPtr           m_pHpiOutput;           /**< The RealTimeHpiResult of the Hpi output.*/

signals:
    void errorsChanged(const QVector<double>& vErrors,
                       double dMeanErrorDist);
};
} // NAMESPACE

#endif // WRITETOFILE_H
