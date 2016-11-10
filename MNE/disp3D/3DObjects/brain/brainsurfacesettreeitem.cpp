//=============================================================================================================
/**
* @file     brainsurfacesettreeitem.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     November, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    BrainSurfaceSetTreeItem class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainsurfacesettreeitem.h"
#include "brainhemispheretreeitem.h"
#include "brainrtsourcelocdatatreeitem.h"
#include "brainrtconnectivitydatatreeitem.h"
#include "brainsurfacetreeitem.h"
#include "brainannotationtreeitem.h"
#include "../digitizer/digitizersettreeitem.h"
#include "../digitizer/digitizertreeitem.h"


#include <fs/label.h>
#include <fs/annotationset.h>
#include <fs/surfaceset.h>

#include <mne/mne_sourceestimate.h>
#include <mne/mne_sourcespace.h>

#include <fiff/fiff_dig_point_set.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QList>
#include <QVariant>
#include <QStringList>
#include <QColor>
#include <QStandardItem>
#include <QStandardItemModel>


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
using namespace CONNECTIVITYLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainSurfaceSetTreeItem::BrainSurfaceSetTreeItem(int iType, const QString& text)
: AbstractTreeItem(iType, text)
, m_pBrainRTSourceLocDataTreeItem(new BrainRTSourceLocDataTreeItem())
, m_pBrainRTConnectivityDataTreeItem(new BrainRTConnectivityDataTreeItem())
{
    this->setEditable(false);
    this->setCheckable(true);
    this->setCheckState(Qt::Checked);
    this->setToolTip("Brain surface set");
}


//*************************************************************************************************************

BrainSurfaceSetTreeItem::~BrainSurfaceSetTreeItem()
{
}


//*************************************************************************************************************

QVariant BrainSurfaceSetTreeItem::data(int role) const
{
    switch(role) {
        case Data3DTreeModelItemRoles::SurfaceSetName:
            return QVariant();
    }

    return AbstractTreeItem::data(role);
}


//*************************************************************************************************************

void  BrainSurfaceSetTreeItem::setData(const QVariant& value, int role)
{
    AbstractTreeItem::setData(value, role);
}


//*************************************************************************************************************

bool BrainSurfaceSetTreeItem::addData(const SurfaceSet& tSurfaceSet, const AnnotationSet& tAnnotationSet, Qt3DCore::QEntity* p3DEntityParent)
{
    //Generate child items based on surface set input parameters
    bool state = false;

    QList<QStandardItem*> itemList = this->findChildren(Data3DTreeModelItemTypes::HemisphereItem);

    //If there are more hemispheres in tSourceSpace than in the tree model
    bool hemiItemFound = false;

    //Search for already created hemi items and add source space data respectivley
    for(int i = 0; i < tSurfaceSet.size(); ++i) {
        for(int j = 0; j < itemList.size(); ++j) {
            BrainHemisphereTreeItem* pHemiItem = dynamic_cast<BrainHemisphereTreeItem*>(itemList.at(j));

            if(pHemiItem->data(Data3DTreeModelItemRoles::SurfaceHemi).toInt() == tSurfaceSet[i].hemi()) {
                hemiItemFound = true;

                if(i < tAnnotationSet.size()) {
                    if(tAnnotationSet[i].hemi() == tSurfaceSet[i].hemi()) {
                        state = pHemiItem->addData(tSurfaceSet[i], tAnnotationSet[i], p3DEntityParent);
                    } else {
                        state = pHemiItem->addData(tSurfaceSet[i], Annotation(), p3DEntityParent);
                    }
                } else {
                    state = pHemiItem->addData(tSurfaceSet[i], Annotation(), p3DEntityParent);
                }
            }
        }

        if(!hemiItemFound) {
            //Item does not exist yet, create it here.
            BrainHemisphereTreeItem* pHemiItem = new BrainHemisphereTreeItem(Data3DTreeModelItemTypes::HemisphereItem);            

            QList<QStandardItem*> list;
            list << pHemiItem;
            list << new QStandardItem(pHemiItem->toolTip());
            this->appendRow(list);

            if(i < tAnnotationSet.size()) {
                if(tAnnotationSet[i].hemi() == tSurfaceSet[i].hemi()) {
                    state = pHemiItem->addData(tSurfaceSet[i], tAnnotationSet[i], p3DEntityParent);
                } else {
                    state = pHemiItem->addData(tSurfaceSet[i], Annotation(), p3DEntityParent);
                }
            } else {
                state = pHemiItem->addData(tSurfaceSet[i], Annotation(), p3DEntityParent);
            }

            connect(pHemiItem->getSurfaceItem(), &BrainSurfaceTreeItem::colorInfoOriginChanged,
                this, &BrainSurfaceSetTreeItem::onColorInfoOriginChanged);
        }

        hemiItemFound = false;
    }

    return state;
}


//*************************************************************************************************************

bool BrainSurfaceSetTreeItem::addData(const Surface& tSurface, const Annotation& tAnnotation, Qt3DCore::QEntity* p3DEntityParent)
{
    //Generate child items based on surface set input parameters
    bool state = false;

    QList<QStandardItem*> itemList = this->findChildren(Data3DTreeModelItemTypes::HemisphereItem);

    bool hemiItemFound = false;

    //Search for already created hemi items and add source space data respectivley
    for(int j = 0; j < itemList.size(); ++j) {
        if(BrainHemisphereTreeItem* pHemiItem = dynamic_cast<BrainHemisphereTreeItem*>(itemList.at(j))) {
            if(pHemiItem->data(Data3DTreeModelItemRoles::SurfaceHemi).toInt() == tSurface.hemi()) {
                hemiItemFound = true;

                if(tAnnotation.hemi() == tSurface.hemi()) {
                    state = pHemiItem->addData(tSurface, tAnnotation, p3DEntityParent);
                } else {
                    state = pHemiItem->addData(tSurface, Annotation(), p3DEntityParent);
                }
            }
        }
    }

    if(!hemiItemFound) {
        //Item does not exist yet, create it here.
        BrainHemisphereTreeItem* pHemiItem = new BrainHemisphereTreeItem(Data3DTreeModelItemTypes::HemisphereItem);

        if(tAnnotation.hemi() == tSurface.hemi()) {
            state = pHemiItem->addData(tSurface, tAnnotation, p3DEntityParent);
        } else {
            state = pHemiItem->addData(tSurface, Annotation(), p3DEntityParent);
        }

        QList<QStandardItem*> list;
        list << pHemiItem;
        list << new QStandardItem(pHemiItem->toolTip());
        this->appendRow(list);

        connect(pHemiItem->getSurfaceItem(), &BrainSurfaceTreeItem::colorInfoOriginChanged,
            this, &BrainSurfaceSetTreeItem::onColorInfoOriginChanged);
    }

    return state;
}


//*************************************************************************************************************

bool BrainSurfaceSetTreeItem::addData(const MNESourceSpace& tSourceSpace, Qt3DCore::QEntity* p3DEntityParent)
{
    //Generate child items based on surface set input parameters
    bool state = false;

    QList<QStandardItem*> itemList = this->findChildren(Data3DTreeModelItemTypes::HemisphereItem);

    //If there are more hemispheres in tSourceSpace than in the tree model
    bool hemiItemFound = false;

    //Search for already created hemi items and add source space data respectivley
    for(int i = 0; i < tSourceSpace.size(); ++i) {
        for(int j = 0; j < itemList.size(); ++j) {
            if(BrainHemisphereTreeItem* pHemiItem = dynamic_cast<BrainHemisphereTreeItem*>(itemList.at(j))) {
                if(pHemiItem->data(Data3DTreeModelItemRoles::SurfaceHemi).toInt() == i) {
                    hemiItemFound = true;
                    state = pHemiItem->addData(tSourceSpace[i], p3DEntityParent);
                }
            }
        }

        if(!hemiItemFound) {
            //Item does not exist yet, create it here.
            BrainHemisphereTreeItem* pHemiItem = new BrainHemisphereTreeItem(Data3DTreeModelItemTypes::HemisphereItem);

            state = pHemiItem->addData(tSourceSpace[i], p3DEntityParent);

            QList<QStandardItem*> list;
            list << pHemiItem;
            list << new QStandardItem(pHemiItem->toolTip());
            this->appendRow(list);
        }

        hemiItemFound = false;
    }

    return state;
}


//*************************************************************************************************************

BrainRTSourceLocDataTreeItem* BrainSurfaceSetTreeItem::addData(const MNESourceEstimate& tSourceEstimate, const MNEForwardSolution& tForwardSolution)
{
    if(!tSourceEstimate.isEmpty()) {
        //Add source estimation data as child
        if(this->findChildren(Data3DTreeModelItemTypes::RTSourceLocDataItem).size() == 0) {
            //If rt data item does not exists yet, create it here!
            if(!tForwardSolution.isEmpty()) {
                m_pBrainRTSourceLocDataTreeItem = new BrainRTSourceLocDataTreeItem();

                QList<QStandardItem*> list;
                list << m_pBrainRTSourceLocDataTreeItem;
                list << new QStandardItem(m_pBrainRTSourceLocDataTreeItem->toolTip());
                this->appendRow(list);

                connect(m_pBrainRTSourceLocDataTreeItem, &BrainRTSourceLocDataTreeItem::rtVertColorChanged,
                        this, &BrainSurfaceSetTreeItem::onRtVertColorChanged);

                //Divide into left right hemi
                QList<QStandardItem*> itemList = this->findChildren(Data3DTreeModelItemTypes::HemisphereItem);

                BrainSurfaceTreeItem* pSurfaceTreeItemLeft = Q_NULLPTR;
                BrainSurfaceTreeItem* pSurfaceTreeItemRight = Q_NULLPTR;
                BrainAnnotationTreeItem* pAnnotTreeItemLeft = Q_NULLPTR;
                BrainAnnotationTreeItem* pAnnotTreeItemRight = Q_NULLPTR;

                for(int j = 0; j < itemList.size(); ++j) {
                    if(BrainHemisphereTreeItem* pHemiItem = dynamic_cast<BrainHemisphereTreeItem*>(itemList.at(j))) {
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
                    m_pBrainRTSourceLocDataTreeItem->init(tForwardSolution,
                                                        pSurfaceTreeItemLeft->data(Data3DTreeModelItemRoles::SurfaceCurrentColorVert).value<QByteArray>(),
                                                        pSurfaceTreeItemRight->data(Data3DTreeModelItemRoles::SurfaceCurrentColorVert).value<QByteArray>(),
                                                        pAnnotTreeItemLeft->data(Data3DTreeModelItemRoles::LabeIds).value<VectorXi>(),
                                                        pAnnotTreeItemRight->data(Data3DTreeModelItemRoles::LabeIds).value<VectorXi>(),
                                                        pAnnotTreeItemLeft->data(Data3DTreeModelItemRoles::LabeList).value<QList<FSLIB::Label>>(),
                                                        pAnnotTreeItemRight->data(Data3DTreeModelItemRoles::LabeList).value<QList<FSLIB::Label>>());
                }

                m_pBrainRTSourceLocDataTreeItem->addData(tSourceEstimate);
            } else {
                qDebug() << "BrainSurfaceSetTreeItem::addData - Cannot add real time data since the forwad solution was not provided and therefore the rt source localization data item has not been initilaized yet. Returning...";
            }
        } else {
            m_pBrainRTSourceLocDataTreeItem->addData(tSourceEstimate);
        }

        return m_pBrainRTSourceLocDataTreeItem;
    } else {
        qDebug() << "BrainSurfaceSetTreeItem::addData - tSourceEstimate is empty";
    }

    return new BrainRTSourceLocDataTreeItem();
}


//*************************************************************************************************************

bool BrainSurfaceSetTreeItem::addData(const FiffDigPointSet &tDigitizer, Qt3DCore::QEntity *p3DEntityParent)
{
    //Find the digitizerkind
    QList<QStandardItem*> itemDigitizerList = this->findChildren(Data3DTreeModelItemTypes::DigitizerSetItem);

    //If digitizer does not exist, create a new one
    if(itemDigitizerList.size() == 0) {
        DigitizerSetTreeItem* digitizerSetItem = new DigitizerSetTreeItem(Data3DTreeModelItemTypes::DigitizerSetItem,"Digitizer");
        itemDigitizerList << digitizerSetItem;
        itemDigitizerList << new QStandardItem(digitizerSetItem->toolTip());
        this->appendRow(itemDigitizerList);
    }

    // Add Data to the first Digitizer Set Item
    bool state = false;

    //Check if it is really a digitizer tree item
    if((itemDigitizerList.at(0)->type() == Data3DTreeModelItemTypes::DigitizerSetItem)) {
        DigitizerSetTreeItem* pDigitizerSetItem = dynamic_cast<DigitizerSetTreeItem*>(itemDigitizerList.at(0));
        state = pDigitizerSetItem->addData(tDigitizer, p3DEntityParent);
    }
    else{
        state = false;
    }

    return state;
}


//*************************************************************************************************************

BrainRTConnectivityDataTreeItem* BrainSurfaceSetTreeItem::addData(Network::SPtr pNetworkData, Qt3DCore::QEntity* p3DEntityParent)
{
    if(!pNetworkData->getNodes().isEmpty()) {
        //Add source estimation data as child
        if(this->findChildren(Data3DTreeModelItemTypes::RTConnectivityDataItem).size() == 0) {
            //If rt data item does not exists yet, create it here!
            m_pBrainRTConnectivityDataTreeItem = new BrainRTConnectivityDataTreeItem();

            QList<QStandardItem*> list;
            list << m_pBrainRTConnectivityDataTreeItem;
            list << new QStandardItem(m_pBrainRTConnectivityDataTreeItem->toolTip());
            this->appendRow(list);

            m_pBrainRTConnectivityDataTreeItem->init(p3DEntityParent);
            m_pBrainRTConnectivityDataTreeItem->addData(pNetworkData);
        } else {
            m_pBrainRTConnectivityDataTreeItem->addData(pNetworkData);
        }

        return m_pBrainRTConnectivityDataTreeItem;
    } else {
        qDebug() << "BrainSurfaceSetTreeItem::addData - network data is empty";
    }

    return new BrainRTConnectivityDataTreeItem();
}


//*************************************************************************************************************

void BrainSurfaceSetTreeItem::onCheckStateChanged(const Qt::CheckState& checkState)
{
    for(int i = 0; i < this->rowCount(); ++i) {
        if(this->child(i)->isCheckable()) {
            this->child(i)->setCheckState(checkState);
        }
    }
}


//*************************************************************************************************************

void BrainSurfaceSetTreeItem::onRtVertColorChanged(const QPair<QByteArray, QByteArray>& sourceColorSamples)
{
    QList<QStandardItem*> itemList = this->findChildren(Data3DTreeModelItemTypes::HemisphereItem);

    for(int j = 0; j < itemList.size(); ++j) {
        if(BrainHemisphereTreeItem* pHemiItem = dynamic_cast<BrainHemisphereTreeItem*>(itemList.at(j))) {
            if(pHemiItem->data(Data3DTreeModelItemRoles::SurfaceHemi).toInt() == 0) {
                pHemiItem->onRtVertColorChanged(sourceColorSamples.first);
            } else if (pHemiItem->data(Data3DTreeModelItemRoles::SurfaceHemi).toInt() == 1) {
                pHemiItem->onRtVertColorChanged(sourceColorSamples.second);
            }
        }
    }
}


//*************************************************************************************************************

void BrainSurfaceSetTreeItem::onColorInfoOriginChanged()
{
    QList<QStandardItem*> itemList = this->findChildren(Data3DTreeModelItemTypes::HemisphereItem);

    BrainSurfaceTreeItem* pSurfaceTreeItemLeft = Q_NULLPTR;
    BrainSurfaceTreeItem* pSurfaceTreeItemRight = Q_NULLPTR;

    for(int j = 0; j < itemList.size(); ++j) {
        if(BrainHemisphereTreeItem* pHemiItem = dynamic_cast<BrainHemisphereTreeItem*>(itemList.at(j))) {
            if(pHemiItem->data(Data3DTreeModelItemRoles::SurfaceHemi).toInt() == 0) {
                pSurfaceTreeItemLeft = pHemiItem->getSurfaceItem();
            } else if(pHemiItem->data(Data3DTreeModelItemRoles::SurfaceHemi).toInt() == 1) {
                pSurfaceTreeItemRight = pHemiItem->getSurfaceItem();
            }
        }
    }

    if(pSurfaceTreeItemLeft && pSurfaceTreeItemRight && !this->findChildren(Data3DTreeModelItemTypes::RTSourceLocDataItem).isEmpty()) {
        m_pBrainRTSourceLocDataTreeItem->onColorInfoOriginChanged(pSurfaceTreeItemLeft->data(Data3DTreeModelItemRoles::SurfaceCurrentColorVert).value<QByteArray>(),
                                                                    pSurfaceTreeItemRight->data(Data3DTreeModelItemRoles::SurfaceCurrentColorVert).value<QByteArray>());
    }
}

