//=============================================================================================================
/**
* @file     dataloader.cpp
* @author   Lorenz Esch <lorenzesch@hotmail.com>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     November, 2019
*
* @section  LICENSE
*
* Copyright (C) 2019 Lorenz Esch and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Definition of the DataLoader class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dataloader.h"
#include "FormFiles/dataloadercontrol.h"

#include <anShared/Model/fiffrawviewmodel.h>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DATALOADEREXTENSION;
using namespace ANSHAREDLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DataLoader::DataLoader()
{

}


//*************************************************************************************************************

DataLoader::~DataLoader()
{

}


//*************************************************************************************************************

QSharedPointer<IExtension> DataLoader::clone() const
{
    QSharedPointer<DataLoader> pDataLoaderClone(new DataLoader);
    return pDataLoaderClone;
}


//*************************************************************************************************************

void DataLoader::init()
{
    m_pDataLoaderControl = new DataLoaderControl();

    initGuiConnections();
}


//*************************************************************************************************************

void DataLoader::unload()
{

}


//*************************************************************************************************************

QString DataLoader::getName() const
{
    return "Data Loader";
}


//*************************************************************************************************************

QMenu *DataLoader::getMenu()
{
    return Q_NULLPTR;
}


//*************************************************************************************************************

QDockWidget *DataLoader::getControl()
{
    if(!m_pControl) {
        m_pControl = new QDockWidget(tr("Data Loader"));
        m_pControl->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
        m_pControl->setWidget(m_pDataLoaderControl);
    }

    return m_pControl;
}


//*************************************************************************************************************

QWidget *DataLoader::getView()
{
    return Q_NULLPTR;
}


//*************************************************************************************************************

void DataLoader::handleEvent(QSharedPointer<Event> e)
{
    Q_UNUSED(e);
}


//*************************************************************************************************************

QVector<EVENT_TYPE> DataLoader::getEventSubscriptions(void) const
{
    return QVector<EVENT_TYPE>();
}


//*************************************************************************************************************

void DataLoader::onLoadFiffFilePressed()
{
    //Get the path
    QString filePath = QFileDialog::getOpenFileName(m_pControl,
                                                    tr("Open Fiff File"),
                                                    QDir::currentPath()+"/MNE-sample-data",
                                                    tr("Fiff file(*.fif *.fiff)"));

    if(!filePath.isNull()) {
        m_pAnalyzeData->loadModel<FiffRawViewModel>(filePath);
    }
}


//*************************************************************************************************************

void DataLoader::initGuiConnections()
{
    connect(m_pDataLoaderControl.data(), &DataLoaderControl::loadFiffFile,
            this, &DataLoader::onLoadFiffFilePressed);
}
