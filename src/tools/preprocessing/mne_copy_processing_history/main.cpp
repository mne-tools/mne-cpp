//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
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
 * @brief    Copy processing history block between FIFF files.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_stream.h>
#include <fiff/fiff_dir_node.h>
#include <fiff/fiff_tag.h>
#include <fiff/fiff_types.h>
#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace UTILSLIB;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

/**
 * Recursively copy all tags from a FIFF directory node into the destination stream.
 */
static bool copyBlock(FiffStream::SPtr &src, FiffStream::SPtr &dst, const FiffDirNode::SPtr &node)
{
    // Start block
    dst->start_block(node->type);

    // Copy all tags in this node
    for (int i = 0; i < node->nent(); ++i) {
        FiffDirEntry::SPtr entry = node->dir[i];
        if (entry->kind == FIFF_BLOCK_START || entry->kind == FIFF_BLOCK_END)
            continue;

        std::unique_ptr<FiffTag> tag;
        if (!src->read_tag(tag, entry->pos)) {
            qWarning("Cannot read tag at pos %lld", static_cast<long long>(entry->pos));
            continue;
        }
        dst->write_tag(tag);
    }

    // Recurse into children
    for (int i = 0; i < node->nchild(); ++i) {
        if (!copyBlock(src, dst, node->children[i]))
            return false;
    }

    // End block
    dst->end_block(node->type);
    return true;
}

//=============================================================================================================

/**
 * Find FIFFB_PROCESSING_HISTORY nodes in the tree.
 */
static QList<FiffDirNode::SPtr> findProcessingHistory(const FiffDirNode::SPtr &node)
{
    QList<FiffDirNode::SPtr> result;
    if (node->type == FIFFB_PROCESSING_HISTORY) {
        result.append(node);
    }
    for (int i = 0; i < node->nchild(); ++i) {
        result.append(findProcessingHistory(node->children[i]));
    }
    return result;
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_copy_processing_history");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Copy processing history block from one FIFF file to another.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption fromOpt("from", "Source FIFF file containing processing history.", "file");
    parser.addOption(fromOpt);

    QCommandLineOption toOpt("to", "Destination FIFF file (will be modified).", "file");
    parser.addOption(toOpt);

    parser.process(app);

    QString fromFile = parser.value(fromOpt);
    QString toFile = parser.value(toOpt);

    if (fromFile.isEmpty()) { qCritical("--from is required."); return 1; }
    if (toFile.isEmpty()) { qCritical("--to is required."); return 1; }

    // Open source file
    QFile srcFile(fromFile);
    FiffStream::SPtr srcStream(new FiffStream(&srcFile));
    if (!srcStream->open()) {
        qCritical("Cannot open source file: %s", qPrintable(fromFile));
        return 1;
    }

    // Find processing history blocks in source
    QList<FiffDirNode::SPtr> histNodes = findProcessingHistory(srcStream->dirtree());
    if (histNodes.isEmpty()) {
        qCritical("No processing history block found in source: %s", qPrintable(fromFile));
        srcStream->close();
        return 1;
    }
    printf("Found %lld processing history block(s) in source.\n",
           static_cast<long long>(histNodes.size()));

    // Open destination file for update
    QFile dstFile(toFile);
    FiffStream::SPtr dstStream = FiffStream::open_update(dstFile);
    if (!dstStream) {
        qCritical("Cannot open destination file for update: %s", qPrintable(toFile));
        srcStream->close();
        return 1;
    }

    // Seek to end of file (before closing tags) and copy all history blocks
    dstStream->device()->seek(dstStream->device()->size());

    for (const FiffDirNode::SPtr &histNode : histNodes) {
        if (!copyBlock(srcStream, dstStream, histNode)) {
            qCritical("Failed to copy processing history block.");
            srcStream->close();
            dstStream->close();
            return 1;
        }
    }

    srcStream->close();
    dstStream->close();

    printf("Successfully copied processing history from %s to %s\n",
           qPrintable(fromFile), qPrintable(toFile));
    return 0;
}
