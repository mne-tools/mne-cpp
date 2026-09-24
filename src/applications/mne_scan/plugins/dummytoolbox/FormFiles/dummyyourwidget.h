//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     dummyyourwidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     January, 2016
 * @brief    Contains the declaration of the DummyYourWidget class.
 */

#ifndef DUMMYYOURWIDGET_H
#define DUMMYYOURWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

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

namespace Ui{
    class DummyYourWidgetGui;
}

//=============================================================================================================
// DEFINE NAMESPACE DUMMYTOOLBOXPLUGIN
//=============================================================================================================

namespace DUMMYTOOLBOXPLUGIN
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * DECLARE CLASS DummyYourWidget
 *
 * @brief The DummyToolbox class provides a dummy widget.
 */
class DummyYourWidget : public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<DummyYourWidget> SPtr;         /**< Shared pointer type for DummyYourWidget. */
    typedef QSharedPointer<DummyYourWidget> ConstSPtr;    /**< Const shared pointer type for DummyYourWidget. */

    //=========================================================================================================
    /**
     * Constructs a DummyToolbox.
     */
    explicit DummyYourWidget(const QString& sSettingsPath = "",
                             QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the DummyToolbox.
     */
    ~DummyYourWidget();

private:
    //=========================================================================================================
    /**
     * Saves all important settings of this view via QSettings.
     */
    void saveSettings();

    //=========================================================================================================
    /**
     * Loads and inits all important settings of this view via QSettings.
     */
    void loadSettings();

    Ui::DummyYourWidgetGui*     m_pUi;              /**< The UI class specified in the designer. */
    QString                     m_sSettingsPath;    /**< The settings path to store the GUI settings to. */

};
}   //namespace

#endif // DUMMYYOURWIDGET_H
