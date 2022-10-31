//=============================================================================================================
/**
 * @file     data3Dtreemodel.h
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Juan Garcia-Prieto <juangpc@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     May, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lars Debor, Juan Garcia-Prieto, Lorenz Esch. All rights reserved.
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

#ifndef DISP3DLIB_DATA3DTREEMODEL_H
#define DISP3DLIB_DATA3DTREEMODEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp3D_global.h"

#include "items/common/types.h"

#include <mne/mne_forwardsolution.h>
#include <mne/mne_bem.h>

#include <connectivity/network/network.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QStandardItemModel>
#include <QPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Qt3DCore {
    class QEntity;
}

class QSurfaceFormat;

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

//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{

//=============================================================================================================
// DISP3DLIB FORWARD DECLARATIONS
//=============================================================================================================

class MneDataTreeItem;
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
class DISP3DSHARED_EXPORT Data3DTreeModel : public QStandardItemModel
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
     * @return                       Returns a QList with the added surface tree items. The ordering.
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
    QList<SourceSpaceTreeItem*> addSourceSpace(const QString& sSubject,
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
    QList<SourceSpaceTreeItem*> addForwardSolution(const QString& sSubject,
                                                   const QString& sMeasurementSetName,
                                                   const MNELIB::MNEForwardSolution& forwardSolution);

    //=========================================================================================================
    /**
     * Adds source estimated activation data (MNE or RTC-MUSIC).
     *
     * @param[in] sSubject               The name of the subject.
     * @param[in] sMeasurementSetName    The name of the measurement set to which the data is to be added. If it does not exist yet, it will be created.
     * @param[in] tSourceEstimate        The MNESourceEstimate.
     * @param[in] tForwardSolution       The MNEForwardSolution.
     * @param[in] tSurfSet               The surface set holding the left and right hemisphere surfaces.
     * @param[in] tAnnotSet              The annotation set holding the left and right hemisphere annotations.
     *
     * @return                           Returns a pointer to the added tree item. Default is a NULL pointer if no item was added.
     */
    MneDataTreeItem* addSourceData(const QString& sSubject,
                                   const QString& sMeasurementSetName,
                                   const MNELIB::MNESourceEstimate& tSourceEstimate,
                                   const MNELIB::MNEForwardSolution& tForwardSolution,
                                   const FSLIB::SurfaceSet& tSurfSet,
                                   const FSLIB::AnnotationSet& tAnnotSet);

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
     * Adds a list of connectivity estimation data.
     *
     * @param[in] sSubject               The name of the subject.
     * @param[in] sMeasurementSetName    The name of the measurement set to which the data is to be added. If it does not exist yet, it will be created.
     * @param[in] networkData            The list of connectivity data.
     *
     * @return                           Returns a lsit with pointers to the added tree items.
     */
    QList<NetworkTreeItem*> addConnectivityData(const QString& sSubject,
                                                const QString& sMeasurementSetName,
                                                const QList<CONNECTIVITYLIB::Network>& networkData);

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
     * Adds MEG sensor info.
     *
     * @param[in] sSubject           The name of the subject.
     * @param[in] sSensorSetName     The name of the sensor set to which the data is to be added. If it does not exist yet, it will be created.
     * @param[in] sensor             The sensor surface information in form of a BEM model. Sensor surfaces are internally represented as MNEBem models.
     * @param[in] lChInfo            The channel information used to plot the MEG channels.
     * @param[in] bads               The bad channel list.
     *
     * @return                       Returns a pointer to the added tree item. Default is a NULL pointer if no item was added.
     */
    SensorSetTreeItem* addMegSensorInfo(const QString& sSubject,
                                        const QString& sSensorSetName,
                                        const QList<FIFFLIB::FiffChInfo>& lChInfo,
                                        const MNELIB::MNEBem& sensor = MNELIB::MNEBem(),
                                        const QStringList &bads = QStringList());

    //=========================================================================================================
    /**
     * Adds EEG sensor info.
     *
     * @param[in] sSubject           The name of the subject.
     * @param[in] sSensorSetName     The name of the sensor set to which the data is to be added. If it does not exist yet, it will be created.
     * @param[in] lChInfo            The channel information used to plot the EEG channels.
     * @param[in] bads               The bad channel list.
     *
     * @return                       Returns a pointer to the added tree item. Default is a NULL pointer if no item was added.
     */
    SensorSetTreeItem* addEegSensorInfo(const QString& sSubject,
                                        const QString& sSensorSetName,
                                        const QList<FIFFLIB::FiffChInfo>& lChInfo,
                                        const QStringList &bads = QStringList());

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
     * @param[in] fiffInfo               The FiffInfo that holds all information about the sensors.
     * @param[in] sDataType              The data type ("MEG" or "EEG").
     *
     * @return                           Returns a pointer to the added tree item. Default is a NULL pointer if no item was added.
     */
    SensorDataTreeItem* addSensorData(const QString& sSubject,
                                      const QString& sMeasurementSetName,
                                      const Eigen::MatrixXd& matSensorData,
                                      const MNELIB::MNEBemSurface& tBemSurface,
                                      const FIFFLIB::FiffInfo &fiffInfo,
                                      const QString &sDataType);

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
     * Create a subject tree item if the item was not found. This is a convenience function.
     *
     * @param[in] sSubject           The name of the subject.
     *
     * @return                       Returns a pointer to the first found or created subject tree item. Default is a NULL pointer if no item was found.
     */
    SubjectTreeItem* addSubject(const QString& sSubject);

    //=========================================================================================================
    /**
     * Adds live sensor data for interpolation with the cpu.
     *
     * @param[in] sSubject               The name of the subject.
     * @param[in] sMeasurementSetName    The name of the measurement set to which the data is to be added. If it does not exist yet, it will be created.
     * @param[in] matSensorData          The Sensor Data.
     * @param[in] tBemSurface            The Bem Surface data.
     * @param[in] fiffInfo             The FiffInfo that holds all information about the sensors.
     * @param[in] sDataType              The data type ("MEG" or "EEG").
     * @param[in] dCancelDist            Distances higher than this are ignored for the interpolation.
     * @param[in] sInterpolationFunction Function that computes interpolation coefficients using the distance values.
     *
     * @return                           Returns a pointer to the added tree item. Default is a NULL pointer if no item was added.
     */
    SensorDataTreeItem *addCpuSensorData(const QString& sSubject,
                                         const QString& sMeasurementSetName,
                                         const Eigen::MatrixXd& matSensorData,
                                         const MNELIB::MNEBemSurface& tBemSurface,
                                         const FIFFLIB::FiffInfo &fiffInfo,
                                         const QString &sDataType,
                                         const double dCancelDist,
                                         const QString &sInterpolationFunction);

    //=========================================================================================================
    /**
     * Adds live sensor data for interpolation with a compute shader.
     *
     * @param[in] sSubject               The name of the subject.
     * @param[in] sMeasurementSetName    The name of the measurement set to which the data is to be added. If it does not exist yet, it will be created.
     * @param[in] matSensorData          The Sensor Data.
     * @param[in] tBemSurface            The Bem Surface data.
     * @param[in] fiffInfo               The FiffInfo that holds all information about the sensors.
     * @param[in] sDataType              The data type ("MEG" or "EEG").
     * @param[in] dCancelDist            Distances higher than this are ignored for the interpolation.
     * @param[in] sInterpolationFunction Function that computes interpolation coefficients using the distance values.
     *
     * @return                           Returns a pointer to the added tree item. Default is a NULL pointer if no item was added.
     */
    SensorDataTreeItem *addGpuSensorData(const QString& sSubject,
                                         const QString& sMeasurementSetName,
                                         const Eigen::MatrixXd& matSensorData,
                                         const MNELIB::MNEBemSurface& tBemSurface,
                                         const FIFFLIB::FiffInfo &fiffInfo,
                                         const QString &sDataType,
                                         const double dCancelDist,
                                         const QString &sInterpolationFunction);

    //=========================================================================================================
    /**
     * Init the meta types
     */
    void initMetatypes();

    QStandardItem*                   m_pRootItem;            /**< The root item of the tree model. */
    QPointer<Qt3DCore::QEntity>      m_pModelEntity;         /**< The parent 3D entity for this model. */
};
} // NAMESPACE

#endif // DISP3DLIB_DATA3DTREEMODEL_H
