#ifndef MNE_FILTER_DEF_H
#define MNE_FILTER_DEF_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

#ifndef MNEFILTERDEF
#define MNEFILTERDEF
/**
 * Filter definition parameters.
 */
class MNESHARED_EXPORT MNEFilterDef
{
public:
    MNEFilterDef() = default;
    ~MNEFilterDef() = default;

    bool  filter_on = false;         /**< Is it on? */
    int   size = 0;                  /**< Length in samples (must be a power of 2). */
    int   taper_size = 0;            /**< How long a taper in the beginning and end. */
    float highpass = 0;              /**< Highpass in Hz. */
    float highpass_width = 0;        /**< Highpass transition width in Hz. */
    float lowpass = 0;               /**< Lowpass in Hz. */
    float lowpass_width = 0;         /**< Lowpass transition width in Hz. */
    float eog_highpass = 0;          /**< EOG highpass in Hz. */
    float eog_highpass_width = 0;    /**< EOG highpass transition width in Hz. */
    float eog_lowpass = 0;           /**< EOG lowpass in Hz. */
    float eog_lowpass_width = 0;     /**< EOG lowpass transition width in Hz. */
};

#endif

} // namespace MNELIB

#endif // MNE_FILTER_DEF_H
