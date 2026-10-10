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
#include <fiff/fiff_cov.h>
#include <fiff/fiff_events.h>
#include <fiff/fiff_proj.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_stream.h>
#include <fs/fs_surface.h>
#include <mne/mne.h>
#include <mne/mne_bem.h>
#include <mne/mne_bem_surface.h>
#include <mne/mne_ch_selection.h>
#include <mne/mne_cov_matrix.h>
#include <mne/mne_ctf_comp_data.h>
#include <mne/mne_ctf_comp_data_set.h>
#include <mne/mne_deriv.h>
#include <mne/mne_deriv_set.h>
#include <mne/mne_description_parser.h>
#include <mne/mne_epoch_data.h>
#include <mne/mne_epoch_data_list.h>
#include <mne/mne_filter_def.h>
#include <mne/mne_hemisphere.h>
#include <mne/mne_event.h>
#include <mne/mne_event_list.h>
#include <mne/mne_icp.h>
#include <mne/mne_layout.h>
#include <mne/mne_meas_data.h>
#include <mne/mne_morph_map.h>
#include <mne/mne_meas_data_set.h>
#include <mne/mne_msh_display_surface.h>
#include <mne/mne_msh_display_surface_set.h>
#include <mne/mne_named_matrix.h>
#include <mne/mne_proj_item.h>
#include <mne/mne_proj_op.h>
#include <mne/mne_project_to_surface.h>
#include <mne/mne_raw_buf_def.h>
#include <mne/mne_raw_data.h>
#include <mne/mne_raw_info.h>
#include <mne/mne_source_spaces.h>
#include <mne/mne_sparse_named_matrix.h>
#include <mne/mne_sss_data.h>
#include <mne/mne_surface.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QFile>
#include <QTemporaryDir>

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
using namespace FSLIB;
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
    QCommandLineOption sssOption("sss", "Raw file with an SSS processing record.", "file", QString(MNE_SSS_FILE));
    parser.addOption(dataOption);
    parser.addOption(ctfOption);
    parser.addOption(sssOption);
    parser.process(app);
    const QString data = parser.value(dataOption);
    const QString ctfPath = parser.value(ctfOption);
    const QString sssPath = parser.value(sssOption);
    const QString layoutDir = QCoreApplication::applicationDirPath() + "/../resources/general/2DLayouts";
    QTemporaryDir tmp;
    bool ok = tmp.isValid();

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

    //! [mne_description_parser_usage]
    // An MNE-C averaging description (mne_process_raw --ave), parsed and run on the raw recording
    const QString aveDesc = tmp.filePath("audvis.ave");
    QFile descFile(aveDesc);
    ok &= descFile.open(QIODevice::WriteOnly | QIODevice::Text);
    descFile.write("average {\n"
                   "    category {\n"
                   "        name   \"Left Auditory\"\n"
                   "        event  1\n"
                   "        tmin   -0.1\n"
                   "        tmax   0.3\n"
                   "        bmin   -0.1\n"
                   "        bmax   0.0\n"
                   "    }\n"
                   "}\n");
    descFile.close();
    AverageDescription averaging;
    MNEDescriptionParser::parseAverageFile(aveDesc, averaging);
    const AverageCategory& leftCategory = averaging.categories.first(); // name, events, tmin/tmax, baseline
    const RejectionParams& rejection = averaging.rej;                   // MNE-C defaults unless the file sets them (EEG 100 uV, ...)
    QString log;
    const FiffEvokedSet averages = FiffEvokedSet::computeAverages(raw, averaging, events.events, log);
    //! [mne_description_parser_usage]
    // mne.Epochs(event_id=1, tmin=-0.1, tmax=0.3, baseline=(-0.1, 0)).average(): 6 epochs, |MEG| 2.01258e-13, EEG 001 at 0 s 7.77233e-10
    const bool averaged = averaging.categories.size() == 1 && leftCategory.doBaseline && leftCategory.comment == "Left Auditory" && leftCategory.events == QVector<unsigned int>{1} && rejection.eegReject == 100e-6f && averages.evoked.size() == 1;
    ok &= expect(averaged && averages.evoked[0].nave == 6 && averages.evoked[0].data.cols() == 121 && near(averages.evoked[0].data.topRows(306).norm(), 2.0125814419559757e-13, 1e-5) && near(averages.evoked[0].data(315, 30), 7.77233498919796e-10, 1e-5),
                 "MNEDescriptionParser: an .ave description averages like mne.Epochs with a baseline");

    //! [mne_cov_description_usage]
    // An MNE-C covariance description (mne_process_raw --cov): pre-stimulus data of events 1 and 2
    const QString covDescPath = tmp.filePath("audvis.cov");
    QFile covDescFile(covDescPath);
    ok &= covDescFile.open(QIODevice::WriteOnly | QIODevice::Text);
    covDescFile.write("cov {\n"
                      "    eegReject 14.230782369752743e-9\n"
                      "    def {\n"
                      "        event 1\n"
                      "        event 2\n"
                      "        tmin  -0.2\n"
                      "        tmax  0.0\n"
                      "    }\n"
                      "}\n");
    covDescFile.close();
    CovDescription covariance;
    MNEDescriptionParser::parseCovarianceFile(covDescPath, covariance);
    covariance.rej.megGradReject = covariance.rej.megMagReject = covariance.rej.eogReject = 0.0f; // EEG limit only
    const CovDefinition& preStim = covariance.defs.first();
    const FiffCov noiseCov = FiffCov::compute_from_epochs(raw, events.events, {1, 2}, preStim.tmin, preStim.tmax, preStim.bmin, preStim.bmax, preStim.doBaseline,
                                                          covariance.removeSampleMean, preStim.ignore, preStim.delay, &covariance.rej);
    //! [mne_cov_description_usage]
    // mne.compute_covariance(mne.Epochs(event_id=[1, 2], tmin=-0.2, tmax=0, baseline=None, reject=dict(eeg=14.23e-9)),
    // keep_sample_mean=False, rank="full"): 6 of 12 epochs kept, nfree 365, C[EEG001, EEG001] = 8.784207369943824e-18
    const int eeg001 = noiseCov.names.indexOf("EEG001");
    ok &= expect(preStim.events == QVector<unsigned int>({1, 2}) && noiseCov.nfree == 365 && near(noiseCov.data(eeg001, eeg001), 8.784207369943824e-18, 1e-6),
                 QString("MNEDescriptionParser: a .cov description with EEG rejection gives nfree %1 like mne.compute_covariance").arg(noiseCov.nfree));

    //! [mne_meas_data_usage]
    // MNE-C measurement container: data set 1 of the evoked file, MEG and EEG channels, time-major
    std::unique_ptr<MNEMeasData> meas(MNEMeasData::mne_read_meas_data(aveFile.fileName(), 1, nullptr, nullptr, {}, 0));
    const MNEMeasDataSet& left = *meas->current;
    const float meg0113 = left.data(180, 0); // t = 99.9 ms
    meas->adjust_baselines(-0.2f, 0.0f);
    VectorXf at100ms(meas->nchan);
    left.getValuesAtTime(0.1f, 0.0f, meas->nchan, false, at100ms.data()); // linear interpolation between samples
    //! [mne_meas_data_usage]
    // mne.read_evokeds(condition=0, baseline=None, proj=False): 366 MEG/EEG channels, 421 samples from -199.8 ms, nave 55;
    // MEG 0113 at sample 180 -4.74022e-12. MNE-C's baseline window stops before the sample at 0 s (120 samples, not mne's 121):
    // EEG 001 baseline -9.38499e-06, at 100 ms (interpolated) -1.45063e-06
    ok &= expect(meas->nchan == 366 && meas->nbad == 2 && left.np == 421 && left.nave == 55 && left.comment == "Left Auditory" && near(left.tmin, -0.19979521315838786, 1e-6) && near(meg0113, -4.740224134628893e-12, 1e-5) && near(at100ms(306), -1.4506279631761285e-06, 1e-4) && near(left.baselines(306), -9.384992481327106e-06, 1e-5),
                 "MNEMeasData/MNEMeasDataSet: evoked set, baseline and interpolated values match mne");

    //! [mne_sss_data_usage]
    auto sss = MNESssData::read(sssPath); // the SSS block written by mne.preprocessing.maxwell_filter
    //! [mne_sss_data_usage]
    // maxwell_filter(origin=(0, 0, 0.04), int_order=8, ext_order=3): head frame, 306 channels, 70 of 80 internal and 15 external components
    ok &= expect(sss && sss->job == FIFFV_SSS_JOB_FILTER && sss->coord_frame == FIFFV_COORD_HEAD && sss->nchan == 306 && sss->in_order == 8 && sss->out_order == 3 && sss->in_nuse == 70 && sss->out_nuse == 15 && std::fabs(sss->origin[2] - 0.04f) < 1e-7f,
                 "MNESssData: Maxwell filter parameters like info['proc_history'] of the fixture");

    //! [mne_facade_usage]
    // The MNE facade mirrors the MNE-Matlab/Python toolbox functions
    const QString eveName = tmp.filePath("audvis-eve.fif");
    {
        QFile eveOut(eveName);
        MNE::write_events_to_fif(eveOut, events.events);
    }
    MatrixXi eventsBack;
    MNE::read_events(eveName, rawFile.fileName(), eventsBack);
    QList<FiffProj> projs = raw.info.projs; // stored inactive in the raw file
    FiffProj::activate_projs(projs);
    MatrixXd ssp; // like mne._fiff.proj.make_projector(info['projs'], ch_names, bads)
    const int nProj = MNE::make_projector(projs, raw.info.ch_names, ssp, raw.info.bads);
    //! [mne_facade_usage]
    // mne.find_events: 25 events, first at sample 13988 with code 2; make_projector: 4 vectors, trace 372 of 376
    ok &= expect(eventsBack.rows() == 25 && eventsBack(0, 0) == 13988 && eventsBack(0, 2) == 2 && nProj == 4 && near(ssp.trace(), 372.0, 1e-9) && near(ssp.norm(), 19.28730152198591, 1e-9),
                 "MNE: event file round trip and SSP projector like mne.find_events / make_projector");

    //! [mne_surface_usage]
    // Total solid angle seen from a point: 4 pi inside a closed surface, 0 outside
    auto skull = MNESurface::read_bem_surface2(bemFile.fileName(), FIFFV_BEM_SURF_ID_BRAIN, true);
    skull->compute_surface_cm();
    const double inside = skull->sum_solids(Map<const Vector3f>(skull->cm)) / (4.0 * M_PI);
    const double outside = skull->sum_solids(Vector3f(0.0f, 0.0f, 0.2f)) / (4.0 * M_PI);

    // The same surface as an MNE-C viewer display surface
    MNEMshDisplaySurfaceSet display;
    display.add_bem_surface(bemFile.fileName(), FIFFV_BEM_SURF_ID_BRAIN, "inner skull", 1, 1); // checks closure
    const MNEMshDisplaySurface& shown = *display.surfs[0];
    //! [mne_surface_usage]
    // mne.surface._get_solids (which returns the solid angle / 2): 0.5 at the centroid, 0 at (0, 0, 200) mm;
    // bounding box -66.7 ... 105.8 mm, centroid (0.67, -10.01, 44.26) mm
    ok &= expect(near(inside, 1.0, 1e-6) && std::fabs(outside) < 1e-6 && (Map<const Vector3f>(skull->cm) - Vector3f(0.00067339f, -0.01001356f, 0.04426273f)).norm() < 1e-6f,
                 "MNESurface: solid angles show the inner skull is closed, like mne._get_solids");
    ok &= expect(display.nsurf == 1 && shown.np == 2562 && shown.surf_name == "inner skull" && near(shown.fov, 0.10576099902391434, 1e-6) && (shown.minv - Vector3f(-0.0667366f, -0.0880172f, -0.0445037f)).norm() < 1e-6f,
                 "MNEMshDisplaySurfaceSet/MNEMshDisplaySurface: the BEM surface and its extent in the viewer");

    //! [mne_event_list_usage]
    // Annotate the detected onsets, save them with their comments and read them back relative to the first sample
    MNEEventList annotated = MNEEventList::fromMatrix(events.events).selectOnsets();
    annotated.sort();
    for (MNEEvent& event : annotated.events) {
        event.comment = event.to == 1 ? "left auditory" : QString();
    }
    const QString commentedName = tmp.filePath("commented-eve.fif");
    annotated.writeFif(commentedName);
    const auto relative = MNEEventList::readFif(commentedName, raw.first_samp);
    const MNEEventList leftAuditory = relative->selectOnsets(1);
    //! [mne_event_list_usage]
    // mne.find_events: 25 onsets, codes up to 32, 6 of code 1, the first at 14385 - 12900
    ok &= expect(annotated.nevent() == 25 && annotated.maxOnset() == 32 && leftAuditory.nevent() == 6 && leftAuditory.events[0].sample == 14385 - 12900 && leftAuditory.events[0].comment == "left auditory" && relative->events[0].comment.isEmpty(),
                 "MNEEventList/MNEEvent: commented event file, onsets selected like mne.find_events");

    //! [mne_deriv_set_usage]
    // A bipolar EOG-like montage in MNE-C's text syntax, saved as a derivation file and applied while reading raw data
    const QString montage = tmp.filePath("montage.txt");
    QFile montageFile(montage);
    ok &= montageFile.open(QIODevice::WriteOnly | QIODevice::Text);
    montageFile.write("\"EEG001-EEG002\" = \"EEG001\" - \"EEG002\"\n\"FRONT\" = 0.5 * \"EEG001\" + 0.5 * \"EEG002\" - \"EEG999\"\n");
    montageFile.close();
    const auto derivations = MNEDerivSet::readText(montage);
    derivations->write(tmp.filePath("montage-deriv.fif"));
    const auto fromFif = MNEDerivSet::read(tmp.filePath("montage-deriv.fif"));
    const int usable = mneRaw->attachDerivations(*fromFif); // FRONT needs EEG999 and is dropped
    MNEChSelection bipolar;
    bipolar.chspick = bipolar.chspick_nospace = {"EEG001-EEG002", "EEG001", "EEG002"};
    bipolar.nchan = bipolar.ndef = 3;
    bipolar.nderiv = 1;
    bipolar.pick = (VectorXi(3) << -1, mneRaw->ch_names.indexOf("EEG001"), mneRaw->ch_names.indexOf("EEG002")).finished();
    bipolar.pick_deriv = (VectorXi(3) << 0, -1, -1).finished();
    mneRaw->proj.reset();
    mneRaw->pick_data(&bipolar, mneRaw->first_samp + 2990, ns, rows.data());
    //! [mne_deriv_set_usage]
    // The derived channel is the difference of its inputs, read in one pass
    ok &= expect(derivations->count() == 2 && fromFif->count() == 2 && usable == 1 && mneRaw->deriv_matched->deriv_data->nrow == 1 && segment.row(0).isApprox(segment.row(1) - segment.row(2), 1e-6f),
                 "MNEDerivSet/MNEDeriv/MNESparseNamedMatrix: MNE-C montage file, matched to the recording and read by MNERawData");

    //! [mne_layout_usage]
    // The Neuromag Vectorview layout: one viewport per channel
    auto layout = MNELayout::read(layoutDir + "/Vectorview-all.lout");
    const QMap<QString, QPointF> positions = layout->channelPositions();                     // lower-left corners, keyed "MEG 0113"
    const int leftHalf = layout->confine(QRectF(QPointF(-85.0, -83.0), QPointF(0.0, 75.0))); // zoom to the left half
    const int shownTriplet = layout->matchPorts(QStringList({"MEG0113", "MEG0112", "MEG0111"}));
    //! [mne_layout_usage]
    // mne.channels.read_layout("Vectorview-all.lout", scale=False): 306 viewports in -85 ... 90 x -83 ... 75, MEG 0113 at (-73.4162, 33.4167); 138 boxes left of x = 0
    ok &= expect(layout && layout->ports.size() == 306 && (positions.value("MEG 0113") - QPointF(-73.416206, 33.416687)).manhattanLength() < 1e-4 && leftHalf == 138 && shownTriplet == 3,
                 "MNELayout/MNELayoutPort: Vectorview layout like mne.channels.read_layout, zoom and channel matching");

    //! [mne_morph_map_usage]
    // Morph map between the registered spheres of two subjects: every vertex of "large" is a
    // barycentric blend of the three corners of the "small" triangle it falls into.
    const QString subjectsDir = QStringLiteral(MNE_MORPH_SUBJECTS_DIR);
    FsSurface smallSphere;
    FsSurface largeSphere;
    FsSurface::read(subjectsDir + "/small/surf/lh.sphere.reg", smallSphere, false);
    FsSurface::read(subjectsDir + "/large/surf/lh.sphere.reg", largeSphere, false);
    MNEMorphMap smallToLarge = MNEMorphMap::compute(smallSphere.rr(), smallSphere.tris(), largeSphere.rr());
    smallToLarge.from_subj = "small";
    smallToLarge.to_subj = "large";
    smallToLarge.hemi = 0;
    const QString mapFile = tmp.filePath("small-large-morph.fif");
    MNEMorphMap::write(mapFile, {&smallToLarge});
    const auto reread = MNEMorphMap::read(mapFile, "small", "large", 0);
    const Eigen::SparseMatrix<double> weights = reread->toEigen(); // 642 x 162, feeds INVLIB::SourceMorph
    //! [mne_morph_map_usage]
    // mne.read_morph_map("small", "large"): vertex 0 of "large" = 0.4372 * v0 + 0.3726 * v42 + 0.1902 * v51 of "small"
    ok &= expect(weights.rows() == 642 && weights.cols() == 162 && std::abs(weights.coeff(0, 0) - 0.43720984) < 1e-5 && std::abs(weights.coeff(0, 42) - 0.37262118) < 1e-5 && std::abs(weights.coeff(0, 51) - 0.19016896) < 1e-5,
                 "MNEMorphMap: sphere-registered morph map like mne.read_morph_map, saved and reloaded");

    qInfo().noquote() << (ok ? "All mne checks passed." : "mne checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
