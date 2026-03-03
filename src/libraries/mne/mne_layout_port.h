#ifndef MNE_LAYOUT_PORT_H
#define MNE_LAYOUT_PORT_H

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
 * @brief Single viewport in a plotter layout.
 *
 * Holds viewport bounds and the colon-separated list of channel
 * names assigned to this viewport.
 */
class MNESHARED_EXPORT MneLayoutPort
{
public:
    MneLayoutPort() = default;
    ~MneLayoutPort() = default;

    int     portno = 0;         /**< Running number of this viewport. */
    int     invert = 0;         /**< Invert the signal coming to this port. */
    float   xmin = 0;           /**< Viewport left bound. */
    float   xmax = 0;           /**< Viewport right bound. */
    float   ymin = 0;           /**< Viewport bottom bound. */
    float   ymax = 0;           /**< Viewport top bound. */
    QString names;              /**< Channels assigned to this port (colon-separated). */
    int     match = 0;          /**< Non-zero if this port matches the current channel. */
};

/** Backward-compatible typedef aliases. */
typedef MneLayoutPort  mneLayoutPortRec;
typedef MneLayoutPort* mneLayoutPort;

} // namespace MNELIB

#endif // MNE_LAYOUT_PORT_H
