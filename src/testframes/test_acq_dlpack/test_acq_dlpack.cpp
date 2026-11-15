//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_acq_dlpack.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Tests for acq_dlpack.h: tensor descriptors per format, layout and memory kind, and a NumPy round trip.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <acq/acq_dlpack.h>

#include <QProcess>
#include <QtTest/QtTest>

#include <cstdint>
#include <memory>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace std::chrono_literals;

extern "C" DLManagedTensorVersioned* acq_dlpack_probe_export(int format, int planar, int channels, int samples);
extern "C" int acq_dlpack_probe_release();

namespace
{
AcqStreamInfo makeInfo(AcqSampleFormat format, std::size_t channels)
{
    AcqStreamInfo info;
    info.name = "test";
    info.format = format;
    info.sfreq = 1000.0;
    for (std::size_t c = 0; c < channels; ++c) {
        AcqChannelInfo ch;
        ch.name = "ch" + std::to_string(c);
        info.channels.push_back(ch);
    }
    return info;
}

// Consumer side: wraps the probe's tensor in a capsule and lets NumPy read it, for every format and layout.
constexpr const char* kNumPyScript = R"(
import ctypes, os, sys
import numpy as np
probe, libdir = sys.argv[1], sys.argv[2]
if hasattr(os, "add_dll_directory"):
    os.add_dll_directory(libdir)
lib = ctypes.CDLL(probe)
lib.acq_dlpack_probe_export.restype = ctypes.c_void_p
lib.acq_dlpack_probe_export.argtypes = [ctypes.c_int] * 4
capsule = ctypes.pythonapi.PyCapsule_New
capsule.restype = ctypes.py_object
capsule.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_void_p]

class View:
    def __init__(self, ptr):
        self.ptr = ptr
    def __dlpack__(self, stream=None, max_version=None, dl_device=None, copy=None):
        return capsule(self.ptr, b"dltensor_versioned", None)
    def __dlpack_device__(self):
        return (1, 0)

channels, samples = 5, 37
expected = np.arange(samples)[None, :] * 16.0 + np.arange(channels)[:, None] - 100.0
formats = {1: np.int16, 3: np.int32, 4: np.float32, 5: np.float64, 7: np.complex64}
for code, dtype in formats.items():
    for planar in (0, 1):
        ptr = lib.acq_dlpack_probe_export(code, planar, channels, samples)
        assert ptr, (code, planar)
        a = np.from_dlpack(View(ptr))
        want = (expected - 1j * expected) if code == 7 else expected
        assert a.dtype == dtype and a.shape == (channels, samples), (code, planar, a.dtype, a.shape)
        assert not a.flags.writeable, (code, planar)
        assert np.array_equal(a, want.astype(dtype)), (code, planar)
        del a
        assert lib.acq_dlpack_probe_release() == 0, (code, planar)
print("NUMPY_OK")
)";
} // namespace

//=============================================================================================================

class TestAcqDlpack : public QObject
{
    Q_OBJECT

private slots:
    void testDescriptor_data();
    void testDescriptor();
    void testUnsupported();
    void testMemoryKinds();
    void testNumPyRoundTrip();
};

//=============================================================================================================

void TestAcqDlpack::testDescriptor_data()
{
    QTest::addColumn<int>("format");
    QTest::addColumn<bool>("planar");
    QTest::addColumn<int>("code");
    QTest::addColumn<int>("bits");
    const struct
    {
        AcqSampleFormat format;
        DLDataTypeCode code;
        int bits;
    } kinds[] = {{AcqSampleFormat::Int16, kDLInt, 16},
                 {AcqSampleFormat::Int32, kDLInt, 32},
                 {AcqSampleFormat::Float32, kDLFloat, 32},
                 {AcqSampleFormat::Float64, kDLFloat, 64},
                 {AcqSampleFormat::ComplexFloat32, kDLComplex, 64}};
    for (const auto& k : kinds) {
        for (const bool planar : {false, true}) {
            QTest::newRow(QString("%1 %2").arg(int(k.format)).arg(planar ? "planar" : "interleaved").toLatin1().constData())
                << int(k.format) << planar << int(k.code) << k.bits;
        }
    }
}

void TestAcqDlpack::testDescriptor()
{
    QFETCH(int, format);
    QFETCH(bool, planar);
    QFETCH(int, code);
    QFETCH(int, bits);
    DLManagedTensorVersioned* m = acq_dlpack_probe_export(format, planar ? 1 : 0, 5, 37);
    QVERIFY(m != nullptr);
    const DLTensor& t = m->dl_tensor;
    QCOMPARE(m->version.major, std::uint32_t(DLPACK_MAJOR_VERSION));
    QCOMPARE(m->flags & DLPACK_FLAG_BITMASK_READ_ONLY, std::uint64_t(DLPACK_FLAG_BITMASK_READ_ONLY));
    QCOMPARE(int(t.device.device_type), int(kDLCPU));
    QCOMPARE(t.ndim, 2);
    QCOMPARE(int(t.dtype.code), code);
    QCOMPARE(int(t.dtype.bits), bits);
    QCOMPARE(int(t.dtype.lanes), 1);
    QCOMPARE(t.shape[0], std::int64_t(5));
    QCOMPARE(t.shape[1], std::int64_t(37));
    if (planar) {
        QCOMPARE(t.strides[1], std::int64_t(1));
        QVERIFY(t.strides[0] >= 37);
    } else {
        QCOMPARE(t.strides[0], std::int64_t(1));
        QCOMPARE(t.strides[1], std::int64_t(5));
    }
    QCOMPARE(t.byte_offset, std::uint64_t(0));
    m->deleter(m);
    QCOMPARE(acq_dlpack_probe_release(), 0);
}

//=============================================================================================================

void TestAcqDlpack::testUnsupported()
{
    for (const AcqSampleFormat format : {AcqSampleFormat::Int24Packed, AcqSampleFormat::ComplexInt16}) {
        auto writer = AcqWriter::create(makeInfo(format, 2));
        QVERIFY(writer);
        auto reader = writer->stream().reader(AcqReader::ReadMode::Lossless);
        QVERIFY(writer->beginWrite(4));
        writer->commit(4);
        auto view = reader->acquire(4, 0ns);
        QVERIFY(view);
        QVERIFY(toDLPack(*view).error() == AcqErrorCode::UnsupportedFormat);
        QVERIFY(reader->release(*view).ok());
    }
    auto writer = AcqWriter::create(makeInfo(AcqSampleFormat::Float32, 2));
    auto reader = writer->stream().reader(AcqReader::ReadMode::Lossless);
    QVERIFY(writer->beginWrite(4));
    writer->commit(4);
    auto view = reader->acquire(4, 0ns);
    QVERIFY(view && view->partCount() == 1);
    QVERIFY(toDLPack(*view, 1).error() == AcqErrorCode::InvalidParameter);
    QVERIFY(reader->release(*view).ok());
}

//=============================================================================================================

void TestAcqDlpack::testMemoryKinds()
{
    // Pinned host, GPU and managed memory map to the CUDA device types with the ring's device ordinal. The
    // samples are never touched, so plain host memory stands in for them.
    constexpr std::size_t kCapacity = 1024;
    constexpr std::size_t kBytes = kCapacity * 3 * sizeof(float);
    const struct
    {
        AcqMemoryKind kind;
        DLDeviceType type;
    } kinds[] = {{AcqMemoryKind::HostPinned, kDLCUDAHost},
                 {AcqMemoryKind::Device, kDLCUDA},
                 {AcqMemoryKind::Managed, kDLCUDAManaged}};
    for (const auto& k : kinds) {
        std::unique_ptr<std::byte[], void (*)(std::byte*)> buffer(
            static_cast<std::byte*>(::operator new[](kBytes, std::align_val_t(4096))),
            [](std::byte* p) { ::operator delete[](p, std::align_val_t(4096)); });
        AcqStreamOptions options;
        options.lockMemory = false;
        options.prefault = false;
        options.external.data = buffer.get();
        options.external.bytes = kBytes;
        options.external.memory = {k.kind, 2};
        auto writer = AcqWriter::create(makeInfo(AcqSampleFormat::Float32, 3), options);
        QVERIFY(writer);
        auto reader = writer->stream().reader(AcqReader::ReadMode::Lossless);
        QVERIFY(writer->grant(8).ok());
        writer->commit(8);
        auto view = reader->acquire(8, 0ns);
        QVERIFY(view);
        auto tensor = toDLPack(*view);
        QVERIFY(tensor);
        QCOMPARE(int((*tensor)->dl_tensor.device.device_type), int(k.type));
        QCOMPARE((*tensor)->dl_tensor.device.device_id, 2);
        QCOMPARE(static_cast<const void*>((*tensor)->dl_tensor.data), static_cast<const void*>(buffer.get()));
        (*tensor)->deleter(*tensor);
        QVERIFY(reader->release(*view).ok());
    }
}

//=============================================================================================================

void TestAcqDlpack::testNumPyRoundTrip()
{
    if (QByteArray(ACQ_DLPACK_PROBE_PATH).isEmpty()) {
        QSKIP("no loadable probe library in this build (sanitizers)");
    }
    const QStringList check = {"-c", "import numpy, sys; sys.exit(0 if hasattr(numpy, 'from_dlpack') else 1)"};
    QString python;
    for (const QString& candidate : {QStringLiteral("python3"), QStringLiteral("python")}) {
        QProcess probe;
        probe.start(candidate, check);
        if (probe.waitForFinished(60000) && probe.exitStatus() == QProcess::NormalExit && probe.exitCode() == 0) {
            python = candidate;
            break;
        }
    }
    if (python.isEmpty()) {
        QSKIP("no Python with NumPy >= 1.22 on PATH");
    }
    QProcess run;
    run.setProcessChannelMode(QProcess::MergedChannels);
    run.start(python, {"-c", kNumPyScript, ACQ_DLPACK_PROBE_PATH, ACQ_LIBRARY_DIR});
    QVERIFY(run.waitForFinished(120000));
    const QByteArray output = run.readAll();
    QVERIFY2(run.exitCode() == 0 && output.contains("NUMPY_OK"), output.constData());
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestAcqDlpack)
#include "test_acq_dlpack.moc"
