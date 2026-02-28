//=============================================================================================================
/**
 * @file     batchprocessor.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     February, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * @brief    BatchProcessor class implementation.
 *           Ported from batch.c (do_batch) by Matti Hamalainen.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "batchprocessor.h"

#include <mne/mne_description_parser.h>
#include <mne/mne_averaging.h>

#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_events.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_file.h>

#include <mne/mne.h>

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEPROCESSRAWAPP;
using namespace FIFFLIB;
using namespace MNELIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

bool BatchProcessor::composeSaveNames(const QString &rawName,
                                      const QString &tag,
                                      bool stripDir,
                                      QString &saveName,
                                      QString &logName)
{
    if (tag.isEmpty() || rawName.isEmpty()) {
        qWarning() << "[BatchProcessor::composeSaveNames] Tag or rawname missing.";
        return false;
    }

    QString base = rawName;

    // Strip known suffixes
    if (base.endsWith("_raw.fif", Qt::CaseInsensitive))
        base.chop(8);
    else if (base.endsWith(".fif", Qt::CaseInsensitive))
        base.chop(4);

    // Strip _sss suffix
    if (base.endsWith("_sss", Qt::CaseInsensitive))
        base.chop(4);

    if (stripDir) {
        QFileInfo fi(base);
        base = fi.fileName();
    }

    saveName = base + tag + ".fif";
    logName  = base + tag + ".log";

    return true;
}

//=============================================================================================================

bool BatchProcessor::writeLog(const QString &logFile, const QString &log)
{
    if (logFile.isEmpty() || log.isEmpty())
        return true;

    QFile file(logFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[BatchProcessor::writeLog] Cannot open log file" << logFile;
        return false;
    }

    QTextStream out(&file);
    out << log;
    file.close();

    qInfo() << "[BatchProcessor::writeLog] Log written to" << logFile;
    return true;
}

//=============================================================================================================

int BatchProcessor::run(const ProcessingSettings &settings)
{
    if (settings.rawFiles.isEmpty()) {
        qCritical() << "Raw data file not specified.";
        return 1;
    }

    qInfo() << "\n--- mne_process_raw batch processing ---\n";

    // Storage for grand average computation
    QList<FiffEvokedSet> allAverages;
    QList<FiffCov> allCovariances;

    // Process each raw file
    for (int f = 0; f < settings.rawFiles.size(); ++f) {
        const QString &rawName = settings.rawFiles[f];

        qInfo() << "\n--- Opening" << rawName << "---\n";

        // Open raw data file
        QFile rawFile(rawName);
        FiffRawData raw(rawFile);

        if (raw.info.nchan == 0) {
            qCritical() << "Failed to open raw data file:" << rawName;
            return 1;
        }

        qInfo() << "Opened:" << rawName
                << "(" << raw.info.nchan << "channels,"
                << "sfreq =" << raw.info.sfreq << "Hz )";

        //---------------------------------------------------------------------
        // Step 1: Event detection / loading
        //---------------------------------------------------------------------
        FiffEvents fiffEvents;

        // Try loading events from an event file first
        QString eventInFile;
        if (f < settings.eventFiles.size())
            eventInFile = settings.eventFiles[f];

        if (!eventInFile.isEmpty()) {
            qInfo() << "Loading events from" << eventInFile;
            if (eventInFile.endsWith(".fif", Qt::CaseInsensitive)) {
                QFile evtFile(eventInFile);
                FiffEvents::read_from_fif(evtFile, fiffEvents);
            } else {
                QFile evtFile(eventInFile);
                FiffEvents::read_from_ascii(evtFile, fiffEvents);
            }
        }

        if (fiffEvents.is_empty()) {
            // Detect from trigger channel
            qInfo() << "Detecting events from trigger channel" << settings.digTrigger;
            FiffEvents::detect_from_raw(raw,
                                     fiffEvents,
                                     settings.digTrigger,
                                     settings.digTriggerMask,
                                     true);
        }

        qInfo() << fiffEvents.num_events() << "events found.";

        //---------------------------------------------------------------------
        // Step 2: Save events if requested
        //---------------------------------------------------------------------
        QString eventOutFile;
        if (f < settings.eventsOutFiles.size())
            eventOutFile = settings.eventsOutFiles[f];

        if (!eventOutFile.isEmpty()) {
            qInfo() << "\n--- Saving events to" << eventOutFile << "---\n";
            if (eventOutFile.endsWith(".fif", Qt::CaseInsensitive)) {
                QFile evtOutFile(eventOutFile);
                fiffEvents.write_to_fif(evtOutFile);
            } else {
                QFile evtOutFile(eventOutFile);
                fiffEvents.write_to_ascii(evtOutFile, raw.info.sfreq);
            }
        }

        //---------------------------------------------------------------------
        // Step 3: Load/Create SSP projections
        //---------------------------------------------------------------------
        if (!settings.projFiles.isEmpty() || settings.makeProj) {
            if (settings.makeProj) {
                qInfo() << "\n--- Creating new projection operator ---\n";

                QMap<QString,double> projReject;
                projReject["grad"] = settings.projGradReject;
                projReject["mag"]  = settings.projMagReject;
                projReject["eeg"]  = settings.projEegReject;

                QList<FiffProj> newProjs = MNE::compute_proj(
                    raw, fiffEvents.events,
                    settings.projEvent,
                    settings.projTmin, settings.projTmax,
                    settings.projNGrad, settings.projNMag, settings.projNEeg,
                    projReject);

                if (!newProjs.isEmpty()) {
                    // Replace projections in the raw info
                    raw.info.projs = newProjs;

                    // Save if requested
                    if (!settings.saveProjTag.isEmpty()) {
                        QString projSaveName, projLogName;
                        composeSaveNames(rawName, settings.saveProjTag, settings.saveHere,
                                         projSaveName, projLogName);
                        QFile projOutFile(projSaveName);
                        FiffStream::SPtr pProjStream = FiffStream::start_file(projOutFile);
                        if (pProjStream) {
                            pProjStream->write_proj(newProjs);
                            pProjStream->end_file();
                        }
                    }
                }
            } else {
                // Load projections from files
                QList<FiffProj> loadedProjs;
                for (const QString &projFile : settings.projFiles) {
                    QFile pFile(projFile);
                    FiffStream::SPtr pStream(new FiffStream(&pFile));
                    if (pStream->open()) {
                        QList<FiffDirNode::SPtr> projNodes = pStream->dirtree()->dir_tree_find(FIFFB_PROJ);
                        if (!projNodes.isEmpty()) {
                            QList<FiffProj> fileProjs = pStream->read_proj(projNodes[0]);
                            loadedProjs.append(fileProjs);
                        }
                        pStream->close();
                    }
                }
                if (!loadedProjs.isEmpty()) {
                    raw.info.projs = loadedProjs;
                }
            }

            // Activate/deactivate projections
            if (settings.projOn >= 0) {
                for (int k = 0; k < raw.info.projs.size(); ++k) {
                    raw.info.projs[k].active = (settings.projOn > 0);
                }
                qInfo() << "Projections turned" << (settings.projOn ? "on." : "off.");
            }

            // Report projection status
            if (!raw.info.projs.isEmpty()) {
                qInfo() << "\n--- Projection status ---";
                for (int k = 0; k < raw.info.projs.size(); ++k) {
                    qInfo() << "  " << raw.info.projs[k].desc
                            << (raw.info.projs[k].active ? "[active]" : "[off]");
                }
            }
        }

        //---------------------------------------------------------------------
        // Step 4: Save filtered/decimated raw data
        //---------------------------------------------------------------------
        QString saveFile;
        if (f < settings.saveFiles.size())
            saveFile = settings.saveFiles[f];

        if (!saveFile.isEmpty()) {
            qInfo() << "\n--- Saving data to" << saveFile << "(decim =" << settings.decimation << ") ---\n";
            QFile rawOutFile(saveFile);
            if (!MNE::save_raw(raw, rawOutFile, RowVectorXi(), settings.decimation)) {
                qCritical() << "Failed to save raw data.";
                return 1;
            }
        }

        //---------------------------------------------------------------------
        // Step 5: Compute averages
        //---------------------------------------------------------------------
        QString aveDescFile;
        if (f < settings.aveFiles.size())
            aveDescFile = settings.aveFiles[f];
        else if (!settings.aveFiles.isEmpty())
            aveDescFile = settings.aveFiles.last();

        if (!aveDescFile.isEmpty()) {
            qInfo() << "\n--- Averaging according to" << aveDescFile << "---\n";

            AverageDescription aveDesc;
            if (!MNEDescriptionParser::parseAverageFile(aveDescFile, aveDesc) || aveDesc.categories.isEmpty()) {
                qCritical() << "Failed to parse averaging description file:" << aveDescFile;
                return 1;
            }

            QString aveLog;
            FiffEvokedSet evokedSet = MNEAveraging::computeAverages(raw, aveDesc, fiffEvents.events, aveLog);

            // Report results
            for (int j = 0; j < evokedSet.evoked.size(); ++j) {
                qInfo() << "  " << evokedSet.evoked[j].comment
                        << ":" << evokedSet.evoked[j].nave << "averages";
            }

            // Compose save name
            QString aveSaveName, aveLogName;
            if (!settings.saveAveTag.isEmpty()) {
                composeSaveNames(rawName, settings.saveAveTag, settings.saveHere,
                                 aveSaveName, aveLogName);
            } else if (!aveDesc.filename.isEmpty()) {
                aveSaveName = aveDesc.filename;
            }

            if (!aveSaveName.isEmpty()) {
                evokedSet.save(aveSaveName);
            }
            writeLog(aveLogName, aveLog);

            allAverages.append(evokedSet);
        }

        //---------------------------------------------------------------------
        // Step 6: Compute covariance matrices
        //---------------------------------------------------------------------
        QString covDescFile;
        if (f < settings.covFiles.size())
            covDescFile = settings.covFiles[f];
        else if (!settings.covFiles.isEmpty())
            covDescFile = settings.covFiles.last();

        if (!covDescFile.isEmpty()) {
            qInfo() << "\n--- Computing covariance matrix according to" << covDescFile << "---\n";

            CovDescription covDesc;
            if (!MNEDescriptionParser::parseCovarianceFile(covDescFile, covDesc) || covDesc.defs.isEmpty()) {
                qCritical() << "Failed to parse covariance description file:" << covDescFile;
                return 1;
            }

            // Compute covariance per definition, then combine
            QList<FiffCov> defCovs;
            QString covLog;
            covLog += QString("Computing covariance matrix\n");

            for (int d = 0; d < covDesc.defs.size(); ++d) {
                const CovDefinition &def = covDesc.defs[d];

                // Convert event codes from unsigned to int
                QList<int> eventCodes;
                for (int ec = 0; ec < def.events.size(); ++ec)
                    eventCodes.append(static_cast<int>(def.events[ec]));

                FiffCov defCov = FiffCov::compute_from_epochs(
                    raw, fiffEvents.events, eventCodes,
                    def.tmin, def.tmax,
                    def.bmin, def.bmax,
                    def.doBaseline,
                    covDesc.removeSampleMean,
                    def.ignore,
                    def.delay);

                if (defCov.dim > 0) {
                    defCovs.append(defCov);
                    covLog += QString("  Definition %1: %2 degrees of freedom\n")
                        .arg(d + 1).arg(defCov.nfree);
                }
            }

            // Combine all definitions into a single covariance
            FiffCov cov;
            if (defCovs.size() == 1) {
                cov = defCovs[0];
            } else if (defCovs.size() > 1) {
                cov = FiffCov::computeGrandAverage(defCovs);
            }

            if (cov.dim > 0) {
                // Compose save name
                QString covSaveName, covLogName;
                if (!settings.saveCovTag.isEmpty()) {
                    composeSaveNames(rawName, settings.saveCovTag, settings.saveHere,
                                     covSaveName, covLogName);
                } else if (!covDesc.filename.isEmpty()) {
                    covSaveName = covDesc.filename;
                }

                if (!covSaveName.isEmpty()) {
                    cov.save(covSaveName);
                }
                writeLog(covLogName, covLog);

                allCovariances.append(cov);
            }
        }
    }

    //-------------------------------------------------------------------------
    // Grand averages (if multiple raw files)
    //-------------------------------------------------------------------------
    if (settings.rawFiles.size() > 1) {
        if (!settings.grandAveFile.isEmpty() && !allAverages.isEmpty()) {
            qInfo() << "\n--- Computing grand average ---";
            FiffEvokedSet grandAvg = FiffEvokedSet::computeGrandAverage(allAverages);
            if (!grandAvg.evoked.isEmpty()) {
                grandAvg.save(settings.grandAveFile);
            }
        }

        if (!settings.grandCovFile.isEmpty() && !allCovariances.isEmpty()) {
            qInfo() << "\n--- Computing grand average covariance ---";
            FiffCov grandCov = FiffCov::computeGrandAverage(allCovariances);
            if (grandCov.dim > 0) {
                grandCov.save(settings.grandCovFile);
            }
        }
    }

    qInfo() << "\n--- Complete ---\n";
    return 0;
}
