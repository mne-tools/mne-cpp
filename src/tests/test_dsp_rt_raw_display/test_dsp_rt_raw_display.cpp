//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_dsp_rt_raw_display.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Unit tests for DisplayCorrection and RtRawDisplay.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/dsp/display_correction.h>
#include <mne/dsp/rt/rt_raw_display.h>
#include <mne/fiff/fiff_ch_info.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_proj.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest/QtTest>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <memory>
#include <string>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

namespace
{

// Three EEG channels and one stimulus channel.
std::shared_ptr<FiffInfo> makeInfo()
{
    auto info = std::make_shared<FiffInfo>();
    info->sfreq = 100.0;
    for (int i = 0; i < 4; ++i) {
        FiffChInfo ch;
        ch.kind = i < 3 ? FIFFV_EEG_CH : FIFFV_STIM_CH;
        ch.ch_name = i < 3 ? "EEG00" + std::to_string(i + 1) : "STI014";
        ch.range = 1.0f;
        ch.cal = 1.0f;
        info->chs.push_back(ch);
        info->ch_names.push_back(ch.ch_name);
    }
    info->nchan = static_cast<int>(info->chs.size());
    return info;
}

// Average reference over the three EEG channels.
FiffProj averageReference(bool active)
{
    StringList rows;
    StringList cols = {"EEG001", "EEG002", "EEG003"};
    MatrixXd vec = RowVector3d::Constant(1.0 / std::sqrt(3.0));
    FiffNamedMatrix data(1, 3, rows, cols, vec);
    return FiffProj(FIFFV_PROJ_ITEM_EEG_AVREF, active, "Average EEG reference", data);
}

MatrixXd wave(Index channels, Index samples, double phase = 0.0)
{
    MatrixXd data(channels, samples);
    for (Index c = 0; c < channels; ++c) {
        for (Index t = 0; t < samples; ++t) {
            data(c, t) = std::sin(0.3 * static_cast<double>(t + c) + phase) + 0.1 * static_cast<double>(c);
        }
    }
    return data;
}

FilterKernel lowPass(int order)
{
    return FilterKernel("lp", 0, order, 0.2, 0.0, 0.05, 100.0, 0);
}

// Linear convolution of a row with a kernel's coefficients.
RowVectorXd convolve(const RowVectorXd& x, const RowVectorXd& h)
{
    RowVectorXd y = RowVectorXd::Zero(x.size() + h.size() - 1);
    for (Index i = 0; i < x.size(); ++i) {
        y.segment(i, h.size()) += x(i) * h;
    }
    return y;
}

} // namespace

class TestDspRtRawDisplay : public QObject
{
    Q_OBJECT

private slots:
    void correction_isIdentityUntilActivated()
    {
        auto info = makeInfo();
        info->projs.push_back(averageReference(false));

        DisplayCorrection correction;
        correction.reset(4);
        correction.update_projection(*info);
        correction.update_compensator(*info, 0);

        QVERIFY(!correction.projection_active());
        QVERIFY(!correction.compensator_active());
        const MatrixXd data = wave(4, 16);
        QVERIFY(correction.apply(data).isApprox(data));
    }

    void correction_appliesActiveProjectorWithBadsZeroed()
    {
        auto info = makeInfo();
        info->projs.push_back(averageReference(true));
        info->bads.push_back("EEG002");

        DisplayCorrection correction;
        correction.reset(4);
        correction.update_projection(*info);

        QVERIFY(correction.projection_active());
        QCOMPARE(correction.projector().col(1).norm(), 0.0);

        const MatrixXd data = wave(4, 16);
        QVERIFY(correction.apply(data).isApprox(correction.projector() * data));

        // A block with another channel count passes through.
        const MatrixXd other = wave(3, 16);
        QVERIFY(correction.apply(other).isApprox(other));
    }

    void rawDisplay_correctsAndDetectsTriggersOnTheRawBlock()
    {
        auto info = makeInfo();
        info->projs.push_back(averageReference(true));

        RtRawDisplay display;
        display.set_info(info);
        display.update_projection();
        display.set_trigger_detection(true, 3, 0.5);

        MatrixXd block = wave(4, 64);
        block.row(3).setZero();
        block.row(3).segment(20, 5).setConstant(1.0);

        const RtRawDisplay::Corrected corrected = display.correct(block);
        QVERIFY(corrected.data.isApprox(display.correction().projector() * block));
        QCOMPARE(corrected.triggers.size(), std::size_t{1});
        QCOMPARE(corrected.triggers.front().sample, 20);
    }

    void rawDisplay_spharaIsIdentityWithoutBases()
    {
        RtRawDisplay display;
        display.set_info(makeInfo());
        display.update_sphara_options("EEG", 2, 0);
        QVERIFY(!display.applies_sphara(4));
        display.set_sphara_active(true);
        QVERIFY(display.applies_sphara(4));
        QVERIFY(!display.applies_sphara(3));
        QVERIFY(MatrixXd(display.sphara_operator()).isIdentity());
    }

    void rawDisplay_filtersSelectedChannelsWithOverlapAdd()
    {
        auto info = makeInfo();
        RtRawDisplay display;
        display.set_info(info);
        display.set_filter_channel_type("EEG");
        display.set_filters({lowPass(16)});
        display.set_filter_active(true);
        QVERIFY(display.filtering());
        QCOMPARE(display.filter_delay(), display.max_filter_length() / 2);

        const int length = display.max_filter_length();
        QVERIFY(!display.filter(wave(4, length - 1)).has_value());

        const MatrixXd first = wave(4, 64);
        const MatrixXd second = wave(4, 64, 64 * 0.3);

        const std::optional<RtRawDisplay::Filtered> a = display.filter(first);
        QVERIFY(a.has_value());
        QCOMPARE(a->filtered_rows, (std::vector<int>{0, 1, 2}));
        QCOMPARE(a->passthrough_rows, (std::vector<int>{3}));
        QCOMPARE(a->zero_columns, length);
        QCOMPARE(a->data.row(0).head(length).norm(), 0.0);
        QVERIFY(a->data.row(3).isApprox(first.row(3)));

        // The second block continues the convolution of both blocks without a seam.
        const std::optional<RtRawDisplay::Filtered> b = display.filter(second);
        QVERIFY(b.has_value());
        QCOMPARE(b->zero_columns, 0);

        MatrixXd both(4, 128);
        both << first, second;
        const RowVectorXd expected = convolve(both.row(1), display.filters().front().coefficients());
        const RowVectorXd got = b->data.row(1);
        QVERIFY((got - expected.segment(64, 64)).cwiseAbs().maxCoeff() < 1e-9);
    }

    void rawDisplay_newKernelsDropTheStartupTransient()
    {
        RtRawDisplay display;
        display.set_info(makeInfo());
        display.set_filter_channel_type("All");
        display.set_filters({lowPass(16)});
        display.set_filter_active(true);
        QVERIFY(display.filter(wave(4, 64)).has_value());
        QCOMPARE(display.filter(wave(4, 64))->zero_columns, 0);

        display.set_filters({lowPass(32)});
        const std::optional<RtRawDisplay::Filtered> after = display.filter(wave(4, 64));
        QVERIFY(after.has_value());
        QCOMPARE(after->zero_columns, display.max_filter_length());
        QCOMPARE(after->filtered_rows, (std::vector<int>{0, 1, 2}));
    }
};

QTEST_GUILESS_MAIN(TestDspRtRawDisplay)
#include "test_dsp_rt_raw_display.moc"
