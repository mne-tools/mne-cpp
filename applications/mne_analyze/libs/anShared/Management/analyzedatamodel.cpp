//=============================================================================================================
/**
 * @file     analyzedatamodel.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.2
 * @date     May, 2019
 *
 * @section  LICENSE
 *
 * Copyright (C) 2019, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the Analyze Data Model Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "analyzedatamodel.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace ANSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AnalyzeDataModel::AnalyzeDataModel(QObject *pParent)
: QStandardItemModel(pParent)
{

}

//=============================================================================================================

AnalyzeDataModel::~AnalyzeDataModel()
{
}

//=============================================================================================================

void AnalyzeDataModel::addData(const QString &sSubjectName,
                               QStandardItem* pNewItem)
{    
    QList<QStandardItem*> pItemList = this->findItems(sSubjectName);
    int iSubjectIndex = 0;
    int iChildModelIndex = 0;

    if(pItemList.isEmpty()) {
        QStandardItem* pSubjectItem = new QStandardItem(sSubjectName);
        pSubjectItem->setToolTip("Subject item");
        pSubjectItem->setChild(pSubjectItem->rowCount(), pNewItem);
        iSubjectIndex = this->rowCount();
        this->appendRow(pSubjectItem);
    } else {
        for(QStandardItem* pItem: pItemList) {
            iChildModelIndex = pItem->rowCount();
            iSubjectIndex = pItem->row();
            pItem->setChild(pItem->rowCount(), pNewItem);

        }
    }
    emit newFileAdded(iSubjectIndex, iChildModelIndex);
}
