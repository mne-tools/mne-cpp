//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file graph.h
 * @since 2022
 * @date  October 2022
 * @brief Abstract base widget providing title, axis labels and resize handling for the in-house 2-D plots.
 *
 * Graph centralises the boilerplate shared by @ref Plot, @ref ImageSc
 * and any other small @c QPainter-based plot widget in DISPLIB:
 * storing the plot title and x/y axis label strings, performing the
 * minimal resize bookkeeping that keeps tick spacing readable, and
 * drawing the label band around the actual plot area. Concrete
 * subclasses paint inside the content rectangle that @ref Graph leaves
 * free after the labels and title.
 */

#ifndef GRAPH_H
#define GRAPH_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QPen>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

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
 * @brief Common base widget providing title, axis labels and resize handling for DISPLIB's QPainter plots.
 *
 * Subclasses such as @ref Plot and @ref ImageSc paint inside the
 * content rectangle that @ref Graph leaves free after stamping the
 * title bar at the top and the x / y axis labels on the bottom and
 * left margins.
 */
class DISPSHARED_EXPORT Graph : public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<Graph> SPtr;            /**< Shared pointer type for Graph class. */
    typedef QSharedPointer<const Graph> ConstSPtr; /**< Const shared pointer type for Graph class. */

    //=========================================================================================================
    /**
     * The constructor.
     *
     * @param[in] parent   The parent widget.
     */
    explicit Graph(QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Init this class.
     */
    void init();

    //=========================================================================================================
    /**
     * Sets the scaled image view title.
     *
     * @param[in] p_sTitle   The title.
     */
    void setTitle(const QString &p_sTitle);

    //=========================================================================================================
    /**
     * Sets the label of the y axes
     *
     * @param[in] p_sXLabel   The x axes label.
     */
    void setXLabel(const QString &p_sXLabel);

    //=========================================================================================================
    /**
     * Sets the label of the y axes
     *
     * @param[in] p_sXLabel   The y axes label.
     */
    void setYLabel(const QString &p_sYLabel);

protected:
//    void paintEvent(QPaintEvent*);
    //=========================================================================================================
    /**
     * Reimplemented resizeEvent
     *
     * @param[in] event   The resize event.
     */
    void resizeEvent(QResizeEvent* event);

    //=========================================================================================================
    /**
     * Draw the labels.
     *
     * @param[in] p_iContentWidth    The contents width.
     * @param[in] p_iContentHeight   The contents height.
     */
    void drawLabels(qint32 p_iContentWidth,
                    qint32 p_iContentHeight);

    QSize       m_qSizeWidget;          /**< current widget size. */

    QString     m_sTitle;               /**< Title. */
    QFont       m_qFontTitle;           /**< Title font. */
    QPen        m_qPenTitle;            /**< Title pen. */

    qint32      m_iBorderTopBottom;     /**< distance to top and bottom. */
    qint32      m_iBorderLeftRight;     /**< distance to left and right. */

    QString     m_sXLabel;              /**< X axes label. */
    QString     m_sYLabel;              /**< Y axes label. */
    QFont       m_qFontAxes;            /**< Axes font. */
    QPen        m_qPenAxes;             /**< Axes pen. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE

#endif // GRAPH_H
