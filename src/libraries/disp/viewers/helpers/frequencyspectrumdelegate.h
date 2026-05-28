//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file frequencyspectrumdelegate.h
 * @since July 2018
 * @brief QAbstractItemDelegate painting one row of the @ref SpectrumView as a horizontal frequency band.
 *
 * FrequencySpectrumDelegate reads the per-row FFT amplitudes from
 * @ref FrequencySpectrumModel, maps them through a Hot / Jet colour
 * table and draws the resulting pixel strip into the row rectangle
 * during @c paint(). It also paints the lower / upper frequency
 * cursors emitted by @ref SpectrumSettingsView.
 */

#ifndef FREQUENCYSPECTRUMDELEGATE_H
#define FREQUENCYSPECTRUMDELEGATE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QAbstractItemDelegate>
#include <QPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QTableView;

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * @brief QAbstractItemDelegate painting one @ref SpectrumView row as a horizontal frequency band.
 *
 * Maps the per-row FFT amplitudes through a Hot / Jet colour table
 * and draws the resulting pixel strip plus the lower / upper
 * frequency cursors during @c paint().
 */
class DISPSHARED_EXPORT FrequencySpectrumDelegate : public QAbstractItemDelegate
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Creates a new abstract item delegate with the given parent.
     *
     * @param[in] parent     Parent of the delegate.
     */
    FrequencySpectrumDelegate(QTableView* m_pTableView,
                              QObject *parent = 0);

    //=========================================================================================================
    /**
     * Set scale type.
     *
     * @param[in] ScaleType.
     */
    void setScaleType(qint8 ScaleType);

    //=========================================================================================================
    /**
     * Use the painter and style option to render the item specified by the item index.
     *
     * (sizeHint() must be implemented also)
     *
     * @param[in] painter    Low-level painting on widgets and other paint devices.
     * @param[in] option     Describes the parameters used to draw an item in a view widget.
     * @param[in] index      Used to locate data in a data model.
     */
    virtual void paint(QPainter *painter,
                       const QStyleOptionViewItem &option,
                       const QModelIndex &index) const;

    //=========================================================================================================
    /**
     * Item size
     *
     * @param[in] option     Describes the parameters used to draw an item in a view widget.
     * @param[in] index      Used to locate data in a data model.
     */
    virtual QSize sizeHint(const QStyleOptionViewItem &option,
                           const QModelIndex &index) const;

    //=========================================================================================================
    /**
     * Receive Mouse location
     *
     * @param[in] row    The select row of tableview.
     * @param[in] x      mouse x pos.
     * @param[in] y      mouse y pos.
     * @param[in] visRect      visual rect of row_tableview.
     *
     */
    void rcvMouseLoc(int row,
                     int x,
                     int y,
                     QRect visRect);

private:
    //=========================================================================================================
    /**
     * CapturePoint capture one QPointer .
     *
     * @param[in]       index   QModelIndex for accessing associated data and model object.
     * @param[in, out]    path    The QPointerPath to create for the data plot.
     */
    void capturePoint(const QModelIndex &index,
                      const QStyleOptionViewItem &option,
                      QPainterPath& path,
                      Eigen::RowVectorXd& data,
                      QPainter *painter) const;

    //=========================================================================================================
    /**
     * createPlotPath creates the QPointer path for the data plot.
     *
     * @param[in]       index   QModelIndex for accessing associated data and model object.
     * @param[in, out]    path    The QPointerPath to create for the data plot.
     */
    void createPlotPath(const QModelIndex &index,
                        const QStyleOptionViewItem &option,
                        QPainterPath& path,
                        Eigen::RowVectorXd& data) const;

    //=========================================================================================================
    /**
     * createGridPath Creates the QPointer path for the grid plot.
     *
     * @param[in, out] path The row vector of the data matrix <1 x nsamples>.
     * @param[in] data The row vector of the data matrix <1 x nsamples>.
     */
    void createGridPath(const QModelIndex &index,
                        const QStyleOptionViewItem &option,
                        QPainterPath& path,
                        Eigen::RowVectorXd& data) const;

    //=========================================================================================================
    /**
     * createGridTick Creates x-axis tickes for the grid plot.
     *
     * Added by LImin Sun; 08.07/2014
     */
    void createGridTick(const QModelIndex &index,
                        const QStyleOptionViewItem &option,
                        QPainter *painter) const;

    QPointer<QTableView>    m_tableview; /**< Pointer to the TableView. */

    int         m_tableview_row;    /**< the selected row of the tableview*/
    int         m_mousex;           /**< the mouse x pos. */
    int         m_mousey;           /**< the mouse y pos. */
    QRect       m_visRect;          /**< visual rect of row of tableview. */
    float       m_x_rate;           /**< the rate of the cursor position in the raw visual rect. */
    float       m_fMaxValue;        /**< Maximum value of the data to plot . */
    float       m_fScaleY;          /**< Maximum amplitude of plot (max is m_dPlotHeight/2). */
    qint8       m_iScaleType;       /**< scale type. */
};
} // NAMESPACE

#endif // FREQUENCYSPECTRUMDELEGATE_H
