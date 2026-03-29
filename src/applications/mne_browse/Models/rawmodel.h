//=============================================================================================================
/**
 * @file     rawmodel.h
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
 * @brief    Declaration of the RawModel class.
 *
 * RawModel is the legacy table-model-backed raw browser data source. The modern browser path uses
 * FiffBlockReader together with the QRhi-based RawView, but RawModel is still kept for filtering,
 * operator management, and compatibility with older mne_browse subsystems that still depend on the
 * table/delegate architecture.
 *
 *           [1] http://qt-project.org/doc/qt-5/QAbstractTableModel.html
 *           [2] http://qt-project.org/doc/qt-5.0/qtconcurrent/qtconcurrent-index.html
 *
 */

#ifndef RAWMODEL_H
#define RAWMODEL_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../Utils/types.h"
#include "../Utils/filteroperator.h"
#include "../Utils/rawsettings.h"
#include "../Utils/datapackage.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QAbstractTableModel>
#include <QMetaEnum>
#include <QMultiMap>
#include <QBrush>
#include <QBuffer>
#include <QByteArray>
#include <QPalette>
#include <QtConcurrent>
#include <QProgressDialog>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>


//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>
#include <fiff/fiff_io.h>
#include <mne/mne.h>
#include <dsp/parksmcclellan.h>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace MNELIB;
using namespace FIFFLIB;



//*************************************************************************************************************
//=============================================================================================================
// Forward Declarations
//=============================================================================================================

namespace FIFFLIB
{
    class FiffIO;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBROWSE
//=============================================================================================================

namespace MNEBROWSE
{


//=============================================================================================================
/**
 * @brief Legacy table-model-based raw data model used by compatibility workflows in mne_browse.
 */
class RawModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    //=========================================================================================================
    /**
     * Constructs an empty raw model.
     *
     * @param[in] parent    Parent QObject.
     */
    RawModel(QObject *parent);

    //=========================================================================================================
    /**
     * Constructs a raw model and immediately loads the given FIFF file.
     *
     * @param[in,out] qFile  Open FIFF file handle.
     * @param[in] parent     Parent QObject.
     */
    RawModel(QFile& qFile, QObject *parent);

    //=========================================================================================================
    /**
     * Returns the number of rows currently exposed by the legacy table model.
     *
     * @param[in] parent    Parent index supplied by Qt.
     * @return Number of visible channels.
     */
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const ;

    //=========================================================================================================
    /**
     * Returns the number of columns exposed by the legacy table model.
     *
     * @param[in] parent    Parent index supplied by Qt.
     * @return Number of model columns.
     */
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;

    //=========================================================================================================
    /**
     * Returns data for one legacy table-model cell.
     *
     * @param[in] index     Requested model index.
     * @param[in] role      Requested Qt role.
     * @return Cell data for the requested role.
     */
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    //=========================================================================================================
    /**
     * Returns header data for the legacy raw model.
     *
     * @param[in] section       Header section index.
     * @param[in] orientation   Header orientation.
     * @param[in] role          Requested Qt role.
     * @return Header value for the requested role.
     */
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    //=========================================================================================================
    /**
     * Loads a FIFF raw file into the legacy model.
     *
     * @param[in] qFile   Device that provides the FIFF raw file content.
     * @return True on success.
     */
    bool loadFiffData(QIODevice *qFile);

    //=========================================================================================================
    /**
     * Writes the current legacy model state back to a FIFF file.
     *
     * @param[in] p_IODevice  Destination device.
     * @return True on success.
     */
    bool writeFiffData(QIODevice *p_IODevice);

    //=========================================================================================================
    /** Returns true when a Fiff file is loaded. */
    bool isFileLoaded() const { return m_bFileloaded; }

    //=========================================================================================================
    /** Returns the list of channel info objects for the loaded file. */
    const QList<FiffChInfo>& channelInfoList() const { return m_chInfolist; }

    //=========================================================================================================
    /** Returns the FiffInfo shared pointer for the loaded file. */
    FiffInfo::SPtr fiffInfo() const { return m_pFiffInfo; }

    //=========================================================================================================
    /**
     * Returns the FIFF unit code for a given channel row.
     * Needed by delegates to distinguish gradiometers (FIFF_UNIT_T_M) from magnetometers.
     *
     * @param row  Channel index.
     * @return     FIFF unit code, or FIFF_UNIT_NONE if not available.
     */
    inline qint32 channelUnit(int row) const;

    //=========================================================================================================
    /** Returns the operator map (non-const for filter application). */
    QMap<QString,QSharedPointer<MNEOperator> >& operators() { return m_Operators; }
    const QMap<QString,QSharedPointer<MNEOperator> >& operators() const { return m_Operators; }

    //=========================================================================================================
    /**
     * Clears all model state and resets the legacy browser backend.
     */
    void clearModel();

private:
    //=========================================================================================================
    /**
     * Creates the standard filter operators used by the legacy browser path.
     */
    void genStdFilterOps();

    //=========================================================================================================
    /**
     * Extracts measurement metadata from the currently loaded raw FIFF file.
     */
    void loadFiffInfos();

    //=========================================================================================================
    /**
     * Repositions the absolute FIFF cursor so the requested scroll position can be loaded.
     *
     * @param[in] position  Absolute sample position that should become visible.
     */
    void resetPosition(qint32 position);

    //=========================================================================================================
    /**
     * Loads another raw-data block either before or after the currently buffered data.
     *
     * @param[in] before    True to load a block before the current buffer, false to load after it.
     */
    void reloadFiffData(bool before);

    //=========================================================================================================
    /**
     * Reads a raw sample segment from the currently loaded FIFF file.
     *
     * @param[in] from  First sample to read.
     * @param[in] to    Last sample to read.
     * @return Pair of data matrix and time matrix.
     */
    QPair<MatrixXd,MatrixXd> readSegment(fiff_int_t from, fiff_int_t to);

    bool                                        m_bFileloaded;   /**< True when a FIFF raw file is loaded. */
    QList<FiffChInfo>                           m_chInfolist;    /**< Cached channel metadata for the loaded raw file. */
    FiffInfo::SPtr                              m_pFiffInfo;     /**< Shared measurement info of the loaded raw file. */
    QSharedPointer<FIFFLIB::FiffIO>             m_pfiffIO;       /**< Legacy FIFF I/O backend used by the table model path. */
    QSharedPointer<QIODevice>                   m_pSourceDevice; /**< Persistent source device used by the legacy raw model. */
    QByteArray                                  m_sourceBuffer;  /**< In-memory copy for non-file-backed source devices. */
    QMap<QString,QSharedPointer<MNEOperator> >  m_Operators;     /**< Available processing operators keyed by operator name. */

    bool                                        m_bStartReached;         /**< True when the buffered window touches the file start. */
    bool                                        m_bEndReached;           /**< True when the buffered window touches the file end. */
    bool                                        m_bReloadBefore;         /**< True if the most recent reload prepended data. */

    QFutureWatcher<QPair<MatrixXd,MatrixXd> > m_reloadFutureWatcher;    /**< QFutureWatcher for watching process of reloading fiff data. */
    bool                                     m_bReloading;               /**< True while a background reload is in progress. */

//    QFutureWatcher<QPair<int,RowVectorXd> > m_operatorFutureWatcher; /**< QFutureWatcher for watching process of applying Operators to reloaded fiff data. */
    QFutureWatcher<void>                    m_operatorFutureWatcher;    /**< QFutureWatcher for watching process of applying Operators to reloaded fiff data. */
    QList<QPair<int,RowVectorXd> >          m_listTmpChData;            /**< Temporary per-channel processing results. */
    bool                                    m_bProcessing;              /**< True while operator processing runs in the background. */
    QString                                 m_filterChType;             /**< Channel-name filter applied to bulk operator updates. */

    QMutex                                  m_Mutex;                    /**< Guards shared state against concurrent background access. */

    QList<QSharedPointer<DataPackage> >     m_data;                     /**< Buffered raw-data packages currently cached in memory. */

    QMultiMap<int,QSharedPointer<MNEOperator> >  m_assignedOperators;   /**< Processing operators assigned per channel row. */

    qint32                                  m_iAbsFiffCursor;           /**< Current absolute FIFF cursor position in samples. */
    qint32                                  m_iCurAbsScrollPos;         /**< Current absolute scroll position in samples. */

    qint32                                  m_iWindowSize;              /**< Length of one buffered data window in samples. */
    qint32                                  m_reloadPos;                /**< Lookahead threshold that triggers another reload in samples. */
    qint8                                   m_maxWindows;               /**< Maximum number of buffered windows retained in memory. */
    qint16                                  m_iFilterTaps;              /**< Number of taps used by legacy FIR filter operators. */
    int                                     m_iCurrentFFTLength;        /**< FFT length used by overlap-add based filtering. */

signals:
    //=========================================================================================================
    /**
     * Emitted after a background data reload has completed.
     */
    void dataReloaded();

    //=========================================================================================================
    /**
     * Emitted after a raw FIFF file has been loaded successfully.
     *
     * @param[in]  FiffInfo  Measurement info of the loaded file.
     */
    void fileLoaded(FiffInfo::SPtr&);

    //=========================================================================================================
    /**
     * Emitted whenever operator assignments change.
     *
     * @param[in]  assignedOperators  Current per-channel operator assignments.
     */
    void assignedOperatorsChanged(const QMultiMap<int,QSharedPointer<MNEOperator> >&);

    void writeProgressChanged(int);

    void writeProgressRangeChanged(int,int);

public slots:
    //=========================================================================================================
    /**
     * Updates the legacy model scroll position and triggers reloads when needed.
     *
     * @param[in] value  Absolute scroll-bar position in samples.
     */
    void updateScrollPos(int value);

    //=========================================================================================================
    /**
     * Marks selected channels as bad or good in the cached FIFF metadata.
     *
     * @param[in] chlist   Selected channel rows.
     * @param[in] status   True to mark channels as bad, false to clear the bad flag.
     */
    void markChBad(QModelIndexList chlist, bool status);

    //=========================================================================================================
    /**
     * Applies an operator to matching channels whose names contain a given type string.
     *
     * @param[in] chlist       Selected channel rows.
     * @param[in] operatorPtr  Operator to assign and apply.
     * @param[in] chType       Name fragment channels must contain to be processed.
     */
    void applyOperator(QModelIndexList chlist, const QSharedPointer<MNEOperator>& operatorPtr, const QString &chType);

    //=========================================================================================================
    /**
     * Applies an operator to the selected channels.
     *
     * @param[in] chlist       Selected channel rows.
     * @param[in] operatorPtr  Operator to assign and apply.
     */
    void applyOperator(QModelIndexList chlist, const QSharedPointer<MNEOperator> &operatorPtr);

    //=========================================================================================================
    /**
     * Applies all assigned operators to one channel vector in place.
     *
     * @param[in,out] chdata  Pair of channel row index and mutable channel samples.
     */
    void applyOperatorsConcurrently(QPair<int, RowVectorXd> &chdata) const;

    //=========================================================================================================
    /**
     * Reprocesses one channel according to the currently assigned operators.
     *
     * @param[in] chan  Channel row to update.
     */
    void updateOperators(QModelIndex chan);

    //=========================================================================================================
    /**
     * Reprocesses a list of channels according to the current operator assignments.
     *
     * @param[in] chlist  Channel rows to update.
     */
    void updateOperators(QModelIndexList chlist);

    /**
     * Reprocesses all buffered channels according to the current operator assignments.
     */
    void updateOperators();

    //=========================================================================================================
    /**
     * Removes one filter operator from the selected channels.
     *
     * @param[in] chlist      Selected channel rows.
     * @param[in] filterPtr   Filter operator to remove.
     */
    void undoFilter(QModelIndexList chlist, const QSharedPointer<MNEOperator> &filterPtr);

    //=========================================================================================================
    /**
     * Removes all filter operators from the selected channels.
     *
     * @param[in] chlist  Selected channel rows.
     */
    void undoFilter(QModelIndexList chlist);

    //=========================================================================================================
    /**
     * Removes all filter operators from channels whose names contain a given type string.
     *
     * @param[in] chType  Name fragment channels must contain.
     */
    void undoFilter(const QString &chType);

    //=========================================================================================================
    /**
     * Removes all filter operators from all channels.
     */
    void undoFilter();

    //=========================================================================================================
    /**
     * Updates the active SSP projection matrix in the legacy model.
     */
    void updateProjections();

    //=========================================================================================================
    /**
     * Update the compensator
     *
     * @param[in] to    Compensator to use in fiff constant format FiffCtfComp.kind (NOT FiffCtfComp.ctfkind)
     */
    void updateCompensator(int to);

private slots:
    //=========================================================================================================
    /**
     * Inserts data that has just been loaded in the background.
     *
     * @param[in] dataTimesPair  Reloaded data and time matrices.
     */
    void insertReloadedData(QPair<MatrixXd,MatrixXd> dataTimesPair);

    //=========================================================================================================
    /**
     * Starts background operator processing for all buffered data.
     */
    void updateOperatorsConcurrently();

    //=========================================================================================================
    /**
     * Starts background operator processing for one buffered window.
     *
     * @param[in] windowIndex  Buffered window index to process.
     */
    void updateOperatorsConcurrently(int windowIndex);

    //=========================================================================================================
    /**
     * Inserts a processed channel row after background processing finishes.
     *
     * @param[in] rowIndex  Channel row index inside the buffered data window.
     */
    void insertProcessedDataRow(int rowIndex);

    //=========================================================================================================
    /**
     * Inserts all processed data for one buffered window.
     *
     * @param[in] windowIndex  Buffered window index.
     */
    void insertProcessedDataAll(int windowIndex);

    //=========================================================================================================
    /**
     * Inserts processed data for the most recently reloaded buffer segment.
     */
    void insertProcessedDataAll();

    //=========================================================================================================
    /**
     * Applies overlap-add post-processing to all buffered windows.
     */
    void performOverlapAdd();

    //=========================================================================================================
    /**
     * Applies overlap-add post-processing to one buffered window.
     *
     * @param[in] windowIndex  Buffered window index.
     */
    void performOverlapAdd(int windowIndex);

public:
    //=========================================================================================================
    /**
     * Returns the total sample span of the loaded raw FIFF file.
     *
     * @return Total number of samples covered by the loaded file.
     */
    inline qint32 sizeOfFiffData();

    //=========================================================================================================
    /**
     * Returns the first sample of the loaded raw FIFF file.
     *
     * @return First file sample.
     */
    inline qint32 firstSample() const;

    //=========================================================================================================
    /**
     * Returns the last sample of the loaded raw FIFF file.
     *
     * @return Last file sample.
     */
    inline qint32 lastSample() const;

    //=========================================================================================================
    /**
     * Returns the number of samples currently buffered in memory.
     *
     * @return Total buffered sample count.
     */
    inline qint32 sizeOfPreloadedData() const;

    //=========================================================================================================
    /**
     * Returns the current cursor relative to the file start.
     *
     * @return Relative cursor position in samples.
     */
    inline qint32 relFiffCursor() const;

    //=========================================================================================================
    /**
     * Returns the current absolute FIFF cursor position.
     *
     * @return Absolute cursor position in samples.
     */
    inline qint32 absFiffCursor() const;
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline qint32 RawModel::sizeOfFiffData() {
    if(!m_pfiffIO->m_qlistRaw.empty())
        return (m_pfiffIO->m_qlistRaw[0]->last_samp-m_pfiffIO->m_qlistRaw[0]->first_samp);
    else return 0;
}


//*************************************************************************************************************

inline qint32 RawModel::firstSample() const {
    if(!m_pfiffIO->m_qlistRaw.empty())
        return m_pfiffIO->m_qlistRaw[0]->first_samp;
    else return 0;
}


//*************************************************************************************************************

inline qint32 RawModel::lastSample() const {
    if(!m_pfiffIO->m_qlistRaw.empty())
        return m_pfiffIO->m_qlistRaw[0]->last_samp;
    else return 0;
}


//*************************************************************************************************************

inline qint32 RawModel::sizeOfPreloadedData() const {
    if(!m_data.empty()) {
        return m_data.size()*m_iWindowSize;
    }
    else return 0;
}


//*************************************************************************************************************

inline qint32 RawModel::relFiffCursor() const {
    return (m_iAbsFiffCursor - m_pfiffIO->m_qlistRaw[0]->first_samp);
}


//*************************************************************************************************************

inline qint32 RawModel::absFiffCursor() const {
    return m_iAbsFiffCursor;
}


//*************************************************************************************************************

inline qint32 RawModel::channelUnit(int row) const {
    if(!m_pfiffIO || m_pfiffIO->m_qlistRaw.isEmpty())
        return FIFF_UNIT_NONE;
    return m_pfiffIO->m_qlistRaw[0]->info.chs[row].unit;
}

} // NAMESPACE

#endif // RAWMODEL_H
