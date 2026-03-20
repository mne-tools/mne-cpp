//=============================================================================================================
/**
 * @file     batchprocessor.h
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
 * @brief    Main batch processing pipeline for mne_process_raw.
 *           Ported from batch.c (do_batch) by Matti Hamalainen.
 *
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

private:
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
