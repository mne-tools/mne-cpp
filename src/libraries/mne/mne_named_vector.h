#ifndef MNE_NAMED_VECTOR_H
#define MNE_NAMED_VECTOR_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

#include <Eigen/Core>

#include <QStringList>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

/**
 * Named vector - vector specification with a channel list.
 */
class MNESHARED_EXPORT MNENamedVector
{
public:
    MNENamedVector() = default;
    ~MNENamedVector() = default;

    int         nvec = 0;   /**< Number of elements. */
    QStringList names;      /**< Name list for the elements. */
    Eigen::VectorXf data;      /**< The data itself. */
};

} // namespace MNELIB

#endif // MNE_NAMED_VECTOR_H
