//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Umbrella header of mne_acq, the headless acquisition and stimulation library (C++20 standard library only).
 *
 * Quick start (the complete program; Eigen via the optional acq_eigen.h):
 * @code
 * #include <mne/acq/acq_eigen.h>
 * #include <cstdio>
 *
 * using namespace mne;
 * using namespace std::chrono_literals;
 *
 * int main()
 * {
 *     auto source = AcqDevice::open("synthetic://");
 *     if (!source) {
 *         std::fprintf(stderr, "%s\n", source.error().text().c_str());
 *         return 1;
 *     }
 *     auto reader = source->stream().reader(AcqReader::ReadMode::Lossless);
 *     source->start();
 *     Eigen::MatrixXf data;                                // channels x samples, physical units
 *     for (int i = 0; i < 10; ++i) {
 *         auto block = read(*reader, 250, data, 1s);
 *         if (!block) { std::fprintf(stderr, "%s\n", block.error().text().c_str()); return 1; }
 *         std::printf("first sample %llu, lost %llu, ch0 mean %g\n",
 *                     static_cast<unsigned long long>(block->firstSample),
 *                     static_cast<unsigned long long>(block->lost), data.row(0).mean());
 *     }
 *     return 0;                                            // ~AcqDevice stops and closes
 * }
 * @endcode
 *
 * Layers: AcqDevice and AcqDeviceRegistry form the control path; AcqStream, AcqReader, AcqWriter and
 * the views form the data path, which after start() performs no heap allocation and takes no lock, in
 * both directions. Backend authors implement AcqBackend. The library builds without Qt, Eigen or
 * exceptions for embedded targets; FIFF users include fiff/acq_fiff.h and link mne_acq_fiff.
 */

#ifndef ACQ_H
#define ACQ_H

#include "acq_global.h"
#include "acq_backend.h"
#include "acq_control.h"
#include "acq_block_view.h"
#include "acq_clock_model.h"
#include "acq_convert.h"
#include "acq_error.h"
#include "acq_memory.h"
#include "acq_reader.h"
#include "acq_sample_format.h"
#include "acq_device.h"
#include "acq_device_registry.h"
#include "acq_stream.h"
#include "acq_stream_info.h"
#include "acq_thread.h"
#include "acq_timebase.h"

#endif // ACQ_H
