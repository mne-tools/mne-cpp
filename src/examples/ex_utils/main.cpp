//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Utilities library: buffers, observers, logging, file formats, montages, layouts and reports.
 *
 * Every step checks its result (round trips, known electrode positions, a
 * hand-written digitiser record) and the example exits non-zero on a mismatch.
 * All files are written to a temporary directory.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/circularbuffer.h>
#include <utils/generics/commandpattern.h>
#include <utils/generics/mne_logger.h>
#include <utils/generics/observerpattern.h>
#include <utils/ioutils.h>
#include <utils/layoutloader.h>
#include <utils/layoutmaker.h>
#include <utils/montage/standard_montage.h>
#include <utils/polhemus/fastrak_parser.h>
#include <utils/report.h>
#include <utils/selectionio.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QTemporaryDir>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <cstdlib>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
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

//=============================================================================================================

//! [observer_pattern_usage]
// A subject notifies every attached observer; observers pull the state they need
class Counter : public Subject
{
public:
    void increment()
    {
        ++m_value;
        notify();
    }
    int value() const
    {
        return m_value;
    }

private:
    int m_value = 0;
};

class LastSeen : public IObserver
{
public:
    void update(Subject* pSubject) override
    {
        m_seen = static_cast<Counter*>(pSubject)->value();
    }
    int seen() const
    {
        return m_seen;
    }

private:
    int m_seen = -1;
};
//! [observer_pattern_usage]

//! [command_pattern_usage]
// A command wraps an action so it can be queued or replayed later
class AppendCommand : public ICommand
{
public:
    AppendCommand(QStringList& target, QString text)
    : m_target(target)
    , m_text(std::move(text))
    {
    }
    void execute() override
    {
        m_target.append(m_text);
    }

private:
    QStringList& m_target;
    QString m_text;
};
//! [command_pattern_usage]

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QTemporaryDir dir;
    bool ok = true;

    //! [mne_logger_usage]
    qInstallMessageHandler(MNELogger::customLogWriter); // coloured console output
    MNELogger::setLogFile(dir.filePath("example.log")); // and a plain-text copy
    qInfo() << "logging to" << MNELogger::logFile();
    //! [mne_logger_usage]
    MNELogger::setLogFile(QString());
    QFile log(dir.filePath("example.log"));
    ok &= expect(log.open(QIODevice::ReadOnly) && log.readAll().contains("logging to"), "MNELogger wrote the message to the log file");

    //! [circular_buffer_usage]
    CircularBuffer<double> buffer(4); // single producer, single consumer
    const double block[3] = {1.0, 2.0, 3.0};
    buffer.push(block, 3);
    double first = 0.0;
    double second = 0.0;
    buffer.pop(first);
    buffer.pop(second);
    //! [circular_buffer_usage]
    ok &= expect(first == 1.0 && second == 2.0 && buffer.getFreeElementsRead() == 1, "CircularBuffer pops in FIFO order, one element left");

    Counter counter;
    LastSeen observer;
    counter.attach(&observer);
    counter.increment();
    counter.increment();
    counter.detach(&observer);
    counter.increment();
    ok &= expect(observer.seen() == 2, QString("observer saw %1 before detaching (2)").arg(observer.seen()));

    QStringList journal;
    QList<AppendCommand*> queue = {new AppendCommand(journal, "a"), new AppendCommand(journal, "b")};
    for (ICommand* command : queue) {
        command->execute();
    }
    qDeleteAll(queue);
    ok &= expect(journal == QStringList({"a", "b"}), "commands executed in order");

    //! [ioutils_usage]
    MatrixXd matrix(2, 3);
    matrix << 1.5, -2.0, 3.25, 0.0, 1e-6, 42.0;
    const QString matrixPath = dir.filePath("matrix.txt");
    IOUtils::write_eigen_matrix(matrix, matrixPath, "example matrix");
    MatrixXd readBack;
    IOUtils::read_eigen_matrix(readBack, matrixPath);
    //! [ioutils_usage]
    ok &= expect(readBack.rows() == 2 && readBack.cols() == 3 && (readBack - matrix).cwiseAbs().maxCoeff() < 1e-12,
                 "IOUtils text matrix round trip");

    //! [selection_io_usage]
    QMultiMap<QString, QStringList> selections;
    selections.insert("Left-temporal", {"MEG0111", "MEG0112", "MEG0113"});
    selections.insert("Vertex", {"MEG0711"});
    const QString selPath = dir.filePath("groups.sel");
    SelectionIO::writeMNESelFile(selPath, selections);
    QMultiMap<QString, QStringList> reread;
    SelectionIO::readMNESelFile(selPath, reread);
    //! [selection_io_usage]
    ok &= expect(reread.size() == 2 && reread.value("Left-temporal") == QStringList({"MEG0111", "MEG0112", "MEG0113"}),
                 "SelectionIO .sel round trip");

    //! [standard_montage_usage]
    const QList<ElectrodePosition> montage1020 = StandardMontage::getMontage(StandardMontage::System::Standard_1020);
    Vector3d cz;
    const bool found = StandardMontage::findElectrode("Cz", cz); // positions in metres, head coordinates
    //! [standard_montage_usage]
    ok &= expect(montage1020.size() == StandardMontage::electrodeCount(StandardMontage::System::Standard_1020) && found && (cz - Vector3d(0.0, 0.0, 0.087)).norm() < 1e-9,
                 QString("10-20 montage: %1 electrodes, Cz at (0, 0, 0.087) m").arg(montage1020.size()));

    //! [layout_maker_usage]
    // Azimuthal equidistant projection around the fitted sphere: Cz lands near the centre, the nose points up
    QList<QVector<float>> points3d;
    QStringList names;
    for (const ElectrodePosition& electrode : montage1020) {
        points3d.append({static_cast<float>(electrode.pos.x()), static_cast<float>(electrode.pos.y()), static_cast<float>(electrode.pos.z())});
        names.append(electrode.name);
    }
    QList<QVector<float>> layout2d;
    QFile layoutFile(dir.filePath("1020.lout"));
    LayoutMaker::makeLayout(points3d, layout2d, names, layoutFile, true, 20.0f, 5.0f, 4.0f, true);
    //! [layout_maker_usage]
    const int iCz = names.indexOf("Cz");
    const int iFp1 = names.indexOf("Fp1");
    const int iO1 = names.indexOf("O1");
    const int iT8 = names.indexOf("T8");
    // Box corners: the box of an electrode at (x, y) starts at (x - w/2, y - h/2)
    const float czX = layout2d[iCz][0] + 2.5f;
    const float czY = layout2d[iCz][1] + 2.0f;
    ok &= expect(layout2d.size() == names.size() && std::hypot(czX, czY) < 1.0f && layout2d[iFp1][1] > layout2d[iCz][1] && layout2d[iO1][1] < layout2d[iCz][1] && layout2d[iFp1][0] < layout2d[iCz][0] && layout2d[iT8][0] > layout2d[iCz][0],
                 QString("LayoutMaker: Cz at (%1, %2), Fp1 front-left, O1 back, T8 right").arg(czX).arg(czY));

    //! [layout_loader_usage]
    QMap<QString, QPointF> channelPositions;
    LayoutLoader::readMNELoutFile(layoutFile.fileName(), channelPositions);
    //! [layout_loader_usage]
    ok &= expect(channelPositions.size() == names.size(), QString("LayoutLoader reads %1 channels back").arg(channelPositions.size()));

    //! [fastrak_parser_usage]
    FastrakParser parser;
    parser.setUnits(FastrakParser::Units::Centimetres);
    parser.append("01   12.50  -3.25   8.00\r\n02  1.0"); // the second record is still incomplete
    FastrakSample sample;
    const bool complete = parser.nextSample(sample);
    const bool pending = parser.nextSample(sample);
    //! [fastrak_parser_usage]
    ok &= expect(complete && !pending && sample.station == 1 && std::fabs(sample.position.x() - 0.125f) < 1e-6f && std::fabs(sample.position.y() + 0.0325f) < 1e-6f,
                 "FastrakParser converts centimetres to metres and waits for the partial record");

    //! [report_usage]
    Report report("Example report");
    report.addText("Summary", "All utilities were exercised.");
    report.addTable("Montage", {"Electrode", "z (m)"}, {{"Cz", "0.087"}, {"Fp1", "0.033"}});
    const QString html = report.toHtml();
    report.save(dir.filePath("report.html"));
    //! [report_usage]
    ok &= expect(report.sectionCount() == 2 && html.contains("<table") && html.contains("Fp1") && QFile::exists(dir.filePath("report.html")),
                 "Report renders text and table sections to HTML");

    qInfo() << (ok ? "All utils checks passed." : "utils checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
