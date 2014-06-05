#ifndef SENSORMODEL_H
#define SENSORMODEL_H

#include <QAbstractTableModel>
#include <QDebug>


#include <xMeas/realtimesamplearraychinfo.h>

#include "sensorlayout.h"
#include "sensorgroup.h"


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class SensorItem;


//=============================================================================================================
/**
* DECLARE CLASS SensorModel
*
* @brief The SensorModel class implements a table model which holds the sensor model properties
*/
class SensorModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    //=========================================================================================================
    /**
    * Constructs a SensorModel for the given parent.
    *
    * @param [in] parent    parent of item
    */
    SensorModel(QObject *parent = 0);

    //=========================================================================================================
    /**
    * Constructs a SensorModel from a device for the given parent.
    *
    * @param [in] device    device to read from
    * @param [in] parent    parent of SensorModel
    */
    SensorModel(QIODevice* device, QObject *parent = 0);

    //=========================================================================================================
    /**
    * Number of rows for the given model index parent
    *
    * @param [in] parent    the parent of the model index
    *
    * @return the number of rows
    */
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;

    //=========================================================================================================
    /**
    * Number of columns for the given model index parent
    *
    * @param [in] parent    the parent of the model index
    *
    * @return the number of columns
    */
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;

    //=========================================================================================================
    /**
    * Data for the row and column and given display role
    *
    * @param [in] row       index row
    * @param [in] column    index column
    * @param [in] role      display role to access
    *
    * @return the accessed data
    */
    inline QVariant data(int row, int column, int role = Qt::DisplayRole) const;

    //=========================================================================================================
    /**
    * Data for the given index and given display role
    *
    * @param [in] index     index row
    * @param [in] role      display role to access
    *
    * @return the accessed data
    */
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;


    //=========================================================================================================
    /**
    * The loaded sensor layouts
    *
    * @return the loaded sensor layouts
    */
    inline const QList<SensorLayout>& getSensorLayouts() const;

    //=========================================================================================================
    /**
    * The number of loaded sensor layouts
    *
    * @return the number of loaded sensor layouts
    */
    inline qint32 getNumLayouts() const;

    //=========================================================================================================
    /**
    * Index of the current layout
    *
    * @return the index of the current layout
    */
    inline qint32 getCurrentLayout() const;

    //=========================================================================================================
    /**
    * The channel name to the overall channel number map
    *
    * @return a reference to channel name to the overall channel number map
    */
    inline const QMap<QString, qint32>& getNameIdMap() const;

    //=========================================================================================================
    /**
    * Apply a sensor selection group
    *
    * @param [in] id    id of the sensor group to apply
    */
    void applySensorGroup(int id);

    //=========================================================================================================
    /**
    * Set the current layout to display
    *
    * @param [in] id    id of the sensor layout to display
    */
    void setCurrentLayout(int id);

    //=========================================================================================================
    /**
    * Get available sensor groups
    *
    * @return list of available sensor groups
    */
    inline const QList<SensorGroup>& getSensorGroups() const;

    //=========================================================================================================
    /**
    * Channel info to map layouts to
    *
    * @param [in] chInfoList    channel info to map layouts to
    */
    void mapChannelInfo(const QList<XMEASLIB::RealTimeSampleArrayChInfo>& chInfoList);

    //=========================================================================================================
    /**
    * Update channel selection state of item
    *
    * @param [in] item      sensor item which changed selection state
    */
    void updateChannelState(SensorItem* item);

    //=========================================================================================================
    /**
    * Change selction without emitting newSelection signal
    *
    * @param [in] selection     list of channels which should be selected
    */
    void silentUpdateSelection(const QList<qint32>& selection);

signals:
    //=========================================================================================================
    /**
    * Signal emitted when selection changed
    *
    * @param [in] selection     list of channels which should be selected
    */
    void newSelection(QList<qint32> selection);

    //=========================================================================================================
    /**
    * Signal emitted when new layout is selected
    */
    void newLayout();

private:
    //=========================================================================================================
    /**
    * Create selection list on the basis of the current item selection
    */
    void createSelection();

    //=========================================================================================================
    /**
    * Read mne-x sensor layout of given device
    *
    * @param [in] device    device to read the layout from
    */
    bool read(QIODevice* device);

    qint32 m_iCurrentLayoutId;                  /**< Current layout index.*/
    QList<SensorLayout> m_qListSensorLayouts;   /**< Available sensor layouts.*/
    QList<SensorGroup> m_qListSensorGroups;     /**< Available sensor selection groups.*/

    QMap<qint32, bool>      m_qMapSelection;    /**< Channel number selected state map.*/
    QMap<QString, qint32>   m_qMapNameId;       /**< Fast lookup between channel name and its index.*/
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline QVariant SensorModel::data(int row, int column, int role) const
{
    return data(index(row, column), role);
}


//*************************************************************************************************************

inline const QList<SensorLayout>& SensorModel::getSensorLayouts() const
{
    return m_qListSensorLayouts;
}


//*************************************************************************************************************

inline qint32 SensorModel::getNumLayouts() const
{
    return m_qListSensorLayouts.size();
}


//*************************************************************************************************************

inline const QList<SensorGroup>& SensorModel::getSensorGroups() const
{
    return m_qListSensorGroups;
}


//*************************************************************************************************************

inline qint32 SensorModel::getCurrentLayout() const
{
    return m_iCurrentLayoutId;
}


//*************************************************************************************************************

inline const QMap<QString, qint32>& SensorModel::getNameIdMap() const
{
    return m_qMapNameId;
}

#endif // SENSORMODEL_H
