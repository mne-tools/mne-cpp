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

#include <bids/bids_coordinate_system.h>
#include <bids/bids_electrode.h>

#include <disp3D/scene/multimodalscene.h>

#include <fiff/fiff_constants.h>
#include <fiff/fiff_dig_point.h>
#include <fiff/fiff_dig_point_set.h>
#include <fiff/fiff_stream.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QByteArray>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>

#include <algorithm>
#include <limits>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace ELECTRODESPLUGIN;
using namespace DISP3DLIB;
using namespace FIFFLIB;

namespace
{

ElectrodeLayout parseLayout(const QString& raw)
{
    const QString s = raw.trimmed().toLower();
    if (s == QLatin1String("strip")) {
        return ElectrodeLayout::Strip;
    }
    if (s == QLatin1String("grid")) {
        return ElectrodeLayout::Grid;
    }
    return ElectrodeLayout::Depth;
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

int ElectrodesPlugin::arrayCount() const
{
    return m_object->arrays().size();
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
    const QVector<ElectrodeArray>& arrays = m_object->arrays();
    QString bestName;
    float bestDistSq = std::numeric_limits<float>::max();
    for (const ElectrodeArray& arr : arrays) {
        for (const ElectrodeContact& c : arr.contacts) {
            const float d = (c.position - worldPosition).lengthSquared();
            if (d < bestDistSq) {
                bestDistSq = d;
                bestName = c.name;
            }
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

void ElectrodesPlugin::setArrays(const QVector<ElectrodeArray>& arrays)
{
    rebuildFromArrays(arrays);
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

    ElectrodeArray array;
    array.label = QFileInfo(path).baseName();
    array.layout = ElectrodeLayout::Depth;
    array.contacts.reserve(picked.size());

    for (qint32 i = 0; i < picked.size(); ++i) {
        const FiffDigPoint& dp = picked[i];
        ElectrodeContact contact;
        contact.name = QStringLiteral("%1_%2").arg(array.label).arg(dp.ident);
        contact.position = QVector3D(dp.r[0], dp.r[1], dp.r[2]);
        array.contacts.append(contact);
    }

    QVector<ElectrodeArray> arrays;
    arrays.append(array);
    rebuildFromArrays(arrays);

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

    QHash<QString, ElectrodeArray> byLabel;
    QStringList order;
    int contacts = 0;

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
        const QString arrayLabel = (parts.size() >= 5 && !parts[4].trimmed().isEmpty())
            ? parts[4].trimmed()
            : fallbackLabel;
        const ElectrodeLayout layout = (parts.size() >= 6)
            ? parseLayout(parts[5])
            : ElectrodeLayout::Depth;

        if (!byLabel.contains(arrayLabel)) {
            ElectrodeArray arr;
            arr.label = arrayLabel;
            arr.layout = layout;
            byLabel.insert(arrayLabel, arr);
            order.append(arrayLabel);
        } else if (byLabel[arrayLabel].layout != layout) {
            // First non-Depth declaration for this array wins.
            if (byLabel[arrayLabel].layout == ElectrodeLayout::Depth) {
                byLabel[arrayLabel].layout = layout;
            }
        }

        ElectrodeContact contact;
        contact.name = name.isEmpty()
            ? QStringLiteral("%1_%2").arg(arrayLabel).arg(byLabel[arrayLabel].contacts.size())
            : name;
        contact.position = QVector3D(x, y, z);
        byLabel[arrayLabel].contacts.append(contact);
        ++contacts;
    }

    if (contacts == 0) {
        qWarning() << "ElectrodesPlugin::loadCsv: no contact rows parsed from" << path;
        return false;
    }

    QVector<ElectrodeArray> arrays;
    arrays.reserve(order.size());
    for (const QString& key : order) {
        ElectrodeArray& arr = byLabel[key];
        if (arr.layout == ElectrodeLayout::Grid) {
            // Default to a 1×N grid when caller did not specify rows/cols.
            arr.gridRows = 1;
            arr.gridCols = arr.contacts.size();
        }
        arrays.append(arr);
    }

    rebuildFromArrays(arrays);
    m_source = ElectrodeSource::Csv;
    m_sourcePath = path;
    return true;
}

//=============================================================================================================

bool ElectrodesPlugin::loadBids(const QString& path)
{
    const QFileInfo info(path);
    // The coordsystem sidecar sharing the longest name prefix, so sessions are never mixed
    QString coordPath;
    int bestPrefix = -1;
    const QStringList candidates = info.dir().entryList({QStringLiteral("*_coordsystem.json")}, QDir::Files);
    for (const QString& candidate : candidates) {
        int prefix = 0;
        while (prefix < candidate.size() && prefix < info.fileName().size() && candidate[prefix] == info.fileName()[prefix])
            ++prefix;
        if (prefix > bestPrefix) {
            bestPrefix = prefix;
            coordPath = info.dir().filePath(candidate);
        }
    }
    if (coordPath.isEmpty()) {
        qWarning() << "ElectrodesPlugin::loadBids: no *_coordsystem.json next to" << path << "- units unknown";
        return false;
    }
    const QString units = BIDSLIB::BidsCoordinateSystem::readJson(coordPath).units;
    const float scale = units == QLatin1String("m") ? 1.0f : units == QLatin1String("cm") ? 0.01f
        : units == QLatin1String("mm")                                                    ? 0.001f
                                                                                          : 0.0f;
    if (scale == 0.0f) {
        qWarning() << "ElectrodesPlugin::loadBids: unsupported iEEGCoordinateUnits" << units << "in" << coordPath;
        return false;
    }

    // Lead name: the contact name without its trailing number; the number orders the lead
    static const QRegularExpression trailingNumber(QStringLiteral("^(.*?)[\\s_-]*(\\d+)$"));
    struct Entry
    {
        int number;
        ElectrodeContact contact;
    };
    QHash<QString, QVector<Entry>> leads;
    QHash<QString, QString> leadTypes;
    QStringList order;
    for (const BIDSLIB::BidsElectrode& e : BIDSLIB::BidsElectrode::readTsv(path)) {
        bool okX = false, okY = false, okZ = false;
        const float x = e.x.toFloat(&okX), y = e.y.toFloat(&okY), z = e.z.toFloat(&okZ);
        if (!okX || !okY || !okZ)
            continue;
        const QRegularExpressionMatch m = trailingNumber.match(e.name);
        const QString lead = (m.hasMatch() && !m.captured(1).isEmpty()) ? m.captured(1) : e.name;
        if (!leads.contains(lead))
            order.append(lead);
        ElectrodeContact contact;
        contact.name = e.name;
        contact.position = QVector3D(x, y, z) * scale;
        leads[lead].append({m.hasMatch() ? m.captured(2).toInt() : 0, contact});
        if (e.type != QLatin1String("n/a") && !e.type.isEmpty())
            leadTypes[lead] = e.type;
    }
    if (order.isEmpty()) {
        qWarning() << "ElectrodesPlugin::loadBids: no contacts with coordinates in" << path;
        return false;
    }

    QVector<ElectrodeArray> arrays;
    for (const QString& lead : std::as_const(order)) {
        QVector<Entry>& entries = leads[lead];
        std::stable_sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) { return a.number < b.number; });
        ElectrodeArray array;
        array.label = lead;
        for (const Entry& entry : std::as_const(entries))
            array.contacts.append(entry.contact);
        if (leadTypes.contains(lead)) {
            array.layout = parseLayout(leadTypes[lead]);
        } else {
            float spacing = std::numeric_limits<float>::max();
            for (int i = 1; i < array.contacts.size(); ++i)
                spacing = std::min(spacing, (array.contacts[i].position - array.contacts[i - 1].position).length());
            array.layout = spacing < 0.006f ? ElectrodeLayout::Depth : ElectrodeLayout::Strip;
        }
        arrays.append(array);
    }

    rebuildFromArrays(arrays);
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

void ElectrodesPlugin::rebuildFromArrays(const QVector<ElectrodeArray>& arrays)
{
    m_object->setArrays(arrays);
    publishToScene();
    emit electrodesChanged();
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
