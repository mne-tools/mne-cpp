//=============================================================================================================
/**
 * @file     fieldline_view.h
 * @author   Juan Garcia-Prieto <jgarciaprieto@mgh.harvard.edu>
 *           Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.9
 * @date     February, 2023
 *
 * @section  LICENSE
 *
 * Copyright (C) 2023, Gabriel Motta, Juan Garcia-Prieto. All rights reserved.
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
 * @brief     FieldlineView class declaration.
 *
 */

#ifndef FIELDLINE_FIELDLINEVIEW_H
#define FIELDLINE_FIELDLINEVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <vector>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
class uiFieldlineView;
}

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================
class QHBoxLayout;
class QTableWidget;
class QTableWidgetItem;

namespace FIELDLINEPLUGIN {

class Fieldline;
// class FieldlineViewChassis;

//=============================================================================================================

class FieldlineView : public QWidget
{
    Q_OBJECT

 public:
    explicit FieldlineView(Fieldline* parent);
    ~FieldlineView();

private:
    void initAcqSystem(int numChassis);
    void initTopMenu();
    void disconnect();
    void macIpTableDoubleClicked(int, int);
    void macIpTableValueChanged(QTableWidgetItem* item);

    void initAcqSystem();
    void initAcqSystemCallbacks();
    void initAcqSystemTopButtons();
    void setNumRowsIpMacFrame(int i);

    void startAllSensors();
    void stopAllSensors();
    void autoTuneAllSensors();
    void restartAllSensors();
    void coarseZeroAllSensors();
    void fineZeroAllSensors();

    Fieldline* m_pFieldlinePlugin;
    Ui::uiFieldlineView* m_pUi;
    QTableWidget* m_pMacIpTable;
    // std::vector<FieldlineViewChassis> m_pAcqSystem;
};

}  // namespace FIELDLINEPLUGIN

#endif  // FIELDLINE_UI_VIEW_H
