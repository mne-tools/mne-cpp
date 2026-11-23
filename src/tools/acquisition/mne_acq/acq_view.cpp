//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_view.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    mne_acq view: live traces of a source in a sweep display (ChannelDataView).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_view.h"

#include <mne/acq/acq_eigen.h>
#include <mne/acq/fiff/acq_fiff.h>
#include <mne/disp/viewers/channeldataview.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QElapsedTimer>
#include <QTextStream>
#include <QTimer>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::disp;
using namespace std::chrono_literals;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

int runAcqView(AcqDevice& device, double window, double duration, const QString& screenshot)
{
    QTextStream err(stderr);
    auto reader = device.stream().reader(AcqReader::ReadMode::Lossless);
    if (!reader) {
        err << "mne_acq: " << reader.error().text().c_str() << "\n";
        return 2;
    }
    const auto streamInfo = reader->stream().info();
    auto fiffInfo = toFiffInfo(*streamInfo);
    if (!fiffInfo) {
        err << "mne_acq: " << fiffInfo.error().text().c_str() << "\n";
        return 2;
    }
    const auto channels = static_cast<Eigen::Index>(streamInfo->channels.size());
    // Sweep display: a fixed window that the newest samples overwrite from left to right, so traces stand still.
    const auto width = std::max<Eigen::Index>(2, static_cast<Eigen::Index>(std::lround(window * streamInfo->sfreq)));

    ChannelDataView view(QStringLiteral("MNEACQ/view"));
    view.setWindowTitle(QStringLiteral("mne_acq view %1").arg(QString::fromStdString(device.description().uri)));
    view.resize(1200, 700);
    view.init(std::make_shared<mne::FiffInfo>(*fiffInfo));
    view.setOverviewBarVisible(false);
    view.setWindowSize(static_cast<float>(window));
    Eigen::MatrixXd sweep = Eigen::MatrixXd::Zero(channels, width);
    view.setFileBounds(0, static_cast<int>(width) - 1);
    view.setData(sweep, 0);
    view.scrollToSample(0, false);
    view.show();

    if (const AcqError started = device.start(); !started.ok()) {
        err << "mne_acq: " << started.text().c_str() << "\n";
        return 2;
    }

    std::vector<AcqEvent> events;
    Eigen::MatrixXd block;
    std::uint64_t next = 0;
    bool ended = false;
    bool shotSaved = true;
    QElapsedTimer elapsed;
    elapsed.start();
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&] {
        bool added = false;
        for (;;) {
            auto acquired = reader->acquire(0, 0ms);
            if (!acquired) {
                const AcqErrorCode code = acquired.error().code();
                ended = code == AcqErrorCode::EndOfStream || code == AcqErrorCode::DeviceStopped;
                break;
            }
            AcqBlockView& acquiredView = *acquired;
            if (acquiredView.sampleCount() == 0) {
                static_cast<void>(reader->release(acquiredView));
                break;
            }
            block.resize(channels, static_cast<Eigen::Index>(acquiredView.sampleCount()));
            const AcqError decoded = decode(acquiredView, ref(block));
            for (const auto* part : {&acquiredView.events().first, &acquiredView.events().second}) {
                events.insert(events.end(), part->begin(), part->end());
            }
            const std::uint64_t first = acquiredView.firstSample();
            if (reader->release(acquiredView).ok() && decoded.ok()) {
                for (Eigen::Index i = 0; i < block.cols(); ++i) {
                    sweep.col(static_cast<Eigen::Index>((first + static_cast<std::uint64_t>(i)) % static_cast<std::uint64_t>(width))) = block.col(i);
                }
                next = first + static_cast<std::uint64_t>(block.cols());
                added = true;
            }
        }
        if (added) {
            const auto bar = static_cast<int>(next % static_cast<std::uint64_t>(width));
            // Events older than one sweep have been overwritten.
            std::erase_if(events, [&](const AcqEvent& e) { return e.sample + static_cast<std::uint64_t>(width) < next; });
            std::vector<ChannelRhiView::EventMarker> markers;
            markers.reserve(events.size() + 1);
            for (const AcqEvent& e : events) {
                markers.push_back({static_cast<int>(e.sample % static_cast<std::uint64_t>(width)), e.code, QColor(), std::to_string(e.code)});
            }
            markers.push_back({bar, 0, QColor(0, 110, 230), "now"});
            // The time ruler lays out the chips from left to right.
            std::sort(markers.begin(), markers.end(), [](const auto& a, const auto& b) { return a.sample < b.sample; });
            view.setData(sweep, 0);
            view.setEvents(markers);
        }
        const bool timeUp = duration > 0.0 && elapsed.elapsed() >= static_cast<qint64>(duration * 1000.0);
        if (ended || timeUp || !view.isVisible()) {
            timer.stop();
            shotSaved = screenshot.isEmpty() || view.grab().save(screenshot);
            QCoreApplication::quit();
        }
    });
    timer.start(33);
    QApplication::exec();
    device.close();

    if (!shotSaved) {
        err << "mne_acq: cannot write " << screenshot << "\n";
        return 2;
    }
    QTextStream(stdout) << "Showed " << next << " samples\n";
    return 0;
}
