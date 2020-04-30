//=============================================================================================================
/**
 * @file     metatreeitem.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     May, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch. All rights reserved.
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "metatreeitem.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector3D>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MetaTreeItem::MetaTreeItem(int iType, const QString& text)
: AbstractTreeItem(iType, text)
{
    initItem();
}

//=============================================================================================================

void MetaTreeItem::initItem()
{
    QString sToolTip;

    switch(m_iType) {
        case MetaTreeItemTypes::FileName:
            sToolTip = "File name";
            break;
        case MetaTreeItemTypes::FilePath:
            sToolTip = "File path";
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
            sToolTip = "The m seconds waited when displaying the data";
            break;
        case MetaTreeItemTypes::LoopedStreaming:
            sToolTip = "Turn looped streaming on/off";
            break;
        case MetaTreeItemTypes::NumberAverages:
            sToolTip = "The number of samples averaged together (downsampling)";
            break;
        case MetaTreeItemTypes::DataThreshold:
            sToolTip = "The threshold used to scale the data";
            break;
        case MetaTreeItemTypes::VisualizationType:
            sToolTip = "The visualization type";
            break;
        case MetaTreeItemTypes::Color:
            sToolTip = "Color item";
            break;
        case MetaTreeItemTypes::AlphaValue:
            sToolTip = "The alpha value";
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
        case MetaTreeItemTypes::TranslateX:
            sToolTip = "x translation value";
            break;
        case MetaTreeItemTypes::TranslateY:
            sToolTip = "y translation value";
            break;
        case MetaTreeItemTypes::TranslateZ:
            sToolTip = "z translation value";
            break;
        case MetaTreeItemTypes::NetworkMatrix:
            sToolTip = "The network distance matrix";
            break;
        case MetaTreeItemTypes::NumberDipoles:
            sToolTip = "The number of dipoles";
            break;
        case MetaTreeItemTypes::MaterialType:
            sToolTip = "The surface material";
            break;
        case MetaTreeItemTypes::Scale:
            sToolTip = "The scaling value";
            break;
        case MetaTreeItemTypes::CancelDistance:
            sToolTip = "The cancel distance";
            break;
        case MetaTreeItemTypes::InterpolationFunction:
            sToolTip = "The function used for interpolation";
            break;
        default: // do nothing;
            break;
    }

    this->setToolTip(sToolTip);
}

//=============================================================================================================

void MetaTreeItem::setData(const QVariant& value, int role)
{
    AbstractTreeItem::setData(value, role);

    if(role >= Qt::UserRole) {
        emit dataChanged(value);
    }
}

