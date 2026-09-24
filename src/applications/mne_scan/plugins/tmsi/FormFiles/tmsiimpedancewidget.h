//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     tmsiimpedancewidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     June, 2014
 * @brief    Contains the declaration of the TmsiImpedanceWidget class.
 */

#ifndef TMSIIMPEDANCEWIDGET_H
#define TMSIIMPEDANCEWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/layoutloader.h>
#include "../tmsielectrodeitem.h"
#include "../tmsiimpedancescene.h"
#include "disp/plots/helpers/colormap.h"

#include <scMeas/realtimemultisamplearray.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMutex>
#include <QWidget>
#include <QGraphicsScene>
#include <QtAlgorithms>
#include <QtSvg/QSvgGenerator>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
    class TMSIImpedanceWidget;
}

//=============================================================================================================
// DEFINE NAMESPACE TMSIPLUGIN
//=============================================================================================================

namespace TMSIPLUGIN
{

//=============================================================================================================
// TMSIPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

class TMSI;

//=============================================================================================================
/**
 * DECLARE CLASS TMSIImpedanceWidget
 *
 * @brief The TMSIImpedanceWidget class provides the TMSIImpedanceWidget configuration window.
 */
class TMSIImpedanceWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TMSIImpedanceWidget(TMSI* pTMSI, QWidget *parent = 0);
    ~TMSIImpedanceWidget();

    //=========================================================================================================
    /**
     * Updates the values of the electrodes placed in the QGraphicsScene.
     */
    void updateGraphicScene(Eigen::VectorXd matValue);

    //=========================================================================================================
    /**
     * Initialises the 2D positions of the electrodes in the QGraphicsScene.
     */
    void initGraphicScene();

private:
    TMSI*                                       m_pTMSI;                    /**< The pointer back to the TMSI plugin.*/

    TMSIImpedanceScene*                         m_qGScene;                  /**< The QGraphicScene.*/

    QMap< QString, int >                        m_qmElectrodeNameIndex;     /**< Lookup table for electrode name and their corresponding index in the received data matrix.*/

    Ui::TMSIImpedanceWidget*                    ui;                         /**< The user interface for the TMSIImpedanceWidget.*/

    QSharedPointer<DISPLIB::ColorMap>           m_cbColorMap;               /**< The pointer the colormap object.*/

    double                                      m_dMaxImpedance;            /**< Maximum impedance value. This is a fixed value to scale the color map.*/

    //=========================================================================================================
    /**
     * Adds an electrode item to the QGraphicScene.
     */
    void addElectrodeItem(QString electrodeName, QVector2D position);

    //=========================================================================================================
    /**
     * Start the measurement process.
     */
    void startImpedanceMeasurement();

    //=========================================================================================================
    /**
     * Stops the measurement process.
     */
    void stopImpedanceMeasurement();

    //=========================================================================================================
    /**
     * Takes a screenshot of the current view.
     */
    void takeScreenshot();

    //=========================================================================================================
    /**
     * Loads a layout from file.
     */
    void loadLayout();

    //=========================================================================================================
    /**
     * Reimplemnted closing event handler. Used to stop the measurement when closing the widget.
     */
    void closeEvent(QCloseEvent *event);

    //=========================================================================================================
    /**
     * Saves the current labels and impedance values to a ASI formated file.
     */
    void saveToFile();

    //=========================================================================================================
    /**
     * Open a help dialog.
     */
    void helpDialog();
};
} // NAMESPACE

#endif // TMSIIMPEDANCEWIDGET_H
