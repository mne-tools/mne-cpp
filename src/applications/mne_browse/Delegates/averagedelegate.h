//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     averagedelegate.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @date     October, 2014
 * @version  2.1.0
 * @brief Contains the declaration of the AverageDelegate class.
 */

#ifndef AVERAGEDELEGATE_H
#define AVERAGEDELEGATE_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../Utils/types.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QItemDelegate>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBROWSE
//=============================================================================================================

namespace MNEBROWSE
{


//=============================================================================================================
/**
 * DECLARE CLASS AverageDelegate
 */

class AverageDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    AverageDelegate(QObject *parent = 0);

    //=========================================================================================================
    /**
     * Reimplemented virtual functions
     *
     */
    virtual void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;

protected:
    //=========================================================================================================
    /**
     * createPlotPath creates the QPointer path for the average data plot.
     *
     * @param[in] index QModelIndex for accessing associated data and model object.
     * @param[in,out] path The QPointerPath to create for the data plot.
     */
    void createPlotPath(const QModelIndex &index, const QStyleOptionViewItem &option, QPainterPath& path, QList<RowVectorPair>& listPairs) const;
};

} //NAMESPACE

#endif // AVERAGEDELEGATE_H
