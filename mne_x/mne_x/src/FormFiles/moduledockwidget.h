//=============================================================================================================
/**
* @file		moduledockwidget.h
* @author	Christoph Dinh <christoph.dinh@live.de>;
* @version	1.0
* @date		October, 2010
*
* @section	LICENSE
*
* Copyright (C) 2010 Christoph Dinh. All rights reserved.
*
* No part of this program may be photocopied, reproduced,
* or translated to another program language without the
* prior written consent of the author.
*
*
* @brief	Contains the declaration of the ModuleDockWidget class.
*
*/

#ifndef MODULEDOCKWIDGET_H
#define MODULEDOCKWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================




//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDockWidget>
#include <QMap>
#include <QVector>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QTreeWidget;
class QTreeWidgetItem;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE CSART
//=============================================================================================================

namespace CSART
{

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS OF NAMESPACE CSART
//=============================================================================================================


//=============================================================================================================
/**
* DECLARE CLASS ModuleDockWidget
*
* @brief The ModuleDockWidget class provides a tree view navigation through the program and its modules.
*/
class ModuleDockWidget : public QDockWidget
{
    Q_OBJECT

    friend class MainWindow;

public:

    //=========================================================================================================
    /**
    * Constructs a ModuleDockWidget which is a child of parent.
    *
    * @param [in] title of the new ModuleDockWidget
    * @param [in] parent pointer to parent widget; If parent is 0, the new ModuleDockWidget becomes a window. If parent is another widget, ModuleDockWidget becomes a child window inside parent. ModuleDockWidget is deleted when its parent is deleted.
    * @param [in] flags are normally 0, but it can be set to customise the frame of a window (i.e. parent must be 0). To customise the frame, use a value composed from the bitwise OR of any of the window flags.
    */
    ModuleDockWidget( const QString& title, QWidget* parent = 0, Qt::WindowFlags flags = 0 );

    //=========================================================================================================
    /**
    * Destroys the ModuleDockWidget.
    */
    virtual ~ModuleDockWidget();

    //=========================================================================================================
    /**
    * Tests whether item corresponds to a valid module.
    *
    * @param [in] item pointer to item which should be tested.
    * @return true it item corresponds to a valid module.
    */
    bool isValidModule(QTreeWidgetItem* item);

    //=========================================================================================================
    /**
    * Access function to last selected valid module index. In the case it hasn't been a valid module selected jet default value -1 is returned.
    *
    * @return the last selected valid module index or -1.
    */
    inline int getCurrentModuleIdx();

    //=========================================================================================================
    /**
    * Access function to the current selected tree item.
    *
    * @return the current selected tree item.
    */
    inline const QTreeWidgetItem* getCurrentItem();

    //=========================================================================================================
    /**
    * Tests whether module n is activated.
    *
    * @param [in] n index of module which should be tested.
    * @return whether module n is activated. If n is larger than module vector false is returned.
    */
    bool isActivated(int n) const;

    //=========================================================================================================
    /**
    * Changes activation status of module n.
    *
    * @param [in] n index of module which's status should be changed.
    * @param [in] status to which module should be changed.
    */
    void activateItem(int n, bool& status);

signals:
    //=========================================================================================================
    /**
    * This signal is emitted when the selected item changed.
    */
    void itemChanged();

    //=========================================================================================================
    /**
    * This signal is emitted when the selected item changed to a valid module.
    * @param [out] currentModuleNum index of current module.
    * @param [out] currentItem corresponding tree item.
    */
    void moduleChanged(int currentModuleNum, const QTreeWidgetItem* currentItem);

protected:

    //=========================================================================================================
    /**
    * This event is triggered when a user performs an action associated with opening a context menu. The actions required to open context menus vary between platforms; for example, on Windows, pressing the menu button or clicking the right mouse button will cause this event to be sent.
    *
    * @param [in] event pointer to context menu event.
    */
    virtual void contextMenuEvent (QContextMenuEvent* event);

private:
    //Tree View Module
    typedef QMap<int, QTreeWidgetItem*> t_ItemQMap;					/**< Defines a new tree widget item mapping type. */
    t_ItemQMap                          m_ItemQMap;					/**< Holds the tree widget item map. */
    int                                 m_iCurrentModuleIdx;		/**< Holds the current module index. */
    QTreeWidgetItem*                    m_pCurrentItem;				/**< Holds the current tree item. */
    QTreeWidget*                        m_pTreeWidgetModuleList;	/**< Holds the tree item list. */

private slots:
    //=========================================================================================================
    /**
    * Performs item validation. Depending on the validation result current item and current module index are set.
    *
    * @param [in] selectedItem whether program is running.
    */
    void itemSelected(QTreeWidgetItem* selectedItem);    /**< creates all actions for user interface of MainWindow class. */
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline int ModuleDockWidget::getCurrentModuleIdx()
{
    return m_iCurrentModuleIdx;
}


//*************************************************************************************************************

inline const QTreeWidgetItem* ModuleDockWidget::getCurrentItem()
{
    return m_pCurrentItem;
}



}//NAMESPACE


#endif // MODULEDOCKWIDGET_H
