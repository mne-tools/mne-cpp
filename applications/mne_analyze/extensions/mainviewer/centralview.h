//=============================================================================================================
/**
* @file     centralview.h
* @author   Simon Heinke <simon.heinke@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2018
*
* @section  LICENSE
*
* Copyright (C) 2018, Simon Heinke and Matti Hamalainen. All rights reserved.
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
* @brief     CentralView class declaration.
*
*/

#ifndef MAINVIEWEREXTENSION_CENTRALVIEW_H
#define MAINVIEWEREXTENSION_CENTRALVIEW_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DCore/QEntity>
#include <Qt3DRender/QCamera>
#include <QVector>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MAINVIEWEREXTENSION
//=============================================================================================================

namespace MAINVIEWEREXTENSION {


//*************************************************************************************************************
//=============================================================================================================
// MAINVIEWEREXTENSION FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* The CentralView class manages the registration and usage of entity trees within MNE-Analzye.
* It is the main display for 3D content in MNEAnalyze. It inherits Qt3DExtras::Qt3DWindow and specifies
* camera, initial view angle, camera controller etc.
* It keeps track of a QEntity-Tree that reflects all registered content. Unfortunately, the Qt3D backend
* is a nightmare to use for dynamic scene managing, so there are quite a few things to pay attention
* to. Read the documentation for methods addEntityTree and removeEntityTree for more details.
* Best thing to do is to register your EntityTree once and then work with setEnabled or similiar methods, that
* do not change the structure above your trees root node.
* Registered SharedPointers are copied into a vector to keep their reference-count mechanism working.
*/
class CentralView : public Qt3DExtras::Qt3DWindow
{
    Q_OBJECT
public:
    typedef QSharedPointer<CentralView> SPtr;            /**< Shared pointer type for CentralView. */
    typedef QSharedPointer<const CentralView> ConstSPtr; /**< Const shared pointer type for CentralView. */

    //=========================================================================================================
    /**
    * Constructs a CentralView object.
    */
    CentralView();

    //=========================================================================================================
    /**
    * Default destructor
    */
    ~CentralView() = default;

    //=========================================================================================================
    /**
    * This will insert the passed QEntity below the views root. Note that, as the shared pointer implies, the
    * ownership of the passed entity tree is now shared. NEVER call delete or clear on a weak or strong pointer
    * to an entity tree that might be still in used, as this will definetely chrash the program. If you do not
    * need a passed entity tree anymore call removeEntityTree and then simply drop all references to the shared
    * pointer so that it will be deleted when the last references within the CentralView are dropped as well.
    *
    * @param[in] pEntity The QEntity to be added.
    */
    void addEntity(QSharedPointer<Qt3DCore::QEntity> pEntity);

    //=========================================================================================================
    /**
    * This will remove the child named with sIdentifier or give out a warning in case the child could not be found.
    *
    * NB: When you want to use your entity tree after removing it (e.g. when passing it to another display), be
    * aware that the root of your entity tree will have a new parent. This parent gets created by the CentralView
    * upon removal and is necessary in order to keep the Qt3D backend alive. Best practice is to act as if you did
    * not read this, meaning that you should just pass the "old" root to any further use cases.
    * Also practise caution when restructuring your entity tree or while overwriting it with a new QEntity tree.
    * Depending on your implementation, this may easily cause double frees or other memory corruptions.
    * If you want to overwrite your QEntity root, make sure that you do not have any other references to these
    * children, that might cause double-frees upon destruction.
    *
    * @param[in] pEntity The QEntity that should be removed
    */
    void removeEntity(QSharedPointer<Qt3DCore::QEntity> pEntity);

    //=========================================================================================================
    /**
    * This is called during shutdown of the program in order to prevent double frees
    */
    void shutdown();

private:

    //=========================================================================================================
    /**
    * Initializes the 3D view.
    */
    void init();

    //=========================================================================================================
    /**
    * Helper function for creating a so-called "Anti Crash Node". These are necessary because QEntites apparently
    * always need a valid parent (i.e. one that is not null) once they were assigned a non-null parent in the
    * first place. The Anti Crash Nodes are collected and occasionally cleaned up (see checkForUnusedAntiCrashNodes)
    */
    Qt3DCore::QEntity* createNewAntiCrashNode();

    //=========================================================================================================
    /**
    * This checks for Anti Crash Nodes that are no longer used because the user has set a new parent for the
    * removed entity tree (e.g. by moving the Entity to another display). This method does not need to be executed
    * in a specific order, but should be called regularly and with a low frequency (e.g. whenever a new entity is
    * added).
    */
    void checkForUnusedAntiCrashNodes();

    Qt3DCore::QEntity *m_pRootEntity;           /**< Root entity */

    Qt3DRender::QCamera *m_pCamera;             /**< Camera */


    /**
    * Since parent-child connections inside the tree are based on normal pointers, we need to keep track of
    * shared pointers in order for the reference-count mechanism to work correctly
    */
    QVector<QSharedPointer<Qt3DCore::QEntity> > m_vEntities;

    QVector<QSharedPointer<Qt3DCore::QEntity> > m_vAntiCrashNodes;  /**< See docu of createNewAntiCrashNode for more details */
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace MAINVIEWEREXTENSION

#endif // MAINVIEWEREXTENSION_CENTRALVIEW_H
