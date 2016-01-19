//=============================================================================================================
/**
* @file     braintreemetaitem.cpp
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
* @brief    BrainTreeMetaItem class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "BrainTreeMetaItem.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainTreeMetaItem::BrainTreeMetaItem(int iType, const QString& text)
: AbstractTreeItem(iType, text)
{
    QString sToolTip;

    switch(m_iType) {
        case BrainTreeMetaItemTypes::SurfaceFileName:
            sToolTip = "Surface file name";
            break;
        case BrainTreeMetaItemTypes::SurfaceFilePath:
            sToolTip = "Surface file path";
            break;
        case BrainTreeMetaItemTypes::AnnotFileName:
            sToolTip = "Annotation file name";
            break;
        case BrainTreeMetaItemTypes::AnnotFilePath:
            sToolTip = "Annotation file path";
            break;
        case BrainTreeMetaItemTypes::SurfaceType:
            sToolTip = "Surface type";
            break;
        case BrainTreeMetaItemTypes::SurfaceColorGyri:
            sToolTip = "Color Gyri";
            break;
        case BrainTreeMetaItemTypes::SurfaceColorSulci:
            sToolTip = "Color Sulci";
            break;
        case BrainTreeMetaItemTypes::SurfaceColorInfoOrigin:
            sToolTip = "Information used to color the surface";
            break;
        case BrainTreeMetaItemTypes::RTDataStreamStatus:
            sToolTip = "Turn real time data streaming on/off";
            break;
        case BrainTreeMetaItemTypes::RTDataSourceSpaceType:
            sToolTip = "The source space type";
            break;
        case BrainTreeMetaItemTypes::RTDataColormapType:
            sToolTip = "The color map type";
            break;
        case BrainTreeMetaItemTypes::RTDataTimeInterval:
            sToolTip = "The m seconds waited in between each sample";
            break;
        case BrainTreeMetaItemTypes::RTDataLoopedStreaming:
            sToolTip = "Turn looped streaming on/off";
            break;
        case BrainTreeMetaItemTypes::RTDataNumberAverages:
            sToolTip = "The number of samples averaged together (downsampling)";
            break;
        case BrainTreeMetaItemTypes::RTDataNormalizationValue:
            sToolTip = "The value to normalize the source localization result";
            break;
        case BrainTreeMetaItemTypes::RTDataVisualizationType:
            sToolTip = "The visualization type";
            break;
        case BrainTreeMetaItemTypes::SurfaceColorItem:
            sToolTip = "Surface color item";
            break;
    }

    this->setToolTip(sToolTip);
}


//*************************************************************************************************************

BrainTreeMetaItem::~BrainTreeMetaItem()
{
}


//*************************************************************************************************************

QVariant BrainTreeMetaItem::data(int role) const
{
    return AbstractTreeItem::data(role);
}


//*************************************************************************************************************

void  BrainTreeMetaItem::setData(const QVariant& value, int role)
{
    AbstractTreeItem::setData(value, role);

    switch(role) {
        case BrainTreeMetaItemRoles::SurfaceColorSulci: {
            emit curvColorsChanged();
            break;
        }

        case BrainTreeMetaItemRoles::SurfaceColorGyri: {
            emit curvColorsChanged();
            break;
        }

        case BrainTreeMetaItemRoles::SurfaceColorInfoOrigin: {
            emit colorInfoOriginChanged();
            break;
        }

        case BrainTreeMetaItemRoles::RTDataTimeInterval: {
            emit rtDataTimeIntervalChanged(value.toInt());
            break;
        }

        case BrainTreeMetaItemRoles::RTDataNormalizationValue: {
            emit rtDataNormalizationValueChanged(value.toDouble());
            break;
        }

        case BrainTreeMetaItemRoles::RTDataColormapType: {
            emit rtDataColormapTypeChanged(value.toString());
            break;
        }

        case BrainTreeMetaItemRoles::RTDataVisualizationType: {
            emit rtDataVisualizationTypeChanged(value.toString());
            break;
        }

        case BrainTreeMetaItemRoles::SurfaceColor: {
            emit surfaceColorChanged(value.value<QColor>());
            break;
        }

        case BrainTreeMetaItemRoles::RTDataNumberAverages: {
            emit rtDataNumberAveragesChanged(value.toInt());
            break;
        }
    }
}

