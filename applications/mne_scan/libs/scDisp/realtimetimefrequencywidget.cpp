//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimetimefrequencywidget.h"

#include <scMeas/realtimetimefrequency.h>

#include <disp/viewers/timefrequencyview.h>
#include <disp/viewers/timefrequencysettingsview.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVBoxLayout>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCDISPLIB;
using namespace SCMEASLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeTimeFrequencyWidget::RealTimeTimeFrequencyWidget(QSharedPointer<QTime> &pTime,
                                                         QWidget* parent)
: MeasurementWidget(parent)
{
    m_pRTTFLayout = new QVBoxLayout(this);

    m_pTFView = new DISPLIB::TimeFrequencyView();

    m_pRTTFLayout->addWidget(m_pTFView);

    this->setLayout(m_pRTTFLayout);
}

//=============================================================================================================

void RealTimeTimeFrequencyWidget::update(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    if(!m_pRTTF) {
        m_pRTTF = qSharedPointerDynamicCast<RealTimeTimeFrequency>(pMeasurement);
    }
}
