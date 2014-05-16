#ifndef REALTIMEMULTISAMPLEARRAYDELEGATE_H
#define REALTIMEMULTISAMPLEARRAYDELEGATE_H

#include <QAbstractItemDelegate>

class RealTimeMultiSampleArrayDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    RealTimeMultiSampleArrayDelegate(QObject *parent = 0);

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

    inline float getHeight()
    {
        return m_fPlotHeight;
    }

private:
    //=========================================================================================================
    /**
    * createPlotPath creates the QPointer path for the data plot.
    *
    * @param[in] index QModelIndex for accessing associated data and model object.
    * @param[in,out] path The QPointerPath to create for the data plot.
    */
    void createPlotPath(const QModelIndex &index, QPainterPath& path, QVector<float>& data) const;

    //=========================================================================================================
    /**
    * createGridPath Creates the QPointer path for the grid plot.
    *
    * @param[in,out] path The row vector of the data matrix <1 x nsamples>.
    * @param[in] data The row vector of the data matrix <1 x nsamples>.
    */
    void createGridPath(QPainterPath& path, QList<  QVector<float> >& data) const;

    //Settings
    qint8 m_nhlines;        /**< Number of horizontal lines for the grid plot */
//    QSettings m_qSettings;


    // Plots settings
    float m_fPlotHeight;   /**< The height of the plot */

    // Scaling
    float m_fMaxValue;     /**< Maximum value of the data to plot  */
    float m_fScaleY;       /**< Maximum amplitude of plot (max is m_dPlotHeight/2) */
    float m_fDx;           /**< pixel difference to the next sample*/

};

#endif // REALTIMEMULTISAMPLEARRAYDELEGATE_H
