//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file bids_path.cpp
 * @since 2026
 * @date  April 2026
 * @brief Implementation of @ref BIDSLIB::BIDSPath — entity-based construction and matching of BIDS-compliant paths and sidecar siblings.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_path.h"
#include "bids_const.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFileInfo>
#include <QDirIterator>
#include <QRegularExpression>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BIDSLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BIDSPath::BIDSPath()
{
}

//=============================================================================================================

BIDSPath::BIDSPath(const QString& sRoot,
                   const QString& sSubject,
                   const QString& sSession,
                   const QString& sTask,
                   const QString& sDatatype,
                   const QString& sSuffix,
                   const QString& sExtension)
    : m_sRoot(sRoot)
    , m_sSubject(sSubject)
    , m_sSession(sSession)
    , m_sTask(sTask)
    , m_sDatatype(sDatatype)
    , m_sSuffix(sSuffix)
    , m_sExtension(sExtension)
{
}

//=============================================================================================================

BIDSPath::BIDSPath(const BIDSPath& other)
    : m_sRoot(other.m_sRoot)
    , m_sSubject(other.m_sSubject)
    , m_sSession(other.m_sSession)
    , m_sTask(other.m_sTask)
    , m_sAcquisition(other.m_sAcquisition)
    , m_sRun(other.m_sRun)
    , m_sProcessing(other.m_sProcessing)
    , m_sSpace(other.m_sSpace)
    , m_sRecording(other.m_sRecording)
    , m_sSplit(other.m_sSplit)
    , m_sDescription(other.m_sDescription)
    , m_sDatatype(other.m_sDatatype)
    , m_sSuffix(other.m_sSuffix)
    , m_sExtension(other.m_sExtension)
{
}

//=============================================================================================================

BIDSPath::~BIDSPath()
{
}

//=============================================================================================================
// Setters
//=============================================================================================================

void BIDSPath::setRoot(const QString& sRoot)            { m_sRoot = sRoot; }
void BIDSPath::setSubject(const QString& sSubject)      { m_sSubject = sSubject; }
void BIDSPath::setSession(const QString& sSession)      { m_sSession = sSession; }
void BIDSPath::setTask(const QString& sTask)             { m_sTask = sTask; }
void BIDSPath::setAcquisition(const QString& sAcq)      { m_sAcquisition = sAcq; }
void BIDSPath::setRun(const QString& sRun)               { m_sRun = zeroPad(sRun); }
void BIDSPath::setProcessing(const QString& sProc)       { m_sProcessing = sProc; }
void BIDSPath::setSpace(const QString& sSpace)           { m_sSpace = sSpace; }
void BIDSPath::setRecording(const QString& sRec)         { m_sRecording = sRec; }
void BIDSPath::setSplit(const QString& sSplit)            { m_sSplit = zeroPad(sSplit); }
void BIDSPath::setDescription(const QString& sDesc)      { m_sDescription = sDesc; }
void BIDSPath::setDatatype(const QString& sDatatype)     { m_sDatatype = sDatatype; }
void BIDSPath::setSuffix(const QString& sSuffix)         { m_sSuffix = sSuffix; }
void BIDSPath::setExtension(const QString& sExtension)   { m_sExtension = sExtension; }

//=============================================================================================================
// Getters
//=============================================================================================================

QString BIDSPath::root() const          { return m_sRoot; }
QString BIDSPath::subject() const       { return m_sSubject; }
QString BIDSPath::session() const       { return m_sSession; }
QString BIDSPath::task() const          { return m_sTask; }
QString BIDSPath::acquisition() const   { return m_sAcquisition; }
QString BIDSPath::run() const           { return m_sRun; }
QString BIDSPath::processing() const    { return m_sProcessing; }
QString BIDSPath::space() const         { return m_sSpace; }
QString BIDSPath::recording() const     { return m_sRecording; }
QString BIDSPath::split() const         { return m_sSplit; }
QString BIDSPath::description() const   { return m_sDescription; }
QString BIDSPath::datatype() const      { return m_sDatatype; }
QString BIDSPath::suffix() const        { return m_sSuffix; }
QString BIDSPath::extension() const     { return m_sExtension; }

//=============================================================================================================
// Path construction
//=============================================================================================================

QString BIDSPath::basename() const
{
    QStringList parts;

    // Build ordered entity key-value pairs
    if(!m_sSubject.isEmpty())
        parts << QStringLiteral("sub-") + m_sSubject;
    if(!m_sSession.isEmpty())
        parts << QStringLiteral("ses-") + m_sSession;
    if(!m_sTask.isEmpty())
        parts << QStringLiteral("task-") + m_sTask;
    if(!m_sAcquisition.isEmpty())
        parts << QStringLiteral("acq-") + m_sAcquisition;
    if(!m_sRun.isEmpty())
        parts << QStringLiteral("run-") + m_sRun;
    if(!m_sProcessing.isEmpty())
        parts << QStringLiteral("proc-") + m_sProcessing;
    if(!m_sSpace.isEmpty())
        parts << QStringLiteral("space-") + m_sSpace;
    if(!m_sRecording.isEmpty())
        parts << QStringLiteral("rec-") + m_sRecording;
    if(!m_sSplit.isEmpty())
        parts << QStringLiteral("split-") + m_sSplit;
    if(!m_sDescription.isEmpty())
        parts << QStringLiteral("desc-") + m_sDescription;

    // Append suffix
    if(!m_sSuffix.isEmpty())
        parts << m_sSuffix;

    QString name = parts.join(QStringLiteral("_"));

    // Append extension
    if(!m_sExtension.isEmpty())
        name += m_sExtension;

    return name;
}

//=============================================================================================================

QString BIDSPath::directory() const
{
    QDir dir(m_sRoot);

    if(!m_sSubject.isEmpty())
        dir = QDir(dir.filePath(QStringLiteral("sub-") + m_sSubject));

    if(!m_sSession.isEmpty())
        dir = QDir(dir.filePath(QStringLiteral("ses-") + m_sSession));

    if(!m_sDatatype.isEmpty())
        dir = QDir(dir.filePath(m_sDatatype));

    return dir.path() + QDir::separator();
}

//=============================================================================================================

QString BIDSPath::filePath() const
{
    return QDir(directory()).filePath(basename());
}

//=============================================================================================================
// Convenience methods
//=============================================================================================================

BIDSPath BIDSPath::withSuffix(const QString& sSuffix, const QString& sExtension) const
{
    BIDSPath result(*this);
    result.m_sSuffix = sSuffix;
    result.m_sExtension = sExtension;
    return result;
}

//=============================================================================================================

BIDSPath BIDSPath::channelsTsvPath() const
{
    return withSuffix(QStringLiteral("channels"), QStringLiteral(".tsv"));
}

//=============================================================================================================

BIDSPath BIDSPath::electrodesTsvPath() const
{
    // Electrodes file typically doesn't include task entity
    BIDSPath result(*this);
    result.m_sTask.clear();
    result.m_sRun.clear();
    result.m_sSuffix = QStringLiteral("electrodes");
    result.m_sExtension = QStringLiteral(".tsv");
    return result;
}

//=============================================================================================================

BIDSPath BIDSPath::coordsystemJsonPath() const
{
    // Coordsystem file typically doesn't include task entity
    BIDSPath result(*this);
    result.m_sTask.clear();
    result.m_sRun.clear();
    result.m_sSuffix = QStringLiteral("coordsystem");
    result.m_sExtension = QStringLiteral(".json");
    return result;
}

//=============================================================================================================

BIDSPath BIDSPath::eventsTsvPath() const
{
    return withSuffix(QStringLiteral("events"), QStringLiteral(".tsv"));
}

//=============================================================================================================

BIDSPath BIDSPath::sidecarJsonPath() const
{
    return withSuffix(m_sSuffix.isEmpty() ? m_sDatatype : m_sSuffix,
                      QStringLiteral(".json"));
}

//=============================================================================================================

bool BIDSPath::exists() const
{
    return QFileInfo::exists(filePath());
}

//=============================================================================================================

bool BIDSPath::mkdirs() const
{
    QDir dir(directory());
    if(dir.exists())
        return true;
    return dir.mkpath(QStringLiteral("."));
}

//=============================================================================================================

QList<BIDSPath> BIDSPath::match() const
{
    QList<BIDSPath> results;

    QDir dir(directory());
    if(!dir.exists())
        return results;

    // Build a glob pattern from the set entities
    QString pattern = QStringLiteral("sub-") + (m_sSubject.isEmpty() ? QStringLiteral("*") : m_sSubject);

    if(!m_sSession.isEmpty())
        pattern += QStringLiteral("_ses-") + m_sSession;
    else
        pattern += QStringLiteral("*");

    pattern += QStringLiteral("*"); // match remaining entities

    if(!m_sSuffix.isEmpty())
        pattern += QStringLiteral("_") + m_sSuffix;

    if(!m_sExtension.isEmpty())
        pattern += m_sExtension;
    else
        pattern += QStringLiteral(".*");

    QStringList entries = dir.entryList({pattern}, QDir::Files);

    // Parse matching filenames back into BIDSPath objects
    static const QRegularExpression entityRx(QStringLiteral("(\\w+)-(\\w+)"));
    for(const QString& entry : entries) {
        BIDSPath p;
        p.setRoot(m_sRoot);
        p.setDatatype(m_sDatatype);

        // Extract entities from filename
        auto it = entityRx.globalMatch(entry);
        while(it.hasNext()) {
            auto match = it.next();
            const QString key = match.captured(1);
            const QString val = match.captured(2);

            if(key == QStringLiteral("sub"))          p.setSubject(val);
            else if(key == QStringLiteral("ses"))     p.setSession(val);
            else if(key == QStringLiteral("task"))    p.setTask(val);
            else if(key == QStringLiteral("acq"))     p.setAcquisition(val);
            else if(key == QStringLiteral("run"))     p.setRun(val);
            else if(key == QStringLiteral("proc"))    p.setProcessing(val);
            else if(key == QStringLiteral("space"))   p.setSpace(val);
            else if(key == QStringLiteral("rec"))     p.setRecording(val);
            else if(key == QStringLiteral("split"))   p.setSplit(val);
            else if(key == QStringLiteral("desc"))    p.setDescription(val);
        }

        // Extract suffix and extension
        // The suffix is the last _<word> before the extension
        int dotIdx = entry.lastIndexOf(QLatin1Char('.'));
        if(dotIdx > 0) {
            p.setExtension(entry.mid(dotIdx));
            QString nameWithoutExt = entry.left(dotIdx);
            int lastUnder = nameWithoutExt.lastIndexOf(QLatin1Char('_'));
            if(lastUnder >= 0) {
                p.setSuffix(nameWithoutExt.mid(lastUnder + 1));
            }
        }

        results.append(p);
    }

    return results;
}

//=============================================================================================================
// Validation
//=============================================================================================================

bool BIDSPath::isValidEntityValue(const QString& sValue)
{
    if(sValue.isEmpty())
        return true;
    // Entity values must not contain -, _, or /
    static const QRegularExpression forbidden(QStringLiteral("[\\-_/]"));
    return !sValue.contains(forbidden);
}

//=============================================================================================================
// Operators
//=============================================================================================================

BIDSPath& BIDSPath::operator=(const BIDSPath& other)
{
    if(this != &other) {
        m_sRoot = other.m_sRoot;
        m_sSubject = other.m_sSubject;
        m_sSession = other.m_sSession;
        m_sTask = other.m_sTask;
        m_sAcquisition = other.m_sAcquisition;
        m_sRun = other.m_sRun;
        m_sProcessing = other.m_sProcessing;
        m_sSpace = other.m_sSpace;
        m_sRecording = other.m_sRecording;
        m_sSplit = other.m_sSplit;
        m_sDescription = other.m_sDescription;
        m_sDatatype = other.m_sDatatype;
        m_sSuffix = other.m_sSuffix;
        m_sExtension = other.m_sExtension;
    }
    return *this;
}

//=============================================================================================================

bool operator==(const BIDSPath& a, const BIDSPath& b)
{
    return a.root() == b.root() &&
           a.subject() == b.subject() &&
           a.session() == b.session() &&
           a.task() == b.task() &&
           a.acquisition() == b.acquisition() &&
           a.run() == b.run() &&
           a.processing() == b.processing() &&
           a.space() == b.space() &&
           a.recording() == b.recording() &&
           a.split() == b.split() &&
           a.description() == b.description() &&
           a.datatype() == b.datatype() &&
           a.suffix() == b.suffix() &&
           a.extension() == b.extension();
}

//=============================================================================================================
// Static helpers
//=============================================================================================================

QString BIDSPath::zeroPad(const QString& sValue)
{
    if(sValue.isEmpty())
        return sValue;

    bool ok = false;
    int num = sValue.toInt(&ok);
    if(ok) {
        return QStringLiteral("%1").arg(num, 2, 10, QLatin1Char('0'));
    }
    return sValue;
}
