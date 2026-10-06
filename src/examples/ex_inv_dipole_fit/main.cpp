//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Inverse library: equivalent current dipole fitting in a sphere model.
 *
 * The field of a 30 nAm dipole at (30, 20, 70) mm is computed for the 305 good
 * MEG channels of the sample recording, written as an evoked file and fitted
 * back, once sample by sample (InvDipoleFitData, InvGuessData) and once through
 * the full pipeline (InvDipoleFitSettings, InvDipoleFit). The result must match
 * mne.fit_dipole on the same data (sphere at (0, 0, 40) mm, ad-hoc noise).
 * Exits non-zero on any mismatch.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <inv/dipole_fit/inv_dipole_fit.h>
#include <inv/dipole_fit/inv_dipole_fit_data.h>
#include <inv/dipole_fit/inv_dipole_fit_settings.h>
#include <inv/dipole_fit/inv_dipole_forward.h>
#include <inv/dipole_fit/inv_ecd.h>
#include <inv/dipole_fit/inv_ecd_set.h>
#include <inv/dipole_fit/inv_guess_data.h>
#include <fiff/fiff_evoked_set.h>

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

#include <cstdlib>
#include <memory>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
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
    const QString sampleAve = parser.value(dataOption) + "/MEG/sample/sample_audvis-ave.fif";
    QTemporaryDir tmp;
    bool ok = tmp.isValid();

    const Vector3f truePos(0.03f, 0.02f, 0.07f);
    const Vector3f trueMoment = (30e-9 * Vector3d(1.0, 0.0, 0.2).normalized()).cast<float>();
    // mne.fit_dipole on the same data: the radial part of the moment is silent in a sphere
    const Vector3d mnePosMm(30.00945623584166, 19.97976658598626, 69.98608396579458);
    const Vector3d mneMomentNAm(14.972583573738602, -9.63296316190587, -8.565797930810295);

    //! [inv_dipole_fit_data_usage]
    // MEG channels and SSP projectors from the sample file, sphere at (0, 0, 40) mm, ad-hoc noise
    // (5e-13 T/m gradiometers, 20 fT magnetometers).
    Vector3f r0(0.0f, 0.0f, 0.04f);
    std::unique_ptr<InvDipoleFitData> fitData(InvDipoleFitData::setup_dipole_fit_data(
        QString(), sampleAve, QString(), &r0, nullptr, false, QString(), QString(),
        5e-13f, 20e-15f, 0.2e-6f, 0.1f, 0.1f, 0.1f, false, {sampleAve}, true, false));
    MatrixXf gain(fitData->nmeg, 3);
    const bool fieldOk = InvDipoleFitData::compute_dipole_field(*fitData, truePos, false, gain) == 0; // projected, not whitened
    VectorXf field = gain * trueMoment;

    InvGuessData guesses(QString(), QString(), 0.0f, 0.02f, 0.015f, fitData.get(), 0.08f); // 15 mm grid in an 80 mm sphere
    std::unique_ptr<InvDipoleForward> atTruth(InvDipoleFitData::dipole_forward_one(fitData.get(), truePos, nullptr));
    InvEcd dipole;
    VectorXf b = field; // fit_one projects and whitens in place
    const bool fitted = InvDipoleFitData::fit_one(fitData.get(), &guesses, 0.0f, b, false, dipole);
    //! [inv_dipole_fit_data_usage]
    ok &= expect(fieldOk && fitted && fitData->nmeg == 305 && guesses.nguess == 627 && atTruth && atTruth->sing.size() == 3 && dipole.valid && (dipole.rd - truePos).norm() < 1e-3f && (1e9 * dipole.Q.cast<double>() - mneMomentNAm).norm() < 0.02 * mneMomentNAm.norm() && dipole.good > 0.9999f,
                 "InvDipoleFitData/InvGuessData/InvDipoleForward/InvEcd: one sample fits the dipole like mne.fit_dipole");

    // Hand the field to the pipeline as an evoked file with two samples.
    QFile aveFile(sampleAve);
    FiffEvokedSet evokedSet(aveFile);
    FiffEvoked evoked = evokedSet.evoked[0].pick_channels(fitData->ch_names.mid(0, fitData->nmeg));
    evoked.info.bads.clear();
    evoked.data = field.cast<double>().replicate(1, 2);
    evoked.first = 0;
    evoked.last = 1;
    evoked.times = RowVectorXf::LinSpaced(2, 0.0f, 1.0f / static_cast<float>(evoked.info.sfreq));
    evoked.nave = 1;
    FiffEvokedSet synthetic;
    synthetic.info = evoked.info;
    synthetic.evoked.append(evoked);
    ok &= expect(synthetic.save(tmp.filePath("synthetic-ave.fif")), "FiffEvokedSet writes the synthetic field");

    //! [inv_dipole_fit_usage]
    InvDipoleFitSettings settings; // the options of the MNE-C mne_dipole_fit tool
    settings.measname = tmp.filePath("synthetic-ave.fif");
    settings.include_meg = true;
    settings.include_eeg = false;
    settings.guess_mindist = 0.0f;
    settings.tmin = 0.0f;
    settings.dipname = tmp.filePath("fit.dat"); // MNE-C .dat dipole file
    settings.checkIntegrity();
    InvDipoleFit fit(&settings);
    InvEcdSet dipoles = fit.calculateFit();
    const bool saved = dipoles.save_dipoles_dip(tmp.filePath("fit.dip")); // readable by mne.read_dipole
    const InvEcdSet reread = InvEcdSet::read_dipoles_dip(tmp.filePath("fit.dip"));
    //! [inv_dipole_fit_usage]
    const Vector3d posMm = dipoles.size() > 0 ? Vector3d(1e3 * dipoles[0].rd.cast<double>()) : Vector3d::Zero();
    ok &= expect(saved && dipoles.size() == 1 && dipoles[0].valid && (posMm - mnePosMm).norm() < 1.0 && std::abs(100.0 * dipoles[0].good - 99.99996959113513) < 0.01 && dipoles[0].nfree == 297 && reread.size() == 1 && (reread[0].rd - dipoles[0].rd).norm() < 1e-4f,
                 "InvDipoleFitSettings/InvDipoleFit/InvEcdSet: the pipeline fit matches mne.fit_dipole and round-trips through .dip");

    qInfo().noquote() << (ok ? "All dipole fit checks passed." : "dipole fit checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
