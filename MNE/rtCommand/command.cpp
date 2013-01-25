
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
#include <QString>
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

Command::Command(bool p_bIsJson, QObject *parent)
: QObject(parent)
, m_sCommand("")
, m_sDescription("")
, m_bIsJson(p_bIsJson)
{

}


//*************************************************************************************************************

Command::Command(const QString &p_sCommand, const QJsonObject &p_qCommandDescription, bool p_bIsJson, QObject *parent)
: QObject(parent)
, m_bIsJson(p_bIsJson)
{
    this->m_sCommand = p_sCommand;
    this->m_sDescription = p_qCommandDescription.value(QString("description")).toString();

    QJsonObject t_jsonObjectParameter = p_qCommandDescription.value(QString("parameters")).toObject();

    QJsonObject::Iterator it;

    for(it = t_jsonObjectParameter.begin(); it != t_jsonObjectParameter.end(); ++it)
    {
        QJsonValue t_jsonValueType = it.value().toObject().value(QString("type"));
        QVariant::Type t_type = QVariant::nameToType(t_jsonValueType.toString().toLatin1().constData());

        this->m_mapParameters.insert(it.key(), QVariant(t_type));
        this->m_vecParamDescriptions.push_back(it.value().toObject().value(QString("description")).toString());
    }
}


//*************************************************************************************************************

Command::Command(const QString &p_sCommand, const QString &p_sDescription, bool p_bIsJson, QObject *parent)
: QObject(parent)
, m_sCommand(p_sCommand)
, m_sDescription(p_sDescription)
, m_bIsJson(p_bIsJson)
{

}


//*************************************************************************************************************

Command::Command(   const QString &p_sCommand, const QString &p_sDescription,
                    const QMap<QString, QVariant> &p_mapParameters, bool p_bIsJson, QObject *parent)
: QObject(parent)
, m_sCommand(p_sCommand)
, m_sDescription(p_sDescription)
, m_bIsJson(p_bIsJson)
{
    m_mapParameters = p_mapParameters;

    for(qint32 i = 0; i < m_mapParameters.size(); ++i)
        m_vecParamDescriptions.append("");
}


//*************************************************************************************************************

Command::Command(   const QString &p_sCommand, const QString &p_sDescription,
                    const QMap<QString, QVariant> &p_mapParameters, const QList<QString> &p_vecParameterDescriptions, bool p_bIsJson, QObject *parent)
: QObject(parent)
, m_sCommand(p_sCommand)
, m_sDescription(p_sDescription)
, m_bIsJson(p_bIsJson)
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
: QObject(p_Command.parent())
, m_sCommand(p_Command.m_sCommand)
, m_sDescription(p_Command.m_sDescription)
, m_mapParameters(p_Command.m_mapParameters)
, m_vecParamDescriptions(p_Command.m_vecParamDescriptions)
, m_bIsJson(p_Command.m_bIsJson)
{

}


//*************************************************************************************************************

Command::~Command()
{
}


//*************************************************************************************************************

void Command::verify(const Command &p_Command)
{
    qDebug() << "in Verify" << this->m_sCommand << " the other " << p_Command.m_sCommand;
    qDebug() << "Number " << p_Command.m_mapParameters.size() << " the other " << this->m_mapParameters.size();
    if(QString::compare(this->m_sCommand, p_Command.m_sCommand) == 0 && p_Command.m_mapParameters.size() == this->m_mapParameters.size())
    {
        qDebug() << "QString::compared";

        for(qint32 i = 0; i < this->m_mapParameters.size(); ++i)
            if(this->m_mapParameters.values()[i].type() != p_Command.m_mapParameters.values()[i].type())
                return;
        this->m_mapParameters.values() = p_Command.m_mapParameters.values();

        emit this->received();
    }
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

QStringList Command::toStringList() const
{
    QStringList p_stringCommandList;

    p_stringCommandList << m_sCommand;

    QString t_sParameters;
    for(qint32 i = 0; i < m_vecParamDescriptions.size(); ++i)
    {
        t_sParameters.append("[");
        t_sParameters.append(m_vecParamDescriptions[i]);
        t_sParameters.append("]");
    }
    p_stringCommandList << t_sParameters;

    p_stringCommandList << m_sDescription;

    return p_stringCommandList;
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
    if(m_mapParameters.contains(key))
        return m_mapParameters[key];
    else
        return defaultVariant;
}


//*************************************************************************************************************

const QVariant Command::operator[] (const QString &key) const
{
    return m_mapParameters[key];
}
