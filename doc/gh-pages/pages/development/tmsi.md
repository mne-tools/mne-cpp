---
title: TMSI
parent: MNE Scan
grand_parent: Develop
nav_order: 9
---
# TMSI

The MNE Scan TMSI plugin adds support for TMSI Refa amplifiers. The plugin is Windows only since the TMSI SDK ships for Windows operating systems. There are no additional steps needed to build the plugin. However, you need to make sure that the `TMSiSDK.dll` is present in `C:/Windows/System32/TMSiSDK.dll`. This is where the plugin will try to load the DLL from.