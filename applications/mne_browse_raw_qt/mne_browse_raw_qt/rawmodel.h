//=============================================================================================================
/**
* @file     rawmodel.h
* @author   Florian Schlembach <florian.schlembach@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*           Jens Haueisen <jens.haueisen@tu-ilmenau.de>
* @version  1.0
* @date     January, 2014
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Implementation of data model of mne_browse_raw_qt
*
*/

#ifndef RAWMODEL_H
#define RAWMODEL_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

//Qt
#include <QDebug>
#include <QAbstractTableModel>

//MNE
#include <fiff/fiff.h>
#include <mne/mne.h>
#include <fiff/fiff_io.h>

//Eigen
#include <Eigen/Core>
#include <Eigen/SparseCore>

//MNE_BROWSE_RAW_QT
#include <types.h>

//*************************************************************************************************************
// USED NAMESPACES

using namespace MNE_BROWSE_RAW_QT;

using namespace Eigen;
using namespace MNELIB;

//=============================================================================================================

class RawModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    RawModel(QObject *parent);
    RawModel(QIODevice& p_IODevice, QObject *parent);
    int rowCount(const QModelIndex &parent = QModelIndex()) const ;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    /**
    * Load fiff data.
    *
    */
    bool loadFiffData(QIODevice &p_IODevice);

    /**
     * @brief clearModel clears all model's members
     */
    void clearModel();

    //Model settings
    qint32 m_iWindowSize; /**< Length of window to load [in samples] */
    qint32 m_iFiffCursor; /**< Cursor that points to the current position in the fiff data file [in samples] */
    qint32 m_iCurScrollPos; /** the current ScrollPosition in the fiff data file */
    qint32 n_reloadPos; /**< Distance that the current window needs to be off the ends of m_data [in samples] */

    /**
     * @brief sizeOfData
     * @return size of loaded m_data
     */
    qint32 sizeOfData();

    /**
     * @brief getMaxDataValue obtains the maximum value for the underlying channel
     *
     * @param chan number of channel in m_chinfolist
     * @return max data value of m_data for scaling purposes.
     */
    double maxDataValue(qint16 chan) const;

    QSharedPointer<FiffIO> m_pfiffIO; /**< FiffIO objects, which holds all the information of the fiff data (excluding the samples!) */

private:
    //display types
    QList<MatrixXdR> m_data; /**< List that holds the fiff matrix data <n_channels x n_samples> */
    QList<MatrixXdR> m_times; /**< List that holds the time axis [in secs] */
    qint16 m_iCurBlockPosition;

    //Fiff
    QList<FiffChInfo> m_chinfolist; /**< List of FiffChInfo objects that holds the corresponding channels information */

    //methods
    /**
    * Load channel infos to m_chinfolist.
    *
    */
    void loadChInfos();

    void reloadFiffData(bool before);

    bool m_reloaded;

signals:

private slots:
    /**
     * @brief reloadData checks, whether the actual position of the QScrollBar demands for a fiff data reload (depending on m_reloadPos)
     * @param value the position of QScrollBar
     */
    void reloadData(int value);

};

Q_DECLARE_METATYPE(MatrixXdR);
Q_DECLARE_METATYPE(RowVectorPair);


#endif // RAWMODEL_H
