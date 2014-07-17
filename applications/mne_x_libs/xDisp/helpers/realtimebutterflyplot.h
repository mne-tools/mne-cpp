#ifndef REALTIMEBUTTERFLYPLOT_H
#define REALTIMEBUTTERFLYPLOT_H

#include "../xdisp_global.h"

#include "realtimeevokedmodel.h"


#include <QWidget>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE XDISPLIB
//=============================================================================================================

namespace XDISPLIB
{


class XDISPSHARED_EXPORT RealTimeButterflyPlot : public QWidget
{
    Q_OBJECT
public:
    explicit RealTimeButterflyPlot(QWidget *parent = 0);

    inline void setModel(RealTimeEvokedModel* model);

    void dataUpdate(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles = QVector<int>());

signals:

public slots:

private:
    RealTimeEvokedModel* m_pRealTimeEvokedModel;

};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline void RealTimeButterflyPlot::setModel(RealTimeEvokedModel* model)
{
    m_pRealTimeEvokedModel = model;

    connect(m_pRealTimeEvokedModel, &RealTimeEvokedModel::dataChanged, this, &RealTimeButterflyPlot::dataUpdate);
}

} // NAMESPACE

#endif // REALTIMEBUTTERFLYPLOT_H
