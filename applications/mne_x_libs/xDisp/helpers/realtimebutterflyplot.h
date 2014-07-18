#ifndef REALTIMEBUTTERFLYPLOT_H
#define REALTIMEBUTTERFLYPLOT_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../xdisp_global.h"

#include "realtimeevokedmodel.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QTimer>
#include <QSharedPointer>


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

protected:
    //=========================================================================================================
    /**
    * Is called to paint the incoming real-time data block.
    * Function is painting the real-time butterfly plot
    *
    * @param [in] event pointer to PaintEvent -> not used.
    */
    virtual void paintEvent( QPaintEvent* event );

private:
    RealTimeEvokedModel* m_pRealTimeEvokedModel;

    QSharedPointer<QTimer> m_pTimerUpdate;

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
