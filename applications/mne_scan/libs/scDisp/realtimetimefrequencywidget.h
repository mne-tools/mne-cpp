#ifndef REALTIMETIMEFREQUENCYWIDGET_H
#define REALTIMETIMEFREQUENCYWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scdisp_global.h"
#include "measurementwidget.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QPointer>
#include <QMap>

//=============================================================================================================
// DEFINE NAMESPACE SCDISPLIB
//=============================================================================================================

namespace SCDISPLIB
{

class SCDISPSHARED_EXPORT RealTimeTimeFrequencyWidget : public MeasurementWidget
{
    Q_OBJECT
public:
    RealTimeTimeFrequencyWidget();
};
}//namespace
#endif // REALTIMETIMEFREQUENCYWIDGET_H
