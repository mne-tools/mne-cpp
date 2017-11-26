//=============================================================================================================
/**
* @file     measurementsettreeitem.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     November, 2016
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
* @brief    MeasurementTreeItem class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "measurementtreeitem.h"
#include "../hemisphere/hemispheretreeitem.h"
#include "../sourcespace/sourcespacetreeitem.h"
#include "../sourceactivity/mneestimatetreeitem.h"
#include "../sourceactivity/ecddatatreeitem.h"
#include "../network/networktreeitem.h"
#include "../freesurfer/fssurfacetreeitem.h"
#include "../freesurfer/fsannotationtreeitem.h"
#include "../digitizer/digitizersettreeitem.h"
#include "../digitizer/digitizertreeitem.h"
#include "../mri/mritreeitem.h"
#include "../subject/subjecttreeitem.h"
#include "../sensordata/cpusensordatatreeitem.h"
#include "../sensordata/gpusensordatatreeitem.h"
#include "../bem/bemtreeitem.h"
#include "../bem/bemsurfacetreeitem.h"
#include "../sensorspace/sensorsettreeitem.h"
#include "../sensorspace/sensorsurfacetreeitem.h"

#include <fs/label.h>
#include <fs/annotationset.h>
#include <fs/surfaceset.h>

#include <mne/mne_sourceestimate.h>
#include <mne/mne_sourcespace.h>
#include <mne/mne_bem_surface.h>

#include <fiff/fiff_dig_point_set.h>

#include <inverse/dipoleFit/ecd_set.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;
using namespace MNELIB;
using namespace DISP3DLIB;
using namespace INVERSELIB;
using namespace CONNECTIVITYLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MeasurementTreeItem::MeasurementTreeItem(int iType,
                                         const QString& text)
: AbstractTreeItem(iType, text)
{
    initItem();
}


//*************************************************************************************************************

void MeasurementTreeItem::initItem()
{
    this->setEditable(false);
    this->setCheckable(true);
    this->setCheckState(Qt::Checked);
    this->setToolTip("Measurement item");
}


//*************************************************************************************************************

SourceSpaceTreeItem* MeasurementTreeItem::addData(const MNESourceSpace& tSourceSpace,
                                                  Qt3DCore::QEntity* p3DEntityParent)
{
    //Generate child items based on surface set input parameters
    SourceSpaceTreeItem* pReturnItem = Q_NULLPTR;

    QList<QStandardItem*> itemList = this->findChildren(Data3DTreeModelItemTypes::HemisphereItem);

    //If there are more hemispheres in tSourceSpace than in the tree model
    bool hemiItemFound = false;

    //Search for already created hemi items and add source space data respectivley
    for(int i = 0; i < tSourceSpace.size(); ++i) {
        for(int j = 0; j < itemList.size(); ++j) {
            if(HemisphereTreeItem* pHemiItem = dynamic_cast<HemisphereTreeItem*>(itemList.at(j))) {
                if(pHemiItem->data(Data3DTreeModelItemRoles::SurfaceHemi).toInt() == i) {
                    hemiItemFound = true;
                    pReturnItem = pHemiItem->addData(tSourceSpace[i], p3DEntityParent);
                }
            }
        }

        if(!hemiItemFound) {
            //Item does not exist yet, create it here.
            HemisphereTreeItem* pHemiItem = new HemisphereTreeItem(Data3DTreeModelItemTypes::HemisphereItem);

            pReturnItem = pHemiItem->addData(tSourceSpace[i], p3DEntityParent);

            QList<QStandardItem*> list;
            list << pHemiItem;
            list << new QStandardItem(pHemiItem->toolTip());
            this->appendRow(list);
        }

        hemiItemFound = false;
    }

    return pReturnItem;
}


//*************************************************************************************************************

MneEstimateTreeItem* MeasurementTreeItem::addData(const MNESourceEstimate& tSourceEstimate,
                                                  const MNEForwardSolution& tForwardSolution)
{
    if(!tSourceEstimate.isEmpty()) {
        //Add source estimation data as child
        if(this->findChildren(Data3DTreeModelItemTypes::MNEEstimateItem).size() == 0) {
            //If rt data item does not exists yet, create it here!
            if(!tForwardSolution.isEmpty()) {
                if(!m_pMneEstimateTreeItem) {
                    m_pMneEstimateTreeItem = new MneEstimateTreeItem();
                }

                QList<QStandardItem*> list;
                list << m_pMneEstimateTreeItem;
                list << new QStandardItem(m_pMneEstimateTreeItem->toolTip());
                this->appendRow(list);

                connect(m_pMneEstimateTreeItem.data(), &MneEstimateTreeItem::sourceVertColorChanged,
                        this, &MeasurementTreeItem::onSourceColorChanged);

                //Divide into left right hemi
                if(SubjectTreeItem* pParent = dynamic_cast<SubjectTreeItem*>(this->QStandardItem::parent())) {
                    QList<QStandardItem*> lMRIChildren = pParent->findChildren(Data3DTreeModelItemTypes::MriItem);
                    MriTreeItem* pMriItem = Q_NULLPTR;

                    //Find MRI data set and hemisphere from parent item
                    //Option 1 - Choose first found MRI set
                    if(!lMRIChildren.isEmpty()) {
                        pMriItem = dynamic_cast<MriTreeItem*>(lMRIChildren.first());
                    }

                    //                    //Option 2 - Choose MRI set by its name
                    //                    QString sMRISetName = "MRI";

                    //                    for(int i = 0; i < lMRIChildren.size(); ++i) {
                    //                        if(lMRIChildren.at(i)->text() == sMRISetName) {
                    //                            if(pMriItem = dynamic_cast<MriTreeItem*>(lMRIChildren.at(i))) {
                    //                                i = lMRIChildren.size();
                    //                            }
                    //                        }
                    //                    }

                    if(pMriItem) {
                        QList<QStandardItem*> itemList = pMriItem->findChildren(Data3DTreeModelItemTypes::HemisphereItem);

                        FsSurfaceTreeItem* pSurfaceTreeItemLeft = Q_NULLPTR;
                        FsSurfaceTreeItem* pSurfaceTreeItemRight = Q_NULLPTR;
                        FsAnnotationTreeItem* pAnnotTreeItemLeft = Q_NULLPTR;
                        FsAnnotationTreeItem* pAnnotTreeItemRight = Q_NULLPTR;

                        for(int j = 0; j < itemList.size(); ++j) {
                            if(HemisphereTreeItem* pHemiItem = dynamic_cast<HemisphereTreeItem*>(itemList.at(j))) {
                                if(pHemiItem->data(Data3DTreeModelItemRoles::SurfaceHemi).toInt() == 0) {
                                    pSurfaceTreeItemLeft = pHemiItem->getSurfaceItem();
                                    pAnnotTreeItemLeft = pHemiItem->getAnnotItem();
                                } else if(pHemiItem->data(Data3DTreeModelItemRoles::SurfaceHemi).toInt() == 1) {
                                    pSurfaceTreeItemRight = pHemiItem->getSurfaceItem();
                                    pAnnotTreeItemRight = pHemiItem->getAnnotItem();
                                }
                            }
                        }

                        if(pSurfaceTreeItemLeft && pSurfaceTreeItemRight && pAnnotTreeItemLeft && pAnnotTreeItemRight) {
                            m_pMneEstimateTreeItem->init(tForwardSolution,
                                                         pSurfaceTreeItemLeft->data(Data3DTreeModelItemRoles::SurfaceCurrentColorVert).value<MatrixX3f>(),
                                                         pSurfaceTreeItemRight->data(Data3DTreeModelItemRoles::SurfaceCurrentColorVert).value<MatrixX3f>(),
                                                         pAnnotTreeItemLeft->data(Data3DTreeModelItemRoles::LabeIds).value<VectorXi>(),
                                                         pAnnotTreeItemRight->data(Data3DTreeModelItemRoles::LabeIds).value<VectorXi>(),
                                                         pAnnotTreeItemLeft->data(Data3DTreeModelItemRoles::LabeList).value<QList<FSLIB::Label>>(),
                                                         pAnnotTreeItemRight->data(Data3DTreeModelItemRoles::LabeList).value<QList<FSLIB::Label>>());
                        }
                    }
                }

                m_pMneEstimateTreeItem->addData(tSourceEstimate);
            } else {
                qDebug() << "MeasurementTreeItem::addData - Cannot add real time data since the forwad solution was not provided and therefore the rt source localization data item has not been initilaized yet. Returning...";
            }
        } else {
            if(m_pMneEstimateTreeItem) {
                m_pMneEstimateTreeItem->addData(tSourceEstimate);
            }
        }

        return m_pMneEstimateTreeItem;
    } else {
        qDebug() << "MeasurementTreeItem::addData - tSourceEstimate is empty";
    }

    return Q_NULLPTR;
}


//*************************************************************************************************************

SensorDataTreeItem *MeasurementTreeItem::addData(const MatrixXd &tSensorData,
                                                 const MNEBemSurface &bemSurface,
                                                 const FiffInfo &fiffInfo,
                                                 const QString &sSensorType,
                                                 const double dCancelDist,
                                                 const QString &sInterpolationFunction,
                                                 Qt3DCore::QEntity* pParent,
                                                 bool bUseGPU)
{
    if(!tSensorData.size() == 0) {
        if(sSensorType == "EEG") {
            if(bUseGPU) {
                //GPU for EEG data
                if(m_pGpuEEGSensorDataTreeItem) {
                    m_pGpuEEGSensorDataTreeItem->addData(tSensorData);
                } else {
                    //Add sensor data as child
                    //If rt data item does not exists yet, create it here!
                    m_pGpuEEGSensorDataTreeItem = new GpuSensorDataTreeItem();
                    m_pGpuEEGSensorDataTreeItem->setText("EEG Data");

                    QList<QStandardItem*> list;
                    list << m_pGpuEEGSensorDataTreeItem;
                    list << new QStandardItem(m_pGpuEEGSensorDataTreeItem->toolTip());
                    this->appendRow(list);

                    m_pGpuEEGSensorDataTreeItem->initData(bemSurface,
                                                      fiffInfo,
                                                      sSensorType,
                                                      dCancelDist,
                                                      sInterpolationFunction,
                                                      pParent);

                    m_pGpuEEGSensorDataTreeItem->addData(tSensorData);
                }

                return dynamic_cast<SensorDataTreeItem*>(m_pGpuEEGSensorDataTreeItem.data());
            } else {
                //CPU for EEG data
                if(m_pCpuEEGSensorDataTreeItem) {
                    m_pCpuEEGSensorDataTreeItem->addData(tSensorData);
                } else {
                    //Add sensor data as child
                    //If rt data item does not exists yet, create it here!
                    m_pCpuEEGSensorDataTreeItem = new CpuSensorDataTreeItem();
                    m_pCpuEEGSensorDataTreeItem->setText("EEG Data");

                    QList<QStandardItem*> list;
                    list << m_pCpuEEGSensorDataTreeItem;
                    list << new QStandardItem(m_pCpuEEGSensorDataTreeItem->toolTip());
                    this->appendRow(list);

                    m_pCpuEEGSensorDataTreeItem->initData(bemSurface,
                                                      fiffInfo,
                                                      sSensorType,
                                                      dCancelDist,
                                                      sInterpolationFunction,
                                                      pParent);

                    m_pCpuEEGSensorDataTreeItem->addData(tSensorData);
                }

                return dynamic_cast<SensorDataTreeItem*>(m_pGpuEEGSensorDataTreeItem.data());
            }
        }

        if(sSensorType == "MEG") {
            if(bUseGPU) {
                //GPU for MEG data
                if(m_pCshMEGSensorDataTreeItem) {
                    m_pCshMEGSensorDataTreeItem->addData(tSensorData);
                } else {
                    //Add sensor data as child
                    //If rt data item does not exists yet, create it here!
                    m_pCshMEGSensorDataTreeItem = new GpuSensorDataTreeItem();
                    m_pCshMEGSensorDataTreeItem->setText("MEG Data");

                    QList<QStandardItem*> list;
                    list << m_pCshMEGSensorDataTreeItem;
                    list << new QStandardItem(m_pCshMEGSensorDataTreeItem->toolTip());
                    this->appendRow(list);

                    m_pCshMEGSensorDataTreeItem->initData(bemSurface,
                                                      fiffInfo,
                                                      sSensorType,
                                                      dCancelDist,
                                                      sInterpolationFunction,
                                                      pParent);

                    m_pCshMEGSensorDataTreeItem->addData(tSensorData);
                }

                return dynamic_cast<SensorDataTreeItem*>(m_pCshMEGSensorDataTreeItem.data());
            } else {
                //CPU for MEG data
                if(m_pCpuMEGSensorDataTreeItem) {
                    m_pCpuMEGSensorDataTreeItem->addData(tSensorData);
                } else {
                    //Add sensor data as child
                    //If rt data item does not exists yet, create it here!
                    m_pCpuMEGSensorDataTreeItem = new CpuSensorDataTreeItem();
                    m_pCpuMEGSensorDataTreeItem->setText("MEG Data");

                    QList<QStandardItem*> list;
                    list << m_pCpuMEGSensorDataTreeItem;
                    list << new QStandardItem(m_pCpuMEGSensorDataTreeItem->toolTip());
                    this->appendRow(list);

                    m_pCpuMEGSensorDataTreeItem->initData(bemSurface,
                                                      fiffInfo,
                                                      sSensorType,
                                                      dCancelDist,
                                                      sInterpolationFunction,
                                                      pParent);

                    m_pCpuMEGSensorDataTreeItem->addData(tSensorData);
                }

                return dynamic_cast<SensorDataTreeItem*>(m_pCpuMEGSensorDataTreeItem.data());

            }
        }
    }

    return Q_NULLPTR;
}


//*************************************************************************************************************

EcdDataTreeItem* MeasurementTreeItem::addData(const ECDSet& pECDSet, Qt3DCore::QEntity* p3DEntityParent)
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


//*************************************************************************************************************

DigitizerSetTreeItem* MeasurementTreeItem::addData(const FiffDigPointSet& tDigitizer, Qt3DCore::QEntity *p3DEntityParent)
{
    //Find the digitizer kind
    QList<QStandardItem*> itemDigitizerList = this->findChildren(Data3DTreeModelItemTypes::DigitizerSetItem);
    DigitizerSetTreeItem* pReturnItem = Q_NULLPTR;

    //If digitizer does not exist, create a new one
    if(itemDigitizerList.size() == 0) {
        DigitizerSetTreeItem* digitizerSetItem = new DigitizerSetTreeItem(Data3DTreeModelItemTypes::DigitizerSetItem,"Digitizer");
        itemDigitizerList << digitizerSetItem;
        itemDigitizerList << new QStandardItem(digitizerSetItem->toolTip());
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
}


//*************************************************************************************************************

NetworkTreeItem* MeasurementTreeItem::addData(const Network& tNetworkData, Qt3DCore::QEntity* p3DEntityParent)
{
    if(!tNetworkData.getNodes().isEmpty()) {
        //Add source estimation data as child
        if(this->findChildren(Data3DTreeModelItemTypes::NetworkItem).size() == 0) {
            //If rt data item does not exists yet, create it here!
            if(!m_pNetworkTreeItem) {
                m_pNetworkTreeItem = new NetworkTreeItem(p3DEntityParent);
            }

            QList<QStandardItem*> list;
            list << m_pNetworkTreeItem;
            list << new QStandardItem(m_pNetworkTreeItem->toolTip());
            this->appendRow(list);

            m_pNetworkTreeItem->addData(tNetworkData);
        } else {
            if(m_pNetworkTreeItem) {
                m_pNetworkTreeItem->addData(tNetworkData);
            }
        }

        return m_pNetworkTreeItem;
    } else {
        qDebug() << "MeasurementTreeItem::addData - network data is empty";
    }

    return Q_NULLPTR;
}


//*************************************************************************************************************

void MeasurementTreeItem::setSourceColors(const MatrixX3f& leftHemiColor, const MatrixX3f& rightHemiColor)
{
    if(m_pMneEstimateTreeItem) {
        m_pMneEstimateTreeItem->setColorOrigin(leftHemiColor, rightHemiColor);
    }
}


//*************************************************************************************************************

void MeasurementTreeItem::onSourceColorChanged(const QVariant &vertColors)
{
    emit sourceColorChanged(vertColors);
}
