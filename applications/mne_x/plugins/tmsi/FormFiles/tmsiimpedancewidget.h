#ifndef TMSIIMPEDANCEWIDGET_H
#define TMSIIMPEDANCEWIDGET_H

#include<QWidget>
#include<QGraphicsScene>

//#include "../tmsi.h"

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

private:
    TMSI* m_pTMSI;

    QGraphicsScene m_scene;

    Ui::TmsiImpedanceWidget *ui;
};

} // NAMESPACE

#endif // TMSIIMPEDANCEWIDGET_H
