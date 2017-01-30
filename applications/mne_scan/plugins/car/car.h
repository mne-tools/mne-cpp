//=============================================================================================================
/**
* @file     car.h
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
* @brief    Contains the declaration of the car class.
*
*/

#ifndef CAR_H
#define CAR_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "car_global.h"


//#include <utils/filterTools/sphara.h>
#include <utils/ioutils.h>
#include <utils/eegref.h>
#include <iostream>

//#include "disp/filterwindow.h"

#include <scShared/Interfaces/IAlgorithm.h>

//#include <rtProcessing/rtfilter.h>

#include <generics/circularmatrixbuffer.h>

#include <scMeas/newrealtimemultisamplearray.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
//#include <QtCore/QtPlugin>
//#include <QDebug>
//#include <QSettings>
//#include <QElapsedTimer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/SparseCore>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE carPlugin
//=============================================================================================================

namespace CARPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCSHAREDLIB;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* DECLARE CLASS car
*
* @brief The Car class provides a car algorithm structure.
*/
class CARSHARED_EXPORT Car : public IAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "car.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::IAlgorithm)

public:
    //=========================================================================================================
    /**
    * Constructs a car.
    */
    Car();

    //=========================================================================================================
    /**
    * Destroys the car.
    */
    ~Car();

    //=========================================================================================================
    /**
    * IAlgorithm functions
    */
    virtual QSharedPointer<IPlugin> clone() const;
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
    * @param[in] pMeasurement    The incoming data in form of a generalized NewMeasurement.
    */
    void update(SCMEASLIB::NewMeasurement::SPtr pMeasurement);

public slots:
//    //=========================================================================================================
//    /**
//    * Set the acquisition system type (BabyMEG, VecotrView, EEG).
//    *
//    * @param[in] sSystem    The type of the acquisition system.
//    */
//    void setAcquisitionSystem(const QString &sSystem);

//    //=========================================================================================================
//    /**
//    * Set the active flag for SPHARA processing.
//    *
//    * @param[in] state    The new activity flag.
//    */
//    void setSpharaMode(bool state);

//    //=========================================================================================================
//    /**
//    * Set the number of base functions to keep for SPHARA processing.
//    *
//    * @param[in] nBaseFctsGrad    The number of grad/mag base functions to keep.
//    * @param[in] nBaseFctsMag     The number of grad/mag base functions to keep.
//    */
//    void setSpharaNBaseFcts(int nBaseFctsGrad, int nBaseFctsMag);

protected slots:
//    //=========================================================================================================
//    /**
//    * Update the SSP projection
//    */
//    void updateProjection();

//    //=========================================================================================================
//    /**
//    * Update the compensator
//    *
//    * @param[in] to    Compensator to use in fiff constant format FiffCtfComp.kind (NOT FiffCtfComp.ctfkind)
//    */
//    void updateCompensator(int to);

//    //=========================================================================================================
//    /**
//    * Sets the type of channel which are to be filtered
//    *
//    * @param[in] sType    the channel type which is to be filtered (EEG, MEG, All)
//    */
//    void setFilterChannelType(QString sType);

//    //=========================================================================================================
//    /**
//    * Filter parameters changed
//    *
//    * @param[in] filterData    list of the currently active filter
//    */
//    void filterChanged(QList<FilterData> filterData);

//    //=========================================================================================================
//    /**
//    * Filter avtivated
//    *
//    * @param[in] state    filter on/off flag
//    */
//    void filterActivated(bool state);

protected:
//    //=========================================================================================================
//    /**
//    * Toggle visibilty the visibility of the options toolbar widget.
//    */
//    void showOptionsWidget();

//    //=========================================================================================================
//    /**
//    * Init the SPHARA method.
//    */
//    void initSphara();

//    //=========================================================================================================
//    /**
//    * Init the temporal filtering.
//    */
//    void initFilter();

//    //=========================================================================================================
//    /**
//    * Shows the filter widget
//    */
//    void showFilterWidget(bool state = true);

//    //=========================================================================================================
//    /**
//    * Create/Update the SPHARA projection operator.
//    */
//    void createSpharaOperator();

    //=========================================================================================================
    /**
    * IAlgorithm function
    */
    virtual void run();

private:
    QMutex                          m_mutex;                                    /**< The threads mutex.*/

//    bool                            m_bCompActivated;                           /**< Compensator activated */
    bool                            m_bIsRunning;                               /**< Flag whether thread is running.*/
//    bool                            m_bSpharaActive;                            /**< Flag whether thread is running.*/
//    bool                            m_bProjActivated;                           /**< Projections activated */
//    bool                            m_bFilterActivated;                         /**< Projections activated */

//    int                             m_iNBaseFctsFirst;                          /**< The number of grad/inner base functions to use for calculating the sphara opreator.*/
//    int                             m_iNBaseFctsSecond;                         /**< The number of grad/outer base functions to use for calculating the sphara opreator.*/
//    int                             m_iMaxFilterLength;                         /**< Max order of the current filters */
//    int                             m_iMaxFilterTapSize;                        /**< maximum number of allowed filter taps. This number depends on the size of the receiving blocks. */

//    QString                         m_sCurrentSystem;                           /**< The current acquisition system (EEG, babyMEG, VectorView).*/
//    QString                         m_sFilterChannelType;                       /**< Kind of channel which is to be filtered */

//    QPushButton*                    m_pShowFilterOptions;                       /**< Holds the show filter options button. */
//    QList<FilterData>               m_filterData;                               /**< List of currently active filters. */

//    Eigen::VectorXi                 m_vecIndicesFirstVV;                        /**< The indices of the channels to pick for the first SPHARA oerpator in case of a VectorView system.*/
//    Eigen::VectorXi                 m_vecIndicesSecondVV;                       /**< The indices of the channels to pick for the second SPHARA oerpator in case of a VectorView system.*/
//    Eigen::VectorXi                 m_vecIndicesFirstBabyMEG;                   /**< The indices of the channels to pick for the first SPHARA oerpator in case of a BabyMEG system.*/
//    Eigen::VectorXi                 m_vecIndicesSecondBabyMEG;                  /**< The indices of the channels to pick for the second SPHARA oerpator in case of a BabyMEG system.*/
//    Eigen::VectorXi                 m_vecIndicesFirstEEG;                       /**< The indices of the channels to pick for the second SPHARA operator in case of an EEG system.*/

//    Eigen::SparseMatrix<double>     m_matSparseSpharaMult;                      /**< The final sparse SPHARA operator .*/
//    Eigen::SparseMatrix<double>     m_matSparseProjCompMult;                    /**< The final sparse projection + compensator operator.*/
//    Eigen::SparseMatrix<double>     m_matSparseProjMult;                        /**< The final sparse SSP projector */
//    Eigen::SparseMatrix<double>     m_matSparseCompMult;                        /**< The final sparse compensator matrix */
//    Eigen::SparseMatrix<double>     m_matSparseFull;                            /**< The final sparse full multiplication matrix  */

//    Eigen::MatrixXd                 m_matSpharaVVGradLoaded;                    /**< The loaded VectorView gradiometer basis functions.*/
//    Eigen::MatrixXd                 m_matSpharaVVMagLoaded;                     /**< The loaded VectorView magnetometer basis functions.*/
//    Eigen::MatrixXd                 m_matSpharaBabyMEGInnerLoaded;              /**< The loaded babyMEG inner layer basis functions.*/
//    Eigen::MatrixXd                 m_matSpharaBabyMEGOuterLoaded;              /**< The loaded babyMEG outer layer basis functions.*/
//    Eigen::MatrixXd                 m_matSpharaEEGLoaded;                       /**< The loaded EEG basis functions.*/

//    QVector<int>                    m_lFilterChannelList;                       /**< The indices of the channels to be filtered.*/

    FIFFLIB::FiffInfo::SPtr                         m_pFiffInfo;                /**< Fiff measurement info.*/

    bool            m_bDisp;            /** Flag for displaying. */

    IOBUFFER::CircularMatrixBuffer<double>::SPtr    m_pCarBuffer;    /**< Holds incoming data.*/

//    carOptionsWidget::SPtr               m_pOptionsWidget;           /**< The noise reduction option widget object.*/
//    QAction*                                        m_pActionShowOptionsWidget; /**< The noise reduction option widget action.*/

//    DISPLIB::FilterWindow::SPtr                     m_pFilterWindow;            /**< Filter window. */
//    RTPROCESSINGLIB::RtFilter::SPtr                       m_pRtFilter;                /**< Real time filter object. */

    SCMEASLIB::NewRealTimeMultiSampleArray::SPtr     m_pRTMSA;                   /**< the real time multi sample array object. */

    PluginInputData<SCMEASLIB::NewRealTimeMultiSampleArray>::SPtr      m_pCarInput;      /**< The NewRealTimeMultiSampleArray of the car input.*/
    PluginOutputData<SCMEASLIB::NewRealTimeMultiSampleArray>::SPtr     m_pCarOutput;     /**< The NewRealTimeMultiSampleArray of the car output.*/

signals:
//    //=========================================================================================================
//    /**
//    * Emitted when fiffInfo is available
//    */
//    void fiffInfoAvailable();

};

} // NAMESPACE

#endif // CAR_H
