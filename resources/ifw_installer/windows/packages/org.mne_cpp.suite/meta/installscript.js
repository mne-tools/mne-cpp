//--------------------------------------------------------------------------------------------------------------
//
// @file     installscript.qs
// @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
// @since    0.1.0
// @date     January, 2016
//
// @section  LICENSE
//
// Copyright (C) 2016, Christoph Dinh. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that
// the following conditions are met:
//     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
//       following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
//       the following disclaimer in the documentation and/or other materials provided with the distribution.
//     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
//       to endorse or promote products derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//
// @brief    MNE-CPP installer script
//
//--------------------------------------------------------------------------------------------------------------

function Component()
{
    // default constructor


    installer.installationFinished.connect(this, Component.prototype.installationFinishedPageIsShown);
    installer.finishButtonClicked.connect(this, Component.prototype.installationFinished);
}


//*************************************************************************************************************

Component.prototype.createOperations = function()
{
    try {
        component.createOperations();

        // create mne library shortcuts
        if (installer.value("os") === "win") {
            //Shortcuts
            component.addOperation("CreateShortcut", "@TargetDir@/MNECppMaintenanceTool.exe", "@StartMenuDir@/MNECppMaintenanceTool.exe.lnk", "workingDirectory=@TargetDir@");//"iconPath=@TargetDir@/App.ico");
        }
    } catch (e) {
        print(e);
    }
}


//*************************************************************************************************************

Component.prototype.installationFinishedPageIsShown = function()
{
    try {
        if (installer.isInstaller() && installer.status == QInstaller.Success) {
            installer.addWizardPageItem( component, "VCRedistCheckBoxForm", QInstaller.InstallationFinished );
        }
    } catch(e) {
        console.log(e);
    }
}


//*************************************************************************************************************

Component.prototype.installationFinished = function()
{
    try {
        if (installer.isInstaller() && installer.status === QInstaller.Success) {
            var isVCRedistCheckBoxChecked = component.userInterface( "VCRedistCheckBoxForm" ).vcRedistCheckBox.checked;
            if (isVCRedistCheckBoxChecked) {
                QDesktopServices.openUrl("file:///" + installer.value("TargetDir") + "/vcredist_x64.exe");
            }
        }
    } catch(e) {
        console.log(e);
    }
}
