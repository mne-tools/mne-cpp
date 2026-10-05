//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    BIDS library: paths, sidecar TSVs, BrainVision/EDF readers and a raw-data round trip.
 *
 * Reads the BIDS dataset of the MNE-CPP test data and compares counts and
 * samples with MNE-Python 1.11 (read_raw_brainvision, read_raw_edf). Exits
 * non-zero on any mismatch.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <bids/bids_electrode.h>
#include <bids/bids_path.h>
#include <bids/bids_raw_data.h>
#include <bids/bids_tsv.h>
#include <bids/readers/bids_abstract_format_reader.h>
#include <bids/readers/bids_brain_vision_reader.h>
#include <bids/readers/bids_edf_reader.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QFileInfo>
#include <QTemporaryDir>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <cstdlib>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BIDSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

bool expect(bool condition, const QString& what)
{
    qInfo().noquote() << (condition ? "  ok  " : "  FAIL") << what;
    return condition;
}

bool near(double value, double expected, double relTol)
{
    return std::fabs(value - expected) <= relTol * std::fabs(expected);
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    QCommandLineOption rootOption("bidsRoot", "BIDS dataset <dir>.", "dir",
                                  QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/BIDS");
    parser.addOption(rootOption);
    parser.process(app);
    const QString root = parser.value(rootOption);
    bool ok = true;

    //! [bids_path_usage]
    BIDSPath ieeg(root, "01", "01", "rest", "ieeg", "ieeg", ".vhdr"); // root, sub, ses, task, datatype, suffix, ext
    const QString vhdr = ieeg.filePath();                             // <root>/sub-01/ses-01/ieeg/sub-01_ses-01_task-rest_ieeg.vhdr
    const QString channelsTsv = ieeg.channelsTsvPath().filePath();    // sibling sidecars share the entities
    //! [bids_path_usage]
    ok &= expect(ieeg.exists() && ieeg.basename() == "sub-01_ses-01_task-rest_ieeg.vhdr" && channelsTsv.endsWith("sub-01_ses-01_task-rest_channels.tsv"),
                 "BIDSPath builds the entity filename and its channels.tsv sidecar");

    //! [bids_tsv_read]
    QStringList headers;
    const QList<BidsTsvRow> rows = BidsTsv::readTsv(ieeg.eventsTsvPath().filePath(), headers);
    // each row maps column name -> cell text, e.g. rows[1]["trial_type"] == "response"
    //! [bids_tsv_read]
    ok &= expect(headers == QStringList({"onset", "duration", "sample", "value", "trial_type"}) && rows.size() == 4 && rows[1]["trial_type"] == "response" && rows[3]["onset"] == "5.250000",
                 "BidsTsv reads the 5 event columns and 4 rows");

    //! [brain_vision_reader_usage]
    BrainVisionReader brainVision;
    const bool bvOpen = brainVision.open(vhdr);
    const MatrixXf bvHead = brainVision.readRawSegment(0, 200); // calibrated, volts
    //! [brain_vision_reader_usage]
    // mne.io.read_raw_brainvision: 32 channels at 1000 Hz, 7900 samples; data[0, 0] = -23.5 uV, data[5, 100] = 38.5 uV
    ok &= expect(bvOpen && brainVision.getChannelCount() == 32 && brainVision.getSampleCount() == 7900 && brainVision.getFrequency() == 1000.0f && near(bvHead(0, 0), -2.35e-5, 1e-4) && near(bvHead(5, 100), 3.85e-5, 1e-4),
                 QString("BrainVision: %1 x %2 samples, (0,0) = %3 V match mne").arg(brainVision.getChannelCount()).arg(brainVision.getSampleCount()).arg(bvHead(0, 0)));

    //! [edf_reader_usage]
    std::unique_ptr<AbstractFormatReader> edf = BidsRawData::createReader(".edf"); // or construct EDFReader directly
    const bool edfOpen = edf->open(BIDSPath(root, "02", "01", "rest", "eeg", "eeg", ".edf").filePath());
    const FIFFLIB::FiffInfo edfInfo = edf->getInfo();
    const MatrixXf edfHead = edf->readRawSegment(0, 201);
    //! [edf_reader_usage]
    // mne.io.read_raw_edf: 25 channels at 128 Hz, 1228 samples, first channel "EEG Fp1"; data[3, 200] = 0.1757724
    ok &= expect(edfOpen && dynamic_cast<EDFReader*>(edf.get()) != nullptr && edf->formatName() == "EDF" && edfInfo.nchan == 25 && edfInfo.ch_names.first() == "EEG Fp1" && edf->getSampleCount() == 1228 && near(edfHead(0, 0), 0.17594066, 1e-5) && near(edfHead(3, 200), 0.17577240650034331, 1e-5),
                 QString("EDF: %1 channels, %2 samples, (3,200) = %3 match mne").arg(edfInfo.nchan).arg(edf->getSampleCount()).arg(edfHead(3, 200), 0, 'g', 8));

    //! [bids_raw_data_read]
    BidsRawData data = BidsRawData::read(ieeg); // raw data plus channels, events, electrodes and sidecar metadata
    //! [bids_raw_data_read]
    ok &= expect(data.isValid() && data.raw.info.nchan == 32 && data.raw.info.bads == QStringList({"P4"}) && data.events.size() == 4 && data.electrodes.size() == 10 && data.ieegReference == "Cz" && data.eventIdMap.value("feedback") == 3,
                 "BidsRawData applies channels.tsv (bad P4), events.tsv, electrodes.tsv and the sidecar");

    //! [bids_raw_data_write]
    QTemporaryDir out;
    BidsRawData::WriteOptions options;
    options.datasetName = "ex_bids";
    const BIDSPath written = data.write(BIDSPath(out.path(), "01", "01", "rest", "ieeg", "ieeg", ".vhdr"), vhdr, options);
    const BidsRawData readBack = BidsRawData::read(written);
    //! [bids_raw_data_write]
    // every EEG channel gets an electrodes.tsv row; the 10 measured positions must come back under the same names
    int samePositions = 0;
    for (const BidsElectrode& before : data.electrodes) {
        for (const BidsElectrode& after : readBack.electrodes) {
            samePositions += after.name == before.name && std::fabs(after.x.toDouble() - before.x.toDouble()) + std::fabs(after.y.toDouble() - before.y.toDouble()) + std::fabs(after.z.toDouble() - before.z.toDouble()) < 1e-6;
        }
    }
    ok &= expect(out.isValid() && QFileInfo::exists(written.filePath()) && readBack.isValid() && readBack.raw.info.bads == data.raw.info.bads && readBack.events.size() == 4 && samePositions == 10,
                 QString("write() then read() keeps bads, events and %1/10 electrode positions").arg(samePositions));

    qInfo().noquote() << (ok ? "All bids checks passed." : "bids checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
