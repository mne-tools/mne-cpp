//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    MNE library: BEM surfaces, surface projection, ICP, source spaces and epochs.
 *
 * Reads the sample BEM, source space and raw recording and compares what it finds with
 * MNE-Python 1.11 (read_bem_surfaces, _project_onto_surface, read_source_spaces,
 * find_events, Epochs). The ICP part recovers a known rigid transform.
 * Exits non-zero on any mismatch.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_constants.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_events.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_stream.h>
#include <mne/mne_bem.h>
#include <mne/mne_bem_surface.h>
#include <mne/mne_ch_selection.h>
#include <mne/mne_cov_matrix.h>
#include <mne/mne_ctf_comp_data.h>
#include <mne/mne_ctf_comp_data_set.h>
#include <mne/mne_epoch_data.h>
#include <mne/mne_epoch_data_list.h>
#include <mne/mne_filter_def.h>
#include <mne/mne_hemisphere.h>
#include <mne/mne_icp.h>
#include <mne/mne_named_matrix.h>
#include <mne/mne_proj_item.h>
#include <mne/mne_proj_op.h>
#include <mne/mne_project_to_surface.h>
#include <mne/mne_raw_buf_def.h>
#include <mne/mne_raw_data.h>
#include <mne/mne_raw_info.h>
#include <mne/mne_source_spaces.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QFile>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Geometry>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <cstdlib>
#include <memory>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace MNELIB;
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
    QCommandLineOption dataOption("data", "MNE-CPP test data <dir>.", "dir",
                                  QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data");
    QCommandLineOption ctfOption("ctf", "CTF raw file stored at compensation grade 3.", "file", QString(MNE_CTF_FILE));
    parser.addOption(dataOption);
    parser.addOption(ctfOption);
    parser.process(app);
    const QString data = parser.value(dataOption);
    const QString ctfPath = parser.value(ctfOption);
    bool ok = true;

    //! [mne_bem_usage]
    QFile bemFile(data + "/subjects/sample/bem/sample-5120-bem.fif");
    MNEBem bem(bemFile); // one MNEBemSurface per BEM layer
    const MNEBemSurface& innerSkull = bem[0];
    //! [mne_bem_usage]
    // mne.read_bem_surfaces: 1 surface, id 1 (inner skull), 2562 vertices, 5120 triangles
    ok &= expect(bem.size() == 1 && innerSkull.id == FIFFV_BEM_SURF_ID_BRAIN && innerSkull.np == 2562 && innerSkull.ntri == 5120 && (innerSkull.rr.row(0) - RowVector3f(0.0006128f, -0.007984f, 0.103732f)).norm() < 1e-6f && (innerSkull.nn.row(0) - RowVector3f(0.05623665f, -0.1050898f, 0.99287134f)).norm() < 1e-5f && MNEBemSurface::id_name(innerSkull.id) == "Brain",
                 "MNEBem/MNEBemSurface: inner skull with 2562 vertices and normals like mne.read_bem_surfaces");

    //! [mne_project_to_surface_usage]
    MNEProjectToSurface projector(innerSkull);
    MatrixXf points(2, 3);
    points << 0.0f, 0.0f, 0.12f, 0.05f, 0.01f, 0.03f; // one point outside, one inside the skull
    MatrixXf onSurface;
    VectorXi triangle;
    VectorXf distance;
    projector.find_closest_on_surface(points, points.rows(), onSurface, triangle, distance);
    //! [mne_project_to_surface_usage]
    // Exact point-to-triangle distance over all 5120 triangles (numpy on mne.read_bem_surfaces):
    // triangle 1037 at 14.6163 mm, and 11.5561 mm on the edge shared by triangles 3143 and 3172.
    // For the first point mne's _project_onto_surface returns a point on the plane of triangle 1038
    // but outside the triangle (barycentric weights 1.2 and -0.39), so it is not on the surface.
    ok &= expect(triangle(0) == 1037 && (triangle(1) == 3143 || triangle(1) == 3172) && (onSurface.row(0) - RowVector3f(-5.97378269e-05f, 1.14249234e-03f, 0.105428493f)).norm() < 1e-6f && (onSurface.row(1) - RowVector3f(0.060674f, 0.01416721f, 0.02850214f)).norm() < 1e-6f && near(distance(0), 0.014616349831736736, 1e-5) && near(distance(1), 0.011556106943744577, 1e-5),
                 "MNEProjectToSurface: closest triangles, points and distances match an exhaustive search");

    // Head shape points: every 20th inner-skull vertex moved by a known rigid transform.
    const Matrix3f trueRot = AngleAxisf(0.05f, Vector3f(0.2f, 1.0f, 0.3f).normalized()).toRotationMatrix();
    const Vector3f trueMove(0.004f, -0.003f, 0.006f);
    MatrixXf headShape(innerSkull.np / 20 + 1, 3);
    for (int i = 0; i < headShape.rows(); ++i) {
        headShape.row(i) = (trueRot.transpose() * (innerSkull.rr.row(20 * i).transpose() - trueMove)).transpose();
    }

    //! [mne_icp_usage]
    // 1. Rough start from three matched points (e.g. the fiducials)
    Matrix4f start;
    MatrixXf mriPoints(3, 3);
    mriPoints << innerSkull.rr.row(0), innerSkull.rr.row(20), innerSkull.rr.row(40);
    fitMatchedPoints(headShape.topRows(3), mriPoints, start);
    FiffCoordTrans headToMri(FIFFV_COORD_HEAD, FIFFV_COORD_MRI, start);
    // 2. Drop head shape points further than 10 mm from the surface, then refine with ICP
    auto surface = QSharedPointer<MNEProjectToSurface>::create(innerSkull);
    VectorXi take;
    MatrixXf kept;
    discard3DPointOutliers(surface, headShape, headToMri, take, kept, 0.01f);
    float rmse = 0.0f;
    performIcp(surface, kept, headToMri, rmse, false, 50, 1e-6f);
    //! [mne_icp_usage]
    ok &= expect(take.size() == headShape.rows() && (headToMri.rot() - trueRot).norm() < 1e-3f && (headToMri.move() - trueMove).norm() < 1e-4f && rmse < 1e-4f,
                 "fitMatchedPoints/discard3DPointOutliers/performIcp recover the known head-to-MRI transform");

    //! [mne_source_spaces_usage]
    QFile srcFile(data + "/subjects/sample/bem/sample-oct-6-src.fif");
    FiffStream::SPtr stream(new FiffStream(&srcFile));
    MNESourceSpaces src;
    stream->open();
    MNESourceSpaces::readFromStream(stream, true, src); // add_geom: normals, triangle data and patches
    stream->close();
    const auto& lh = dynamic_cast<const MNEHemisphere&>(src[0]);
    QList<VectorXi> vertno = src.get_vertno(); // used vertices per hemisphere
    //! [mne_source_spaces_usage]
    // mne.read_source_spaces: lh 155407 vertices, 4098 in use (8192 used triangles), vertno 14, 54, 59; rh 156866 / 4098
    ok &= expect(src.size() == 2 && lh.np == 155407 && lh.nuse == 4098 && lh.nuse_tri == 8192 && lh.coord_frame == FIFFV_COORD_MRI && vertno[0].head(3) == Vector3i(14, 54, 59) && src[1].np == 156866 && vertno[1].size() == 4098 && lh.pinfo.size() == 4098,
                 "MNESourceSpaces/MNEHemisphere: oct-6 source space with patches like mne.read_source_spaces");

    //! [mne_epoch_data_list_usage]
    QFile rawFile(data + "/MEG/sample/sample_audvis_trunc_raw.fif");
    const FiffRawData raw(rawFile);
    FiffEvents events;
    FiffEvents::detect_from_raw(raw, events); // rising edges on STI 014
    // Epochs of event 1 from -100 to 300 ms, no rejection
    MNEEpochDataList epochs = MNEEpochDataList::readEpochs(raw, events.events, -0.1f, 0.3f, 1, {});
    const MNEEpochData& firstEpoch = *epochs.first();
    FiffEvoked evoked = epochs.average(raw.info, raw.first_samp, raw.last_samp);
    //! [mne_epoch_data_list_usage]
    // mne.Epochs(event_id=1, tmin=-0.1, tmax=0.3, baseline=None, proj=False): 6 epochs of 376 x 121, MEG 0113 first sample -7.5857e-15
    ok &= expect(epochs.size() == 6 && firstEpoch.eventSample == 14385 && firstEpoch.epoch.rows() == 376 && firstEpoch.epoch.cols() == 121 && near(firstEpoch.epoch(0, 0), -7.585747577225703e-15, 1e-6) && near(firstEpoch.epoch.topRows(306).norm(), 7.05600718833613e-13, 1e-6),
                 "MNEEpochDataList/MNEEpochData: 6 epochs of event 1 match mne.Epochs");
    // mne Epochs.average(): |MEG| = 6.0801e-13 over 6 epochs, times from -99.898 to 299.693 ms with 0 at sample 30
    ok &= expect(evoked.nave == 6 && near(evoked.data.topRows(306).norm(), 6.080119131626428e-13, 1e-6) && evoked.times.size() == 121 && near(evoked.times[0], -0.09989760657919393, 1e-6) && near(evoked.times[120], 0.2996928197375818, 1e-6) && evoked.times[30] == 0.0f,
                 "MNEEpochDataList::average matches mne Epochs.average");

    //! [mne_raw_data_usage]
    // MNE-C style reader: buffer directory, channel selection, projection and overlap-add filter
    MNEFilterDef filter; // 40 Hz lowpass, 5 Hz transition, 4096-sample blocks
    filter.filter_on = true;
    filter.size = 4096;
    filter.taper_size = 2048;
    filter.lowpass = filter.eog_lowpass = 40.0f;
    filter.lowpass_width = filter.eog_lowpass_width = 5.0f;
    std::unique_ptr<MNERawData> mneRaw(MNERawData::open_file(rawFile.fileName(), false, false, filter));
    const MNERawInfo& rawInfo = *mneRaw->info;
    const MNERawBufDef& firstBuffer = mneRaw->bufs.front();

    MNEChSelection sel; // channel names are stored without spaces
    sel.chspick = sel.chspick_nospace = {"MEG0113", "EEG001", "EOG061"};
    sel.nchan = sel.ndef = static_cast<int>(sel.chspick.size());
    sel.pick_deriv = VectorXi::Constant(sel.nchan, -1);
    sel.pick.resize(sel.nchan);
    for (int c = 0; c < sel.nchan; ++c) {
        sel.pick[c] = static_cast<int>(mneRaw->ch_names.indexOf(sel.chspick[c]));
    }

    const int ns = 25;
    Matrix<float, Dynamic, Dynamic, RowMajor> segment(sel.nchan, ns);
    std::vector<float*> rows{segment.row(0).data(), segment.row(1).data(), segment.row(2).data()};
    mneRaw->pick_data(&sel, mneRaw->first_samp + 2990, ns, rows.data()); // calibrated, across a buffer boundary
    const double eegRaw = segment(1, 0);
    mneRaw->pick_data_filt(&sel, mneRaw->first_samp + 2990, ns, rows.data());
    const double eegFilt = segment(1, 0);

    // The file stores its 4 SSP projectors as inactive; makeProjection switches them on
    std::unique_ptr<MNEProjOp> proj;
    MNEProjOp::makeProjection({rawFile.fileName()}, rawInfo.chInfo, rawInfo.nchan, proj);
    proj->assign_channels(mneRaw->ch_names, rawInfo.nchan);
    proj->make_proj();
    const MNEProjItem& firstItem = proj->items[0];
    mneRaw->proj = std::move(proj);
    mneRaw->pick_data_proj(&sel, mneRaw->first_samp + 2990, ns, rows.data());
    const double eegProj = segment(1, 0);
    //! [mne_raw_data_usage]
    // mne.io.read_raw_fif: first_samp 12900, 6007 samples, 376 channels, 2 bads; EEG 001 at offset 2990 -1.01223e-09
    // numpy port of MNE-C's overlap-add 40 Hz lowpass: -1.06402e-09; apply_proj(): -4.10778e-10
    ok &= expect(mneRaw->first_samp == 12900 && mneRaw->nsamp == 6007 && rawInfo.nchan == 376 && mneRaw->nbad == 2 && firstBuffer.firsts == 12900 && mneRaw->bufs.size() > 1,
                 "MNERawData/MNERawInfo/MNERawBufDef: buffer directory of the sample recording like mne.io.read_raw_fif");
    ok &= expect(sel.pick.minCoeff() >= 0 && near(eegRaw, -1.0122332562412516e-09, 1e-5) && near(eegProj, -4.107782047805238e-10, 1e-4) && near(eegFilt, -1.064022285676407e-09, 1e-3),
                 "MNEChSelection/MNEFilterDef: plain, projected and filtered reads match mne");
    ok &= expect(mneRaw->proj->nitems == 4 && mneRaw->proj->nvec == 4 && firstItem.active && firstItem.nvec == 1,
                 "MNEProjOp/MNEProjItem: 4 active SSP vectors like raw.info['projs']");

    //! [mne_cov_matrix_usage]
    // Whiten one time point of the evoked response with the projected noise covariance
    QFile aveFile(data + "/MEG/sample/sample_audvis-ave.fif");
    FiffEvoked ave(aveFile, 0, QPair<float, float>(-1.0f, -1.0f), false);
    QStringList megNames; // the 305 good MEG channels
    for (const QString& name : ave.info.ch_names) {
        if (name.startsWith("MEG") && !ave.info.bads.contains(name)) {
            megNames << name;
        }
    }
    ave = ave.pick_channels(megNames);
    const int nMeg = static_cast<int>(megNames.size());
    std::unique_ptr<MNEProjOp> aveProj;
    MNEProjOp::makeProjection({aveFile.fileName()}, ave.info.chs, nMeg, aveProj);
    aveProj->assign_channels(megNames, nMeg);
    aveProj->make_proj();
    VectorXf x = ave.data.col(200).cast<float>(); // t = 133 ms
    aveProj->project_vector(x, true);

    auto noise = MNECovMatrix::read(data + "/MEG/sample/sample_audvis-cov.fif", FIFFV_MNE_NOISE_COV);
    const int ncovFile = noise->ncov;
    auto noiseMeg = noise->pick_chs_omit(megNames, nMeg, false, ave.info.chs);
    aveProj->apply_cov(noiseMeg.get());
    noiseMeg->classify_channels(ave.info.chs, nMeg);
    noiseMeg->decompose_eigen(); // the 3 projected directions become zero eigenvalues
    VectorXf whitened(nMeg);
    noiseMeg->whiten_vector(x, whitened, nMeg);
    //! [mne_cov_matrix_usage]
    // mne.read_cov: 366 channels, nfree 15972; x^T pinv(P C P) x with P the 3 SSP vectors: 1462.05
    ok &= expect(ncovFile == 366 && noise->nfree == 15972 && !noise->is_diag() && noiseMeg->nzero == 3 && near(whitened.cast<double>().squaredNorm(), 1462.0502702959539, 1e-4),
                 "MNECovMatrix: projected noise covariance whitens like mne's pseudo-inverse");

    //! [mne_ctf_comp_data_set_usage]
    // Open a third-order gradiometer recording and read it uncompensated (grade 0)
    std::unique_ptr<MNERawData> ctfRaw(MNERawData::open_file_comp(ctfPath, false, false, MNEFilterDef(), 0));
    const MNECTFCompDataSet& comp = *ctfRaw->comp; // grades 1, 2, 3 and two 4D kinds
    const MNECTFCompData* third = nullptr;
    for (const auto& c : comp.comps) {
        if (c->kind == MNECTFCompDataSet::map_comp_kind(3)) {
            third = c.get();
        }
    }
    const int grade = MNECTFCompDataSet::get_comp(ctfRaw->info->chInfo, ctfRaw->info->nchan);
    Matrix<float, Dynamic, Dynamic, RowMajor> ctf(ctfRaw->info->nchan, 21);
    std::vector<float*> ctfRows(ctf.rows());
    for (int c = 0; c < ctf.rows(); ++c) {
        ctfRows[c] = ctf.row(c).data();
    }
    ctfRaw->pick_data(nullptr, ctfRaw->first_samp, 21, ctfRows.data());
    //! [mne_ctf_comp_data_set_usage]
    // raw.apply_gradient_compensation(0): sum |MEG| over the 7 MEG channels 2.45885e-10 (1.47942e-11 as stored)
    ok &= expect(comp.ncomp == 5 && grade == 0 && third && MNECTFCompDataSet::explain_comp(third->kind) == "third order gradiometer" && third->data->nrow == 7 && near(ctf.topRows(7).cast<double>().cwiseAbs().sum(), 2.4588499274586883e-10, 1e-5),
                 "MNECTFCompDataSet/MNECTFCompData/MNENamedMatrix: grade 3 -> 0 like raw.apply_gradient_compensation");

    qInfo().noquote() << (ok ? "All mne checks passed." : "mne checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
