#ifndef MNERTCLIENTSETUPFIFFFILESIMULATORWIDGET_H
#define MNERTCLIENTSETUPFIFFFILESIMULATORWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
class MneRtClientSetupFiffFileSimulatorWidget;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MneRtClientPlugin
//=============================================================================================================

namespace MneRtClientPlugin
{


//=============================================================================================================
/**
* DECLARE CLASS MneRtClientSetupFiffFileSimulatorWidget
*
* @brief The MneRtClientSetupFiffFileSimulatorWidget class provides the Fiff File Simulator configuration window.
*/
class MneRtClientSetupFiffFileSimulatorWidget : public QWidget
{
    Q_OBJECT
public:
    typedef QSharedPointer<MneRtClientSetupFiffFileSimulatorWidget> SPtr;              /**< Shared pointer type for MneRtClientSetupFiffFileSimulatorWidget. */
    typedef QSharedPointer<const MneRtClientSetupFiffFileSimulatorWidget> ConstSPtr;   /**< Const shared pointer type for MneRtClientSetupFiffFileSimulatorWidget. */

    explicit MneRtClientSetupFiffFileSimulatorWidget(QWidget *parent = 0);
    ~MneRtClientSetupFiffFileSimulatorWidget();
    
private:
    Ui::MneRtClientSetupFiffFileSimulatorWidget *ui;
};

} // NAMESPACE

#endif // MNERTCLIENTSETUPFIFFFILESIMULATORWIDGET_H
