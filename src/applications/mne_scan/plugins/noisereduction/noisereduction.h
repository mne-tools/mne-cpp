//=============================================================================================================
/**
 * @file     noisereduction.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Christoph Dinh, Lorenz Esch. All rights reserved.
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "noisereduction_global.h"

#include <utils/generics/circularbuffer.h>

#include <fiff/fiff_proj.h>

#include <rtprocessing/helpers/filterkernel.h>

#include <scShared/Plugins/abstractalgorithm.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/SparseCore>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB{
    class FiffInfo;
}

namespace RTPROCESSINGLIB{
    class Filter;
}

namespace SCMEASLIB{
    class RealTimeMultiSampleArray;
}

//=============================================================================================================
// DEFINE NAMESPACE NOISEREDUCTIONPLUGIN
//=============================================================================================================

namespace NOISEREDUCTIONPLUGIN
{

//=============================================================================================================
// NOISEREDUCTIONPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * DECLARE CLASS NoiseReduction
 *
 * @brief The NoiseReduction class provides a tools to reduce noise of an incoming data stream. It then forwards the processed data to subsequent plugins.
 */
class NOISEREDUCTIONSHARED_EXPORT NoiseReduction : public SCSHAREDLIB::AbstractAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "noisereduction.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::AbstractAlgorithm)

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
     * AbstractAlgorithm functions
     */
    virtual QSharedPointer<SCSHAREDLIB::AbstractPlugin> clone() const;
    virtual void init();
    virtual void unload();
    virtual bool start();
    virtual bool stop();
    virtual AbstractPlugin::PluginType getType() const;
    virtual QString getName() const;
    virtual QWidget* setupWidget();
    virtual QString getBuildInfo();

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
    void initPluginControlWidgets();

    //=========================================================================================================
    /**
     * Set the active flag for SPHARA processing.
     *
     * @param[in] state    The new activity flag.
     */
    void setSpharaActive(bool state);

    //=========================================================================================================
    /**
     * Set the number of base functions and acquisition system for SPHARA processing.
     *
     * @param[in] sSytemType         The acquisition system.
     * @param[in] nBaseFctsGrad      The number of grad/mag base functions to keep.
     * @param[in] nBaseFctsMag       The number of grad/mag base functions to keep.
     */
    void setSpharaOptions(const QString& sSytemType,
                          int nBaseFctsFirst,
                          int nBaseFctsSecond);

protected:
    //=========================================================================================================
    /**
     * AbstractAlgorithm function
     */
    virtual void run();

    //=========================================================================================================
    /**
     * Update the SSP projection
     */
    void updateProjection(const QList<FIFFLIB::FiffProj>& projs);

    //=========================================================================================================
    /**
     * Update the compensator
     *
     * @param[in] to    Compensator to use in fiff constant format FiffCtfComp.kind (NOT FiffCtfComp.ctfkind).
     */
    void updateCompensator(int to);

    //=========================================================================================================
    /**
     * Sets the type of channel which are to be filtered
     *
     * @param[in] sType    the channel type which is to be filtered (EEG, MEG, All).
     */
    void setFilterChannelType(QString sType);

    //=========================================================================================================
    /**
     * Filter parameters changed
     *
     * @param[in] filterData    currently active filter.
     */
    void setFilter(const RTPROCESSINGLIB::FilterKernel& filterData);

    //=========================================================================================================
    /**
     * Filter avtivated
     *
     * @param[in] state    filter on/off flag.
     */
    void setFilterActive(bool state);

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

    bool                            m_bCompActivated;                           /**< Compensator activated. */
    bool                            m_bSpharaActive;                            /**< Flag whether thread is running.*/
    bool                            m_bProjActivated;                           /**< Projections activated. */
    bool                            m_bFilterActivated;                         /**< Projections activated. */

    int                             m_iNBaseFctsFirst;                          /**< The number of grad/inner base functions to use for calculating the sphara opreator.*/
    int                             m_iNBaseFctsSecond;                         /**< The number of grad/outer base functions to use for calculating the sphara opreator.*/
    int                             m_iMaxFilterLength;                         /**< Max order of the current filters. */
    int                             m_iMaxFilterTapSize;                        /**< maximum number of allowed filter taps. This number depends on the size of the receiving blocks. */

    QString                         m_sCurrentSystem;                           /**< The current acquisition system (EEG, babyMEG, VectorView).*/
    QString                         m_sFilterChannelType;                       /**< Kind of channel which is to be filtered. */

    RTPROCESSINGLIB::FilterKernel     m_filterKernel;                             /**< The currently active filter. */

    Eigen::VectorXi                 m_vecIndicesFirstVV;                        /**< The indices of the channels to pick for the first SPHARA oerpator in case of a VectorView system.*/
    Eigen::VectorXi                 m_vecIndicesSecondVV;                       /**< The indices of the channels to pick for the second SPHARA oerpator in case of a VectorView system.*/
    Eigen::VectorXi                 m_vecIndicesFirstBabyMEG;                   /**< The indices of the channels to pick for the first SPHARA oerpator in case of a BabyMEG system.*/
    Eigen::VectorXi                 m_vecIndicesSecondBabyMEG;                  /**< The indices of the channels to pick for the second SPHARA oerpator in case of a BabyMEG system.*/
    Eigen::VectorXi                 m_vecIndicesFirstEEG;                       /**< The indices of the channels to pick for the second SPHARA operator in case of an EEG system.*/

    Eigen::SparseMatrix<double>     m_matSparseSpharaMult;                      /**< The final sparse SPHARA operator .*/
    Eigen::SparseMatrix<double>     m_matSparseProjCompMult;                    /**< The final sparse projection + compensator operator.*/
    Eigen::SparseMatrix<double>     m_matSparseProjMult;                        /**< The final sparse SSP projector. */
    Eigen::SparseMatrix<double>     m_matSparseCompMult;                        /**< The final sparse compensator matrix. */
    Eigen::SparseMatrix<double>     m_matSparseFull;                            /**< The final sparse full multiplication matrix . */

    Eigen::MatrixXd                 m_matSpharaVVGradLoaded;                    /**< The loaded VectorView gradiometer basis functions.*/
    Eigen::MatrixXd                 m_matSpharaVVMagLoaded;                     /**< The loaded VectorView magnetometer basis functions.*/
    Eigen::MatrixXd                 m_matSpharaBabyMEGInnerLoaded;              /**< The loaded babyMEG inner layer basis functions.*/
    Eigen::MatrixXd                 m_matSpharaBabyMEGOuterLoaded;              /**< The loaded babyMEG outer layer basis functions.*/
    Eigen::MatrixXd                 m_matSpharaEEGLoaded;                       /**< The loaded EEG basis functions.*/

    Eigen::RowVectorXi              m_lFilterChannelList;                       /**< The indices of the channels to be filtered.*/

    QSharedPointer<FIFFLIB::FiffInfo>                               m_pFiffInfo;            /**< Fiff measurement info.*/

    QSharedPointer<UTILSLIB::CircularBuffer_Matrix_double>          m_pCircularBuffer;      /**< Holds incoming raw data. */

    SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeMultiSampleArray>::SPtr      m_pNoiseReductionInput;      /**< The RealTimeMultiSampleArray of the NoiseReduction input.*/
    SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeMultiSampleArray>::SPtr     m_pNoiseReductionOutput;     /**< The RealTimeMultiSampleArray of the NoiseReduction output.*/

signals:
};
} // NAMESPACE

#endif // NOISEREDUCTION_H
