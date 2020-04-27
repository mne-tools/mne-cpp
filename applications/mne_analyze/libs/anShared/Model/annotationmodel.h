#ifndef ANSHAREDLIB_ANNOTATIONMODEL_H
#define ANSHAREDLIB_ANNOTATIONMODEL_H

//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include "../anshared_global.h"
#include "../Utils/types.h"
#include "abstractmodel.h"
#include <QColor>
#include <QAbstractTableModel>

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

    float               m_fFreq;

    QString             m_sFilterEventType;
    QMap<int, QColor>   m_eventTypeColor;

};
}
#endif // ANSHAREDLIB_ANNOTATIONMODEL_H
