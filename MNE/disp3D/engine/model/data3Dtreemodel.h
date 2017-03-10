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
}

namespace FIFFLIB{
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
class DigitizerSetTreeItem;
class SubjectTreeItem;
class MeasurementTreeItem;


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
    * @param[in] subject            The name of the subject.
    * @param[in] sMriSetName        The name of the MRI set to which the data is to be added. If it does not exist yet, it will be created.
    * @param[in] tSurfaceSet        FreeSurfer surface set.
    * @param[in] tAnnotationSet     FreeSurfer annotation set.
    *
    * @return                       Returns a QList with the added surface tree items. The ordering
    *                               of the list hereby corresponds to the ordering of the input surface set.
    *                               The list is empty if no item was added.
    */
    QList<FsSurfaceTreeItem*> addSurfaceSet(const QString& subject, const QString& sMriSetName, const FSLIB::SurfaceSet& tSurfaceSet, const FSLIB::AnnotationSet& tAnnotationSet = FSLIB::AnnotationSet());

    //=========================================================================================================
    /**
    * Adds FreeSurfer brain data.
    *
    * @param[in] subject            The name of the subject.
    * @param[in] sMriSetName        The name of the MRI set to which the data is to be added. If it does not exist yet, it will be created.
    * @param[in] pSurface           FreeSurfer surface.
    * @param[in] pAnnotation        FreeSurfer annotation.
    *
    * @return                       Returns a pointer to the added tree item. Default is a NULL pointer if no item was added.
    */
    FsSurfaceTreeItem* addSurface(const QString& subject, const QString& set, const FSLIB::Surface& tSurface, const FSLIB::Annotation& tAnnotation = FSLIB::Annotation());

    //=========================================================================================================
    /**
    * Adds source space brain data.
    *
    * @param[in] subject                The name of the subject.
    * @param[in] sMeasurementSetName    The name of the measurement set to which the data is to be added. If it does not exist yet, it will be created.
    * @param[in] tSourceSpace           The source space information.
    *
    * @return                           Returns a pointer to the added tree item. Default is a NULL pointer if no item was added.
    */
    SourceSpaceTreeItem* addSourceSpace(const QString& subject, const QString& sMeasurementSetName, const MNELIB::MNESourceSpace& tSourceSpace);

    //=========================================================================================================
    /**
    * Adds a forward solution data to the brain tree model. Convenient function to addBrainData(const QString& text, const MNESourceSpace& tSourceSpace).
    *
    * @param[in] subject                The name of the subject.
    * @param[in] sMeasurementSetName    The name of the measurement set to which the data is to be added. If it does not exist yet, it will be created.
    * @param[in] tForwardSolution       The forward solution information.
    *
    * @return                           Returns a pointer to the added tree item. Default is a NULL pointer if no item was added.
    */
    SourceSpaceTreeItem* addForwardSolution(const QString& subject, const QString& sMeasurementSetName, const MNELIB::MNEForwardSolution& tForwardSolution);

    //=========================================================================================================
    /**
    * Adds source estimated activation data (MNE or RTC-MUSIC).
    *
    * @param[in] subject                The name of the subject.
    * @param[in] sMeasurementSetName    The name of the measurement set to which the data is to be added. If it does not exist yet, it will be created.
    * @param[in] tSourceEstimate        The MNESourceEstimate.
    * @param[in] tForwardSolution       The MNEForwardSolution.
    *
    * @return                           Returns a pointer to the added tree item. Default is a NULL pointer if no item was added.
    */
    MneEstimateTreeItem* addSourceData(const QString& subject, const QString& sMeasurementSetName, const MNELIB::MNESourceEstimate& tSourceEstimate, const MNELIB::MNEForwardSolution& tForwardSolution = MNELIB::MNEForwardSolution());

    //=========================================================================================================
    /**
    * Adds source estimated activation data (dipole fit).
    *
    * @param[in] subject                The name of the subject.
    * @param[in] sMeasurementSetName    The name of the measurement set to which the data is to be added. If it does not exist yet, it will be created.
    * @param[in] tECDSet                The ECDSet dipole data.
    *
    * @return                           Returns a pointer to the added tree item. Default is a NULL pointer if no item was added.
    */
    EcdDataTreeItem* addDipoleFitData(const QString& subject, const QString& set, const INVERSELIB::ECDSet& tECDSet);

    //=========================================================================================================
    /**
    * Adds connectivity estimation data.
    *
    * @param[in] subject                The name of the subject.
    * @param[in] sMeasurementSetName    The name of the measurement set to which the data is to be added. If it does not exist yet, it will be created.
    * @param[in] tNetworkData           The connectivity data.
    *
    * @return                           Returns a pointer to the added tree item. Default is a NULL pointer if no item was added.
    */
    NetworkTreeItem* addConnectivityData(const QString& subject, const QString& sMeasurementSetName, const CONNECTIVITYLIB::Network& tNetworkData);

    //=========================================================================================================
    /**
    * Adds BEM data.
    *
    * @param[in] subject            The name of the subject.
    * @param[in] sBemSetName        The name of the BEM set to which the data is to be added. If it does not exist yet, it will be created.
    * @param[in] tBem               The Bem information.
    *
    * @return                       Returns a pointer to the added tree item. Default is a NULL pointer if no item was added.
    */
    BemTreeItem* addBemData(const QString& subject, const QString& sBemSetName, const MNELIB::MNEBem& tBem);

    //=========================================================================================================
    /**
    * Adds digitizer data.
    *
    * @param[in] subject                The name of the subject.
    * @param[in] sMeasurementSetName    The name of the measurement set to which the data is to be added. If it does not exist yet, it will be created.
    * @param[in] tDigitizer             The digitizer information.
    *
    * @return                           Returns a pointer to the added tree item. Default is a NULL pointer if no item was added.
    */
    DigitizerSetTreeItem* addDigitizerData(const QString& subject, const QString& sMeasurementSetName, const FIFFLIB::FiffDigPointSet &tDigitizer);

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
    * @param[in] subject            The name of the subject.
    *
    * @return                       Returns a pointer to the first found or created subject tree item. Default is a NULL pointer if no item was found.
    */
    SubjectTreeItem* addSubject(const QString& subject);

    //=========================================================================================================
    /**
    * Adds an item with its toolTip as second coulm item as description to the model.
    *
    * @param[in] pItemParent         The parent item.
    * @param[in] pItemAdd            The item which is added as a row to the parent item.
    */
    void addItemWithDescription(QStandardItem* pItemParent, QStandardItem* pItemAdd);

    //=========================================================================================================
    /**
    * Connects measurement items and their data (i.e. MNE source data) to already loaded MRI data
    *
    * @param[in] pSubjectItem           The subject item which holds the MRI data items.
    * @param[in] pMeasurementItem       The measurement item which is to be connected.
    */
    void connectMeasurementToMriItems(SubjectTreeItem* pSubjectItem, MeasurementTreeItem* pMeasurementItem);

    QStandardItem*                   m_pRootItem;            /**< The root item of the tree model. */
    QPointer<Qt3DCore::QEntity>      m_pModelEntity;         /**< The parent 3D entity for this model. */
};

} // NAMESPACE

#endif // DATA3DTREEMODEL_H
