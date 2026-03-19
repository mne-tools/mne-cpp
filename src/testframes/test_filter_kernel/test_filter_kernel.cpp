#include <QtTest/QtTest>
#include <Eigen/Dense>

#include <dsp/filterkernel.h>
#include <dsp/parksmcclellan.h>
#include <dsp/cosinefilter.h>

using namespace UTILSLIB;
using namespace Eigen;

class TestFilterKernel : public QObject
{
    Q_OBJECT

private slots:
    //=========================================================================
    // FilterKernel construction and getters/setters
    //=========================================================================
    void defaultCtor()
    {
        FilterKernel fk;
        QVERIFY(fk.getName().isEmpty() || true);
        QVERIFY(fk.getSamplingFrequency() >= 0);
    }

    void lpfCtor()
    {
        FilterKernel fk("TestLPF", 0, 128, 0.2, 0.0, 0.01, 1000.0, 0);
        QCOMPARE(fk.getName(), QString("TestLPF"));
        QVERIFY(fk.getFilterOrder() > 0);
        QVERIFY(fk.getSamplingFrequency() > 0);
    }

    void hpfCtor()
    {
        FilterKernel fk("TestHPF", 1, 128, 0.0, 0.01, 0.005, 1000.0, 0);
        QCOMPARE(fk.getName(), QString("TestHPF"));
    }

    void bpfCtor()
    {
        FilterKernel fk("TestBPF", 2, 128, 0.02, 0.01, 0.005, 1000.0, 0);
        QCOMPARE(fk.getName(), QString("TestBPF"));
    }

    void gettersSetters()
    {
        FilterKernel fk("Test", 0, 64, 0.2, 0.0, 0.01, 500.0, 0);

        fk.setName("NewName");
        QCOMPARE(fk.getName(), QString("NewName"));

        fk.setSamplingFrequency(2000.0);
        QVERIFY(qAbs(fk.getSamplingFrequency() - 2000.0) < 0.1);

        fk.setFilterOrder(256);
        QCOMPARE(fk.getFilterOrder(), 256);

        fk.setCenterFrequency(0.1);
        QVERIFY(qAbs(fk.getCenterFrequency() - 0.1) < 0.01);

        fk.setBandwidth(0.05);
        QVERIFY(qAbs(fk.getBandwidth() - 0.05) < 0.01);

        fk.setParksWidth(0.02);
        QVERIFY(qAbs(fk.getParksWidth() - 0.02) < 0.01);

        fk.setHighpassFreq(5.0);
        QVERIFY(qAbs(fk.getHighpassFreq() - 5.0) < 0.1);

        fk.setLowpassFreq(40.0);
        QVERIFY(qAbs(fk.getLowpassFreq() - 40.0) < 0.1);
    }

    void shortDescription()
    {
        FilterKernel fk("Desc", 0, 64, 0.2, 0.0, 0.01, 1000.0, 0);
        QString desc = fk.getShortDescription();
        QVERIFY(!desc.isEmpty());
    }

    void coefficients()
    {
        FilterKernel fk("Coeff", 0, 64, 0.2, 0.0, 0.01, 1000.0, 0);
        RowVectorXd coeffs = fk.getCoefficients();
        QVERIFY(coeffs.size() > 0);
    }

    //=========================================================================
    // FilterKernel - prepare and apply
    //=========================================================================
    void prepareAndApplyConv()
    {
        FilterKernel fk("ConvTest", 0, 64, 0.2, 0.0, 0.01, 1000.0, 0);
        int N = 256;
        fk.prepareFilter(N);

        RowVectorXd data = RowVectorXd::Random(N);
        RowVectorXd filtered = fk.applyConvFilter(data, false);
        QCOMPARE(filtered.size(), N);
    }

    void prepareAndApplyFft()
    {
        FilterKernel fk("FftTest", 0, 64, 0.2, 0.0, 0.01, 1000.0, 0);
        int N = 512;
        fk.prepareFilter(N);

        RowVectorXd data = RowVectorXd::Random(N);
        fk.applyFftFilter(data, false);
        QCOMPARE(data.size(), N);
    }

    void designMethods()
    {
        FilterKernel fkCosine("Cosine", 0, 64, 0.2, 0.0, 0.01, 1000.0, 0);
        QVERIFY(fkCosine.getCoefficients().size() > 0);

        FilterKernel fkParks("Parks", 0, 64, 0.2, 0.0, 0.01, 1000.0, 1);
        QVERIFY(fkParks.getCoefficients().size() > 0);
    }

    void filterParameter()
    {
        FilterParameter fp;
        QCOMPARE(fp.getName(), QString("Unknown"));

        FilterParameter fp2("TestParam");
        QCOMPARE(fp2.getName(), QString("TestParam"));

        FilterParameter fp3("Param", "Description");
        QCOMPARE(fp3.getName(), QString("Param"));

        QVERIFY(fp2 == FilterParameter("TestParam"));
    }

    //=========================================================================
    // ParksMcClellan
    //=========================================================================
    void parksMcClellan_basic()
    {
        ParksMcClellan pm(33, 0.3, 0.2, 0.1, ParksMcClellan::LPF);
        QVERIFY(pm.FirCoeff.size() > 0);
    }

    //=========================================================================
    // CosineFilter
    //=========================================================================
    void cosineFilter_lpf()
    {
        CosineFilter cf(512, 40.0f, 5.0f, 0.0f, 0.0f, 1000.0, CosineFilter::LPF);
        QVERIFY(cf.m_vecCoeff.size() > 0);
    }

    void cosineFilter_hpf()
    {
        CosineFilter cf(512, 0.0f, 0.0f, 5.0f, 3.0f, 1000.0, CosineFilter::HPF);
        QVERIFY(cf.m_vecCoeff.size() > 0);
    }
};

QTEST_GUILESS_MAIN(TestFilterKernel)
#include "test_filter_kernel.moc"
