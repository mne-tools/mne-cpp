//=============================================================================================================
/**
* @file     analyzedata.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Lars Debor <lars.debor@tu-ilmenau.de>;
*           Simon Heinke <simon.heinke@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Christoph Dinh, Lars Debor, Simon Heinke and Matti Hamalainen. All rights reserved.
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
* @brief    Definition of the Analyze Data Container Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "analyzedata.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QVector>
#include <QSharedPointer>
#include <QString>
#include <QtConcurrent/QtConcurrent>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace ANSHAREDLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AnalyzeData::AnalyzeData(QObject *pParent)
: QObject(pParent)
{

}


//*************************************************************************************************************

AnalyzeData::~AnalyzeData()
{
    // @TODO make sure all objects are safely deleted
}


//*************************************************************************************************************

QVector<QSharedPointer<AbstractModel> > AnalyzeData::getObjectsOfType(MODEL_TYPE mtype) const
{
    // simply iterate over map, number of objects in memory should be small enough to ensure acceptable execution time
    QVector<QSharedPointer<AbstractModel> > result;
    QHash<QString, QSharedPointer<AbstractModel> >::const_iterator iter = m_data.cbegin();
    for (; iter != m_data.cend(); iter++)
    {
        if (iter.value()->getType() == mtype)
        {
            result.push_back(iter.value());
        }
    }
    return result;
}


//*************************************************************************************************************

QSharedPointer<AbstractModel> AnalyzeData::getModel(const QString &sName) const
{
    return m_data.value(sName);
}



//*************************************************************************************************************

void AnalyzeData::removeModel(const QString &sModelPath)
{
    int numRemovedModels = m_data.remove(sModelPath);
    if(numRemovedModels > 0) {
        emit modelRemoved(sModelPath);
    }
}


//*************************************************************************************************************

void AnalyzeData::changeModelPath(const QString &sOldModelPath, const QString &sNewModelPath)
{
    QSharedPointer<AbstractModel> pModel = m_data.value(sOldModelPath);
    if(!pModel.isNull() && !m_data.contains(sNewModelPath)) {
        m_data.remove(sOldModelPath);
        m_data.insert(sNewModelPath, pModel);
        pModel->setModelPath(sNewModelPath);
        emit modelPathChanged(pModel, sOldModelPath, sNewModelPath);
    }
    else {
        qDebug() << "[AnalyzeData::changeModelPath] Changing model name from " << sOldModelPath <<
                    " to " << sNewModelPath << " unsuccessful!";
    }
}


//*************************************************************************************************************

QList<QSharedPointer<AbstractModel> > AnalyzeData::getModels() const
{
    return m_data.values();
}


//*************************************************************************************************************
