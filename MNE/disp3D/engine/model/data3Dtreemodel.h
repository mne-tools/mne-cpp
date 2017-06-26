//=============================================================================================================
/**
* @file     data3Dtreemodel.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2016
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
* @brief    Data3DTreeModel class declaration
*
*/

#ifndef DATA3DTREEMODEL_H
#define DATA3DTREEMODEL_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp3D_global.h"

#include <mne/mne_forwardsolution.h>
#include <connectivity/network/network.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QStandardItemModel>
#include <QPointer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Qt3DCore {
    class QEntity;
}

namespace FSLIB {
    class SurfaceSet;
    class AnnotationSet;
    class Annotation;
    class Surface;
}

namespace MNELIB {
    class MNESourceSpace;
    class MNEBem;
    class MNESourceEstimate;
    class MNEBemSurface;
}

namespace FIFFLIB{
    class FiffEvoked;
    class FiffDigPointSet;
}

namespace INVERSELIB{
    class ECDSet;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class MneEstimateTreeItem;
class NetworkTreeItem;
class EcdDataTreeItem;
class FsSurfaceTreeItem;
class SourceSpaceTreeItem;
class BemTreeItem;
class SensorSetTreeItem;
class DigitizerSetTreeItem;
class SubjectTreeItem;
class MeasurementTreeItem;
class SensorDataTreeItem;


//=============================================================================================================
/**
* Data3DTreeModel provides a tree based data model to hold all information about data which was added to the View 3D.
*
* @brief Data3DTreeModel provides a tree based data model to hold all information about data which was added to the View 3D.
*/
class DISP3DNEWSHARED_EXPORT Data3DTreeModel : public QStandardItemModel
{
    Q_OBJECT

public:
    typedef QSharedPointer<Data3DTreeModel> SPtr;             /**< Shared pointer type for Data3DTreeModel class. */
    typedef QSharedPointer<const Data3DTreeModel> ConstSPtr;  /**< Const shared pointer type for Data3DTreeModel class. */

    //=========================================================================================================
    /**
    * Default constructor.
    *
    * @param[in] parent         The parent of this class.
    */
    explicit Data3DTreeModel(QObject *parent = 0);

    //=========================================================================================================
    /**
    * Default destructor.
    */
    ~Data3DTreeModel();

    //=========================================================================================================
    /**
    * QStandardItemModel functions
    */
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    //=========================================================================================================
    /**
    * Adds FreeSurfer brain data SETS.
    *
    * @param[in] sSubject           The name of the subject.
    * @param[in] sMriSetName        The name of the MRI set to which the data is to be added. If it does not exist yet, it will be created.
    * @param[in] surfaceSet         FreeSurfer surface set.
    * @param[in] annotationSet      FreeSurfer annotation set.
    *
    * @return                       Returns a QList with the added surface tree items. The ordering
    *                               of the list hereby corresponds to the ordering of the input surface set.
    *                               The list is empty if no item was added.
    */
    QList<FsSurfaceTreeItem*> addSurfaceSet(const QString& sSubject,
                                            const QString& sMriSetName,
                                            const FSLIB::SurfaceSet& surfaceSet,
                                            const FSLIB::AnnotationSet& annotationSet = FSLIB::AnnotationSet());

    //=========================================================================================================
    /**
    * Adds FreeSurfer brain data.
    *
    * @param[in] sSubject           The name of the subject.
    * @param[in] sMriSetName        The name of the MRI set to which the data is to be added. If it does not exist yet, it will be created.
    * @param[in] surface            FreeSurfer surface.
    * @param[in] annotation         FreeSurfer annotation.
    *
    * @return                       Returns a pointer to the added tree item. Default is a NULL pointer if no item was added.
    */
    FsSurfaceTreeItem* addSurface(const QString& sSubject,
                                  const QString& sSet,
                                  const FSLIB::Surface& surface,
                                  const FSLIB::Annotation& annotation = FSLIB::Annotation());

    //=========================================================================================================
    /**
    * Adds source space brain data.
    *
    * @param[in] sSubject               The name of the subject.
    * @param[in] sMeasurementSetName    The name of the measurement set to which the data is to be added. If it does not exist yet, it will be created.
    * @param[in] sourceSpace            The source space information.
    *
    * @return                           Returns a pointer to the added tree item. Default is a NULL pointer if no item was added.
    */
    SourceSpaceTreeItem* addSourceSpace(const QString& sSubject,
                                        const QString& sMeasurementSetName,
                                        const MNELIB::MNESourceSpace& sourceSpace);

    //=========================================================================================================
    /**
    * Adds a forward solution data to the brain tree model. Convenient function to addBrainData(const QString& text, const MNESourceSpace& tSourceSpace).
    *
    * @param[in] sSubject               The name of the subject.
    * @param[in] sMeasurementSetName    The name of the measurement set to which the data is to be added. If it does not exist yet, it will be created.
    * @param[in] forwardSolution        The forward solution information.
    *
    * @return                           Returns a pointer to the added tree item. Default is a NULL pointer if no item was added.
    */
    SourceSpaceTreeItem* addForwardSolution(const QString& sSubject,
                                            const QString& sMeasurementSetName,
                                            const MNELIB::MNEForwardSolution& forwardSolution);

    //=========================================================================================================
    /**
    * Adds source estimated activation data (MNE or RTC-MUSIC).
    *
    * @param[in] sSubject               The name of the subject.
    * @param[in] sMeasurementSetName    The name of the measurement set to which the data is to be added. If it does not exist yet, it will be created.
    * @param[in] sourceEstimate         The MNESourceEstimate.
    * @param[in] forwardSolution        The MNEForwardSolution.
    *
    * @return                           Returns a pointer to the added tree item. Default is a NULL pointer if no item was added.
    */
    MneEstimateTreeItem* addSourceData(const QString& sSubject,
                                       const QString& sMeasurementSetName,
                                       const MNELIB::MNESourceEstimate& sourceEstimate,
                                       const MNELIB::MNEForwardSolution& forwardSolution = MNELIB::MNEForwardSolution());

    //=========================================================================================================
    /**
    * Adds source estimated activation data (dipole fit).
    *
    * @param[in] sSubject               The name of the subject.
    * @param[in] sMeasurementSetName    The name of the measurement set to which the data is to be added. If it does not exist yet, it will be created.
    * @param[in] ecdSet                 The ECDSet dipole data.
    *
    * @return                           Returns a pointer to the added tree item. Default is a NULL pointer if no item was added.
    */
    EcdDataTreeItem* addDipoleFitData(const QString& sSubject,
                                      const QString& sSet,
                                      const INVERSELIB::ECDSet& ecdSet);

    //=========================================================================================================
    /**
    * Adds connectivity estimation data.
    *
    * @param[in] sSubject               The name of the subject.
    * @param[in] sMeasurementSetName    The name of the measurement set to which the data is to be added. If it does not exist yet, it will be created.
    * @param[in] networkData            The connectivity data.
    *
    * @return                           Returns a pointer to the added tree item. Default is a NULL pointer if no item was added.
    */
    NetworkTreeItem* addConnectivityData(const QString& sSubject,
                                         const QString& sMeasurementSetName,
                                         const CONNECTIVITYLIB::Network& networkData);

    //=========================================================================================================
    /**
    * Adds BEM data.
    *
    * @param[in] sSubject           The name of the subject.
    * @param[in] sBemSetName        The name of the BEM set to which the data is to be added. If it does not exist yet, it will be created.
    * @param[in] bem                The Bem information.
    *
    * @return                       Returns a pointer to the added tree item. Default is a NULL pointer if no item was added.
    */
    BemTreeItem* addBemData(const QString& sSubject,
                            const QString& sBemSetName,
                            const MNELIB::MNEBem& bem);


    //=========================================================================================================
    /**
    * Adds sensor data.
    *
    * @param[in] sSubject           The name of the subject.
    * @param[in] sSensorSetName     The name of the sensor set to which the data is to be added. If it does not exist yet, it will be created.
    * @param[in] sensor             The sensor surface information in form of a BEM model. Sensor surfaces are internally represented as MNEBem models.
    * @param[in] lChInfo            The channel information used to plot the MEG channels.
    *
    * @return                       Returns a pointer to the added tree item. Default is a NULL pointer if no item was added.
    */
    SensorSetTreeItem* addMegSensorInfo(const QString& sSubject,
                                        const QString& sSensorSetName,
                                        const MNELIB::MNEBem& sensor,
                                        const QList<FIFFLIB::FiffChInfo>& lChInfo = QList<FIFFLIB::FiffChInfo>());

    //=========================================================================================================
    /**
    * Adds digitizer data.
    *
    * @param[in] sSubject               The name of the subject.
    * @param[in] sMeasurementSetName    The name of the measurement set to which the data is to be added. If it does not exist yet, it will be created.
    * @param[in] digitizer              The digitizer information.
    *
    * @return                           Returns a pointer to the added tree item. Default is a NULL pointer if no item was added.
    */
    DigitizerSetTreeItem* addDigitizerData(const QString& sSubject,
                                           const QString& sMeasurementSetName,
                                           const FIFFLIB::FiffDigPointSet &digitizer);

    //=========================================================================================================
    /**
    * Adds live sensor data.
    *
    * @param[in] sSubject               The name of the subject.
    * @param[in] sMeasurementSetName    The name of the measurement set to which the data is to be added. If it does not exist yet, it will be created.
    * @param[in] matSensorData          The Sensor Data.
    * @param[in] tBemSurface            The Bem Surface data.
    * @param[in] fiffInfo             The FiffInfo that holds all information about the sensors.
    * @param[in] sDataType              The data type ("MEG" or "EEG").
    * @param[in] dCancelDist            Distances higher than this are ignored for the interpolation
    * @param[in] sInterpolationFunction Function that computes interpolation coefficients using the distance values
    *
    * @return                           Returns a pointer to the added tree item. Default is a NULL pointer if no item was added.
    */
    SensorDataTreeItem* addSensorData(const QString& sSubject,
                                      const QString& sMeasurementSetName,
                                      const Eigen::MatrixXd& matSensorData,
                                      const MNELIB::MNEBemSurface& tBemSurface,
                                      const FIFFLIB::FiffInfo &fiffInfo,
                                      const QString &sDataType,
                                      const double &dCancelDist,
                                      const QString &sInterpolationFunction);

    //=========================================================================================================
    /**
    * Returns the 3D model root entity.
    *
    * @return   The model's root entity to acess the scenegraph.
    */
    QPointer<Qt3DCore::QEntity> getRootEntity();

protected:
    //=========================================================================================================
    /**
    * Init the meta types
    */
    void initMetatypes();

    //=========================================================================================================
    /**
    * Create a subject tree item if the item was not found. This is a convenience function.
    *
    * @param[in] sSubject           The name of the subject.
    *
    * @return                       Returns a pointer to the first found or created subject tree item. Default is a NULL pointer if no item was found.
    */
    SubjectTreeItem* addSubject(const QString& sSubject);

    //=========================================================================================================
    /**
    * Adds an item with its toolTip as second coulm item as description to the model.
    *
    * @param[in] pItemParent         The parent item.
    * @param[in] pItemAdd            The item which is added as a row to the parent item.
    */
    void addItemWithDescription(QStandardItem* pItemParent, QStandardItem* pItemAdd);

    QStandardItem*                   m_pRootItem;            /**< The root item of the tree model. */
    QPointer<Qt3DCore::QEntity>      m_pModelEntity;         /**< The parent 3D entity for this model. */
};

} // NAMESPACE

#endif // DATA3DTREEMODEL_H
