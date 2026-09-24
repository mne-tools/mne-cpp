//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     butterflysceneitem.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @date     October, 2014
 * @version  2.1.0
 * @brief    Contains the declaration of the ButterflySceneItem class.
 */

#ifndef BUTTERFLYSCENEITEM_H
#define BUTTERFLYSCENEITEM_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <Eigen/Core>
#include <fiff/fiff.h>
#include "types.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGraphicsItem>
#include <QString>
#include <QColor>
#include <QPainter>
#include <QStaticText>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBROWSE
//=============================================================================================================

namespace MNEBROWSE
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================


//=============================================================================================================
/**
 * ButterflySceneItem...
 *
 * @brief The ButterflySceneItem class provides a new data structure for visualizing averages in a 2D layout.
 */
class ButterflySceneItem : public QGraphicsItem
{

public:
    //=========================================================================================================
    /**
     * Constructs a ButterflySceneItem.
     */
    ButterflySceneItem(QString setName, int setKind = FIFFV_MEG_CH, int setUnit = FIFF_UNIT_T_M, const QList<QColor> &defaultColors = QList<QColor>());

    //=========================================================================================================
    /**
     * Returns the bounding rect of the electrode item. This rect describes the area which the item uses to plot in.
     */
    QRectF boundingRect() const;

    //=========================================================================================================
    /**
     * Reimplemented paint function.
     */
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    //=========================================================================================================
    /**
     * Replace the displayed evoked data with an owned copy.
     *
     * @param evoked the evoked data to draw.
     */
    void setEvokedData(const FIFFLIB::FiffEvoked& evoked);

    //=========================================================================================================
    /**
     * Returns the plot area within boundingRect (excluding axis margins).
     */
    QRectF plotArea() const;

    //=========================================================================================================
    /**
     * Convert a scene X coordinate to a time value in seconds.
     */
    double xToTime(double sceneX) const;

    //=========================================================================================================
    /**
     * Convert a scene Y coordinate to an amplitude value in SI units.
     */
    double yToAmplitude(double sceneY) const;

    QString                 m_sSetName;                 /**< The set name.*/
    fiff_int_t              m_iSetKind;                 /**< The set kind which is to be plotted (MEG or EEG).*/
    fiff_int_t              m_iSetUnit;                 /**< The set unit. Used to determine whether mag or grad channels are to be plotted.*/
    const FiffInfo*         m_pFiffInfo;                /**< The fiff info.*/
    FIFFLIB::FiffEvoked     m_displayEvoked;            /**< Owned evoked data for stable plotting pointers. */

    QList<QColor>           m_cAverageColors;           /**< The current average color.*/
    RowVectorPair           m_lAverageData;             /**< The channels average data which is to be plotted.*/
    QPair<int,int>          m_firstLastSample;          /**< The first and last sample.*/
    QMap<QString,double>    m_scaleMap;                 /**< Map with all channel types and their current scaling value.*/
    bool                    m_bShowGFP = false;         /**< Whether to paint the GFP (Global Field Power) trace. */

    //=========================================================================================================
    /**
     * Update the plot dimensions to fill a given viewport size.
     */
    void setPlotSize(int plotW, int plotH);

    int m_plotWidth  = 800;
    int m_plotHeight = 400;
    static constexpr int kMarginLeft  = 70;
    static constexpr int kMarginRight = 20;
    static constexpr int kMarginTop   = 30;
    static constexpr int kMarginBottom = 40;

protected:
    //=========================================================================================================
    /**
     * Create a plot path and paint the average data
     *
     * @param [in] painter The painter used to plot in this item.
     */
    void paintAveragePaths(QPainter *painter);

    //=========================================================================================================
    /**
     * Paint the GFP (Global Field Power) trace as a filled area.
     */
    void paintGFP(QPainter *painter);

    //=========================================================================================================
    /**
     * Paint the axes, stim line, tick marks, and labels.
     */
    void paintAxes(QPainter *painter);
};

} // NAMESPACE MNEBROWSE

#endif // BUTTERFLYSCENEITEM_H
