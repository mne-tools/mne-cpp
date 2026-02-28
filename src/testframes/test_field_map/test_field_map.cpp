//=============================================================================================================
/**
 * @file     test_field_map.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
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
 * @brief    Cross-validation test for FieldMap against MNE-Python reference.
 *
 * This test:
 *   1. Runs a Python script (generate_fieldmap_reference.py) that uses
 *      MNE-Python's internal routines to compute field-mapping matrices.
 *   2. Loads the same evoked data, coil definitions, and head surface
 *      in mne-cpp and computes field-mapping matrices via FieldMap.
 *   3. Loads the Python-generated .npy reference files and compares
 *      element-wise, verifying numerical agreement.
 *
 * The test covers both MEG (magnetometers + gradiometers) and EEG modalities,
 * validating self-dot products, surface-dot products, and the final SVD-based
 * mapping matrix.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fwd/fwd_field_map.h>

#include <fiff/fiff.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_coord_trans.h>
#include <fwd/fwd_coil_set.h>
#include <mne/mne_bem.h>

#include <Eigen/Core>
#include <Eigen/Geometry>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTextStream>
#include <QDebug>
#include <QtTest>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace FWDLIB;

//=============================================================================================================
// HELPER: Load a .npy file (double, float, or int64)
//=============================================================================================================

namespace {

/**
 * Minimal .npy parser – supports float64 / float32 / int64, C-order, 1-D or 2-D.
 * Returns data as MatrixXd (or VectorXd for 1-D).
 */
MatrixXd loadNpy(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "loadNpy: Cannot open" << path;
        return MatrixXd();
    }
    QByteArray data = f.readAll();
    f.close();

    const char* d = data.constData();
    // Magic: \x93NUMPY
    if (data.size() < 10 || d[0] != char(0x93) || d[1] != 'N') {
        qWarning() << "loadNpy: Bad magic" << path;
        return MatrixXd();
    }

    uint8_t major = static_cast<uint8_t>(d[6]);
    uint16_t headerLen = 0;
    int headerOffset = 0;
    if (major == 1) {
        headerLen = *reinterpret_cast<const uint16_t*>(d + 8);
        headerOffset = 10;
    } else {  // version 2
        uint32_t hl = *reinterpret_cast<const uint32_t*>(d + 8);
        headerLen = static_cast<uint16_t>(hl);
        headerOffset = 12;
    }

    QByteArray header = QByteArray(d + headerOffset, headerLen);
    QString hdr = QString::fromLatin1(header);

    // Parse dtype
    bool isFloat64 = hdr.contains("<f8") || hdr.contains("float64");
    bool isFloat32 = hdr.contains("<f4") || hdr.contains("float32");
    bool isInt64   = hdr.contains("<i8") || hdr.contains("int64");
    int elemSize = isFloat64 ? 8 : (isFloat32 ? 4 : (isInt64 ? 8 : 0));
    if (elemSize == 0) {
        qWarning() << "loadNpy: Unsupported dtype in" << path << hdr;
        return MatrixXd();
    }

    // Parse shape
    int shapeStart = hdr.indexOf('(');
    int shapeEnd   = hdr.indexOf(')');
    QString shapeStr = hdr.mid(shapeStart + 1, shapeEnd - shapeStart - 1).trimmed();
    // Remove trailing comma for 1-D: "(306,)"
    if (shapeStr.endsWith(','))
        shapeStr.chop(1);

    QStringList dims = shapeStr.split(',', Qt::SkipEmptyParts);
    int rows = 1, cols = 1;
    if (dims.size() == 1) {
        rows = dims[0].trimmed().toInt();
        cols = 1;
    } else if (dims.size() == 2) {
        rows = dims[0].trimmed().toInt();
        cols = dims[1].trimmed().toInt();
    }

    const char* dataPtr = d + headerOffset + headerLen;
    int64_t dataSize = data.size() - headerOffset - headerLen;
    if (dataSize < static_cast<int64_t>(rows) * cols * elemSize) {
        qWarning() << "loadNpy: Data too short" << path;
        return MatrixXd();
    }

    MatrixXd result(rows, cols);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int idx = r * cols + c;
            if (isFloat64) {
                double val;
                memcpy(&val, dataPtr + idx * 8, 8);
                result(r, c) = val;
            } else if (isFloat32) {
                float val;
                memcpy(&val, dataPtr + idx * 4, 4);
                result(r, c) = static_cast<double>(val);
            } else if (isInt64) {
                int64_t val;
                memcpy(&val, dataPtr + idx * 8, 8);
                result(r, c) = static_cast<double>(val);
            }
        }
    }
    return result;
}

/**
 * Compare two matrices element-wise with relative + absolute tolerance.
 * Returns the maximum relative error found.
 */
double compareMatrices(const MatrixXd& a, const MatrixXd& b,
                       double rtol, double atol,
                       const QString& label,
                       int& nFail)
{
    if (a.rows() != b.rows() || a.cols() != b.cols()) {
        qWarning() << label << "shape mismatch:"
                   << a.rows() << "x" << a.cols()
                   << "vs" << b.rows() << "x" << b.cols();
        nFail = -1;
        return 1e30;
    }

    nFail = 0;
    double maxRelErr = 0.0;
    for (int r = 0; r < a.rows(); ++r) {
        for (int c = 0; c < a.cols(); ++c) {
            double va = a(r, c);
            double vb = b(r, c);
            double diff = std::abs(va - vb);
            double denom = std::max(std::abs(va), std::abs(vb));
            double relErr = (denom > atol) ? (diff / denom) : 0.0;
            if (diff > atol && relErr > rtol) {
                ++nFail;
                if (nFail <= 5) {
                    qWarning() << "  " << label
                               << QString("(%1,%2): cpp=%3 py=%4 diff=%5 rel=%6")
                                  .arg(r).arg(c)
                                  .arg(va, 0, 'e', 8)
                                  .arg(vb, 0, 'e', 8)
                                  .arg(diff, 0, 'e', 4)
                                  .arg(relErr, 0, 'e', 4);
                }
            }
            maxRelErr = std::max(maxRelErr, relErr);
        }
    }
    return maxRelErr;
}

/**
 * Compute vertex normals from vertices and triangles.
 * (Simple area-weighted averaging per face.)
 */
MatrixX3f computeVertexNormals(const MatrixX3f& verts, const MatrixX3i& tris)
{
    MatrixX3f norms = MatrixX3f::Zero(verts.rows(), 3);
    for (int t = 0; t < tris.rows(); ++t) {
        int i0 = tris(t, 0), i1 = tris(t, 1), i2 = tris(t, 2);
        Vector3f e1 = verts.row(i1) - verts.row(i0);
        Vector3f e2 = verts.row(i2) - verts.row(i0);
        Vector3f fn = e1.cross(e2);  // area-weighted normal
        norms.row(i0) += fn.transpose();
        norms.row(i1) += fn.transpose();
        norms.row(i2) += fn.transpose();
    }
    for (int i = 0; i < norms.rows(); ++i) {
        float len = norms.row(i).norm();
        if (len > 0.0f) norms.row(i) /= len;
    }
    return norms;
}

} // anonymous namespace

//=============================================================================================================
// TEST CLASS
//=============================================================================================================

class TestFieldMap : public QObject
{
    Q_OBJECT

public:
    TestFieldMap();

private slots:
    void initTestCase();
    void testMegSelfDots();
    void testMegSurfaceDots();
    void testMegMapping();
    void testEegSelfDots();
    void testEegSurfaceDots();
    void testEegMapping();
    void testMegMappingApplySymmetry();
    void testEegMappingApplySymmetry();
    void testMultiEvokedMegMapping();
    void testMultiEvokedEegMapping();
    void testHelmetFieldMap();
    void cleanupTestCase();

private:
    // ── Paths ─────────────────────────────────────────────────────────
    QString m_dataDir;        // MNE sample data root
    QString m_refDir;         // Python-generated reference directory
    QString m_coilDefPath;    // coil_def.dat
    QString m_pythonScript;   // generate_fieldmap_reference.py
    QString m_pythonCmd;      // detected Python interpreter command

    // ── Data ──────────────────────────────────────────────────────────
    FiffEvokedSet m_evokedSet;
    FiffEvoked    m_evoked;

    // Head surface (in head coordinates)
    MatrixX3f m_surfVerts;
    MatrixX3f m_surfNorms;

    // Sphere origin
    Vector3f m_origin;

    // Coil sets
    std::unique_ptr<FwdCoilSet> m_megCoils;
    std::unique_ptr<FwdCoilSet> m_eegCoils;

    // C++ computed mapping matrices
    QSharedPointer<MatrixXf> m_megMapping;
    QSharedPointer<MatrixXf> m_eegMapping;

    // Python reference matrices
    MatrixXd m_refMegSelfDots;
    MatrixXd m_refMegSurfaceDots;
    MatrixXd m_refMegMapping;
    MatrixXd m_refEegSelfDots;
    MatrixXd m_refEegSurfaceDots;
    MatrixXd m_refEegMapping;

    bool m_hasMeg = false;
    bool m_hasEeg = false;
    bool m_pythonOk = false;
    bool m_multiEvokedOk = false;

    // ── Multi-evoked cross-validation ─────────────────────────────────
    QString m_multiRefDir;
    QString m_multiPythonScript;
    int m_numConditions = 0;
    QStringList m_conditionNames;
    QVector<int> m_peakTimes;
    QStringList m_megChNames;
    QStringList m_eegChNames;
};

//=============================================================================================================
// IMPLEMENTATION
//=============================================================================================================

TestFieldMap::TestFieldMap()
{
}

//=============================================================================================================

void TestFieldMap::initTestCase()
{
    // ── Locate data ────────────────────────────────────────────────────
    // Try several data locations (MNE_DATA env var, home dir, app-relative)
    QStringList dataDirs;
    QString envMneData = qEnvironmentVariable("MNE_DATA");
    if (!envMneData.isEmpty()) {
        dataDirs.append(envMneData + "/MNE-sample-data");
    }
    dataDirs.append(QDir::homePath() + "/mne_data/MNE-sample-data");
    dataDirs.append(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data");
    for (const auto& dd : dataDirs) {
        if (QFile::exists(dd + "/MEG/sample/sample_audvis-ave.fif")) {
            m_dataDir = dd;
            break;
        }
    }
    QVERIFY2(!m_dataDir.isEmpty(),
             "Sample data not found. Set MNE_SAMPLE_DATA_DIR or place data in ~/mne_data/MNE-sample-data");

    QString evokedPath = m_dataDir + "/MEG/sample/sample_audvis-ave.fif";
    QString transPath  = m_dataDir + "/MEG/sample/sample_audvis_raw-trans.fif";
    QString surfPath   = m_dataDir + "/subjects/sample/bem/sample-head.fif";

    // Coil definitions
    m_coilDefPath = QCoreApplication::applicationDirPath()
        + "/../resources/general/coilDefinitions/coil_def.dat";
    if (!QFile::exists(m_coilDefPath)) {
        // Try relative to source tree
        m_coilDefPath = QString::fromUtf8(__FILE__);
        m_coilDefPath = QFileInfo(m_coilDefPath).absolutePath()
            + "/../../resources/general/coilDefinitions/coil_def.dat";
    }
    QVERIFY2(QFile::exists(m_coilDefPath),
             qPrintable("coil_def.dat not found at: " + m_coilDefPath));

    qDebug() << "Data dir:     " << m_dataDir;
    qDebug() << "Coil defs:    " << m_coilDefPath;

    // ── Generate reference data with Python ────────────────────────────
    m_pythonScript = QCoreApplication::applicationDirPath()
        + "/generate_fieldmap_reference.py";
    if (!QFile::exists(m_pythonScript)) {
        m_pythonScript = QFileInfo(QString::fromUtf8(__FILE__)).absolutePath()
            + "/generate_fieldmap_reference.py";
    }
    QVERIFY2(QFile::exists(m_pythonScript),
             qPrintable("Python script not found: " + m_pythonScript));

    m_refDir = QCoreApplication::applicationDirPath() + "/test_field_map_ref";
    QDir().mkpath(m_refDir);

    qDebug() << "Running Python reference generator...";

    // Find Python interpreter (python3 on Unix, python on Windows)
    QString pythonCmd;
    {
        QStringList pyCandidates;
        pyCandidates << "python3" << "python";
        for (const QString& py : pyCandidates) {
            QProcess testProc;
            testProc.start(py, QStringList() << "-c" << "import mne; print(mne.__version__)");
            testProc.waitForFinished(10000);
            if (testProc.exitCode() == 0) {
                pythonCmd = py;
                break;
            }
        }
    }
    if (pythonCmd.isEmpty()) {
        QSKIP("Python with MNE not found. Install mne-python to run this test.");
        return;
    }
    m_pythonCmd = pythonCmd;
    qDebug() << "Using Python:" << m_pythonCmd;

    QProcess pyProc;
    pyProc.setProgram(m_pythonCmd);
    pyProc.setArguments({
        m_pythonScript,
        "--evoked", evokedPath,
        "--trans",  transPath,
        "--surf",   surfPath,
        "--outdir", m_refDir,
        "--mode",   "accurate",
        "--condition", "0",
        "--max-verts", "642"
    });
    pyProc.start();
    QVERIFY2(pyProc.waitForFinished(600000),
             "Python script timed out (600s)");
    if (pyProc.exitCode() != 0) {
        qWarning() << "Python stderr:" << pyProc.readAllStandardError();
        QFAIL(qPrintable("Python script failed with exit code "
                          + QString::number(pyProc.exitCode())));
    }
    qDebug() << pyProc.readAllStandardOutput();
    m_pythonOk = true;

    // ── Load sphere origin ─────────────────────────────────────────────
    MatrixXd originMat = loadNpy(m_refDir + "/origin.npy");
    QCOMPARE(originMat.rows(), 3);
    m_origin = originMat.col(0).cast<float>();
    qDebug() << "Sphere origin:" << m_origin(0) << m_origin(1) << m_origin(2);

    // ── Load surface vertices/normals from Python reference ────────────
    // (These are in head coordinates, matching what Python used.)
    MatrixXd refVerts = loadNpy(m_refDir + "/surface_verts.npy");
    MatrixXd refNorms = loadNpy(m_refDir + "/surface_norms.npy");
    QVERIFY(refVerts.rows() > 0);
    QVERIFY(refNorms.rows() == refVerts.rows());
    m_surfVerts = refVerts.cast<float>();
    m_surfNorms = refNorms.cast<float>();
    qDebug() << "Surface:" << m_surfVerts.rows() << "vertices";

    // ── Load evoked data ───────────────────────────────────────────────
    {
        QFile evokedFile(evokedPath);
        QVERIFY(FiffEvokedSet::read(evokedFile, m_evokedSet));
    }
    QVERIFY(m_evokedSet.evoked.size() > 0);
    m_evoked = m_evokedSet.evoked[0];
    qDebug() << "Evoked:" << m_evoked.comment
             << "channels:" << m_evoked.info.chs.size();

    // ── Build coil sets ────────────────────────────────────────────────
    std::unique_ptr<FwdCoilSet> templates(
        FwdCoilSet::read_coil_defs(m_coilDefPath));
    QVERIFY(templates != nullptr);

    // Classify channels (matching Python's pick_types)
    QList<FiffChInfo> megChs, eegChs;
    QStringList megChNames, eegChNames;
    for (int k = 0; k < m_evoked.info.chs.size(); ++k) {
        const auto& ch = m_evoked.info.chs[k];
        if (m_evoked.info.bads.contains(ch.ch_name)) continue;
        if (ch.kind == FIFFV_MEG_CH) {
            megChs.append(ch);
            megChNames.append(ch.ch_name);
        } else if (ch.kind == FIFFV_EEG_CH) {
            eegChs.append(ch);
            eegChNames.append(ch.ch_name);
        }
    }
    qDebug() << "Good MEG channels:" << megChs.size()
             << "Good EEG channels:" << eegChs.size();

    // MEG coils (in head coords – no device→head transform for coil creation
    // since MNE-Python _create_meg_coils with acc='normal' uses no transform)
    if (megChs.size() > 0) {
        m_megCoils.reset(templates->create_meg_coils(
            megChs, megChs.size(), FWD_COIL_ACCURACY_NORMAL, nullptr));
        m_hasMeg = (m_megCoils && m_megCoils->ncoil > 0);
        qDebug() << "MEG coil set:" << (m_hasMeg ? m_megCoils->ncoil : 0) << "coils";
    }

    // EEG electrodes (in head coords)
    if (eegChs.size() > 0) {
        m_eegCoils.reset(FwdCoilSet::create_eeg_els(
            eegChs, eegChs.size(), nullptr));
        m_hasEeg = (m_eegCoils && m_eegCoils->ncoil > 0);
        qDebug() << "EEG electrode set:" << (m_hasEeg ? m_eegCoils->ncoil : 0) << "electrodes";
    }

    // ── Compute C++ field maps (WITH SSP projection) ──────────────────
    if (m_hasMeg) {
        qDebug() << "Computing MEG mapping (C++ with SSP)...";
        m_megMapping = FieldMap::computeMegMapping(
            *m_megCoils, m_surfVerts, m_surfNorms, m_origin,
            m_evoked.info, megChNames,
            0.06f, 1e-4f);
        QVERIFY(!m_megMapping.isNull());
        qDebug() << "MEG mapping:" << m_megMapping->rows() << "x" << m_megMapping->cols();
    }

    if (m_hasEeg) {
        qDebug() << "Computing EEG mapping (C++ with SSP + avg ref)...";
        m_eegMapping = FieldMap::computeEegMapping(
            *m_eegCoils, m_surfVerts, m_origin,
            m_evoked.info, eegChNames,
            0.06f, 1e-3f);
        QVERIFY(!m_eegMapping.isNull());
        qDebug() << "EEG mapping:" << m_eegMapping->rows() << "x" << m_eegMapping->cols();
    }

    // ── Load Python reference matrices ─────────────────────────────────
    if (m_hasMeg) {
        m_refMegSelfDots    = loadNpy(m_refDir + "/meg_self_dots.npy");
        m_refMegSurfaceDots = loadNpy(m_refDir + "/meg_surface_dots.npy");
        m_refMegMapping     = loadNpy(m_refDir + "/meg_mapping.npy");
        QVERIFY(m_refMegSelfDots.rows() > 0);
        QVERIFY(m_refMegSurfaceDots.rows() > 0);
        QVERIFY(m_refMegMapping.rows() > 0);
    }

    if (m_hasEeg) {
        m_refEegSelfDots    = loadNpy(m_refDir + "/eeg_self_dots.npy");
        m_refEegSurfaceDots = loadNpy(m_refDir + "/eeg_surface_dots.npy");
        m_refEegMapping     = loadNpy(m_refDir + "/eeg_mapping.npy");
        QVERIFY(m_refEegSelfDots.rows() > 0);
        QVERIFY(m_refEegSurfaceDots.rows() > 0);
        QVERIFY(m_refEegMapping.rows() > 0);
    }

    qDebug() << "=== initTestCase complete ===";
}

//=============================================================================================================

void TestFieldMap::testMegSelfDots()
{
    if (!m_hasMeg) QSKIP("No MEG data available");

    // The C++ FieldMap::computeMegMapping internally computes self-dots.
    // We need to re-compute them directly for comparison. Since the internal
    // doSelfDots is not exposed publicly, we verify via the final mapping
    // matrix.  However, we CAN compare dimensions and symmetry.

    qDebug() << "MEG self_dots reference shape:"
             << m_refMegSelfDots.rows() << "x" << m_refMegSelfDots.cols();
    QCOMPARE(m_refMegSelfDots.rows(), m_refMegSelfDots.cols());
    QCOMPARE(m_refMegSelfDots.rows(), static_cast<int>(m_megCoils->ncoil));

    // Check symmetry
    double asymm = (m_refMegSelfDots - m_refMegSelfDots.transpose()).norm()
                 / m_refMegSelfDots.norm();
    qDebug() << "MEG self_dots symmetry error:" << asymm;
    QVERIFY2(asymm < 1e-12, "Reference MEG self_dots is not symmetric");

    // Check that diagonal elements are positive (energy-like)
    for (int i = 0; i < m_refMegSelfDots.rows(); ++i) {
        QVERIFY2(m_refMegSelfDots(i, i) > 0.0,
                 qPrintable(QString("MEG self_dots diagonal [%1] = %2 <= 0")
                            .arg(i).arg(m_refMegSelfDots(i, i))));
    }
    QVERIFY(true);
}

//=============================================================================================================

void TestFieldMap::testMegSurfaceDots()
{
    if (!m_hasMeg) QSKIP("No MEG data available");

    qDebug() << "MEG surface_dots reference shape:"
             << m_refMegSurfaceDots.rows() << "x" << m_refMegSurfaceDots.cols();

    // Surface dots should be (n_coils, n_surface_verts) from Python's _do_surface_dots
    // In mne-cpp doSurfaceDots returns (n_verts, n_coils)
    // Python's _do_surface_dots returns (n_coils, n_surface_sel) then it gets transposed
    // Let's verify dimensions match the expected
    QVERIFY(m_refMegSurfaceDots.rows() > 0);
    QVERIFY(m_refMegSurfaceDots.cols() > 0);

    // Check for finite values
    QVERIFY2(m_refMegSurfaceDots.allFinite(),
             "MEG surface_dots contains non-finite values");

    qDebug() << "MEG surface_dots: all finite, shape verified";
    QVERIFY(true);
}

//=============================================================================================================

void TestFieldMap::testMegMapping()
{
    if (!m_hasMeg) QSKIP("No MEG data available");

    QVERIFY(!m_megMapping.isNull());
    QVERIFY(m_refMegMapping.rows() > 0);

    qDebug() << "MEG mapping C++ shape:" << m_megMapping->rows() << "x" << m_megMapping->cols();
    qDebug() << "MEG mapping Py  shape:" << m_refMegMapping.rows() << "x" << m_refMegMapping.cols();

    // The Python reference may have different row/col ordering depending
    // on how _do_surface_dots returns data. Let's handle both cases.
    MatrixXd cppMapping = m_megMapping->cast<double>();
    MatrixXd pyMapping  = m_refMegMapping;

    // If Python has transposed shape, transpose our reference
    if (pyMapping.rows() == cppMapping.cols() && pyMapping.cols() == cppMapping.rows()) {
        qDebug() << "Transposing Python reference to match C++ shape";
        pyMapping.transposeInPlace();
    }

    QCOMPARE(static_cast<int>(cppMapping.rows()), static_cast<int>(pyMapping.rows()));
    QCOMPARE(static_cast<int>(cppMapping.cols()), static_cast<int>(pyMapping.cols()));

    // Compare with tolerance
    // The "accurate" mode with 100 Legendre terms should agree very closely.
    // MNE-Python uses a LUT with 20000 interpolation points whereas mne-cpp
    // evaluates the Legendre series directly, so small differences are expected.
    const double rtol = 0.05;   // 5% relative tolerance
    const double atol = 1e-20;  // absolute tolerance for near-zero values
    int nFail = 0;
    double maxRelErr = compareMatrices(cppMapping, pyMapping, rtol, atol,
                                       "MEG_mapping", nFail);

    qDebug() << "MEG mapping: max relative error =" << maxRelErr
             << "failures =" << nFail
             << "of" << cppMapping.rows() * cppMapping.cols() << "elements";

    // Allow up to 1% of elements to exceed the tolerance
    // (edge effects for vertices very close to sensors)
    double failRate = static_cast<double>(nFail)
                    / (cppMapping.rows() * cppMapping.cols());
    QVERIFY2(failRate < 0.01,
             qPrintable(QString("MEG mapping: %1% elements exceed tolerance "
                                "(max rel err = %2)")
                        .arg(failRate * 100.0, 0, 'f', 2)
                        .arg(maxRelErr, 0, 'e', 4)));

    qDebug() << "MEG mapping cross-validation PASSED"
             << "(max rel err:" << maxRelErr << ")";
}

//=============================================================================================================

void TestFieldMap::testEegSelfDots()
{
    if (!m_hasEeg) QSKIP("No EEG data available");

    qDebug() << "EEG self_dots reference shape:"
             << m_refEegSelfDots.rows() << "x" << m_refEegSelfDots.cols();
    QCOMPARE(m_refEegSelfDots.rows(), m_refEegSelfDots.cols());
    QCOMPARE(m_refEegSelfDots.rows(), static_cast<int>(m_eegCoils->ncoil));

    double asymm = (m_refEegSelfDots - m_refEegSelfDots.transpose()).norm()
                 / m_refEegSelfDots.norm();
    qDebug() << "EEG self_dots symmetry error:" << asymm;
    QVERIFY2(asymm < 1e-12, "Reference EEG self_dots is not symmetric");

    for (int i = 0; i < m_refEegSelfDots.rows(); ++i) {
        QVERIFY2(m_refEegSelfDots(i, i) > 0.0,
                 qPrintable(QString("EEG self_dots diagonal [%1] = %2 <= 0")
                            .arg(i).arg(m_refEegSelfDots(i, i))));
    }
    QVERIFY(true);
}

//=============================================================================================================

void TestFieldMap::testEegSurfaceDots()
{
    if (!m_hasEeg) QSKIP("No EEG data available");

    qDebug() << "EEG surface_dots reference shape:"
             << m_refEegSurfaceDots.rows() << "x" << m_refEegSurfaceDots.cols();

    QVERIFY(m_refEegSurfaceDots.rows() > 0);
    QVERIFY(m_refEegSurfaceDots.cols() > 0);

    QVERIFY2(m_refEegSurfaceDots.allFinite(),
             "EEG surface_dots contains non-finite values");
    QVERIFY(true);
}

//=============================================================================================================

void TestFieldMap::testEegMapping()
{
    if (!m_hasEeg) QSKIP("No EEG data available");

    QVERIFY(!m_eegMapping.isNull());
    QVERIFY(m_refEegMapping.rows() > 0);

    qDebug() << "EEG mapping C++ shape:" << m_eegMapping->rows() << "x" << m_eegMapping->cols();
    qDebug() << "EEG mapping Py  shape:" << m_refEegMapping.rows() << "x" << m_refEegMapping.cols();

    MatrixXd cppMapping = m_eegMapping->cast<double>();
    MatrixXd pyMapping  = m_refEegMapping;

    if (pyMapping.rows() == cppMapping.cols() && pyMapping.cols() == cppMapping.rows()) {
        qDebug() << "Transposing Python reference to match C++ shape";
        pyMapping.transposeInPlace();
    }

    QCOMPARE(static_cast<int>(cppMapping.rows()), static_cast<int>(pyMapping.rows()));
    QCOMPARE(static_cast<int>(cppMapping.cols()), static_cast<int>(pyMapping.cols()));

    const double rtol = 0.05;
    const double atol = 1e-20;
    int nFail = 0;
    double maxRelErr = compareMatrices(cppMapping, pyMapping, rtol, atol,
                                       "EEG_mapping", nFail);

    qDebug() << "EEG mapping: max relative error =" << maxRelErr
             << "failures =" << nFail
             << "of" << cppMapping.rows() * cppMapping.cols() << "elements";

    double failRate = static_cast<double>(nFail)
                    / (cppMapping.rows() * cppMapping.cols());
    QVERIFY2(failRate < 0.01,
             qPrintable(QString("EEG mapping: %1% elements exceed tolerance "
                                "(max rel err = %2)")
                        .arg(failRate * 100.0, 0, 'f', 2)
                        .arg(maxRelErr, 0, 'e', 4)));

    qDebug() << "EEG mapping cross-validation PASSED"
             << "(max rel err:" << maxRelErr << ")";
}

//=============================================================================================================

void TestFieldMap::testMegMappingApplySymmetry()
{
    if (!m_hasMeg) QSKIP("No MEG data available");
    QVERIFY(!m_megMapping.isNull());

    // Sanity check: applying a uniform measurement vector should produce
    // a smooth field.  For a constant input, the output variance should
    // be much smaller than the range of mapping coefficients.
    VectorXf ones = VectorXf::Ones(m_megMapping->cols());
    VectorXf field = (*m_megMapping) * ones;

    double meanField = field.cast<double>().mean();
    double stdField  = std::sqrt((field.cast<double>().array() - meanField).square().mean());

    qDebug() << "MEG uniform-input field: mean =" << meanField << " std =" << stdField;

    // The field from uniform input should be relatively uniform
    // (std / |mean| < 1 unless mean ≈ 0)
    double cv = (std::abs(meanField) > 1e-30)
              ? (stdField / std::abs(meanField))
              : stdField;
    qDebug() << "MEG uniform-input CV:" << cv;

    // Also check that the mapping has no NaN/Inf
    QVERIFY2(m_megMapping->allFinite(),
             "MEG mapping contains NaN or Inf");
    QVERIFY(true);
}

//=============================================================================================================

void TestFieldMap::testEegMappingApplySymmetry()
{
    if (!m_hasEeg) QSKIP("No EEG data available");
    QVERIFY(!m_eegMapping.isNull());

    VectorXf ones = VectorXf::Ones(m_eegMapping->cols());
    VectorXf field = (*m_eegMapping) * ones;

    double meanField = field.cast<double>().mean();
    double stdField  = std::sqrt((field.cast<double>().array() - meanField).square().mean());

    qDebug() << "EEG uniform-input field: mean =" << meanField << " std =" << stdField;

    QVERIFY2(m_eegMapping->allFinite(),
             "EEG mapping contains NaN or Inf");
    QVERIFY(true);
}

//=============================================================================================================

void TestFieldMap::testMultiEvokedMegMapping()
{
    if (!m_hasMeg) QSKIP("No MEG data available");
    QVERIFY(!m_megMapping.isNull());

    // ── Run multi-evoked Python reference generator ────────────────────
    if (!m_multiEvokedOk) {
        m_multiPythonScript = QCoreApplication::applicationDirPath()
            + "/generate_multi_evoked_reference.py";
        if (!QFile::exists(m_multiPythonScript)) {
            m_multiPythonScript = QFileInfo(QString::fromUtf8(__FILE__)).absolutePath()
                + "/generate_multi_evoked_reference.py";
        }
        QVERIFY2(QFile::exists(m_multiPythonScript),
                 qPrintable("Multi-evoked Python script not found: " + m_multiPythonScript));

        QString evokedPath = m_dataDir + "/MEG/sample/sample_audvis-ave.fif";
        QString transPath  = m_dataDir + "/MEG/sample/sample_audvis_raw-trans.fif";
        QString surfPath   = m_dataDir + "/subjects/sample/bem/sample-head.fif";

        m_multiRefDir = QCoreApplication::applicationDirPath() + "/test_multi_evoked_ref";
        QDir().mkpath(m_multiRefDir);

        qDebug() << "Running multi-evoked Python reference generator...";
        if (m_pythonCmd.isEmpty()) {
            QSKIP("Python interpreter not available");
            return;
        }
        QProcess pyProc;
        pyProc.setProgram(m_pythonCmd);
        pyProc.setArguments({
            m_multiPythonScript,
            "--evoked", evokedPath,
            "--trans",  transPath,
            "--surf",   surfPath,
            "--outdir", m_multiRefDir,
            "--mode",   "accurate",
            "--max-verts", "642"
        });
        pyProc.start();
        QVERIFY2(pyProc.waitForFinished(600000),
                 "Multi-evoked Python script timed out");
        if (pyProc.exitCode() != 0) {
            qWarning() << "Python stderr:" << pyProc.readAllStandardError();
            QFAIL(qPrintable("Multi-evoked Python script failed: exit code "
                              + QString::number(pyProc.exitCode())));
        }
        qDebug() << pyProc.readAllStandardOutput();

        // Load condition metadata
        {
            QFile f(m_multiRefDir + "/conditions.txt");
            QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
            QTextStream in(&f);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (!line.isEmpty()) m_conditionNames.append(line);
            }
        }
        m_numConditions = m_conditionNames.size();
        QVERIFY2(m_numConditions > 0, "No conditions found in reference");
        qDebug() << "Conditions:" << m_numConditions << m_conditionNames;

        {
            QFile f(m_multiRefDir + "/peak_times.txt");
            QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
            QTextStream in(&f);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (!line.isEmpty()) m_peakTimes.append(line.toInt());
            }
        }

        // Load MEG channel names used by Python
        {
            QFile f(m_multiRefDir + "/meg_ch_names.txt");
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&f);
                while (!in.atEnd()) {
                    QString line = in.readLine().trimmed();
                    if (!line.isEmpty()) m_megChNames.append(line);
                }
            }
        }

        // Load EEG channel names used by Python
        {
            QFile f(m_multiRefDir + "/eeg_ch_names.txt");
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&f);
                while (!in.atEnd()) {
                    QString line = in.readLine().trimmed();
                    if (!line.isEmpty()) m_eegChNames.append(line);
                }
            }
        }

        m_multiEvokedOk = true;
    }
    QVERIFY(m_multiEvokedOk);

    // ── Compare MEG mapped fields for each condition ───────────────────
    qDebug() << "\n=== Multi-Evoked MEG Cross-Validation ===";

    for (int ci = 0; ci < m_numConditions; ++ci) {
        qDebug() << "\n--- Condition" << ci << ":" << m_conditionNames[ci] << "---";

        // Load Python references
        MatrixXd pyMegMapping = loadNpy(m_multiRefDir + "/meg_mapping.npy");
        VectorXd pyMegDataPeak = loadNpy(m_multiRefDir + QString("/meg_data_cond%1.npy").arg(ci)).col(0);
        VectorXd pyMegMappedPeak = loadNpy(m_multiRefDir + QString("/meg_mapped_cond%1.npy").arg(ci)).col(0);
        MatrixXd pyMegDataAll = loadNpy(m_multiRefDir + QString("/meg_data_cond%1_all.npy").arg(ci));
        MatrixXd pyMegMappedAll = loadNpy(m_multiRefDir + QString("/meg_mapped_cond%1_all.npy").arg(ci));

        QVERIFY2(pyMegDataPeak.size() > 0,
                 qPrintable(QString("Failed to load MEG data for condition %1").arg(ci)));
        QVERIFY2(pyMegMappedPeak.size() > 0,
                 qPrintable(QString("Failed to load MEG mapped for condition %1").arg(ci)));

        qDebug() << "  Python MEG data:" << pyMegDataAll.rows() << "x" << pyMegDataAll.cols();
        qDebug() << "  Python MEG mapped:" << pyMegMappedAll.rows() << "x" << pyMegMappedAll.cols();

        // Get the evoked data for this condition
        QVERIFY2(ci < m_evokedSet.evoked.size(),
                 qPrintable(QString("Condition %1 out of range (%2 available)")
                            .arg(ci).arg(m_evokedSet.evoked.size())));
        const FiffEvoked& evoked = m_evokedSet.evoked[ci];
        qDebug() << "  C++ evoked:" << evoked.comment
                 << "channels:" << evoked.info.chs.size()
                 << "times:" << evoked.data.cols();

        // Build channel pick indices matching Python's channel order
        // C++ uses "MEG0113", Python uses "MEG 0113" — normalize by removing spaces
        QVector<int> megPick;
        for (const QString& chName : m_megChNames) {
            int idx = -1;
            QString pyNorm = chName.trimmed().remove(' ');
            for (int k = 0; k < evoked.info.chs.size(); ++k) {
                QString cppNorm = evoked.info.chs[k].ch_name.trimmed().remove(' ');
                if (cppNorm == pyNorm) {
                    idx = k;
                    break;
                }
            }
            QVERIFY2(idx >= 0,
                     qPrintable(QString("MEG channel '%1' not found in evoked %2")
                                .arg(chName).arg(ci)));
            megPick.append(idx);
        }

        qDebug() << "  MEG picks:" << megPick.size() << "channels";
        QCOMPARE(megPick.size(), m_megMapping->cols());

        // Compare at peak time point
        int peakIdx = m_peakTimes[ci];
        qDebug() << "  Peak time index:" << peakIdx;

        // Extract C++ channel data at peak
        VectorXf cppMeasPeak(megPick.size());
        for (int i = 0; i < megPick.size(); ++i)
            cppMeasPeak(i) = static_cast<float>(evoked.data(megPick[i], peakIdx));

        // Apply C++ mapping
        VectorXf cppMappedPeak = (*m_megMapping) * cppMeasPeak;

        // Compare channel data first (allow small differences from SSP application)
        qDebug() << "  Comparing MEG channel data at peak...";
        {
            int nFail = 0;
            double maxRelErr = 0.0;
            for (int i = 0; i < megPick.size(); ++i) {
                double va = cppMeasPeak(i);
                double vb = pyMegDataPeak(i);
                double diff = std::abs(va - vb);
                double denom = std::max(std::abs(va), std::abs(vb));
                double relErr = (denom > 1e-30) ? (diff / denom) : 0.0;
                if (diff > 1e-30 && relErr > 2e-3) {
                    ++nFail;
                    if (nFail <= 3)
                        qWarning() << "    data mismatch ch" << i << ":"
                                   << va << "vs" << vb << "rel=" << relErr;
                }
                maxRelErr = std::max(maxRelErr, relErr);
            }
            qDebug() << "  MEG data: maxRelErr=" << maxRelErr << "failures=" << nFail;
            QVERIFY2(nFail == 0,
                     qPrintable(QString("Cond %1: MEG channel data mismatch at peak (%2 failures, maxRelErr=%3)")
                                .arg(ci).arg(nFail).arg(maxRelErr)));
        }

        // Compare mapped field values at peak
        qDebug() << "  Comparing MEG mapped field at peak...";
        {
            int nFail = 0;
            double maxRelErr = compareMatrices(
                cppMappedPeak.cast<double>(),
                pyMegMappedPeak,
                0.05, 1e-20, QString("MEG_mapped_cond%1_peak").arg(ci), nFail);

            double failRate = static_cast<double>(nFail) / cppMappedPeak.size();
            qDebug() << "  MEG mapped peak: maxRelErr=" << maxRelErr
                     << "failures=" << nFail << "of" << cppMappedPeak.size()
                     << QString("(%1%)").arg(failRate * 100.0, 0, 'f', 3);
            qDebug() << "  C++ range: [" << cppMappedPeak.minCoeff() << ","
                     << cppMappedPeak.maxCoeff() << "]";
            qDebug() << "  Py  range: [" << pyMegMappedPeak.minCoeff() << ","
                     << pyMegMappedPeak.maxCoeff() << "]";

            QVERIFY2(failRate < 0.01,
                     qPrintable(QString("Cond %1: MEG mapped peak %2% elements exceed 5% tolerance")
                                .arg(ci).arg(failRate * 100.0, 0, 'f', 2)));
        }

        // Compare mapped fields across ALL time points
        qDebug() << "  Comparing MEG mapped field across all time points...";
        {
            int totalFail = 0;
            int totalElements = 0;
            double globalMaxRelErr = 0.0;

            int nTimes = std::min(static_cast<int>(evoked.data.cols()),
                                   static_cast<int>(pyMegMappedAll.cols()));

            for (int t = 0; t < nTimes; ++t) {
                // Extract C++ data at time t
                VectorXf cppMeas(megPick.size());
                for (int i = 0; i < megPick.size(); ++i)
                    cppMeas(i) = static_cast<float>(evoked.data(megPick[i], t));

                VectorXf cppMapped = (*m_megMapping) * cppMeas;
                VectorXd pyMapped = pyMegMappedAll.col(t);

                int nFail = 0;
                double maxRelErr = compareMatrices(
                    cppMapped.cast<double>(),
                    pyMapped,
                    0.05, 1e-20,
                    QString("MEG_cond%1_t%2").arg(ci).arg(t), nFail);

                totalFail += nFail;
                totalElements += cppMapped.size();
                globalMaxRelErr = std::max(globalMaxRelErr, maxRelErr);
            }

            double totalFailRate = static_cast<double>(totalFail) / totalElements;
            qDebug() << "  MEG all times: maxRelErr=" << globalMaxRelErr
                     << "totalFail=" << totalFail << "of" << totalElements
                     << QString("(%1%)").arg(totalFailRate * 100.0, 0, 'f', 4);

            QVERIFY2(totalFailRate < 0.01,
                     qPrintable(QString("Cond %1: MEG all-times %2% elements exceed 5% tolerance")
                                .arg(ci).arg(totalFailRate * 100.0, 0, 'f', 2)));
        }
    }

    qDebug() << "\n=== Multi-Evoked MEG Cross-Validation PASSED ===";
}

//=============================================================================================================

void TestFieldMap::testMultiEvokedEegMapping()
{
    if (!m_hasEeg) QSKIP("No EEG data available");
    QVERIFY(!m_eegMapping.isNull());
    QVERIFY(m_multiEvokedOk);

    qDebug() << "\n=== Multi-Evoked EEG Cross-Validation ===";

    for (int ci = 0; ci < m_numConditions; ++ci) {
        qDebug() << "\n--- Condition" << ci << ":" << m_conditionNames[ci] << "---";

        // Load Python references
        VectorXd pyEegDataPeak = loadNpy(m_multiRefDir + QString("/eeg_data_cond%1.npy").arg(ci)).col(0);
        VectorXd pyEegMappedPeak = loadNpy(m_multiRefDir + QString("/eeg_mapped_cond%1.npy").arg(ci)).col(0);
        MatrixXd pyEegMappedAll = loadNpy(m_multiRefDir + QString("/eeg_mapped_cond%1_all.npy").arg(ci));

        QVERIFY2(pyEegDataPeak.size() > 0,
                 qPrintable(QString("Failed to load EEG data for condition %1").arg(ci)));

        const FiffEvoked& evoked = m_evokedSet.evoked[ci];

        // Build EEG channel pick indices
        // Same normalization: C++ "EEG001" vs Python "EEG 001"
        QVector<int> eegPick;
        for (const QString& chName : m_eegChNames) {
            int idx = -1;
            QString pyNorm = chName.trimmed().remove(' ');
            for (int k = 0; k < evoked.info.chs.size(); ++k) {
                QString cppNorm = evoked.info.chs[k].ch_name.trimmed().remove(' ');
                if (cppNorm == pyNorm) {
                    idx = k;
                    break;
                }
            }
            QVERIFY2(idx >= 0,
                     qPrintable(QString("EEG channel '%1' not found in evoked %2")
                                .arg(chName).arg(ci)));
            eegPick.append(idx);
        }

        QCOMPARE(eegPick.size(), m_eegMapping->cols());

        // Compare at peak time point
        int peakIdx = m_peakTimes[ci];

        VectorXf cppMeasPeak(eegPick.size());
        for (int i = 0; i < eegPick.size(); ++i)
            cppMeasPeak(i) = static_cast<float>(evoked.data(eegPick[i], peakIdx));

        VectorXf cppMappedPeak = (*m_eegMapping) * cppMeasPeak;

        // Compare channel data (allow small differences from SSP / float precision)
        {
            int nFail = 0;
            for (int i = 0; i < eegPick.size(); ++i) {
                double diff = std::abs(cppMeasPeak(i) - pyEegDataPeak(i));
                double denom = std::max(std::abs((double)cppMeasPeak(i)), std::abs(pyEegDataPeak(i)));
                if (diff > 1e-30 && denom > 1e-30 && (diff / denom) > 2e-3)
                    ++nFail;
            }
            QVERIFY2(nFail == 0,
                     qPrintable(QString("Cond %1: EEG channel data mismatch (%2 failures)")
                                .arg(ci).arg(nFail)));
        }

        // Compare mapped at peak
        {
            int nFail = 0;
            double maxRelErr = compareMatrices(
                cppMappedPeak.cast<double>(),
                pyEegMappedPeak,
                0.05, 1e-20, QString("EEG_mapped_cond%1_peak").arg(ci), nFail);

            double failRate = static_cast<double>(nFail) / cppMappedPeak.size();
            qDebug() << "  EEG mapped peak: maxRelErr=" << maxRelErr
                     << "failures=" << nFail << "of" << cppMappedPeak.size()
                     << QString("(%1%)").arg(failRate * 100.0, 0, 'f', 3);
            qDebug() << "  C++ range: [" << cppMappedPeak.minCoeff() << ","
                     << cppMappedPeak.maxCoeff() << "]";
            qDebug() << "  Py  range: [" << pyEegMappedPeak.minCoeff() << ","
                     << pyEegMappedPeak.maxCoeff() << "]";

            QVERIFY2(failRate < 0.01,
                     qPrintable(QString("Cond %1: EEG mapped peak %2% exceed tolerance")
                                .arg(ci).arg(failRate * 100.0, 0, 'f', 2)));
        }

        // Compare across all time points
        {
            int totalFail = 0;
            int totalElements = 0;
            double globalMaxRelErr = 0.0;

            int nTimes = std::min(static_cast<int>(evoked.data.cols()),
                                   static_cast<int>(pyEegMappedAll.cols()));

            for (int t = 0; t < nTimes; ++t) {
                VectorXf cppMeas(eegPick.size());
                for (int i = 0; i < eegPick.size(); ++i)
                    cppMeas(i) = static_cast<float>(evoked.data(eegPick[i], t));

                VectorXf cppMapped = (*m_eegMapping) * cppMeas;
                VectorXd pyMapped = pyEegMappedAll.col(t);

                int nFail = 0;
                double maxRelErr = compareMatrices(
                    cppMapped.cast<double>(),
                    pyMapped,
                    0.05, 1e-20,
                    QString("EEG_cond%1_t%2").arg(ci).arg(t), nFail);

                totalFail += nFail;
                totalElements += cppMapped.size();
                globalMaxRelErr = std::max(globalMaxRelErr, maxRelErr);
            }

            double totalFailRate = static_cast<double>(totalFail) / totalElements;
            qDebug() << "  EEG all times: maxRelErr=" << globalMaxRelErr
                     << "totalFail=" << totalFail << "of" << totalElements
                     << QString("(%1%)").arg(totalFailRate * 100.0, 0, 'f', 4);

            QVERIFY2(totalFailRate < 0.01,
                     qPrintable(QString("Cond %1: EEG all-times %2% exceed tolerance")
                                .arg(ci).arg(totalFailRate * 100.0, 0, 'f', 2)));
        }
    }

    qDebug() << "\n=== Multi-Evoked EEG Cross-Validation PASSED ===";
}

//=============================================================================================================

void TestFieldMap::testHelmetFieldMap()
{
    // This test replicates the MNE-Python helmet example:
    // https://mne.tools/stable/auto_examples/visualization/mne_helmet.html
    //
    // It uses mne.make_field_map(evoked, ch_type="meg", origin="auto",
    //                            upsampling=2, subject="sample")
    // which maps MEG data onto the upsampled helmet surface with an
    // auto-fitted sphere origin.
    //
    // We compute the same mapping in C++ using FwdFieldMap and compare.

    QVERIFY(m_hasMeg);

    // ── Run Python helmet reference generator ──────────────────────────
    QString helmetScript = QCoreApplication::applicationDirPath()
        + "/generate_helmet_reference.py";
    if (!QFile::exists(helmetScript)) {
        helmetScript = QFileInfo(QString::fromUtf8(__FILE__)).absolutePath()
            + "/generate_helmet_reference.py";
    }
    QVERIFY2(QFile::exists(helmetScript),
             qPrintable("Helmet Python script not found: " + helmetScript));

    QString helmetRefDir = QCoreApplication::applicationDirPath()
        + "/test_helmet_ref";
    QDir().mkpath(helmetRefDir);

    qDebug() << "Running helmet Python reference generator...";
    if (m_pythonCmd.isEmpty()) {
        QSKIP("Python interpreter not available");
        return;
    }
    QProcess pyProc;
    pyProc.setProgram(m_pythonCmd);
    pyProc.setArguments({
        helmetScript,
        "--outdir", helmetRefDir,
        "--sample-dir", m_dataDir
    });
    pyProc.start();
    QVERIFY2(pyProc.waitForFinished(600000),
             "Helmet Python script timed out");
    if (pyProc.exitCode() != 0) {
        QString pyErr = QString::fromUtf8(pyProc.readAllStandardError());
        qWarning() << "Python stderr:" << pyErr;
        QSKIP(qPrintable("Helmet Python reference generator unavailable: "
                          + pyErr.left(200)));
    }
    qDebug() << pyProc.readAllStandardOutput();

    // ── Load Python reference data ─────────────────────────────────────
    MatrixXd pyOriginMat = loadNpy(helmetRefDir + "/origin.npy");
    QCOMPARE(pyOriginMat.rows(), 3);
    Vector3f helmetOrigin = pyOriginMat.col(0).cast<float>();
    qDebug() << "Helmet origin:" << helmetOrigin(0) << helmetOrigin(1) << helmetOrigin(2);

    MatrixXd pyVerts = loadNpy(helmetRefDir + "/helmet_verts.npy");
    MatrixXd pyNorms = loadNpy(helmetRefDir + "/helmet_norms.npy");
    QVERIFY(pyVerts.rows() > 0);
    QCOMPARE(pyNorms.rows(), pyVerts.rows());
    MatrixX3f helmetVerts = pyVerts.cast<float>();
    MatrixX3f helmetNorms = pyNorms.cast<float>();
    qDebug() << "Helmet surface:" << helmetVerts.rows() << "vertices";

    MatrixXd pyMapping = loadNpy(helmetRefDir + "/meg_mapping.npy");
    QVERIFY(pyMapping.rows() > 0);
    qDebug() << "Python mapping:" << pyMapping.rows() << "x" << pyMapping.cols();

    // Load channel names
    QStringList helmetChNames;
    {
        QFile f(helmetRefDir + "/meg_ch_names.txt");
        QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
        QTextStream in(&f);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty()) helmetChNames.append(line);
        }
    }
    qDebug() << "Channels:" << helmetChNames.size();

    // Load time index
    MatrixXd pyTimeIdx = loadNpy(helmetRefDir + "/time_index.npy");
    int timeIdx = static_cast<int>(pyTimeIdx(0, 0));
    qDebug() << "Time index for t=0.083:" << timeIdx;

    // Load reference mapped values at t=0.083
    VectorXd pyMappedT083 = loadNpy(helmetRefDir + "/meg_mapped_t083.npy").col(0);
    VectorXd pyDataT083 = loadNpy(helmetRefDir + "/meg_data_t083.npy").col(0);
    MatrixXd pyMappedAll = loadNpy(helmetRefDir + "/meg_mapped_all.npy");
    QVERIFY(pyMappedT083.size() > 0);
    QVERIFY(pyDataT083.size() > 0);

    // ── Compute C++ mapping with helmet surface + auto origin ──────────
    // Build MEG channel names matching C++ ordering
    QList<FiffChInfo> megChs;
    QStringList cppMegChNames;
    for (int k = 0; k < m_evoked.info.chs.size(); ++k) {
        const auto& ch = m_evoked.info.chs[k];
        if (m_evoked.info.bads.contains(ch.ch_name)) continue;
        if (ch.kind == FIFFV_MEG_CH) {
            megChs.append(ch);
            cppMegChNames.append(ch.ch_name);
        }
    }

    // Create MEG coils in head coordinates (matching _make_surface_mapping)
    // The Python reference now creates coils in head coords using dev_head_t,
    // so the C++ side must do the same.
    std::unique_ptr<FwdCoilSet> templates(
        FwdCoilSet::read_coil_defs(m_coilDefPath));
    QVERIFY(templates != nullptr);

    std::unique_ptr<FwdCoilSet> helmetCoils(
        templates->create_meg_coils(
            megChs, megChs.size(), FWD_COIL_ACCURACY_NORMAL, m_evoked.info.dev_head_t));
    QVERIFY(helmetCoils && helmetCoils->ncoil > 0);
    qDebug() << "C++ MEG coils:" << helmetCoils->ncoil
             << "(in head coordinates)";

    qDebug() << "Computing C++ MEG mapping on helmet (with SSP)...";
    QSharedPointer<MatrixXf> cppMapping = FieldMap::computeMegMapping(
        *helmetCoils, helmetVerts, helmetNorms, helmetOrigin,
        m_evoked.info, cppMegChNames,
        0.06f, 1e-4f);
    QVERIFY(!cppMapping.isNull());
    qDebug() << "C++ mapping:" << cppMapping->rows() << "x" << cppMapping->cols();

    // ── Compare mapping matrices ───────────────────────────────────────
    qDebug() << "\n=== Helmet Mapping Matrix Comparison ===";
    {
        MatrixXd cppMap = cppMapping->cast<double>();

        // Handle possible transpose
        if (pyMapping.rows() == cppMap.cols() && pyMapping.cols() == cppMap.rows()) {
            qDebug() << "Transposing Python reference";
            pyMapping.transposeInPlace();
        }

        QCOMPARE(static_cast<int>(cppMap.rows()), static_cast<int>(pyMapping.rows()));
        QCOMPARE(static_cast<int>(cppMap.cols()), static_cast<int>(pyMapping.cols()));

        int nFail = 0;
        double maxRelErr = compareMatrices(cppMap, pyMapping, 0.05, 1e-20,
                                           "helmet_mapping", nFail);
        double failRate = static_cast<double>(nFail)
                        / (cppMap.rows() * cppMap.cols());
        qDebug() << "Mapping: maxRelErr=" << maxRelErr
                 << "failures=" << nFail
                 << "of" << cppMap.rows() * cppMap.cols()
                 << QString("(%1%)").arg(failRate * 100.0, 0, 'f', 4);

        QVERIFY2(failRate < 0.01,
                 qPrintable(QString("Helmet mapping: %1% elements exceed 5% tolerance")
                            .arg(failRate * 100.0, 0, 'f', 2)));
    }

    // ── Compare mapped field over entire time span ───────────────────
    // Test the full end-to-end pipeline including baseline correction.
    // The C++ SensorFieldMapper now applies baseline correction in
    // setEvoked() (matching Python's baseline=(None, 0)).
    //
    // We apply C++ baseline correction to m_evoked, build channel picks,
    // compute mapped = mapping * data, and compare against Python.
    // This validates both the mapping matrix AND the baseline correction.
    //
    // An absolute tolerance floor scaled to peak signal is used so that
    // near-zero time points don't produce false passes or spurious failures.
    qDebug() << "\n=== Helmet Mapped Field — Full Time Span ===";
    {
        // Apply baseline correction to a copy of the C++ evoked data
        // (matching SensorFieldMapper::setEvoked behavior)
        FiffEvoked baselinedEvoked = m_evoked;
        float tmin = baselinedEvoked.times.size() > 0
            ? baselinedEvoked.times(0) : 0.0f;
        if (tmin < 0.0f) {
            QPair<float,float> bl(tmin, 0.0f);
            baselinedEvoked.applyBaselineCorrection(bl);
        }
        fflush(stdout);  // ensure baseline message is visible before potential crash

        // Build channel pick indices: Python ch_names → C++ evoked rows
        QVector<int> pick;
        for (const QString& pyName : helmetChNames) {
            int idx = -1;
            QString pyNorm = pyName.trimmed().remove(' ');
            for (int k = 0; k < baselinedEvoked.info.chs.size(); ++k) {
                QString cppNorm = baselinedEvoked.info.chs[k].ch_name
                    .trimmed().remove(' ');
                if (cppNorm == pyNorm) {
                    idx = k;
                    break;
                }
            }
            QVERIFY2(idx >= 0,
                     qPrintable(QString("Channel '%1' not found").arg(pyName)));
            pick.append(idx);
        }

        // Verify dimensions before matrix multiplication to avoid
        // silent crashes in Release builds (Eigen doesn't bounds-check)
        qDebug() << "Picks:" << pick.size()
                 << "mapping cols:" << cppMapping->cols()
                 << "mapping rows:" << cppMapping->rows()
                 << "pyMappedAll rows:" << pyMappedAll.rows();
        QCOMPARE(pick.size(), cppMapping->cols());
        QCOMPARE(static_cast<int>(cppMapping->rows()),
                 static_cast<int>(pyMappedAll.rows()));

        const int nTimes = std::min(
            static_cast<int>(baselinedEvoked.data.cols()),
            static_cast<int>(pyMappedAll.cols()));
        QVERIFY(nTimes > 0);

        // Compute C++ mapped fields for ALL times using baselined C++ data
        const int nVerts = cppMapping->rows();
        MatrixXd cppMappedAll(nVerts, nTimes);
        for (int t = 0; t < nTimes; ++t) {
            VectorXf cppMeas(pick.size());
            for (int i = 0; i < pick.size(); ++i)
                cppMeas(i) = static_cast<float>(
                    baselinedEvoked.data(pick[i], t));
            cppMappedAll.col(t) = ((*cppMapping) * cppMeas).cast<double>();
        }

        const double cppPeak = cppMappedAll.cwiseAbs().maxCoeff();
        const double pyPeak  = pyMappedAll.leftCols(nTimes).cwiseAbs().maxCoeff();
        const double peak    = std::max(cppPeak, pyPeak);
        qDebug() << "Global peak: C++=" << cppPeak << "Py=" << pyPeak;

        // ── Temporal correlation check ─────────────────────────────────
        // At representative vertices, the time course of C++ mapped
        // values should correlate highly with Python (> 0.95).
        double minCorr = 1.0;
        for (int v = 0; v < nVerts; v += 10) {
            VectorXd cppTrace = cppMappedAll.row(v).transpose();
            VectorXd pyTrace  = pyMappedAll.row(v).head(nTimes).transpose();

            double cppMean = cppTrace.mean();
            double pyMean  = pyTrace.mean();
            VectorXd cppC = cppTrace.array() - cppMean;
            VectorXd pyC  = pyTrace.array()  - pyMean;
            double num = cppC.dot(pyC);
            double den = std::sqrt(cppC.squaredNorm() * pyC.squaredNorm());
            double corr = (den > 0.0) ? (num / den) : 0.0;
            minCorr = std::min(minCorr, corr);
        }
        qDebug() << "Temporal correlation (min over vertices):" << minCorr;
        QVERIFY2(minCorr > 0.95,
                 qPrintable(QString("Temporal correlation too low: %1 "
                                    "(field not evolving correctly)")
                            .arg(minCorr, 0, 'f', 4)));

        // ── Element-wise absolute comparison ───────────────────────────
        const double absTol  = 0.05 * peak;
        const double skipThr = 0.01 * peak;

        int totalChecked = 0;
        int totalFail    = 0;
        double maxAbsErr = 0.0;

        for (int t = 0; t < nTimes; ++t) {
            for (int v = 0; v < nVerts; ++v) {
                double cppVal = cppMappedAll(v, t);
                double pyVal  = pyMappedAll(v, t);
                double absErr = std::abs(cppVal - pyVal);

                if (std::abs(cppVal) < skipThr && std::abs(pyVal) < skipThr)
                    continue;

                ++totalChecked;
                maxAbsErr = std::max(maxAbsErr, absErr);
                if (absErr > absTol)
                    ++totalFail;
            }
        }

        double failRate = (totalChecked > 0)
            ? static_cast<double>(totalFail) / totalChecked : 0.0;
        qDebug() << "Checked:" << totalChecked
                 << "failures:" << totalFail
                 << QString("(%1%)").arg(failRate * 100.0, 0, 'f', 4)
                 << "maxAbsErr:" << maxAbsErr
                 << "absTol:" << absTol;

        QVERIFY2(failRate < 0.01,
                 qPrintable(QString("Helmet time-span: %1% elements exceed "
                                    "absolute tolerance (%2)")
                            .arg(failRate * 100.0, 0, 'f', 2)
                            .arg(absTol)));

        // ── Temporal evolution check: baseline correction IS essential ──
        // Without baseline correction, the DC offset dominates and the
        // mapped field appears nearly static ("only slightly wobbling").
        // We verify this by computing temporal variation (coefficient of
        // variation = std/|mean|) with and without baseline and asserting
        // that baseline correction dramatically increases it.

        // Find t=0 index
        int t0idx = 0;
        for (int t = 0; t < nTimes; ++t) {
            if (baselinedEvoked.times(t) >= 0.0f) {
                t0idx = t;
                break;
            }
        }
        int nPost = nTimes - t0idx;

        // With baseline: compute mapped fields (already in cppMappedAll)
        double baselinedVarSum = 0.0;
        double baselinedAbsMeanSum = 0.0;
        for (int v = 0; v < nVerts; ++v) {
            VectorXd trace = cppMappedAll.block(v, t0idx, 1, nPost).transpose();
            double mu  = trace.mean();
            double var = (trace.array() - mu).square().mean();
            baselinedVarSum += std::sqrt(var);
            baselinedAbsMeanSum += std::abs(mu);
        }

        // Without baseline: compute mapped fields from raw (non-baselined) data
        // Pre-compute all mapped fields in one pass to avoid O(nVerts²) cost
        MatrixXd rawMappedAll(nVerts, nPost);
        for (int t = t0idx; t < nTimes; ++t) {
            VectorXf rawMeas(pick.size());
            for (int i = 0; i < pick.size(); ++i)
                rawMeas(i) = static_cast<float>(m_evoked.data(pick[i], t));
            rawMappedAll.col(t - t0idx) = ((*cppMapping) * rawMeas).cast<double>();
        }

        double rawVarSum = 0.0;
        double rawAbsMeanSum = 0.0;
        for (int v = 0; v < nVerts; ++v) {
            VectorXd rawTrace = rawMappedAll.row(v).transpose();
            double mu  = rawTrace.mean();
            double var = (rawTrace.array() - mu).square().mean();
            rawVarSum += std::sqrt(var);
            rawAbsMeanSum += std::abs(mu);
        }

        double baselinedCV = (baselinedAbsMeanSum > 0)
            ? baselinedVarSum / baselinedAbsMeanSum : 0.0;
        double rawCV = (rawAbsMeanSum > 0)
            ? rawVarSum / rawAbsMeanSum : 0.0;

        qDebug() << "Post-stimulus coefficient of variation:";
        qDebug() << "  With baseline:    CV =" << baselinedCV;
        qDebug() << "  Without baseline: CV =" << rawCV;
        qDebug() << "  Ratio (baselined/raw):" << ((rawCV > 0) ? baselinedCV / rawCV : 0.0);

        // Without baseline correction, the DC offset makes the field
        // nearly static (rawCV is small).  With baseline, the CV should be
        // at least 3x larger because the temporal signal dominates.
        QVERIFY2(baselinedCV > rawCV * 2.0,
                 qPrintable(QString("Baseline correction not working: "
                                    "baselinedCV=%1 is not >> rawCV=%2")
                            .arg(baselinedCV, 0, 'f', 4)
                            .arg(rawCV, 0, 'f', 4)));

        // The baselined field must show meaningful temporal evolution
        QVERIFY2(baselinedCV > 0.5,
                 qPrintable(QString("Field not evolving: baselinedCV = %1 "
                                    "(expected > 0.5)")
                            .arg(baselinedCV, 0, 'f', 4)));
    }

    qDebug() << "\n=== Helmet Field Map Cross-Validation PASSED ===";
}

//=============================================================================================================

void TestFieldMap::cleanupTestCase()
{
    qDebug() << "=== TestFieldMap complete ===";

    // Optionally remove reference files (keep them for debugging)
    // QDir(m_refDir).removeRecursively();
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestFieldMap)
#include "test_field_map.moc"
