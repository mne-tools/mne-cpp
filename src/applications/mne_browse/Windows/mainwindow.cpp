//=============================================================================================================
/**
 * @file     mainwindow.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  2.1.0
 * @date     January, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Lorenz Esch. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    mne_browse is the QT equivalent of the already existing C-version of mne_browse_raw. It is pursued
 *           to reimplement the full feature set of mne_browse_raw and even extend these.
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mainwindow.h"

#include <fiff/fiff_stream.h>

#include <QBuffer>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QInputDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QSet>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QFileInfo>

#include <cmath>
#include <algorithm>
#include <functional>

#include <inv/minimum_norm/inv_minimum_norm.h>
#include <inv/inv_source_estimate.h>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBROWSE;

#ifdef WASMBUILD
static QByteArray s_wasmByteArray;
#endif

namespace
{

QString defaultEvokedFilePath(const QString& rawFilePath)
{
    if(rawFilePath.isEmpty()) {
        return QString();
    }

    if(rawFilePath.endsWith(".fif", Qt::CaseInsensitive)) {
        return rawFilePath.left(rawFilePath.size() - 4) + "-ave.fif";
    }

    return rawFilePath + "-ave.fif";
}

QString defaultCovFilePath(const QString& rawFilePath)
{
    if(rawFilePath.isEmpty()) {
        return QString();
    }

    if(rawFilePath.endsWith(".fif", Qt::CaseInsensitive)) {
        return rawFilePath.left(rawFilePath.size() - 4) + "-cov.fif";
    }

    return rawFilePath + "-cov.fif";
}

QString defaultSourceEstimateDirectory(const QSettings& settings,
                                       const QString& rawFilePath,
                                       const QString& evokedFilePath,
                                       const QString& inverseOperatorPath)
{
    const QString savedDirectory =
        settings.value("MainWindow/SourceEstimate/outputDirectory").toString().trimmed();
    if(!savedDirectory.isEmpty() && QDir(savedDirectory).exists()) {
        return savedDirectory;
    }

    const QStringList candidatePaths = {
        evokedFilePath,
        rawFilePath,
        inverseOperatorPath
    };

    for(const QString& candidatePath : candidatePaths) {
        if(candidatePath.isEmpty()) {
            continue;
        }

        const QString absolutePath = QFileInfo(candidatePath).absolutePath();
        if(QDir(absolutePath).exists()) {
            return absolutePath;
        }
    }

    return QDir::homePath();
}

QString defaultAnnotationFilePath(const QString& rawFilePath)
{
    if(rawFilePath.isEmpty()) {
        return QString();
    }

    if(rawFilePath.endsWith(".fif", Qt::CaseInsensitive)) {
        return rawFilePath.left(rawFilePath.size() - 4) + "-annot.json";
    }

    return rawFilePath + "-annot.json";
}

QStringList defaultAnnotationCandidatePaths(const QString& rawFilePath)
{
    const QString defaultJsonPath = defaultAnnotationFilePath(rawFilePath);
    if(defaultJsonPath.isEmpty()) {
        return {};
    }

    QString basePath = defaultJsonPath;
    if(basePath.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive)) {
        basePath.chop(5);
    }

    QStringList candidates = {defaultJsonPath,
                              basePath + QStringLiteral(".fif")};

    if(rawFilePath.endsWith(QStringLiteral(".fif"), Qt::CaseInsensitive)) {
        candidates.append(rawFilePath.left(rawFilePath.size() - 4) + QStringLiteral("_annot.fif"));
    }

    candidates.append(basePath + QStringLiteral(".csv"));
    candidates.append(basePath + QStringLiteral(".txt"));
    return candidates;
}

QString defaultVirtualChannelFilePath(const QString& rawFilePath)
{
    if(rawFilePath.isEmpty()) {
        return QString();
    }

    if(rawFilePath.endsWith(".fif", Qt::CaseInsensitive)) {
        return rawFilePath.left(rawFilePath.size() - 4) + "-virtchan.json";
    }

    return rawFilePath + "-virtchan.json";
}

void applySessionFilterToEpochList(MNELIB::MNEEpochDataList& epochList,
                                   const MNEBROWSE::SessionFilter& filter,
                                   const FIFFLIB::FiffInfo& info,
                                   const QMap<QString,double>& rejectMap)
{
    for(const auto& epoch : epochList) {
        if(epoch.isNull()) {
            continue;
        }

        epoch->epoch = filter.applyToMatrix(epoch->epoch, info);
        if(!rejectMap.isEmpty()) {
            epoch->bReject = MNELIB::MNEEpochDataList::checkForArtifact(epoch->epoch,
                                                                        info,
                                                                        rejectMap);
        }
    }
}

FIFFLIB::FiffCov computeCovarianceFromEpochLists(const QList<MNELIB::MNEEpochDataList>& epochLists,
                                                 const FIFFLIB::FiffInfo& info,
                                                 bool removeMean)
{
    FIFFLIB::FiffCov covariance;
    if(epochLists.isEmpty()) {
        return covariance;
    }

    Eigen::MatrixXd covAccum = Eigen::MatrixXd::Zero(info.nchan, info.nchan);
    Eigen::VectorXd meanAccum = Eigen::VectorXd::Zero(info.nchan);
    int totalSamples = 0;
    int acceptedEpochs = 0;

    for(const MNELIB::MNEEpochDataList& epochList : epochLists) {
        for(const auto& epoch : epochList) {
            if(epoch.isNull() || epoch->epoch.size() == 0) {
                continue;
            }

            covAccum += epoch->epoch * epoch->epoch.transpose();
            if(removeMean) {
                meanAccum += epoch->epoch.rowwise().mean() * static_cast<double>(epoch->epoch.cols());
            }

            totalSamples += static_cast<int>(epoch->epoch.cols());
            ++acceptedEpochs;
        }
    }

    if(totalSamples < 2) {
        return covariance;
    }

    if(removeMean) {
        const Eigen::VectorXd grandMean = meanAccum / static_cast<double>(totalSamples);
        covariance.data = (covAccum / static_cast<double>(totalSamples - 1))
                          - (grandMean * grandMean.transpose())
                                * (static_cast<double>(totalSamples) / static_cast<double>(totalSamples - 1));
    } else {
        covariance.data = covAccum / static_cast<double>(totalSamples - 1);
    }

    covariance.kind = FIFFV_MNE_NOISE_COV;
    covariance.dim = info.nchan;
    covariance.names = info.ch_names;
    covariance.nfree = totalSamples - 1;
    covariance.bads = info.bads;
    covariance.projs = info.projs;

    qInfo() << "[MainWindow] Computed filtered covariance:" << info.nchan
            << "channels," << acceptedEpochs << "epochs," << totalSamples << "total samples.";

    return covariance;
}

bool writeFilteredRawFile(const QString& rawFilePath,
                          QIODevice* pIODevice,
                          const MNEBROWSE::SessionFilter& filter)
{
    QFile rawFile(rawFilePath);
    FIFFLIB::FiffRawData raw(rawFile);
    if(raw.isEmpty()) {
        qWarning() << "[MainWindow] Could not reopen raw data for filtered write" << rawFilePath;
        return false;
    }

    Eigen::RowVectorXd cals;
    Eigen::SparseMatrix<double> mult;
    Eigen::RowVectorXi sel;

    FIFFLIB::FiffStream::SPtr outfid = FIFFLIB::FiffStream::start_writing_raw(*pIODevice, raw.info, cals);
    if(!outfid) {
        qWarning() << "[MainWindow] Could not open output stream for filtered write.";
        return false;
    }

    const int padSamples = filter.recommendedPaddingSamples();
    const FIFFLIB::fiff_int_t from = raw.first_samp;
    const FIFFLIB::fiff_int_t to = raw.last_samp;
    const FIFFLIB::fiff_int_t quantum = static_cast<FIFFLIB::fiff_int_t>(std::ceil(10.0 * raw.info.sfreq));

    bool firstBuffer = true;
    for(FIFFLIB::fiff_int_t first = from; first <= to; first += quantum) {
        const FIFFLIB::fiff_int_t last = std::min(first + quantum - 1, to);
        const FIFFLIB::fiff_int_t paddedFirst = std::max(from, first - padSamples);
        const FIFFLIB::fiff_int_t paddedLast = std::min(to, last + padSamples);

        Eigen::MatrixXd data;
        Eigen::MatrixXd times;
        if(!raw.read_raw_segment(data, times, mult, paddedFirst, paddedLast, sel)) {
            qWarning() << "[MainWindow] Error during filtered read_raw_segment.";
            return false;
        }

        const Eigen::MatrixXd filteredData = filter.applyToMatrix(data, raw.info);
        const int cropStart = static_cast<int>(first - paddedFirst);
        const int cropLength = static_cast<int>(last - first + 1);
        const Eigen::MatrixXd croppedData = filteredData.block(0,
                                                               cropStart,
                                                               filteredData.rows(),
                                                               cropLength);

        if(firstBuffer) {
            if(first > 0) {
                outfid->write_int(FIFF_FIRST_SAMPLE, &first);
            }
            firstBuffer = false;
        }

        outfid->write_raw_buffer(croppedData, mult);
    }

    outfid->finish_writing_raw();
    return true;
}

QMap<int, int> countEventsByType(const MatrixXi& events)
{
    QMap<int, int> counts;

    if(events.cols() < 3) {
        return counts;
    }

    for(int row = 0; row < events.rows(); ++row) {
        const int eventCode = events(row, 2);
        if(eventCode != 0) {
            counts[eventCode] = counts.value(eventCode) + 1;
        }
    }

    return counts;
}

QString normalizeStimChannelName(const QString& channelName)
{
    QString normalizedName = channelName.trimmed().toUpper();
    normalizedName.remove(QLatin1Char(' '));
    return normalizedName;
}

bool detectFallbackStimEvents(const FIFFLIB::FiffRawData& raw,
                              MatrixXi& events,
                              QString& sourceDescription)
{
    QStringList preferredStimChannels;
    QStringList fallbackStimChannels;

    for(int channelIndex = 0; channelIndex < raw.info.nchan; ++channelIndex) {
        if(raw.info.chs[channelIndex].kind != FIFFV_STIM_CH) {
            continue;
        }

        const QString stimChannel = raw.info.ch_names.value(channelIndex);
        const QString normalizedName = normalizeStimChannelName(stimChannel);

        if(normalizedName == QLatin1String("STI014") ||
           normalizedName == QLatin1String("STI101")) {
            preferredStimChannels.append(stimChannel);
        } else {
            fallbackStimChannels.append(stimChannel);
        }
    }

    const QStringList stimChannels = preferredStimChannels + fallbackStimChannels;
    for(const QString& stimChannel : stimChannels) {
        FIFFLIB::FiffEvents detectedEvents;
        if(FIFFLIB::FiffEvents::detect_from_raw(raw, detectedEvents, stimChannel)) {
            events = detectedEvents.events;
            sourceDescription = QString("Using trigger events detected from %1.").arg(stimChannel);
            return true;
        }
    }

    return false;
}

bool populateEventModelFromRaw(const QString& rawFilePath,
                               MNEBROWSE::EventModel* eventModel,
                               QString& sourceDescription)
{
    if(!eventModel || rawFilePath.isEmpty() || !QFile::exists(rawFilePath)) {
        return false;
    }

    QFile rawFile(rawFilePath);
    FIFFLIB::FiffRawData raw(rawFile);
    if(raw.isEmpty()) {
        return false;
    }

    MatrixXi events;
    if(!detectFallbackStimEvents(raw, events, sourceDescription) || events.rows() == 0) {
        return false;
    }

    eventModel->setEventMatrix(events, false);
    return true;
}

QStringList evokedEventCodesToSettingValue(const QList<int>& selectedEventCodes)
{
    QStringList codes;
    codes.reserve(selectedEventCodes.size());

    for(int code : selectedEventCodes) {
        codes << QString::number(code);
    }

    return codes;
}

QList<int> evokedEventCodesFromSettings(const QSettings& settings)
{
    QList<int> selectedEventCodes;
    const QStringList savedCodes =
        settings.value("MainWindow/Averaging/eventCodes").toStringList();

    for(const QString& codeText : savedCodes) {
        bool ok = false;
        const int code = codeText.toInt(&ok);
        if(ok) {
            selectedEventCodes.append(code);
        }
    }

    return selectedEventCodes;
}

QString sanitizeFileToken(const QString& text, const QString& fallback = QStringLiteral("item"))
{
    QString sanitized;
    sanitized.reserve(text.size());

    bool lastWasSeparator = false;
    for(const QChar& ch : text.trimmed()) {
        if(ch.isLetterOrNumber()) {
            sanitized.append(ch.toLower());
            lastWasSeparator = false;
        } else if((ch == QLatin1Char('_') || ch == QLatin1Char('-') || ch == QLatin1Char('.'))
                  && !lastWasSeparator) {
            sanitized.append(ch);
            lastWasSeparator = true;
        } else if(!lastWasSeparator) {
            sanitized.append(QLatin1Char('_'));
            lastWasSeparator = true;
        }
    }

    while(sanitized.startsWith(QLatin1Char('_')) || sanitized.startsWith(QLatin1Char('-'))) {
        sanitized.remove(0, 1);
    }
    while(sanitized.endsWith(QLatin1Char('_')) || sanitized.endsWith(QLatin1Char('-'))) {
        sanitized.chop(1);
    }

    if(sanitized.isEmpty()) {
        return fallback;
    }

    return sanitized;
}

QString defaultSourceEstimateStem(const QString& rawFilePath,
                                  const QString& evokedFilePath,
                                  const QString& inverseOperatorPath)
{
    QString stem;

    if(!evokedFilePath.isEmpty()) {
        stem = QFileInfo(evokedFilePath).completeBaseName();
    } else if(!rawFilePath.isEmpty()) {
        stem = QFileInfo(rawFilePath).completeBaseName();
    } else if(!inverseOperatorPath.isEmpty()) {
        stem = QFileInfo(inverseOperatorPath).completeBaseName();
    }

    const QStringList removableSuffixes = {
        QStringLiteral("-ave"),
        QStringLiteral("_ave"),
        QStringLiteral("-raw"),
        QStringLiteral("_raw"),
        QStringLiteral("-inv"),
        QStringLiteral("_inv")
    };

    for(const QString& suffix : removableSuffixes) {
        if(stem.endsWith(suffix, Qt::CaseInsensitive)) {
            stem.chop(suffix.size());
            break;
        }
    }

    return sanitizeFileToken(stem, QStringLiteral("source_estimate"));
}

QString uniqueStem(const QString& stem, QSet<QString>& usedStems)
{
    if(!usedStems.contains(stem)) {
        usedStems.insert(stem);
        return stem;
    }

    int suffix = 2;
    QString candidateStem;
    do {
        candidateStem = QStringLiteral("%1_%2").arg(stem).arg(suffix);
        ++suffix;
    } while(usedStems.contains(candidateStem));

    usedStems.insert(candidateStem);
    return candidateStem;
}

INVLIB::InvEstimateMethod estimateMethodFromString(const QString& method)
{
    if(method.compare(QStringLiteral("MNE"), Qt::CaseInsensitive) == 0) {
        return INVLIB::InvEstimateMethod::MNE;
    }
    if(method.compare(QStringLiteral("sLORETA"), Qt::CaseInsensitive) == 0) {
        return INVLIB::InvEstimateMethod::sLORETA;
    }
    if(method.compare(QStringLiteral("eLORETA"), Qt::CaseInsensitive) == 0) {
        return INVLIB::InvEstimateMethod::eLORETA;
    }
    return INVLIB::InvEstimateMethod::dSPM;
}

INVLIB::InvOrientationType orientationTypeFromInverse(const MNELIB::MNEInverseOperator& inverseOperator)
{
    if(inverseOperator.source_ori == FIFFV_MNE_FREE_ORI) {
        return INVLIB::InvOrientationType::Free;
    }
    if(inverseOperator.source_ori == FIFFV_MNE_FIXED_ORI) {
        return INVLIB::InvOrientationType::Fixed;
    }
    return INVLIB::InvOrientationType::Unknown;
}

QString summarizeSelectedEvokeds(const QStringList& comments)
{
    if(comments.isEmpty()) {
        return QStringLiteral("No evoked sets selected.");
    }

    QStringList displayComments = comments;
    for(QString& comment : displayComments) {
        if(comment.trimmed().isEmpty()) {
            comment = QStringLiteral("<unnamed>");
        }
    }

    const int previewCount = std::min(3, static_cast<int>(displayComments.size()));
    const QString preview = displayComments.mid(0, previewCount).join(QStringLiteral(", "));
    if(displayComments.size() > previewCount) {
        return QStringLiteral("%1 selected (%2, ...)")
            .arg(displayComments.size())
            .arg(preview);
    }

    return QStringLiteral("%1 selected (%2)")
        .arg(displayComments.size())
        .arg(preview);
}

int sourceEstimateSplitIndex(const INVLIB::InvSourceEstimate& sourceEstimate,
                             const MNELIB::MNEInverseOperator& inverseOperator)
{
    if(inverseOperator.src.size() >= 2) {
        int leftSourceCount = 0;
        if(inverseOperator.src[0].vertno.size() > 0) {
            leftSourceCount = inverseOperator.src[0].vertno.size();
        } else if(inverseOperator.src[0].nuse > 0) {
            leftSourceCount = inverseOperator.src[0].nuse;
        }

        if(leftSourceCount > 0
           && leftSourceCount < sourceEstimate.data.rows()
           && leftSourceCount < sourceEstimate.vertices.size()) {
            return leftSourceCount;
        }
    }

    for(int index = 1; index < sourceEstimate.vertices.size(); ++index) {
        if(sourceEstimate.vertices(index) < sourceEstimate.vertices(index - 1)) {
            return index;
        }
    }

    return -1;
}

bool splitSourceEstimateByHemisphere(const INVLIB::InvSourceEstimate& sourceEstimate,
                                     const MNELIB::MNEInverseOperator& inverseOperator,
                                     INVLIB::InvSourceEstimate& leftHemisphere,
                                     INVLIB::InvSourceEstimate& rightHemisphere)
{
    const int splitIndex = sourceEstimateSplitIndex(sourceEstimate, inverseOperator);
    if(splitIndex <= 0
       || splitIndex >= sourceEstimate.data.rows()
       || splitIndex >= sourceEstimate.vertices.size()) {
        return false;
    }

    leftHemisphere = sourceEstimate;
    rightHemisphere = sourceEstimate;

    leftHemisphere.data = sourceEstimate.data.topRows(splitIndex);
    leftHemisphere.vertices = sourceEstimate.vertices.head(splitIndex);

    rightHemisphere.data = sourceEstimate.data.bottomRows(sourceEstimate.data.rows() - splitIndex);
    rightHemisphere.vertices = sourceEstimate.vertices.tail(sourceEstimate.vertices.size() - splitIndex);

    return true;
}

WhiteningSettings whiteningSettingsFromSettings(const QSettings& settings)
{
    WhiteningSettings whiteningSettings;
    whiteningSettings.regMag = settings.value("MainWindow/Covariance/whitenRegMag", whiteningSettings.regMag).toDouble();
    whiteningSettings.regGrad = settings.value("MainWindow/Covariance/whitenRegGrad", whiteningSettings.regGrad).toDouble();
    whiteningSettings.regEeg = settings.value("MainWindow/Covariance/whitenRegEeg", whiteningSettings.regEeg).toDouble();
    whiteningSettings.useProj = settings.value("MainWindow/Covariance/whitenUseProj", whiteningSettings.useProj).toBool();
    return whiteningSettings;
}

void saveWhiteningSettings(QSettings& settings, const WhiteningSettings& whiteningSettings)
{
    settings.setValue("MainWindow/Covariance/whitenRegMag", whiteningSettings.regMag);
    settings.setValue("MainWindow/Covariance/whitenRegGrad", whiteningSettings.regGrad);
    settings.setValue("MainWindow/Covariance/whitenRegEeg", whiteningSettings.regEeg);
    settings.setValue("MainWindow/Covariance/whitenUseProj", whiteningSettings.useProj);
}

Eigen::VectorXi reviewedEpochSelection(const MNELIB::MNEEpochDataList& epochList,
                                       bool respectAutoRejects)
{
    QList<int> selectedIndices;
    selectedIndices.reserve(epochList.size());

    for(int index = 0; index < epochList.size(); ++index) {
        const auto& epoch = epochList.at(index);
        if(epoch && !epoch->isRejected(respectAutoRejects)) {
            selectedIndices.append(index);
        }
    }

    Eigen::VectorXi selection(selectedIndices.size());
    for(int index = 0; index < selectedIndices.size(); ++index) {
        selection(index) = selectedIndices.at(index);
    }

    return selection;
}

bool showComputeEvokedDialog(QWidget* parent,
                             QSettings& settings,
                             const QMap<int, int>& eventCounts,
                             const QString& sourceDescription,
                             QList<int>& selectedEventCodes,
                             float& tmin,
                             float& tmax,
                             bool& applyBaseline,
                             float& baselineFrom,
                             float& baselineTo,
                             bool& dropRejected)
{
    QDialog dialog(parent);
    dialog.setWindowTitle("Compute Evoked");

    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);

    QLabel* sourceLabel = new QLabel(sourceDescription, &dialog);
    sourceLabel->setWordWrap(true);
    mainLayout->addWidget(sourceLabel);

    QGroupBox* timingGroup = new QGroupBox("Epoch Settings", &dialog);
    QFormLayout* timingLayout = new QFormLayout(timingGroup);

    QSpinBox* preStimSpinBox = new QSpinBox(timingGroup);
    preStimSpinBox->setRange(0, 10000);
    preStimSpinBox->setSingleStep(10);
    preStimSpinBox->setPrefix("-");
    preStimSpinBox->setValue(settings.value("MainWindow/Averaging/preStimMs", 100).toInt());

    QSpinBox* postStimSpinBox = new QSpinBox(timingGroup);
    postStimSpinBox->setRange(10, 10000);
    postStimSpinBox->setSingleStep(10);
    postStimSpinBox->setValue(settings.value("MainWindow/Averaging/postStimMs", 400).toInt());

    QCheckBox* baselineCheckBox = new QCheckBox("Apply baseline correction", timingGroup);
    baselineCheckBox->setChecked(settings.value("MainWindow/Averaging/applyBaseline", true).toBool());

    QSpinBox* baselineFromSpinBox = new QSpinBox(timingGroup);
    baselineFromSpinBox->setSingleStep(10);
    baselineFromSpinBox->setRange(-preStimSpinBox->value(), postStimSpinBox->value());
    baselineFromSpinBox->setValue(settings.value("MainWindow/Averaging/baselineFromMs",
                                                 -preStimSpinBox->value()).toInt());

    QSpinBox* baselineToSpinBox = new QSpinBox(timingGroup);
    baselineToSpinBox->setSingleStep(10);
    baselineToSpinBox->setRange(-preStimSpinBox->value(), postStimSpinBox->value());
    baselineToSpinBox->setValue(settings.value("MainWindow/Averaging/baselineToMs", 0).toInt());

    QCheckBox* rejectCheckBox = new QCheckBox("Drop rejected epochs", timingGroup);
    rejectCheckBox->setChecked(settings.value("MainWindow/Averaging/dropRejected", true).toBool());

    timingLayout->addRow("Pre-stimulus (ms)", preStimSpinBox);
    timingLayout->addRow("Post-stimulus (ms)", postStimSpinBox);
    timingLayout->addRow(baselineCheckBox);
    timingLayout->addRow("Baseline from (ms)", baselineFromSpinBox);
    timingLayout->addRow("Baseline to (ms)", baselineToSpinBox);
    timingLayout->addRow(rejectCheckBox);
    mainLayout->addWidget(timingGroup);

    auto syncSpinBoxBounds = [=]() {
        const int minValue = -preStimSpinBox->value();
        const int maxValue = postStimSpinBox->value();

        baselineFromSpinBox->setRange(minValue, maxValue);
        baselineToSpinBox->setRange(minValue, maxValue);

        if(baselineFromSpinBox->value() > baselineToSpinBox->value()) {
            baselineFromSpinBox->setValue(minValue);
            baselineToSpinBox->setValue(0);
        }
    };

    syncSpinBoxBounds();

    QObject::connect(preStimSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                     &dialog, [=](int) { syncSpinBoxBounds(); });
    QObject::connect(postStimSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                     &dialog, [=](int) { syncSpinBoxBounds(); });
    QObject::connect(baselineCheckBox, &QCheckBox::toggled,
                     baselineFromSpinBox, &QSpinBox::setEnabled);
    QObject::connect(baselineCheckBox, &QCheckBox::toggled,
                     baselineToSpinBox, &QSpinBox::setEnabled);
    baselineFromSpinBox->setEnabled(baselineCheckBox->isChecked());
    baselineToSpinBox->setEnabled(baselineCheckBox->isChecked());

    QGroupBox* eventGroup = new QGroupBox("Event Types", &dialog);
    QVBoxLayout* eventLayout = new QVBoxLayout(eventGroup);
    QLabel* eventHint = new QLabel("Select one or more event codes to average.", eventGroup);
    eventHint->setWordWrap(true);
    eventLayout->addWidget(eventHint);

    QListWidget* eventListWidget = new QListWidget(eventGroup);
    eventListWidget->setSelectionMode(QAbstractItemView::MultiSelection);

    for(auto it = eventCounts.constBegin(); it != eventCounts.constEnd(); ++it) {
        QListWidgetItem* item = new QListWidgetItem(QString("%1 (%2 epochs)").arg(it.key()).arg(it.value()),
                                                    eventListWidget);
        item->setData(Qt::UserRole, it.key());
        item->setSelected(true);
    }

    eventLayout->addWidget(eventListWidget);
    mainLayout->addWidget(eventGroup);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                       Qt::Horizontal,
                                                       &dialog);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    if(dialog.exec() != QDialog::Accepted) {
        return false;
    }

    selectedEventCodes.clear();
    for(int row = 0; row < eventListWidget->count(); ++row) {
        QListWidgetItem* item = eventListWidget->item(row);
        if(item->isSelected()) {
            selectedEventCodes.append(item->data(Qt::UserRole).toInt());
        }
    }

    if(selectedEventCodes.isEmpty()) {
        QMessageBox::warning(parent,
                             "Compute Evoked",
                             "Select at least one event type to compute an evoked response.");
        return false;
    }

    settings.setValue("MainWindow/Averaging/preStimMs", preStimSpinBox->value());
    settings.setValue("MainWindow/Averaging/postStimMs", postStimSpinBox->value());
    settings.setValue("MainWindow/Averaging/applyBaseline", baselineCheckBox->isChecked());
    settings.setValue("MainWindow/Averaging/baselineFromMs", baselineFromSpinBox->value());
    settings.setValue("MainWindow/Averaging/baselineToMs", baselineToSpinBox->value());
    settings.setValue("MainWindow/Averaging/dropRejected", rejectCheckBox->isChecked());
    settings.setValue("MainWindow/Averaging/eventCodes",
                      evokedEventCodesToSettingValue(selectedEventCodes));

    tmin = -static_cast<float>(preStimSpinBox->value()) / 1000.0f;
    tmax = static_cast<float>(postStimSpinBox->value()) / 1000.0f;
    applyBaseline = baselineCheckBox->isChecked();
    baselineFrom = static_cast<float>(baselineFromSpinBox->value()) / 1000.0f;
    baselineTo = static_cast<float>(baselineToSpinBox->value()) / 1000.0f;
    dropRejected = rejectCheckBox->isChecked();

    return true;
}

bool showComputeCovarianceDialog(QWidget* parent,
                                 QSettings& settings,
                                 const QMap<int, int>& eventCounts,
                                 const QString& sourceDescription,
                                 QList<int>& selectedEventCodes,
                                 float& tmin,
                                 float& tmax,
                                 bool& applyBaseline,
                                 float& baselineFrom,
                                 float& baselineTo,
                                 bool& removeMean)
{
    QDialog dialog(parent);
    dialog.setWindowTitle("Compute Covariance");

    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);

    QLabel* sourceLabel = new QLabel(sourceDescription, &dialog);
    sourceLabel->setWordWrap(true);
    mainLayout->addWidget(sourceLabel);

    QGroupBox* timingGroup = new QGroupBox("Covariance Window", &dialog);
    QFormLayout* timingLayout = new QFormLayout(timingGroup);

    QSpinBox* fromSpinBox = new QSpinBox(timingGroup);
    fromSpinBox->setRange(-10000, 10000);
    fromSpinBox->setSingleStep(10);
    fromSpinBox->setValue(settings.value("MainWindow/Covariance/tminMs", -200).toInt());

    QSpinBox* toSpinBox = new QSpinBox(timingGroup);
    toSpinBox->setRange(-10000, 10000);
    toSpinBox->setSingleStep(10);
    toSpinBox->setValue(settings.value("MainWindow/Covariance/tmaxMs", 0).toInt());

    QCheckBox* baselineCheckBox = new QCheckBox("Apply baseline correction", timingGroup);
    baselineCheckBox->setChecked(settings.value("MainWindow/Covariance/applyBaseline", false).toBool());

    QSpinBox* baselineFromSpinBox = new QSpinBox(timingGroup);
    baselineFromSpinBox->setRange(-10000, 10000);
    baselineFromSpinBox->setSingleStep(10);
    baselineFromSpinBox->setValue(settings.value("MainWindow/Covariance/baselineFromMs", -200).toInt());

    QSpinBox* baselineToSpinBox = new QSpinBox(timingGroup);
    baselineToSpinBox->setRange(-10000, 10000);
    baselineToSpinBox->setSingleStep(10);
    baselineToSpinBox->setValue(settings.value("MainWindow/Covariance/baselineToMs", 0).toInt());

    QCheckBox* removeMeanCheckBox = new QCheckBox("Remove sample mean", timingGroup);
    removeMeanCheckBox->setChecked(settings.value("MainWindow/Covariance/removeMean", true).toBool());

    timingLayout->addRow("Window from (ms)", fromSpinBox);
    timingLayout->addRow("Window to (ms)", toSpinBox);
    timingLayout->addRow(baselineCheckBox);
    timingLayout->addRow("Baseline from (ms)", baselineFromSpinBox);
    timingLayout->addRow("Baseline to (ms)", baselineToSpinBox);
    timingLayout->addRow(removeMeanCheckBox);
    mainLayout->addWidget(timingGroup);

    auto syncRanges = [=]() {
        if(fromSpinBox->value() > toSpinBox->value()) {
            toSpinBox->setValue(fromSpinBox->value());
        }

        baselineFromSpinBox->setMinimum(fromSpinBox->value());
        baselineFromSpinBox->setMaximum(toSpinBox->value());
        baselineToSpinBox->setMinimum(fromSpinBox->value());
        baselineToSpinBox->setMaximum(toSpinBox->value());

        if(baselineFromSpinBox->value() > baselineToSpinBox->value()) {
            baselineFromSpinBox->setValue(fromSpinBox->value());
            baselineToSpinBox->setValue(toSpinBox->value());
        }
    };

    syncRanges();

    QObject::connect(fromSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                     &dialog, [=](int) { syncRanges(); });
    QObject::connect(toSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                     &dialog, [=](int) { syncRanges(); });
    QObject::connect(baselineCheckBox, &QCheckBox::toggled,
                     baselineFromSpinBox, &QSpinBox::setEnabled);
    QObject::connect(baselineCheckBox, &QCheckBox::toggled,
                     baselineToSpinBox, &QSpinBox::setEnabled);
    baselineFromSpinBox->setEnabled(baselineCheckBox->isChecked());
    baselineToSpinBox->setEnabled(baselineCheckBox->isChecked());

    QGroupBox* eventGroup = new QGroupBox("Event Types", &dialog);
    QVBoxLayout* eventLayout = new QVBoxLayout(eventGroup);
    QLabel* eventHint = new QLabel("Select one or more event codes whose epochs should contribute to the covariance estimate.",
                                   eventGroup);
    eventHint->setWordWrap(true);
    eventLayout->addWidget(eventHint);

    QListWidget* eventListWidget = new QListWidget(eventGroup);
    eventListWidget->setSelectionMode(QAbstractItemView::MultiSelection);

    for(auto it = eventCounts.constBegin(); it != eventCounts.constEnd(); ++it) {
        QListWidgetItem* item = new QListWidgetItem(QString("%1 (%2 epochs)").arg(it.key()).arg(it.value()),
                                                    eventListWidget);
        item->setData(Qt::UserRole, it.key());
        item->setSelected(true);
    }

    eventLayout->addWidget(eventListWidget);
    mainLayout->addWidget(eventGroup);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                       Qt::Horizontal,
                                                       &dialog);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    if(dialog.exec() != QDialog::Accepted) {
        return false;
    }

    selectedEventCodes.clear();
    for(int row = 0; row < eventListWidget->count(); ++row) {
        QListWidgetItem* item = eventListWidget->item(row);
        if(item->isSelected()) {
            selectedEventCodes.append(item->data(Qt::UserRole).toInt());
        }
    }

    if(selectedEventCodes.isEmpty()) {
        QMessageBox::warning(parent,
                             "Compute Covariance",
                             "Select at least one event type to compute a covariance matrix.");
        return false;
    }

    settings.setValue("MainWindow/Covariance/tminMs", fromSpinBox->value());
    settings.setValue("MainWindow/Covariance/tmaxMs", toSpinBox->value());
    settings.setValue("MainWindow/Covariance/applyBaseline", baselineCheckBox->isChecked());
    settings.setValue("MainWindow/Covariance/baselineFromMs", baselineFromSpinBox->value());
    settings.setValue("MainWindow/Covariance/baselineToMs", baselineToSpinBox->value());
    settings.setValue("MainWindow/Covariance/removeMean", removeMeanCheckBox->isChecked());

    tmin = static_cast<float>(fromSpinBox->value()) / 1000.0f;
    tmax = static_cast<float>(toSpinBox->value()) / 1000.0f;
    applyBaseline = baselineCheckBox->isChecked();
    baselineFrom = static_cast<float>(baselineFromSpinBox->value()) / 1000.0f;
    baselineTo = static_cast<float>(baselineToSpinBox->value()) / 1000.0f;
    removeMean = removeMeanCheckBox->isChecked();

    return true;
}

bool showComputeSourceEstimateDialog(QWidget* parent,
                                     QSettings& settings,
                                     const QString& inverseDescription,
                                     const QStringList& selectedEvokedComments,
                                     QString& method,
                                     double& snr,
                                     bool& pickNormal,
                                     QString& outputDirectory)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(QStringLiteral("Compute Source Estimate"));

    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);

    QLabel* inverseLabel = new QLabel(inverseDescription, &dialog);
    inverseLabel->setWordWrap(true);
    mainLayout->addWidget(inverseLabel);

    QLabel* evokedLabel = new QLabel(summarizeSelectedEvokeds(selectedEvokedComments), &dialog);
    evokedLabel->setWordWrap(true);
    mainLayout->addWidget(evokedLabel);

    QGroupBox* settingsGroup = new QGroupBox(QStringLiteral("Inverse Settings"), &dialog);
    QFormLayout* settingsLayout = new QFormLayout(settingsGroup);

    QComboBox* methodComboBox = new QComboBox(settingsGroup);
    methodComboBox->addItems({QStringLiteral("MNE"),
                              QStringLiteral("dSPM"),
                              QStringLiteral("sLORETA"),
                              QStringLiteral("eLORETA")});
    const QString savedMethod =
        settings.value("MainWindow/SourceEstimate/method", QStringLiteral("dSPM")).toString();
    const int methodIndex = methodComboBox->findText(savedMethod, Qt::MatchFixedString);
    methodComboBox->setCurrentIndex(methodIndex >= 0 ? methodIndex : 1);

    QDoubleSpinBox* snrSpinBox = new QDoubleSpinBox(settingsGroup);
    snrSpinBox->setRange(0.1, 100.0);
    snrSpinBox->setDecimals(2);
    snrSpinBox->setSingleStep(0.1);
    snrSpinBox->setValue(settings.value("MainWindow/SourceEstimate/snr", 3.0).toDouble());

    QCheckBox* pickNormalCheckBox =
        new QCheckBox(QStringLiteral("Pick normal component for free-orientation inverses"),
                      settingsGroup);
    pickNormalCheckBox->setChecked(settings.value("MainWindow/SourceEstimate/pickNormal", false).toBool());

    settingsLayout->addRow(QStringLiteral("Method"), methodComboBox);
    settingsLayout->addRow(QStringLiteral("SNR"), snrSpinBox);
    settingsLayout->addRow(pickNormalCheckBox);
    mainLayout->addWidget(settingsGroup);

    QGroupBox* outputGroup = new QGroupBox(QStringLiteral("Output"), &dialog);
    QVBoxLayout* outputLayout = new QVBoxLayout(outputGroup);

    QLabel* outputHint = new QLabel(QStringLiteral("Each selected evoked set is exported beside the chosen directory "
                                                   "using `<dataset>_<condition>_<method>-lh.stc` and `...-rh.stc` "
                                                   "when hemisphere splitting is available."),
                                    outputGroup);
    outputHint->setWordWrap(true);
    outputLayout->addWidget(outputHint);

    QHBoxLayout* directoryLayout = new QHBoxLayout();
    QLineEdit* outputDirectoryEdit = new QLineEdit(outputDirectory, outputGroup);
    QPushButton* browseButton = new QPushButton(QStringLiteral("Browse..."), outputGroup);
    directoryLayout->addWidget(outputDirectoryEdit, 1);
    directoryLayout->addWidget(browseButton);
    outputLayout->addLayout(directoryLayout);
    mainLayout->addWidget(outputGroup);

    QObject::connect(browseButton, &QPushButton::clicked, &dialog, [&dialog, outputDirectoryEdit]() {
        const QString directory = QFileDialog::getExistingDirectory(&dialog,
                                                                    QStringLiteral("Select output directory"),
                                                                    outputDirectoryEdit->text());
        if(!directory.isEmpty()) {
            outputDirectoryEdit->setText(directory);
        }
    });

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                       Qt::Horizontal,
                                                       &dialog);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    if(dialog.exec() != QDialog::Accepted) {
        return false;
    }

    outputDirectory = outputDirectoryEdit->text().trimmed();
    if(outputDirectory.isEmpty()) {
        QMessageBox::warning(parent,
                             QStringLiteral("Compute Source Estimate"),
                             QStringLiteral("Choose an output directory for the exported source estimates."));
        return false;
    }

    settings.setValue("MainWindow/SourceEstimate/method", methodComboBox->currentText());
    settings.setValue("MainWindow/SourceEstimate/snr", snrSpinBox->value());
    settings.setValue("MainWindow/SourceEstimate/pickNormal", pickNormalCheckBox->isChecked());
    settings.setValue("MainWindow/SourceEstimate/outputDirectory", outputDirectory);

    method = methodComboBox->currentText();
    snr = snrSpinBox->value();
    pickNormal = pickNormalCheckBox->isChecked();

    return true;
}

} // namespace


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MainWindow::MainWindow(QWidget *parent)
: QMainWindow(parent)
//, m_qFileRaw(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis_raw.fif")
//, m_qEventFile(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis_raw-eve.fif")
//, m_qEvokedFile(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis-ave.fif")
, m_qSettings()
, m_rawSettings()
, ui(new Ui::MainWindowWidget)
, m_pStatusLabel(nullptr)
, m_pCrosshairAction(nullptr)
, m_pButterflyAction(nullptr)
, m_pScalebarsAction(nullptr)
, m_pWhitenButterflyAction(nullptr)
, m_pAnnotationModeAction(nullptr)
{
    ui->setupUi(this);

    //The following functions and their point of calling should NOT be changed
    //Setup the windows first - this NEEDS to be done here because important pointers (window pointers) which are needed for further processing are generated in this function
    setupWindowWidgets();
    setupMainWindow();

    // Setup rest of the GUI
    connectMenus();
    setWindowStatus();

    //Set standard LogLevel
    setLogLevel(_LogLvMax);
}


//*************************************************************************************************************

MainWindow::~MainWindow()
{
}


//*************************************************************************************************************

RawModel* MainWindow::rawModel() const
{
    return m_pDataWindow ? m_pDataWindow->getDataModel() : nullptr;
}


//*************************************************************************************************************

bool MainWindow::ensureLegacyRawModelLoaded(const QString& featureName)
{
    RawModel* model = rawModel();
    if (!model) {
        return false;
    }

    if (model->isFileLoaded() && model->fiffInfo() && !model->fiffInfo()->chs.isEmpty()) {
        return true;
    }

    const QString dialogTitle = featureName.isEmpty()
        ? QStringLiteral("mne_browse")
        : featureName;

#ifdef WASMBUILD
    if (s_wasmByteArray.isEmpty()) {
        QMessageBox::warning(this,
                             dialogTitle,
                             QStringLiteral("Load a raw FIFF file before using this feature."));
        return false;
    }

    QBuffer rawBuffer(&s_wasmByteArray);
        if (!model->loadFiffData(&rawBuffer)) {
            QMessageBox::warning(this,
                             dialogTitle,
                             QStringLiteral("The compatibility processing model could not be initialized from the loaded raw buffer."));
            return false;
        }
#else
    if (m_qFileRaw.fileName().isEmpty() || !QFile::exists(m_qFileRaw.fileName())) {
        QMessageBox::warning(this,
                             dialogTitle,
                             QStringLiteral("Load a raw FIF file before using this feature."));
        return false;
    }

    QFile rawFile(m_qFileRaw.fileName());
    if (!model->loadFiffData(&rawFile)) {
        QMessageBox::warning(this,
                             dialogTitle,
                             QStringLiteral("The compatibility processing model could not be initialized from %1.")
                                 .arg(m_qFileRaw.fileName()));
        return false;
    }
#endif

    if (model->fiffInfo() && m_pDataWindow && m_pDataWindow->fiffInfo()) {
        model->fiffInfo()->bads = m_pDataWindow->fiffInfo()->bads;
        model->fiffInfo()->projs = m_pDataWindow->fiffInfo()->projs;
        model->fiffInfo()->comps = m_pDataWindow->fiffInfo()->comps;
    }

    if (model->fiffInfo()) {
        syncAuxWindowsToFiffInfo(model->fiffInfo(),
                                 model->firstSample(),
                                 model->lastSample());
    }

    return model->isFileLoaded();
}


//*************************************************************************************************************

WhiteningSettings MainWindow::covarianceWhiteningSettings() const
{
    return m_pAverageWindow ? m_pAverageWindow->whiteningSettings()
                            : whiteningSettingsFromSettings(m_qSettings);
}


//*************************************************************************************************************

void MainWindow::setCovarianceWhiteningSettings(const WhiteningSettings& settings, bool saveSettings)
{
    if(saveSettings) {
        saveWhiteningSettings(m_qSettings, settings);
    }

    if(m_pAverageWindow) {
        m_pAverageWindow->setWhiteningSettings(settings);
    }

    if(m_pCovarianceWindow) {
        m_pCovarianceWindow->setWhiteningSettings(settings);
    }

    if(m_pWhitenButterflyAction) {
        QSignalBlocker blocker(m_pWhitenButterflyAction);
        m_pWhitenButterflyAction->setChecked(settings.enableButterfly);
    }
}


//*************************************************************************************************************

void MainWindow::setupWindowWidgets()
{
    //Create data window
    m_pDataWindow = new DataWindow(this);

    //Create dockble event window - QTDesigner used - see / FormFiles
    m_pEventWindow = new EventWindow(this);
    addDockWidget(Qt::LeftDockWidgetArea, m_pEventWindow);
    m_pEventWindow->hide();

    //Create dockable annotation window
    m_pAnnotationWindow = new AnnotationWindow(this);
    addDockWidget(Qt::LeftDockWidgetArea, m_pAnnotationWindow);
    m_pAnnotationWindow->hide();

    //Create dockable covariance window
    m_pCovarianceWindow = new CovarianceWindow(this);
    addDockWidget(Qt::LeftDockWidgetArea, m_pCovarianceWindow);
    m_pCovarianceWindow->hide();

    //Create dockable epoch-review window
    m_pEpochWindow = new EpochWindow(this);
    addDockWidget(Qt::LeftDockWidgetArea, m_pEpochWindow);
    m_pEpochWindow->hide();

    //Create dockable virtual-channel window
    m_pVirtualChannelWindow = new VirtualChannelWindow(this);
    addDockWidget(Qt::LeftDockWidgetArea, m_pVirtualChannelWindow);
    m_pVirtualChannelWindow->hide();

    //Create dockable information window - QTDesigner used - see / FormFiles
    m_pInformationWindow = new InformationWindow(this);
    addDockWidget(Qt::BottomDockWidgetArea, m_pInformationWindow);
    m_pInformationWindow->hide();

    //Create about window - QTDesigner used - see / FormFiles
    m_pAboutWindow = new AboutWindow(this);
    m_pAboutWindow->hide();

    //Create channel info window - QTDesigner used - see / FormFiles
    m_pChInfoWindow = new ChInfoWindow(this);
    addDockWidget(Qt::BottomDockWidgetArea, m_pChInfoWindow);
    m_pChInfoWindow->hide();

    //Create selection manager window - QTDesigner used - see / FormFiles
    m_pChannelSelectionViewDock = new QDockWidget(tr("Layout"), this);
    addDockWidget(Qt::BottomDockWidgetArea, m_pChannelSelectionViewDock);
    m_pChannelSelectionView = new ChannelSelectionView(QString("MNEBrowse"),
                                                       m_pChannelSelectionViewDock,
                                                       m_pChInfoWindow->getDataModel(),
                                                       Qt::Widget);
    m_pChannelSelectionViewDock->setWidget(m_pChannelSelectionView);
    m_pChannelSelectionViewDock->hide();

    //Create average manager window - QTDesigner used - see / FormFiles
    m_pAverageWindow = new AverageWindow(this, m_qEvokedFile);
    addDockWidget(Qt::BottomDockWidgetArea, m_pAverageWindow);
    m_pAverageWindow->hide();

    //Create scale window - QTDesigner used - see / FormFiles
    m_pScaleWindow = new ScaleWindow(this);
    addDockWidget(Qt::LeftDockWidgetArea, m_pScaleWindow);
    m_pScaleWindow->hide();

    //Create filter window - QTDesigner used - see / FormFiles
    m_pFilterWindow = new FilterWindow(this, this);
    addDockWidget(Qt::BottomDockWidgetArea, m_pFilterWindow);
    m_pFilterWindow->hide();

    //Create noise reduction window - QTDesigner used - see / FormFiles
    m_pNoiseReductionWindow = new NoiseReductionWindow(this);
    addDockWidget(Qt::LeftDockWidgetArea, m_pNoiseReductionWindow);
    m_pNoiseReductionWindow->hide();

    //Init windows - TODO: get rid of this here, do this inside the window classes
    m_pDataWindow->init();
    m_pEventWindow->init();
    m_pAnnotationWindow->init();
    m_pCovarianceWindow->init();
    m_pEpochWindow->init();
    m_pVirtualChannelWindow->init();
    m_pScaleWindow->init();

    //Create the toolbar after all indows have been initiliased
    createToolBar();
    m_pAverageWindow->setRecomputeAvailable(!evokedEventCodesFromSettings(m_qSettings).isEmpty());
    setCovarianceWhiteningSettings(whiteningSettingsFromSettings(m_qSettings), false);

    //Connect window signals
    //Change scaling of the data and averaged data whenever a spinbox value changed or the user performs a pinch gesture on the view
    connect(m_pScaleWindow, &ScaleWindow::scalingChannelValueChanged,
            m_pDataWindow, &DataWindow::scaleData);

    connect(m_pScaleWindow, &ScaleWindow::scalingViewValueChanged,
            m_pDataWindow, &DataWindow::changeRowHeight);

    connect(m_pDataWindow, &DataWindow::scaleChannels,
            m_pScaleWindow, &ScaleWindow::scaleAllChannels);

    connect(m_pScaleWindow, &ScaleWindow::scalingChannelValueChanged,
            m_pAverageWindow, &AverageWindow::scaleAveragedData);

    //Hide non selected channels in the data view
    connect(m_pChannelSelectionView, &ChannelSelectionView::showSelectedChannelsOnly,
            m_pDataWindow, &DataWindow::showSelectedChannelsOnly);

    //Connect selection manager with average manager
    connect(m_pChannelSelectionView, &ChannelSelectionView::selectionChanged,
            m_pAverageWindow, &AverageWindow::channelSelectionManagerChanged);

    connect(m_pAverageWindow, &AverageWindow::addAverageRequested,
            this, &MainWindow::computeEvoked);

    connect(m_pAverageWindow, &AverageWindow::recomputeAverageRequested,
            this, &MainWindow::recomputeEvoked);

    connect(m_pAnnotationWindow->getAnnotationModel(), &AnnotationModel::annotationsChanged,
            this, [this]() {
                QVector<DISPLIB::ChannelRhiView::AnnotationSpan> spans;
                const QVector<AnnotationSpanData> modelSpans =
                    m_pAnnotationWindow->getAnnotationModel()->getAnnotationSpans();
                spans.reserve(modelSpans.size());
                for(const AnnotationSpanData& modelSpan : modelSpans) {
                    DISPLIB::ChannelRhiView::AnnotationSpan span;
                    span.startSample = modelSpan.startSample;
                    span.endSample = modelSpan.endSample;
                    span.color = modelSpan.color;
                    span.label = modelSpan.label;
                    spans.append(span);
                }

                m_pDataWindow->setAnnotations(spans);
                setWindowStatus();
            });

    connect(m_pEpochWindow->getEpochModel(), &EpochModel::epochsChanged,
            this, [this]() {
                refreshReviewedEvokedSet(QStringLiteral("Evoked responses updated from the reviewed epochs."));
            });

    connect(m_pCovarianceWindow, &CovarianceWindow::whiteningSettingsChanged,
            this, [this](const WhiteningSettings& settings) {
                setCovarianceWhiteningSettings(settings);
                if((settings.enableButterfly || settings.enableLayout)
                   && !m_covariance.isEmpty()
                   && !m_pAverageWindow->isVisible()) {
                    showWindow(m_pAverageWindow);
                }
            });

    connect(m_pDataWindow, &DataWindow::annotationRangeSelected,
            this, &MainWindow::handleAnnotationRangeSelected);

    connect(m_pDataWindow, &DataWindow::annotationBoundaryMoved,
            this, [this](int annotationIndex, bool isStartBoundary, int newSample) {
                m_pAnnotationWindow->getAnnotationModel()->updateAnnotationBoundary(
                    annotationIndex, isStartBoundary, newSample);
            });

    // Connect crosshair cursor data to status bar
    // The signal is forwarded: ChannelRhiView → ChannelDataView → here
    if (m_pDataWindow->getChannelDataView()) {
        connect(m_pDataWindow->getChannelDataView(), &DISPLIB::ChannelDataView::cursorDataChanged,
                this, [this](float timeSec, float amplitude, const QString &channelName, const QString &unitLabel) {
                    auto fmtAmp = [](float amp, const QString &unit) -> QString {
                        float absAmp = qAbs(amp);
                        if (absAmp == 0.f) return QStringLiteral("0 ") + unit;
                        if (absAmp < 1e-9f) return QString::number(amp * 1e12f, 'f', 1) + QStringLiteral(" p") + unit;
                        if (absAmp < 1e-6f) return QString::number(amp * 1e9f, 'f', 1) + QStringLiteral(" n") + unit;
                        if (absAmp < 1e-3f) return QString::number(amp * 1e6f, 'f', 1) + QStringLiteral(" µ") + unit;
                        if (absAmp < 1.f) return QString::number(amp * 1e3f, 'f', 1) + QStringLiteral(" m") + unit;
                        return QString::number(amp, 'f', 3) + QStringLiteral(" ") + unit;
                    };
                    QString timeStr;
                    bool isClock = m_pDataWindow->getChannelDataView()->clockTimeFormat();
                    if (isClock && timeSec >= 0.f) {
                        int totalMs = static_cast<int>(timeSec * 1000.f + 0.5f);
                        int m   = totalMs / 60000;
                        int sec = (totalMs % 60000) / 1000;
                        int ms  = totalMs % 1000;
                        timeStr = QString("%1:%2.%3")
                            .arg(m, 2, 10, QChar('0'))
                            .arg(sec, 2, 10, QChar('0'))
                            .arg(ms, 3, 10, QChar('0'));
                    } else {
                        timeStr = QString::number(timeSec, 'f', 3) + QStringLiteral(" s");
                    }
                    QString msg = QString("  %1  |  t = %2  |  %3")
                        .arg(channelName, timeStr, fmtAmp(amplitude, unitLabel));
                    statusBar()->showMessage(msg);
                });

        // Sync toolbar action checked states when toggled via keyboard shortcuts
        connect(m_pDataWindow->getChannelDataView(), &DISPLIB::ChannelDataView::crosshairToggled,
                m_pCrosshairAction, &QAction::setChecked);
        connect(m_pDataWindow->getChannelDataView(), &DISPLIB::ChannelDataView::butterflyToggled,
                m_pButterflyAction, &QAction::setChecked);
        connect(m_pDataWindow->getChannelDataView(), &DISPLIB::ChannelDataView::scalebarsToggled,
                m_pScalebarsAction, &QAction::setChecked);
    }

    // Connect time format toggle requested from DataWindow (via 'T' key)
    connect(m_pDataWindow, &DataWindow::timeFormatToggleRequested,
            this, [this]() {
                if (m_pDataWindow->getChannelDataView()) {
                    m_pDataWindow->getChannelDataView()->toggleTimeFormat();
                    bool isClock = m_pDataWindow->getChannelDataView()->clockTimeFormat();
                    statusBar()->showMessage(isClock
                        ? QStringLiteral("Time format: mm:ss.ms")
                        : QStringLiteral("Time format: seconds"), 3000);
                }
            });

    connect(m_pVirtualChannelWindow->getVirtualChannelModel(), &VirtualChannelModel::virtualChannelsChanged,
            this, [this]() {
                m_pDataWindow->setVirtualChannels(
                    m_pVirtualChannelWindow->getVirtualChannelModel()->virtualChannels());
                setWindowStatus();
            });

    //Connect channel info window with legacy processing updates, layout manager, average manager and the data window
    connect(rawModel(), &RawModel::assignedOperatorsChanged,
            m_pChInfoWindow->getDataModel().data(), &ChannelInfoModel::assignedOperatorsChanged);

    connect(m_pChannelSelectionView, &ChannelSelectionView::loadedLayoutMap,
            m_pChInfoWindow->getDataModel().data(), &ChannelInfoModel::layoutChanged);

    connect(m_pChInfoWindow->getDataModel().data(), &ChannelInfoModel::channelsMappedToLayout,
            m_pChannelSelectionView, &ChannelSelectionView::setCurrentlyMappedFiffChannels);

    connect(m_pChInfoWindow->getDataModel().data(), &ChannelInfoModel::channelsMappedToLayout,
            m_pAverageWindow, &AverageWindow::setMappedChannelNames);

    connect(m_pNoiseReductionWindow, &NoiseReductionWindow::projSelectionChanged,
            this, [this]() {
                // Update projections in the demand-paged GPU path
                m_pDataWindow->updateProjections();

                // Also update legacy model if it is already loaded
                if (rawModel() && rawModel()->isFileLoaded())
                    rawModel()->updateProjections();
            });

    connect(m_pNoiseReductionWindow, &NoiseReductionWindow::compSelectionChanged,
            this, [this](int to) {
                if (ensureLegacyRawModelLoaded(QStringLiteral("Noise Reduction"))) {
                    rawModel()->updateCompensator(to);
                }
            });

    if(m_pDataWindow->isFiffFileLoaded()) {
        auto fiffInfo = m_pDataWindow->fiffInfo();
        m_pScaleWindow->hideSpinBoxes(fiffInfo);
        m_pChInfoWindow->getDataModel()->setFiffInfo(fiffInfo);
        m_pChInfoWindow->getDataModel()->layoutChanged(m_pChannelSelectionView->getLayoutMap());
        m_pChannelSelectionView->setCurrentlyMappedFiffChannels(m_pChInfoWindow->getDataModel()->getMappedChannelsList());
        m_pChannelSelectionView->newFiffFileLoaded(fiffInfo);
        m_pFilterWindow->newFileLoaded(fiffInfo);
        m_pNoiseReductionWindow->setFiffInfo(fiffInfo);
    }
}


//*************************************************************************************************************

void MainWindow::createToolBar()
{
    //Create toolbar
    QToolBar *toolBar = new QToolBar(this);
    toolBar->setOrientation(Qt::Vertical);
    toolBar->setMovable(true);
    // Suppress macOS default white-rectangle rendering for pressed/checked buttons
    toolBar->setStyleSheet(QStringLiteral(
        "QToolButton { background: transparent; border: none; padding: 4px; }"
        "QToolButton:checked { background: rgba(60,140,220,40); border: 1px solid rgba(60,140,220,120); border-radius: 4px; }"
        "QToolButton:pressed { background: rgba(60,140,220,70); border: 1px solid rgba(60,140,220,160); border-radius: 4px; }"
        "QToolButton:hover { background: rgba(0,0,0,15); border-radius: 4px; }"
    ));

    //Add DC removal action
    m_pRemoveDCAction = new QAction(QIcon(":/Resources/Images/removeDC.png"),tr("Remove DC component"), this);
    m_pRemoveDCAction->setStatusTip(tr("Remove the DC component by subtracting the channel mean"));
    connect(m_pRemoveDCAction,&QAction::triggered, [this](){
        if(m_pDataWindow->getDataDelegate()->isRemoveDC()) {
            m_pRemoveDCAction->setIcon(QIcon(":/Resources/Images/removeDC.png"));
            m_pRemoveDCAction->setToolTip("Remove DC component");
            m_pDataWindow->setStatusTip(tr("Remove the DC component by subtracting the channel mean"));
            m_pDataWindow->getDataDelegate()->setRemoveDC(false);
            if (m_pDataWindow->getChannelDataView())
                m_pDataWindow->getChannelDataView()->setRemoveDC(false);
        }
        else {
            m_pRemoveDCAction->setIcon(QIcon(":/Resources/Images/addDC.png"));
            m_pRemoveDCAction->setToolTip("Add DC component");
            m_pRemoveDCAction->setStatusTip(tr("Add the DC component"));
            m_pDataWindow->getDataDelegate()->setRemoveDC(true);
            if (m_pDataWindow->getChannelDataView())
                m_pDataWindow->getChannelDataView()->setRemoveDC(true);
        }

        m_pDataWindow->updateDataTableViews();
    });
    toolBar->addAction(m_pRemoveDCAction);

    //Add show/hide bad channel button
    m_pHideBadAction = new QAction(QIcon(":/Resources/Images/hideBad.png"),tr("Hide all bad channels"), this);
    m_pHideBadAction->setStatusTip(tr("Hide all bad channels"));
    connect(m_pHideBadAction,&QAction::triggered, [this](){
        if(m_pHideBadAction->toolTip() == "Show all bad channels") {
            m_pHideBadAction->setIcon(QIcon(":/Resources/Images/hideBad.png"));
            m_pHideBadAction->setToolTip("Hide all bad channels");
            m_pDataWindow->setStatusTip(tr("Hide all bad channels"));
            m_pDataWindow->hideBadChannels(false);
        }
        else {
            m_pHideBadAction->setIcon(QIcon(":/Resources/Images/showBad.png"));
            m_pHideBadAction->setToolTip("Show all bad channels");
            m_pHideBadAction->setStatusTip(tr("Show all bad channels"));
            m_pDataWindow->hideBadChannels(true);
        }
    });
    toolBar->addAction(m_pHideBadAction);

    toolBar->addSeparator();

    // --- Interactive inspection tools ---

    // Helper – creates a QIcon with Off (normal) and On (highlighted) states
    auto makeIcon = [](const std::function<void(QPainter&, int)>& paintFn) -> QIcon {
        const int sz = 128;
        QIcon icon;
        // --- Off state ---
        {
            QImage img(sz, sz, QImage::Format_ARGB32_Premultiplied);
            img.fill(Qt::transparent);
            QPainter p(&img);
            p.setRenderHint(QPainter::Antialiasing);
            paintFn(p, sz);
            p.end();
            icon.addPixmap(QPixmap::fromImage(img), QIcon::Normal, QIcon::Off);
        }
        // --- On (checked) state – tinted background ---
        {
            QImage img(sz, sz, QImage::Format_ARGB32_Premultiplied);
            img.fill(Qt::transparent);
            QPainter p(&img);
            p.setRenderHint(QPainter::Antialiasing);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(60, 140, 220, 50));
            p.drawRoundedRect(4, 4, sz-8, sz-8, 12, 12);
            paintFn(p, sz);
            // Thin highlight border
            p.setPen(QPen(QColor(60, 140, 220, 160), 4));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(4, 4, sz-8, sz-8, 12, 12);
            p.end();
            icon.addPixmap(QPixmap::fromImage(img), QIcon::Normal, QIcon::On);
        }
        return icon;
    };

    //Toggle crosshair (X)
    QIcon crosshairIcon = makeIcon([](QPainter& p, int sz){
        QPen pen(QColor(0xd0, 0x40, 0x40), 3);
        p.setPen(pen);
        p.drawLine(sz/2, 4, sz/2, sz-4);
        p.drawLine(4, sz/2, sz-4, sz/2);
    });
    m_pCrosshairAction = new QAction(crosshairIcon, tr("Toggle crosshair (X)"), this);
    m_pCrosshairAction->setCheckable(true);
    m_pCrosshairAction->setStatusTip(tr("Toggle crosshair cursor overlay"));
    connect(m_pCrosshairAction, &QAction::triggered, [this](bool checked){
        if(m_pDataWindow && m_pDataWindow->getChannelDataView())
            m_pDataWindow->getChannelDataView()->setCrosshairEnabled(checked);
    });
    toolBar->addAction(m_pCrosshairAction);

    //Toggle butterfly mode (B)
    QIcon butterflyIcon = makeIcon([](QPainter& p, int sz){
        QPen pen(QColor(0x40, 0x80, 0xd0), 2);
        p.setPen(pen);
        // Simple butterfly-like overlapping sine curves
        for(int k = 0; k < 3; ++k) {
            QPainterPath path;
            double phase = k * 1.0;
            path.moveTo(4, sz/2);
            for(int x = 4; x <= sz-4; ++x) {
                double t = double(x-4)/(sz-8) * 2.0 * M_PI;
                double y = sz/2 + (sz/4) * std::sin(t + phase) * (1.0 - 0.2*k);
                path.lineTo(x, y);
            }
            p.drawPath(path);
        }
    });
    m_pButterflyAction = new QAction(butterflyIcon, tr("Toggle butterfly mode (B)"), this);
    m_pButterflyAction->setCheckable(true);
    m_pButterflyAction->setStatusTip(tr("Overlay all channels of the same type"));
    connect(m_pButterflyAction, &QAction::triggered, [this](bool checked){
        if(m_pDataWindow && m_pDataWindow->getChannelDataView())
            m_pDataWindow->getChannelDataView()->setButterflyMode(checked);
    });
    toolBar->addAction(m_pButterflyAction);

    //Toggle scalebars (S)
    QIcon scalebarsIcon = makeIcon([](QPainter& p, int sz){
        QPen pen(QColor(0x40, 0xa0, 0x40), 3);
        p.setPen(pen);
        // Vertical bar with horizontal ticks
        int cx = sz/2, top = 8, bot = sz-8;
        p.drawLine(cx, top, cx, bot);
        p.drawLine(cx-8, top, cx+8, top);
        p.drawLine(cx-8, bot, cx+8, bot);
        p.drawLine(cx-6, sz/2, cx+6, sz/2);
    });
    m_pScalebarsAction = new QAction(scalebarsIcon, tr("Toggle scalebars (S)"), this);
    m_pScalebarsAction->setCheckable(true);
    m_pScalebarsAction->setStatusTip(tr("Show amplitude scalebars on the signal view"));
    connect(m_pScalebarsAction, &QAction::triggered, [this](bool checked){
        if(m_pDataWindow && m_pDataWindow->getChannelDataView())
            m_pDataWindow->getChannelDataView()->setScalebarsVisible(checked);
    });
    toolBar->addAction(m_pScalebarsAction);

    //Toggle time format (T)
    QIcon timeIcon = makeIcon([](QPainter& p, int sz){
        QFont f = p.font();
        f.setPixelSize(sz * 0.45);
        f.setBold(true);
        p.setFont(f);
        p.setPen(QColor(0x60, 0x60, 0xc0));
        p.drawText(QRect(0,0,sz,sz), Qt::AlignCenter, "T");
    });
    QAction* timeFormatAction = new QAction(timeIcon, tr("Toggle time format (T)"), this);
    timeFormatAction->setCheckable(true);
    timeFormatAction->setStatusTip(tr("Switch between seconds and clock time display"));
    connect(timeFormatAction, &QAction::triggered, [this](bool checked){
        Q_UNUSED(checked);
        if(m_pDataWindow && m_pDataWindow->getChannelDataView())
            m_pDataWindow->getChannelDataView()->toggleTimeFormat();
    });
    toolBar->addAction(timeFormatAction);

    toolBar->addSeparator();

    //Toggle visibility of the event manager
    QAction* showEventManager = new QAction(QIcon(":/Resources/Images/showEventManager.png"),tr("Toggle event manager"), this);
    showEventManager->setStatusTip(tr("Toggle the event manager"));
    connect(showEventManager, &QAction::triggered, this, [this](){
        showWindow(m_pEventWindow);
    });
    toolBar->addAction(showEventManager);

    //Toggle visibility of the filter window
    QAction* showFilterWindow = new QAction(QIcon(":/Resources/Images/showFilterWindow.png"),tr("Toggle filter window"), this);
    showFilterWindow->setStatusTip(tr("Toggle filter window"));
    connect(showFilterWindow, &QAction::triggered, this, [this](){
        showWindow(m_pFilterWindow);
    });
    toolBar->addAction(showFilterWindow);

    //Toggle visibility of the Selection manager
    QAction* showSelectionManager = new QAction(QIcon(":/Resources/Images/showSelectionManager.png"),tr("Toggle selection manager"), this);
    showSelectionManager->setStatusTip(tr("Toggle the selection manager"));
    connect(showSelectionManager, &QAction::triggered, this, [this](){
        showWindow(m_pChannelSelectionViewDock);
    });
    toolBar->addAction(showSelectionManager);

    //Toggle visibility of the scaling window
    QAction* showScalingWindow = new QAction(QIcon(":/Resources/Images/showScalingWindow.png"),tr("Toggle scaling window"), this);
    showScalingWindow->setStatusTip(tr("Toggle the scaling window"));
    connect(showScalingWindow, &QAction::triggered, this, [this](){
        showWindow(m_pScaleWindow);
    });
    toolBar->addAction(showScalingWindow);

    //Toggle visibility of the average manager
    QAction* showAverageManager = new QAction(QIcon(":/Resources/Images/showAverageManager.png"),tr("Toggle average manager"), this);
    showAverageManager->setStatusTip(tr("Toggle the average manager"));
    connect(showAverageManager, &QAction::triggered, this, [this](){
        showWindow(m_pAverageWindow);
    });
    toolBar->addAction(showAverageManager);

    //Toggle visibility of the noise reduction manager
    QAction* showNoiseReductionManager = new QAction(QIcon(":/Resources/Images/showNoiseReductionWindow.png"),tr("Toggle noise reduction manager"), this);
    showNoiseReductionManager->setStatusTip(tr("Toggle the noise reduction manager"));
    connect(showNoiseReductionManager, &QAction::triggered, this, [this](){
        showWindow(m_pNoiseReductionWindow);
    });
    toolBar->addAction(showNoiseReductionManager);

    toolBar->addSeparator();

    //Toggle visibility of the channel information window manager
    QAction* showChInfo = new QAction(QIcon(":/Resources/Images/showChInformationWindow.png"),tr("Channel info"), this);
    showChInfo->setStatusTip(tr("Toggle channel info window"));
    connect(showChInfo, &QAction::triggered, this, [this](){
        showWindow(m_pChInfoWindow);
    });
    toolBar->addAction(showChInfo);

    //Toggle visibility of the information window
//    QAction* showInformationWindow = new QAction(QIcon(":/Resources/Images/showInformationWindow.png"),tr("Toggle information window"), this);
//    showInformationWindow->setStatusTip(tr("Toggle the information window"));
//    connect(showInformationWindow, &QAction::triggered, this, [=](){
//        showWindow(m_pInformationWindow);
//    });
//    toolBar->addAction(showInformationWindow);

    this->addToolBar(Qt::RightToolBarArea,toolBar);
}


//*************************************************************************************************************

void MainWindow::connectMenus()
{
    QAction* loadInverseOperatorAction = new QAction(tr("Load Inverse Operator..."), this);
    ui->menuTest->insertAction(ui->m_loadEvokedAction, loadInverseOperatorAction);

    QAction* loadCovarianceAction = new QAction(tr("Load Covariance..."), this);
    ui->menuTest->insertAction(ui->m_loadEvokedAction, loadCovarianceAction);

    QAction* computeCovarianceAction = new QAction(tr("Compute Covariance..."), this);
    ui->menuTest->insertAction(ui->m_loadEvokedAction, computeCovarianceAction);

    QAction* saveCovarianceAction = new QAction(tr("Save Covariance (fif)..."), this);
    ui->menuTest->insertAction(ui->m_loadEvokedAction, saveCovarianceAction);

    QAction* computeEvokedAction = new QAction(tr("Compute Evoked..."), this);
    ui->menuTest->insertAction(ui->m_loadEvokedAction, computeEvokedAction);

    QAction* recomputeEvokedAction = new QAction(tr("Recompute Last Evoked"), this);
    ui->menuTest->insertAction(ui->m_loadEvokedAction, recomputeEvokedAction);

    QAction* saveEvokedAction = new QAction(tr("Save Evoked (fif)..."), this);
    ui->menuTest->insertAction(ui->m_quitAction, saveEvokedAction);

    QAction* computeSourceEstimateAction = new QAction(tr("Compute Source Estimate..."), this);
    ui->menuTest->insertAction(ui->m_quitAction, computeSourceEstimateAction);

    QAction* loadAnnotationsAction = new QAction(tr("Load Annotations..."), this);
    ui->menuTest->insertAction(ui->m_loadEvokedAction, loadAnnotationsAction);

    QAction* saveAnnotationsAction = new QAction(tr("Save Annotations..."), this);
    ui->menuTest->insertAction(ui->m_loadEvokedAction, saveAnnotationsAction);

    QAction* loadVirtualChannelsAction = new QAction(tr("Load Virtual Channels..."), this);
    ui->menuTest->insertAction(ui->m_loadEvokedAction, loadVirtualChannelsAction);

    QAction* saveVirtualChannelsAction = new QAction(tr("Save Virtual Channels..."), this);
    ui->menuTest->insertAction(ui->m_loadEvokedAction, saveVirtualChannelsAction);

    m_pWhitenButterflyAction = new QAction(tr("Whiten Butterfly Plot"), this);
    m_pWhitenButterflyAction->setCheckable(true);
    ui->menuTest->insertAction(ui->m_quitAction, m_pWhitenButterflyAction);

    QAction* annotationManagerAction = new QAction(tr("Annotation manager..."), this);
    ui->menuWindows->insertAction(ui->m_informationAction, annotationManagerAction);

    QAction* covarianceManagerAction = new QAction(tr("Covariance manager..."), this);
    ui->menuWindows->insertAction(ui->m_informationAction, covarianceManagerAction);

    QAction* epochManagerAction = new QAction(tr("Epoch manager..."), this);
    ui->menuWindows->insertAction(ui->m_informationAction, epochManagerAction);

    QAction* virtualChannelManagerAction = new QAction(tr("Virtual channel manager..."), this);
    ui->menuWindows->insertAction(ui->m_informationAction, virtualChannelManagerAction);

    m_pAnnotationModeAction = new QAction(tr("Annotation Mode"), this);
    m_pAnnotationModeAction->setCheckable(true);
    m_pAnnotationModeAction->setStatusTip(tr("When enabled, Shift-drag in the raw browser creates annotation spans."));
    ui->menuAdjust->addAction(m_pAnnotationModeAction);

    //File
    connect(ui->m_openAction, &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->m_writeAction, &QAction::triggered, this, &MainWindow::writeFile);
    connect(ui->m_loadEvents, &QAction::triggered, this, &MainWindow::loadEvents);
    connect(ui->m_saveEvents, &QAction::triggered, this, &MainWindow::saveEvents);
    connect(loadInverseOperatorAction, &QAction::triggered, this, &MainWindow::loadInverseOperator);
    connect(loadAnnotationsAction, &QAction::triggered, this, &MainWindow::loadAnnotations);
    connect(saveAnnotationsAction, &QAction::triggered, this, &MainWindow::saveAnnotations);
    connect(loadVirtualChannelsAction, &QAction::triggered, this, &MainWindow::loadVirtualChannels);
    connect(saveVirtualChannelsAction, &QAction::triggered, this, &MainWindow::saveVirtualChannels);
    connect(loadCovarianceAction, &QAction::triggered, this, &MainWindow::loadCovariance);
    connect(computeCovarianceAction, &QAction::triggered, this, &MainWindow::computeCovariance);
    connect(saveCovarianceAction, &QAction::triggered, this, &MainWindow::saveCovariance);
    connect(computeEvokedAction, &QAction::triggered, this, &MainWindow::computeEvoked);
    connect(recomputeEvokedAction, &QAction::triggered, this, &MainWindow::recomputeEvoked);
    connect(ui->m_loadEvokedAction, &QAction::triggered, this, &MainWindow::loadEvoked);
    connect(saveEvokedAction, &QAction::triggered, this, &MainWindow::saveEvoked);
    connect(computeSourceEstimateAction, &QAction::triggered, this, &MainWindow::computeSourceEstimate);
    connect(m_pWhitenButterflyAction, &QAction::toggled, this, [this](bool checked){
        if(checked && m_covariance.isEmpty()) {
            QMessageBox::warning(this,
                                 "Whiten Butterfly Plot",
                                 "Load or compute a covariance matrix before enabling whitening.");
            m_pWhitenButterflyAction->setChecked(false);
            return;
        }

        WhiteningSettings settings = covarianceWhiteningSettings();
        settings.enableButterfly = checked;
        setCovarianceWhiteningSettings(settings);

        if(checked && !m_pAverageWindow->isVisible()) {
            showWindow(m_pAverageWindow);
        }
    });
    connect(ui->m_quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));

    //Adjust
    connect(ui->m_filterAction, &QAction::triggered, this, [this](){
        showWindow(m_pFilterWindow);
    });
    connect(m_pAnnotationModeAction, &QAction::toggled, this, [this](bool checked) {
        m_pDataWindow->setAnnotationSelectionEnabled(checked);
        statusBar()->showMessage(checked
                                 ? QStringLiteral("Annotation mode enabled: Shift-drag in the raw browser to create spans.")
                                 : QStringLiteral("Annotation mode disabled."),
                                 4000);
    });

    //Windows
    connect(ui->m_eventAction, &QAction::triggered, this, [this](){
        showWindow(m_pEventWindow);
    });
    connect(annotationManagerAction, &QAction::triggered, this, [this](){
        showWindow(m_pAnnotationWindow);
    });
    connect(covarianceManagerAction, &QAction::triggered, this, [this](){
        showWindow(m_pCovarianceWindow);
    });
    connect(epochManagerAction, &QAction::triggered, this, [this](){
        showWindow(m_pEpochWindow);
    });
    connect(virtualChannelManagerAction, &QAction::triggered, this, [this](){
        showWindow(m_pVirtualChannelWindow);
    });
    connect(ui->m_informationAction, &QAction::triggered, this, [this](){
        showWindow(m_pInformationWindow);
    });
    connect(ui->m_channelSelectionManagerAction, &QAction::triggered, this, [this](){
        showWindow(m_pChannelSelectionViewDock);
    });
    connect(ui->m_averageWindowAction, &QAction::triggered, this, [this](){
        showWindow(m_pAverageWindow);
    });
    connect(ui->m_scalingAction, &QAction::triggered, this, [this](){
        showWindow(m_pScaleWindow);
    });
    connect(ui->m_ChInformationAction, &QAction::triggered, this, [this](){
        showWindow(m_pChInfoWindow);
    });
    connect(ui->m_noiseReductionManagerAction, &QAction::triggered, this, [this](){
        showWindow(m_pNoiseReductionWindow);
    });

    //Help
    connect(ui->m_aboutAction, &QAction::triggered, this, [this](){
        showWindow(m_pAboutWindow);
    });

    connect(ui->m_keyboardShortcutsAction, &QAction::triggered, this, [this](){
        QMessageBox::information(this, tr("Keyboard Shortcuts"),
            tr("<b>Navigation:</b><br>"
               "← / → — Scroll left/right (¼ page)<br>"
               "Shift+← / → — Scroll left/right (full page)<br>"
               "Home / End — Decrease/increase time window<br>"
               "+/= — Increase amplitude scale<br>"
               "- — Decrease amplitude scale<br>"
               "<br>"
               "<b>Display:</b><br>"
               "B — Toggle butterfly mode<br>"
               "D — Toggle DC removal<br>"
               "S — Toggle scalebars<br>"
               "T — Toggle time format (seconds / clock)<br>"
               "X — Toggle crosshair cursor<br>"
               "Ctrl+D — Clear channel selection<br>"
               "? — Show this help<br>"
               "<br>"
               "<b>Mouse (Data View):</b><br>"
               "Left-drag — Pan through time<br>"
               "Right-drag — Ruler measurement<br>"
               "Alt+Left-drag — Pan (alternative)<br>"
               "Double-click — Toggle channel bad/good<br>"
               "Scroll wheel — Scroll through channels<br>"
               "<br>"
               "<b>Annotations:</b><br>"
               "Enable Annotation Mode from toolbar<br>"
               "Right-drag — Select time range for annotation<br>"
               "Drag boundary — Resize annotation span<br>"
               "<br>"
               "<b>File:</b><br>"
               "Ctrl+O — Open file<br>"
               "Ctrl+S — Save file"));
    });
}


//*************************************************************************************************************

void MainWindow::setupMainWindow()
{
    //set Window functions
    resize(m_qSettings.value("MainWindow/size", QSize(MAINWINDOW_WINDOW_SIZE_W, MAINWINDOW_WINDOW_SIZE_H)).toSize()); //Resize to predefined default size
    move(m_qSettings.value("MainWindow/position", QPoint(MAINWINDOW_WINDOW_POSITION_X, MAINWINDOW_WINDOW_POSITION_Y)).toPoint()); // Move this main window to position 50/50 on the screen

    // Restore dock widget layout (positions, sizes, docking areas)
    if (m_qSettings.contains("MainWindow/state")) {
        restoreState(m_qSettings.value("MainWindow/state").toByteArray());
    }

    // Restore view toggle states
    if (m_pRemoveDCAction && m_qSettings.contains("MainWindow/View/removeDC"))
        m_pRemoveDCAction->setChecked(m_qSettings.value("MainWindow/View/removeDC").toBool());
    if (m_pHideBadAction && m_qSettings.contains("MainWindow/View/hideBad"))
        m_pHideBadAction->setChecked(m_qSettings.value("MainWindow/View/hideBad").toBool());
    if (m_pCrosshairAction && m_qSettings.contains("MainWindow/View/crosshair"))
        m_pCrosshairAction->setChecked(m_qSettings.value("MainWindow/View/crosshair").toBool());
    if (m_pButterflyAction && m_qSettings.contains("MainWindow/View/butterfly"))
        m_pButterflyAction->setChecked(m_qSettings.value("MainWindow/View/butterfly").toBool());
    if (m_pScalebarsAction && m_qSettings.contains("MainWindow/View/scalebars"))
        m_pScalebarsAction->setChecked(m_qSettings.value("MainWindow/View/scalebars").toBool());

    //Set data window as central widget - This is needed because we are using QDockWidgets
    setCentralWidget(m_pDataWindow);
}


//*************************************************************************************************************

void MainWindow::setWindowStatus()
{
    //Set window title
    QString title;
    //title = QString("%1").arg(CInfo::AppNameShort());
    title = QString("Visualize and Process");
    setWindowTitle(title);

    //Set status bar
    //Set data file informations
    if (m_pDataWindow->isFiffFileLoaded()) {
        double dur = m_pDataWindow->fiffFileDurationSeconds();
        int durMin = static_cast<int>(dur) / 60;
        double durSec = dur - durMin * 60.0;
        QString durStr = durMin > 0
            ? QString("%1 min %2 s").arg(durMin).arg(durSec, 0, 'f', 1)
            : QString("%1 s").arg(dur, 0, 'f', 1);
        title = QString("Data file: %1  |  Duration: %2")
            .arg(m_pDataWindow->fiffFileName(), durStr);
    } else {
        title = QString("No data file");
    }

    //Set event file informations
    if(m_pEventWindow->getEventModel()->isFileLoaded()) {
        int idx = m_qEventFile.fileName().lastIndexOf("/");
        QString filename = m_qEventFile.fileName().remove(0,idx+1);

        title.append(QString("  -  Event file: %1").arg(filename));
    }
    else
        title.append("  -  No event file");

    if(m_pAnnotationWindow->getAnnotationModel()->rowCount() > 0) {
        if(m_qAnnotationFile.fileName().isEmpty()) {
            title.append("  -  Annotations: unsaved");
        } else {
            int idx = m_qAnnotationFile.fileName().lastIndexOf("/");
            QString filename = m_qAnnotationFile.fileName().remove(0,idx+1);
            title.append(QString("  -  Annotations: %1").arg(filename));
        }
    } else {
        title.append("  -  No annotations");
    }

    if(m_pVirtualChannelWindow->getVirtualChannelModel()->rowCount() > 0) {
        if(m_qVirtualChannelFile.fileName().isEmpty()) {
            title.append("  -  Virtual channels: unsaved");
        } else {
            int idx = m_qVirtualChannelFile.fileName().lastIndexOf("/");
            QString filename = m_qVirtualChannelFile.fileName().remove(0,idx+1);
            title.append(QString("  -  Virtual channels: %1").arg(filename));
        }
    } else {
        title.append("  -  No virtual channels");
    }

    //Set evoked file informations
    if(m_pAverageWindow->getAverageModel()->isFileLoaded()) {
        if(m_qEvokedFile.fileName().isEmpty()) {
            title.append("  -  Evoked data: unsaved");
        } else {
            int idx = m_qEvokedFile.fileName().lastIndexOf("/");
            QString filename = m_qEvokedFile.fileName().remove(0,idx+1);
            title.append(QString("  -  Evoked file: %1").arg(filename));
        }
    }
    else
        title.append("  -  No evoked file");

    if(!m_covariance.isEmpty()) {
        if(m_qCovFile.fileName().isEmpty()) {
            title.append("  -  Covariance: unsaved");
        } else {
            int idx = m_qCovFile.fileName().lastIndexOf("/");
            QString filename = m_qCovFile.fileName().remove(0,idx+1);
            title.append(QString("  -  Covariance file: %1").arg(filename));
        }
    } else {
        title.append("  -  No covariance");
    }

    if(m_inverseOperator.nsource > 0 && !m_inverseOperator.src.isEmpty()) {
        if(m_qInverseOperatorFile.fileName().isEmpty()) {
            title.append("  -  Inverse: loaded");
        } else {
            int idx = m_qInverseOperatorFile.fileName().lastIndexOf("/");
            QString filename = m_qInverseOperatorFile.fileName().remove(0,idx+1);
            title.append(QString("  -  Inverse file: %1").arg(filename));
        }
    } else {
        title.append("  -  No inverse");
    }

    if(!m_pStatusLabel) {
        m_pStatusLabel = new QLabel(this);
        statusBar()->addWidget(m_pStatusLabel, 1);
    }
    m_pStatusLabel->setText(title);
}

//*************************************************************************************************************

void MainWindow::syncAuxWindowsToFiffInfo(FiffInfo::SPtr fiffInfo,
                                          int firstSample,
                                          int lastSample)
{
    if (!fiffInfo)
        return;

    m_pEventWindow->getEventModel()->setFiffInfo(fiffInfo);
    m_pEventWindow->getEventModel()->setFirstLastSample(firstSample, lastSample);
    m_pAnnotationWindow->getAnnotationModel()->setFiffInfo(fiffInfo);
    m_pAnnotationWindow->getAnnotationModel()->setFirstLastSample(firstSample, lastSample);
    m_pVirtualChannelWindow->setAvailableChannelNames(fiffInfo->ch_names);
    if(!m_covariance.isEmpty()) {
        const QString covarianceSource = m_qCovFile.fileName().isEmpty()
            ? QStringLiteral("Computed during this session")
            : QStringLiteral("Loaded from %1").arg(QFileInfo(m_qCovFile.fileName()).fileName());
        m_pCovarianceWindow->setCovariance(m_covariance, covarianceSource, fiffInfo);
    }

    m_pScaleWindow->hideSpinBoxes(fiffInfo);

    m_pChInfoWindow->getDataModel()->setFiffInfo(fiffInfo);
    m_pChInfoWindow->getDataModel()->layoutChanged(m_pChannelSelectionView->getLayoutMap());
    m_pChannelSelectionView->setCurrentlyMappedFiffChannels(m_pChInfoWindow->getDataModel()->getMappedChannelsList());
    m_pChannelSelectionView->newFiffFileLoaded(fiffInfo);

    m_pFilterWindow->newFileLoaded(fiffInfo);
    m_pNoiseReductionWindow->setFiffInfo(fiffInfo);
}


//*************************************************************************************************************
//Log
void MainWindow::writeToLog(const QString& logMsg, LogKind lgknd, LogLevel lglvl)
{
    m_pInformationWindow->writeToLog(logMsg, lgknd, lglvl);
}


//*************************************************************************************************************

void MainWindow::setLogLevel(LogLevel lvl)
{
    m_pInformationWindow->setLogLevel(lvl);
}


//*************************************************************************************************************
// SLOTS
void MainWindow::openFile()
{
#ifdef WASMBUILD
    auto fileContentReady = [&](const QString &fileName, const QByteArray &fileContent) {
        if (!fileName.isEmpty()) {
            if(m_qFileRaw.isOpen())
                m_qFileRaw.close();

            m_qFileRaw.setFileName(fileName);
            m_covariance.clear();
            if(m_qCovFile.isOpen())
                m_qCovFile.close();
            m_qCovFile.setFileName(QString());
            if(m_qInverseOperatorFile.isOpen())
                m_qInverseOperatorFile.close();
            m_qInverseOperatorFile.setFileName(QString());
            m_inverseOperator = MNELIB::MNEInverseOperator();
            if(m_qAnnotationFile.isOpen())
                m_qAnnotationFile.close();
            m_qAnnotationFile.setFileName(QString());
            if(m_qVirtualChannelFile.isOpen())
                m_qVirtualChannelFile.close();
            m_qVirtualChannelFile.setFileName(QString());
            m_pAverageWindow->clearNoiseCovariance();
            m_pCovarianceWindow->clearCovariance();
            WhiteningSettings whiteningSettings = covarianceWhiteningSettings();
            whiteningSettings.enableButterfly = false;
            whiteningSettings.enableLayout = false;
            setCovarianceWhiteningSettings(whiteningSettings, false);
            if(m_pWhitenButterflyAction) {
                m_pWhitenButterflyAction->setChecked(false);
            }

            rawModel()->clearModel();
            m_pEventWindow->getEventModel()->clearModel();
            m_pAnnotationWindow->getAnnotationModel()->clearModel();
            clearEpochReviewSession();
            {
                QSignalBlocker blocker(m_pVirtualChannelWindow->getVirtualChannelModel());
                m_pVirtualChannelWindow->getVirtualChannelModel()->clearModel();
            }
            m_pVirtualChannelWindow->setAvailableChannelNames(QStringList());
            m_pDataWindow->setVirtualChannels({}, false);

            s_wasmByteArray = fileContent;
            const bool ok = m_pDataWindow->loadFiffBuffer(fileContent, fileName);
            if(ok)
                qInfo() << "Fiff data file" << fileName << "loaded (ChannelDataView).";
            else
                qWarning() << "ERROR loading fiff data file" << fileName;

            if (ok) {
                syncAuxWindowsToFiffInfo(m_pDataWindow->fiffInfo(),
                                         m_pDataWindow->firstSample(),
                                         m_pDataWindow->lastSample());

                QBuffer rawBuffer(&s_wasmByteArray);
                FIFFLIB::FiffRawData raw(rawBuffer);
                MatrixXi events;
                QString detectedEventSource;
                if(!raw.isEmpty()
                   && detectFallbackStimEvents(raw, events, detectedEventSource)
                   && events.rows() > 0) {
                    m_pEventWindow->getEventModel()->setEventMatrix(events, false);
                    statusBar()->showMessage(detectedEventSource, 5000);
                }
            }

            setWindowStatus();
            m_qFileRaw.close();
        }
    };
    QFileDialog::getOpenFileContent("Fiff File (*.fif *.fiff)",  fileContentReady);
#else
    QString filename = QFileDialog::getOpenFileName(this,
                                                    QString("Open fiff data file"),
                                                    QString(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/"),
                                                    tr("fif data files (*.fif)"));

    if(filename.isEmpty())
        return;

    loadRawFile(filename);
#endif
}


//*************************************************************************************************************

void MainWindow::writeFile()
{
    if(!ensureLegacyRawModelLoaded(QStringLiteral("Write FIFF File"))) {
        return;
    }

    QString filename = QFileDialog::getSaveFileName(this,
                                                    QString("Write fiff data file"),
                                                    QString(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/"),
                                                    tr("fif data files (*.fif)"));

    if(filename.isEmpty())
        return;

    if(filename == m_qFileRaw.fileName()) {
        QMessageBox msgBox;
        msgBox.setText("You are trying to write to the file you are currently loading the data from. Please choose another file to write to.");
        msgBox.exec();
        return;
    }

    //Create output file, progress dialog and future watcher
    QFile qFileOutput (filename);
    if(qFileOutput.isOpen())
        qFileOutput.close();

    QFutureWatcher<bool> writeFileFutureWatcher;
    QProgressDialog progressDialog("Writing to fif file...", QString(), 0, 0, this, Qt::Dialog);
    const QSharedPointer<SessionFilter> activeFilter = m_pDataWindow->activeSessionFilter();
    const bool useFilteredWrite = activeFilter && activeFilter->isValid();
    const SessionFilter filterCopy = useFilteredWrite ? *activeFilter : SessionFilter();

    //Connect future watcher and dialog
    connect(&writeFileFutureWatcher, &QFutureWatcher<bool>::finished,
            &progressDialog, &QProgressDialog::reset);

    connect(&progressDialog, &QProgressDialog::canceled,
            &writeFileFutureWatcher, &QFutureWatcher<bool>::cancel);

    if(!useFilteredWrite) {
        connect(rawModel(), &RawModel::writeProgressRangeChanged,
                &progressDialog, &QProgressDialog::setRange);

        connect(rawModel(), &RawModel::writeProgressChanged,
                &progressDialog, &QProgressDialog::setValue);
    }

    //Run the file writing in seperate thread
    writeFileFutureWatcher.setFuture(QtConcurrent::run([this, &qFileOutput, filterCopy, useFilteredWrite](){
        if(useFilteredWrite) {
            return writeFilteredRawFile(m_qFileRaw.fileName(), &qFileOutput, filterCopy);
        }

        return rawModel()->writeFiffData(&qFileOutput);
    }));

    progressDialog.exec();

    writeFileFutureWatcher.waitForFinished();

    if(!writeFileFutureWatcher.future().result())
        qWarning() << "MainWindow: ERROR writing fiff data file" << qFileOutput.fileName();
    else
        qInfo() << "MainWindow: Successfully written to" << qFileOutput.fileName();
}


//*************************************************************************************************************

void MainWindow::loadEvents()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    QString("Open fiff event data file"),
                                                    QString(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/"),
                                                    tr("fif event data files (*-eve.fif);;fif data files (*.fif)"));

    if(filename.isEmpty())
        return;

    loadEventsFile(filename);
}


//*************************************************************************************************************

void MainWindow::saveEvents()
{
    QString filename = QFileDialog::getSaveFileName(this,
                                                    QString("Save fiff event data file"),
                                                    QString(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/"),
                                                    tr("fif event data files (*-eve.fif);;fif data files (*.fif)"));
    if(filename.isEmpty())
        return;

    if(m_qEventFile.isOpen())
        m_qEventFile.close();
    m_qEventFile.setFileName(filename);

    if(m_pEventWindow->getEventModel()->saveEventData(m_qEventFile))
        qInfo() << "Fiff event data file" << filename << "saved.";
    else
        qWarning() << "ERROR saving fiff event data file" << filename;
}

//*************************************************************************************************************

void MainWindow::loadAnnotations()
{
    if(m_qFileRaw.fileName().isEmpty()) {
        QMessageBox::warning(this,
                             "Load Annotations",
                             "Load a raw FIF file before opening annotations.");
        return;
    }

    QString filename = QFileDialog::getOpenFileName(this,
                                                    QString("Open annotation file"),
                                                    QFileInfo(defaultAnnotationFilePath(m_qFileRaw.fileName())).absolutePath(),
                                                    tr("annotation files (*-annot.json *_annot.fif *-annot.fif *.json *.csv *.txt *.fif)"));

    if(filename.isEmpty()) {
        return;
    }

    loadAnnotationsFile(filename);
}

//*************************************************************************************************************

void MainWindow::saveAnnotations()
{
    if(m_pAnnotationWindow->getAnnotationModel()->rowCount() == 0) {
        QMessageBox::warning(this,
                             "Save Annotations",
                             "Create or load at least one annotation before saving.");
        return;
    }

    QString defaultPath = m_qAnnotationFile.fileName();
    if(defaultPath.isEmpty()) {
        defaultPath = defaultAnnotationFilePath(m_qFileRaw.fileName());
    }

    QString filename = QFileDialog::getSaveFileName(this,
                                                    QString("Save annotation file"),
                                                    defaultPath,
                                                    tr("annotation files (*-annot.json *_annot.fif *-annot.fif *.json *.csv *.txt *.fif)"));

    if(filename.isEmpty()) {
        return;
    }

    if(m_qAnnotationFile.isOpen()) {
        m_qAnnotationFile.close();
    }
    m_qAnnotationFile.setFileName(filename);

    if(m_pAnnotationWindow->getAnnotationModel()->saveAnnotationData(m_qAnnotationFile)) {
        qInfo() << "Annotation file" << filename << "saved.";
    } else {
        qWarning() << "ERROR saving annotation file" << filename;
    }

    setWindowStatus();
}

//*************************************************************************************************************

void MainWindow::loadVirtualChannels()
{
    if(m_qFileRaw.fileName().isEmpty()) {
        QMessageBox::warning(this,
                             "Load Virtual Channels",
                             "Load a raw FIF file before opening virtual channels.");
        return;
    }

    const QString filename = QFileDialog::getOpenFileName(this,
                                                          QString("Open virtual-channel file"),
                                                          QFileInfo(defaultVirtualChannelFilePath(m_qFileRaw.fileName())).absolutePath(),
                                                          tr("virtual channel files (*-virtchan.json *.json)"));

    if(filename.isEmpty()) {
        return;
    }

    loadVirtualChannelsFile(filename);
}

//*************************************************************************************************************

void MainWindow::saveVirtualChannels()
{
    if(m_pVirtualChannelWindow->getVirtualChannelModel()->rowCount() == 0) {
        QMessageBox::warning(this,
                             "Save Virtual Channels",
                             "Create or load at least one virtual channel before saving.");
        return;
    }

    QString defaultPath = m_qVirtualChannelFile.fileName();
    if(defaultPath.isEmpty()) {
        defaultPath = defaultVirtualChannelFilePath(m_qFileRaw.fileName());
    }

    const QString filename = QFileDialog::getSaveFileName(this,
                                                          QString("Save virtual-channel file"),
                                                          defaultPath,
                                                          tr("virtual channel files (*-virtchan.json *.json)"));

    if(filename.isEmpty()) {
        return;
    }

    if(m_qVirtualChannelFile.isOpen()) {
        m_qVirtualChannelFile.close();
    }
    m_qVirtualChannelFile.setFileName(filename);

    if(m_pVirtualChannelWindow->getVirtualChannelModel()->saveVirtualChannels(m_qVirtualChannelFile)) {
        qInfo() << "Virtual-channel file" << filename << "saved.";
        setWindowStatus();
    } else {
        qWarning() << "ERROR saving virtual-channel file" << filename;
    }
}


//*************************************************************************************************************

bool MainWindow::loadRawFile(const QString& filename)
{
    if(m_qFileRaw.isOpen())
        m_qFileRaw.close();

    m_qFileRaw.setFileName(filename);
    m_covariance.clear();
    if(m_qCovFile.isOpen())
        m_qCovFile.close();
    m_qCovFile.setFileName(QString());
    if(m_qInverseOperatorFile.isOpen())
        m_qInverseOperatorFile.close();
    m_qInverseOperatorFile.setFileName(QString());
    m_inverseOperator = MNELIB::MNEInverseOperator();
    if(m_qAnnotationFile.isOpen())
        m_qAnnotationFile.close();
    m_qAnnotationFile.setFileName(QString());
    if(m_qVirtualChannelFile.isOpen())
        m_qVirtualChannelFile.close();
    m_qVirtualChannelFile.setFileName(QString());
    m_pAverageWindow->clearNoiseCovariance();
    m_pCovarianceWindow->clearCovariance();
    WhiteningSettings whiteningSettings = covarianceWhiteningSettings();
    whiteningSettings.enableButterfly = false;
    whiteningSettings.enableLayout = false;
    setCovarianceWhiteningSettings(whiteningSettings, false);
    if(m_pWhitenButterflyAction) {
        m_pWhitenButterflyAction->setChecked(false);
    }

    rawModel()->clearModel();
    m_pEventWindow->getEventModel()->clearModel();
    m_pAnnotationWindow->getAnnotationModel()->clearModel();
    clearEpochReviewSession();
    {
        QSignalBlocker blocker(m_pVirtualChannelWindow->getVirtualChannelModel());
        m_pVirtualChannelWindow->getVirtualChannelModel()->clearModel();
    }
    m_pVirtualChannelWindow->setAvailableChannelNames(QStringList());
    m_pDataWindow->setVirtualChannels({}, false);

    // ── ChannelDataView path: demand-paged, opens header only (fast) ──
    const bool ok = m_pDataWindow->loadFiffFile(filename);
    if(ok)
        qInfo() << "Fiff data file" << filename << "loaded (ChannelDataView).";
    else
        qWarning() << "ERROR loading fiff data file" << filename;

    if (ok)
        syncAuxWindowsToFiffInfo(m_pDataWindow->fiffInfo(),
                                 m_pDataWindow->firstSample(),
                                 m_pDataWindow->lastSample());

    if (ok) {
        QString detectedEventSource;
        if(populateEventModelFromRaw(filename, m_pEventWindow->getEventModel(), detectedEventSource)) {
            statusBar()->showMessage(detectedEventSource, 5000);
        }
    }

    if (ok) {
        const QStringList annotationCandidates = defaultAnnotationCandidatePaths(filename);
        for(const QString& annotationCandidate : annotationCandidates) {
            if(QFileInfo::exists(annotationCandidate)) {
                loadAnnotationsFile(annotationCandidate, false);
                break;
            }
        }

        const QString defaultVirtualChannelPath = defaultVirtualChannelFilePath(filename);
        if(QFileInfo::exists(defaultVirtualChannelPath)) {
            loadVirtualChannelsFile(defaultVirtualChannelPath, false);
        }
    }

    setWindowStatus();
    return ok;
}


//*************************************************************************************************************

bool MainWindow::loadEventsFile(const QString& filename)
{
    if(m_qEventFile.isOpen())
        m_qEventFile.close();

    m_qEventFile.setFileName(filename);

    const bool ok = m_pEventWindow->getEventModel()->loadEventData(m_qEventFile);
    if(ok)
        qInfo() << "Fiff event data file" << filename << "loaded.";
    else
        qWarning() << "ERROR loading fiff event data file" << filename;

    setWindowStatus();

    if(ok && !m_pEventWindow->isVisible())
        m_pEventWindow->show();

    return ok;
}

//*************************************************************************************************************

bool MainWindow::loadAnnotationsFile(const QString& filename, bool showWindow)
{
    if(m_qAnnotationFile.isOpen()) {
        m_qAnnotationFile.close();
    }

    m_qAnnotationFile.setFileName(filename);

    const bool ok = m_pAnnotationWindow->getAnnotationModel()->loadAnnotationData(m_qAnnotationFile);
    if(ok) {
        qInfo() << "Annotation file" << filename << "loaded.";
    } else {
        qWarning() << "ERROR loading annotation file" << filename;
    }

    setWindowStatus();

    if(ok && showWindow && !m_pAnnotationWindow->isVisible()) {
        m_pAnnotationWindow->show();
    }

    return ok;
}

//*************************************************************************************************************

bool MainWindow::loadVirtualChannelsFile(const QString& filename, bool showWindow)
{
    if(m_qVirtualChannelFile.isOpen()) {
        m_qVirtualChannelFile.close();
    }

    m_qVirtualChannelFile.setFileName(filename);

    const bool ok = m_pVirtualChannelWindow->getVirtualChannelModel()->loadVirtualChannels(m_qVirtualChannelFile);
    if(ok) {
        qInfo() << "Virtual-channel file" << filename << "loaded.";
    } else {
        qWarning() << "ERROR loading virtual-channel file" << filename;
    }

    setWindowStatus();

    if(ok && showWindow && !m_pVirtualChannelWindow->isVisible()) {
        m_pVirtualChannelWindow->show();
    }

    return ok;
}

//*************************************************************************************************************

bool MainWindow::loadInverseOperatorFile(const QString& filename)
{
    QFile inverseOperatorFile(filename);
    MNELIB::MNEInverseOperator inverseOperator;

    if(!MNELIB::MNEInverseOperator::read_inverse_operator(inverseOperatorFile, inverseOperator)
       || inverseOperator.nsource <= 0
       || inverseOperator.src.isEmpty()) {
        QMessageBox::warning(this,
                             QStringLiteral("Load Inverse Operator"),
                             QStringLiteral("Could not load an inverse operator from %1.").arg(filename));
        return false;
    }

    if(m_qInverseOperatorFile.isOpen()) {
        m_qInverseOperatorFile.close();
    }

    m_qInverseOperatorFile.setFileName(filename);
    m_inverseOperator = inverseOperator;

    setWindowStatus();
    statusBar()->showMessage(QStringLiteral("Loaded inverse operator from %1.")
                                 .arg(QFileInfo(filename).fileName()),
                             4000);
    return true;
}

//*************************************************************************************************************

void MainWindow::loadInverseOperator()
{
    const QString defaultDirectory = !m_qInverseOperatorFile.fileName().isEmpty()
        ? QFileInfo(m_qInverseOperatorFile.fileName()).absolutePath()
        : !m_qFileRaw.fileName().isEmpty()
            ? QFileInfo(m_qFileRaw.fileName()).absolutePath()
            : QString(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/");

    const QString filename = QFileDialog::getOpenFileName(this,
                                                          QStringLiteral("Open inverse-operator file"),
                                                          defaultDirectory,
                                                          tr("fif inverse-operator files (*-inv.fif);;fif data files (*.fif)"));

    if(filename.isEmpty()) {
        return;
    }

    loadInverseOperatorFile(filename);
}

//*************************************************************************************************************

void MainWindow::handleAnnotationRangeSelected(int startSample, int endSample)
{
    if(startSample > endSample) {
        std::swap(startSample, endSample);
    }

    if(startSample == endSample) {
        return;
    }

    const QString defaultLabel = m_qSettings.value("MainWindow/Annotations/defaultLabel",
                                                   QStringLiteral("BAD_manual")).toString();

    bool accepted = false;
    QString label = QInputDialog::getText(this,
                                          QStringLiteral("Add Annotation"),
                                          QStringLiteral("Annotation description"),
                                          QLineEdit::Normal,
                                          defaultLabel,
                                          &accepted).trimmed();

    if(!accepted) {
        return;
    }

    if(label.isEmpty()) {
        label = defaultLabel;
    }

    m_qSettings.setValue("MainWindow/Annotations/defaultLabel", label);

    m_pAnnotationWindow->addAnnotation(startSample, endSample, label);
    if(!m_pAnnotationWindow->isVisible()) {
        showWindow(m_pAnnotationWindow);
    }

    if(m_qAnnotationFile.fileName().isEmpty()) {
        setWindowStatus();
    }
}

//*************************************************************************************************************

void MainWindow::clearEpochReviewSession()
{
    m_epochReviewLists.clear();
    m_epochReviewEventCodes.clear();
    m_epochReviewComments.clear();
    m_pEpochReviewInfo.clear();
    m_epochReviewBaseline = QPair<float,float>(0.0f, 0.0f);

    if(m_pEpochWindow) {
        m_pEpochWindow->setRespectAutoRejects(true);
        m_pEpochWindow->getEpochModel()->clearModel();
    }
}

//*************************************************************************************************************

bool MainWindow::refreshReviewedEvokedSet(const QString& statusMessage)
{
    if(!m_pEpochReviewInfo || m_epochReviewLists.isEmpty()) {
        return false;
    }

    FIFFLIB::FiffEvokedSet evokedSet;
    evokedSet.info = *m_pEpochReviewInfo;

    const bool respectAutoRejects =
        m_pEpochWindow && m_pEpochWindow->getEpochModel()->respectAutoRejects();

    for(int index = 0; index < m_epochReviewLists.size(); ++index) {
        const MNELIB::MNEEpochDataList& epochList = m_epochReviewLists.at(index);
        if(epochList.isEmpty()) {
            continue;
        }

        const Eigen::VectorXi selection = reviewedEpochSelection(epochList, respectAutoRejects);
        if(selection.size() == 0) {
            continue;
        }

        const auto& firstEpoch = epochList.first();
        if(!firstEpoch) {
            continue;
        }

        const int minSamp = static_cast<int>(std::round(firstEpoch->tmin * m_pEpochReviewInfo->sfreq));
        const int maxSamp = static_cast<int>(std::round(firstEpoch->tmax * m_pEpochReviewInfo->sfreq));

        FIFFLIB::FiffEvoked evoked = epochList.average(*m_pEpochReviewInfo,
                                                       minSamp,
                                                       maxSamp,
                                                       selection);
        evoked.comment = index < m_epochReviewComments.size()
            ? m_epochReviewComments.at(index)
            : QString::number(index < m_epochReviewEventCodes.size() ? m_epochReviewEventCodes.at(index) : index);
        evoked.baseline = m_epochReviewBaseline;
        evokedSet.evoked.append(evoked);
    }

    if(m_qEvokedFile.isOpen()) {
        m_qEvokedFile.close();
    }
    m_qEvokedFile.setFileName(QString());

    m_pAverageWindow->getAverageModel()->setEvokedData(evokedSet);
    m_pAverageWindow->setRecomputeAvailable(true);
    setWindowStatus();

    if(!statusMessage.isEmpty()) {
        const QString message = evokedSet.evoked.isEmpty()
            ? QStringLiteral("All reviewed epochs are currently excluded.")
            : statusMessage;
        statusBar()->showMessage(message, 4000);
    }

    if(!evokedSet.evoked.isEmpty()) {
        if(!m_pAverageWindow->isVisible()) {
            m_pAverageWindow->show();
        }
        m_pAverageWindow->raise();
    }

    return !evokedSet.evoked.isEmpty();
}


//*************************************************************************************************************

void MainWindow::computeEvoked()
{
    runEvokedComputation(true);
}


//*************************************************************************************************************

void MainWindow::recomputeEvoked()
{
    runEvokedComputation(false);
}


//*************************************************************************************************************

bool MainWindow::runEvokedComputation(bool promptForSettings)
{
    const QString title = promptForSettings
        ? QStringLiteral("Compute Evoked")
        : QStringLiteral("Recompute Evoked");

    if(m_qFileRaw.fileName().isEmpty() || !QFile::exists(m_qFileRaw.fileName())) {
        QMessageBox::warning(this,
                             title,
                             "Load a raw FIF file before computing evoked responses.");
        return false;
    }

    QFile rawFile(m_qFileRaw.fileName());
    FIFFLIB::FiffRawData raw(rawFile);

    if(raw.isEmpty()) {
        QMessageBox::warning(this,
                             title,
                             QString("Could not open raw data from %1.").arg(m_qFileRaw.fileName()));
        return false;
    }

    MatrixXi events = m_pEventWindow->getEventModel()->getEventMatrix();
    QString eventSourceDescription;

    if(events.rows() > 0) {
        eventSourceDescription = QString("Using %1 event(s) from the event manager.")
                                     .arg(events.rows());
    } else if(!detectFallbackStimEvents(raw, events, eventSourceDescription)) {
        QMessageBox::warning(this,
                             title,
                             "No events were loaded, and no trigger events could be detected from STI 014 or STI 101.");
        return false;
    }

    const QMap<int, int> eventCounts = countEventsByType(events);
    if(eventCounts.isEmpty()) {
        QMessageBox::warning(this,
                             title,
                             "No non-zero event codes were found for averaging.");
        return false;
    }

    QList<int> selectedEventCodes;
    float tmin = -0.1f;
    float tmax = 0.4f;
    bool applyBaseline = true;
    float baselineFrom = -0.1f;
    float baselineTo = 0.0f;
    bool dropRejected = true;

    if(promptForSettings) {
        if(!showComputeEvokedDialog(this,
                                    m_qSettings,
                                    eventCounts,
                                    eventSourceDescription,
                                    selectedEventCodes,
                                    tmin,
                                    tmax,
                                    applyBaseline,
                                    baselineFrom,
                                    baselineTo,
                                    dropRejected)) {
            return false;
        }
    } else {
        selectedEventCodes = evokedEventCodesFromSettings(m_qSettings);
        if(selectedEventCodes.isEmpty()) {
            statusBar()->showMessage(QStringLiteral("No saved evoked setup found. Opening the full evoked dialog instead."),
                                     4000);
            return runEvokedComputation(true);
        }

        const int preStimMs = m_qSettings.value("MainWindow/Averaging/preStimMs", 100).toInt();
        const int postStimMs = m_qSettings.value("MainWindow/Averaging/postStimMs", 400).toInt();
        tmin = -static_cast<float>(preStimMs) / 1000.0f;
        tmax = static_cast<float>(postStimMs) / 1000.0f;
        applyBaseline = m_qSettings.value("MainWindow/Averaging/applyBaseline", true).toBool();
        baselineFrom = static_cast<float>(m_qSettings.value("MainWindow/Averaging/baselineFromMs",
                                                            -preStimMs).toInt()) / 1000.0f;
        baselineTo = static_cast<float>(m_qSettings.value("MainWindow/Averaging/baselineToMs", 0).toInt()) / 1000.0f;
        dropRejected = m_qSettings.value("MainWindow/Averaging/dropRejected", true).toBool();

        QList<int> filteredEventCodes;
        for(int eventCode : selectedEventCodes) {
            if(eventCounts.contains(eventCode)) {
                filteredEventCodes.append(eventCode);
            }
        }

        if(filteredEventCodes.isEmpty()) {
            QMessageBox::information(this,
                                     title,
                                     "The last evoked setup references event codes that are not available for the current data. Choose a new setup.");
            return runEvokedComputation(true);
        }

        selectedEventCodes = filteredEventCodes;
    }

    QMap<QString,double> rejectMap;
    if(dropRejected) {
        rejectMap.insert("grad", 2000e-13);
        rejectMap.insert("mag", 3e-12);
        rejectMap.insert("eeg", 100e-6);
        rejectMap.insert("eog", 150e-6);
    }

    const QPair<float, float> baseline = applyBaseline
        ? QPair<float, float>(baselineFrom, baselineTo)
        : QPair<float, float>(0.0f, 0.0f);

    QApplication::setOverrideCursor(Qt::BusyCursor);
    qApp->processEvents();

    QList<MNELIB::MNEEpochDataList> epochLists;
    QList<int> epochEventCodes;
    QStringList epochComments;
    const QSharedPointer<SessionFilter> activeFilter = m_pDataWindow->activeSessionFilter();
    const bool useSessionFilter = activeFilter && activeFilter->isValid();
    const QMap<QString,double> epochRejectMap = useSessionFilter ? QMap<QString,double>() : rejectMap;

    for(int eventCode : selectedEventCodes) {
        MNELIB::MNEEpochDataList epochList = MNELIB::MNEEpochDataList::readEpochs(raw,
                                                                                   events,
                                                                                   tmin,
                                                                                   tmax,
                                                                                   eventCode,
                                                                                   epochRejectMap);

        if(epochList.isEmpty()) {
            continue;
        }

        if(useSessionFilter) {
            applySessionFilterToEpochList(epochList,
                                          *activeFilter,
                                          raw.info,
                                          rejectMap);
        }

        if(applyBaseline) {
            epochList.applyBaselineCorrection(baseline);
        }

        epochLists.append(epochList);
        epochEventCodes.append(eventCode);
        epochComments.append(QString::number(eventCode));
    }

    QApplication::restoreOverrideCursor();

    if(epochLists.isEmpty()) {
        QMessageBox::warning(this,
                             title,
                             "No evoked responses could be computed. The selected events may have been rejected or out of bounds.");
        return false;
    }

    clearEpochReviewSession();
    m_epochReviewLists = epochLists;
    m_epochReviewEventCodes = epochEventCodes;
    m_epochReviewComments = epochComments;
    m_pEpochReviewInfo = FIFFLIB::FiffInfo::SPtr(new FIFFLIB::FiffInfo(raw.info));
    m_epochReviewBaseline = applyBaseline ? baseline : QPair<float,float>(0.0f, 0.0f);
    {
        QSignalBlocker blocker(m_pEpochWindow->getEpochModel());
        m_pEpochWindow->setRespectAutoRejects(dropRejected);
        m_pEpochWindow->getEpochModel()->setEpochs(m_epochReviewLists,
                                                   m_epochReviewEventCodes,
                                                   raw.first_samp,
                                                   raw.info.sfreq);
    }
    m_pEpochWindow->refreshFromModel();

    if(!m_pEpochWindow->isVisible()) {
        m_pEpochWindow->show();
    }
    m_pEpochWindow->raise();

    const bool refreshed = refreshReviewedEvokedSet(promptForSettings
                                                    ? QStringLiteral("Evoked responses computed.")
                                                    : QStringLiteral("Evoked responses recomputed from the last saved setup."));
    if(!refreshed) {
        QMessageBox::warning(this,
                             title,
                             "All reviewed epochs are currently excluded.");
        return false;
    }

    return true;
}


//*************************************************************************************************************

void MainWindow::saveEvoked()
{
    if(!m_pAverageWindow->getAverageModel()->isFileLoaded()) {
        QMessageBox::warning(this,
                             "Save Evoked",
                             "There is no evoked data to save.");
        return;
    }

    const QString defaultPath = !m_qEvokedFile.fileName().isEmpty()
        ? m_qEvokedFile.fileName()
        : defaultEvokedFilePath(m_qFileRaw.fileName());

    const QString filename = QFileDialog::getSaveFileName(this,
                                                          QString("Save evoked fiff data file"),
                                                          defaultPath,
                                                          tr("fif evoked data files (*-ave.fif);;fif data files (*.fif)"));

    if(filename.isEmpty()) {
        return;
    }

    if(m_qEvokedFile.isOpen()) {
        m_qEvokedFile.close();
    }
    m_qEvokedFile.setFileName(filename);

    if(m_pAverageWindow->getAverageModel()->saveEvokedData(m_qEvokedFile)) {
        qInfo() << "Fiff evoked data file" << filename << "saved.";
        setWindowStatus();
    } else {
        qWarning() << "ERROR saving evoked data file" << filename;
        QMessageBox::warning(this,
                             "Save Evoked",
                             QString("Could not save evoked data to %1.").arg(filename));
    }
}


//*************************************************************************************************************

void MainWindow::computeSourceEstimate()
{
    AverageModel* averageModel = m_pAverageWindow->getAverageModel();
    if(!averageModel || !averageModel->isFileLoaded()) {
        QMessageBox::warning(this,
                             QStringLiteral("Compute Source Estimate"),
                             QStringLiteral("Load or compute an evoked response before exporting source estimates."));
        return;
    }

    if(m_inverseOperator.nsource <= 0 || m_inverseOperator.src.isEmpty()) {
        const QMessageBox::StandardButton answer =
            QMessageBox::question(this,
                                  QStringLiteral("Compute Source Estimate"),
                                  QStringLiteral("No inverse operator is loaded yet. Do you want to load one now?"),
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::Yes);
        if(answer != QMessageBox::Yes) {
            return;
        }

        loadInverseOperator();
        if(m_inverseOperator.nsource <= 0 || m_inverseOperator.src.isEmpty()) {
            return;
        }
    }

    QList<int> selectedRows = m_pAverageWindow->selectedSetRows();
    if(selectedRows.isEmpty()) {
        for(int row = 0; row < averageModel->rowCount(); ++row) {
            selectedRows.append(row);
        }
    }

    QStringList selectedComments;
    for(int row : selectedRows) {
        const FIFFLIB::FiffEvoked* evoked = averageModel->getEvoked(row);
        if(!evoked) {
            continue;
        }

        const QString comment = evoked->comment.trimmed().isEmpty()
            ? QStringLiteral("set_%1").arg(row + 1)
            : evoked->comment.trimmed();
        selectedComments.append(comment);
    }

    if(selectedComments.isEmpty()) {
        QMessageBox::warning(this,
                             QStringLiteral("Compute Source Estimate"),
                             QStringLiteral("Select at least one valid evoked set."));
        return;
    }

    QString method = m_qSettings.value("MainWindow/SourceEstimate/method",
                                       QStringLiteral("dSPM")).toString();
    double snr = m_qSettings.value("MainWindow/SourceEstimate/snr", 3.0).toDouble();
    bool pickNormal = m_qSettings.value("MainWindow/SourceEstimate/pickNormal", false).toBool();
    QString outputDirectory = defaultSourceEstimateDirectory(m_qSettings,
                                                             m_qFileRaw.fileName(),
                                                             m_qEvokedFile.fileName(),
                                                             m_qInverseOperatorFile.fileName());

    const QString inverseDescription = QStringLiteral("Inverse operator: %1 (%2 sources, %3 channels).")
        .arg(m_qInverseOperatorFile.fileName().isEmpty()
                 ? QStringLiteral("loaded in memory")
                 : QFileInfo(m_qInverseOperatorFile.fileName()).fileName())
        .arg(m_inverseOperator.nsource)
        .arg(m_inverseOperator.nchan);

    if(!showComputeSourceEstimateDialog(this,
                                        m_qSettings,
                                        inverseDescription,
                                        selectedComments,
                                        method,
                                        snr,
                                        pickNormal,
                                        outputDirectory)) {
        return;
    }

    QDir outputDir(outputDirectory);
    if(!outputDir.exists() && !QDir().mkpath(outputDirectory)) {
        QMessageBox::warning(this,
                             QStringLiteral("Compute Source Estimate"),
                             QStringLiteral("Could not create the output directory %1.").arg(outputDirectory));
        return;
    }

    const float lambda2 = static_cast<float>(1.0 / (snr * snr));
    const QString datasetStem = defaultSourceEstimateStem(m_qFileRaw.fileName(),
                                                          m_qEvokedFile.fileName(),
                                                          m_qInverseOperatorFile.fileName());

    QStringList writtenFiles;
    QStringList failedSets;
    QSet<QString> usedStems;

    QApplication::setOverrideCursor(Qt::BusyCursor);
    qApp->processEvents();

    for(int row : selectedRows) {
        const FIFFLIB::FiffEvoked* evoked = averageModel->getEvoked(row);
        if(!evoked) {
            continue;
        }

        const QString comment = evoked->comment.trimmed().isEmpty()
            ? QStringLiteral("set_%1").arg(row + 1)
            : evoked->comment.trimmed();

        INVLIB::InvMinimumNorm minimumNorm(m_inverseOperator, lambda2, method);
        INVLIB::InvSourceEstimate sourceEstimate = minimumNorm.calculateInverse(*evoked, pickNormal);

        if(sourceEstimate.isEmpty()) {
            failedSets.append(comment);
            continue;
        }

        sourceEstimate.method = estimateMethodFromString(method);
        sourceEstimate.orientationType = orientationTypeFromInverse(m_inverseOperator);

        const QString commentToken = sanitizeFileToken(comment, QStringLiteral("set_%1").arg(row + 1));
        const QString methodToken = sanitizeFileToken(method, QStringLiteral("dspm"));
        const QString stem = uniqueStem(QStringLiteral("%1_%2_%3")
                                            .arg(datasetStem)
                                            .arg(commentToken)
                                            .arg(methodToken),
                                        usedStems);

        INVLIB::InvSourceEstimate leftHemisphere;
        INVLIB::InvSourceEstimate rightHemisphere;
        if(splitSourceEstimateByHemisphere(sourceEstimate,
                                           m_inverseOperator,
                                           leftHemisphere,
                                           rightHemisphere)) {
            const QString leftPath = outputDir.filePath(stem + QStringLiteral("-lh.stc"));
            const QString rightPath = outputDir.filePath(stem + QStringLiteral("-rh.stc"));

            QFile leftFile(leftPath);
            QFile rightFile(rightPath);

            if(!leftHemisphere.write(leftFile) || !rightHemisphere.write(rightFile)) {
                failedSets.append(comment);
                continue;
            }

            writtenFiles << leftPath << rightPath;
        } else {
            const QString outputPath = outputDir.filePath(stem + QStringLiteral(".stc"));
            QFile outputFile(outputPath);
            if(!sourceEstimate.write(outputFile)) {
                failedSets.append(comment);
                continue;
            }

            writtenFiles << outputPath;
        }
    }

    QApplication::restoreOverrideCursor();

    if(writtenFiles.isEmpty()) {
        QMessageBox::warning(this,
                             QStringLiteral("Compute Source Estimate"),
                             QStringLiteral("No source estimates could be exported. Check that the selected evoked sets match the inverse operator channels."));
        return;
    }

    QStringList previewFiles;
    const int previewCount = std::min(4, static_cast<int>(writtenFiles.size()));
    for(int index = 0; index < previewCount; ++index) {
        previewFiles.append(QFileInfo(writtenFiles.at(index)).fileName());
    }

    QString resultText = QStringLiteral("Exported %1 source-estimate file(s) to %2.")
        .arg(writtenFiles.size())
        .arg(outputDirectory);

    if(!previewFiles.isEmpty()) {
        resultText += QStringLiteral("\n\n%1").arg(previewFiles.join(QStringLiteral("\n")));
        if(writtenFiles.size() > previewFiles.size()) {
            resultText += QStringLiteral("\n...");
        }
    }

    if(!failedSets.isEmpty()) {
        resultText += QStringLiteral("\n\nFailed for: %1").arg(failedSets.join(QStringLiteral(", ")));
    }

    QMessageBox::information(this,
                             QStringLiteral("Compute Source Estimate"),
                             resultText);
    statusBar()->showMessage(QStringLiteral("Exported %1 source-estimate file(s).")
                                 .arg(writtenFiles.size()),
                             4000);
}


//*************************************************************************************************************

void MainWindow::computeCovariance()
{
    if(m_qFileRaw.fileName().isEmpty() || !QFile::exists(m_qFileRaw.fileName())) {
        QMessageBox::warning(this,
                             "Compute Covariance",
                             "Load a raw FIF file before computing a covariance matrix.");
        return;
    }

    QFile rawFile(m_qFileRaw.fileName());
    FIFFLIB::FiffRawData raw(rawFile);

    if(raw.isEmpty()) {
        QMessageBox::warning(this,
                             "Compute Covariance",
                             QString("Could not open raw data from %1.").arg(m_qFileRaw.fileName()));
        return;
    }

    MatrixXi events = m_pEventWindow->getEventModel()->getEventMatrix();
    QString eventSourceDescription;

    if(events.rows() > 0) {
        eventSourceDescription = QString("Using %1 event(s) from the event manager.")
                                     .arg(events.rows());
    } else if(!detectFallbackStimEvents(raw, events, eventSourceDescription)) {
        QMessageBox::warning(this,
                             "Compute Covariance",
                             "No events were loaded, and no trigger events could be detected from STI 014 or STI 101.");
        return;
    }

    const QMap<int, int> eventCounts = countEventsByType(events);
    if(eventCounts.isEmpty()) {
        QMessageBox::warning(this,
                             "Compute Covariance",
                             "No non-zero event codes were found for covariance computation.");
        return;
    }

    QList<int> selectedEventCodes;
    float tmin = -0.2f;
    float tmax = 0.0f;
    bool applyBaseline = false;
    float baselineFrom = -0.2f;
    float baselineTo = 0.0f;
    bool removeMean = true;

    if(!showComputeCovarianceDialog(this,
                                    m_qSettings,
                                    eventCounts,
                                    eventSourceDescription,
                                    selectedEventCodes,
                                    tmin,
                                    tmax,
                                    applyBaseline,
                                    baselineFrom,
                                    baselineTo,
                                    removeMean)) {
        return;
    }

    QApplication::setOverrideCursor(Qt::BusyCursor);
    qApp->processEvents();

    const QSharedPointer<SessionFilter> activeFilter = m_pDataWindow->activeSessionFilter();
    const bool useSessionFilter = activeFilter && activeFilter->isValid();
    FIFFLIB::FiffCov covariance;
    if(useSessionFilter) {
        QList<MNELIB::MNEEpochDataList> epochLists;
        for(int eventCode : selectedEventCodes) {
            MNELIB::MNEEpochDataList epochList = MNELIB::MNEEpochDataList::readEpochs(raw,
                                                                                       events,
                                                                                       tmin,
                                                                                       tmax,
                                                                                       eventCode,
                                                                                       {});
            if(epochList.isEmpty()) {
                continue;
            }

            applySessionFilterToEpochList(epochList,
                                          *activeFilter,
                                          raw.info,
                                          {});

            if(applyBaseline) {
                epochList.applyBaselineCorrection(QPair<float, float>(baselineFrom, baselineTo));
            }

            epochLists.append(epochList);
        }

        covariance = computeCovarianceFromEpochLists(epochLists,
                                                     raw.info,
                                                     removeMean);
    } else {
        covariance = FIFFLIB::FiffCov::compute_from_epochs(raw,
                                                           events,
                                                           selectedEventCodes,
                                                           tmin,
                                                           tmax,
                                                           baselineFrom,
                                                           baselineTo,
                                                           applyBaseline,
                                                           removeMean);
    }

    QApplication::restoreOverrideCursor();

    if(covariance.isEmpty()) {
        QMessageBox::warning(this,
                             "Compute Covariance",
                             "No covariance matrix could be computed. Check the selected events and time window.");
        return;
    }

    m_covariance = covariance;
    if(m_qCovFile.isOpen()) {
        m_qCovFile.close();
    }
    m_qCovFile.setFileName(QString());
    m_pAverageWindow->setNoiseCovariance(m_covariance);
    m_pCovarianceWindow->setCovariance(m_covariance,
                                       QStringLiteral("Computed from %1").arg(QFileInfo(m_qFileRaw.fileName()).fileName()),
                                       FIFFLIB::FiffInfo::SPtr(new FIFFLIB::FiffInfo(raw.info)));

    setWindowStatus();

    QMessageBox::information(this,
                             "Compute Covariance",
                             QString("Computed a %1 x %1 covariance matrix with %2 degrees of freedom.\nUse File > Save Covariance (fif)... to store it.")
                                 .arg(m_covariance.dim)
                                 .arg(m_covariance.nfree));

    if(!m_pCovarianceWindow->isVisible()) {
        m_pCovarianceWindow->show();
    }
    m_pCovarianceWindow->raise();
}


//*************************************************************************************************************

void MainWindow::loadCovariance()
{
    const QString filename = QFileDialog::getOpenFileName(this,
                                                          QString("Open covariance fiff data file"),
                                                          QString(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/"),
                                                          tr("fif covariance data files (*-cov.fif);;fif data files (*.fif)"));

    if(filename.isEmpty()) {
        return;
    }

    QFile covarianceFile(filename);
    FIFFLIB::FiffCov covariance(covarianceFile);

    if(covariance.isEmpty()) {
        QMessageBox::warning(this,
                             "Load Covariance",
                             QString("Could not load covariance data from %1.").arg(filename));
        return;
    }

    if(m_qCovFile.isOpen()) {
        m_qCovFile.close();
    }

    m_qCovFile.setFileName(filename);
    m_covariance = covariance;
    m_pAverageWindow->setNoiseCovariance(m_covariance);
    m_pCovarianceWindow->setCovariance(m_covariance,
                                       QStringLiteral("Loaded from %1").arg(QFileInfo(filename).fileName()),
                                       m_pDataWindow->fiffInfo());

    setWindowStatus();

    if(!m_pCovarianceWindow->isVisible()) {
        m_pCovarianceWindow->show();
    }
    m_pCovarianceWindow->raise();
}


//*************************************************************************************************************

void MainWindow::saveCovariance()
{
    if(m_covariance.isEmpty()) {
        QMessageBox::warning(this,
                             "Save Covariance",
                             "There is no covariance matrix to save.");
        return;
    }

    const QString defaultPath = !m_qCovFile.fileName().isEmpty()
        ? m_qCovFile.fileName()
        : defaultCovFilePath(m_qFileRaw.fileName());

    const QString filename = QFileDialog::getSaveFileName(this,
                                                          QString("Save covariance fiff data file"),
                                                          defaultPath,
                                                          tr("fif covariance data files (*-cov.fif);;fif data files (*.fif)"));

    if(filename.isEmpty()) {
        return;
    }

    if(m_qCovFile.isOpen()) {
        m_qCovFile.close();
    }
    m_qCovFile.setFileName(filename);

    if(m_covariance.save(filename)) {
        qInfo() << "Fiff covariance data file" << filename << "saved.";
        setWindowStatus();
    } else {
        qWarning() << "ERROR saving covariance data file" << filename;
        QMessageBox::warning(this,
                             "Save Covariance",
                             QString("Could not save covariance data to %1.").arg(filename));
    }
}


//*************************************************************************************************************

void MainWindow::applyCommandLineOptions(const QString& rawFile,
                                         const QString& eventsFile,
                                         double highpass,
                                         double lowpass)
{
    if(!rawFile.isEmpty()) {
        if(!QFile::exists(rawFile)) {
            qWarning() << "[mne_browse] --raw: file not found:" << rawFile;
        } else {
            loadRawFile(rawFile);
        }
    }

    if(!eventsFile.isEmpty()) {
        if(!QFile::exists(eventsFile)) {
            qWarning() << "[mne_browse] --events: file not found:" << eventsFile;
        } else {
            loadEventsFile(eventsFile);
        }
    }

    if(highpass >= 0.0 || lowpass >= 0.0) {
        m_pFilterWindow->setFrequencies(highpass, lowpass);
        m_pFilterWindow->applyFilter();
        if(!m_pFilterWindow->isVisible())
            m_pFilterWindow->show();
    }
}


//*************************************************************************************************************

void MainWindow::loadEvoked()
{
    QString filename = QFileDialog::getOpenFileName(this,QString("Open evoked fiff data file"),QString(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/"),tr("fif evoked data files (*-ave.fif);;fif data files (*.fif)"));

    if(filename.isEmpty())
        return;

    clearEpochReviewSession();

    if(m_qEvokedFile.isOpen())
        m_qEvokedFile.close();

    m_qEvokedFile.setFileName(filename);

    if(m_pAverageWindow->getAverageModel()->loadEvokedData(m_qEvokedFile))
        qInfo() << "Fiff evoked data file" << filename << "loaded.";
    else
        qWarning() << "ERROR loading evoked data file" << filename;

    //Update status bar
    setWindowStatus();

    m_pAverageWindow->show();
    m_pAverageWindow->raise();
}


//*************************************************************************************************************

void MainWindow::showWindow(QWidget *window)
{
    //Note: A widget that happens to be obscured by other windows on the screen is considered to be visible.
    if(!window->isVisible())
    {
        window->show();
        window->raise();
    }
    else // if visible raise the widget to be sure that it is not obscured by other windows
        window->hide();
}


//*************************************************************************************************************

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Save window geometry
    m_qSettings.setValue("MainWindow/size", size());
    m_qSettings.setValue("MainWindow/position", pos());
    m_qSettings.setValue("MainWindow/state", saveState());

    // Save dock widget visibility
    m_qSettings.setValue("MainWindow/Docks/eventVisible", m_pEventWindow && m_pEventWindow->isVisible());
    m_qSettings.setValue("MainWindow/Docks/annotationVisible", m_pAnnotationWindow && m_pAnnotationWindow->isVisible());
    m_qSettings.setValue("MainWindow/Docks/filterVisible", m_pFilterWindow && m_pFilterWindow->isVisible());
    m_qSettings.setValue("MainWindow/Docks/averageVisible", m_pAverageWindow && m_pAverageWindow->isVisible());
    m_qSettings.setValue("MainWindow/Docks/scaleVisible", m_pScaleWindow && m_pScaleWindow->isVisible());
    m_qSettings.setValue("MainWindow/Docks/chInfoVisible", m_pChInfoWindow && m_pChInfoWindow->isVisible());
    m_qSettings.setValue("MainWindow/Docks/noiseReductionVisible", m_pNoiseReductionWindow && m_pNoiseReductionWindow->isVisible());
    m_qSettings.setValue("MainWindow/Docks/epochVisible", m_pEpochWindow && m_pEpochWindow->isVisible());
    m_qSettings.setValue("MainWindow/Docks/covarianceVisible", m_pCovarianceWindow && m_pCovarianceWindow->isVisible());

    // Save view toggle states
    if (m_pRemoveDCAction)
        m_qSettings.setValue("MainWindow/View/removeDC", m_pRemoveDCAction->isChecked());
    if (m_pHideBadAction)
        m_qSettings.setValue("MainWindow/View/hideBad", m_pHideBadAction->isChecked());
    if (m_pCrosshairAction)
        m_qSettings.setValue("MainWindow/View/crosshair", m_pCrosshairAction->isChecked());
    if (m_pButterflyAction)
        m_qSettings.setValue("MainWindow/View/butterfly", m_pButterflyAction->isChecked());
    if (m_pScalebarsAction)
        m_qSettings.setValue("MainWindow/View/scalebars", m_pScalebarsAction->isChecked());

    QMainWindow::closeEvent(event);
}
