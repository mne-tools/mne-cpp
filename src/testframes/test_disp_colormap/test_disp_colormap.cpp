#include <QtTest/QtTest>
#include <QColor>
#include <disp/plots/helpers/colormap.h>

using namespace DISPLIB;

class TestDispColormap : public QObject
{
    Q_OBJECT

private:
    void verifyValidColor(QRgb c) {
        QVERIFY(qRed(c) >= 0 && qRed(c) <= 255);
        QVERIFY(qGreen(c) >= 0 && qGreen(c) <= 255);
        QVERIFY(qBlue(c) >= 0 && qBlue(c) <= 255);
    }

private slots:
    // Jet colormap
    void testJetZero()       { verifyValidColor(ColorMap::valueToJet(0.0)); }
    void testJetQuarter()    { verifyValidColor(ColorMap::valueToJet(0.25)); }
    void testJetHalf()       { verifyValidColor(ColorMap::valueToJet(0.5)); }
    void testJetThreeQ()     { verifyValidColor(ColorMap::valueToJet(0.75)); }
    void testJetOne()        { verifyValidColor(ColorMap::valueToJet(1.0)); }
    void testJetSmall()      { verifyValidColor(ColorMap::valueToJet(0.125)); }
    void testJetLarge()      { verifyValidColor(ColorMap::valueToJet(0.875)); }

    // Hot colormap
    void testHotZero()       { verifyValidColor(ColorMap::valueToHot(0.0)); }
    void testHotQuarter()    { verifyValidColor(ColorMap::valueToHot(0.25)); }
    void testHotHalf()       { verifyValidColor(ColorMap::valueToHot(0.5)); }
    void testHotThreeQ()     { verifyValidColor(ColorMap::valueToHot(0.75)); }
    void testHotOne()        { verifyValidColor(ColorMap::valueToHot(1.0)); }

    // HotNeg1 colormap
    void testHotNeg1Zero()   { verifyValidColor(ColorMap::valueToHotNegative1(0.0)); }
    void testHotNeg1Half()   { verifyValidColor(ColorMap::valueToHotNegative1(0.5)); }
    void testHotNeg1One()    { verifyValidColor(ColorMap::valueToHotNegative1(1.0)); }
    void testHotNeg1Quarter(){ verifyValidColor(ColorMap::valueToHotNegative1(0.25)); }
    void testHotNeg1ThreeQ() { verifyValidColor(ColorMap::valueToHotNegative1(0.75)); }

    // HotNeg2 colormap
    void testHotNeg2Zero()   { verifyValidColor(ColorMap::valueToHotNegative2(0.0)); }
    void testHotNeg2Half()   { verifyValidColor(ColorMap::valueToHotNegative2(0.5)); }
    void testHotNeg2One()    { verifyValidColor(ColorMap::valueToHotNegative2(1.0)); }
    void testHotNeg2Quarter(){ verifyValidColor(ColorMap::valueToHotNegative2(0.25)); }
    void testHotNeg2ThreeQ() { verifyValidColor(ColorMap::valueToHotNegative2(0.75)); }

    // Bone colormap
    void testBoneZero()      { verifyValidColor(ColorMap::valueToBone(0.0)); }
    void testBoneQuarter()   { verifyValidColor(ColorMap::valueToBone(0.25)); }
    void testBoneHalf()      { verifyValidColor(ColorMap::valueToBone(0.5)); }
    void testBoneThreeQ()    { verifyValidColor(ColorMap::valueToBone(0.75)); }
    void testBoneOne()       { verifyValidColor(ColorMap::valueToBone(1.0)); }

    // RedBlue colormap (input range -1 to 1)
    void testRedBlueNeg1()   { verifyValidColor(ColorMap::valueToRedBlue(-1.0)); }
    void testRedBlueNegHalf(){ verifyValidColor(ColorMap::valueToRedBlue(-0.5)); }
    void testRedBlueZero()   { verifyValidColor(ColorMap::valueToRedBlue(0.0)); }
    void testRedBlueHalf()   { verifyValidColor(ColorMap::valueToRedBlue(0.5)); }
    void testRedBlueOne()    { verifyValidColor(ColorMap::valueToRedBlue(1.0)); }

    // Cool colormap
    void testCoolZero()      { verifyValidColor(ColorMap::valueToCool(0.0)); }
    void testCoolHalf()      { verifyValidColor(ColorMap::valueToCool(0.5)); }
    void testCoolOne()       { verifyValidColor(ColorMap::valueToCool(1.0)); }

    // Viridis colormap
    void testViridisZero()   { verifyValidColor(ColorMap::valueToViridis(0.0)); }
    void testViridisQuarter(){ verifyValidColor(ColorMap::valueToViridis(0.25)); }
    void testViridisHalf()   { verifyValidColor(ColorMap::valueToViridis(0.5)); }
    void testViridisThreeQ() { verifyValidColor(ColorMap::valueToViridis(0.75)); }
    void testViridisOne()    { verifyValidColor(ColorMap::valueToViridis(1.0)); }

    // ViridisNegated colormap
    void testViridisNegZero(){ verifyValidColor(ColorMap::valueToViridisNegated(0.0)); }
    void testViridisNegHalf(){ verifyValidColor(ColorMap::valueToViridisNegated(0.5)); }
    void testViridisNegOne() { verifyValidColor(ColorMap::valueToViridisNegated(1.0)); }

    // valueToColor dispatcher
    void testDispatchJet()   { verifyValidColor(ColorMap::valueToColor(0.5, "Jet")); }
    void testDispatchHot()   { verifyValidColor(ColorMap::valueToColor(0.5, "Hot")); }
    void testDispatchHotN1() { verifyValidColor(ColorMap::valueToColor(0.5, "Hot Negative 1")); }
    void testDispatchHotN2() { verifyValidColor(ColorMap::valueToColor(0.5, "Hot Negative 2")); }
    void testDispatchBone()  { verifyValidColor(ColorMap::valueToColor(0.5, "Bone")); }
    void testDispatchRedBlue(){ verifyValidColor(ColorMap::valueToColor(0.0, "RedBlue")); }
    void testDispatchCool()  { verifyValidColor(ColorMap::valueToColor(0.5, "Cool")); }
    void testDispatchViridis(){ verifyValidColor(ColorMap::valueToColor(0.5, "Viridis")); }

    // Boundary conditions
    void testJetNegative()   { verifyValidColor(ColorMap::valueToJet(-0.5)); }
    void testJetOverOne()    { verifyValidColor(ColorMap::valueToJet(1.5)); }
    void testHotNegative()   { verifyValidColor(ColorMap::valueToHot(-0.5)); }
    void testBoneNegative()  { verifyValidColor(ColorMap::valueToBone(-0.5)); }

    // Jet structure: blue at 0, red at 1
    void testJetBlueAtZero()
    {
        QRgb c = ColorMap::valueToJet(0.0);
        // At 0.0 jet should be blue-ish
        QVERIFY(qBlue(c) > qRed(c));
    }

    void testJetRedAtOne()
    {
        QRgb c = ColorMap::valueToJet(1.0);
        // At 1.0 jet should be red-ish
        QVERIFY(qRed(c) > qBlue(c));
    }

    // RedBlue: blue at -1, red at +1
    void testRedBlueRedEnd()
    {
        QRgb c = ColorMap::valueToRedBlue(1.0);
        QVERIFY(qRed(c) > qBlue(c));
    }

    void testRedBlueBlueEnd()
    {
        QRgb c = ColorMap::valueToRedBlue(-1.0);
        QVERIFY(qBlue(c) > qRed(c));
    }
};

QTEST_GUILESS_MAIN(TestDispColormap)
#include "test_disp_colormap.moc"
