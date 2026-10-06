//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fiff_file_sharer.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Hands finished FIFF recordings from one application to another through a watched directory.
 *
 * The producer (e.g. MNE Scan's file writer) calls @ref FIFFLIB::FiffFileSharer::copyRealtimeFile, which copies a
 * recording into the shared directory as @c realtime_file<N>_raw.fif and closes its raw block so it is a complete
 * FIFF file. The consumer (e.g. MNE Analyze) calls @ref FIFFLIB::FiffFileSharer::initWatcher and receives
 * @c newFileAtPath once the next file has been written.
 */

#ifndef FIFF_FILE_SHARER_H
#define FIFF_FILE_SHARER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_io.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFileSystemWatcher>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
/**
 * @brief Copies FIFF recordings into a shared directory and signals their arrival to a watching consumer.
 *
 * @snippet ex_fiff_structure/main.cpp fiff_file_sharer_usage
 */
class FIFFSHARED_EXPORT FiffFileSharer : public QObject
{
    Q_OBJECT
public:
    //=========================================================================================================
    /**
     * Constructs a FiffFileSharer with default paramaters (based on m_sDefaultDirectory and m_sDefaultFileName)
     */
    FiffFileSharer();

    //=========================================================================================================
    /**
     * Constructs a FiffFileSharer to watch/save to sDirName
     *
     * @param[in] sDirName      Directory to/from which data will saved/read
     */
    FiffFileSharer(const QString& sDirName);

    //=========================================================================================================
    /**
     * Copies an unfinished FIFF file into the shared directory and closes its raw block; the copy appears
     * there only once it is complete
     *
     * @param[in] sSourcePath   source file to ber copied
     */
    void copyRealtimeFile(const QString& sSourcePath);

    //=========================================================================================================
    /**
     * Sets member QFileSystemWatcher to watch set directory
     */
    void initWatcher();

private:
    //=========================================================================================================
    /**
     * Creates specified shared directory if it does not exist.
     *
     * @return Returns whether shared directory exists in file structure.
     */
    bool initSharedDirectory();

    //=========================================================================================================
    /**
     * Clears shared directory of old shared files.
     */
    void clearSharedDirectory();

    //=========================================================================================================
    /**
     * Called when the watched directory changes; emits newFileAtPath for every new complete file.
     *
     * @param[in] sPath     Path of directory that was changed.
     */
    void onDirectoryChanged(const QString& sPath);

    QFileSystemWatcher m_fileWatcher; /**< Watches m_sDirectory for new files. */

    QString m_sDirectory; /**< Directory where files will be saved to / read from. */
    int m_iFileIndex;     /**< File counter to give files unique name */

signals:

    //=========================================================================================================
    /**
     * Emits path of new shared fiff file.
     *
     * @param[in] sPath     Path of new shared fiff file.
     */
    void newFileAtPath(const QString& sPath);
};
} //namespace

#endif // FIFF_FILE_SHARER_H
