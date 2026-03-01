#ifndef MNE_LAYOUT_H
#define MNE_LAYOUT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_layout_port.h"

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

/**
 * Plotter layout.
 */
class MNESHARED_EXPORT MneLayout
{
public:
    MneLayout() = default;
    ~MneLayout() = default;

    char          *name = nullptr;   /**< File where this came from. */
    float         xmin = 0, xmax = 0, ymin = 0, ymax = 0;     /**< The VDC limits. */
    float         cxmin = 0, cxmax = 0, cymin = 0, cymax = 0; /**< The confined VDC limits. */
    int           nport = 0;         /**< Number of viewports. */
    MneLayoutPort *ports = nullptr;  /**< Array of viewports. */
    int           **match = nullptr; /**< Matching matrix. */
    int           nmatch = 0;        /**< How many channels in matching matrix. */
};

/** Backward-compatible typedef aliases. */
typedef MneLayout  mneLayoutRec;
typedef MneLayout* mneLayout;

} // namespace MNELIB

#endif // MNE_LAYOUT_H
