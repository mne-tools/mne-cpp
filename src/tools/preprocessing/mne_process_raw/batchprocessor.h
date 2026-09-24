//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     batchprocessor.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 * @brief    Main batch processing pipeline for mne_process_raw.
 *           Ported from batch.c (do_batch) by Matti Hamalainen.
 */

#ifndef MNE_PROCESS_RAW_BATCHPROCESSOR_H
#define MNE_PROCESS_RAW_BATCHPROCESSOR_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne_process_description.h>

#include <QString>
#include <QStringList>

//=============================================================================================================
// DEFINE NAMESPACE
//=============================================================================================================

namespace MNEPROCESSRAWAPP
{

using namespace MNELIB;

//=============================================================================================================
/**
 * Main batch processing pipeline. Orchestrates all processing steps:
 * open raw data, attach events, apply projections, save data, compute
 * averages, compute covariance matrices, and create SSP operators.
 *
 * Ported from do_batch() in batch.c (MNE-C).
 */
class BatchProcessor
{
public:
    /**
     * Run the complete batch processing pipeline.
     *
     * @param[in] settings  All processing settings from command line arguments.
     * @return 0 on success, non-zero on failure.
     */
    static int run(const ProcessingSettings &settings);

    /**
     * Compose output file names based on the raw file name and a tag.
     *
     * @param[in] rawName   Raw data file name.
     * @param[in] tag       Output tag suffix.
     * @param[in] stripDir  Strip directory from raw name.
     * @param[out] saveName Composed save file name.
     * @param[out] logName  Composed log file name.
     * @return true on success.
     */
    static bool composeSaveNames(const QString &rawName,
                                 const QString &tag,
                                 bool stripDir,
                                 QString &saveName,
                                 QString &logName);

    /**
     * Write a log string to a log file.
     */
    static bool writeLog(const QString &logFile, const QString &log);
};

} // namespace

#endif // MNE_PROCESS_RAW_BATCHPROCESSOR_H
