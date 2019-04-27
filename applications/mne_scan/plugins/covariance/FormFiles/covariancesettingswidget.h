#ifndef COVARIANCESETTINGSWIDGET_H
#define COVARIANCESETTINGSWIDGET_H

//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QSpinBox>
#include <QPair>

#include <QComboBox>
#include <QCheckBox>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE COVARIANCEPLUGIN
//=============================================================================================================

namespace COVARIANCEPLUGIN
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class Covariance;


class CovarianceSettingsWidget : public QWidget
{
    Q_OBJECT

    friend class Covariance;

public:
    typedef QSharedPointer<CovarianceSettingsWidget> SPtr;         /**< Shared pointer type for CovarianceAdjustmentWidget. */
    typedef QSharedPointer<CovarianceSettingsWidget> ConstSPtr;    /**< Const shared pointer type for CovarianceAdjustmentWidget. */

    explicit CovarianceSettingsWidget(Covariance *toolbox, QWidget *parent = 0);

signals:

public slots:

private:
    Covariance* m_pCovarianceToolbox;
    QSpinBox* m_pSpinBoxNumSamples;
};

} // NAMESPACE

#endif // COVARIANCESETTINGSWIDGET_H
