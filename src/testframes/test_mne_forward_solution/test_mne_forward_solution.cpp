//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     test_mne_forward_solution.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     December, 2016
 * @brief    The forward solution test implementation
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/log.h>

#include <fwd/compute_fwd/compute_fwd_settings.h>
#include <fwd/compute_fwd/compute_fwd.h>
#include <mne/mne_forward_solution.h>

#include <algorithm>
#include <memory>
#include <mne/mne.h>

#include <fiff/fiff.h>
#include <fiff/fiff_ch_name.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_named_matrix.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
/**
 * DECLARE CLASS TestMneForwardSolution
 *
 * @brief The TestMneForwardSolution class provides dipole fit tests
 *
 */
class TestMneForwardSolution : public QObject
{
    Q_OBJECT

public:
    TestMneForwardSolution();

private slots:
    void initTestCase();
    void computeForward();
    void cleanupTestCase();

private:
    std::shared_ptr<MNEForwardSolution> m_pFwdMEGEEGRead;
    std::shared_ptr<MNEForwardSolution> m_pFwdMEGEEGRef;
};

//=============================================================================================================

TestMneForwardSolution::TestMneForwardSolution()
{
}

//=============================================================================================================

void TestMneForwardSolution::initTestCase()
{
    mne::Log::install_qt_message_handler();
}

//=============================================================================================================

void TestMneForwardSolution::computeForward()
{
    // Compute and Write Forward Solution
    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Compute/Write/Read MEG/EEG Forward Solution >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    // Read reference forward solution
    QString fwdMEGEEGFileRef(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif");
    m_pFwdMEGEEGRef = std::make_shared<MNEForwardSolution>(fwdMEGEEGFileRef.toStdU16String());

    //Following is equivalent to:
    //mne_forward_solution
    // --meg
    // --eeg
    // --accurate
    // --src ../resources/data/mne-cpp-test-data/subjects/sample/bem/sample-oct-6-src.fif
    // --meas ../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif
    // --mri ../resources/data/mne-cpp-test-data/MEG/sample/all-trans.fif
    // --bem ../resources/data/mne-cpp-test-data/subjects/sample/bem/sample-1280-1280-1280-bem.fif
    // --mindist 5
    // --fwd ../resources/data/mne-cpp-test-data/Result/sample_audvis-meg-eeg-oct-6-fwd.fif

    std::shared_ptr<ComputeFwdSettings> pSettingsMEGEEG = std::make_shared<ComputeFwdSettings>();

    pSettingsMEGEEG->include_meg = true;
    pSettingsMEGEEG->include_eeg = true;
    pSettingsMEGEEG->accurate = true;
    pSettingsMEGEEG->srcname = (QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/subjects/sample/bem/sample-oct-6-src.fif").toStdString();
    pSettingsMEGEEG->measname = (QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif").toStdString();
    pSettingsMEGEEG->mriname = (QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/all-trans.fif").toStdString();
    pSettingsMEGEEG->transname.clear();
    pSettingsMEGEEG->bemname = (QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/subjects/sample/bem/sample-1280-1280-1280-bem.fif").toStdString();
    pSettingsMEGEEG->mindist = 5.0f / 1000.0f;
    pSettingsMEGEEG->solname = (QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/Result/sample_audvis-meg-eeg-oct-6-fwd.fif").toStdString();


    mne::FiffRawData raw(pSettingsMEGEEG->measname);
    std::shared_ptr<mne::FiffInfo> pFiffInfo = std::make_shared<mne::FiffInfo>(raw.info);

    pSettingsMEGEEG->pFiffInfo = pFiffInfo;
    pSettingsMEGEEG->checkIntegrity();

    std::shared_ptr<ComputeFwd> pFwdComputer = std::make_shared<ComputeFwd>(pSettingsMEGEEG);
    auto pFwdSolution = pFwdComputer->calculateFwd();

    // recalculate with same meg_head_t to check that we still get the same result
    pFwdComputer->updateHeadPos(pFiffInfo->dev_head_t, *pFwdSolution);

    QVERIFY(pFwdSolution->write(mne::pathFromUtf8(pSettingsMEGEEG->solname)));

    // Read newly created fwd
    m_pFwdMEGEEGRead = std::make_shared<MNEForwardSolution>(mne::pathFromUtf8(pSettingsMEGEEG->solname));

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Compute/Write/Read MEG/EEG Forward Solution Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");

    // The reference was written while MNE-CPP dropped the spaces of channel names ("MEG0113"); the new
    // forward keeps the measurement's names ("MEG 0113"). Compare them as the same channels.
    const auto sameChannels = [](const mne::StringList& a, const mne::StringList& b) {
        return std::ranges::equal(a, b, [](const std::string& x, const std::string& y) { return mne::same_channel(x, y); });
    };
    QVERIFY(sameChannels(m_pFwdMEGEEGRead->info.ch_names, m_pFwdMEGEEGRef->info.ch_names));
    QVERIFY(sameChannels(m_pFwdMEGEEGRead->info.bads, m_pFwdMEGEEGRef->info.bads));
    mne::FiffInfoBase refInfo = m_pFwdMEGEEGRef->info;
    refInfo.ch_names = m_pFwdMEGEEGRead->info.ch_names;
    refInfo.bads = m_pFwdMEGEEGRead->info.bads;
    for (std::size_t k = 0; k < refInfo.chs.size(); ++k)
        refInfo.chs[k].ch_name = m_pFwdMEGEEGRead->info.chs[k].ch_name;
    QVERIFY(m_pFwdMEGEEGRead->info == refInfo);
    QVERIFY(m_pFwdMEGEEGRead->source_ori == m_pFwdMEGEEGRef->source_ori);
    QVERIFY(m_pFwdMEGEEGRead->surf_ori == m_pFwdMEGEEGRef->surf_ori);
    QVERIFY(m_pFwdMEGEEGRead->coord_frame == m_pFwdMEGEEGRef->coord_frame);
    QVERIFY(m_pFwdMEGEEGRead->nsource == m_pFwdMEGEEGRef->nsource);
    QVERIFY(m_pFwdMEGEEGRead->nchan == m_pFwdMEGEEGRef->nchan);

    // Compare sol data row-by-row (Eigen 5 isApprox on full large matrices can crash)
    QVERIFY(m_pFwdMEGEEGRead->sol.nrow == m_pFwdMEGEEGRef->sol.nrow);
    QVERIFY(m_pFwdMEGEEGRead->sol.ncol == m_pFwdMEGEEGRef->sol.ncol);
    QVERIFY(sameChannels(m_pFwdMEGEEGRead->sol.row_names, m_pFwdMEGEEGRef->sol.row_names));
    QVERIFY(m_pFwdMEGEEGRead->sol.col_names == m_pFwdMEGEEGRef->sol.col_names);
    for (Eigen::Index r = 0; r < m_pFwdMEGEEGRead->sol.data.rows(); ++r) {
        QVERIFY(m_pFwdMEGEEGRead->sol.data.row(r).isApprox(m_pFwdMEGEEGRef->sol.data.row(r), 0.0001));
    }

    // sol_grad may not be present in the forward solution files
    if (m_pFwdMEGEEGRead->sol_grad.nrow > 0 && m_pFwdMEGEEGRef->sol_grad.nrow > 0) {
        for (Eigen::Index r = 0; r < m_pFwdMEGEEGRead->sol_grad.data.rows(); ++r) {
            QVERIFY(m_pFwdMEGEEGRead->sol_grad.data.row(r).isApprox(m_pFwdMEGEEGRef->sol_grad.data.row(r), 0.0001));
        }
    }

    QVERIFY(m_pFwdMEGEEGRead->mri_head_t == m_pFwdMEGEEGRef->mri_head_t);
    // Note: Full src (MNESourceSpaces) deep comparison skipped here because
    // MNEHemisphere::operator== calls isApprox on large geometry matrices which
    // can crash with Eigen 5. Source positions and normals are verified below.
    QVERIFY(m_pFwdMEGEEGRead->source_rr.isApprox(m_pFwdMEGEEGRef->source_rr, 1e-4f));
    QVERIFY(m_pFwdMEGEEGRead->source_nn.isApprox(m_pFwdMEGEEGRef->source_nn, 1e-4f));

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Compare MEG/EEG Forward Solution Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

//=============================================================================================================

void TestMneForwardSolution::cleanupTestCase()
{
    QString fwdMEGEEGFileRef(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/Result/sample_audvis-meg-eeg-oct-6-fwd.fif");
    QFile::remove(fwdMEGEEGFileRef);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestMneForwardSolution)
#include "test_mne_forward_solution.moc"
