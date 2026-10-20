// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 MNE-CPP Authors
//   Christoph Dinh <christoph.dinh@mne-cpp.org>
//
// Installs MNE-Python into its own virtual environment <install dir>/mne-python,
// so neither the system Python nor PEP 668 protected installations are touched.

function Component()
{
}

Component.prototype.createOperations = function()
{
    component.createOperations();
    var env = installer.value("TargetDir") + "/mne-python";
    var failed = "errormessage=MNE-Python could not be installed. It needs Python 3.10 or newer and an internet connection.";

    if (systemInfo.kernelType === "winnt") {
        var winEnv = installer.toNativeSeparators(env);
        component.addOperation("Execute", "cmd.exe", "/C",
            "(py -3 -m venv \"" + winEnv + "\" || python -m venv \"" + winEnv + "\") && \"" + winEnv +
            "\\Scripts\\python.exe\" -m pip install --upgrade pip mne",
            failed,
            "UNDOEXECUTE", "cmd.exe", "/C", "rmdir /S /Q \"" + winEnv + "\"");
    } else {
        component.addOperation("Execute", "/bin/sh", "-c",
            "python3 -c 'import sys; sys.exit(sys.version_info < (3, 10))' && python3 -m venv \"" + env +
            "\" && \"" + env + "/bin/python\" -m pip install --upgrade pip mne",
            failed,
            "UNDOEXECUTE", "/bin/rm", "-rf", env);
    }
}
