//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     neuronalconnectivitysetupwidget.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     October, 2016
 * @brief    Definition of the NeuronalConnectivitySetupWidget class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "neuronalconnectivitysetupwidget.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace NEURONALCONNECTIVITYPLUGIN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NeuronalConnectivitySetupWidget::NeuronalConnectivitySetupWidget(NeuronalConnectivity* toolbox, QWidget *parent)
: QWidget(parent)
, m_pNeuronalConnectivity(toolbox)
{
    ui.setupUi(this);
}

//=============================================================================================================

NeuronalConnectivitySetupWidget::~NeuronalConnectivitySetupWidget()
{
}
