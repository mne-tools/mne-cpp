//=============================================================================================================
/**
 * @file     fiff_id.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    FiffId class declaration.
 *
 */

#ifndef FIFF_ID_H
#define FIFF_ID_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_types.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
/**
 * These universially unique identifiers are also used to identify blocks within the files.
 * Replaces fiffIdRec which had a size of 5*4 = 20
 *
 * @brief Universially unique identifier.
 **/

class FIFFSHARED_EXPORT FiffId {

public:
    typedef QSharedPointer<FiffId> SPtr;            /**< Shared pointer type for FiffId. */
    typedef QSharedPointer<const FiffId> ConstSPtr; /**< Const shared pointer type for FiffId. */

    //=========================================================================================================
    /**
     * Default Constructor
     */
    FiffId();

    //=========================================================================================================
    /**
     * Destructor.
     */
    ~FiffId() = default;

    //=========================================================================================================
    /**
     * Constructs a (hopefully) unique file id
     * Refactored: fiff_new_file_id (fiff_id.c)
     */
    static FiffId new_file_id();

    //=========================================================================================================
    /**
     * Resets the File identifier.
     */
    void clear();

    //=========================================================================================================
    /**
     * Returns the machine ID consisting of a two integer number
     * Refactored: fiff_get_machid (fiff_get_machid.c)
     */
    static bool get_machid(int *fixed_id);

    //=========================================================================================================
    /**
     * Returns a default FiffId object to be used as an mutable placeholder for a default instance of the class.
     */
    static FiffId& getDefault();

    //=========================================================================================================
    /**
     * Prints the id
     * Refactored: print_id (fiff_dir_tree.c)
     */
    void print() const;

    //=========================================================================================================
    /**
     * True if FIFF id is empty.
     *
     * @return true if FIFF id is empty.
     */
    inline bool isEmpty() const;

    //=========================================================================================================
    /**
     * Size of the old struct (fiffIdRec) 5*int = 5*4 = 20
     *
     * @return the size of the old struct fiffIdRec.
     */
    inline static qint32 storageSize();

    //=========================================================================================================
    /**
    * Print mac address in a fashionable manner.
    */
    QString toMachidString() const;
    
    friend bool operator== (const FiffId &f1, const FiffId &f2);

public:
    fiff_int_t version;     /**< File version. */
    fiff_int_t machid[2];   /**< Unique machine ID. */
    fiffTimeRec time;       /**< Time of the ID creation. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool FiffId::isEmpty() const
{
    return this->version <= 0;
}

//=============================================================================================================

inline qint32 FiffId::storageSize()
{
    return 20;
}

//=============================================================================================================

inline bool operator== (const FiffId &a, const FiffId &b)
{
    return (a.version == b.version &&
            a.machid == b.machid &&
            a.time.secs == b.time.secs &&
            a.time.usecs == b.time.usecs);
}
} // NAMESPACE

#endif // FIFF_ID_H
