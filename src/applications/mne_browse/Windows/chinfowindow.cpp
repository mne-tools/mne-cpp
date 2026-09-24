//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     chinfowindow.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @date     November, 2014
 * @version  2.1.0
 * @brief    Definition of the ChInfoWindow class.
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "chinfowindow.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBROWSE;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ChInfoWindow::ChInfoWindow(QWidget *parent)
: QDockWidget(parent)
, ui(new Ui::ChInfoWindow)
{
    ui->setupUi(this);

    initMVC();
    initTableViews();
}


//*************************************************************************************************************

ChInfoWindow::~ChInfoWindow()
{
}


//*************************************************************************************************************

ChannelInfoModel::SPtr ChInfoWindow::getDataModel()
{
    return m_pChannelInfoModel;
}


//*************************************************************************************************************

void ChInfoWindow::initMVC()
{
    m_pChannelInfoModel = ChannelInfoModel::SPtr(new ChannelInfoModel(this));
}


//*************************************************************************************************************

void ChInfoWindow::initTableViews()
{
    ui->m_tableView_chInfos->setModel(m_pChannelInfoModel.data());
    ui->m_tableView_chInfos->verticalHeader()->setVisible(false);

    connect(m_pChannelInfoModel.data(), &ChannelInfoModel::dataChanged,
            ui->m_tableView_chInfos, &QTableView::resizeColumnsToContents);
}



