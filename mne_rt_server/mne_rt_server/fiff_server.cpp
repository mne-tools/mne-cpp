//=============================================================================================================
/**
* @file     fiff_server.cpp
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
* @brief    Contains the implementation of the ModuleManager class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_server.h"
#include "fortunethread.h"


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <stdlib.h>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MSERVER;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffServer::FiffServer(QObject *parent)
    : QTcpServer(parent)
{
    fortunes << tr("You've been leading a dog's life. Stay off the furniture.")
             << tr("You've got to think about tomorrow.")
             << tr("You will be surprised by a loud noise.")
             << tr("You will feel hungry again in another hour.")
             << tr("You might have mail.")
             << tr("You cannot kill time without injuring eternity.")
             << tr("Computers are not intelligent. They only think they are.");
}


//*************************************************************************************************************

void FiffServer::incomingConnection(qintptr socketDescriptor)
{

    QString fortune = fortunes.at(qrand() % fortunes.size());

    FortuneThread *thread = new FortuneThread(socketDescriptor, fortune, this);

    //when thread has finished it gets deleted
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();
}
