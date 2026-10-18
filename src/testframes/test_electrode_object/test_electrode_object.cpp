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
 *           Tests the view of an IeegImplantation: selection, value overlay, bounding box.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <disp3D/renderable/electrodeobject.h>
#include <ieeg/ieeg_implantation.h>

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
using namespace IEEGLIB;

namespace
{
IeegContact makeContact(const QString& name, const Eigen::Vector3f& position)
{
    IeegContact contact;
    contact.name = name;
    contact.position = position;
    return contact;
}

// nLeads depth leads "S<l>_C<c>" with 3.5 mm spacing, 2 cm apart
IeegImplantation createTestImplantation(int nLeads, int contactsPerLead)
{
    QVector<IeegContact> contacts;
    for (int l = 0; l < nLeads; ++l) {
        for (int c = 0; c < contactsPerLead; ++c)
            contacts.append(makeContact(QString("S%1_C%2").arg(l).arg(c), Eigen::Vector3f(l * 0.02f, 0.0f, c * 0.0035f)));
    }
    return IeegImplantation(contacts);
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
    void testSetImplantation();
    void testContactSelection();
    void testContactValues();
    void testBoundingBox();
    void testSingleContact();
    void testGeometryGeneration();

    void cleanupTestCase();
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
    QVERIFY(obj.implantation().isEmpty());
    QVERIFY(obj.selectedContact().isEmpty());
    QCOMPARE(obj.contactStreams().instanceCount, 0);
}

//=============================================================================================================

void TestElectrodeObject::testSetImplantation()
{
    ElectrodeObject obj;
    obj.setImplantation(createTestImplantation(10, 12));
    QCOMPARE(obj.totalContactCount(), 120);
    QCOMPARE(obj.implantation().leads().size(), 10);
    QCOMPARE(obj.contactStreams().instanceCount, 120);
    QCOMPARE(obj.bodyStreams().instanceCount, 10); // one shaft per depth lead

    obj.setImplantation(createTestImplantation(2, 3));
    QCOMPARE(obj.totalContactCount(), 6);
    QCOMPARE(obj.contactAt(4)->name, QString("S1_C1"));
}

//=============================================================================================================

void TestElectrodeObject::testContactSelection()
{
    ElectrodeObject obj;
    obj.setImplantation(createTestImplantation(2, 4));

    obj.selectContact("S0_C2");
    QCOMPARE(obj.selectedContact(), QString("S0_C2"));
    obj.selectContact("S1_C0");
    QCOMPARE(obj.selectedContact(), QString("S1_C0"));
    obj.selectContact("nope");
    QVERIFY(obj.selectedContact().isEmpty());

    obj.selectContact("S0_C1");
    obj.clearSelection();
    QVERIFY(obj.selectedContact().isEmpty());
}

//=============================================================================================================

void TestElectrodeObject::testContactValues()
{
    ElectrodeObject obj;
    obj.setImplantation(createTestImplantation(1, 4));
    const quint64 generation = obj.contactStreams().instances.generation;

    QMap<QString, float> values;
    values["S0_C0"] = 0.0f;
    values["S0_C2"] = 1.0f;
    obj.setContactValues(values, QStringLiteral("Jet"));
    QCOMPARE(obj.contactStreams().instances.generation, generation + 1);
    QCOMPARE(obj.totalContactCount(), 4);

    obj.clearContactValues();
    QCOMPARE(obj.contactStreams().instances.generation, generation + 2);
    QVERIFY(obj.surfaceColors(Eigen::MatrixX3f::Zero(3, 3)).isEmpty());

    // Painting the cortex: a depth contact stays in place and fades out linearly over 1 cm
    values.clear();
    values["S0_C0"] = 2.0f;
    obj.setContactValues(values);
    Eigen::MatrixX3f vertices(3, 3);
    vertices << 0.0f, 0.0f, 0.0f, -0.005f, 0.0f, 0.0f, -0.02f, 0.0f, 0.0f;
    const QVector<uint32_t> colors = obj.surfaceColors(vertices);
    QCOMPARE(colors.size(), 3);
    QCOMPARE(colors[0] >> 24, 255u);
    QCOMPARE(colors[1] >> 24, 127u);
    QCOMPARE(colors[1] & 0xFFFFFFu, colors[0] & 0xFFFFFFu); // same value, same colour
    QCOMPARE(colors[2] >> 24, 0u);
}

//=============================================================================================================

void TestElectrodeObject::testBoundingBox()
{
    ElectrodeObject obj;
    const IeegImplantation implantation = createTestImplantation(2, 5);
    obj.setImplantation(implantation);

    const QVector3D bbMin = obj.boundingBoxMin();
    const QVector3D bbMax = obj.boundingBoxMax();
    const float pad = ElectrodeObject::kDepthContactRadius - 1e-6f;
    for (const IeegContact& contact : implantation.contacts()) {
        for (int k = 0; k < 3; ++k) {
            QVERIFY(bbMin[k] <= contact.position[k] - pad);
            QVERIFY(bbMax[k] >= contact.position[k] + pad);
        }
    }
}

//=============================================================================================================

void TestElectrodeObject::testSingleContact()
{
    // A lead with a single contact has no body
    ElectrodeObject obj;
    obj.setImplantation(IeegImplantation({makeContact("X1", Eigen::Vector3f(0.01f, 0.02f, 0.03f))}));
    QCOMPARE(obj.totalContactCount(), 1);
    QCOMPARE(obj.contactStreams().instanceCount, 1);
    QCOMPARE(obj.bodyStreams().instanceCount, 0);
}

//=============================================================================================================

void TestElectrodeObject::testGeometryGeneration()
{
    // A depth lead, a strip and a 2 x 2 grid, in metres
    QVector<IeegContact> contacts;
    for (int i = 0; i < 4; ++i)
        contacts.append(makeContact(QString("LA%1").arg(i + 1), Eigen::Vector3f(0.0f, 0.0f, 0.0035f * float(i))));
    for (int i = 0; i < 4; ++i)
        contacts.append(makeContact(QString("ST%1").arg(i + 1), Eigen::Vector3f(0.05f + 0.01f * float(i), 0.0f, 0.0f)));
    for (int i = 0; i < 4; ++i)
        contacts.append(makeContact(QString("GR%1").arg(i + 1), Eigen::Vector3f(0.01f * float(i % 2), 0.05f + 0.01f * float(i / 2), 0.0f)));
    // A gap too wide for one array is not tied
    contacts[7].position.x() = 0.2f;
    IeegImplantation implantation(contacts);
    QVERIFY(implantation.setLeadType("GR", IeegLeadType::Grid, 2, 2));

    ElectrodeObject obj;
    obj.setImplantation(implantation);

    // One sphere per contact; bodies: 1 shaft + 2 strip ties + 4 grid ties
    const InstancedStreams streams = obj.contactStreams();
    QCOMPARE(streams.instanceCount, 12);
    QVERIFY(streams.indexCount > 0 && streams.vertices.bytes > 0);
    QCOMPARE(obj.bodyStreams().instanceCount, 1 + 2 + 4);

    // Drawn sizes follow the lead type: depth contacts are smaller than subdural ones
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
    const quint64 contactGeneration = streams.instances.generation;
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

QTEST_GUILESS_MAIN(TestElectrodeObject)
#include "test_electrode_object.moc"
