
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
: m_sCommand("")
, m_sDescription("")
{

}


//*************************************************************************************************************

Command::Command(const Command &p_Command)
: m_sCommand(p_Command.m_sCommand)
, m_sDescription(p_Command.m_sDescription)
, m_vecParamNames(p_Command.m_vecParamNames)
, m_vecParamDescriptions(p_Command.m_vecParamDescriptions)
, m_vecParamValue(p_Command.m_vecParamValue)
{

}


//*************************************************************************************************************

Command::~Command()
{
}


//*************************************************************************************************************

Command Command::fromQJsonObject(QString &p_sCommand, QJsonObject &p_qCommandDescription)
{
    Command p_Command;
    p_Command.m_sCommand = p_sCommand;
    p_Command.m_sDescription = p_qCommandDescription.value(QString("description")).toString();

    QJsonObject t_jsonObjectParameter = p_qCommandDescription.value(QString("parameters")).toObject();

    QJsonObject::Iterator it;

    for(it = t_jsonObjectParameter.begin(); it != t_jsonObjectParameter.end(); ++it)
    {
        p_Command.m_vecParamNames.push_back(it.key());
        p_Command.m_vecParamDescriptions.push_back(it.value().toObject().value(QString("description")).toString());
        QVariant::Type type(static_cast<QVariant::Type>((int)it.value().toObject().value(QString("typeId")).toDouble()));
        p_Command.m_vecParamValue.push_back(QVariant(type));

        qDebug() << "Type: " <<  p_Command.m_vecParamValue[p_Command.m_vecParamValue.size()-1].type();
    }

    return p_Command;
}
