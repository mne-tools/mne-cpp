//=============================================================================================================
/**
 * @file     analyzedata.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 *           Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Christoph Dinh, Lorenz Esch, Lars Debor, Simon Heinke, Gabriel Motta. All rights reserved.
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
 * @brief    Contains declaration of AnalyzeData Container class.
 *
 */

#ifndef ANALYZEDATA_H
#define ANALYZEDATA_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../anshared_global.h"
#include "../Model/abstractmodel.h"
#include "../Utils/types.h"
#include <disp/viewers/helpers/bidsviewmodel.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QString>
#include <QPointer>
#include <QFileInfo>
#include <QStandardItemModel>
#include <QDateTime>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace ANSHAREDLIB {
    class Communicator;
}

//=============================================================================================================
// DEFINE NAMESPACE ANSHAREDLIB
//=============================================================================================================

namespace ANSHAREDLIB
{

//=============================================================================================================
// ANSHAREDLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// ENUMERATIONS
//=============================================================================================================

//=========================================================================================================
/**
 * DECLARE CLASS AnalyzeData
 *
 * @brief The AnalyzeData class is the base data container.
 */
class ANSHAREDSHARED_EXPORT AnalyzeData : public QObject
{
    Q_OBJECT

public:
    typedef QSharedPointer<AnalyzeData> SPtr;               /**< Shared pointer type for AnalyzeData. */
    typedef QSharedPointer<const AnalyzeData> ConstSPtr;    /**< Const shared pointer type for AnalyzeData. */

    //=========================================================================================================
    /**
     * Constructs the Analyze Data.
     */
    AnalyzeData(QObject* pParent = nullptr);

    //=========================================================================================================
    /**
     * Destroys the Analyze Data.
     */
    ~AnalyzeData();

    //=========================================================================================================
    /**
     * Returns all models.
     *
     * @param[in] parent             The entry point/model index to tart looking.
     *
     * @return                       Vector of all models
     */
    QVector<QSharedPointer<AbstractModel> > getAllModels(QModelIndex parent = QModelIndex()) const;

    //=========================================================================================================
    /**
     * Returns a vector of all loaded models that have the specified type
     *
     * @param[in] mtype              The type to search for.
     * @param[in] parent             The entry point/model index to tart looking.
     *
     * @return                       Vector of models that have the specified type
     */
    QVector<QSharedPointer<AbstractModel> > getModelsByType(MODEL_TYPE mtype,
                                                            QModelIndex parent = QModelIndex()) const;

    //=========================================================================================================
    /**
     * Returns the requested model. If multiple matches are found the first one is returned.
     * If the path name is not used a nullptr is returned.
     *
     * @param[in] sName              Model name (not the model path).
     *
     * @return                       Pointer to the model
     */
    QSharedPointer<AbstractModel> getModelByName(const QString &sName) const;

    //=========================================================================================================
    /**
     * Returns the requested model. If multiple matches are found the first one is returned.
     * If the path name is not used a nullptr is returned.
     *
     * @param[in] sPath              Model path (not the model name).
     * @param[in] parent             The entry point/model index to tart looking.
     *
     * @return                       Pointer to the model
     */
    QSharedPointer<AbstractModel> getModelByPath(const QString& sPath,
                                                 QModelIndex parent = QModelIndex()) const;

    //=========================================================================================================
    /**
     * Returns the QStandardItemModel holding all the data
     */
    QStandardItemModel* getDataModel();

    //=========================================================================================================
    /**
     * Removes model stored under the given index.
     *
     * @param[in] index     The index to the item to be deleted.
     *
     * @return              Returns true if successful
     */
    bool removeModel(const QModelIndex &index);

    //=========================================================================================================
    /**
     * Add subject with name sSubjectName to BidsViewModel singleton
     *
     * @param[in] sSubjectName     Name of the new subject item.
     *
     * @return returns pointer to new subject item
     */
    QStandardItem* addSubject(const QString &sSubjectName);

    //=========================================================================================================
    /**
     * Updates stored indexes of currently selected data and item
     *
     * @param[in] index    index of the new selected item.
     */
    void newSelection(const QModelIndex &index);

    //=========================================================================================================
    /**
     * This is the main function for instanciating models. It simply calls the models constructor with the
     * provided path and inserts the model to the central item model. NO ERROR CHECKING IS PERFORMED !
     */
    template<class T>
    QSharedPointer<T> loadModel(const QString& sPath,
                                const QByteArray& byteLoadedData = QByteArray())
    {
        // check if model was already loaded
        if(QSharedPointer<AbstractModel> pModel = getModelByPath(sPath)) {
            qInfo() << "[AnalyzeData::loadModel] Data has been loaded already.";
            return qSharedPointerDynamicCast<T>(pModel);
        }

        // call model constructor with provided path
        QSharedPointer<T> sm = QSharedPointer<T>::create(sPath, byteLoadedData);
        QSharedPointer<AbstractModel> temp = qSharedPointerCast<AbstractModel>(sm);
        temp->setModelPath(sPath);

        int iType;
        QModelIndex index;

        switch(temp->getType()){
        case ANSHAREDLIB_FIFFRAW_MODEL:
        case ANSHAREDLIB_NOISE_MODEL: {
            iType = BIDS_FUNCTIONALDATA;
            index = m_SelectedItem;
            break;
        }
        case ANSHAREDLIB_BEMDATA_MODEL:
        case ANSHAREDLIB_MRICOORD_MODEL: {
            iType = BIDS_ANATOMICALDATA;
            index = m_SelectedItem;
            break;
        }
        case ANSHAREDLIB_EVENT_MODEL: {
            iType = BIDS_EVENT;
            index = m_SelectedFunctionalData;
            break;
        }
        case ANSHAREDLIB_AVERAGING_MODEL: {
            iType = BIDS_AVERAGE;
            index = m_SelectedFunctionalData;
            break;
        }
        default: {
            iType = BIDS_UNKNOWN;
            index = m_SelectedItem;
        }
        }

        QStandardItem* pItem = new QStandardItem(temp->getModelName());
        pItem->setEditable(false);
        pItem->setDragEnabled(true);
        pItem->setToolTip(temp->getModelPath());

        QVariant data;
        data.setValue(temp);
        pItem->setData(data);
        m_pData->addData(index,
                         pItem,
                         iType);
        return sm;

    }

    //=========================================================================================================
    template<class T>
    QSharedPointer<T> addModel(QSharedPointer<T> pNewModel,
                               const QString& sModelName){
        QSharedPointer<AbstractModel> temp = qSharedPointerCast<AbstractModel>(pNewModel);
        QStandardItem* pItem = new QStandardItem(sModelName);
        pItem->setEditable(true);
        pItem->setDragEnabled(true);
        pItem->setToolTip(temp->getModelPath());

        QVariant data;
        data.setValue(temp);

        switch(temp->getType()){
        case ANSHAREDLIB_AVERAGING_MODEL:{
            pItem->setData(data);
            m_pData->addToData(pItem,
                               m_SelectedFunctionalData,
                               BIDS_AVERAGE);
            break;
        }
        case ANSHAREDLIB_EVENT_MODEL: {
            pItem->setData(data);
            m_pData->addToData(pItem,
                               m_SelectedFunctionalData,
                               BIDS_EVENT);
            break;
        }
        case ANSHAREDLIB_DIPOLEFIT_MODEL:{
            pItem->setData(data);
            m_pData->addToData(pItem,
                               m_SelectedFunctionalData,
                               BIDS_DIPOLE);
            break;
        }
        case ANSHAREDLIB_FORWARDSOLUTION_MODEL:{
            pItem->setData(data);
            m_pData->addToData(pItem,
                               m_SelectedFunctionalData,
                               BIDS_UNKNOWN);
            break;
        }
        case ANSHAREDLIB_SOURCEESTIMATE_MODEL:{
            pItem->setData(data);
            m_pData->addToData(pItem,
                               m_SelectedFunctionalData,
                               BIDS_UNKNOWN);
            break;
        }
        case ANSHAREDLIB_FIFFRAW_MODEL: {
            pItem->setData(data);
            QModelIndex index = m_SelectedItem;
            m_pData->addData(index,
                             pItem,
                             BIDS_FUNCTIONALDATA);
            break;
        }
        default:{
            qWarning() << "[AnalyzeData::addModel] Model type not recognized.";
            break;
        }
        }
        return pNewModel;
    }

private:

    //=========================================================================================================
    /**
     * Returns a list of all items (including child items) in the BidsViewModel;
     *
     * @param[in] parent   index of parent to search under. QModelIndex() by default.
     *
     * @return list of all items in BidsViewModel
     */
    QList<QStandardItem*> getAllItems(QModelIndex parent = QModelIndex()) const;


    QPointer<DISPLIB::BidsViewModel>        m_pData;                    /**< The BidsViewModel that holds all the subject, session, and data items. */

    QPointer<ANSHAREDLIB::Communicator>     m_pCommu;                   /**< Used to send events. */

    QModelIndex                             m_SelectedItem;             /**< Index of currently selected item. */
    QModelIndex                             m_SelectedFunctionalData;   /**< Index of currently selected data item. */

signals:

    //=========================================================================================================
    /**
     * This is emitted whenever a model changes its path.
     *
     * @param[in] pModel             Pointer to the model.
     * @param[in] sOldModelPath      Old model path.
     * @param[in] sNewModelPath      New model path.
     */
    void modelPathChanged(QSharedPointer<AbstractModel> pModel,
                          const QString &sOldModelPath,
                          const QString &sNewModelPath);
};

} //Namespace

#endif //ANALYZEDATA_H
