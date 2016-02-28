//=============================================================================================================
/**
* @file     noisereduction.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the NoiseReduction class.
*
*/

#ifndef NOISEREDUCTION_H
#define NOISEREDUCTION_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "noisereduction_global.h"

#include <utils/filterTools/sphara.h>
#include <utils/ioutils.h>

#include <mne_x/Interfaces/IAlgorithm.h>
#include <generics/circularmatrixbuffer.h>
#include <xMeas/newrealtimemultisamplearray.h>
#include "FormFiles/noisereductionsetupwidget.h"
#include "FormFiles/noisereductionoptionswidget.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QtCore/QtPlugin>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/SparseCore>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE NoiseReductionPlugin
//=============================================================================================================

namespace NoiseReductionPlugin
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* DECLARE CLASS NoiseReduction
*
* @brief The NoiseReduction class provides a noisereduction algorithm structure.
*/
class NOISEREDUCTIONSHARED_EXPORT NoiseReduction : public MNEX::IAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "mne_x/1.0" FILE "noisereduction.json") //NEw Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(MNEX::IAlgorithm)

    friend class NoiseReductionOptionsWidget;

public:
    //=========================================================================================================
    /**
    * Constructs a NoiseReduction.
    */
    NoiseReduction();

    //=========================================================================================================
    /**
    * Destroys the NoiseReduction.
    */
    ~NoiseReduction();

    //=========================================================================================================
    /**
    * IAlgorithm functions
    */
    virtual QSharedPointer<MNEX::IPlugin> clone() const;
    virtual void init();
    virtual void unload();
    virtual bool start();
    virtual bool stop();
    virtual MNEX::IPlugin::PluginType getType() const;
    virtual QString getName() const;
    virtual QWidget* setupWidget();

    //=========================================================================================================
    /**
    * Udates the pugin with new (incoming) data.
    *
    * @param[in] pMeasurement    The incoming data in form of a generalized NewMeasurement.
    */
    void update(XMEASLIB::NewMeasurement::SPtr pMeasurement);

public slots:
    //=========================================================================================================
    /**
    * Set the acquisition system type (BabyMEG, VecotrView, EEG).
    *
    * @param[in] sSystem    The type of the acquisition system.
    */
    void setAcquisitionSystem(const QString &sSystem);

    //=========================================================================================================
    /**
    * Set the active flag for SPHARA processing.
    *
    * @param[in] state    The new activity flag.
    */
    void setSpharaMode(bool state);

    //=========================================================================================================
    /**
    * Set the number of base functions to keep for SPHARA processing.
    *
    * @param[in] nBaseFctsGrad    The number of grad/mag base functions to keep.
    * @param[in] nBaseFctsMag     The number of grad/mag base functions to keep.
    */
    void setSpharaNBaseFcts(int nBaseFctsGrad, int nBaseFctsMag);

protected slots:
    //=========================================================================================================
    /**
    * Update the SSP projection
    */
    void updateProjection();

protected:
    //=========================================================================================================
    /**
    * IAlgorithm function
    */
    virtual void run();

    //=========================================================================================================
    /**
    * Toggle visibilty the visibility of the options toolbar widget.
    */
    void showOptionsWidget();

    //=========================================================================================================
    /**
    * Init the SPHARA method.
    */
    void initSphara();

    //=========================================================================================================
    /**
    * Create/Update the SPHARA projection operator.
    */
    void createSpharaOperator();

private:
    QMutex                          m_mutex;                                    /**< The threads mutex.*/

    bool                            m_bCompActivated;                           /**< Compensator activated */
    bool                            m_bIsRunning;                               /**< Flag whether thread is running.*/
    bool                            m_bSpharaActive;                            /**< Flag whether thread is running.*/
    bool                            m_bProjActivated;                           /**< Projections activated */

    int                             m_iNBaseFctsFirst;                          /**< The number of grad/inner base functions to use for calculating the sphara opreator.*/
    int                             m_iNBaseFctsSecond;                         /**< The number of grad/outer base functions to use for calculating the sphara opreator.*/
    QString                         m_sCurrentSystem;                           /**< The current acquisition system (EEG, babyMEG, VectorView).*/

    Eigen::VectorXi                 m_vecIndicesFirstVV;                        /**< The indices of the channels to pick for the first SPHARA oerpator in case of a VectorView system.*/
    Eigen::VectorXi                 m_vecIndicesSecondVV;                       /**< The indices of the channels to pick for the second SPHARA oerpator in case of a VectorView system.*/
    Eigen::VectorXi                 m_vecIndicesFirstBabyMEG;                   /**< The indices of the channels to pick for the first SPHARA oerpator in case of a BabyMEG system.*/
    Eigen::VectorXi                 m_vecIndicesSecondBabyMEG;                  /**< The indices of the channels to pick for the second SPHARA oerpator in case of a BabyMEG system.*/

    Eigen::SparseMatrix<double>     m_matSparseSpharaMult;                      /**< The final sparse SPHARA operator .*/
    Eigen::SparseMatrix<double>     m_matSparseSpharaProjMult;                  /**< The final sparse SPHARA + projection operator.*/
    Eigen::SparseMatrix<double>     m_matSparseSpharaCompMult;                  /**< The final sparse SPHARA + compensator operator.*/
    Eigen::SparseMatrix<double>     m_matSparseProjCompMult;                    /**< The final sparse projection + compensator operator.*/
    Eigen::SparseMatrix<double>     m_matSparseProj;                            /**< Sparse SSP projector */
    Eigen::SparseMatrix<double>     m_matSparseComp;                            /**< Sparse compensator matrix */
    Eigen::SparseMatrix<double>     m_matSparseFull;                            /**< Full multiplication matrix  */

    Eigen::MatrixXd                 m_matSpharaVVGradLoaded;                    /**< The loaded VectorView gradiometer basis functions.*/
    Eigen::MatrixXd                 m_matSpharaVVMagLoaded;                     /**< The loaded VectorView magnetometer basis functions.*/
    Eigen::MatrixXd                 m_matSpharaBabyMEGInnerLoaded;              /**< The loaded babyMEG inner layer basis functions.*/
    Eigen::MatrixXd                 m_matSpharaBabyMEGOuterLoaded;              /**< The loaded babyMEG outer layer basis functions.*/

    FIFFLIB::FiffInfo::SPtr                         m_pFiffInfo;                /**< Fiff measurement info.*/

    IOBuffer::CircularMatrixBuffer<double>::SPtr    m_pNoiseReductionBuffer;    /**< Holds incoming data.*/

    QSharedPointer<NoiseReductionOptionsWidget>     m_pOptionsWidget;           /**< flag whether thread is running.*/
    QAction*                                        m_pActionShowOptionsWidget; /**< flag whether thread is running.*/

    MNEX::PluginInputData<XMEASLIB::NewRealTimeMultiSampleArray>::SPtr      m_pNoiseReductionInput;      /**< The NewRealTimeMultiSampleArray of the NoiseReduction input.*/
    MNEX::PluginOutputData<XMEASLIB::NewRealTimeMultiSampleArray>::SPtr     m_pNoiseReductionOutput;     /**< The NewRealTimeMultiSampleArray of the NoiseReduction output.*/

signals:
    //=========================================================================================================
    /**
    * Emitted when fiffInfo is available
    */
    void fiffInfoAvailable();
};

} // NAMESPACE

#endif // NOISEREDUCTION_H
