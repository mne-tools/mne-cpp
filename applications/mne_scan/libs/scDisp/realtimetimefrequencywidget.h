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
// FORWARD DECLARATIONS
//=============================================================================================================

class QTime;

namespace SCMEASLIB {
    class RealTimeTimeFrequency;
}

namespace DISPLIB {
    class TimeFrequencyView;
    class TimeFrequencySettingsView;
}

class QVBoxLayout;

//=============================================================================================================
// DEFINE NAMESPACE SCDISPLIB
//=============================================================================================================

namespace SCDISPLIB
{

class SCDISPSHARED_EXPORT RealTimeTimeFrequencyWidget : public MeasurementWidget
{
    Q_OBJECT
public:
    RealTimeTimeFrequencyWidget(QSharedPointer<QTime> &pTime,
                                QWidget* parent = 0);

    //=========================================================================================================
    /**
     * Initialise the MeasurementWidget.
     */
    virtual void init(){}

    //=========================================================================================================
    /**
     * Is called when new data are available.
     *
     * @param [in] pMeasurement  pointer to measurement -> not used because its direct attached to the measurement.
     */
    virtual void update(SCMEASLIB::Measurement::SPtr pMeasurement);

private:

    QSharedPointer<SCMEASLIB::RealTimeTimeFrequency>        m_pRTTF;

    QPointer<DISPLIB::TimeFrequencyView>                    m_pTFView;

    QPointer<QVBoxLayout>                                   m_pRTTFLayout;            /**< RTE Widget layout */



};
}//namespace
#endif // REALTIMETIMEFREQUENCYWIDGET_H
