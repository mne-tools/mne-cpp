//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     multiviewwindow.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     February 2020
 * @brief    Implementation of the MultiViewWindow QDockWidget wrapper.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "multiviewwindow.h"
#include "multiview.h"

//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QGridLayout>
#include <QListView>
#include <QDockWidget>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MultiViewWindow::MultiViewWindow(QWidget *parent,
                                 Qt::WindowFlags flags)
: QDockWidget(parent, flags)
{
    //this->setWidget(parent);
}

//=============================================================================================================

MultiViewWindow::~MultiViewWindow()
{

}
