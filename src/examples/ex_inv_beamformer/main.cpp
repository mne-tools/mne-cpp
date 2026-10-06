//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Inverse library: LCMV and DICS beamformers and the RAP-MUSIC family of dipole scanners.
 *
 * Two sources in lh.V1 (20 and 30 nAm, normal orientation) are added to the
 * sample noise covariance. LCMV and DICS must find them with the power
 * mne.beamformer.apply_lcmv_cov / apply_dics_csd computes on the same input;
 * RAP-MUSIC, PWL-RAP-MUSIC and TRAP-MUSIC must recover them from noise-free data,
 * where the true sources are the oracle. Exits non-zero on any mismatch.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <inv/beamformer/inv_beamformer.h>
#include <inv/beamformer/inv_dics.h>
#include <inv/beamformer/inv_lcmv.h>
#include <inv/inv_source_estimate.h>
#include <inv/rap_music/inv_dipole.h>
#include <inv/rap_music/inv_pwl_rap_music.h>
#include <inv/rap_music/inv_rap_music.h>
#include <inv/rap_music/inv_trap_music.h>
#include <mne/mne_forward_solution.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_evoked.h>
#include <fs/fs_label.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QFile>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <cstdlib>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace MNELIB;
using namespace FIFFLIB;
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

bool isPair(const InvDipolePair<double>& pair, int a, int b)
{
    return std::min(pair.m_iIdx1, pair.m_iIdx2) == std::min(a, b) && std::max(pair.m_iIdx1, pair.m_iIdx2) == std::max(a, b);
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
    bool ok = true;

    // The 305 good MEG channels; the forward names them without the space ("MEG0113").
    QFile aveFile(data + "/MEG/sample/sample_audvis-ave.fif");
    const FiffEvoked sample(aveFile, 0);
    QStringList good;
    for (const QString& name : sample.info.ch_names) {
        if (name.startsWith("MEG") && !sample.info.bads.contains(name))
            good << name;
    }
    FsLabel v1;
    FsLabel::read(data + "/subjects/sample/label/lh.V1.label", v1);
    QFile fwdFile(data + "/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif");
    MNEForwardSolution fwd = MNEForwardSolution(fwdFile).pick_regions({v1}).pick_channels(good);
    FiffEvoked evoked = sample.pick_channels(good);
    const FiffInfo info = evoked.info;
    QFile covFile(data + "/MEG/sample/sample_audvis-cov.fif");
    const FiffCov noiseCov = FiffCov(covFile).pick_channels(good).prepare_noise_cov(info, good);

    // 20 nAm at V1 source 10 and 30 nAm at source 50, both along z.
    const MatrixXd& G = fwd.sol->data;
    const VectorXd g1 = G.col(3 * 10 + 2) * 20e-9;
    const VectorXd g2 = G.col(3 * 50 + 2) * 30e-9;

    //! [inv_lcmv_usage]
    FiffCov dataCov = noiseCov; // noise plus the two sources
    dataCov.data += g1 * g1.transpose() + g2 * g2.transpose();
    const InvBeamformer lcmv = InvLCMV::makeLCMV(info, fwd, dataCov, 0.05, noiseCov,
                                                 BeamformerPickOri::MaxPower, BeamformerWeightNorm::UnitNoiseGain);
    const VectorXd lcmvPower = InvLCMV::applyLCMVCov(dataCov, lcmv).data.col(0); // one value per source
    //! [inv_lcmv_usage]
    // mne.beamformer.make_lcmv(..., pick_ori="max-power", weight_norm="unit-noise-gain") + apply_lcmv_cov:
    // sum 117.705, source 10 16.5306, source 50 3.92173
    Index lcmvBest = 0;
    lcmvPower.maxCoeff(&lcmvBest);
    ok &= expect(lcmv.isValid() && lcmvPower.size() == 70 && lcmvBest == 10 && near(lcmvPower.sum(), 117.70495401512653, 1e-5) && near(lcmvPower(10), 16.530644994286707, 1e-5) && near(lcmvPower(50), 3.9217347708280537, 1e-5),
                 "InvLCMV/InvBeamformer: max-power LCMV power on V1 matches mne.beamformer.apply_lcmv_cov");

    //! [inv_dics_usage]
    // A single real cross-spectral density at 10 Hz equal to the data covariance gives the LCMV power.
    const VectorXd frequencies = VectorXd::Constant(1, 10.0);
    const InvBeamformer dics = InvDICS::makeDICS(info, fwd, {dataCov.data}, frequencies, 0.05, true, noiseCov,
                                                 BeamformerPickOri::MaxPower, BeamformerWeightNorm::UnitNoiseGain);
    const VectorXd dicsPower = InvDICS::applyDICSCsd({dataCov.data}, frequencies, dics).data.col(0);
    //! [inv_dics_usage]
    // mne.beamformer.make_dics(..., real_filter=True, depth=None) + apply_dics_csd: sum 117.705, source 10 16.5306
    ok &= expect(dics.isValid() && near(dicsPower.sum(), 117.70495401992135, 1e-5) && near(dicsPower(10), 16.530644991359814, 1e-5),
                 "InvDICS: DICS power for a 10 Hz CSD matches mne.beamformer.apply_dics_csd");

    //! [inv_rap_music_usage]
    // Noise-free data from the two sources with different time courses; RAP-MUSIC scans source pairs.
    const int nSamples = 60;
    MatrixXd measured(G.rows(), nSamples);
    for (int t = 0; t < nSamples; ++t)
        measured.col(t) = g1 * std::sin(0.21 * t) + g2 * std::sin(0.53 * t + 0.4);
    QList<InvDipolePair<double>> pairs;
    InvRapMusic rap(fwd, false, 1, 0.5); // one correlated pair, subspace correlation >= 0.5
    rap.calculateInverse(measured, pairs);
    QList<InvDipolePair<double>> powellPairs;
    InvPwlRapMusic pwlRap(fwd, false, 1, 0.5); // Powell coordinate search: far fewer pairs, may stop at a local optimum
    pwlRap.calculateInverse(measured, powellPairs);
    //! [inv_rap_music_usage]
    ok &= expect(pairs.size() == 1 && isPair(pairs[0], 10, 50) && pairs[0].m_vCorrelation > 1.0 - 1e-6 && powellPairs.size() == 1 && powellPairs[0].m_vCorrelation > 0.99,
                 QString("InvRapMusic/InvPwlRapMusic/InvDipolePair: the exhaustive scan finds sources 10 and 50 (correlation 1), Powell a pair at %1")
                     .arg(powellPairs.isEmpty() ? 0.0 : powellPairs[0].m_vCorrelation, 0, 'f', 4));

    //! [inv_trap_music_usage]
    MatrixX3d positions = fwd.source_rr.cast<double>();
    InvTrapMusic trap(2, 0.85); // at most 2 dipoles, stop below a subspace correlation of 0.85
    const QList<TrapMusicDipole> dipoles = trap.compute(G, measured, positions, 3);
    //! [inv_trap_music_usage]
    QVector<int> found;
    for (const TrapMusicDipole& d : dipoles)
        found << d.sourceIdx;
    std::sort(found.begin(), found.end());
    ok &= expect(found == QVector<int>({10, 50}) && std::abs(dipoles[0].orientation.z()) > 0.999,
                 "InvTrapMusic: TRAP-MUSIC finds both sources one at a time, oriented along z");

    qInfo().noquote() << (ok ? "All beamformer checks passed." : "beamformer checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
