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

    //=========================================================================================================
    /**
     * Pick elements from this named vector by name matching, writing them
     * into a result vector ordered according to the supplied name list.
     *
     * @param[in]  names        List of names to pick.
     * @param[in]  nnames       Number of names in the list.
     * @param[in]  require_all  If true, fail when any name is not found.
     * @param[out] res          Output vector (must have at least nnames elements).
     *
     * @return OK on success, FAIL on error.
     */
    int pick(const QStringList& names, int nnames, bool require_all, Eigen::Ref<Eigen::VectorXf> res) const;

    int         nvec = 0;   /**< Number of elements. */
    QStringList names;      /**< Name list for the elements. */
    Eigen::VectorXf data;      /**< The data itself. */
};

} // namespace MNELIB

#endif // MNE_NAMED_VECTOR_H
