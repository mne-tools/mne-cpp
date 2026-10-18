//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *

 * @file     electrodes.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @brief    ElectrodesPlugin implementation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "electrodes.h"

#include <ieeg/ieeg_implantation.h>
#include <mri/mri_vol_data.h>

#include <disp3D/scene/multimodalscene.h>

#include <fiff/fiff_constants.h>
#include <fiff/fiff_dig_point.h>
#include <fiff/fiff_dig_point_set.h>
#include <fiff/fiff_stream.h>

#include <Eigen/LU>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QStringList>
#include <QTextStream>

#include <algorithm>
#include <cmath>
#include <limits>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace ELECTRODESPLUGIN;
using namespace DISP3DLIB;
using namespace FIFFLIB;

namespace
{

IEEGLIB::IeegLeadType parseLeadType(const QString& raw)
{
    const QString s = raw.trimmed().toLower();
    if (s == QLatin1String("strip"))
        return IEEGLIB::IeegLeadType::Strip;
    if (s == QLatin1String("grid"))
        return IEEGLIB::IeegLeadType::Grid;
    return IEEGLIB::IeegLeadType::Depth;
}

} // anonymous namespace

//=============================================================================================================

ElectrodesPlugin::ElectrodesPlugin(QObject* parent)
: QObject(parent)
, m_object(std::make_unique<ElectrodeObject>())
{
}

//=============================================================================================================

ElectrodesPlugin::~ElectrodesPlugin() = default;

//=============================================================================================================

int ElectrodesPlugin::leadCount() const
{
    return int(m_object->implantation().leads().size());
}

//=============================================================================================================

int ElectrodesPlugin::contactCount() const
{
    return m_object->totalContactCount();
}

//=============================================================================================================

QString ElectrodesPlugin::selectedContact() const
{
    return m_object->selectedContact();
}

//=============================================================================================================

void ElectrodesPlugin::selectContact(const QString& name)
{
    if (name.isEmpty()) {
        clearSelection();
        return;
    }
    m_object->selectContact(name);
    emit contactPicked(name);
}

//=============================================================================================================

QString ElectrodesPlugin::highlightNearest(const QVector3D& worldPosition)
{
    const Eigen::Vector3f target(worldPosition.x(), worldPosition.y(), worldPosition.z());
    QString bestName;
    float bestDistSq = std::numeric_limits<float>::max();
    for (const IEEGLIB::IeegContact& c : m_object->implantation().contacts()) {
        const float d = (c.position - target).squaredNorm();
        if (d < bestDistSq) {
            bestDistSq = d;
            bestName = c.name;
        }
    }
    if (bestName.isEmpty()) {
        return QString();
    }
    m_object->selectContact(bestName);
    emit contactPicked(bestName);
    return bestName;
}

//=============================================================================================================

void ElectrodesPlugin::clearSelection()
{
    m_object->clearSelection();
    emit contactPicked(QString());
}

//=============================================================================================================

void ElectrodesPlugin::setImplantation(const IEEGLIB::IeegImplantation& implantation)
{
    rebuild(implantation);
    m_source = ElectrodeSource::None;
    m_sourcePath.clear();
}

//=============================================================================================================

bool ElectrodesPlugin::loadFiff(const QString& path)
{
    QFile file(path);
    if (!file.exists()) {
        qWarning() << "ElectrodesPlugin::loadFiff: file does not exist:" << path;
        return false;
    }

    FiffDigPointSet dig;
    {
        FiffStream::SPtr stream = FiffStream::SPtr(new FiffStream(&file));
        if (!stream->open()) {
            qWarning() << "ElectrodesPlugin::loadFiff: failed to open FIFF stream:" << path;
            return false;
        }
        if (!FiffDigPointSet::readFromStream(stream, dig)) {
            qWarning() << "ElectrodesPlugin::loadFiff: no digitiser data in" << path;
            return false;
        }
    }

    const QList<int> keepKinds{FIFFV_POINT_EEG, FIFFV_POINT_EXTRA};
    const FiffDigPointSet picked = dig.pickTypes(keepKinds);

    if (picked.isEmpty()) {
        qWarning() << "ElectrodesPlugin::loadFiff: no EEG/EXTRA points in" << path;
        return false;
    }

    // All points form one lead named after the file
    const QString lead = QFileInfo(path).baseName();
    QVector<IEEGLIB::IeegContact> contacts;
    contacts.reserve(picked.size());
    for (qint32 i = 0; i < picked.size(); ++i) {
        const FiffDigPoint& dp = picked[i];
        IEEGLIB::IeegContact contact;
        contact.name = QStringLiteral("%1_%2").arg(lead).arg(dp.ident);
        contact.position = Eigen::Vector3f(dp.r[0], dp.r[1], dp.r[2]);
        contact.lead = lead;
        contacts.append(contact);
    }

    rebuild(IEEGLIB::IeegImplantation(contacts));
    m_source = ElectrodeSource::Fiff;
    m_sourcePath = path;
    return true;
}

//=============================================================================================================

bool ElectrodesPlugin::loadCsv(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "ElectrodesPlugin::loadCsv: cannot open" << path;
        return false;
    }

    QTextStream in(&file);
    const QString fallbackLabel = QFileInfo(path).baseName();

    QVector<IEEGLIB::IeegContact> contacts;
    QHash<QString, IEEGLIB::IeegLeadType> declared;
    QHash<QString, int> leadSize;

    while (!in.atEnd()) {
        const QString rawLine = in.readLine().trimmed();
        if (rawLine.isEmpty() || rawLine.startsWith(QLatin1Char('#'))) {
            continue;
        }

        const QStringList parts = rawLine.split(QLatin1Char(','), Qt::KeepEmptyParts);
        if (parts.size() < 4) {
            continue;
        }

        // Skip header row if first numeric field is not parseable.
        bool ok = false;
        const float x = parts[1].trimmed().toFloat(&ok);
        if (!ok) {
            continue;
        }
        const float y = parts[2].trimmed().toFloat(&ok);
        if (!ok) {
            continue;
        }
        const float z = parts[3].trimmed().toFloat(&ok);
        if (!ok) {
            continue;
        }

        const QString name = parts[0].trimmed();
        const QString lead = (parts.size() >= 5 && !parts[4].trimmed().isEmpty())
            ? parts[4].trimmed()
            : fallbackLabel;
        // The first non-depth declaration for a lead wins
        if (parts.size() >= 6) {
            const IEEGLIB::IeegLeadType type = parseLeadType(parts[5]);
            if (!declared.contains(lead) || declared.value(lead) == IEEGLIB::IeegLeadType::Depth)
                declared.insert(lead, type);
        }

        IEEGLIB::IeegContact contact;
        contact.name = name.isEmpty() ? QStringLiteral("%1_%2").arg(lead).arg(leadSize.value(lead)) : name;
        contact.position = Eigen::Vector3f(x, y, z);
        contact.lead = lead;
        contacts.append(contact);
        leadSize[lead] += 1;
    }

    if (contacts.isEmpty()) {
        qWarning() << "ElectrodesPlugin::loadCsv: no contact rows parsed from" << path;
        return false;
    }

    IEEGLIB::IeegImplantation implantation(contacts);
    for (auto it = declared.cbegin(); it != declared.cend(); ++it)
        implantation.setLeadType(it.key(), it.value());
    rebuild(implantation);
    m_source = ElectrodeSource::Csv;
    m_sourcePath = path;
    return true;
}

//=============================================================================================================

bool ElectrodesPlugin::loadBids(const QString& path, const QString& t1Path)
{
    m_lastError.clear();
    IEEGLIB::IeegImplantation implantation = IEEGLIB::IeegImplantation::readBids(path, &m_lastError);
    if (implantation.isEmpty()) {
        qWarning() << "ElectrodesPlugin::loadBids:" << m_lastError;
        return false;
    }

    // Scanner RAS of the T1 -> surface (tkr) RAS of the FreeSurfer surfaces, as MNE's convert_montage_to_mri
    if (implantation.isScannerRas()) {
        MRILIB::MriVolData t1;
        if (t1Path.isEmpty() || !t1.read(t1Path)) {
            m_lastError = QStringLiteral("%1 positions are scanner RAS of the subject's T1; T1.mgz is needed to place them on the surfaces")
                              .arg(implantation.coordinateSystem());
            qWarning() << "ElectrodesPlugin::loadBids:" << m_lastError;
            return false;
        }
        implantation = implantation.transformed(t1.computeVox2RasTkr() * t1.computeVox2Ras().inverse(), QStringLiteral("surface RAS"));
    }

    rebuild(implantation);
    m_source = ElectrodeSource::Bids;
    m_sourcePath = path;
    return true;
}

//=============================================================================================================

void ElectrodesPlugin::attachScene(MultimodalScene* scene)
{
    if (m_scene == scene) {
        return;
    }

    if (m_scene) {
        QObject::disconnect(m_scene, &MultimodalScene::picked,
                            this, &ElectrodesPlugin::handlePick);
        m_scene->removeLayer(sceneLayerId());
    }

    m_scene = scene;

    if (m_scene) {
        QObject::connect(m_scene, &MultimodalScene::picked,
                         this, &ElectrodesPlugin::handlePick);
        publishToScene();
    }
}

//=============================================================================================================

void ElectrodesPlugin::handlePick(const PickResult& pick)
{
    if (pick.kind != PickKind::ElectrodeContact) {
        return;
    }
    if (!pick.sourceId.isEmpty() && pick.sourceId != sceneLayerId()) {
        return;
    }
    if (pick.label.isEmpty()) {
        return;
    }
    m_object->selectContact(pick.label);
    emit contactPicked(pick.label);
}

//=============================================================================================================

bool ElectrodesPlugin::loadValues(const QString& path)
{
    m_lastError.clear();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_lastError = QStringLiteral("Cannot open %1").arg(path);
        return false;
    }

    QMap<QString, float> values;
    QTextStream in(&file);
    bool firstRow = true;
    int lineNumber = 0;
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        ++lineNumber;
        if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
            continue;
        }
        const QStringList parts = line.split(line.contains(QLatin1Char('\t')) ? QLatin1Char('\t') : QLatin1Char(','));
        bool ok = false;
        const float value = parts.size() >= 2 ? parts[1].trimmed().toFloat(&ok) : 0.0f;
        const QString name = parts[0].trimmed();
        if (!ok || name.isEmpty() || !std::isfinite(value)) {
            if (firstRow && parts.size() >= 2) {
                firstRow = false;
                continue;
            }
            m_lastError = QStringLiteral("Line %1 is not 'name, value': %2").arg(lineNumber).arg(line);
            return false;
        }
        firstRow = false;
        values.insert(name, value);
    }

    QMap<QString, float> matched;
    for (const IEEGLIB::IeegContact& contact : m_object->implantation().contacts()) {
        const auto it = values.constFind(contact.name);
        if (it != values.cend())
            matched.insert(contact.name, it.value());
    }
    if (matched.isEmpty()) {
        m_lastError = values.isEmpty() ? QStringLiteral("No values in %1").arg(path)
                                       : QStringLiteral("None of the %1 names matches a loaded contact").arg(values.size());
        return false;
    }
    setValues(matched);
    return true;
}

//=============================================================================================================

void ElectrodesPlugin::setValues(const QMap<QString, float>& values)
{
    m_values.clear();
    for (const IEEGLIB::IeegContact& contact : m_object->implantation().contacts()) {
        const auto it = values.constFind(contact.name);
        if (it != values.cend())
            m_values.insert(contact.name, it.value());
    }
    emit valuesChanged();
}

//=============================================================================================================

void ElectrodesPlugin::rebuild(const IEEGLIB::IeegImplantation& implantation)
{
    m_object->setImplantation(implantation);
    publishToScene();
    emit electrodesChanged();
    if (!m_values.isEmpty()) {
        m_values.clear();
        emit valuesChanged();
    }
}

//=============================================================================================================

void ElectrodesPlugin::publishToScene()
{
    if (!m_scene) {
        return;
    }
    SceneLayer layer;
    layer.id = sceneLayerId();
    layer.displayName = QStringLiteral("Electrodes");
    layer.kind = SceneLayerKind::Electrode;
    // Non-owning shared_ptr: the plugin retains ownership of the object.
    layer.payload = std::shared_ptr<void>(m_object.get(), [](void*) {});
    m_scene->addLayer(std::move(layer));
}
