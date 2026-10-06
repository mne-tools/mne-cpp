//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Forward library: coil definitions, BEM and sphere models, field maps and forward solutions.
 *
 * Computes fields of a single dipole and a label-restricted forward solution for
 * the sample recording and compares them with MNE-Python 1.11
 * (make_forward_dipole, _make_surface_mapping, _compute_forwards_meeg).
 * Exits non-zero on any mismatch.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fwd/compute_fwd/compute_fwd.h>
#include <fwd/compute_fwd/compute_fwd_settings.h>
#include <fwd/fwd.h>
#include <fwd/fwd_bem_model.h>
#include <fwd/fwd_bem_solution.h>
#include <fwd/fwd_coil_set.h>
#include <fwd/fwd_eeg_sphere_model.h>
#include <fwd/fwd_eeg_sphere_model_set.h>
#include <fwd/fwd_field_map.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_raw_data.h>
#include <mne/mne_forward_solution.h>

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

using namespace FWDLIB;
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
    parser.addOption(dataOption);
    parser.process(app);
    const QString data = parser.value(dataOption);
    const QString coilDefs = QCoreApplication::applicationDirPath() + "/../resources/general/coilDefinitions/coil_def.dat";
    QTemporaryDir tmp;
    bool ok = tmp.isValid();

    QFile rawFile(data + "/MEG/sample/sample_audvis_trunc_raw.fif");
    const FiffRawData raw(rawFile);
    QList<FiffChInfo> meg;
    QList<FiffChInfo> goodMeg;
    QList<FiffChInfo> eeg;
    for (const FiffChInfo& ch : raw.info.chs) {
        if (ch.kind == FIFFV_MEG_CH) {
            meg << ch;
            if (!raw.info.bads.contains(ch.ch_name)) {
                goodMeg << ch;
            }
        } else if (ch.kind == FIFFV_EEG_CH) {
            eeg << ch;
        }
    }

    // One dipole in head coordinates, used by the single-dipole checks below.
    const Vector3f rd(0.01f, -0.02f, 0.05f);
    const Vector3f Q = Vector3f(0.3f, -0.4f, 0.866f).normalized();

    //! [fwd_coil_set_usage]
    auto templates = FwdCoilSet::read_coil_defs(coilDefs); // integration points per coil type
    auto coils = templates->create_meg_coils(meg, meg.size(), FWD_COIL_ACCURACY_ACCURATE, raw.info.dev_head_t);
    const FwdCoil& first = *coils->coils[0]; // MEG 0113 in head coordinates
    //! [fwd_coil_set_usage]
    // mne _create_meg_coils(accurate): 3012 planar gradiometer, 8 points, r0 (-106.15, 29.14, -14.73) mm
    ok &= expect(coils->ncoil() == 306 && first.type == 3012 && first.np == 8 && coils->is_planar_coil_type(first.type) && (first.r0 - Vector3f(-0.10614990f, 0.02914091f, -0.01472596f)).norm() < 1e-6f,
                 "FwdCoilSet/FwdCoil: 306 accurate MEG coils, MEG0113 a planar gradiometer with 8 points");

    //! [fwd_bem_model_usage]
    auto bem = FwdBemModel::fwd_bem_load_homog_surface(data + "/subjects/sample/bem/sample-5120-bem.fif");
    bem->fwd_bem_load_recompute_solution(data + "/subjects/sample/bem/sample-5120-bem-sol.fif", FWD_BEM_LINEAR_COLL, 0);
    bem->fwd_bem_set_head_mri_t(FiffCoordTrans::readMriTransform(data + "/MEG/sample/all-trans.fif"));
    bem->fwd_bem_specify_coils(coils.get()); // stores a FwdBemSolution per coil set in coils->user_data
    VectorXf B(coils->ncoil());
    FwdBemModel::fwd_bem_field(rd, Q, *coils, B, bem.get()); // T per A m
    //! [fwd_bem_model_usage]
    // mne.make_forward_dipole with the same BEM: |B| = 2.3646584e-4, B[0] = -7.2922803e-6
    ok &= expect(coils->user_data && coils->user_data->ncoil == 306 && near(B.norm(), 2.3646584e-4, 1e-3) && near(B[0], -7.2922803e-6, 1e-2),
                 "FwdBemModel/FwdBemSolution: single-layer BEM field matches mne.make_forward_dipole");

    //! [fwd_eeg_sphere_model_usage]
    std::unique_ptr<FwdEegSphereModelSet> models(FwdEegSphereModelSet::fwd_add_default_eeg_sphere_model(nullptr));
    std::unique_ptr<FwdEegSphereModel> sphere(models->fwd_select_eeg_sphere_model("Default")); // 4 layers: brain, CSF, skull, scalp
    // Scalp radius 90 mm; 3 Berg-Scherg dipoles approximate the layered potentials
    sphere->fwd_setup_eeg_sphere_model(0.09f, true, 3);
    sphere->r0 = Vector3f(0.0f, 0.0f, 0.04f);
    auto els = FwdCoilSet::create_eeg_els(eeg, eeg.size());
    VectorXf V(els->ncoil());
    FwdEegSphereModel::fwd_eeg_spherepot_coil(rd, Q, *els, V, sphere.get()); // V per A m
    //! [fwd_eeg_sphere_model_usage]
    // mne.make_sphere_model(r0=(0, 0, 0.04), head_radius=0.09): layers at 81, 82.8, 87.3, 90 mm; |V| = 452.86725
    ok &= expect(sphere->nlayer() == 4 && near(sphere->layers[2].rel_rad, 0.97, 1e-6) && near(sphere->layers[2].sigma, 0.004, 1e-6) && near(V.norm(), 452.86725, 1e-3) && near(V[0], 12.079552, 1e-2),
                 "FwdEegSphereModel/FwdEegSphereModelSet/FwdEegSphereLayer: 4-layer sphere potentials match mne");

    //! [fwd_field_map_usage]
    auto normalCoils = templates->create_meg_coils(goodMeg, goodMeg.size(), FWD_COIL_ACCURACY_NORMAL, raw.info.dev_head_t);
    const Vector3f origin(0.0f, 0.0f, 0.04f);
    MatrixX3f surfRr(3, 3);
    surfRr << 0.0f, 0.0f, 0.12f, 0.08f, 0.0f, 0.08f, 0.0f, -0.09f, 0.07f;
    const MatrixX3f surfNn = (surfRr.rowwise() - origin.transpose()).rowwise().normalized();
    auto mapping = FwdFieldMap::computeMegMapping(*normalCoils, surfRr, surfNn, origin); // nvert x nchan
    //! [fwd_field_map_usage]
    // mne _make_surface_mapping(mode="accurate", origin=(0, 0, 0.04)), no projectors: row norms 10.0598, 1.88928, 0.69807
    ok &= expect(mapping && mapping->rows() == 3 && mapping->cols() == 305 && near(mapping->row(0).norm(), 10.059843, 1e-3) && near(mapping->row(1).norm(), 1.8892755, 1e-3) && near(mapping->row(2).norm(), 0.69807071, 1e-3),
                 "FwdFieldMap maps 305 good MEG channels onto surface points like mne");

    //! [compute_fwd_usage]
    // mne_forward_solution --meg --eeg --accurate --label V1-lh.label with sphere models instead of a BEM
    const QString label = tmp.filePath("V1-lh.label"); // the hemisphere is taken from the "-lh.label" suffix
    QFile::copy(data + "/subjects/sample/label/lh.V1.label", label);
    auto settings = std::make_shared<ComputeFwdSettings>();
    settings->include_meg = true;
    settings->include_eeg = true;
    settings->accurate = true;
    settings->srcname = data + "/subjects/sample/bem/sample-oct-6-src.fif";
    settings->measname = rawFile.fileName();
    settings->mriname = data + "/MEG/sample/all-trans.fif";
    settings->r0 = origin;
    settings->eeg_sphere_rad = 0.09f;
    settings->mindist = 0.0f;
    settings->filter_spaces = false;
    settings->labels = {label};
    settings->nlabel = 1;
    settings->solname = tmp.filePath("V1-fwd.fif");
    settings->pFiffInfo = QSharedPointer<FiffInfo>::create(raw.info);
    settings->checkIntegrity();
    auto fwd = std::make_shared<ComputeFwd>(settings)->calculateFwd();
    QFile solFile(settings->solname);
    fwd->write(solFile);
    //! [compute_fwd_usage]
    // mne _compute_forwards_meeg on the 78 V1 sources: |G| of source 0, x component, over 306 MEG channels
    ok &= expect(fwd && fwd->nsource == 78 && fwd->sol->data.rows() == 366 && near(fwd->sol->data.col(0).head(306).norm(), 1.2455023232288232e-3, 1e-4),
                 "ComputeFwd: 78 V1 sources, 366 channels, sphere-model gain matches mne");

    //! [fwd_read_usage]
    MNEForwardSolution back;
    Fwd::read_forward_solution(solFile, back); // all 366 rows, like mne.read_forward_solution
    MNEForwardSolution good;
    MNEForwardSolution::read(solFile, good, false, false, {}, {}, true); // without the channels marked bad
    //! [fwd_read_usage]
    ok &= expect(back.nsource == 78 && back.nchan == 366 && good.nchan == 364 && (back.sol->data - fwd->sol->data).norm() <= 1e-6 * fwd->sol->data.norm(),
                 "Fwd::read_forward_solution reads all 366 rows back; bExcludeBads drops the 2 bads");

    qInfo().noquote() << (ok ? "All fwd checks passed." : "fwd checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
