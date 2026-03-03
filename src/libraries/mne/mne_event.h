#ifndef MNE_EVENT_H
#define MNE_EVENT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

#include <QString>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
/**
 * @brief Single trigger-event marker.
 *
 * Records a stimulus transition (from one value to another) at a given
 * sample position, together with an optional free-text comment.
 */
class MNESHARED_EXPORT MneEvent
{
public:
    MneEvent() = default;
    ~MneEvent() = default;

    unsigned int from = 0;          /**< Source transition value. */
    unsigned int to = 0;            /**< Destination transition value. */
    int          sample = 0;        /**< Sample number. */
    int          show = 0;          /**< Display flag (application-defined). */
    int          created_here = 0;  /**< Non-zero if this event was created in the program. */
    QString      comment;           /**< Free-text event comment. */
};

} // namespace MNELIB

#endif // MNE_EVENT_H
