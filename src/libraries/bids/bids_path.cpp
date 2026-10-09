//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bids_path.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Implementation of @ref BIDSLIB::BIDSPath — entity-based construction and matching of BIDS-compliant paths and sidecar siblings.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_path.h"
#include "bids_const.h"

#include <algorithm>

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

void BIDSPath::setRoot(const QString& sRoot)
{
    m_sRoot = sRoot;
}
void BIDSPath::setSubject(const QString& sSubject)
{
    m_sSubject = sSubject;
}
void BIDSPath::setSession(const QString& sSession)
{
    m_sSession = sSession;
}
void BIDSPath::setTask(const QString& sTask)
{
    m_sTask = sTask;
}
void BIDSPath::setAcquisition(const QString& sAcq)
{
    m_sAcquisition = sAcq;
}
void BIDSPath::setRun(const QString& sRun)
{
    m_sRun = zeroPad(sRun);
}
void BIDSPath::setProcessing(const QString& sProc)
{
    m_sProcessing = sProc;
}
void BIDSPath::setSpace(const QString& sSpace)
{
    m_sSpace = sSpace;
}
void BIDSPath::setRecording(const QString& sRec)
{
    m_sRecording = sRec;
}
void BIDSPath::setSplit(const QString& sSplit)
{
    m_sSplit = zeroPad(sSplit);
}
void BIDSPath::setDescription(const QString& sDesc)
{
    m_sDescription = sDesc;
}
void BIDSPath::setDatatype(const QString& sDatatype)
{
    m_sDatatype = sDatatype;
}
void BIDSPath::setSuffix(const QString& sSuffix)
{
    m_sSuffix = sSuffix;
}
void BIDSPath::setExtension(const QString& sExtension)
{
    m_sExtension = sExtension;
}

//=============================================================================================================
// Getters
//=============================================================================================================

QString BIDSPath::root() const
{
    return m_sRoot;
}
QString BIDSPath::subject() const
{
    return m_sSubject;
}
QString BIDSPath::session() const
{
    return m_sSession;
}
QString BIDSPath::task() const
{
    return m_sTask;
}
QString BIDSPath::acquisition() const
{
    return m_sAcquisition;
}
QString BIDSPath::run() const
{
    return m_sRun;
}
QString BIDSPath::processing() const
{
    return m_sProcessing;
}
QString BIDSPath::space() const
{
    return m_sSpace;
}
QString BIDSPath::recording() const
{
    return m_sRecording;
}
QString BIDSPath::split() const
{
    return m_sSplit;
}
QString BIDSPath::description() const
{
    return m_sDescription;
}
QString BIDSPath::datatype() const
{
    return m_sDatatype;
}
QString BIDSPath::suffix() const
{
    return m_sSuffix;
}
QString BIDSPath::extension() const
{
    return m_sExtension;
}

//=============================================================================================================
// Path construction
//=============================================================================================================

QString BIDSPath::basename() const
{
    QStringList parts;

    // Build ordered entity key-value pairs
    if (!m_sSubject.isEmpty())
        parts << QStringLiteral("sub-") + m_sSubject;
    if (!m_sSession.isEmpty())
        parts << QStringLiteral("ses-") + m_sSession;
    if (!m_sTask.isEmpty())
        parts << QStringLiteral("task-") + m_sTask;
    if (!m_sAcquisition.isEmpty())
        parts << QStringLiteral("acq-") + m_sAcquisition;
    if (!m_sRun.isEmpty())
        parts << QStringLiteral("run-") + m_sRun;
    if (!m_sProcessing.isEmpty())
        parts << QStringLiteral("proc-") + m_sProcessing;
    if (!m_sSpace.isEmpty())
        parts << QStringLiteral("space-") + m_sSpace;
    if (!m_sRecording.isEmpty())
        parts << QStringLiteral("recording-") + m_sRecording;
    if (!m_sSplit.isEmpty())
        parts << QStringLiteral("split-") + m_sSplit;
    if (!m_sDescription.isEmpty())
        parts << QStringLiteral("desc-") + m_sDescription;

    // Append suffix
    if (!m_sSuffix.isEmpty())
        parts << m_sSuffix;

    QString name = parts.join(QStringLiteral("_"));

    // Append extension
    if (!m_sExtension.isEmpty())
        name += m_sExtension;

    return name;
}

//=============================================================================================================

QString BIDSPath::directory() const
{
    QDir dir(m_sRoot);

    if (!m_sSubject.isEmpty())
        dir = QDir(dir.filePath(QStringLiteral("sub-") + m_sSubject));

    if (!m_sSession.isEmpty())
        dir = QDir(dir.filePath(QStringLiteral("ses-") + m_sSession));

    if (!m_sDatatype.isEmpty())
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
    if (dir.exists())
        return true;
    return dir.mkpath(QStringLiteral("."));
}

//=============================================================================================================

QList<BIDSPath> BIDSPath::match() const
{
    // Like mne_bids.BIDSPath.match: every set entity must equal the file's, unset ones match anything
    QList<BIDSPath> results;
    if (m_sRoot.isEmpty() || !QDir(m_sRoot).exists())
        return results;

    QDirIterator it(m_sRoot, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QFileInfo file(it.next());
        const QString name = file.fileName();
        const int dot = name.indexOf(QLatin1Char('.'));
        if (dot <= 0)
            continue;

        BIDSPath p;
        p.setRoot(m_sRoot);
        p.setExtension(name.mid(dot));
        const QString dirName = file.dir().dirName();
        if (!dirName.startsWith(QStringLiteral("sub-")) && !dirName.startsWith(QStringLiteral("ses-")))
            p.setDatatype(dirName);

        bool valid = true;
        const QStringList parts = name.left(dot).split(QLatin1Char('_'));
        for (int i = 0; i < parts.size() && valid; ++i) {
            const int dash = parts[i].indexOf(QLatin1Char('-'));
            if (dash < 0) {
                // Only the last part may be the suffix
                valid = (i == parts.size() - 1);
                p.setSuffix(parts[i]);
                continue;
            }
            const QString key = parts[i].left(dash);
            const QString val = parts[i].mid(dash + 1);
            if (key == QStringLiteral("sub"))
                p.setSubject(val);
            else if (key == QStringLiteral("ses"))
                p.setSession(val);
            else if (key == QStringLiteral("task"))
                p.setTask(val);
            else if (key == QStringLiteral("acq"))
                p.setAcquisition(val);
            else if (key == QStringLiteral("run"))
                p.setRun(val);
            else if (key == QStringLiteral("proc"))
                p.setProcessing(val);
            else if (key == QStringLiteral("space"))
                p.setSpace(val);
            else if (key == QStringLiteral("recording"))
                p.setRecording(val);
            else if (key == QStringLiteral("split"))
                p.setSplit(val);
            else if (key == QStringLiteral("desc"))
                p.setDescription(val);
        }
        if (!valid || p.subject().isEmpty())
            continue;

        const QList<std::pair<QString, QString>> wanted{
            {m_sSubject, p.subject()}, {m_sSession, p.session()}, {m_sTask, p.task()}, {m_sAcquisition, p.acquisition()}, {m_sRun, p.run()}, {m_sProcessing, p.processing()}, {m_sSpace, p.space()}, {m_sRecording, p.recording()}, {m_sSplit, p.split()}, {m_sDescription, p.description()}, {m_sDatatype, p.datatype()}, {m_sSuffix, p.suffix()}, {m_sExtension, p.extension()}};
        if (std::all_of(wanted.cbegin(), wanted.cend(), [](const auto& w) { return w.first.isEmpty() || w.first == w.second; }))
            results.append(p);
    }

    std::sort(results.begin(), results.end(), [](const BIDSPath& a, const BIDSPath& b) { return a.filePath() < b.filePath(); });
    return results;
}

//=============================================================================================================
// Validation
//=============================================================================================================

bool BIDSPath::isValidEntityValue(const QString& sValue)
{
    if (sValue.isEmpty())
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
    if (this != &other) {
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
// Static helpers
//=============================================================================================================

QString BIDSPath::zeroPad(const QString& sValue)
{
    if (sValue.isEmpty())
        return sValue;

    bool ok = false;
    int num = sValue.toInt(&ok);
    if (ok) {
        return QStringLiteral("%1").arg(num, 2, 10, QLatin1Char('0'));
    }
    return sValue;
}
