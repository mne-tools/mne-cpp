//=============================================================================================================
/**
 * @file     graph.h
 * @author   Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     June, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Gabriel B Motta, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    Graph class declaration
 *
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
 * Graph base class
 *
 * @brief Base class for graphs
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
