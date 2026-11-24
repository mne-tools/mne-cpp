//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     batchprocessor.cpp
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     2004
 * @brief    BatchProcessor class implementation.
 *           Ported from batch.c (do_batch) by Matti Hamalainen.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/log.h>
#include "batchprocessor.h"

#include <mne/mne/mne_description_parser.h>
#include <mne/fiff/fiff_raw_data.h>
#include <mne/fiff/fiff_events.h>
#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_file.h>
#include <mne/mne/mne.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <algorithm>
#include <iterator>
#include <vector>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <optional>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEPROCESSRAWAPP;
using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

bool BatchProcessor::composeSaveNames(std::string_view rawName,
                                      std::string_view tag,
                                      bool stripDir,
                                      std::string& saveName,
                                      std::string& logName)
{
    if (tag.empty() || rawName.empty()) {
        Log::warning("[BatchProcessor::composeSaveNames] Tag or rawname missing.");
        return false;
    }

    const auto stripSuffix = [](std::string_view& name, std::string_view suffix) {
        if (name.size() >= suffix.size() && iequals(name.substr(name.size() - suffix.size()), suffix)) {
            name.remove_suffix(suffix.size());
            return true;
        }
        return false;
    };

    std::string_view base = rawName;

    // Strip known suffixes, then _sss
    if (!stripSuffix(base, "_raw.fif"))
        stripSuffix(base, ".fif");
    stripSuffix(base, "_sss");

    if (stripDir) {
        const std::size_t slash = base.find_last_of('/');
        if (slash != std::string_view::npos)
            base.remove_prefix(slash + 1);
    }

    saveName = std::format("{}{}.fif", base, tag);
    logName = std::format("{}{}.log", base, tag);

    return true;
}

//=============================================================================================================

bool BatchProcessor::writeLog(const std::string& logFile, std::string_view log)
{
    if (logFile.empty() || log.empty())
        return true;

    std::ofstream out(path_from_utf8(logFile));
    if (!out || !(out << log)) {
        Log::warning("[BatchProcessor::writeLog] Cannot open log file {}", logFile);
        return false;
    }

    Log::info("[BatchProcessor::writeLog] Log written to {}", logFile);
    return true;
}

//=============================================================================================================

int BatchProcessor::run(const ProcessingSettings& settings)
{
    if (settings.rawFiles.empty()) {
        Log::critical("Raw data file not specified.");
        return 1;
    }

    Log::info("\n--- mne_process_raw batch processing ---\n");

    // Storage for grand average computation
    std::vector<FiffEvokedSet> allAverages;
    std::vector<FiffCov> allCovariances;

    // Process each raw file
    // The f-th entry of a per-run list, or nothing
    const auto perRun = [](const StringList& list, std::size_t f) {
        return f < list.size() ? list[f] : std::string();
    };
    // The f-th entry of a per-run list, the last one for later runs
    const auto perRunOrLast = [](const StringList& list, std::size_t f) {
        return list.empty() ? std::string() : list[std::min(f, list.size() - 1)];
    };

    for (std::size_t f = 0; f < settings.rawFiles.size(); ++f) {
        const std::string& rawName = settings.rawFiles[f];

        Log::info("\n--- Opening {} ---\n", rawName);

        // Open raw data file
        std::optional<FiffRawData> rawData;
        try {
            rawData.emplace(path_from_utf8(rawName));
        } catch (const std::exception& error) {
            Log::critical("Failed to open raw data file: {} {}", rawName, error.what());
            return 1;
        }
        FiffRawData& raw = *rawData;

        if (raw.info.nchan == 0) {
            Log::critical("Failed to open raw data file: {}", rawName);
            return 1;
        }

        Log::info("Opened: {} ( {} channels, sfreq = {} Hz )", rawName, raw.info.nchan, raw.info.sfreq);

        //---------------------------------------------------------------------
        // Step 1: Event detection / loading
        //---------------------------------------------------------------------
        FiffEvents fiffEvents;

        // Try loading events from an event file first
        const std::string eventInFile = perRun(settings.eventFiles, f);

        if (!eventInFile.empty()) {
            Log::info("Loading events from {}", eventInFile);
            if (auto read = FiffEvents::read(path_from_utf8(eventInFile))) {
                fiffEvents = std::move(*read);
            } else {
                Log::warning("{}", read.error().what());
            }
        }

        if (fiffEvents.empty()) {
            // Detect from trigger channel
            Log::info("Detecting events from trigger channel {}", settings.digTrigger);
            // As in MNE-C (mne_raw_scan_triggers), a mask of 0 means no masking.
            if (auto found = FiffEvents::find_events(raw, settings.digTrigger.empty() ? std::string("STI 014") : settings.digTrigger, settings.digTriggerMask == 0 ? 0xFFFFFFFFu : settings.digTriggerMask)) {
                fiffEvents = std::move(*found);
            } else {
                Log::warning("{}", found.error().what());
            }
        }

        Log::info("{} events found.", fiffEvents.num_events());

        //---------------------------------------------------------------------
        // Step 2: Save events if requested
        //---------------------------------------------------------------------
        const std::string eventOutFile = perRun(settings.eventsOutFiles, f);

        if (!eventOutFile.empty()) {
            Log::info("\n--- Saving events to {} ---\n", eventOutFile);
            if (auto written = fiffEvents.write(path_from_utf8(eventOutFile), raw.info.sfreq); !written) {
                Log::warning("{}", written.error().what());
            }
        }

        //---------------------------------------------------------------------
        // Step 3: Load/Create SSP projections
        //---------------------------------------------------------------------
        if (!settings.projFiles.empty() || settings.makeProj) {
            if (settings.makeProj) {
                Log::info("\n--- Creating new projection operator ---\n");

                const std::map<std::string, double> projReject{
                    {"grad", settings.projGradReject}, {"mag", settings.projMagReject}, {"eeg", settings.projEegReject}};

                std::vector<FiffProj> newProjs = FiffProj::compute_from_raw(
                    raw, fiffEvents.events,
                    settings.projEvent,
                    settings.projTmin, settings.projTmax,
                    settings.projNGrad, settings.projNMag, settings.projNEeg,
                    projReject);

                if (!newProjs.empty()) {
                    // Replace projections in the raw info
                    raw.info.projs = newProjs;

                    // Save if requested
                    if (!settings.saveProjTag.empty()) {
                        std::string projSaveName, projLogName;
                        composeSaveNames(rawName, settings.saveProjTag, settings.saveHere,
                                         projSaveName, projLogName);
                        auto pProjStream = FiffStream::start_file(projSaveName);
                        if (pProjStream) {
                            pProjStream->write_proj(newProjs);
                            pProjStream->end_file();
                        }
                    }
                }
            } else {
                // Load projections from files
                std::vector<FiffProj> loadedProjs;
                for (const std::string& projFile : settings.projFiles) {
                    FiffStream pStream(projFile);
                    if (pStream.open()) {
                        std::vector<std::shared_ptr<FiffDirNode>> projNodes = pStream.dirtree()->dir_tree_find(FIFFB_PROJ);
                        if (!projNodes.empty()) {
                            std::vector<FiffProj> fileProjs = pStream.read_proj(projNodes[0]);
                            std::ranges::move(fileProjs, std::back_inserter(loadedProjs));
                        }
                        pStream.close();
                    }
                }
                if (!loadedProjs.empty()) {
                    raw.info.projs = loadedProjs;
                }
            }

            // Activate/deactivate projections
            if (settings.projOn >= 0) {
                for (auto& proj : raw.info.projs) {
                    proj.active = (settings.projOn > 0);
                }
                Log::info("Projections turned {}", (settings.projOn ? "on." : "off."));
            }

            // Report projection status
            if (!raw.info.projs.empty()) {
                Log::info("\n--- Projection status ---");
                for (const auto& proj : raw.info.projs) {
                    Log::info("  {} {}", proj.desc, (proj.active ? "[active]" : "[off]"));
                }
            }
        }

        //---------------------------------------------------------------------
        // Step 4: Save filtered/decimated raw data
        //---------------------------------------------------------------------
        const std::string saveFile = perRun(settings.saveFiles, f);

        if (!saveFile.empty()) {
            Log::info("\n--- Saving data to {} (decim = {} ) ---\n", saveFile, settings.decimation);
            if (!raw.save(path_from_utf8(saveFile), RowVectorXi(), settings.decimation)) {
                Log::critical("Failed to save raw data.");
                return 1;
            }
        }

        //---------------------------------------------------------------------
        // Step 5: Compute averages
        //---------------------------------------------------------------------
        const std::string aveDescFile = perRunOrLast(settings.aveFiles, f);

        if (!aveDescFile.empty()) {
            Log::info("\n--- Averaging according to {} ---\n", aveDescFile);

            const std::optional<AverageDescription> parsedAve = MNEDescriptionParser::read_average(path_from_utf8(aveDescFile));
            if (!parsedAve || parsedAve->categories.empty()) {
                Log::critical("Failed to parse averaging description file: {}", aveDescFile);
                return 1;
            }
            const AverageDescription& aveDesc = *parsedAve;

            std::string aveLog;
            FiffEvokedSet evokedSet = FiffEvokedSet::computeAverages(raw, aveDesc, fiffEvents.events, aveLog);

            // Report results
            for (int j = 0; j < std::ssize(evokedSet.evoked); ++j) {
                Log::info("  {} : {} averages", evokedSet.evoked[j].comment, evokedSet.evoked[j].nave);
            }

            // Compose save name
            std::string aveSaveName, aveLogName;
            if (!settings.saveAveTag.empty()) {
                composeSaveNames(rawName, settings.saveAveTag, settings.saveHere,
                                 aveSaveName, aveLogName);
            } else if (!aveDesc.filename.empty()) {
                aveSaveName = aveDesc.filename;
            }

            if (!aveSaveName.empty()) {
                if (const auto saved = evokedSet.save(path_from_utf8(aveSaveName)); !saved) {
                    Log::critical("{}", saved.error().what());
                }
            }
            writeLog(aveLogName, aveLog);

            allAverages.push_back(std::move(evokedSet));
        }

        //---------------------------------------------------------------------
        // Step 6: Compute covariance matrices
        //---------------------------------------------------------------------
        const std::string covDescFile = perRunOrLast(settings.covFiles, f);

        if (!covDescFile.empty()) {
            Log::info("\n--- Computing covariance matrix according to {} ---\n", covDescFile);

            const std::optional<CovDescription> parsedCov = MNEDescriptionParser::read_covariance(path_from_utf8(covDescFile));
            if (!parsedCov || parsedCov->defs.empty()) {
                Log::critical("Failed to parse covariance description file: {}", covDescFile);
                return 1;
            }
            const CovDescription& covDesc = *parsedCov;

            // Compute covariance per definition, then combine
            std::vector<FiffCov> defCovs;
            std::string covLog = "Computing covariance matrix\n";

            for (int d = 0; d < std::ssize(covDesc.defs); ++d) {
                const CovDefinition& def = covDesc.defs[d];

                // Convert event codes from unsigned to int
                const std::vector<int> eventCodes(def.events.begin(), def.events.end());

                FiffCov defCov = FiffCov::compute_from_epochs(
                    raw, fiffEvents.events, eventCodes,
                    def.tmin, def.tmax,
                    def.bmin, def.bmax,
                    def.doBaseline,
                    covDesc.removeSampleMean,
                    def.ignore,
                    def.delay,
                    &covDesc.rej);

                if (defCov.dim > 0) {
                    defCovs.push_back(defCov);
                    std::format_to(std::back_inserter(covLog), "  Definition {}: {} degrees of freedom\n", d + 1, defCov.nfree);
                }
            }

            // Combine all definitions into a single covariance
            FiffCov cov;
            if (defCovs.size() == 1) {
                cov = defCovs[0];
            } else if (defCovs.size() > 1) {
                cov = FiffCov::compute_grand_average(defCovs);
            }

            if (cov.dim > 0) {
                // Compose save name
                std::string covSaveName, covLogName;
                if (!settings.saveCovTag.empty()) {
                    composeSaveNames(rawName, settings.saveCovTag, settings.saveHere,
                                     covSaveName, covLogName);
                } else if (!covDesc.filename.empty()) {
                    covSaveName = covDesc.filename;
                }

                if (!covSaveName.empty()) {
                    if (const auto saved = cov.save(path_from_utf8(covSaveName)); !saved) {
                        Log::critical("{}", saved.error().what());
                    }
                }
                writeLog(covLogName, covLog);

                allCovariances.push_back(std::move(cov));
            }
        }
    }

    //-------------------------------------------------------------------------
    // Grand averages (if multiple raw files)
    //-------------------------------------------------------------------------
    if (settings.rawFiles.size() > 1) {
        if (!settings.grandAveFile.empty() && !allAverages.empty()) {
            Log::info("\n--- Computing grand average ---");
            // As in MNE-C (mne_browse_raw/grand_average.c), every run is weighted by its
            // nave and the category keeps its comment.
            FiffEvokedSet grandAvg;
            grandAvg.info = allAverages.front().info;
            for (int j = 0; j < std::ssize(allAverages.front().evoked); ++j) {
                std::vector<FiffEvoked> runs;
                for (const FiffEvokedSet& run : allAverages) {
                    if (j < std::ssize(run.evoked)) {
                        runs.push_back(run.evoked[j]);
                    }
                }
                FiffEvoked avg = FiffEvoked::combine(runs, FiffEvoked::CombineWeights::Nave);
                if (avg.empty()) {
                    Log::critical("Grand average: incompatible data sets for {}", runs.front().comment);
                    return 1;
                }
                avg.comment = runs.front().comment;
                grandAvg.evoked.push_back(avg);
            }
            if (!grandAvg.evoked.empty()) {
                if (const auto saved = grandAvg.save(path_from_utf8(settings.grandAveFile)); !saved) {
                    Log::critical("{}", saved.error().what());
                }
            }
        }

        if (!settings.grandCovFile.empty() && !allCovariances.empty()) {
            Log::info("\n--- Computing grand average covariance ---");
            FiffCov grandCov = FiffCov::compute_grand_average(allCovariances);
            if (grandCov.dim > 0) {
                if (const auto saved = grandCov.save(path_from_utf8(settings.grandCovFile)); !saved) {
                    Log::critical("{}", saved.error().what());
                }
            }
        }
    }

    Log::info("\n--- Complete ---\n");
    return 0;
}
