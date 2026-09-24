//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     tmsielectrodeitem.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     June, 2014
 * @brief    Contains the declaration of the TMSIElectrodeItem class.
 */

#ifndef TMSIELECTRODEITEM_H
#define TMSIELECTRODEITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGraphicsItem>
#include <QString>
#include <QColor>
#include <QPainter>
#include <QStaticText>

//=============================================================================================================
// DEFINE NAMESPACE TMSIPLUGIN
//=============================================================================================================

namespace TMSIPLUGIN
{

//=============================================================================================================
/**
 * TMSIElectrodeItem...
 *
 * @brief The TMSIElectrodeItem class provides a new data structure for impedance values.
 */
class TMSIElectrodeItem : public QGraphicsItem
{

public:
    //=========================================================================================================
    /**
     * Constructs a TMSIElectrodeItem.
     */
    TMSIElectrodeItem(QString electrodeName, QPointF electrodePosition, QColor electrodeColor, int channelIndex);

    //=========================================================================================================
    /**
     * Sets the color of the electrode item.
     */
    void setColor(QColor electrodeColor);

    //=========================================================================================================
    /**
     * Returns the bounding rect of the electrode item. This rect describes the area which the item uses to plot in.
     */
    QRectF boundingRect() const;

    //=========================================================================================================
    /**
     * Reimplemented paint function.
     */
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    //=========================================================================================================
    /**
     * Returns the electrode name.
     */
    QString getElectrodeName();

    //=========================================================================================================
    /**
     * Sets the impedance value.
     */
    void setImpedanceValue(double impedanceValue);

    //=========================================================================================================
    /**
     * Returns the impedance value.
     */
    double getImpedanceValue();

    //=========================================================================================================
    /**
     * Updates the electrodes position.
     */
    void setPosition(QPointF newPosition);

    //=========================================================================================================
    /**
     * Updates the electrodes position.
     */
    QPointF getPosition();

    //=========================================================================================================
    /**
     * Returns the device channel index of the electrode.
     */
    int getChannelIndex();

private:
    QString     m_sElectrodeName;           /**< Holds the electrode name.*/
    QPointF     m_qpElectrodePosition;      /**< Holds the electrode 2D position in the scene.*/
    QColor      m_cElectrodeColor;          /**< Holds the current electrode color.*/
    double      m_dImpedanceValue;          /**< Holds the current electrode impedance value.*/
    int         m_iChannelIndex;            /**< Holds the corresonding channel index.*/
};
} // NAMESPACE

#endif // TMSIELECTRODEITEM_H
