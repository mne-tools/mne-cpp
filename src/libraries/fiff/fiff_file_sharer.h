//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file fiff_file_sharer.h
 * @since 2026
 * @date  March 2026
 * @brief Memory-mapped FIFF file handle shared between cooperating processes / threads to avoid duplicate reads of large raw recordings.
 *
 * Large continuous FIFF recordings (multi-GB) are routinely opened by
 * several cooperating components at once: the GUI viewer, the realtime
 * processor, the recording dumper. @ref FiffFileSharer wraps a
 * memory-mapped view of the file and a small refcount so each consumer
 * gets a zero-copy @c QByteArray view of the same backing pages, instead
 * of every process pulling the data through its own @c QFile read.
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
 * @brief Refcounted memory-mapped view of a FIFF file shared between cooperating consumers.
 *
 * Each consumer gets a zero-copy @c QByteArray view of the same backing
 * pages. When the last reference goes away the mapping is unmapped.
 * Used to let the GUI, the realtime pipeline and the recording dumper
 * share one open FIFF file without duplicate I/O.
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
     * Copies unfinished fiff file to set directory and appends relevant end tags to copy
     *
     * @param[in] sSourcePath   source file to ber copied
     */
    void copyRealtimeFile(const QString &sSourcePath);

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
     * Called when directory QFileSystemWatcher m_fileWatcher is watching gets updated/changed.
     * Adds next realtime file to be watched to m_fileWatcher
     *
     * @param[in] sPath     Path of directory that was changed.
     */
    void onDirectoryChanged(const QString &sPath);

    //=========================================================================================================
    /**
     * Called when a file QFileSystemWatcher m_fileWatcher is watching gets updated.
     * Emits path of new file and stops watching it.
     *
     * @param sPath
     */
    void onFileChanged(const QString &sPath);

    QFileSystemWatcher      m_fileWatcher;          /**< Watches m_sDirectory for new files. */

    QString                 m_sDirectory;           /**< Directory where files will be saved to / read from. */
    int                     m_iFileIndex;           /**< File counter to give files unique name */

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
