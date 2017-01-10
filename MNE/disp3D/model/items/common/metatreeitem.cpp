//=============================================================================================================
/**
* @file     metatreeitem.cpp
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
* @brief    MetaTreeItem class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "metatreeitem.h"


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
#include <QVector3D>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MetaTreeItem::MetaTreeItem(int iType, const QString& text)
: AbstractTreeItem(iType, text)
{
    QString sToolTip;

    switch(m_iType) {
        case MetaTreeItemTypes::FileName:
            sToolTip = "File name";
            break;
        case MetaTreeItemTypes::FilePath:
            sToolTip = "File path";
            break;
        case MetaTreeItemTypes::SurfaceType:
            sToolTip = "Surface type";
            break;
        case MetaTreeItemTypes::SurfaceColorGyri:
            sToolTip = "Color Gyri";
            break;
        case MetaTreeItemTypes::SurfaceColorSulci:
            sToolTip = "Color Sulci";
            break;
        case MetaTreeItemTypes::StreamStatus:
            sToolTip = "Turn real time data streaming on/off";
            break;
        case MetaTreeItemTypes::SourceSpaceType:
            sToolTip = "The source space type";
            break;
        case MetaTreeItemTypes::ColormapType:
            sToolTip = "The color map type";
            break;
        case MetaTreeItemTypes::StreamingTimeInterval:
            sToolTip = "The m seconds waited in between each sample";
            break;
        case MetaTreeItemTypes::LoopedStreaming:
            sToolTip = "Turn looped streaming on/off";
            break;
        case MetaTreeItemTypes::NumberAverages:
            sToolTip = "The number of samples averaged together (downsampling)";
            break;
        case MetaTreeItemTypes::ThresholdValue:
            sToolTip = "The value to normalize the source localization result";
            break;
        case MetaTreeItemTypes::VisualizationType:
            sToolTip = "The visualization type";
            break;
        case MetaTreeItemTypes::SurfaceColor:
            sToolTip = "Surface color item";
            break;
        case MetaTreeItemTypes::PointColor:
            sToolTip = "Point color item";
            break;
        case MetaTreeItemTypes::SurfaceAlpha:
            sToolTip = "Surface alpha value";
            break;
        case MetaTreeItemTypes::SurfaceTessInner:
            sToolTip = "Surface inner tesselation value";
            break;
        case MetaTreeItemTypes::SurfaceTessOuter:
            sToolTip = "Surface outer tesselation value";
            break;
        case MetaTreeItemTypes::SurfaceTriangleScale:
            sToolTip = "Surface triangle scale value";
            break;
        case MetaTreeItemTypes::SurfaceTranslateX:
            sToolTip = "Surface x translation value";
            break;
        case MetaTreeItemTypes::SurfaceTranslateY:
            sToolTip = "Surface y translation value";
            break;
        case MetaTreeItemTypes::SurfaceTranslateZ:
            sToolTip = "Surface z translation value";
            break;
        case MetaTreeItemTypes::NetworkThreshold:
            sToolTip = "The threshold used to scale the network";
            break;
        case MetaTreeItemTypes::NetworkMatrix:
            sToolTip = "The network distance matrix";
            break;
        default: // do nothing;
            break;
    }

    this->setToolTip(sToolTip);
}


//*************************************************************************************************************

MetaTreeItem::~MetaTreeItem()
{
}


//*************************************************************************************************************

QVariant MetaTreeItem::data(int role) const
{
    return AbstractTreeItem::data(role);
}


//*************************************************************************************************************

void  MetaTreeItem::setData(const QVariant& value, int role)
{
    AbstractTreeItem::setData(value, role);

    switch(role) {
        case MetaTreeItemRoles::SurfaceColorSulci: {
            emit curvColorsChanged();
            break;
        }

        case MetaTreeItemRoles::SurfaceColorGyri: {
            emit curvColorsChanged();
            break;
        }

        case MetaTreeItemRoles::StreamingTimeInterval: {
            emit rtDataTimeIntervalChanged(value.toInt());
            break;
        }

        case MetaTreeItemRoles::ThresholdValue: {
            QVector3D vecTemp = value.value<QVector3D>();
            emit rtDataNormalizationValueChanged(vecTemp);
            break;
        }

        case MetaTreeItemRoles::ColormapType: {
            emit rtDataColormapTypeChanged(value.toString());
            break;
        }

        case MetaTreeItemRoles::VisualizationType: {
            emit rtDataVisualizationTypeChanged(value.toString());
            break;
        }

        case MetaTreeItemRoles::SurfaceColor: {
            emit surfaceColorChanged(value.value<QColor>());
            break;
        }

        case MetaTreeItemRoles::PointColor: {
            emit surfaceColorChanged(value.value<QColor>());
            break;
        }

        case MetaTreeItemRoles::NumberAverages: {
            emit rtDataNumberAveragesChanged(value.toInt());
            break;
        }

        case MetaTreeItemRoles::SurfaceAlpha: {
            emit surfaceAlphaChanged(value.toFloat());
            break;
        }

        case MetaTreeItemRoles::SurfaceTessInner: {
            emit surfaceTessInnerChanged(value.toFloat());
            break;
        }

        case MetaTreeItemRoles::SurfaceTessOuter: {
            emit surfaceTessOuterChanged(value.toFloat());
            break;
        }

        case MetaTreeItemRoles::SurfaceTriangleScale: {
            emit surfaceTriangleScaleChanged(value.toFloat());
            break;
        }

        case MetaTreeItemRoles::SurfaceTranslateX: {
            emit surfaceTranslationXChanged(value.toFloat());
            break;
        }

        case MetaTreeItemRoles::SurfaceTranslateY: {
            emit surfaceTranslationYChanged(value.toFloat());
            break;
        }

        case MetaTreeItemRoles::SurfaceTranslateZ: {
            emit surfaceTranslationZChanged(value.toFloat());
            break;
        }

        case MetaTreeItemRoles::NetworkThreshold: {
            QVector3D vecTemp = value.value<QVector3D>();
            emit networkThresholdChanged(vecTemp);
            break;
        }

        default: // do nothing;
            break;
    }
}

