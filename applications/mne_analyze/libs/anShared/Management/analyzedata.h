//=============================================================================================================
/**
 * @file     analyzedata.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Christoph Dinh, Lorenz Esch, Lars Debor, Simon Heinke. All rights reserved.
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
#include "analyzedatamodel.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QString>
#include <QPointer>
#include <QFileInfo>
#include <QStandardItemModel>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

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
     * @param[in] parent             The entry point/model index to tart looking
     *
     * @return                       Vector of all models
     */
    QVector<QSharedPointer<AbstractModel> > getAllModels(QModelIndex parent = QModelIndex()) const;

    //=========================================================================================================
    /**
     * Returns a vector of all loaded models that have the specified type
     *
     * @param[in] mtype              The type to search for
     * @param[in] parent             The entry point/model index to tart looking
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
     * @param[in] sName              Model name (not the model path)
     *
     * @return                       Pointer to the model
     */
    QSharedPointer<AbstractModel> getModelByName(const QString &sName) const;

    //=========================================================================================================
    /**
     * Returns the requested model. If multiple matches are found the first one is returned.
     * If the path name is not used a nullptr is returned.
     *
     * @param[in] sPath              Model path (not the model name)
     * @param[in] parent             The entry point/model index to tart looking
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

        if(temp->isInit()) {
            // add to record, and tell others about the new model
            QStandardItem* pItem = new QStandardItem(temp->getModelName());
            pItem->setEditable(false);
            pItem->setDragEnabled(true);
            pItem->setToolTip(temp->getModelPath());

            QVariant data;
            data.setValue(temp);
            pItem->setData(data);
            m_pData->addData("Sample Subject", pItem);

            emit newModelAvailable(temp);
            return sm;
        } else {
            return Q_NULLPTR;
        }
    }

private:
    QPointer<AnalyzeDataModel>            m_pData;         /**< The loaded models in form of a QStandardItemModel. */

signals:
    //=========================================================================================================
    /**
     * This is emitted whenever a new model is loaded.
     *
     * @param[in] pModel      The newly available model
     */
    void newModelAvailable(QSharedPointer<AbstractModel> pModel);

    //=========================================================================================================
    /**
     * This is emitted whenever the model is completely.
     */
    void modelIsEmpty();

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
