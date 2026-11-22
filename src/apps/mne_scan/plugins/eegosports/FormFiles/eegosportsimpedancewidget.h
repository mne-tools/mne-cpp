//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     eegosportsimpedancewidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Johannes Vorwerk <johannes.vorwerk@umit.at>
 * @since    0.1.0
 * @date     July, 2014
 * @brief    Contains the declaration of the EEGoSportsImpedanceWidget class.
 */

#ifndef EEGOSPORTSIMPEDANCEWIDGET_H
#define EEGOSPORTSIMPEDANCEWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../eegosports_global.h"
#include "../eegosportselectrodeitem.h"
#include "../eegosportsimpedancescene.h"
#include "mne/disp/plots/helpers/colormap.h"
#include <mne/channels/layoutloader.h>
#include <scMeas/realtimemultisamplearray.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMutex>
#include <QWidget>
#include <QGraphicsScene>
#include <QtAlgorithms>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui
{
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
using namespace mne::disp;

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
class EEGOSPORTSSHARED_EXPORT EEGoSportsImpedanceWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EEGoSportsImpedanceWidget(EEGoSports* pEEGoSports, QWidget* parent = nullptr);
    ~EEGoSportsImpedanceWidget() override;

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
    * Adds every electrode with a 2-D position of a cap to the QGraphicScene and to the name lookup table.
    * @param[in] layout The cap.
    */
    void addElectrodeItems(const mne::ElectrodeLayout& layout);

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
    void closeEvent(QCloseEvent* event) override;

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

    EEGoSports* m_pEEGoSports; /**< The pointer back to the EEGoSports plugin.*/

    EEGoSportsImpedanceScene* m_qGScene; /**< The QGraphicScene.*/

    QMap<QString, int> m_qmElectrodeNameIndex; /**< Lookup table for electrode name and their corresponding index in the received data matrix.*/

    std::unique_ptr<Ui::EEGoSportsImpedanceWidget> m_pUi; /**< The user interface for the EEGoSportsImpedanceWidget.*/

    std::shared_ptr<ColorMap> m_cbColorMap; /**< The pointer the colormap object.*/

    double m_dMaxImpedance; /**< Maximum impedance value. This is a fixed value to scale the color map.*/
};
} // NAMESPACE

#endif // EEGOSPORTSIMPEDANCEWIDGET_H
