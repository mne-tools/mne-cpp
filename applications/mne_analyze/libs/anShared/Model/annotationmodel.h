//=============================================================================================================
/**
 * @file     annotationmodel.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 *           Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @version  dev
 * @date     March, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Christoph Dinh, Lorenz Esch, Gabriel Motta. All rights reserved.
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
 * @brief    Declaration of the AnnotationModel Class.
 *
 */

#ifndef ANSHAREDLIB_ANNOTATIONMODEL_H
#define ANSHAREDLIB_ANNOTATIONMODEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../anshared_global.h"
#include "../Utils/types.h"
#include "abstractmodel.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QColor>
#include <QAbstractTableModel>

//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//=============================================================================================================
// DEFINE NAMESPACE ANSHAREDLIB
//=============================================================================================================

namespace ANSHAREDLIB {


class ANSHAREDSHARED_EXPORT AnnotationModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    AnnotationModel(QObject* parent = Q_NULLPTR);

    //=========================================================================================================
    bool insertRows(int position, int span, const QModelIndex & parent);
    bool removeRows(int position, int span, const QModelIndex & parent = QModelIndex());
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex & index) const;

    //=========================================================================================================
    QStringList getEventTypeList() const;

    //=========================================================================================================
    void setSamplePos(int iSamplePos);

    //=========================================================================================================
    void setEventFilterType(const QString eventType);

    //=========================================================================================================
    void setFirstLastSample(int firstSample,
                            int lastSample);

    //=========================================================================================================
    QPair<int, int> getFirstLastSample() const;

    //=========================================================================================================
    float getSampleFreq() const;

    //=========================================================================================================
    void setSampleFreq(float fFreq);

    //=========================================================================================================
    int getNumberOfAnnotations() const;

    //=========================================================================================================
    int getAnnotation(int iIndex) const;

    //=========================================================================================================
    QMap<int, QColor>& getTypeColors();

    //=========================================================================================================
    void addNewAnnotationType(const QString &eventType,
                              const QColor &typeColor);

    void setSelectedAnn(int iSelected);

    int getSelectedAnn();

    void setShowSelected(int iSelectedState);

    int getShowSelected();

    float getFreq();

    //=========================================================================================================
    /**
     * Saves model to the current model path if possible.
     *
     * @param[in] sPath   The path where the file should be saved to.
     *
     * @returns      True if saving was successful
     */
    bool saveToFile(const QString& sPath);

    void setLastType(int iType);

signals:

    void updateEventTypes(const QString& currentFilterType);

private:

    QStringList         m_eventTypeList;

    QVector<int>        m_dataSamples;
    QVector<int>        m_dataTypes;
    QVector<int>        m_dataIsUserEvent;

    QVector<int>        m_dataSamples_Filtered;
    QVector<int>        m_dataTypes_Filtered;
    QVector<int>        m_dataIsUserEvent_Filtered;

    int                 m_iSamplePos;
    int                 m_iFirstSample;
    int                 m_iLastSample;

    int                 m_iActiveCheckState;
    int                 m_iSelectedCheckState;
    int                 m_iSelectedAnn;

    int                 m_iLastTypeAdded;

    float               m_fFreq;

    QString             m_sFilterEventType;
    QMap<int, QColor>   m_eventTypeColor;

};
}
#endif // ANSHAREDLIB_ANNOTATIONMODEL_H
