//=============================================================================================================
/**
 * @file     forwardsolution.h
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 *           Juan G Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     June, 2022
 *
 * @section  LICENSE
 *
 * Copyright (C) 2022, Gabriel B Motta, Juan G Prieto. All rights reserved.
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
 * @brief    ForwardSolution class declaration.
 *
 */

#ifndef MNEANALYZE_FORWARDSOLUTION_H
#define MNEANALYZE_FORWARDSOLUTION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "forwardsolution_global.h"

#include <anShared/Plugins/abstractplugin.h>

#include <fiff/fiff_coord_trans.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QtCore/QtPlugin>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace ANSHAREDLIB {
    class Communicator;
    class AbstractModel;
}

namespace DISPLIB {
    class FwdSettingsView;
}

namespace FSLIB{
    class AnnotationSet;
}

namespace FWDLIB {
    class ComputeFwdSettings;
    class ComputeFwd;
}

namespace MNELIB {
    class MNEForwardSolution;
}

//=============================================================================================================
// DEFINE NAMESPACE FORWARDSOLUTIONPLUGIN
//=============================================================================================================

namespace FORWARDSOLUTIONPLUGIN
{

//=============================================================================================================
// FORWARDSOLUTIONPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * ForwardSolution Plugin
 *
 * @brief The forwardsolution class provides a plugin for computing averages.
 */
class FORWARDSOLUTIONSHARED_EXPORT ForwardSolution : public ANSHAREDLIB::AbstractPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "ansharedlib/1.0" FILE "forwardsolution.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(ANSHAREDLIB::AbstractPlugin)

public:
    //=========================================================================================================
    /**
     * Constructs an ForwardSolution object.
     */
    ForwardSolution();

    //=========================================================================================================
    /**
     * Destroys the ForwardSolution object.
     */
    ~ForwardSolution() override;

    // AbstractPlugin functions
    virtual QSharedPointer<AbstractPlugin> clone() const override;
    virtual void init() override;
    virtual void unload() override;
    virtual QString getName() const override;

    virtual QMenu* getMenu() override;
    virtual QDockWidget* getControl() override;
    virtual QWidget* getView() override;
    virtual QString getBuildInfo() override;

    virtual void handleEvent(QSharedPointer<ANSHAREDLIB::Event> e) override;
    virtual QVector<ANSHAREDLIB::EVENT_TYPE> getEventSubscriptions() const override;

private slots:
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
     * Call this function whenever the atlas directory is set.
     *
     * @param[in] sDirPath              The path to the atlas directory.
     * @param[in] pAnnotationSet        The Annotation set.
     */
    void onAtlasDirChanged(const QString& sDirPath,
                           const QSharedPointer<FSLIB::AnnotationSet> pAnnotationSet);

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

private:

    //=========================================================================================================
    /**
     * Loads new Fiff model whan current loaded model is changed
     *
     * @param[in, out] pNewModel    pointer to currently loaded FiffRawView Model.
     */
    void onModelChanged(QSharedPointer<ANSHAREDLIB::AbstractModel> pNewModel);


    QPointer<ANSHAREDLIB::Communicator>                     m_pCommu;                   /**< To broadcst signals. */

    DISPLIB::FwdSettingsView*                               m_pFwdSettingsView;

    QSharedPointer<FWDLIB::ComputeFwdSettings>              m_pFwdSettings;         /**< Forward Solution Settings. */

    QSharedPointer<MNELIB::MNEForwardSolution>              m_pFwdSolution;

    QSharedPointer<FIFFLIB::FiffInfo>                       m_pFiffInfo;                /**< Fiff measurement info.*/
    FIFFLIB::FiffCoordTrans                                 m_transDevHead;             /**< Updated meg->head transformation. */
    QSharedPointer<FSLIB::AnnotationSet>                    m_pAnnotationSet;       /**< Annotation set. */

    QMutex                                                  m_mutex;                    /**< The threads mutex.*/

    float                                                   m_fThreshRot;               /**< The allowed rotation in degree.**/
    float                                                   m_fThreshMove;              /**< The Allowed movement in mm.**/
    bool                                                    m_bBusy;                    /**< Indicates if we have to update headposition.**/
    bool                                                    m_bDoRecomputation;         /**< If recomputation is activated.**/
    bool                                                    m_bDoClustering;            /**< If clustering is activated.**/
    bool                                                    m_bDoFwdComputation;        /**< Do a forward computation. **/

    QString                                                 m_sAtlasDir;                /**< File to Atlas. */

    QString                                                 m_sFiffInfoSource;
};

} // NAMESPACE

#endif // MNEANALYZE_FORWARDSOLUTION_H
