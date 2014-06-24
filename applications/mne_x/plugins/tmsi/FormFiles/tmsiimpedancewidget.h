#ifndef TMSIIMPEDANCEWIDGET_H
#define TMSIIMPEDANCEWIDGET_H

#include <QWidget>

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

private:
    TMSI* m_pTMSI;

    Ui::TmsiImpedanceWidget *ui;
};

} // NAMESPACE

#endif // TMSIIMPEDANCEWIDGET_H
