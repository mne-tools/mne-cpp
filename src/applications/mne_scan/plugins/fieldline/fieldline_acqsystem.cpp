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

#define QUOTE(str) #str
#define EXPAND_AND_QUOTE(str) QUOTE(str)

const char libPythonBugFix[] = "libpython" 
                                EXPAND_AND_QUOTE(PYTHON_VERSION_MAJOR)
                                "."
                                EXPAND_AND_QUOTE(PYTHON_VERSION_MINOR)
                                ".so";

#include <dlfcn.h>
#include <iomanip>
#include <string>
#include <iostream>
#include <thread>
#include <chrono>

#include <QCoreApplication>

#include "fieldline/fieldline_acqsystem.h"

namespace FIELDLINEPLUGIN {

const std::string resourcesPath(QCoreApplication::applicationDirPath().toStdString() +
                                                                "/../resources/mne_scan/plugins/fieldline/");
const std::string entryFile(resourcesPath + "main.py");

bool restartFinished = false;

void callbackOnFinishedWhileRestart(int chassisId, int sensorId) {
    std::cout << "Sensor: (" << chassisId << ", " << sensorId << ") Finished restart.\n";
}

void callbackOnFinishedWhileCoarseZero(int chassisId, int sensorId) {
    std::cout << "Sensor: (" << chassisId << ", " << sensorId << ") Finished coarse zero.\n";
}

void callbackOnFinishedWhileFineZero(int chassisId, int sensorId) {
    std::cout << "Sensor: (" << chassisId << ", " << sensorId << ") Finished fine zero.\n";
}

void callbackOnErrorWhileRestart(int chassisId, int sensorId, int errorId) {
    std::cout << "Error while restarting sensor: (" << chassisId << ", " << sensorId << ") Code: " << errorId << ".\n";
}

void callbackOnErrorWhileCoarseZero(int chassisId, int sensorId, int errorId) {
    std::cout << "Error while restarting sensor: (" << chassisId << ", " << sensorId << ") Code: " << errorId << ".\n";
}

void callbackOnErrorWhileFineZero(int chassisId, int sensorId, int errorId) {
    std::cout << "Error while restarting sensor: (" << chassisId << ", " << sensorId << ") Code: " << errorId << ".\n";
}

void callbackOnCompletionRestart() {
    std::cout << "About to change restartFinished\n";
    restartFinished = true;
    std::cout << "Restart of sensors, finished.\n";
}

void callbackOnCompletionCoarseZero() {
    std::cout << "Coarse zero of sensors, finished.\n";
}

void callbackOnCompletionFineZero() {
    std::cout << "Fine zero of sensors, finished.\n";
}

static PyObject* callbackOnFinishedWhileRestart_py(PyObject* self, PyObject* args) {
    int chassis, sensor;
    if (!PyArg_ParseTuple(args, "ii", &chassis, &sensor)) {
        return NULL;
    }
    callbackOnFinishedWhileRestart(chassis, sensor);
    return Py_None;
}

static PyObject* callbackOnFinishedWhileCoarseZero_py(PyObject* self, PyObject* args) {
    int chassis, sensor;
    if (!PyArg_ParseTuple(args, "ii", &chassis, &sensor)) {
        return NULL;
    }
    callbackOnFinishedWhileCoarseZero(chassis, sensor);
    return Py_None;
}

static PyObject* callbackOnFinishedWhileFineZero_py(PyObject* self, PyObject* args) {
    int chassis, sensor;
    if (!PyArg_ParseTuple(args, "ii", &chassis, &sensor)) {
        return NULL;
    }
    callbackOnFinishedWhileFineZero(chassis, sensor);
    return Py_None;
}

static PyObject* callbackOnErrorWhileRestart_py(PyObject* self, PyObject* args) {
    int chassis, sensor, error;
    if (!PyArg_ParseTuple(args, "iii", &chassis, &sensor, &error)) {
        return NULL;
    }
    callbackOnErrorWhileRestart(chassis, sensor, error);
    return Py_None;
}

static PyObject* callbackOnErrorWhileCoarseZero_py(PyObject* self, PyObject* args) {
    int chassis, sensor, error;
    if (!PyArg_ParseTuple(args, "iii", &chassis, &sensor, &error)) {
        return NULL;
    }
    callbackOnErrorWhileCoarseZero(chassis, sensor, error);
    return Py_None;
}

static PyObject* callbackOnErrorWhileFinzeZero_py(PyObject* self, PyObject* args) {
    int chassis, sensor, error;
    if (!PyArg_ParseTuple(args, "iii", &chassis, &sensor, &error)) {
        return NULL;
    }
    callbackOnErrorWhileFineZero(chassis, sensor, error);
    return Py_None;
}

static PyObject* callbackOnCompletionRestart_py(PyObject* self, PyObject* args) {
    Py_ssize_t arg_count = PyTuple_Size(args);
    if (arg_count != 0) {
        PyErr_SetString(PyExc_ValueError, "This function should be called with no arguments.");
        return nullptr;
    }
    callbackOnCompletionRestart();
    return Py_None;
}

static PyObject* callbackOnCompletionCoarseZero_py(PyObject* self, PyObject* args) {
    Py_ssize_t arg_count = PyTuple_Size(args);
    if (arg_count != 0) {
        PyErr_SetString(PyExc_ValueError, "This function should be called with no arguments.");
        return nullptr;
    }
    callbackOnCompletionCoarseZero();
    return Py_None;
}

static PyObject* callbackOnCompletionFineZero_py(PyObject* self, PyObject* args) {
    Py_ssize_t arg_count = PyTuple_Size(args);
    if (arg_count != 0) {
        PyErr_SetString(PyExc_ValueError, "This function should be called with no arguments.");
        return nullptr;
    }
    callbackOnCompletionFineZero();
    return Py_None;
}

static struct PyMethodDef fieldlineCallbacksMethods[] = {
    {"callbackOnFinishedWhileRestart", callbackOnFinishedWhileRestart_py,
        METH_VARARGS, "Function to be called when a sensor has finished its restart."},
    {"callbackOnFinishedWhileCoarseZero", callbackOnFinishedWhileCoarseZero_py,
        METH_VARARGS, "Function to be called when a sensor has finished its coarse zeroing."},
    {"callbackOnFinishedWhileFineZero", callbackOnFinishedWhileFineZero_py,
        METH_VARARGS, "Function to be called when a sensor has finished its fine zeroing."},
    {"callbackOnErrorWhileRestart", callbackOnErrorWhileRestart_py,
        METH_VARARGS, "Function to be called when a sensor has an error while restarting."},
    {"callbackOnErrorWhileCoarseZero", callbackOnErrorWhileCoarseZero_py,
        METH_VARARGS, "Function to be called when a sensor has an error while coarse zeroing."},
    {"callbackOnErrorWhileFinzeZero", callbackOnErrorWhileFinzeZero_py,
        METH_VARARGS, "Function to be called when a sensor has an error while fine zeroing"},
    {"callbackOnCompletionRestart", callbackOnCompletionRestart_py,
        METH_VARARGS, "Function to be called when restarting has finished for all sensors."},
    {"callbackOnCompletionCoarseZero", callbackOnCompletionCoarseZero_py,
        METH_VARARGS, "Function to be called when coarse zeroing has finished for all sensors."},
    {"callbackOnCompletionFineZero", callbackOnCompletionFineZero_py,
        METH_VARARGS, "Function to be called when fine zeroing has finished for all sensors."},
    {NULL, NULL, 0, NULL}
};

static PyModuleDef FieldlineCallbacksModule = {
    PyModuleDef_HEAD_INIT, "fieldline_callbacks", NULL, -1, fieldlineCallbacksMethods,
    NULL, NULL, NULL, NULL
};

static PyObject* PyInit_fieldline_callbacks(void) {
    return PyModule_Create(&FieldlineCallbacksModule);
}

static FieldlineAcqSystem* acq_system(nullptr);

static PyObject* dict_parser(PyObject* self, PyObject* args) {
    PyObject* dataDict;
    if (!PyArg_ParseTuple(args, "O", &dataDict)) {
        PyErr_SetString(PyExc_TypeError, "data is not valid.");
        return NULL;
    }

    PyObject* data_frames = PyDict_GetItemString(dataDict, "data_frames");
    if (!data_frames || !PyDict_Check(data_frames)) {
        PyErr_SetString(PyExc_TypeError, "data_frames is not a dictionary.");
        return NULL;
    }

    PyObject* key;
    PyObject* value;
    Py_ssize_t pos = 0;

    while (PyDict_Next(data_frames, &pos, &key, &value)) {
        PyObject* data = PyDict_GetItemString(value, "data");
        // printLog(std::string("pos: ") + std::to_string(pos));
        // printLog(std::string("value: ") + std::to_string((double) PyLong_AsLong(data)));
        acq_system->addSampleToSamplesColumn((int) pos-1, (double) PyLong_AsLong(data));
    }
    return Py_None;
}

static struct PyMethodDef callbacksParseMethods[] = {
  {"dict_parser", dict_parser, METH_VARARGS, "Parse dictionary"},
  {NULL, NULL, 0, NULL}
};

static PyModuleDef CallbacksParseModule = {
  PyModuleDef_HEAD_INIT, "callbacks_parsing", NULL, -1, callbacksParseMethods,
  NULL, NULL, NULL, NULL
};

static PyObject* PyInit_callbacks_parsing(void) {
  return PyModule_Create(&CallbacksParseModule);
}

FieldlineAcqSystem::FieldlineAcqSystem(Fieldline* parent)
: m_pControllerParent(parent),
  m_numSamplesPerBlock(200),
  m_numSensors(33)
{
    printLog("Initializing Python");
    printLog(libPythonBugFix);
    void*const libBugFix = dlopen(libPythonBugFix, RTLD_LAZY | RTLD_GLOBAL);
    
    Py_Initialize();

    preConfigurePython();

    m_pCallbackModule = loadModule("callback");


    m_pCallsModule = loadCModule("fieldline_callbacks", *(void*(*)(void))&PyInit_fieldline_callbacks);
    PyObject* parseCallbacksModule = (PyObject*)loadCModule("callbacks_parsing", *(void*(*)(void))&PyInit_callbacks_parsing);
    if (parseCallbacksModule == NULL)
    {
      printLog("callbacks module wrong!");
    } else{
      printLog("callbacks module ok!");
    }

   // PyObject* FieldlineModule = (PyObject*)loadModule("fieldline_api_mock.fieldline_service");
    PyObject* FieldlineModule = (PyObject*)loadModule("fieldline_api.fieldline_service");
    if (FieldlineModule == NULL)
    {
      printLog("fieldline module wrong!");
    } else{
      printLog("fieldline module ok!");
    }

    PyObject* fService = PyObject_GetAttrString(FieldlineModule, "FieldLineService");
    if (fService == NULL)
    {
      printLog("fservice wrong!");
    } else{
      printLog("fservice ok!");
    }
    PyObject* ipList = Py_BuildValue("([ss])", "172.21.16.181", "172.21.16.139");
    PyObject* fServiceInstance = PyObject_CallObject(fService, ipList);
    m_fServiceInstance = (void*) fServiceInstance;
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


    m_pThreadState = (void*)PyEval_SaveThread();

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

        PyObject* pResult = PyObject_CallNoArgs(openMethod);
        if (pResult == NULL)
        {
          printLog("pResult wrong!");
        } else{
          printLog("pResult ok!");
        }

    PyGILState_Release(gstate);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    // PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();


        PyObject* setCloseLoop = PyObject_GetAttrString(fServiceInstance, "set_closed_loop");
        PyObject* trueTuple = PyTuple_New(1);
        PyTuple_SetItem(trueTuple, 0, Py_True);
        PyObject* pResult2 = PyObject_CallObject(setCloseLoop, trueTuple);

        PyObject* loadSensors = PyObject_GetAttrString(fServiceInstance, "load_sensors");
        PyObject* sensors = PyObject_CallNoArgs(loadSensors);
        PyObject* restartAllSensors = PyObject_GetAttrString(fServiceInstance, "restart_sensors");

        if (restartAllSensors == NULL)
        {
            printLog("restart sensors broken");
        } else {
            printLog("restartAllSensors ok!");
        }

        PyObject* callback_on_finished = PyObject_GetAttrString((PyObject*)m_pCallsModule, "callbackOnFinishedWhileRestart");
        if (callback_on_finished == NULL)
        {
            printLog("callback on finished broken");
        } else {
            printLog("callback restart ok!");
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        PyObject* callback_on_error = PyObject_GetAttrString((PyObject*)m_pCallsModule, "callbackOnErrorWhileRestart");
        if (callback_on_error == NULL)
        {
            printLog("callback on error broken");
        } else {
            printLog("callback error ok!");
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        PyObject* callback_on_completion = PyObject_GetAttrString((PyObject*)m_pCallsModule, "callbackOnCompletionRestart");
        if (callback_on_completion == NULL)
        {
            printLog("callback on completion broken");
        } else {
            printLog("callback completion ok!");
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        PyObject* argsRestart = PyTuple_New(4);
        PyTuple_SetItem(argsRestart, 0, sensors);
        PyTuple_SetItem(argsRestart, 1, callback_on_finished);
        PyTuple_SetItem(argsRestart, 2, callback_on_error);
        PyTuple_SetItem(argsRestart, 3, callback_on_completion);

        restartFinished = false;

        PyObject* resultRestart = PyObject_CallObject(restartAllSensors, argsRestart);
        if (resultRestart == NULL)
        {
            printLog("restart call broken");
        } else {
            printLog("restart call ok!");
        }

    PyGILState_Release(gstate);
     // while (restartFinished == false) {
         // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
         // printLog("waiting...");
     // }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    //  PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();


    PyObject* readDataFcn = PyObject_GetAttrString(fServiceInstance, "read_data");
    if (readDataFcn == NULL) {
        printLog("problem readDataFcn");
    } else {
        printLog("readDataFcn ok");
    }

    PyObject* parserCallback = PyObject_GetAttrString(parseCallbacksModule, "dict_parser");
    if (parserCallback == NULL) {
        printLog("problem parserCallback");
    } else {
        printLog("parserCallback ok");
    }
    PyObject* argsSetDataParser = PyTuple_New(1);
    PyTuple_SetItem(argsSetDataParser, 0, parserCallback);

    PyObject* result33 = PyObject_CallObject(readDataFcn, argsSetDataParser);
    if (result33 == NULL)
    {
        printLog("result33 bad");
    } else {
        printLog("result33 ok");
    }

    PyGILState_Release(gstate);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    // PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    // register "this" into somewhere findable by python's execution flow.
    acq_system = this;
    initSampleArrays();

    Py_DECREF(parseCallbacksModule);
    Py_DECREF(FieldlineModule);
    Py_DECREF(fService);
    Py_DECREF(ipList);
    Py_DECREF(fServiceInstance);
    Py_DECREF(openMethod);
    Py_DECREF(pResult);
    Py_DECREF(setCloseLoop);
    Py_DECREF(trueTuple);
    Py_DECREF(pResult2);
    Py_DECREF(loadSensors);
    Py_DECREF(restartAllSensors);
    Py_DECREF(callback_on_finished);
    Py_DECREF(callback_on_error);
    Py_DECREF(callback_on_completion);
    Py_DECREF(argsRestart);
    Py_DECREF(resultRestart);
    Py_DECREF(readDataFcn);
    Py_DECREF(parserCallback);
    Py_DECREF(argsSetDataParser);
    Py_DECREF(result33);

    PyGILState_Release(gstate);
    // m_pThreadState = (void*)PyEval_SaveThread();
}

FieldlineAcqSystem::~FieldlineAcqSystem()
{
    PyEval_RestoreThread(reinterpret_cast<PyThreadState*>(m_pThreadState));

    Py_XDECREF(m_pCallbackModule);
    Py_XDECREF(m_pCallsModule);

    if (m_samplesBlock) {
        delete[] m_samplesBlock;
    }
    if (m_samplesBlock2) {
        delete[] m_samplesBlock2;
    }

    Py_Finalize();
}

void FieldlineAcqSystem::setCallback()
{
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    // PyObject *pSetCallbackFunc = NULL;
    // PyObject *pCallback1 = NULL;
    // PyObject *pArgs = NULL;
    // PyObject *pResult = NULL;
    //
    // // Get a reference to the function
    // pSetCallbackFunc = PyObject_GetAttrString((PyObject*)m_pCallbackModule, "set_callback");
    // if (pSetCallbackFunc == NULL) {
    //     printLog(std::string("Error finding function: ").append("set_callback").c_str());
    //     PyErr_Print();
    // }
    //
    // pCallback1 = PyObject_GetAttrString((PyObject*)m_pCallsModule, "callback1");
    // if (pCallback1 && PyCallable_Check(pCallback1)) {
    //     pArgs = PyTuple_New(1);
    //     PyTuple_SetItem(pArgs, 0, pCallback1);
    // } else {
    //     if (PyErr_Occurred())
    //         PyErr_Print();
    //     printLog(std::string("Cannot find function callback1 in calls module."));
    // }
    // // Call the set_callback function
    // pResult = PyObject_CallObject(pSetCallbackFunc, pArgs);
    // if (pResult == NULL) {
    //     printLog(std::string("Error calling function: ").append("set_callback").c_str());
    //     PyErr_Print();
    // }
    // Py_XDECREF(pSetCallbackFunc);
    // Py_XDECREF(pCallback1);
    // Py_XDECREF(pArgs);
    // Py_XDECREF(pResult);
    //
    PyGILState_Release(gstate);
}

void FieldlineAcqSystem::startADC() {
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    PyObject* start_data = PyObject_GetAttrString((PyObject*)m_fServiceInstance, "start_adc");
    PyObject* argsStartData = PyTuple_New(1);
    PyObject* zeroArg = PyLong_FromLong(0);
    PyTuple_SetItem(argsStartData, 0, zeroArg);
    PyObject* startResult = PyObject_CallObject(start_data, argsStartData);
    if (startResult == NULL)
    {
        printLog("startResult bad");
    } else {
        printLog("startResult ok");
    }

    Py_DECREF(argsStartData);
    Py_DECREF(zeroArg);
    Py_DECREF(startResult);

    PyGILState_Release(gstate);
}

void FieldlineAcqSystem::stopADC() {
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    PyObject* stop_data = PyObject_GetAttrString((PyObject*)m_fServiceInstance, "stop_adc");
    PyObject* argsstopData = PyTuple_New(1);
    PyObject* stopArg = PyLong_FromLong(0);
    PyTuple_SetItem(argsstopData, 0, stopArg);
    PyObject* stopResult = PyObject_CallObject(stop_data, argsstopData);
    if (stopResult == NULL)
    {
        printLog("stopResult bad");
    } else {
        printLog("stopResult ok");
    }

    Py_DECREF(argsstopData);
    Py_DECREF(stopArg);
    Py_DECREF(stopResult);

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

    Py_DECREF(pModule);

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
        PyObject* PyModule = (PyObject*)moduleInitFunc();
        PyObject* sys_modules = PyImport_GetModuleDict();
        PyDict_SetItemString(sys_modules, moduleName, PyModule);
        Py_DECREF(PyModule);
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

void FieldlineAcqSystem::callFunction(const std::string& moduleName, const std::string& funcName)
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
            printLog("Check the Path.");
        }
    }

    PyObject* pFunc = PyObject_GetAttrString(pModule, funcName.c_str());
    if (pFunc == NULL) {
        printLog(std::string("Error finding function: ").append(funcName).c_str());
        PyErr_Print();
    }

    PyObject* pResult = PyObject_CallObject(pFunc, NULL);
    if (pResult == NULL) {
        printLog(std::string("Error calling function: ").append(funcName).c_str());
        PyErr_Print();
    }

    Py_XDECREF(pModule);
    Py_XDECREF(pFunc);
    Py_XDECREF(pResult);

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
    PyObject* resourcesObj = PyUnicode_FromString(resourcesPath.c_str());
    PyList_Insert(path, 0, resourcesObj);
    Py_DECREF(sys);
    Py_DECREF(resourcesObj);

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

void FieldlineAcqSystem::initSampleArrays()
{
    m_samplesBlock = new double[m_numSensors * m_numSamplesPerBlock];
    m_samplesBlock2 = new double[m_numSensors * m_numSamplesPerBlock];
}

void FieldlineAcqSystem::addSampleToSamplesColumn(int sensorIdx, double value)
{
    static int sampleIdx = 0;
    // printLog(std::string("sensorIdx: ") + std::to_string(sensorIdx));
    // printLog(std::string("sampleIdx: ") + std::to_string(sampleIdx));
    // printLog(std::string("m_numSensors = ") + std::to_string(m_numSensors));
    // printLog(std::string("m_numSamplesPerBlock = ") + std::to_string(m_numSamplesPerBlock));
    // printLog(std::string("m_samplesBlock[").append(std::to_string(sensorIdx * m_numSamplesPerBlock + sampleIdx)).append("] = ").append(std::to_string(value)));
    m_samplesBlock[sensorIdx * m_numSamplesPerBlock + sampleIdx] = value;
    if (sensorIdx == m_numSensors -1) {
        sampleIdx++;
        if (sampleIdx == m_numSamplesPerBlock) {
            sampleIdx = 0;
            m_pControllerParent->newData(m_samplesBlock, m_numSensors, m_numSamplesPerBlock);
            // memcpy((void*)m_samplesBlock2, (void*)m_samplesBlock, m_numSamplesPerBlock * m_numSensors);
            // std::thread([this](){
            //     m_pControllerParent->newData(m_samplesBlock2, m_numSensors, m_numSamplesPerBlock);
            // });
        }
    }
}

}  // namespace FIELDLINEPLUGIN

