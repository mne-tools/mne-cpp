#ifndef MNE_LAYOUT_PORT_H
#define MNE_LAYOUT_PORT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

/**
 * Plotter layout port definition.
 */
class MNESHARED_EXPORT MneLayoutPort
{
public:
    MneLayoutPort() = default;
    ~MneLayoutPort() = default;

    int   portno = 0;      /**< Running number of this viewport. */
    int   invert = 0;      /**< Invert the signal coming to this port. */
    float xmin = 0, xmax = 0, ymin = 0, ymax = 0;  /**< Limits. */
    char  *names = nullptr; /**< Channels to go into this port (one line, separated by colons). */
    int   match = 0;        /**< Does this port match with our present channel? */
};

/** Backward-compatible typedef aliases. */
typedef MneLayoutPort  mneLayoutPortRec;
typedef MneLayoutPort* mneLayoutPort;

} // namespace MNELIB

#endif // MNE_LAYOUT_PORT_H
