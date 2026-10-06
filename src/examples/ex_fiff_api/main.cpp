//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    FIFF library: measurement info, channels, digitizer, transforms, projectors, covariance, events,
 *           annotations, evoked sets and the low-level file structure.
 *
 * Reads the sample recording of the MNE-CPP test data and compares every value
 * with MNE-Python 1.11 (read_raw_fif, read_cov, read_evokeds, read_trans,
 * find_events). Exits non-zero on any mismatch.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>
#include <fiff/fiff_annotation_event_utils.h>
#include <fiff/fiff_annotations.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_dig_point_set.h>
#include <fiff/fiff_digitizer_data.h>
#include <fiff/fiff_dir_node.h>
#include <fiff/fiff_events.h>
#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_id.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_io.h>
#include <fiff/fiff_named_matrix.h>
#include <fiff/fiff_proj.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_sparse_matrix.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QFile>
#include <QTemporaryDir>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <cstdlib>
#include <memory>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
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

bool near(double value, double expected, double relTol = 1e-6)
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
    QCommandLineOption dataOption("data", "Sample MEG <dir>.", "dir",
                                  QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample");
    parser.addOption(dataOption);
    parser.process(app);
    const QString dir = parser.value(dataOption);
    QTemporaryDir tmp;
    bool ok = tmp.isValid();

    //! [fiff_info_usage]
    QFile rawFile(dir + "/sample_audvis_trunc_raw.fif");
    FiffRawData raw(rawFile);        // reads the measurement info, raw data is read on demand
    const FiffInfo& info = raw.info; // channels, sampling rate, bads, projectors, digitizer, transforms
    const FiffChInfo& first = info.chs[0];
    const RowVectorXi megPicks = info.pick_types(true, false, false, QStringList(), info.bads); // good MEG channels
    //! [fiff_info_usage]
    // mne.io.read_raw_fif: 376 channels at 300.307 Hz, bads MEG 2443 + EEG 053, 4 projectors
    ok &= expect(info.nchan == 376 && near(info.sfreq, 300.3074951171875) && info.bads == QStringList({"MEG2443", "EEG053"}) && info.projs.size() == 4 && megPicks.size() == 305 && raw.first_samp == 12900 && raw.last_samp == 18906,
                 "FiffInfo: 376 channels at 300.307 Hz, 2 bads, 4 projectors, 305 good MEG channels");
    // ch 0: MEG 0113, planar gradiometer 3012 at (-0.1066, 0.0464, -0.0604) m, cal 3.16e-9
    ok &= expect(first.ch_name == "MEG0113" && first.kind == FIFFV_MEG_CH && first.chpos.coil_type == FIFFV_COIL_VV_PLANAR_T1 && (first.chpos.r0 - Vector3f(-0.1066f, 0.0464f, -0.0604f)).norm() < 1e-6f && near(first.cal, 3.1600000394149674e-09),
                 "FiffChInfo/FiffChPos: MEG0113 is a 3012 planar gradiometer at (-0.1066, 0.0464, -0.0604) m");

    //! [fiff_dig_point_set_usage]
    FiffDigPointSet digitizer(info.dig);                                    // 146 points in head coordinates
    FiffDigPointSet cardinal = digitizer.pickTypes({FIFFV_POINT_CARDINAL}); // LPA, nasion, RPA
    //! [fiff_dig_point_set_usage]
    // mne: 146 points; dig[0] = LPA at (-0.0713766, 0, 5.1e-9)
    ok &= expect(digitizer.size() == 146 && cardinal.size() == 3 && cardinal[0].ident == FIFFV_POINT_LPA && std::fabs(cardinal[0].r[0] + 0.07137661f) < 1e-7f,
                 "FiffDigPointSet/FiffDigPoint: 146 points, LPA at x = -71.38 mm");

    //! [fiff_coord_trans_usage]
    QFile transFile(dir + "/all-trans.fif");
    FiffCoordTrans headToMri(transFile);            // FIFFV_COORD_HEAD (4) -> FIFFV_COORD_MRI (5)
    const FiffCoordTrans devHead = info.dev_head_t; // device -> head, from the measurement info
    FiffCoordTrans back = devHead.inverted();
    float r[3] = {0.0f, 0.0f, 0.0f};
    FiffCoordTrans::apply_trans(r, devHead, true); // the device origin in head coordinates
    //! [fiff_coord_trans_usage]
    // mne.read_trans: 1 -> 4 ... ; info["dev_head_t"] translation (-0.006129, 0.000064, 0.064742) m
    ok &= expect(devHead.from == FIFFV_COORD_DEVICE && devHead.to == FIFFV_COORD_HEAD && (Vector3f(r[0], r[1], r[2]) - Vector3f(-0.006129f, 0.000064f, 0.064742f)).norm() < 1e-6f && (back.trans * devHead.trans).isIdentity(1e-5f) && !headToMri.isEmpty(),
                 "FiffCoordTrans: device origin maps to (-6.13, 0.06, 64.74) mm in head coordinates");

    //! [fiff_proj_usage]
    const FiffProj& pca = info.projs[0]; // "PCA-v1", 1 vector over 102 magnetometers, inactive
    QList<FiffProj> projs = info.projs;
    FiffProj::activate_projs(projs);
    //! [fiff_proj_usage]
    ok &= expect(pca.desc == "PCA-v1" && !pca.active && pca.data->nrow == 1 && pca.data->ncol == 102 && projs[0].active,
                 "FiffProj/FiffNamedMatrix: PCA-v1 is 1 x 102 and inactive until activated");

    //! [fiff_cov_usage]
    QFile covFile(dir + "/sample_audvis-cov.fif");
    FiffCov cov(covFile);
    const FiffCov megCov = cov.pick_channels(info.ch_names.mid(0, 306), cov.bads); // good MEG channels only
    //! [fiff_cov_usage]
    // mne.read_cov: dim 366, nfree 15972, 2 bads, 4 projectors, C[0, 0] = 2.272355891906954e-23
    ok &= expect(cov.dim == 366 && cov.nfree == 15972 && cov.bads.size() == 2 && cov.projs.size() == 4 && near(cov.data(0, 0), 2.272355891906954e-23) && megCov.dim == 305,
                 "FiffCov: 366 channels, 15972 degrees of freedom, C[0,0] matches mne.read_cov");

    //! [fiff_events_usage]
    FiffEvents events;
    FiffEvents::detect_from_raw(raw, events, "STI 014"); // rising edges of the trigger channel
    QFile eventFile(tmp.filePath("sample-eve.fif"));
    const bool eventsWritten = eventFile.open(QIODevice::WriteOnly) && events.write_to_fif(eventFile);
    eventFile.close();
    FiffEvents eventsBack(eventFile); // mne.read_events reads this file too
    //! [fiff_events_usage]
    // mne.find_events(raw, "STI 014"): 25 events, first [13988, 0, 2], [14172, 0, 3], [14385, 0, 1]
    ok &= expect(events.num_events() == 25 && events.events.row(0) == RowVector3i(13988, 0, 2) && events.events.row(2) == RowVector3i(14385, 0, 1) && eventsWritten && eventsBack.events == events.events,
                 "FiffEvents: 25 trigger events match mne.find_events and round-trip through -eve.fif");

    //! [fiff_annotations_usage]
    MatrixXi firstThree = events.events.topRows(3);
    FiffAnnotations annotations = annotationsFromEvents(firstThree, info.sfreq, {{1, "auditory/left"}, {2, "auditory/right"}}, raw.first_samp);
    annotations.append({0.5, 1.0, "BAD_blink", {}, {}, {}});
    FiffAnnotations::write(tmp.filePath("annot.csv"), annotations);
    const FiffAnnotations annotationsBack = FiffAnnotations::read(tmp.filePath("annot.csv"));
    const MatrixXi eventsAgain = eventsFromAnnotations(annotationsBack.select("auditory"), info.sfreq, {{"auditory/left", 1}, {"auditory/right", 2}}, raw.first_samp);
    //! [fiff_annotations_usage]
    // Event 13988 (code 2) is (13988 - 12900) / 300.307 s = 3.6230 s after the first sample; code 3 has no description and stays "3"; the CSV keeps onsets to well within half a sample
    ok &= expect(annotationsBack.size() == 4 && std::fabs(annotationsBack.select("auditory/right").toVector()[0].onset - (13988 - 12900) / info.sfreq) < 0.5 / info.sfreq && eventsAgain.rows() == 2 && eventsAgain(0, 0) == 13988 && eventsAgain(1, 0) == 14385 && eventsAgain(1, 2) == 1 && annotationsBack.select("3").size() == 1,
                 "FiffAnnotations: events become onsets in seconds and back, with descriptions as codes");

    //! [fiff_evoked_set_usage]
    QFile aveFile(dir + "/sample_audvis-ave.fif");
    FiffEvokedSet evokedSet(aveFile); // all conditions of an -ave.fif file
    const FiffEvoked& leftAuditory = evokedSet.evoked[0];
    //! [fiff_evoked_set_usage]
    // mne.read_evokeds: 4 conditions, "Left Auditory" nave 55, 376 x 421 samples starting at -0.1998 s
    ok &= expect(evokedSet.evoked.size() == 4 && leftAuditory.comment == "Left Auditory" && leftAuditory.nave == 55 && leftAuditory.data.cols() == 421 && std::fabs(leftAuditory.times(0) + 0.19979521f) < 1e-6f,
                 "FiffEvokedSet/FiffEvoked: 4 conditions; Left Auditory averages 55 epochs over 421 samples");

    //! [fiff_stream_usage]
    QFile structureFile(dir + "/sample_audvis-ave.fif");
    FiffStream::SPtr stream(new FiffStream(&structureFile));
    const bool opened = stream->open();
    const QList<FiffDirNode::SPtr> measNodes = stream->dirtree()->dir_tree_find(FIFFB_MEAS_INFO);
    std::unique_ptr<FiffTag> tag;
    const bool hasNchan = measNodes.first()->find_tag(stream, FIFF_NCHAN, tag); // raw tag access
    const int nchanTag = *tag->toInt();
    const FiffId fileId = stream->id();
    stream->close();
    //! [fiff_stream_usage]
    ok &= expect(opened && measNodes.size() == 1 && hasNchan && nchanTag == 376 && !fileId.isEmpty() && fileId.version > 0,
                 "FiffStream/FiffDirNode/FiffTag/FiffId: the FIFF_NCHAN tag of the measurement block reads 376");

    //! [fiff_digitizer_data_usage]
    FiffDigitizerData digData;
    digData.points = info.dig;
    digData.npoint = info.dig.size();
    digData.coord_frame = FIFFV_COORD_HEAD;
    digData.head_mri_t_adj = std::make_unique<FiffCoordTrans>(headToMri);
    digData.pickCardinalFiducials(); // LPA, nasion and RPA moved into MRI coordinates
    //! [fiff_digitizer_data_usage]
    float lpaMri[3] = {cardinal[0].r[0], cardinal[0].r[1], cardinal[0].r[2]};
    FiffCoordTrans::apply_trans(lpaMri, headToMri, true);
    ok &= expect(digData.nfids() == 3 && (Map<Vector3f>(digData.mri_fids[0].r) - Map<Vector3f>(lpaMri)).norm() < 1e-7f, "FiffDigitizerData moves the 3 cardinal fiducials into MRI coordinates");

    //! [fiff_sparse_matrix_usage]
    SparseMatrix<double> eigen(3, 4);
    eigen.insert(0, 1) = 2.0;
    eigen.insert(2, 3) = -1.5;
    eigen.makeCompressed();
    const FiffSparseMatrix fiffSparse = FiffSparseMatrix::fromEigenSparse(eigen); // FIFF storage layout
    const SparseMatrix<double> backToEigen = fiffSparse.toEigenSparse();
    //! [fiff_sparse_matrix_usage]
    ok &= expect(backToEigen.coeff(2, 3) == -1.5 && backToEigen.nonZeros() == 2, "FiffSparseMatrix round-trips an Eigen sparse matrix");

    //! [fiff_facade_usage]
    QFile facadeFile(dir + "/sample_audvis_trunc_raw.fif");
    FiffStream::SPtr facadeStream;
    const bool facadeOpened = Fiff::open(facadeFile, facadeStream); // static wrappers around FiffStream
    FiffDirNode::SPtr facadeRoot = facadeStream->dirtree();
    const QStringList facadeBads = Fiff::read_bad_channels(facadeStream, facadeRoot);
    facadeStream->close();
    //! [fiff_facade_usage]
    ok &= expect(facadeOpened && facadeBads == info.bads, "Fiff::open and Fiff::read_bad_channels find the same 2 bads as FiffInfo");

    //! [fiff_io_usage]
    QFile ioFile(dir + "/sample_audvis-ave.fif");
    FiffIO io(ioFile); // detects what the file holds (raw, evoked, fwd, ...)
    //! [fiff_io_usage]
    ok &= expect(io.m_qlistEvoked.size() == 4 && io.m_qlistRaw.isEmpty(), "FiffIO finds the 4 evoked sets in an -ave.fif file");

    qInfo().noquote() << (ok ? "All fiff checks passed." : "fiff checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
