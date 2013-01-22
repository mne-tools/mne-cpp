
//*************************************************************************************************************
//=============================================================================================================
// Includes
//=============================================================================================================

#include "command.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt Includes
//=============================================================================================================

#include <QVector>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTCOMMANDLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Command::Command()
{

}


//*************************************************************************************************************

Command::Command(const Command &p_Command)
: m_sCommandName(p_Command.m_sCommandName)
, m_sDescription(p_Command.m_sDescription)
, m_vecParamNames(p_Command.m_vecParamNames)
, m_vecParamDescription(p_Command.m_vecParamDescription)
, m_vecParamValue(p_Command.m_vecParamValue)
{

}



//*************************************************************************************************************

Command::~Command()
{
}


//*************************************************************************************************************

Command Command::fromQJsonObject(QString &p_sCommandName, QJsonObject &p_qCommandDescription)
{
    Command p_Command;
    p_Command.m_sCommandName = p_sCommandName;
    p_Command.m_sDescription = p_qCommandDescription.value(QString("description")).toString();

    QJsonObject t_jsonObjectParameter = p_qCommandDescription.value(QString("parameters")).toObject();

    QJsonObject::Iterator it;

    for(it = t_jsonObjectParameter.begin(); it != t_jsonObjectParameter.end(); ++it)
    {
        qDebug() << "Key: " << it.key();
        qDebug() << "Description: " << it.value().toObject().value(QString("description")).toString();
        qDebug() << "Type: " << it.value().toObject().value(QString("type"));
//        m_vecParamNames.insert(it.key());
    }


    qDebug() << "Name: " << p_Command.m_sCommandName;
    qDebug() << "Description: " << p_Command.m_sDescription;


    return p_Command;
}
