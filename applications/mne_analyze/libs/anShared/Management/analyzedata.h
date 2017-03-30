//=============================================================================================================
/**
* @file     analyzedata.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Christoph Dinh and Matti Hamalainen. All rights reserved.
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

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../anshared_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace INVERSELIB {
    class ECDSet;
}

//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE ANSHAREDLIB
//=============================================================================================================

namespace ANSHAREDLIB
{


//*************************************************************************************************************
//=============================================================================================================
// ENUMERATIONS
//=============================================================================================================



//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
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
    AnalyzeData(QObject* parent = 0);

    //=========================================================================================================
    /**
    * Destroys the Analyze Data.
    */
    virtual ~AnalyzeData();

// Database Interface for now -> consider using a Abstract Item Model
signals:
    void currentECDSetChanged_signal();    /**< Emmited when the current ECD Set changed*/

public:
    //=========================================================================================================
    /**
    * Returns the current ECD Set
    *
    * @return The current ECD Set
    */
    QSharedPointer<INVERSELIB::ECDSet> currentECDSet() const;
    //=========================================================================================================
    /**
    * Sets the current ECD Set
    *
    * @param [in] ecdSet    Sets the current ECD Set;
    */
    void setCurrentECDSet(const QSharedPointer<INVERSELIB::ECDSet> &ecdSet);

    //=========================================================================================================
    /**
    * Returns a list of all past ECD Sets
    *
    * @return All past ECD Sets
    */
    QList<QSharedPointer<INVERSELIB::ECDSet> > ecdSets() const;

// Database -> Consierd using abstract item models or other datamanagement architecture
private:
    // Dipole ECDs
    QSharedPointer<INVERSELIB::ECDSet>          m_pCurrentECDSet;   /**< Current ECD Set.*/
    QList< QSharedPointer<INVERSELIB::ECDSet> > m_qListECDSets;     /**< List of all past ECD Sets.*/
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} //Namespace

#endif //ANALYZEDATA_H
