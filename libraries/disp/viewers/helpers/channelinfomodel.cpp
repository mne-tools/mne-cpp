//=============================================================================================================
/**
 * @file     channelinfomodel.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     November, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    The definition for ChannelInfoModel.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "channelinfomodel.h"
#include "mneoperator.h"

#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector3D>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace Eigen;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ChannelInfoModel::ChannelInfoModel(FiffInfo::SPtr& pFiffInfo, QObject *parent)
: QAbstractTableModel(parent)
, m_pFiffInfo(pFiffInfo)
{
    setFiffInfo(m_pFiffInfo);
}

//=============================================================================================================

ChannelInfoModel::ChannelInfoModel(QObject *parent)
: QAbstractTableModel(parent)
, m_pFiffInfo(FiffInfo::SPtr(new FiffInfo))
{
}

//=============================================================================================================

int ChannelInfoModel::rowCount(const QModelIndex & /*parent*/) const
{
    //Return number of stored evoked sets
    if(!m_pFiffInfo->chs.size()==0)
        return m_pFiffInfo->chs.size();
    else
        return 0;
}

//=============================================================================================================

int ChannelInfoModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 13;
}

//=============================================================================================================

QVariant ChannelInfoModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole && role != Qt::TextAlignmentRole)
        return QVariant();

    switch(role) {
        case Qt::TextAlignmentRole:
            return Qt::AlignHCenter + Qt::AlignVCenter;

        case Qt::DisplayRole: {
            //Return the number and description/comment of the fiff evoked data in the set as vertical header
            if(orientation == Qt::Vertical)
                if(section<m_pFiffInfo->chs.size())
                    return QString("Ch %1").arg(section);

            //Return the horizontal header
            if(orientation == Qt::Horizontal) {
                switch(section) {
                    case 0:
                        return QString("%1").arg("Data number");
                        break;

                    case 1:
                        return QString("%1").arg("Original name");
                        break;

                    case 2:
                        return QString("%1").arg("Alias");
                        break;

                    case 3:
                        return QString("%1").arg("Mapped layout name");
                        break;

                    case 4:
                        return QString("%1").arg("Channel kind");
                        break;

                    case 5:
                        return QString("%1").arg("MEG type");
                        break;

                    case 6:
                        return QString("%1").arg("Unit");
                        break;

                    case 7:
                        return QString("%1").arg("Position");
                        break;

                    case 8:
                        return QString("%1").arg("Digitizer (cm)");
                        break;

                    case 9:
                        return QString("%1").arg("Active filter");
                        break;

                    case 10:
                        return QString("%1").arg("Coil Type");
                        break;

                    case 11:
                        return QString("%1").arg("Bad channel");
                        break;

                    case 12:
                        return QString("%1").arg("# of Compensators");
                        break;
                }
            }
        }
    }

    return QVariant();
}

//=============================================================================================================

QVariant ChannelInfoModel::data(const QModelIndex &index, int role) const
{
    if(index.row() >= m_pFiffInfo->chs.size())
        return QVariant();

    if (index.isValid()) {
        //******** first column (channel number - corresponds to row in fiff data matrix) ********
        if(index.column()==0) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole:
                    v.setValue(index.row());
                    return v;

                case ChannelInfoModelRoles::GetChNumber:
                    v.setValue(index.row());
                    return v;

                case Qt::TextAlignmentRole:
                    return Qt::AlignHCenter + Qt::AlignVCenter;
            }
        }//end column check

        //******** second column (original channel name) ********
        if(index.column()==1) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole:
                    v.setValue(QString("%1").arg(m_pFiffInfo->chs.at(index.row()).ch_name));
                    return v;

                case ChannelInfoModelRoles::GetOrigChName:
                    v.setValue(QString("%1").arg(m_pFiffInfo->chs.at(index.row()).ch_name));
                    return v;

                case Qt::TextAlignmentRole:
                    return Qt::AlignHCenter + Qt::AlignVCenter;
            }
        }//end column check

        //******** third column (channel alias) ********
        if(index.column()==2) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole:
                    if(index.row()<m_aliasNames.size())
                        v.setValue(QString("%1").arg(m_aliasNames.at(index.row())));
                    return v;

                case ChannelInfoModelRoles::GetChAlias:
                    if(index.row()<m_aliasNames.size())
                        v.setValue(QString("%1").arg(m_aliasNames.at(index.row())));
                    return v;

                case Qt::TextAlignmentRole:
                    return Qt::AlignHCenter + Qt::AlignVCenter;
            }
        }//end column check

        //******** fourth column (mapped layout channel name) ********
        if(index.column()==3) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole:
                    if(index.row()<m_mappedLayoutChNames.size())
                        v.setValue(QString("%1").arg(m_mappedLayoutChNames.at(index.row())));
                    return v;

                case ChannelInfoModelRoles::GetMappedLayoutChName:
                    if(index.row()<m_mappedLayoutChNames.size())
                        v.setValue(QString("%1").arg(m_mappedLayoutChNames.at(index.row())));
                    return v;

                case Qt::TextAlignmentRole:
                    return Qt::AlignHCenter + Qt::AlignVCenter;
            }
        }//end column check

        //******** fifth column (channel kind - MEG, EEG, etc) ********
        if(index.column()==4) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole:
                    v.setValue(QString("%1").arg(m_pFiffInfo->chs.at(index.row()).kind));
                    return v;

                case ChannelInfoModelRoles::GetChKind:
                    v.setValue(m_pFiffInfo->chs.at(index.row()).kind);
                    return v;

                case Qt::TextAlignmentRole:
                    return Qt::AlignHCenter + Qt::AlignVCenter;
            }
        }//end column check

        //******** sixth column (MEG type) ********
        if(index.column()==5) {
            QVariant v;

            v.setValue(QString("%1").arg("non_MEG"));

            if(m_pFiffInfo->chs.at(index.row()).kind == FIFFV_MEG_CH) {
                qint32 unit = m_pFiffInfo->chs.at(index.row()).unit;
                if(unit == FIFF_UNIT_T_M)
                    v.setValue(QString("MEG_grad"));
                else if(unit == FIFF_UNIT_T)
                    v.setValue(QString("MEG_mag"));
            }

            switch(role) {
                case Qt::DisplayRole:
                    return v;

                case ChannelInfoModelRoles::GetMEGType:
                    return v;

                case Qt::TextAlignmentRole:
                    return Qt::AlignHCenter + Qt::AlignVCenter;
            }
        }//end column check

        //******** seventh column (channel unit) ********
        if(index.column()==6) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole:
                    v.setValue(QString("%1").arg(m_pFiffInfo->chs.at(index.row()).unit));
                    return v;

                case ChannelInfoModelRoles::GetChUnit:
                    v.setValue(m_pFiffInfo->chs.at(index.row()).unit);
                    return v;

                case Qt::TextAlignmentRole:
                    return Qt::AlignHCenter + Qt::AlignVCenter;
            }
        }//end column check

        //******** eigth column (channel layout position) ********
        if(index.column()==7) {
            QVariant v;

            QPointF point = m_layoutMap[m_mappedLayoutChNames.at(index.row())];

            switch(role) {
                case Qt::DisplayRole:
                    v.setValue(QString("(%1|%2)").arg(point.x()).arg(point.y()));
                    return v;

                case ChannelInfoModelRoles::GetChPosition:
                    v.setValue(point);
                    return v;

                case Qt::TextAlignmentRole:
                    return Qt::AlignHCenter + Qt::AlignVCenter;
            }
        }//end column check

        //******** ninth column (channel digitizer position) ********
        if(index.column()==8) {
            QVariant v;

            QVector3D point3D(  m_pFiffInfo->chs.at(index.row()).chpos.r0[0] * 100, //convert to cm
                                m_pFiffInfo->chs.at(index.row()).chpos.r0[1] * 100,
                                m_pFiffInfo->chs.at(index.row()).chpos.r0[2] * 100  );

            switch(role) {
                case Qt::DisplayRole:
                    v.setValue(QString("(%1|%2|%3)").arg(point3D.x()).arg(point3D.y()).arg(point3D.z()));
                    return v;

                case ChannelInfoModelRoles::GetChDigitizer:
                    v.setValue(point3D);
                    return v;

                case Qt::TextAlignmentRole:
                    return Qt::AlignHCenter + Qt::AlignVCenter;
            }
        }//end column check

        //******** tenth column (active channel filter type) ********
        if(index.column()==9) {
            QVariant v;

//            //Check if mne operator is a filter operator
//            QSharedPointer<MNEOperator> operatorPtr = m_assignedOperators.value(index.row(), QSharedPointer<MNEOperator>(new MNEOperator()));
//            QSharedPointer<FilterOperator> filterOperator;

            switch(role) {
                case Qt::DisplayRole: {
                    return v;
                }
//                    if(operatorPtr->m_OperatorType == MNEOperator::FILTER) {
//                        filterOperator = operatorPtr.staticCast<FilterOperator>();
//                    }
//                    else {
//                        v.setValue(QString("%1").arg("none"));
//                        return v;
//                    }

//                    switch(filterOperator->m_Type) {
//                        case FilterOperator::LPF: {
//                            v.setValue(QString("%1 | %2").arg("LP").arg(filterOperator->m_dCenterFreq*m_pFiffInfo->sfreq/2));
//                            return v;
//                        }

//                        case FilterOperator::HPF: {
//                            v.setValue(QString("%1 | %2").arg("HP").arg(filterOperator->m_dCenterFreq*m_pFiffInfo->sfreq/2));
//                            return v;
//                        }

//                        case FilterOperator::BPF: {
//                            double fsample = m_pFiffInfo->sfreq;
//                            double low = (filterOperator->m_dCenterFreq*fsample/2) - (filterOperator->m_dBandwidth*fsample/4); // /4 because we also need to devide by 2 to get the nyquist freq
//                            double high = (filterOperator->m_dCenterFreq*fsample/2) + (filterOperator->m_dBandwidth*fsample/4);
//                            v.setValue(QString("%1 | %2 | %3").arg("BP").arg(low).arg(high));
//                            return v;
//                        }

//                        case FilterOperator::NOTCH: {
//                            double fsample = m_pFiffInfo->sfreq;
//                            double low = (filterOperator->m_dCenterFreq*fsample/2) - (filterOperator->m_dBandwidth*fsample/4);
//                            double high = (filterOperator->m_dCenterFreq*fsample/2) + (filterOperator->m_dBandwidth*fsample/4);
//                            v.setValue(QString("%1 | %2 | %3").arg("NOTCH").arg(low).arg(high));
//                            return v;
//                        }
//                    }
//                }

//                case ChannelInfoModelRoles::GetChActiveFilter: {
//                    if(operatorPtr->m_OperatorType == MNEOperator::FILTER) {
//                        filterOperator = operatorPtr.staticCast<FilterOperator>();
//                    }
//                    else {
//                        v.setValue(QString("%1").arg("none"));
//                        return v;
//                    }

//                    v.setValue(operatorPtr);
//                    return v;
//                }

                case Qt::TextAlignmentRole:
                    return Qt::AlignHCenter + Qt::AlignVCenter;
            }
        }

        //******** eleventh column (coil type) ********
        if(index.column()==10) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole:
                    v.setValue(QString("%1").arg(m_pFiffInfo->chs.at(index.row()).chpos.coil_type));
                    return v;

                case ChannelInfoModelRoles::GetChCoilType:
                    v.setValue(m_pFiffInfo->chs.at(index.row()).chpos.coil_type);
                    return v;

                case Qt::TextAlignmentRole:
                    return Qt::AlignHCenter + Qt::AlignVCenter;
            }
        }//end column check

        //******** twelve column (channel bad) ********
        if(index.column()==11) {
            QVariant v;
            bool isBad = false;
            QString chName = m_pFiffInfo->chs.at(index.row()).ch_name;

            switch(role) {
                case Qt::DisplayRole:
                    isBad = m_pFiffInfo->bads.contains(chName);
                    v.setValue(isBad);
                    return v;

                case ChannelInfoModelRoles::GetIsBad:
                    isBad = m_pFiffInfo->bads.contains(chName);
                    v.setValue(isBad);
                    return v;

                case Qt::TextAlignmentRole:
                    return Qt::AlignHCenter + Qt::AlignVCenter;
            }
        }//end column check

        //******** twelve column (compensators bad) ********
        if(index.column()==12) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole:
                    v.setValue(m_pFiffInfo->comps.size());
                    return v;

                case Qt::TextAlignmentRole:
                    return Qt::AlignHCenter + Qt::AlignVCenter;
            }
        }//end column check
    } // end index.valid() check

    return QVariant();
}

//=============================================================================================================

bool ChannelInfoModel::insertRows(int position, int span, const QModelIndex & parent)
{
    Q_UNUSED(position);
    Q_UNUSED(span);
    Q_UNUSED(parent);

    return true;
}

//=============================================================================================================

bool ChannelInfoModel::removeRows(int position, int span, const QModelIndex & parent)
{
    Q_UNUSED(position);
    Q_UNUSED(span);
    Q_UNUSED(parent);

    return true;
}

//=============================================================================================================

Qt::ItemFlags ChannelInfoModel::flags(const QModelIndex & index) const
{
    Q_UNUSED(index);
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable /*| Qt::ItemIsEditable*/;
}

//=============================================================================================================

bool ChannelInfoModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(index);
    Q_UNUSED(value);
    Q_UNUSED(role);

    return true;
}

//=============================================================================================================

void ChannelInfoModel::setFiffInfo(FiffInfo::SPtr& pFiffInfo)
{
    beginResetModel();

    m_pFiffInfo = pFiffInfo;
    m_aliasNames = m_pFiffInfo->ch_names;
    m_mappedLayoutChNames = m_pFiffInfo->ch_names;

    mapLayoutToChannels();

    endResetModel();

    emit dataChanged(createIndex(0,0), createIndex(rowCount(), columnCount()));
}

//=============================================================================================================

void ChannelInfoModel::assignedOperatorsChanged(const QMap<int,QSharedPointer<MNEOperator> > &assignedOperators)
{
    beginResetModel();

    m_assignedOperators = assignedOperators;

    endResetModel();

    emit dataChanged(createIndex(0,0), createIndex(rowCount(), columnCount()));
}

//=============================================================================================================

void ChannelInfoModel::layoutChanged(const QMap<QString,QPointF> &layoutMap)
{
    beginResetModel();

    m_layoutMap = layoutMap;
    m_aliasNames = m_pFiffInfo->ch_names;
    m_mappedLayoutChNames = m_pFiffInfo->ch_names;

    mapLayoutToChannels();

    endResetModel();

    emit dataChanged(createIndex(0,0), createIndex(rowCount(), columnCount()));
}

//=============================================================================================================

const QStringList & ChannelInfoModel::getMappedChannelsList()
{

    return m_mappedLayoutChNames;
}

//=============================================================================================================

int ChannelInfoModel::getIndexFromOrigChName(QString chName)
{
    return m_pFiffInfo->ch_names.indexOf(chName);
}

//=============================================================================================================

int ChannelInfoModel::getIndexFromMappedChName(QString chName)
{
    return m_mappedLayoutChNames.indexOf(chName);
}

//=============================================================================================================

QStringList ChannelInfoModel::getBadChannelList()
{
    return m_pFiffInfo->bads;
}

//=============================================================================================================

void ChannelInfoModel::mapLayoutToChannels()
{
    //Map channels to layout
    QList<FiffChInfo> channelList = m_pFiffInfo->chs;
    for(int i = 0; i<channelList.size(); i++) {
        //Get current channel information
        FiffChInfo chInfo = channelList.at(i);
        QString chName = chInfo.ch_name;
        QRegExp regExpRemove;
        bool flagOk = false;

        switch(chInfo.kind) {
            case FIFFV_MEG_CH:
                //Scan for MEG string and other characters
                regExpRemove = QRegExp("(MEG|-|_|/|\| )");
                chName.remove(regExpRemove);

                //After cleaning the string try to convert the residual to an int number
                flagOk = false;
                m_mappedLayoutChNames.replace(i, QString("%1 %2").arg("MEG").arg(chName));

                break;

            case FIFFV_EEG_CH: {
                //Scan for EEG string and other characters
                regExpRemove = QRegExp("(EEG|-|_|/|\| )");
                chName.remove(regExpRemove);

                //After cleaning the string try to convert the residual to an int number
                flagOk = false;
                m_mappedLayoutChNames.replace(i, QString("%1 %2").arg("EEG").arg(chName));

                break;
            }
        }
    } //end fiff chs

    emit channelsMappedToLayout(m_mappedLayoutChNames);
}

//=============================================================================================================

void ChannelInfoModel::clearModel()
{
    beginResetModel();

    m_pFiffInfo = FiffInfo::SPtr(new FiffInfo);
    m_layoutMap.clear();
    m_aliasNames.clear();
    m_mappedLayoutChNames.clear();

    endResetModel();

    qDebug("ChannelInfoModel cleared.");
}
