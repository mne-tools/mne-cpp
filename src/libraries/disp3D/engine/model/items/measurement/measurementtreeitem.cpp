//=============================================================================================================
/**
 * @file     measurementtreeitem.cpp
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Juan Garcia-Prieto <juangpc@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     November, 2016
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
 * @brief    MeasurementTreeItem class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "measurementtreeitem.h"
#include "../hemisphere/hemispheretreeitem.h"
#include "../sourcespace/sourcespacetreeitem.h"
#include "../sourcedata/mnedatatreeitem.h"
#include "../sourcedata/ecddatatreeitem.h"
#include "../network/networktreeitem.h"
#include "../freesurfer/fssurfacetreeitem.h"
#include "../freesurfer/fsannotationtreeitem.h"
#include "../digitizer/digitizersettreeitem.h"
#include "../digitizer/digitizertreeitem.h"
#include "../mri/mritreeitem.h"
#include "../subject/subjecttreeitem.h"
#include "../bem/bemtreeitem.h"
#include "../bem/bemsurfacetreeitem.h"
#include "../sensorspace/sensorsettreeitem.h"
#include "../sensorspace/sensorsurfacetreeitem.h"
#include "../sensordata/sensordatatreeitem.h"

#include <fs/label.h>
#include <fs/annotationset.h>
#include <fs/surfaceset.h>

#include <mne/mne_sourceestimate.h>
#include <mne/mne_sourcespace.h>
#include <mne/mne_bem_surface.h>

#include <fiff/fiff_dig_point_set.h>

#include <inverse/dipoleFit/ecd_set.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;
using namespace MNELIB;
using namespace DISP3DLIB;
using namespace INVERSELIB;
using namespace CONNECTIVITYLIB;
using namespace Eigen;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MeasurementTreeItem::MeasurementTreeItem(int iType,
                                         const QString& text)
: AbstractTreeItem(iType, text)
{
    initItem();
}

//=============================================================================================================

void MeasurementTreeItem::initItem()
{
    this->setEditable(false);
    this->setCheckable(true);
    this->setCheckState(Qt::Checked);
    this->setToolTip("Measurement item");
}

//=============================================================================================================

QList<SourceSpaceTreeItem*> MeasurementTreeItem::addData(const MNESourceSpace& tSourceSpace,
                                                         Qt3DCore::QEntity* p3DEntityParent)
{
    //Generate child items based on surface set input parameters
    QList<SourceSpaceTreeItem*> pReturnItem;

    QList<QStandardItem*> itemList = this->findChildren(Data3DTreeModelItemTypes::HemisphereItem);

    //If there are more hemispheres in tSourceSpace than in the tree model
    bool hemiItemFound = false;

    //Search for already created hemi items and add source space data respectivley
    for(int i = 0; i < tSourceSpace.size(); ++i) {
        for(int j = 0; j < itemList.size(); ++j) {
            if(HemisphereTreeItem* pHemiItem = dynamic_cast<HemisphereTreeItem*>(itemList.at(j))) {
                if(pHemiItem->data(Data3DTreeModelItemRoles::SurfaceHemi).toInt() == i) {
                    hemiItemFound = true;
                    pReturnItem.append(pHemiItem->addData(tSourceSpace[i], p3DEntityParent));
                }
            }
        }

        if(!hemiItemFound) {
            //Item does not exist yet, create it here.
            HemisphereTreeItem* pHemiItem = new HemisphereTreeItem(Data3DTreeModelItemTypes::HemisphereItem);

            pReturnItem.append(pHemiItem->addData(tSourceSpace[i], p3DEntityParent));

            QList<QStandardItem*> list;
            list << pHemiItem;
            list << new QStandardItem(pHemiItem->toolTip());
            this->appendRow(list);
        }

        hemiItemFound = false;
    }

    return pReturnItem;
}

//=============================================================================================================

MneDataTreeItem* MeasurementTreeItem::addData(const MNESourceEstimate& tSourceEstimate,
                                              const MNEForwardSolution& tForwardSolution,
                                              const SurfaceSet& tSurfSet,
                                              const AnnotationSet& tAnnotSet,
                                              Qt3DCore::QEntity* p3DEntityParent,
                                              bool bUseGPU)
{
    if(!tSourceEstimate.isEmpty()) {        
        //CPU for source data
        if(m_pMneDataTreeItem) {
            m_pMneDataTreeItem->addData(tSourceEstimate);
        } else {
            //Add sensor data as child
            //If item does not exists yet, create it here!
            m_pMneDataTreeItem = new MneDataTreeItem(Data3DTreeModelItemTypes::MNEDataItem,
                                                             "MNE data",
                                                             bUseGPU);

            QList<QStandardItem*> list;
            list << m_pMneDataTreeItem;
            list << new QStandardItem(m_pMneDataTreeItem->toolTip());
            this->appendRow(list);

            m_pMneDataTreeItem->initData(tForwardSolution,
                                             tSurfSet,
                                             tAnnotSet,
                                             p3DEntityParent);

            m_pMneDataTreeItem->addData(tSourceEstimate);
        }

        return m_pMneDataTreeItem;
    }

    return Q_NULLPTR;
}

//=============================================================================================================

SensorDataTreeItem *MeasurementTreeItem::addData(const MatrixXd &tSensorData,
                                                 const MNEBemSurface &bemSurface,
                                                 const FiffInfo &fiffInfo,
                                                 const QString &sSensorType,
                                                 Qt3DCore::QEntity* p3DEntityParent,
                                                 bool bUseGPU)
{
    if(!tSensorData.size() == 0) {
        if(sSensorType == "EEG") {
            //GPU for EEG data
            if(m_pEEGSensorDataTreeItem) {
                m_pEEGSensorDataTreeItem->addData(tSensorData);
            } else {
                //Add sensor data as child
                //If item does not exists yet, create it here!
                m_pEEGSensorDataTreeItem = new SensorDataTreeItem(Data3DTreeModelItemTypes::SensorDataItem,
                                                                  "Sensor Data",
                                                                  bUseGPU);
                m_pEEGSensorDataTreeItem->setText("EEG Data");

                QList<QStandardItem*> list;
                list << m_pEEGSensorDataTreeItem;
                list << new QStandardItem(m_pEEGSensorDataTreeItem->toolTip());
                this->appendRow(list);

                m_pEEGSensorDataTreeItem->initData(bemSurface,
                                                   fiffInfo,
                                                   sSensorType,
                                                   p3DEntityParent);

                m_pEEGSensorDataTreeItem->addData(tSensorData);
            }

            return m_pEEGSensorDataTreeItem;
        }

        if(sSensorType == "MEG") {
            //GPU for EEG data
            if(m_pMEGSensorDataTreeItem) {
                m_pMEGSensorDataTreeItem->addData(tSensorData);
            } else {
                //Add sensor data as child
                //If item does not exists yet, create it here!
                m_pMEGSensorDataTreeItem = new SensorDataTreeItem(Data3DTreeModelItemTypes::SensorDataItem,
                                                                  "Sensor Data",
                                                                  bUseGPU);
                m_pMEGSensorDataTreeItem->setText("MEG Data");

                QList<QStandardItem*> list;
                list << m_pMEGSensorDataTreeItem;
                list << new QStandardItem(m_pMEGSensorDataTreeItem->toolTip());
                this->appendRow(list);

                m_pMEGSensorDataTreeItem->initData(bemSurface,
                                                   fiffInfo,
                                                   sSensorType,
                                                   p3DEntityParent);

                m_pMEGSensorDataTreeItem->addData(tSensorData);
            }

            return m_pMEGSensorDataTreeItem;
        }
    }

    return Q_NULLPTR;
}

//=============================================================================================================

EcdDataTreeItem* MeasurementTreeItem::addData(const ECDSet& pECDSet,
                                              Qt3DCore::QEntity* p3DEntityParent)
{
    if(pECDSet.size() > 0) {
        //Add source estimation data as child
        if(this->findChildren(Data3DTreeModelItemTypes::ECDDataItem).size() == 0) {
            //If ecd data item does not exists yet, create it here!
            if(!m_EcdDataTreeItem) {
                m_EcdDataTreeItem = new EcdDataTreeItem(p3DEntityParent);
            }

            QList<QStandardItem*> list;
            list << m_EcdDataTreeItem;
            list << new QStandardItem(m_EcdDataTreeItem->toolTip());
            this->appendRow(list);

            m_EcdDataTreeItem->addData(pECDSet);

        } else {
            if(m_EcdDataTreeItem) {
                m_EcdDataTreeItem->addData(pECDSet);
            }
        }

        return m_EcdDataTreeItem;
    } else {
        qDebug() << "MeasurementTreeItem::addData - pECDSet is empty";
    }

    return Q_NULLPTR;
}

//=============================================================================================================

DigitizerSetTreeItem* MeasurementTreeItem::addData(const FiffDigPointSet& tDigitizer,
                                                   Qt3DCore::QEntity *p3DEntityParent)
{
    if(tDigitizer.size() > 0) {
        //Find the digitizer kind
        QList<QStandardItem*> itemDigitizerList = this->findChildren(Data3DTreeModelItemTypes::DigitizerSetItem);
        DigitizerSetTreeItem* pReturnItem = Q_NULLPTR;

        //If digitizer does not exist, create a new one
        if(itemDigitizerList.size() == 0) {
            pReturnItem = new DigitizerSetTreeItem(Data3DTreeModelItemTypes::DigitizerSetItem,"Digitizer");
            itemDigitizerList << pReturnItem;
            itemDigitizerList << new QStandardItem(pReturnItem->toolTip());
            this->appendRow(itemDigitizerList);
        }

        // Add Data to the first Digitizer Set Item
        //Check if it is really a digitizer tree item
        if(itemDigitizerList.at(0)->type() == Data3DTreeModelItemTypes::DigitizerSetItem) {
            if(pReturnItem = dynamic_cast<DigitizerSetTreeItem*>(itemDigitizerList.at(0))) {
                pReturnItem->addData(tDigitizer, p3DEntityParent);
            }
        }

        return pReturnItem;
    } else {
        qDebug() << "MeasurementTreeItem::addData - digitizer set is empty";
    }

    return Q_NULLPTR;
}

//=============================================================================================================

NetworkTreeItem* MeasurementTreeItem::addData(const Network& tNetworkData,
                                              Qt3DCore::QEntity* p3DEntityParent)
{
    if(!tNetworkData.getNodes().isEmpty()) {
        NetworkTreeItem* pReturnItem = Q_NULLPTR;

        QPair<float,float> freqs = tNetworkData.getFrequencyRange();
        QString sItemName = QString("%1").arg(tNetworkData.getConnectivityMethod());//.arg(QString::number(freqs.first)).arg(QString::number(freqs.second));

        //Add network estimation data as child
        QList<QStandardItem*> lItems = this->findChildren(sItemName);

        if(lItems.isEmpty()) {
            pReturnItem = new NetworkTreeItem(p3DEntityParent);

            pReturnItem->setText(sItemName);

            QList<QStandardItem*> list;
            list << pReturnItem;
            list << new QStandardItem(pReturnItem->toolTip());
            this->appendRow(list);

            pReturnItem->addData(tNetworkData);
        } else {
            if(lItems.first()) {
                if(pReturnItem = dynamic_cast<NetworkTreeItem*>(lItems.first())) {
                    pReturnItem->addData(tNetworkData);
                }
            }
        }

        return pReturnItem;
    } else {
        qDebug() << "MeasurementTreeItem::addData - network data is empty";
    }

    return Q_NULLPTR;
}
