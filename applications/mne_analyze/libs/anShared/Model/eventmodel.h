//=============================================================================================================
/**
 * @file     eventmodel.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Gabriel Motta <gbmotta@mgh.harvard.edu>;
 *           Juan Garcia-Prieto <juangpc@gmail.com>
 * @since    0.1.9
 * @date     March, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Christoph Dinh, Lorenz Esch, Gabriel Motta, Juan Garcia-Prieto. All rights reserved.
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
 * @brief    Declaration of the EventModel Class.
 *
 */

#ifndef ANSHAREDLIB_EVENTMODEL_H
#define ANSHAREDLIB_EVENTMODEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../anshared_global.h"
#include "../Utils/types.h"
#include "abstractmodel.h"

#include <events/eventmanager.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QColor>
#include <QListWidgetItem>
#include <QStack>

//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//=============================================================================================================
// DEFINE NAMESPACE ANSHAREDLIB
//=============================================================================================================

namespace ANSHAREDLIB {

//=============================================================================================================
// ANSHAREDLIB FORWARD DECLARATIONS
//=============================================================================================================

class FiffRawViewModel;

//=============================================================================================================
/**
 * Model that holds the event information associated with a fiff file.
 */
class ANSHAREDSHARED_EXPORT EventModel : public AbstractModel
{
    Q_OBJECT

public:
    typedef QSharedPointer<EventModel> SPtr;              /**< Shared pointer type for EventModel. */
    typedef QSharedPointer<const EventModel> ConstSPtr;   /**< Const shared pointer type for EventModel. */

    //=========================================================================================================
    /**
     * Constructs an event model
     *
     * @param[in] parent   QObject parent of the model.
     */
    EventModel(QObject* parent = Q_NULLPTR);

    //=========================================================================================================

    EventModel(QSharedPointer<FiffRawViewModel> pFiffModel,
                    QObject* parent = Q_NULLPTR);

    //=========================================================================================================

    EventModel(const QString &sFilePath,
               const QByteArray& byteLoadedData = QByteArray(),
               float fSampFreq = 600,
               int iFirstSampOffst = 0,
               QObject* parent = Q_NULLPTR);

    //=========================================================================================================
    /**
     * Destructs an event model.
     */
    ~EventModel();

    //=========================================================================================================
    /**
     * Inserts span rows at position
     *
     * @param[in] position     where to insert rows.
     * @param[in] span         how many rows to insert.
     * @param[in] parent       parent of inserted rows (unused).
     *
     * @return  returns true if successful.
     */
    bool insertRows(int position,
                    int span,
                    const QModelIndex & parent) override;

    //=========================================================================================================
    /**
     * Removes span rows at position
     *
     * @param[in] position     where to remove rows.
     * @param[in] span         how many rows to remove.
     * @param[in] parent       parent of inserted rows (unused).
     *
     * @return  returns true if successful.
     */
    bool removeRows(int position,
                    int span,
                    const QModelIndex & parent = QModelIndex()) override;

    //=========================================================================================================
    /**
     * Returns the number of rows in the model
     *
     * @param[in] parent     The parent index.
     */
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    //=========================================================================================================
    /**
     * Returns the number of columns in the model
     *
     * @param[in] parent     The parent index.
     */
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    //=========================================================================================================
    /**
     * Returns the data stored under the given role for the index.
     *
     * @param[in] index   The index that referres to the requested item.
     * @param[in] role    The requested role.
     */
    virtual QVariant data(const QModelIndex &index,
                          int role = Qt::DisplayRole) const override;

    //=========================================================================================================
    /**
     * Returns the data for the given role and section in the header with the specified orientation.
     *
     * @param[in] section        For horizontal headers, the section number corresponds to the column number. Similarly, for vertical headers, the section number corresponds to the row number.
     * @param[in] orientation    Qt::Horizontal or Qt::Vertical.
     * @param[in] role           role to show.
     *
     * @return accessed eader data.
     */
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role) const override;

    //=========================================================================================================
    /**
     * Sets index to value based on role
     *
     * @param[in] index    model index to which the data will be set.
     * @param[in] value    data to be set.
     * @param[in] role     Qt role.
     *
     * @return returns true if successful.
     */
    bool setData(const QModelIndex & index,
                 const QVariant & value,
                 int role = Qt::EditRole) override;

    //=========================================================================================================
    /**
     * Returns the item flags for the given index.
     *
     * @param[in] index   The index that referres to the requested item.
     */
    Qt::ItemFlags flags(const QModelIndex & index) const override;

    //=========================================================================================================
    /**
     * Sets saved sample, used to prepare sample to be added to model.
     *
     * @param[in] iSamplePos   sample number to be set.
     */
    void setSamplePos(int iSamplePos);

    //=========================================================================================================
    /**
     * Used to pass first and last sample parameters to the model
     *
     * @param[in] firstSample  sample number of the first sample in the currently loaded fiff file.
     * @param[in] lastSample   sample number of the last sample in the currently loaded fiff file.
     */
    void setFirstLastSample(int firstSample,
                            int lastSample);

    //=========================================================================================================
    /**
     * returns the first and last sample parameters currently stored in the model
     *
     * @return Returns first and last sample parameters.
     */
    QPair<int, int> getFirstLastSample() const;

    //=========================================================================================================
    /**
     * Returns frequency parameter stored in the model
     *
     * @return frequency of the samples.
     */
    float getSampleFreq() const;

    //=========================================================================================================
    /**
     * Sets the stored frequerncy parameter to the function input
     *
     * @param[in] fFreq    frequency of the currently loaded fiff file.
     */
    void setSampleFreq(float fFreq);

    //=========================================================================================================
    /**
     * Return number of events to be displayed, based on current filter parameters
     *
     * @return number on events to display.
     */
    int getNumberOfEventsToDisplay() const;

    //=========================================================================================================
    /**
     * Returns number of groups
     *
     * @return Number of groups.
     */
    int getNumberOfGroups() const;

    //=========================================================================================================
    /**
     * Return event stored at index given by input parameter
     *
     * @param [in] iIndex   Index of the event to be retreived.
     *
     * @return Returns event at index given by input parameter.
     */
    int getEvent(int iIndex) const;

    //=========================================================================================================
    /**
     * Sets whether only to show selected events
     *
     * @param[in] iSelectedState   whether to show only selected. 0 - no, 2 - yes.
     */
    void setShowSelected(int iSelectedState);

    //=========================================================================================================
    /**
     * Returns whther to only show selected events
     *
     * @return whther to show selected events.
     */
    int getShowSelected();

    //=========================================================================================================
    /**
     * Returns sample freqency.
     *
     * @return sample frequency.
     */
    float getFreq();

    //=========================================================================================================
    /**
     * Saves model to the current model path if possible.
     *
     * @param[in] sPath   The path where the file should be saved to.
     *
     * @returns      True if saving was successful.
     */
    virtual bool saveToFile(const QString& sPath) override;

    //=========================================================================================================
    /**
     * Clears list of currently sleected rows
     */
    void clearEventSelection();

    //=========================================================================================================
    /**
     * Adds new row to list of selected rows
     *
     * @param[in] iSelectedIndex   row index to be added.
     */
    void appendSelected(int iSelectedIndex);

    //=========================================================================================================
    /**
     * Returns formatted matrix with event data based on current display and type settings
     *
     * @return Returns a matrix of formatted event data.
     */
    MatrixXi getEventMatrix();

    //=========================================================================================================
    /**
     * Sets the color of group iGroupIndex to color groupColor
     *
     * @param[in] iGroupIndex   Index of the goup to be changed.
     * @param[in] groupColor    Color the group should be changed to.
     */
    void setGroupColor(const QColor& groupColor);

    //=========================================================================================================
    /**
     * Sets the name of group at index iGroup index to sGroupName
     *
     * @param[in] iGroupIndex   index of which group to change.
     * @param[in] sGroupName     new name for group.
     */
    void setGroupName(int iGroupIndex,
                      const QString& sGroupName);

    //=========================================================================================================
    /**
     * Sets the name of selected group (or first selected if multiple) to currentText
     *
     * @param[in] currentText
     */
    void setSelectedGroupName(const QString &sGroupName);

    //=========================================================================================================
    /**
     * The type of this model (EventModel)
     *
     * @return The type of this model (EventModel).
     */
    inline MODEL_TYPE getType() const override;

    //=========================================================================================================
    /**
     * Returns the parent index of the given index.
     * In this Model the parent index in always QModelIndex().
     *
     * @param[in] index   The index that referres to the child.
     */
    inline QModelIndex parent(const QModelIndex &index) const override;

    //=========================================================================================================
    /**
     * Returns the index for the item in the model specified by the given row, column and parent index.
     * Currently only Qt::DisplayRole is supported.
     * Index rows reflect channels, first column is channel names, second is raw data.
     *
     * @param[in] row      The specified row.
     * @param[in] column   The specified column.
     * @param[in] parent   The parent index.
     */
    inline QModelIndex index(int row,
                             int column,
                             const QModelIndex &parent = QModelIndex()) const override;

    //=========================================================================================================
    /**
     * Sets the FiffViewModel to whitch this event model cooresponds
     *
     * @param[in] pModel   pointer to FiffRawViewModel.
     */
    void setFiffModel(QSharedPointer<FiffRawViewModel> pModel);

    //=========================================================================================================
    /**
     * Returns FiffRawViewModel associated with this event model
     *
     * @return pointer to cooresponding FiffRawViewModel.
     */
    QSharedPointer<FiffRawViewModel> getFiffModel();

    //=========================================================================================================
    /**
     * Adds event with given paramters.
     *
     * @param[in] iSample   Sample of new event.
     */
    void addEvent(int iSample);

    //=========================================================================================================
    /**
     * Adds event group with given parameters.
     *
     * @param[in] sName     Group name.
     * @param[in] color     Group color.
     */
    void addGroup(QString sName,
                  QColor color);

    //=========================================================================================================
    /**
     * Returns events between two samples
     *
     * @param[in] iBegin    Lower bound for sample (inclusive).
     * @param[in] iEnd      Upper bound for sample (exclusive).
     *
     * @return  Pointer to a vector of events.
     */
    std::unique_ptr<std::vector<EVENTSLIB::Event> > getEventsToDisplay(int iBegin,
                                                                       int iEnd) const;

    //=========================================================================================================
    /**
     * Returns all groups
     *
     * @return Pointer to a vector of groups
     */
    std::unique_ptr<std::vector<EVENTSLIB::EventGroup> > getGroupsToDisplay() const;

    //=========================================================================================================
    /**
     * Returns color of the group with given input parameters.
     *
     * @param[in] iGroupId      Group Id number.
     *
     * @return QColor of the group's color.
     */
    QColor getGroupColor(int iGroupId) const;

    //=========================================================================================================
    /**
     * Clears tracking of group selection.
     */
    void clearGroupSelection();

    //=========================================================================================================
    /**
     * Adds to tracking of selected groups.
     *
     * @param[in] iGroupId      Id of group to be added.
     */
    void addToSelectedGroups(int iGroupId);

    //=========================================================================================================
    /**
     * Returns which groups are selected
     *
     * @return  vector of group Ids of selected groups
     */
    std::vector<idNum> getSelectedGroups() const;

    //=========================================================================================================
    /**
     * Deletes selected group and events within
     */
    void deleteSelectedGroups();

    //=========================================================================================================
    /**
     * Turns shared memory on/off based on input parameters
     *
     * @param[in] bState    True - on  /  False - off.
     */
    void setSharedMemory(bool bState);

    //=========================================================================================================
    /**
     * Returns which events are selected
     *
     * @return vector of indices of selected groups
     */
    std::vector<uint> getEventSelection() const;

    //=========================================================================================================
    /**
     * Updates selection based on input list
     *
     * @param[in] indexList     List of selected indeces.
     */
    void updateSelectedGroups(const QList<QModelIndex>& indexList);

    //=========================================================================================================
    /**
     * Gets new events from trigger channels
     */
    void getEventsFromNewData();

signals:

    //=========================================================================================================
    /**
     * Emits updated new type to be added to GUI
     *
     * @param[in] currentFilterType    Type to be updated in GUI.
     */
    void updateEventTypes(const QString& currentFilterType);

    //=========================================================================================================
    /**
     * Emits to notify that there have been changes to the event groups
     */
    void eventGroupsUpdated();

private:

    //=========================================================================================================
    /**
     * Sets up default paramaters ofr new model
     */
    void initModel();

    //=========================================================================================================
    /**
     * Instantiates model from data in file pointed to by sFilePath
     *
     * @param[in] sFilePath    path to file with event data.
     */
    void initFromFile(const QString& sFilePath);

    //=========================================================================================================
    /**
     * Emit signals to trigger update of relevant views
     */
    void eventsUpdated();

    int                                 m_iSamplePos;                   /**< Sample of event to be added */
    int                                 m_iFirstSample;                 /**< First sample of file */
    int                                 m_iLastSample;                  /**< Last sample of file */

    int                                 m_iSelectedCheckState;          /**< State of checkbox of whether to show only selected events. */

    std::vector<uint>                   m_listEventSelection;           /**< List of selected events. */
    std::vector<idNum>                  m_selectedEventGroups;          /**< Vector of selected groups. */

    float                               m_fFreq;                        /**< Frequency of data file. */

    QSharedPointer<FiffRawViewModel>    m_pFiffModel;                   /**< Pointer to FiffRawViewModel associated with the events stored in this model. */

    EVENTSLIB::EventManager             m_EventManager;                 /**< Database of of events. */

    const static double                 m_dThreshold;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline MODEL_TYPE EventModel::getType() const
{
    return MODEL_TYPE::ANSHAREDLIB_EVENT_MODEL;
}

//=============================================================================================================

QModelIndex EventModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

//=============================================================================================================

QModelIndex EventModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return createIndex(row, column);
}

}
#endif // ANSHAREDLIB_EVENTMODEL_H
