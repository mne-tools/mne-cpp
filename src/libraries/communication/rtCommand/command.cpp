
//=============================================================================================================
// Includes
//=============================================================================================================

#include "command.h"
#include "commandmanager.h"

//=============================================================================================================
// Qt Includes
//=============================================================================================================

#include <QVector>
#include <QString>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace COMMUNICATIONLIB;

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

//=============================================================================================================

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
        QVariant::Type t_type = QVariant::nameToType(t_jsonValueType.toString().toUtf8().constData());

        this->m_qListParamNames.push_back(it.key());
        this->m_qListParamValues.push_back(QVariant(t_type));
        this->m_qListParamDescriptions.push_back(it.value().toObject().value(QString("description")).toString());
    }
}

//=============================================================================================================

Command::Command(const QString &p_sCommand, const QString &p_sDescription, bool p_bIsJson, QObject *parent)
: QObject(parent)
, m_sCommand(p_sCommand)
, m_sDescription(p_sDescription)
, m_bIsJson(p_bIsJson)
{
}

//=============================================================================================================

Command::Command(   const QString &p_sCommand, const QString &p_sDescription,
                    const QStringList &p_qListParamNames, const QList<QVariant> &p_qListParamValues, bool p_bIsJson, QObject *parent)
: QObject(parent)
, m_sCommand(p_sCommand)
, m_sDescription(p_sDescription)
, m_bIsJson(p_bIsJson)
{
    m_qListParamNames = p_qListParamNames;
    m_qListParamValues = p_qListParamValues;

    for(qint32 i = 0; i < p_qListParamValues.size(); ++i)
        m_qListParamDescriptions.append("");
}

//=============================================================================================================

Command::Command(   const QString &p_sCommand, const QString &p_sDescription,
                    const QStringList &p_qListParamNames, const QList<QVariant> &p_qListParamValues, const QStringList &p_vecParameterDescriptions, bool p_bIsJson, QObject *parent)
: QObject(parent)
, m_sCommand(p_sCommand)
, m_sDescription(p_sDescription)
, m_bIsJson(p_bIsJson)
{
    if(p_qListParamNames.size() == p_qListParamValues.size())
    {
        if(p_qListParamValues.size() == p_vecParameterDescriptions.size())
        {
            m_qListParamNames = p_qListParamNames;
            m_qListParamValues = p_qListParamValues;
            m_qListParamDescriptions = p_vecParameterDescriptions;
        }
    }
    else
    {
        printf("error: description vector hasn't the same size like parameter map.\n");
        return;
    }
}

//=============================================================================================================

Command::Command(const Command &p_Command)
: QObject(p_Command.parent())
, m_sCommand(p_Command.m_sCommand)
, m_sDescription(p_Command.m_sDescription)
, m_qListParamNames(p_Command.m_qListParamNames)
, m_qListParamValues(p_Command.m_qListParamValues)
, m_qListParamDescriptions(p_Command.m_qListParamDescriptions)
, m_bIsJson(p_Command.m_bIsJson)
{
}

//=============================================================================================================

Command::~Command()
{
}

//=============================================================================================================

void Command::execute()
{
    emit this->executed(*this);
}

//=============================================================================================================

void Command::reply(const QString &p_sReply)
{
    CommandManager* t_commandManager = static_cast<CommandManager*> (this->parent());

    if(t_commandManager) {
        emit t_commandManager->response(p_sReply, *this);
    }
}

//=============================================================================================================

void Command::send()
{
    CommandManager* t_commandManager = static_cast<CommandManager*> (this->parent());

    if(t_commandManager) {
        emit t_commandManager->triggered(*this);
    }
}

//=============================================================================================================

QJsonObject Command::toJsonObject() const
{
    QJsonObject p_jsonCommandObject;
    p_jsonCommandObject.insert("description", QJsonValue(m_sDescription));

    QJsonObject t_jsonAllParametersObject;
    for(qint32 i = 0; i < m_qListParamValues.size(); ++i)
    {
        QJsonObject t_jsonParameterObject;
        t_jsonParameterObject.insert("description",QJsonValue(m_qListParamDescriptions[i]));
        t_jsonParameterObject.insert("type",QString(m_qListParamValues[i].typeName()));
        t_jsonAllParametersObject.insert(m_qListParamNames[i], QJsonValue(t_jsonParameterObject));
    }
    p_jsonCommandObject.insert("parameters", QJsonValue(t_jsonAllParametersObject));

    return p_jsonCommandObject;
}

//=============================================================================================================

QStringList Command::toStringList() const
{
    QStringList p_stringCommandList;

    p_stringCommandList << m_sCommand;

    QString t_sParameters;
    for(qint32 i = 0; i < m_qListParamDescriptions.size(); ++i)
    {
        t_sParameters.append("[");
        t_sParameters.append(m_qListParamDescriptions[i]);
        t_sParameters.append("]");
    }
    p_stringCommandList << t_sParameters;

    p_stringCommandList << m_sDescription;

    return p_stringCommandList;
}

//=============================================================================================================

QString Command::toStringReadySend() const
{
    QString p_stringCommand;

    QString t_sParameters;
    for(qint32 i = 0; i < m_qListParamNames.size(); ++i)
    {
//        qDebug() << m_qListParamValues[i];
        t_sParameters.append(QString("\"%1\":\"%2\"").arg(m_qListParamNames[i]).arg(m_qListParamValues[i].toString()));

        if(i < m_qListParamNames.size()-1)
            t_sParameters.append(",");
    }

    p_stringCommand.append(QString("\"%1\":{%2}").arg(m_sCommand).arg(t_sParameters));

    return p_stringCommand;
}

//=============================================================================================================

Command& Command::operator= (const Command &rhs)
{
    if (this != &rhs) // protect against invalid self-assignment
    {
        m_sCommand = rhs.m_sCommand;
        m_sDescription = rhs.m_sDescription;
        m_qListParamNames = rhs.m_qListParamNames;
        m_qListParamValues = rhs.m_qListParamValues;
        m_qListParamDescriptions = rhs.m_qListParamDescriptions;
    }
    // to support chained assignment operators (a=b=c), always return *this
    return *this;
}

//=============================================================================================================

QVariant& Command::operator[] (const QString &key)
{
    if(m_qListParamNames.contains(key))
        return m_qListParamValues[m_qListParamNames.indexOf(key)];
    else
        return defaultVariant;
}

//=============================================================================================================

QVariant& Command::operator[] (qint32 idx)
{
    if(m_qListParamValues.size() > idx)
        return m_qListParamValues[idx];
    else
        return defaultVariant;
}

//=============================================================================================================

const QVariant Command::operator[] (const QString &key) const
{
    if(m_qListParamNames.contains(key))
        return m_qListParamValues[m_qListParamNames.indexOf(key)];
    else
        return defaultVariant;
}
