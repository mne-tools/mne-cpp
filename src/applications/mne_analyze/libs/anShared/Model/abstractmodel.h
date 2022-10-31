//=============================================================================================================
/**
 * @file     abstractmodel.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @since    0.1.0
 * @date     April, 2018
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
 * @brief     AbstractModel class declaration.
 *
 */

#ifndef ANSHAREDLIB_ABSTRACTMODEL_H
#define ANSHAREDLIB_ABSTRACTMODEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../anshared_global.h"
#include "../Utils/types.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QPointer>
#include <QAbstractItemModel>
#include <QStandardItemModel>
#include <QDebug>

//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE ANSHAREDLIB
//=============================================================================================================

namespace ANSHAREDLIB {

//=============================================================================================================
// ANSHAREDLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 *
 * @brief Super class for all models that are intended to be used by AnalyzeData.
 *        Holds information such as model type.
 */
class ANSHAREDSHARED_EXPORT AbstractModel : public QAbstractItemModel
{
    Q_OBJECT

    //=========================================================================================================
    /**
     * This Structure is used to store the complete model path.
     * The path has uses the following pattern:
     *   sDirectoryPath/sModelName
     */
    struct ModelPath {
        ModelPath() {
            sDirectoryPath = QStringLiteral("");
            sModelName = QStringLiteral("");
        }

        ModelPath(const QString &sCompletePath) {
            setCompleteModelPath(sCompletePath);
        }

        void setCompleteModelPath(const QString &sCompleteModelPath) {
            sModelName = sCompleteModelPath.section('/', -1);
            sDirectoryPath = sCompleteModelPath.left(sCompleteModelPath.size() - sModelName.size());
        }

    public:
        QString sDirectoryPath;
        QString sModelName;
    };

public:
    typedef QSharedPointer<AbstractModel> SPtr;            /**< Shared pointer type for AbstractModel. */
    typedef QSharedPointer<const AbstractModel> ConstSPtr; /**< Const shared pointer type for AbstractModel. */

    //=========================================================================================================
    /**
     * Constructs a AbstractModel object. Simply pass potential parent object to super class.
     */
    AbstractModel(QObject *pParent = nullptr)
    : QAbstractItemModel(pParent) {}

    //=========================================================================================================
    /**
     * Constructs a AbstractModel object. It initializes the model path and passes potential parent object to super class.
     */
    AbstractModel(const QString &sPath, QObject *pParent = nullptr)
    : QAbstractItemModel(pParent)
    , m_modelPath(ModelPath(sPath)) {}

    //=========================================================================================================
    /**
     * Default destructor.
     */
    virtual ~AbstractModel() = default;

    //=========================================================================================================
    /**
     * Getter for the model type
     *
     * @return The type of the respective subclasses.
     */
    virtual MODEL_TYPE getType() const = 0;

    //=========================================================================================================
    /**
     * Returns the a unique path for this model.
     * If the model does not have an directory path a temporary path is used.
     * Temporary path: MODELTYPE/ModelName
     */
    virtual inline QString getModelPath() const;

    //=========================================================================================================
    /**
     * Sets a new path for the model.
     */
    virtual inline void setModelPath(const QString &sNewPath);

    //=========================================================================================================
    /**
     * Returns name of the model. The name is not unique!
     */
    virtual inline QString getModelName() const;

    //=========================================================================================================
    /**
     * Saves model to the current model path if possible.
     *
     * @param[in] sPath   The path where the file should be saved to.
     *
     * @returns      True if saving was successful.
     */
    virtual inline bool saveToFile(const QString& sPath);

    //=========================================================================================================
    /**
     * Whether the model has been initialized.
     *
     * @returns Tru or false flag whether the model has been initialized or not.
     */
    virtual inline bool isInit();

    //=========================================================================================================
    // Inherited by QAbstractItemModel:
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override = 0;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override = 0;
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override = 0;
    virtual QModelIndex parent(const QModelIndex &index) const override = 0;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override = 0;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override = 0;

protected:
    ModelPath                   m_modelPath;                /**< Path to model data in file structure. */

    bool                        m_bIsInit = false;          /**< Whether the model has been initialized. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

QString AbstractModel::getModelPath() const
{
    return m_modelPath.sDirectoryPath + m_modelPath.sModelName;
}

//=============================================================================================================

void AbstractModel::setModelPath(const QString &sNewPath)
{
    m_modelPath.setCompleteModelPath(sNewPath);
}

//=============================================================================================================

QString AbstractModel::getModelName() const
{
    return m_modelPath.sModelName;
}

//=============================================================================================================

bool AbstractModel::saveToFile(const QString& sPath)
{
    qDebug() << "[AbstractModel::saveToFile] Saving data to" << sPath << "is not implemented for MODELTYPE = " << getType();
    return false;
}

//=============================================================================================================

bool AbstractModel::isInit()
{
    return m_bIsInit;
}

} // namespace ANSHAREDLIB

#endif // ANSHAREDLIB_ABSTRACTMODEL_H
