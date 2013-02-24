//=============================================================================================================
/**
* @file     numericwidget.cpp
* @author   Christoph Dinh <christoph.dinh@live.de>;
* @version  1.0
* @date     October, 2010
*
* @section  LICENSE
*
* Copyright (C) 2010 Christoph Dinh. All rights reserved.
*
* No part of this program may be photocopied, reproduced,
* or translated to another program language without the
* prior written consent of the author.
*
*
* @brief    Contains the implementation of the NumericWidget class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "numericwidget.h"
#include <rtMeas/Measurement/numeric.h>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace RTMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NumericWidget::NumericWidget(Numeric* pNumeric, QWidget* parent)
: MeasurementWidget(parent)
, m_pNumeric(pNumeric)

{
    ui.setupUi(this);
}


//*************************************************************************************************************

NumericWidget::~NumericWidget()
{

}


//*************************************************************************************************************

void NumericWidget::update(Subject*)
{
    ui.m_qLcdNumber_Value->display(m_pNumeric->getValue());
}


//*************************************************************************************************************

void NumericWidget::init()
{
    ui.m_qLabel_Caption->setText(m_pNumeric->getName());
    ui.m_qLabel_Unit->setText(m_pNumeric->getUnit());
}
