//=============================================================================================================
/**
 * @file         fieldline_system_controller.cpp
 * @author   Juan GarciaPrieto <jgarciaprieto@mgh.harvard.edu>;
 *                   Gabriel B Motta <gbmotta@mgh.harvard.edu>;
 * @since        0.1.0
 * @date         February, 2023
 *
 * @section  LICENSE
 *
 * Copyright (C) 2023, Juan G Prieto, Gabriel B Motta. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *       * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *       * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *       * Neither the name of MNE-CPP authors nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief        Contains the definition of the Fieldline class.
 *
 */
#define PY_SSIZE_T_CLEAN
#include "Python.h"

#include <iomanip>
#include <string>
#include <iostream>
#include <thread>
#include <chrono>

#include <QCoreApplication>

#include "fieldline/fieldline_acqsystem.h"

//extern "C" {
//
//PyObject *restartFinished(PyObject *self, PyObject *args) {
//  long chassis, sensor;
//  if (PyArg_ParseTuple(args, "ii", &chassis, &sensor)) {
//      std::cout << std::setfill('0') << std::setw(2) << chassis << ":" << sensor
//                          << " - restart finished;\n";
//  } else {
//      std::cout << "A sensor has finished restarting!\n";
//  }
//
//  return NULL;
//}
//PyObject *coarseZeroFinished(PyObject *self, PyObject *args) {
//  long chassis, sensor;
//  if (PyArg_ParseTuple(args, "ii", &chassis, &sensor)) {
//      std::cout << std::setfill('0') << std::setw(2) << chassis << ":" << sensor
//                          << " - coarse zero finished;\n";
//  } else {
//      std::cout << "A sensor has finished coarse zeroing!\n";
//  }
//
//  return NULL;
//}
//PyObject *fineZeroFinished(PyObject *self, PyObject *args) {
//  long chassis, sensor;
//  if (PyArg_ParseTuple(args, "ii", &chassis, &sensor)) {
//      std::cout << std::setfill('0') << std::setw(2) << chassis << ":" << sensor
//                          << " - fine zero finished;\n";
//  } else {
//      std::cout << "A sensor has finished fine zeroing!\n";
//  }
//
//  return NULL;
//}
//}
//
//static PyMethodDef my_module_methods[] = {
//      {"restartFinished", restartFinished, METH_VARARGS, " "},
//      {"coarseZeroFinished", coarseZeroFinished, METH_VARARGS, " "},
//      {"fineZeroFinished", fineZeroFinished, METH_VARARGS, " "},
//      {NULL, NULL, 0, NULL}};
//
//static PyModuleDef my_module_def = {
//      PyModuleDef_HEAD_INIT,
//      "mne_cpp_callbacks",
//      "A module of callback functions for mne-cpp.",
//      -1,
//      my_module_methods,
//      NULL,
//      NULL,
//      NULL,
//      NULL};
//
//PyMODINIT_FUNC PyInit_my_module(void) {
//  return PyModule_Create(&my_module_def);
//}

namespace FIELDLINEPLUGIN {

const std::string resourcesPath(QCoreApplication::applicationDirPath().toStdString() +
                                                                "/../resources/mne_scan/plugins/fieldline/");
const std::string entryFile(resourcesPath + "main.py");


void callback1() {
    printLog("This is callback1.");
}

static PyObject* callback1_py(PyObject* self, PyObject* args) {
    callback1();
    return Py_None;
}

static struct PyMethodDef callbacksMethods[] = {
    {"callback1", callback1_py, METH_VARARGS, "Call to callback1"},
    {NULL, NULL, 0, NULL}
};

static PyModuleDef CallModule = {
    PyModuleDef_HEAD_INIT, "calls", NULL, -1, callbacksMethods,
    NULL, NULL, NULL, NULL
};

static PyObject* PyInit_calls(void) {
    return PyModule_Create(&CallModule);
}

FieldlineAcqSystem::FieldlineAcqSystem(Fieldline* parent)
: m_pControllerParent(parent)
{
    Py_Initialize();

    preConfigurePython();

    m_pCallbackModule = loadModule("callback");


    m_pCallsModule = loadCModule("calls", *(void*(*)(void))&PyInit_calls);

    PyObject* FieldlineModule = (PyObject*)loadModule("fieldline_api_mock.fieldline_service");

    PyObject* fService = PyObject_GetAttrString(FieldlineModule, "FieldLineService");
    if (fService == NULL)
    {
      printLog("fservice wrong!");
    } else{
      printLog("fservice ok!");
    }
    PyObject* ipList = Py_BuildValue("([ss])", "8.8.8.8", "1.1.1.1");
    PyObject* fServiceInstance = PyObject_CallObject(fService, ipList);
    printLog("here!!!");
    if (fServiceInstance == NULL)
    {
      printLog("fServiceInstance wrong!");
    } else{
      printLog("fServiceInstance ok!");
    }
    PyObject* openMethod = PyObject_GetAttrString(fServiceInstance, "open");
    if (openMethod == NULL)
    {
      printLog("openMethod wrong!");
    } else{
      printLog("openMethod ok!");
    }
    PyObject* pResult = PyObject_CallMethodNoArgs(fServiceInstance, PyUnicode_FromString("open"));
    if (pResult == NULL)
    {
      printLog("pResult wrong!");
    } else{
      printLog("pResult ok!");
    }
    PyObject* pResult2 = PyObject_CallNoArgs(openMethod);
    if (pResult2 == NULL)
    {
      printLog("pResult2 wrong!");
    } else{
      printLog("pResult2 ok!");
    }

    Py_DECREF(FieldlineModule);
    Py_DECREF(fService);
    Py_DECREF(ipList);
    Py_DECREF(fServiceInstance);
    Py_DECREF(openMethod);
    Py_DECREF(pResult);

    m_pThreadState = (void*)PyEval_SaveThread();
}

FieldlineAcqSystem::~FieldlineAcqSystem()
{
    PyEval_RestoreThread(reinterpret_cast<PyThreadState*>(m_pThreadState));
    Py_DECREF(m_pCallbackModule);
    Py_DECREF(m_pCallsModule);
    Py_Finalize();
}

void FieldlineAcqSystem::setCallback()
{
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    PyObject *pSetCallbackFunc = NULL;
    PyObject *pCallback1 = NULL;
    PyObject *pArgs = NULL;
    PyObject *pResult = NULL;

    // Get a reference to the function
    pSetCallbackFunc = PyObject_GetAttrString((PyObject*)m_pCallbackModule, "set_callback");
    if (pSetCallbackFunc == NULL) {
        printLog(std::string("Error finding function: ").append("set_callback").c_str());
        PyErr_Print();
    }

    pCallback1 = PyObject_GetAttrString((PyObject*)m_pCallsModule, "callback1");
    if (pCallback1 && PyCallable_Check(pCallback1)) {
        pArgs = PyTuple_New(1);
        PyTuple_SetItem(pArgs, 0, pCallback1);
    } else {
        if (PyErr_Occurred())
            PyErr_Print();
        printLog(std::string("Cannot find function callback1 in calls module."));
    }
    // Call the set_callback function
    pResult = PyObject_CallObject(pSetCallbackFunc, pArgs);
    if (pResult == NULL) {
        printLog(std::string("Error calling function: ").append("set_callback").c_str());
        PyErr_Print();
    }
    Py_XDECREF(pSetCallbackFunc);
    Py_XDECREF(pCallback1);
    Py_XDECREF(pArgs);
    Py_XDECREF(pResult);

    PyGILState_Release(gstate);
}

void* FieldlineAcqSystem::loadModule(const char* moduleName)
{
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    PyObject* pModule = PyImport_ImportModule(moduleName);
    if (pModule == NULL) {
        printLog(std::string("Error loading module ").append(moduleName).append(".").c_str());
        PyErr_Print();
    }

    PyGILState_Release(gstate);

    return (void*)pModule;
}

void* FieldlineAcqSystem::loadCModule(const char* moduleName, void*(*moduleInitFunc)(void))
{
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    if (Py_IsInitialized())
    {
        PyImport_AddModule(moduleName);
        PyObject* pyModule = (PyObject*)moduleInitFunc();
        PyObject* sys_modules = PyImport_GetModuleDict();
        PyDict_SetItemString(sys_modules, moduleName, pyModule);
        Py_DECREF(pyModule);
    } else {
        PyImport_AppendInittab(moduleName, (PyObject*(*)(void)) moduleInitFunc);
    }

    PyGILState_Release(gstate);

    return loadModule(moduleName);
}

void FieldlineAcqSystem::callFunctionAsync(const char* moduleName, const char* funcName)
{
        std::thread t([this, moduleName, funcName]() {
             this->callFunction(moduleName, funcName);
        });
        t.detach();
}

void FieldlineAcqSystem::callFunction(const std::string moduleName, const std::string funcName)
{
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    PyObject* pModule = PyImport_GetModule(PyUnicode_FromString(moduleName.c_str()));
    if (pModule == NULL) {
        printLog(std::string("Module ").append(moduleName).append(" has not been imported yet.").c_str());
        printLog(std::string("Attempting to import the module ").append(moduleName).append(".").c_str());
        pModule = PyImport_ImportModule(moduleName.c_str());
        if (pModule == NULL) {
            PyErr_Print();
            printLog(std::string("Import failed. Can't find ").append(moduleName).append(".").c_str());
        }
    }

    PyObject* pFunc = PyObject_GetAttrString(pModule, funcName.c_str());
    if (pFunc == NULL) {
        printLog(std::string("Error finding function: ").append(funcName).c_str());
        PyErr_Print();
        Py_DECREF(pModule);
    }

    PyObject* pResult = PyObject_CallObject(pFunc, NULL);
    if (pResult == NULL) {
        printLog(std::string("Error calling function: ").append(funcName).c_str());
        PyErr_Print();
        Py_DECREF(pModule);
        Py_DECREF(pFunc);
    }

    Py_DECREF(pModule);
    Py_DECREF(pFunc);
    Py_DECREF(pResult);

    PyGILState_Release(gstate);
}

void FieldlineAcqSystem::preConfigurePython() const
{
    PyObject* sys = PyImport_ImportModule("sys");
    PyObject* versionInfo = PyObject_GetAttrString(sys, "version_info");
    PyObject* versionInfoMajor = PyObject_GetAttrString(versionInfo, "major");
    PyObject* versionInfoMinor = PyObject_GetAttrString(versionInfo, "minor");
    const std::string pythonVer(std::to_string(PyLong_AsLong(versionInfoMajor)) + \
                                                "." + std::to_string(PyLong_AsLong(versionInfoMinor)));
    Py_DECREF(versionInfoMajor);
    Py_DECREF(versionInfoMinor);
    Py_DECREF(versionInfo);

    PyObject* path = PyObject_GetAttrString(sys, "path");
    PyList_Insert(path, 0, PyUnicode_FromString(resourcesPath.c_str()));
    Py_DECREF(sys);

    const std::string pathVenvMods(resourcesPath + "venv/lib/python" + pythonVer + "/site-packages/");
    printLog(pathVenvMods);
    PyList_Insert(path, 1, PyUnicode_FromString(pathVenvMods.c_str()));
    Py_DECREF(path);
}

void FieldlineAcqSystem::runPythonFile(const char* file, const char* comment) const
{
    FILE *py_file = fopen(file, "r");
    if (py_file)
    {
        PyObject* global_dict = PyDict_New();
        PyObject* local_dict = PyDict_New();
        PyObject* result = PyRun_File(py_file, comment, Py_file_input, global_dict, local_dict);
        Py_DECREF(global_dict);
        Py_DECREF(local_dict);
        Py_DECREF(result);
        fclose(py_file);
    }
}

}  // namespace FIELDLINEPLUGIN

