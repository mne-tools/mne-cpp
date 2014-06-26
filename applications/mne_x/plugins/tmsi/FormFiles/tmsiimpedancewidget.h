#ifndef TMSIIMPEDANCEWIDGET_H
#define TMSIIMPEDANCEWIDGET_H

#include<QWidget>
#include<QGraphicsScene>

#include <utils/asaelc.h>

#include <xMeas/newrealtimemultisamplearray.h>

namespace Ui {
class TmsiImpedanceWidget;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE TMSIPlugin
//=============================================================================================================

namespace TMSIPlugin
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class TMSI;


class TmsiImpedanceWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TmsiImpedanceWidget(TMSI* p_pTMSI, QWidget *parent = 0);
    ~TmsiImpedanceWidget();

    //=========================================================================================================
    /**
    * Updates the values of the electrodes placed in the QGraphicsScene.
    */
    void updateGraphicScene(MatrixXf &matValue);

    //=========================================================================================================
    /**
    * Initialises the 2D positions of the electrodes in the QGraphicsScene.
    */
    void initGraphicScene();

private:
    TMSI*                       m_pTMSI;

    QGraphicsScene              m_scene;

    QMap<QString, QVector2D>    m_qmElectrodePositions;
    QMap<int, QString>          m_qmElectrodeIndex;

    Ui::TmsiImpedanceWidget     *ui;

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

};

} // NAMESPACE

#endif // TMSIIMPEDANCEWIDGET_H
