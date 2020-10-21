//=============================================================================================================
/**
 * @file     annotationmodel.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.0
 * @date     March, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Christoph Dinh, Lorenz Esch, Gabriel Motta. All rights reserved.
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
 * @brief    Declaration of the AnnotationModel Class.
 *
 */

#ifndef ANSHAREDLIB_ANNOTATIONMODEL_H
#define ANSHAREDLIB_ANNOTATIONMODEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../anshared_global.h"
#include "../Utils/types.h"
#include "abstractmodel.h"

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
 * Data struture that stores a group of events and the parameters of the group and the events within
 */
struct EventGroup{
    int                 groupNumber;
    int                 groupType;

    QString             groupName;

    bool                isUserMade;

    QVector<int>        dataSamples;
    QVector<int>        dataTypes;
    QVector<int>        dataIsUserEvent;

    QVector<int>        dataSamples_Filtered;
    QVector<int>        dataTypes_Filtered;
    QVector<int>        dataIsUserEvent_Filtered;
};

//=============================================================================================================
/**
 * Model that holds the event information associated with a fiff file.
 */
class ANSHAREDSHARED_EXPORT AnnotationModel : public AbstractModel
{
    Q_OBJECT

public:
    typedef QSharedPointer<AnnotationModel> SPtr;              /**< Shared pointer type for AnnotationModel. */
    typedef QSharedPointer<const AnnotationModel> ConstSPtr;   /**< Const shared pointer type for AnnotationModel. */

    //=========================================================================================================
    /**
     * Constructs an annotation model
     *
     * @param [in] parent   QObject parent of the model
     */
    AnnotationModel(QObject* parent = Q_NULLPTR);

    //=========================================================================================================

    AnnotationModel(QSharedPointer<FiffRawViewModel> pFiffModel,
                    QObject* parent = Q_NULLPTR);

    //=========================================================================================================

    AnnotationModel(const QString &sFilePath,
                    const QByteArray& byteLoadedData = QByteArray(),
                    float fSampFreq = 600,
                    int iFirstSampOffst = 0,
                    QObject* parent = Q_NULLPTR);

    //=========================================================================================================
    /**
     * Destructs an annotation model.
     */
    ~AnnotationModel();

    //=========================================================================================================
    /**
     * Inserts span rows at position
     *
     * @param [in] position     where to insert rows
     * @param [in] span         how many rows to insert
     * @param [in] parent       parent of inserted rows (unused)
     *
     * @return  returns true if successful
     */
    bool insertRows(int position,
                    int span,
                    const QModelIndex & parent) override;

    //=========================================================================================================
    /**
     * Removes span rows at position
     *
     * @param [in] position     where to remove rows
     * @param [in] span         how many rows to remove
     * @param [in] parent       parent of inserted rows (unused)
     *
     * @return  returns true if successful
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
     * @param[in] orientation    Qt::Horizontal or Qt::Vertical
     * @param[in] role           role to show
     *
     * @return accessed eader data
     */
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role) const override;

    //=========================================================================================================
    /**
     * Sets index to value based on role
     *
     * @param [in] index    model index to which the data will be set
     * @param [in] value    data to be set
     * @param [in] role     Qt role
     *
     * @return returns true if successful
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
     * Gets a list of event types currently held by the model
     *
     * @return Returns a list of event types
     */
    QStringList getEventTypeList() const;

    //=========================================================================================================
    /**
     * Sets saved sample, used to prepare sample to be added to model.
     *
     * @param [in] iSamplePos   sample number to be set
     */
    void setSamplePos(int iSamplePos);

    //=========================================================================================================
    /**
     * Sets current filter setting sto only display selected annotation type
     *
     * @param [in] eventType    Type of annotation that is to be displayed when filterd
     */
    void setEventFilterType(const QString eventType);

    //=========================================================================================================
    /**
     * Used to pass first and last sample parameters to the model
     *
     * @param [in] firstSample  sample number of the first sample in the currently loaded fiff file
     * @param [in] lastSample   sample number of the last sample in the currently loaded fiff file
     */
    void setFirstLastSample(int firstSample,
                            int lastSample);

    //=========================================================================================================
    /**
     * returns the first and last sample parameters currently stored in the model
     *
     * @return Returns first and last sample parameters
     */
    QPair<int, int> getFirstLastSample() const;

    //=========================================================================================================
    /**
     * Returns frequency parameter stored in the model
     *
     * @return frequency of the samples
     */
    float getSampleFreq() const;

    //=========================================================================================================
    /**
     * Sets the stored frequerncy parameter to the function input
     *
     * @param [in] fFreq    frequency of the currently loaded fiff file
     */
    void setSampleFreq(float fFreq);

    //=========================================================================================================
    /**
     * Return number of annotations to be displayed, based on current filter parameters
     *
     * @return number on annotations to display
     */
    int getNumberOfAnnotations() const;

    //=========================================================================================================
    /**
     * Return annotation stored at index given by input parameter
     *
     * @param [in] iIndex   Index of the annotation to be retreived
     *
     * @return Returns annotation at index given by input parameter
     */
    int getAnnotation(int iIndex) const;

    //=========================================================================================================
    /**
     * Returns map of the colors assigned to each of the annotation types
     *
     * @return Map of annotation colors
     */
    QMap<int, QColor>& getTypeColors();

    //=========================================================================================================
    /**
     * Returns map of the colors assigned to each of the annotation groups
     *
     * @return Map of annotation colors
     */
    QMap<int, QColor>& getGroupColors();

    //=========================================================================================================
    /**
     * Adds a new annotation type with the input parameters as configuration parameters
     *
     * @param [in] eventType    type number (0-99)
     * @param [in] typeColor    color to be used for drawing
     */
    void addNewAnnotationType(const QString &eventType,
                              const QColor &typeColor);

    //=========================================================================================================
    /**
     * Pass which annotations are currenlty selected in the view GUI
     *
     * @param [in] iSelected    currently selected annotation
     */
    void setSelectedAnn(int iSelected);

    //=========================================================================================================
    /**
     * Returns currently selected annotation stored locally in the model
     *
     * @return Returns stored selected annotation
     */
    int getSelectedAnn();

    //=========================================================================================================
    /**
     * Sets whether only to show selected annotations
     *
     * @param [in] iSelectedState   whether to show only selected. 0 - no, 2 - yes
     */
    void setShowSelected(int iSelectedState);

    //=========================================================================================================
    /**
     * Returns whther to only show selected annotations
     *
     * @return whther to show selected annotations
     */
    int getShowSelected();

    //=========================================================================================================
    /**
     * Returns sample freqency
     *
     * @return sample frequency
     */
    float getFreq();

    //=========================================================================================================
    /**
     * Saves model to the current model path if possible.
     *
     * @param [in] sPath   The path where the file should be saved to.
     *
     * @returns      True if saving was successful
     */
    virtual bool saveToFile(const QString& sPath) override;

    //=========================================================================================================
    /**
     * Saves last added type or last type to be filterd to
     *
     * @param [in] iType    type to be saved
     */
    void setLastType(int iType);

    //=========================================================================================================
    /**
     * Updates the value of a sample for real time annotation scrolling
     *
     * @param [in] iIndex   Index of sample to be changed
     * @param [in] iSample  Sample value to be changed to
     */
    void updateFilteredSample(int iIndex,
                              int iSample);

    //=========================================================================================================
    /**
     * Updates the value of the currently selected sample for real time annotation scrolling
     *
     * @param [in] iSample  Sample value to be changed to
     */
    void updateFilteredSample(int iSample);

    //=========================================================================================================
    /**
     * Clears list of currently sleected rows
     */
    void clearSelected();

    //=========================================================================================================
    /**
     * Adds new row to list of selected rows
     *
     * @param [in] iSelectedIndex   row index to be added
     */
    void appendSelected(int iSelectedIndex);

    //=========================================================================================================
    /**
     * Returns formatted matrix with annotation data based on current display and type settings
     *
     * @return Returns a matrix of formatted annotation data
     */
    MatrixXi getAnnotationMatrix(int iGroup = 9999);

    //=========================================================================================================
    /**
     * Creates a new event group with the passed parameters
     *
     * @param[in] sGroupName        name of the group
     * @param[in] bIsUserMade       whether the group is user made
     * @param[in] iType             default group type when adding events
     *
     * @return the index of the newly added group
     */
    int createGroup(const QString& sGroupName,
                    bool bIsUserMade = false,
                    int iType = 0,
                    const QColor &typeColor = QColor(Qt::black));

    //=========================================================================================================
    /**
     * Switches to a group based on the index (map key), triggers view to update
     *
     * @param[in] iGroupIndex   index(map key) of desired group
     */
    void switchGroup(int iGroupIndex);

    //=========================================================================================================
    /**
     * Removes a group based on the index (map key), triggers view to update
     *
     * @param[in] iGroupIndex   index(map key) of the group to be deleted
     */
    void removeGroup(int iGroupIndex);

    //=========================================================================================================
    /**
     * Returns how many groups are stored in m_mAnnotationHub
     *
     * @return the amount of groups stored
     */
    int getHubSize();

    //=========================================================================================================
    /**
     * Returns whether the group at a certain index is user made
     *
     * @param[in] iIndex    index of the stored group
     *
     * @return whether the group is user made
     */
    bool getHubUserMade(int iIndex);

    //=========================================================================================================
    /**
     * Retruns whether current slected group is made by the user
     *
     * @return whether current group is use made
     */
    bool isUserMade();

    //=========================================================================================================
    /**
     * Displays all events from all groups and triggers view updates
     *
     * @param[in] bSet      whether the checkbox is checked or not
     */
    void showAll(bool bSet);

    //=========================================================================================================
    /**
     * Loads events from all groups
     */
    void loadAllGroups();

    //=========================================================================================================
    /**
     * Clears events and triggers view to update
     */
    void hideAll();

    //=========================================================================================================
    /**
     * Returns counting index that gives each group a unique number
     *
     * @return m_iIndexCount
     */
    int getIndexCount();

    //=========================================================================================================
    /**
     * Returns index of group with name sGroupName. Returns 9999 if no group present with that name
     *
     * @param[in] sGroupName     name of group of which we want the index
     *
     * @return index of group with name sGroupName
     */
    int getIndexFromName(const QString& sGroupName);

    //=========================================================================================================
    /**
     * Returns the group of the event pointed to by parameter iIndex
     *
     * @param[in] iIndex    Index for which we want to get the group
     *
     * @return the group event at iIndex belongs to
     */
    int currentGroup(int iIndex);

    //=========================================================================================================
    /**
     * Push widget item into member stack - used for storing groups when file is unloaded
     *
     * @param[in] item      stores item in the member stack
     */
    void pushGroup(QListWidgetItem* item);

    //=========================================================================================================
    /**
     * Pops widget item from member stack - used for restroing groups when file is loaded
     *
     * @return retruns widget item on top of the member stack
     */
    QListWidgetItem* popGroup();

    //=========================================================================================================
    /**
     * Returns how many item are currently stored in the member stack
     *
     * @return size of member stack m_dataStoredGroups
     */
    int getGroupStackSize();

    //=========================================================================================================
    /**
     * Sets the color of group iGroupIndex to color groupColor
     *
     * @param[in] iGroupIndex   Index of the goup to be changed
     * @param[in] groupColor    Color the group should be changed to
     */
    void setGroupColor(int iGroupIndex,
                       const QColor& groupColor);

    //=========================================================================================================
    /**
     * Sets the name of group at index iGroup index to sGroupName
     *
     * @param[in] iGroupIndex   index of which group to change
     * @param[in] sGroupName     new name for group
     */
    void setGroupName(int iGroupIndex,
                      const QString& sGroupName);

    //=========================================================================================================
    /**
     * Saves event group to m_mAnnotationHub
     */
    void saveGroup();

    //=========================================================================================================
    /**
     * Returns group name of the group with map key iMapKey
     *
     * @param[in] iMapKey       map key(group index) of the group from which we are getting the name
     *
     * @return name of group at from m_mAnnotationHub at iMapKey
     */
    QString getGroupName(int iMapKey);

    //=========================================================================================================
    /**
     * Returns the name of the event at index iListIndex as if the map of groups were an array. Does not use Group Index.
     *
     * @param[in] iListIndex    index of the group from which we are getting the name
     *
     * @return name of the iListIndex-th event group for m_mAnnotationHub
     */
    QString getGroupNameFromList(int iListIndex);

    //=========================================================================================================
    /**
     * The type of this model (AnnotationModel)
     *
     * @return The type of this model (AnnotationModel)
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
     * @param [in] pModel   pointer to FiffRawViewModel
     */
    void setFiffModel(QSharedPointer<FiffRawViewModel> pModel);

    //=========================================================================================================
    /**
     * Shift saved smaples based on offset iFirstSampleOffset
     *
     * @param [in] iFirstSampleOffset   offset due to sample index of first sample
     */
    void applyOffset(int iFirstSampleOffset);

    //=========================================================================================================
    /**
     * Returns FiffRawViewModel associated with this annotation model
     *
     * @return pointer to cooresponding FiffRawViewModel
     */
    QSharedPointer<FiffRawViewModel> getFiffModel();

signals:

    //=========================================================================================================
    /**
     * Emits updated new type to be added to GUI
     *
     * @param [in] currentFilterType    Type to be updated in GUI
     */
    void updateEventTypes(const QString& currentFilterType);

    //=========================================================================================================
    /**
     * Signals for new event to be added to the model, based on current m_iSamplePos
     */
    void addNewAnnotation();

private:

    //=========================================================================================================
    /**
     * Clears selected group of events
     */
    void resetSelection();

    //=========================================================================================================
    /**
     * Sets up default paramaters ofr new model
     */
    void initModel();

    //=========================================================================================================
    /**
     * Instantiates model from data in file pointed to by sFilePath
     *
     * @param [in] sFilePath    path to file with event data
     */
    void initFromFile(const QString& sFilePath);

    QStringList                         m_eventTypeList;                /** <List of the possible event types */

    QMap<int,EventGroup*>               m_mAnnotationHub;               /** <Map of the EventGroups, which holds groups of events */

    int                                 m_iSelectedGroup;               /** <Index in m_mAnnotationHub of the current selected group */
    int                                 m_iType;                        /** <Type of the currently selected event group */
    int                                 m_iIndexCount;

    QVector<int>                        m_dataSamples;                  /**< Vector of samples of events of the currently loded event group */
    QVector<int>                        m_dataTypes;                    /**< Types of the events of the currently loaded event group */
    QVector<int>                        m_dataIsUserEvent;              /**< Whether the events in the currently loaded event group are user-made */
    QVector<int>                        m_dataGroup;

    QVector<int>                        m_dataSamplesFiltered;         /**< Vector of samples of events to be displayed of the currently loded event group */
    QVector<int>                        m_dataTypesFiltered;           /**< Types of the events to be displayed of the currently loaded event group */
    QVector<int>                        m_dataIsUserEventFiltered;     /**< Whether the events to be displayed in the currently loaded event group are user-made */
    QVector<int>                        m_dataGroupFiltered;

    bool                                m_bIsUserMade;                  /**< Whether the current loaded group is user made */

    int                                 m_iSamplePos;                   /**< Sample of event to be added */
    int                                 m_iFirstSample;                 /**< First sample of file */
    int                                 m_iLastSample;                  /**< Last sample of file */

    int                                 m_iSelectedCheckState;          /**< State of checkbox of whether to show only selected events */
    int                                 m_iSelectedAnn;                 /**< Index of selected events */

    QList<int>                          m_dataSelectedRows;             /**< List of selected rows for multiple evnt selection */

    int                                 m_iLastTypeAdded;               /**< Stores last created type */

    float                               m_fFreq;                        /**< Frequency of data file */

    QString                             m_sFilterEventType;             /**< String for diplaying event types */
    QMap<int, QColor>                   m_eventTypeColor;               /**< Stores colors to display for each event type */
    QMap<int, QColor>                   m_eventGroupColor;              /**< Stores colors to display for each event group */

    QStack<QListWidgetItem*>            m_dataStoredGroups;             /**< Stores the groups for switching between files */

    QSharedPointer<FiffRawViewModel>    m_pFiffModel;                   /**< Pointer to FiffRawViewModel associated with the events stored in this model */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline MODEL_TYPE AnnotationModel::getType() const
{
    return MODEL_TYPE::ANSHAREDLIB_ANNOTATION_MODEL;
}

//=============================================================================================================

QModelIndex AnnotationModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

//=============================================================================================================

QModelIndex AnnotationModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return createIndex(row, column);
}

}
#endif // ANSHAREDLIB_ANNOTATIONMODEL_H
