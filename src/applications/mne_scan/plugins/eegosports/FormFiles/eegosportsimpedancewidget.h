//=============================================================================================================
/**
 * @file     eegosportsimpedancewidget.h
 * @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Johannes Vorwerk <johannes.vorwerk@umit.at>
 * @since    0.1.0
 * @date     February, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch, Christoph Dinh, Matti Hamalainen, Johannes Vorwerk. All rights reserved.
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
 * @brief    Contains the declaration of the EEGoSportsImpedanceWidget class.
 *
 */

#ifndef EEGOSPORTSIMPEDANCEWIDGET_H
#define EEGOSPORTSIMPEDANCEWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/layoutloader.h>
#include "../eegosportselectrodeitem.h"
#include "../eegosportsimpedancescene.h"
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
    class EEGoSportsImpedanceWidget;
}

//=============================================================================================================
// DEFINE NAMESPACE EEGOSPORTSPLUGIN
//=============================================================================================================

namespace EEGOSPORTSPLUGIN
{

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace UTILSLIB;
using namespace DISPLIB;

//=============================================================================================================
// EEGOSPORTSPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

class EEGoSports;

//=============================================================================================================
/**
 * DECLARE CLASS EEGoSportsImpedanceWidget
 *
 * @brief The EEGoSportsImpedanceWidget class provides the EEGoSportsImpedanceWidget configuration window.
 */
class EEGoSportsImpedanceWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EEGoSportsImpedanceWidget(EEGoSports* pEEGoSports, QWidget *parent = 0);
    ~EEGoSportsImpedanceWidget();

    //=========================================================================================================
    /**
    * Updates the values of the electrodes placed in the QGraphicsScene.
    * @param[in] matValue Matrix of electrode positions.
    */
    void updateGraphicScene(const VectorXd& matValue);

    //=========================================================================================================
    /**
    * Initialises the 2D positions of the electrodes in the QGraphicsScene.
    */
    void initGraphicScene();

private:
    //=========================================================================================================
    /**
    * Adds an electrode item to the QGraphicScene.
    * @param[in] electrodeName Name of added electrode.
    * @param[in] position of added electrode (2D).
    */
    void addElectrodeItem(const QString& electrodeName, const QVector2D& position);

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
    * Indicates whether index of Electrode a is larger than index of Electrode b.
    * @param[in] a first electrode.
    * @param[in] b second electrode.
    */
    static bool compareChannelIndex(EEGoSportsElectrodeItem* a, EEGoSportsElectrodeItem* b);

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
    
    EEGoSports*                                 m_pEEGoSports;              /**< The pointer back to the EEGoSports plugin.*/

    EEGoSportsImpedanceScene*                   m_qGScene;                  /**< The QGraphicScene.*/

    QMap< QString, int >                        m_qmElectrodeNameIndex;     /**< Lookup table for electrode name and their corresponding index in the received data matrix.*/

    Ui::EEGoSportsImpedanceWidget*              m_pUi;                      /**< The user interface for the EEGoSportsImpedanceWidget.*/

    QSharedPointer<ColorMap>                    m_cbColorMap;               /**< The pointer the colormap object.*/

    double                                      m_dMaxImpedance;            /**< Maximum impedance value. This is a fixed value to scale the color map.*/
};
} // NAMESPACE

#endif // EEGOSPORTSIMPEDANCEWIDGET_H
