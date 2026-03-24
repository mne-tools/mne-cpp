//=============================================================================================================
/**
 * @file     irawdataview.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Declares the extension-facing raw data view interface used by the workbench.
 */

#ifndef MNE_ANALYZE_STUDIO_IRAWDATAVIEW_H
#define MNE_ANALYZE_STUDIO_IRAWDATAVIEW_H

#include <QString>

namespace MNEANALYZESTUDIO
{

/**
 * @brief Narrow interface for workbench interactions with an active raw data browser view.
 */
class IRawDataView
{
public:
    virtual ~IRawDataView() = default;

    virtual QString filePath() const = 0;
    virtual QString summaryText() const = 0;
    virtual QString stateText() const = 0;
    virtual bool gotoSample(int sample) = 0;
    virtual bool setZoomPixelsPerSample(double pixelsPerSample) = 0;
    virtual double pixelsPerSample() const = 0;
    virtual int cursorSample() const = 0;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_IRAWDATAVIEW_H
