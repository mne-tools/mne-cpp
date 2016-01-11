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

using namespace DISP3DNEWLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainTreeMetaItem::BrainTreeMetaItem(const int& iType, const QString& text)
: AbstractTreeItem(iType, text)
{
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
            emit curvColorsUpdated();
            break;
        }

        case BrainTreeMetaItemRoles::SurfaceColorGyri: {
            emit curvColorsUpdated();
            break;
        }

        case BrainTreeMetaItemRoles::SurfaceColorInfoOrigin: {
            emit colorInfoOriginUpdated();
            break;
        }

        case BrainTreeMetaItemRoles::RTDataTimeInterval: {
            emit rtDataTimeIntervalUpdated(value.toInt());
            break;
        }

        case BrainTreeMetaItemRoles::RTDataNormalizationValue: {
            emit rtDataNormalizationValueUpdated(value.toDouble());
            break;
        }

        case BrainTreeMetaItemRoles::RTDataColormapType: {
            emit rtDataColormapTypeUpdated(value.toString());
            break;
        }

        case BrainTreeMetaItemRoles::RTDataVisualizationType: {
            emit rtDataVisualizationTypeUpdated(value.toString());
            break;
        }

    }
}

