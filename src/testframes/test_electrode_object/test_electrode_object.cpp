//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_electrode_object.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Tests for ElectrodeObject — sEEG / ECoG electrode geometry and data overlay.
 *           Tests array construction, contact selection, value overlay, and bounding box.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <disp3D/renderable/electrodeobject.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>
#include <QVector3D>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;

namespace
{
DISP3DLIB::ElectrodeContact makeContact(const QString& name, const QVector3D& position)
{
    DISP3DLIB::ElectrodeContact contact;
    contact.name = name;
    contact.position = position;
    return contact;
}
} // namespace

//=============================================================================================================
/**
 * DECLARE CLASS TestElectrodeObject
 *
 * @brief Tests for ElectrodeObject sEEG/ECoG electrode rendering data.
 */
class TestElectrodeObject : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void testEmptyConstruction();
    void testSetArrays();
    void testTotalContactCount();
    void testContactSelection();
    void testClearSelection();
    void testContactValues();
    void testBoundingBox();
    void testSingleContact();
    void testGeometryGeneration();

    void cleanupTestCase();

private:
    QVector<ElectrodeArray> createTestArrays(int nArrays, int contactsPerArray) const;
};

//=============================================================================================================

void TestElectrodeObject::initTestCase()
{
}

//=============================================================================================================

void TestElectrodeObject::testEmptyConstruction()
{
    ElectrodeObject obj;
    QCOMPARE(obj.totalContactCount(), 0);
    QVERIFY(obj.arrays().isEmpty());
    QVERIFY(obj.selectedContact().isEmpty());
}

//=============================================================================================================

void TestElectrodeObject::testSetArrays()
{
    ElectrodeObject obj;
    QVector<ElectrodeArray> arrays = createTestArrays(3, 8);
    obj.setArrays(arrays);

    QCOMPARE(obj.arrays().size(), 3);
    QCOMPARE(obj.arrays()[0].contacts.size(), 8);
    QCOMPARE(obj.arrays()[1].contacts.size(), 8);
    QCOMPARE(obj.arrays()[2].contacts.size(), 8);
}

//=============================================================================================================

void TestElectrodeObject::testTotalContactCount()
{
    ElectrodeObject obj;

    // 10 arrays × 12 contacts = 120 total
    QVector<ElectrodeArray> arrays = createTestArrays(10, 12);
    obj.setArrays(arrays);
    QCOMPARE(obj.totalContactCount(), 120);

    // Different sizes
    QVector<ElectrodeArray> mixed;
    ElectrodeArray a1;
    a1.label = "A";
    for (int i = 0; i < 5; ++i) {
        ElectrodeContact c;
        c.name = QString("A%1").arg(i);
        c.position = QVector3D(0, 0, i);
        a1.contacts.append(c);
    }
    ElectrodeArray a2;
    a2.label = "B";
    for (int i = 0; i < 3; ++i) {
        ElectrodeContact c;
        c.name = QString("B%1").arg(i);
        c.position = QVector3D(10, 0, i);
        a2.contacts.append(c);
    }
    mixed << a1 << a2;
    obj.setArrays(mixed);
    QCOMPARE(obj.totalContactCount(), 8);
}

//=============================================================================================================

void TestElectrodeObject::testContactSelection()
{
    ElectrodeObject obj;
    QVector<ElectrodeArray> arrays = createTestArrays(2, 4);
    obj.setArrays(arrays);

    obj.selectContact("S0_C2");
    QCOMPARE(obj.selectedContact(), QString("S0_C2"));

    // Selecting a different contact replaces the selection
    obj.selectContact("S1_C0");
    QCOMPARE(obj.selectedContact(), QString("S1_C0"));
}

//=============================================================================================================

void TestElectrodeObject::testClearSelection()
{
    ElectrodeObject obj;
    QVector<ElectrodeArray> arrays = createTestArrays(1, 4);
    obj.setArrays(arrays);

    obj.selectContact("S0_C1");
    QVERIFY(!obj.selectedContact().isEmpty());

    obj.clearSelection();
    QVERIFY(obj.selectedContact().isEmpty());
}

//=============================================================================================================

void TestElectrodeObject::testContactValues()
{
    ElectrodeObject obj;
    QVector<ElectrodeArray> arrays = createTestArrays(1, 4);
    obj.setArrays(arrays);

    QMap<QString, float> values;
    values["S0_C0"] = 0.0f;
    values["S0_C1"] = 0.5f;
    values["S0_C2"] = 1.0f;
    values["S0_C3"] = 0.75f;

    obj.setContactValues(values, Qt::blue, Qt::red);

    // After setting values, the contacts should have updated colors
    // Verify the array still has the correct contact count
    QCOMPARE(obj.arrays()[0].contacts.size(), 4);
}

//=============================================================================================================

void TestElectrodeObject::testBoundingBox()
{
    ElectrodeObject obj;
    QVector<ElectrodeArray> arrays = createTestArrays(2, 5);
    obj.setArrays(arrays);

    QVector3D bbMin = obj.boundingBoxMin();
    QVector3D bbMax = obj.boundingBoxMax();

    // Bounding box should enclose all contact positions
    for (const auto& arr : arrays) {
        for (const auto& contact : arr.contacts) {
            // The box includes the drawn contact radius
            const QVector3D pad(ElectrodeObject::kDepthContactRadius, ElectrodeObject::kDepthContactRadius, ElectrodeObject::kDepthContactRadius);
            QVERIFY(bbMin.x() <= contact.position.x() - pad.x() + 1e-6f && bbMax.x() >= contact.position.x() + pad.x() - 1e-6f);
            QVERIFY(bbMin.y() <= contact.position.y() - pad.y() + 1e-6f && bbMax.y() >= contact.position.y() + pad.y() - 1e-6f);
            QVERIFY(bbMin.z() <= contact.position.z() - pad.z() + 1e-6f && bbMax.z() >= contact.position.z() + pad.z() - 1e-6f);
        }
    }

    QVERIFY(bbMax.x() >= bbMin.x());
    QVERIFY(bbMax.y() >= bbMin.y());
    QVERIFY(bbMax.z() >= bbMin.z());
}

//=============================================================================================================

void TestElectrodeObject::testSingleContact()
{
    // Edge case: a depth array with a single contact (no cylinder can be drawn)
    ElectrodeObject obj;
    ElectrodeArray arr;
    arr.label = "X";
    ElectrodeContact c;
    c.name = "X1";
    c.position = QVector3D(0.01f, 0.02f, 0.03f);
    arr.contacts.append(c);

    QVector<ElectrodeArray> arrays;
    arrays << arr;
    obj.setArrays(arrays);

    QCOMPARE(obj.totalContactCount(), 1);
    QCOMPARE(obj.arrays().size(), 1);
}

//=============================================================================================================

void TestElectrodeObject::testGeometryGeneration()
{
    // A depth lead, a strip and a 2 x 2 grid, in metres
    ElectrodeArray depth;
    depth.label = "LA";
    ElectrodeArray strip;
    strip.label = "ST";
    strip.layout = ElectrodeLayout::Strip;
    ElectrodeArray grid;
    grid.label = "GR";
    grid.layout = ElectrodeLayout::Grid;
    grid.gridRows = 2;
    grid.gridCols = 2;
    for (int i = 0; i < 4; ++i) {
        depth.contacts.append(makeContact(QString("LA%1").arg(i + 1), QVector3D(0.0f, 0.0f, 0.0035f * float(i))));
        strip.contacts.append(makeContact(QString("ST%1").arg(i + 1), QVector3D(0.05f + 0.01f * float(i), 0.0f, 0.0f)));
        grid.contacts.append(makeContact(QString("GR%1").arg(i + 1), QVector3D(0.01f * float(i % 2), 0.05f + 0.01f * float(i / 2), 0.0f)));
    }
    // A gap too wide for one array is not tied
    strip.contacts[3].position.setX(0.2f);

    ElectrodeObject obj;
    obj.setArrays({depth, strip, grid});

    // One sphere per contact; bodies: 1 shaft + 2 strip ties + 4 grid ties
    const InstancedStreams contacts = obj.contactStreams();
    QCOMPARE(contacts.instanceCount, 12);
    QVERIFY(contacts.indexCount > 0 && contacts.vertices.bytes > 0);
    QCOMPARE(obj.bodyStreams().instanceCount, 1 + 2 + 4);

    // Drawn sizes follow the layout: depth contacts are smaller than subdural ones
    float dist = 0.0f;
    const QVector3D down(0.0f, 0.0f, -1.0f);
    QCOMPARE(obj.intersect(QVector3D(ElectrodeObject::kDepthContactRadius * 0.9f, 0.0f, 1.0f), down, dist), 3);
    QVERIFY(qAbs(dist - (1.0f - 0.0105f - 0.00048f)) < 1e-4f);
    QCOMPARE(obj.intersect(QVector3D(ElectrodeObject::kDepthContactRadius * 1.1f, 0.0f, 1.0f), down, dist), -1);
    QCOMPARE(obj.intersect(QVector3D(0.05f + ElectrodeObject::kDepthContactRadius * 1.1f, 0.0f, 1.0f), down, dist), 4);
    QCOMPARE(obj.contactLabel(4), QString("ST1 (strip ST)"));
    QCOMPARE(obj.contactAt(9)->name, QString("GR2"));
    QVERIFY(obj.contactAt(12) == nullptr);

    // Leads get distinct colours; hovering re-uploads only the contact instances
    QVERIFY(ElectrodeObject::leadColor(0) != ElectrodeObject::leadColor(1));
    const quint64 bodyGeneration = obj.bodyStreams().instances.generation;
    const quint64 contactGeneration = contacts.instances.generation;
    obj.setHoveredContact(4);
    obj.setHoveredContact(4);
    QCOMPARE(obj.contactStreams().instances.generation, contactGeneration + 1);
    QCOMPARE(obj.bodyStreams().instances.generation, bodyGeneration);
}

//=============================================================================================================

void TestElectrodeObject::cleanupTestCase()
{
}

//=============================================================================================================

QVector<ElectrodeArray> TestElectrodeObject::createTestArrays(int nArrays, int contactsPerArray) const
{
    QVector<ElectrodeArray> arrays;
    for (int s = 0; s < nArrays; ++s) {
        ElectrodeArray arr;
        arr.label = QString("S%1").arg(s);

        for (int c = 0; c < contactsPerArray; ++c) {
            ElectrodeContact contact;
            contact.name = QString("S%1_C%2").arg(s).arg(c);
            contact.position = QVector3D(s * 0.02f, 0.0f, c * 0.0035f);
            contact.color = Qt::white;
            contact.selected = false;
            contact.value = 0.0f;
            arr.contacts.append(contact);
        }
        arrays.append(arr);
    }
    return arrays;
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestElectrodeObject)
#include "test_electrode_object.moc"
