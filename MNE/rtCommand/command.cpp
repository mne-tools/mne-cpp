
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

Command::Command(QObject *parent)
    : QObject(parent)
, m_sCommand("")
, m_sDescription("")
{

}


//*************************************************************************************************************

Command::Command(QString &p_sCommand, QJsonObject &p_qCommandDescription)
{
    this->m_sCommand = p_sCommand;
    this->m_sDescription = p_qCommandDescription.value(QString("description")).toString();

    QJsonObject t_jsonObjectParameter = p_qCommandDescription.value(QString("parameters")).toObject();

    QJsonObject::Iterator it;

    for(it = t_jsonObjectParameter.begin(); it != t_jsonObjectParameter.end(); ++it)
    {
        this->m_vecParamDescriptions.push_back(it.value().toObject().value(QString("description")).toString());
        QVariant::Type type(static_cast<QVariant::Type>((int)it.value().toObject().value(QString("typeId")).toDouble()));
        this->m_mapParameters.insert(it.key(), QVariant(type));
//        qDebug() << "Type: " <<  this->m_vecParamValue[this->m_vecParamValue.size()-1].type();
    }
}


//*************************************************************************************************************

Command::Command(   const QString &p_sCommand, const QString &p_sDescription,
                    const QMap<QString, QVariant> &p_mapParameters, const QList<QString> &p_vecParameterDescriptions)
: m_sCommand(p_sCommand)
, m_sDescription(p_sDescription)
{
    if(p_mapParameters.size() == p_vecParameterDescriptions.size())
    {
        m_mapParameters = p_mapParameters;
        m_vecParamDescriptions = p_vecParameterDescriptions;
    }
    else
    {
        printf("error: description vector hasn't the same size like parameter map.\n");
        return;
    }

}

//*************************************************************************************************************

Command::Command(const Command &p_Command)
: m_sCommand(p_Command.m_sCommand)
, m_sDescription(p_Command.m_sDescription)
, m_mapParameters(p_Command.m_mapParameters)
, m_vecParamDescriptions(p_Command.m_vecParamDescriptions)
{

}


//*************************************************************************************************************

Command::~Command()
{
}


//*************************************************************************************************************

QJsonObject Command::toJsonObject() const
{
    QJsonObject p_jsonCommandObject;
    p_jsonCommandObject.insert("description", QJsonValue(m_sDescription));

    QJsonObject t_jsonAllParametersObject;
    for(qint32 i = 0; i < m_mapParameters.size(); ++i)
    {
        QJsonObject t_jsonParameterObject;
        t_jsonParameterObject.insert("description",QJsonValue(m_vecParamDescriptions[i]));
        t_jsonParameterObject.insert("typeId",QJsonValue(static_cast<int> (m_mapParameters.values()[i].type())));
        t_jsonAllParametersObject.insert(m_mapParameters.keys()[i], QJsonValue(t_jsonParameterObject));
    }
    p_jsonCommandObject.insert("parameters", QJsonValue(t_jsonAllParametersObject));

    return p_jsonCommandObject;
}


//*************************************************************************************************************

Command& Command::operator= (const Command &rhs)
{
    if (this != &rhs) // protect against invalid self-assignment
    {
        m_sCommand = rhs.m_sCommand;
        m_sDescription = rhs.m_sDescription;
        m_mapParameters = rhs.m_mapParameters;
        m_vecParamDescriptions = rhs.m_vecParamDescriptions;
    }
    // to support chained assignment operators (a=b=c), always return *this
    return *this;
}


//*************************************************************************************************************

QVariant& Command::operator[] (const QString &key)
{
    return m_mapParameters[key];
}


//*************************************************************************************************************

const QVariant Command::operator[] (const QString &key) const
{
    return m_mapParameters[key];
}
