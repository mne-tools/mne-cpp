#ifndef MNE_NAMED_VECTOR_H
#define MNE_NAMED_VECTOR_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

#include <QStringList>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

/**
 * Named vector - vector specification with a channel list.
 */
class MNESHARED_EXPORT MneNamedVector
{
public:
    MneNamedVector() = default;
    ~MneNamedVector() = default;

    int         nvec = 0;   /**< Number of elements. */
    QStringList names;      /**< Name list for the elements. */
    float*      data = nullptr; /**< The data itself. */
};

/** Backward-compatible typedef aliases. */
typedef MneNamedVector  mneNamedVectorRec;
typedef MneNamedVector* mneNamedVector;

} // namespace MNELIB

#endif // MNE_NAMED_VECTOR_H
