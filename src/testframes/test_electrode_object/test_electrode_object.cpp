//=============================================================================================================
/**
 * @file     test_electrode_object.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Tests for ElectrodeObject — sEEG electrode geometry and data overlay.
 *           Tests shaft construction, contact selection, value overlay, and bounding box.
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

//=============================================================================================================
/**
 * DECLARE CLASS TestElectrodeObject
 *
 * @brief Tests for ElectrodeObject sEEG electrode rendering data.
 */
class TestElectrodeObject : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void testEmptyConstruction();
    void testSetShafts();
    void testTotalContactCount();
    void testContactSelection();
    void testClearSelection();
    void testContactValues();
    void testBoundingBox();
    void testSingleContact();
    void testGeometryGeneration();

    void cleanupTestCase();

private:
    QVector<ElectrodeShaft> createTestShafts(int nShafts, int contactsPerShaft) const;
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
    QVERIFY(obj.shafts().isEmpty());
    QVERIFY(obj.selectedContact().isEmpty());
}

//=============================================================================================================

void TestElectrodeObject::testSetShafts()
{
    ElectrodeObject obj;
    QVector<ElectrodeShaft> shafts = createTestShafts(3, 8);
    obj.setShafts(shafts);

    QCOMPARE(obj.shafts().size(), 3);
    QCOMPARE(obj.shafts()[0].contacts.size(), 8);
    QCOMPARE(obj.shafts()[1].contacts.size(), 8);
    QCOMPARE(obj.shafts()[2].contacts.size(), 8);
}

//=============================================================================================================

void TestElectrodeObject::testTotalContactCount()
{
    ElectrodeObject obj;

    // 10 shafts × 12 contacts = 120 total
    QVector<ElectrodeShaft> shafts = createTestShafts(10, 12);
    obj.setShafts(shafts);
    QCOMPARE(obj.totalContactCount(), 120);

    // Different sizes
    QVector<ElectrodeShaft> mixed;
    ElectrodeShaft s1;
    s1.label = "A";
    for (int i = 0; i < 5; ++i) {
        ElectrodeContact c;
        c.name = QString("A%1").arg(i);
        c.position = QVector3D(0, 0, i);
        s1.contacts.append(c);
    }
    ElectrodeShaft s2;
    s2.label = "B";
    for (int i = 0; i < 3; ++i) {
        ElectrodeContact c;
        c.name = QString("B%1").arg(i);
        c.position = QVector3D(10, 0, i);
        s2.contacts.append(c);
    }
    mixed << s1 << s2;
    obj.setShafts(mixed);
    QCOMPARE(obj.totalContactCount(), 8);
}

//=============================================================================================================

void TestElectrodeObject::testContactSelection()
{
    ElectrodeObject obj;
    QVector<ElectrodeShaft> shafts = createTestShafts(2, 4);
    obj.setShafts(shafts);

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
    QVector<ElectrodeShaft> shafts = createTestShafts(1, 4);
    obj.setShafts(shafts);

    obj.selectContact("S0_C1");
    QVERIFY(!obj.selectedContact().isEmpty());

    obj.clearSelection();
    QVERIFY(obj.selectedContact().isEmpty());
}

//=============================================================================================================

void TestElectrodeObject::testContactValues()
{
    ElectrodeObject obj;
    QVector<ElectrodeShaft> shafts = createTestShafts(1, 4);
    obj.setShafts(shafts);

    QMap<QString, float> values;
    values["S0_C0"] = 0.0f;
    values["S0_C1"] = 0.5f;
    values["S0_C2"] = 1.0f;
    values["S0_C3"] = 0.75f;

    obj.setContactValues(values, Qt::blue, Qt::red);

    // After setting values, the contacts should have updated colors
    // Verify the shafts still have correct contact count
    QCOMPARE(obj.shafts()[0].contacts.size(), 4);
}

//=============================================================================================================

void TestElectrodeObject::testBoundingBox()
{
    ElectrodeObject obj;
    QVector<ElectrodeShaft> shafts = createTestShafts(2, 5);
    obj.setShafts(shafts);

    QVector3D bbMin = obj.boundingBoxMin();
    QVector3D bbMax = obj.boundingBoxMax();

    // Bounding box should enclose all contact positions
    for (const auto& shaft : shafts) {
        for (const auto& contact : shaft.contacts) {
            QVERIFY2(contact.position.x() >= bbMin.x() - 1.0f &&
                     contact.position.x() <= bbMax.x() + 1.0f,
                     "Contact X outside bounding box");
            QVERIFY2(contact.position.y() >= bbMin.y() - 1.0f &&
                     contact.position.y() <= bbMax.y() + 1.0f,
                     "Contact Y outside bounding box");
            QVERIFY2(contact.position.z() >= bbMin.z() - 1.0f &&
                     contact.position.z() <= bbMax.z() + 1.0f,
                     "Contact Z outside bounding box");
        }
    }

    // BB max should be >= BB min in each dimension
    QVERIFY(bbMax.x() >= bbMin.x());
    QVERIFY(bbMax.y() >= bbMin.y());
    QVERIFY(bbMax.z() >= bbMin.z());
}

//=============================================================================================================

void TestElectrodeObject::testSingleContact()
{
    // Edge case: a shaft with a single contact (no cylinder can be drawn)
    ElectrodeObject obj;
    ElectrodeShaft shaft;
    shaft.label = "X";
    ElectrodeContact c;
    c.name = "X1";
    c.position = QVector3D(10.0f, 20.0f, 30.0f);
    c.radius = 0.5f;
    shaft.contacts.append(c);

    QVector<ElectrodeShaft> shafts;
    shafts << shaft;
    obj.setShafts(shafts);

    QCOMPARE(obj.totalContactCount(), 1);
    QCOMPARE(obj.shafts().size(), 1);
}

//=============================================================================================================

void TestElectrodeObject::testGeometryGeneration()
{
    ElectrodeObject obj;
    QVector<ElectrodeShaft> shafts = createTestShafts(3, 6);
    obj.setShafts(shafts);

    // Generate shaft geometry (cylinder mesh)
    auto shaftGeo = obj.generateShaftGeometry(16);  // 16 segments
    // Should have geometry data for 3 shafts
    QVERIFY(!shaftGeo.isEmpty());

    // Generate contact instances (instanced spheres)
    auto contactInstances = obj.generateContactInstances();
    // 3 shafts × 6 contacts = 18 instances
    QCOMPARE(contactInstances.size(), 18);
}

//=============================================================================================================

void TestElectrodeObject::cleanupTestCase()
{
}

//=============================================================================================================

QVector<ElectrodeShaft> TestElectrodeObject::createTestShafts(int nShafts, int contactsPerShaft) const
{
    QVector<ElectrodeShaft> shafts;
    for (int s = 0; s < nShafts; ++s) {
        ElectrodeShaft shaft;
        shaft.label = QString("S%1").arg(s);
        shaft.shaftRadius = 0.4f;
        shaft.shaftColor = Qt::gray;

        for (int c = 0; c < contactsPerShaft; ++c) {
            ElectrodeContact contact;
            contact.name = QString("S%1_C%2").arg(s).arg(c);
            // Space contacts along Z axis, separated by shaft index in X
            contact.position = QVector3D(s * 20.0f, 0.0f, c * 3.5f);
            contact.radius = 0.5f;
            contact.color = Qt::white;
            contact.selected = false;
            contact.value = 0.0f;
            shaft.contacts.append(contact);
        }
        shafts.append(shaft);
    }
    return shafts;
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestElectrodeObject)
#include "test_electrode_object.moc"
