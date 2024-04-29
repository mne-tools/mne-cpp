//=============================================================================================================
/**
 * @file     rtfwd.h
 * @author   Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>
 * @since    0.1.1
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

#include <scShared/Plugins/abstractalgorithm.h>

#include <utils/generics/circularbuffer.h>

#include <fiff/fiff_coord_trans.h>

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

namespace MNELIB{
    class MNEForwardSolution;
}

namespace FSLIB{
    class AnnotationSet;
}

namespace INVERSELIB{
    struct HpiFitResult;
}

namespace SCMEASLIB{
    class RealTimeFwdSolution;
    class RealTimeHpiResult;
    class RealTimeMultiSampleArray;
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
 * @brief The RtFwd class provides a plugin for calculating and updating the forward solution.
 */
class RTFWDSHARED_EXPORT RtFwd : public SCSHAREDLIB::AbstractAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "rtfwd.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::AbstractAlgorithm)

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
     * AbstractAlgorithm functions
     */
    virtual QSharedPointer<SCSHAREDLIB::AbstractPlugin> clone() const;
    virtual void init();
    virtual void unload();
    virtual bool start();
    virtual bool stop();
    virtual SCSHAREDLIB::AbstractPlugin::PluginType getType() const;
    virtual QString getName() const;
    virtual QWidget* setupWidget();
    virtual QString getBuildInfo();

    //=========================================================================================================
    /**
     * Udates the plugin with new (incoming) data.
     *
     * @param[in] pMeasurement    The incoming data in form of a generalized Measurement.
     */
    void update(SCMEASLIB::Measurement::SPtr pMeasurement);

    QSharedPointer<FWDLIB::ComputeFwdSettings>                  m_pFwdSettings;         /**< Forward Solution Settings. */

protected:
    //=========================================================================================================
    /**
     * Inits widgets which are used to control this plugin, then emits them in form of a QList.
     */
    void initPluginControlWidgets();

    //=========================================================================================================
    /**
     * AbstractAlgorithm function
     */
    virtual void run();

private:
    //=========================================================================================================
    /**
     * Call this funciton whenever a forward computation was requested.
     */
    void onDoForwardComputation();

    //=========================================================================================================
    /**
     * Call this function whenever the recompution status changed.
     *
     * @param[in] bDoRecomputation    If recomputation is activated.
     */
    void onRecompStatusChanged(bool bDoRecomputation);

    //=========================================================================================================
    /**
     * Call this function whenever the clustering status changed.
     *
     * @param[in] bDoClustering   If clustering is activated.
     */
    void onClusteringStatusChanged(bool bDoRecomputation);

    //=========================================================================================================
    /**
     * Call this function whenever the number of dipoles per cluster changed.
     *
     * @param[in] iNClusterNumber   Number of dipoles per cluster.
     */
    void onClusterNumberChanged(int iNClusterNumber);

    //=========================================================================================================
    /**
     * Call this function whenever the atlas directory is set.
     *
     * @param[in] sDirPath              The path to the atlas directory.
     * @param[in] pAnnotationSet        The Annotation set.
     */
    void onAtlasDirChanged(const QString& sDirPath,
                           const QSharedPointer<FSLIB::AnnotationSet> pAnnotationSet);

    QMutex                                      m_mutex;                    /**< The threads mutex.*/
    QFuture<void>                               m_future;                   /**< The future monitoring the clustering and forward calculation. */

    float                                       m_fThreshRot;               /**< The allowed rotation in degree.**/
    float                                       m_fThreshMove;              /**< The Allowed movement in mm.**/
    bool                                        m_bBusy;                    /**< Indicates if we have to update headposition.**/
    bool                                        m_bDoRecomputation;         /**< If recomputation is activated.**/
    bool                                        m_bDoClustering;            /**< If clustering is activated.**/
    bool                                        m_bNClusterChanged;         /**< Perform new clustering when cluster size changed**/

    bool                                        m_bDoFwdComputation;        /**< Do a forward computation. **/

    QString                                     m_sAtlasDir;                /**< File to Atlas. */

    QSharedPointer<INVERSELIB::HpiFitResult>    m_pHpiFitResult;            /**< The Hpi fitting result.**/

    QSharedPointer<FIFFLIB::FiffInfo>           m_pFiffInfo;                /**< Fiff measurement info.*/
    FIFFLIB::FiffCoordTrans                     m_transDevHead;             /**< Updated meg->head transformation. */

    QSharedPointer<FSLIB::AnnotationSet>                                        m_pAnnotationSet;       /**< Annotation set. */

    SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeHpiResult>::SPtr            m_pHpiInput;            /**< The incoming Hpi data.*/
    SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeMultiSampleArray>::SPtr     m_pRTMSAInput;          /**< The incoming data.*/

    SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeFwdSolution>::SPtr         m_pRTFSOutput;          /**< The fwd solution.*/

signals:
    //=========================================================================================================
    /**
     * Emitted when forward solution is available
     */
    void fwdSolutionAvailable(FIFFLIB::fiff_int_t iSourceOri,
                              FIFFLIB::fiff_int_t iCoordFrame,
                              int iNSource,
                              int iNChan,
                              int iNSpaces);

    //=========================================================================================================
    /**
     * Emitted whenever clustered forward solution is available
     */
    void clusteringAvailable(int iNSource);

    //=========================================================================================================
    /**
     * Emit this signal whenever the clustering status changed
     * (0 Initializing, 1 Computing, 2 Recomputing, 3 Clustering, 4 Not Computed, 5 Finished).
     *
     * @param[in] iStatus            status of recomputation.
     */
    void statusInformationChanged(int iStatus);

};
} // NAMESPACE

#endif // RTFWD_H
