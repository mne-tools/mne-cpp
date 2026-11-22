//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Downstream consumer of an installed MNE-CPP SDK: acquire, filter and describe one second of data.
 *
 * Reads one second of the seeded synthetic source (mne_acq), low-pass filters every channel (mne_dsp) and
 * builds FIFF measurement info for it (mne_fiff). Exits non-zero when any step fails.
 */

#include <mne/acq/acq.h>
#include <mne/acq/acq_eigen.h>
#include <mne/dsp/iirfilter.h>
#include <mne/fiff/fiff_info.h>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>

int main()
{
    using namespace std::chrono_literals;

    auto device = mne::AcqDevice::open("synthetic://?eeg=4&sfreq=250&duration=1&realtime=0&seed=3");
    if (!device) {
        std::fprintf(stderr, "open: %s\n", device.error().text().c_str());
        return EXIT_FAILURE;
    }
    auto reader = device->stream().reader(mne::AcqReader::ReadMode::Lossless);
    if (!reader || !device->start().ok()) {
        std::fprintf(stderr, "cannot start the source\n");
        return EXIT_FAILURE;
    }
    const auto info = reader->stream().info();
    Eigen::MatrixXd data(static_cast<Eigen::Index>(info->channels.size()), 250);
    auto view = reader->acquire(250, 5s, 250);
    if (!view || !mne::decode(*view, mne::ref(data)).ok()) {
        std::fprintf(stderr, "cannot read one second\n");
        return EXIT_FAILURE;
    }
    static_cast<void>(reader->release(*view));
    device->close();

    const auto sos = mne::IirFilter::designButterworth(4, mne::IirFilter::LowPass, 30.0, 0.0, info->sfreq);
    const Eigen::MatrixXd filtered = mne::IirFilter::applyZeroPhaseMatrix(data, sos);

    mne::FiffInfo fiffInfo;
    fiffInfo.sfreq = static_cast<float>(info->sfreq);
    fiffInfo.nchan = static_cast<int>(info->channels.size());

    const bool ok = filtered.allFinite() && filtered.rows() == data.rows() && fiffInfo.nchan == 5 && filtered.row(0).cwiseAbs().maxCoeff() > 0.0;
    std::printf("%s: %d channels at %g Hz, peak %g after a 30 Hz low-pass\n", ok ? "OK" : "FAILED", fiffInfo.nchan,
                static_cast<double>(fiffInfo.sfreq), filtered.row(0).cwiseAbs().maxCoeff());
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
