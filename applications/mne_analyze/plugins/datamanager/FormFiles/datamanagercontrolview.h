//=============================================================================================================
/**
 * @file     datamanagerview.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @since    0.1.0
 * @date     August, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch, Lars Debor, Simon Heinke. All rights reserved.
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
 * @brief    Contains the declaration of the DataManagerView class.
 *
 */

#ifndef DATAMANAGERVIEW_H
#define DATAMANAGERVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QStandardItem>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
    class DataManagerView;
}

namespace ANSHARED {
    class AbstractModel;
}

//=============================================================================================================
/**
 * DataManagerView Plugin Control
 *
 * @brief The DataManagerView class provides the plugin control.
 */
class DataManagerControlView : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs the DataManagerView
     *
     * @param[in] parent     If parent is not NULL the QWidget becomes a child of QWidget inside parent.
     */
    explicit DataManagerControlView(QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the DataManagerView.
     */
    virtual ~DataManagerControlView();

    //=========================================================================================================
    /**
     * Sets the model to the tree view.
     *
     * @param[in] pModel       The new model.
     */
    void setModel(QAbstractItemModel *pModel);

private:

    //=========================================================================================================
    void customMenuRequested(QPoint pos);

    //=========================================================================================================
    /**
     * Sends signal to trigger model change when a new model is selcted
     *
     * @param [in] selected     New item being selected
     * @param [in] deselected   UNUSED - previously selected item
     */
    void onCurrentItemChanged(const QItemSelection &selected,
                              const QItemSelection &deselected);

    //=========================================================================================================
    /**
     * Uses the indeces of newly added subject and file to select it in the GUI view
     *
     * @param [in] iSubject     index of the subject the new file was added to
     * @param [in] iModel       index of the new model file relative to the subject
     */
    void onNewFileLoaded(int iSubject,
                         int iModel);

    void keyPressEvent(QKeyEvent *event);

    Ui::DataManagerView *m_pUi;   /**< The user interface */

signals:
    void removeItem(const QModelIndex& pIndex);
    void selectedModelChanged(const QVariant& data);
};

#endif // DATAMANAGERVIEW_H
