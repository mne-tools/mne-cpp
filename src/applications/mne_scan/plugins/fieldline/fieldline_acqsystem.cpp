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

struct GILHandler {
    GILHandler():isAcquired(false) { 
        acquireGIL();
    }

    ~GILHandler() {
        releaseGIL();
    }

    void acquireGIL() {
        if (!isAcquired) {
            m_gstate = PyGILState_Ensure();
            isAcquired = true;
        }
    }
    void releaseGIL() {
        if (isAcquired) {
            PyGILState_Release(m_gstate);
            isAcquired = false;
        }
    }

    bool isAcquired;
    PyGILState_STATE m_gstate;
};

struct AcqOpener {
    AcqOpener(PyObject* s): m_serviceInstance(s) {
        openAcq();
    }
    ~AcqOpener() {
        closeAcq();
    }

    void openAcq() {
        PyObject* openMethod = PyObject_GetAttrString(m_serviceInstance, "open");
        if (openMethod == NULL)
        {
            printLog("openMethod wrong!");
        } else{
            printLog("openMethod ok!");
        }

        PyObject* openMethodCall = PyObject_CallNoArgs(openMethod);
        if (openMethodCall == NULL)
        {
          printLog("openMethodCall wrong!");
        } else{
          printLog("openMethodCall ok!");
        }
        Py_XDECREF(openMethod);
        Py_XDECREF(openMethodCall);
    }

    void closeAcq() {
        PyObject* closeMethod = PyObject_GetAttrString(m_serviceInstance, "close");
        if (closeMethod == NULL)
            {
                printLog("closeMethod wrong!");
            } else{
            printLog("closeMethod ok!");
        }

        PyObject* closeMethodCall = PyObject_CallNoArgs(closeMethod);
        if (closeMethodCall == NULL)
        {
          printLog("closeMethodCall wrong!");
        } else{
          printLog("closeMethodCall ok!");
        }

        Py_XDECREF(closeMethod);
        Py_XDECREF(closeMethodCall);
    }

    PyObject* m_serviceInstance;

};

bool restartFinished = false;
bool coarseZeroFinished = false;
bool fineZeroFinished = false;

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
    std::cout << "Error while coarse zeroing sensor: (" << chassisId << ", " << sensorId << ") Code: " << errorId << ".\n";
}

void callbackOnErrorWhileFineZero(int chassisId, int sensorId, int errorId) {
    std::cout << "Error while fine zeroing sensor: (" << chassisId << ", " << sensorId << ") Code: " << errorId << ".\n";
}

void callbackOnCompletionRestart() {
    std::cout << "About to change restartFinished\n";
    restartFinished = true;
    std::cout << "Restarting sensors, finished.\n";
}

void callbackOnCompletionCoarseZero() {
    coarseZeroFinished = true;
    std::cout << "Coarse zero sensors, finished.\n";
}

void callbackOnCompletionFineZero() {
    fineZeroFinished = true;
    std::cout << "Fine zero sensors, finished.\n";
}

static PyObject* callbackOnFinishedWhileRestart_py(PyObject* self, PyObject* args) {
    int chassis, sensor;
    if (!PyArg_ParseTuple(args, "ii", &chassis, &sensor)) {
        return NULL;
    }
    callbackOnFinishedWhileRestart(chassis, sensor);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* callbackOnFinishedWhileCoarseZero_py(PyObject* self, PyObject* args) {
    int chassis, sensor;
    if (!PyArg_ParseTuple(args, "ii", &chassis, &sensor)) {
        return NULL;
    }
    callbackOnFinishedWhileCoarseZero(chassis, sensor);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* callbackOnFinishedWhileFineZero_py(PyObject* self, PyObject* args) {
    int chassis, sensor;
    if (!PyArg_ParseTuple(args, "ii", &chassis, &sensor)) {
        return NULL;
    }
    callbackOnFinishedWhileFineZero(chassis, sensor);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* callbackOnErrorWhileRestart_py(PyObject* self, PyObject* args) {
    int chassis, sensor, error;
    if (!PyArg_ParseTuple(args, "iii", &chassis, &sensor, &error)) {
        return NULL;
    }
    callbackOnErrorWhileRestart(chassis, sensor, error);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* callbackOnErrorWhileCoarseZero_py(PyObject* self, PyObject* args) {
    int chassis, sensor, error;
    if (!PyArg_ParseTuple(args, "iii", &chassis, &sensor, &error)) {
        return NULL;
    }
    callbackOnErrorWhileCoarseZero(chassis, sensor, error);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* callbackOnErrorWhileFinzeZero_py(PyObject* self, PyObject* args) {
    int chassis, sensor, error;
    if (!PyArg_ParseTuple(args, "iii", &chassis, &sensor, &error)) {
        return NULL;
    }
    callbackOnErrorWhileFineZero(chassis, sensor, error);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* callbackOnCompletionRestart_py(PyObject* self, PyObject* args) {
    Py_ssize_t arg_count = PyTuple_Size(args);
    if (arg_count != 0) {
        PyErr_SetString(PyExc_ValueError, "This function should be called with no arguments.");
        return nullptr;
    }
    callbackOnCompletionRestart();
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* callbackOnCompletionCoarseZero_py(PyObject* self, PyObject* args) {
    Py_ssize_t arg_count = PyTuple_Size(args);
    if (arg_count != 0) {
        PyErr_SetString(PyExc_ValueError, "This function should be called with no arguments.");
        return nullptr;
    }
    callbackOnCompletionCoarseZero();
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* callbackOnCompletionFineZero_py(PyObject* self, PyObject* args) {
    Py_ssize_t arg_count = PyTuple_Size(args);
    if (arg_count != 0) {
        PyErr_SetString(PyExc_ValueError, "This function should be called with no arguments.");
        return nullptr;
    }
    callbackOnCompletionFineZero();
    Py_INCREF(Py_None);
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
    {"callbackOnErrorWhileFineZero", callbackOnErrorWhileFinzeZero_py,
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
        printLog("problem with dataDict.");
        PyErr_SetString(PyExc_TypeError, "data is not valid.");
        return NULL;
    }

    PyObject* data_frames = PyDict_GetItemString(dataDict, "data_frames");
    if (!data_frames || !PyDict_Check(data_frames)) {
        printLog("Problem with data frames.");
        PyErr_SetString(PyExc_TypeError, "data_frames is not a dictionary.");
        return NULL;
    }

    PyObject* key;
    PyObject* value;
    Py_ssize_t pos = 0;

    while (PyDict_Next(data_frames, &pos, &key, &value)) {
        PyObject* data = PyDict_GetItemString(value, "data");
        //printLog(std::string("pos: ") + std::to_string(pos));
        //printLog(std::string("value: ") + std::to_string((double) PyLong_AsLong(data)));
        double sample = PyLong_AsDouble(data);

        Py_BEGIN_ALLOW_THREADS
        acq_system->addSampleToSamplesColumn(pos-1, sample);
        Py_END_ALLOW_THREADS

        if (pos == 3) 
            break;
    }
    Py_INCREF(Py_None);
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
  m_numSensors(3)
{
    printLog("Initializing Python");
    printLog(libPythonBugFix);

    void*const libBugFix = dlopen(libPythonBugFix, RTLD_LAZY | RTLD_GLOBAL);

    acq_system = this;
    
    Py_Initialize();

    m_pThreadState = (void*)PyEval_SaveThread();

    GILHandler g;

    preConfigurePython();

    runPythonFile("./config2.py","bla");

    m_pCallsModule = loadCModule("fieldline_callbacks", *(void*(*)(void))&PyInit_fieldline_callbacks);

    //PyObject* FieldlineModule = (PyObject*)loadModule("fieldline_api_mock.fieldline_service");
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

    Py_XDECREF(FieldlineModule);
    Py_XDECREF(fService);
    Py_XDECREF(ipList);

    initSampleArrays();
}

FieldlineAcqSystem::~FieldlineAcqSystem()
{
    GILHandler g;
    
    Py_XDECREF(m_fServiceInstance);
    Py_XDECREF(m_pCallsModule);

    if (m_samplesBlock) {
        delete[] m_samplesBlock;
    }
    Py_Finalize();
}

void* FieldlineAcqSystem::loadSensors() {
    PyObject* loadSensors = PyObject_GetAttrString((PyObject*)m_fServiceInstance, "load_sensors");
    PyObject* sensors = PyObject_CallNoArgs(loadSensors);
    Py_XDECREF(loadSensors);
    return (void*)sensors;
}

void FieldlineAcqSystem::setCloseLoop() {

    GILHandler g;

    AcqOpener o((PyObject*)m_fServiceInstance);

    PyObject* setCloseLoopCall = PyObject_GetAttrString((PyObject*)m_fServiceInstance, "set_closed_loop");
    PyObject* trueTuple = PyTuple_New(1);
    PyTuple_SetItem(trueTuple, 0, Py_True);
    PyObject* setCloseLoopCallResult = PyObject_CallObject(setCloseLoopCall, trueTuple);
    if (setCloseLoopCallResult == NULL)
    {
      printLog("setCloseLoopCallResult wrong!");
    } else{
      printLog("setCloseLoopCallResult ok!");
    }

    Py_XDECREF(setCloseLoopCall);
    Py_XDECREF(trueTuple);
    Py_XDECREF(setCloseLoopCallResult);
}

// for activating trigger sensor: service.restart_sensors({0:[0]})
// for deactivating trigger sensors: service.turn_off_sensors({0:[0]})
// typical calibration value: 2.27e-15 T
//

void FieldlineAcqSystem::restartAllSensors() {

    restartFinished = false;

    GILHandler g;

    AcqOpener o((PyObject*)m_fServiceInstance);

    PyObject* sensors = (PyObject*) loadSensors();

    PyObject* restartAllSensorsCall = PyObject_GetAttrString((PyObject*)m_fServiceInstance, "restart_sensors");
    if (restartAllSensorsCall == NULL)
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

    PyObject* callback_on_error = PyObject_GetAttrString((PyObject*)m_pCallsModule, "callbackOnErrorWhileRestart");
    if (callback_on_error == NULL)
    {
        printLog("callback on error broken");
    } else {
        printLog("callback error ok!");
    }

    PyObject* callback_on_completion = PyObject_GetAttrString((PyObject*)m_pCallsModule, "callbackOnCompletionRestart");
    if (callback_on_completion == NULL)
    {
        printLog("callback on completion broken");
    } else {
        printLog("callback completion ok!");
    }

    PyObject* argsRestart = PyTuple_New(4);
    PyTuple_SetItem(argsRestart, 0, sensors);
    PyTuple_SetItem(argsRestart, 1, callback_on_finished);
    PyTuple_SetItem(argsRestart, 2, callback_on_error);
    PyTuple_SetItem(argsRestart, 3, callback_on_completion);

    PyObject* resultRestart = PyObject_CallObject(restartAllSensorsCall, argsRestart);
    if (resultRestart == NULL)
    {
        printLog("restart call broken");
    } else {
        printLog("restart call ok!");
    }

    Py_BEGIN_ALLOW_THREADS
    while (restartFinished == false) {
        std::this_thread::sleep_for(std::chrono::milliseconds(600));
    }
    Py_END_ALLOW_THREADS

    Py_XDECREF(sensors);
    Py_XDECREF(restartAllSensorsCall);
    Py_XDECREF(callback_on_finished);
    Py_XDECREF(callback_on_error);
    Py_XDECREF(callback_on_completion);
    Py_XDECREF(argsRestart);
    Py_XDECREF(resultRestart);
}

void FieldlineAcqSystem::coarseZeroAllSensors() {

    coarseZeroFinished = false;

    GILHandler g;

    AcqOpener o((PyObject*)m_fServiceInstance);

    PyObject* sensors = (PyObject*) loadSensors();

    PyObject* coarseZeroAllSensorsCall = PyObject_GetAttrString((PyObject*)m_fServiceInstance, "coarse_zero_sensors");
    if (coarseZeroAllSensorsCall == NULL)
    {
        printLog("coarse zero sensors broken");
    } else {
        printLog("coarseZeroAllSensors ok!");
    }

    PyObject* callback_on_finished_coarse_zero = PyObject_GetAttrString((PyObject*)m_pCallsModule, "callbackOnFinishedWhileCoarseZero");
    if (callback_on_finished_coarse_zero == NULL)
    {
        printLog("callback on finished coarse zero broken");
    } else {
        printLog("callback on finished coarse zero ok!");
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    PyObject* callback_on_error_coarse_zero = PyObject_GetAttrString((PyObject*)m_pCallsModule, "callbackOnErrorWhileCoarseZero");
    if (callback_on_error_coarse_zero == NULL)
    {
        printLog("callback on error coarse zero broken");
    } else {
        printLog("callback coarse zero error ok!");
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    PyObject* callback_on_completion_coarse_zero = PyObject_GetAttrString((PyObject*)m_pCallsModule, "callbackOnCompletionCoarseZero");
    if (callback_on_completion_coarse_zero == NULL)
    {
        printLog("callback on completion coarse zero broken");
    } else {
        printLog("callback coarse zero completion ok!");
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    PyObject* argsCoarseZero = PyTuple_New(4);
    PyTuple_SetItem(argsCoarseZero, 0, sensors);
    PyTuple_SetItem(argsCoarseZero, 1, callback_on_finished_coarse_zero);
    PyTuple_SetItem(argsCoarseZero, 2, callback_on_error_coarse_zero);
    PyTuple_SetItem(argsCoarseZero, 3, callback_on_completion_coarse_zero);

    PyObject* resultCoarseZero = PyObject_CallObject(coarseZeroAllSensorsCall, argsCoarseZero);
    if (resultCoarseZero == NULL)
    {
        printLog("call coarsezero call broken");
    } else {
        printLog("coarse zero call call ok!");
    }
    
    Py_BEGIN_ALLOW_THREADS
    while (coarseZeroFinished == false) {
        std::this_thread::sleep_for(std::chrono::milliseconds(600));
    }
    Py_END_ALLOW_THREADS

    Py_XDECREF(sensors);
    Py_XDECREF(coarseZeroAllSensorsCall);
    Py_XDECREF(callback_on_finished_coarse_zero);
    Py_XDECREF(callback_on_error_coarse_zero);
    Py_XDECREF(callback_on_completion_coarse_zero);
    Py_XDECREF(argsCoarseZero);
    Py_XDECREF(resultCoarseZero);
}

void FieldlineAcqSystem::fineZeroAllSensors() {

    fineZeroFinished = false;

    GILHandler g;

    AcqOpener o((PyObject*)m_fServiceInstance);

    PyObject* sensors = (PyObject*) loadSensors();

    PyObject* fineZeroAllSensorsCall = PyObject_GetAttrString((PyObject*) m_fServiceInstance, "fine_zero_sensors");

    if (fineZeroAllSensorsCall == NULL)
    {
        printLog("fine zero sensors broken");
    } else {
        printLog("fineZeroAllSensors ok!");
    }

    PyObject* callback_on_finished_fine_zero = PyObject_GetAttrString((PyObject*)m_pCallsModule, "callbackOnFinishedWhileFineZero");
    if (callback_on_finished_fine_zero == NULL)
    {
        printLog("callback on finished fine zero broken");
    } else {
        printLog("callback on finished fine zero ok!");
    }

    PyObject* callback_on_error_fine_zero = PyObject_GetAttrString((PyObject*)m_pCallsModule, "callbackOnErrorWhileFineZero");
    if (callback_on_error_fine_zero == NULL)
    {
        printLog("callback on error fine zero broken");
    } else {
        printLog("callback fine zero error ok!");
    }

    PyObject* callback_on_completion_fine_zero = PyObject_GetAttrString((PyObject*)m_pCallsModule, "callbackOnCompletionFineZero");
    if (callback_on_completion_fine_zero == NULL)
    {
        printLog("callback on completion fine zero broken");
    } else {
        printLog("callback fine zero completion ok!");
    }

    PyObject* argsFineZero = PyTuple_New(4);
    PyTuple_SetItem(argsFineZero, 0, sensors);
    PyTuple_SetItem(argsFineZero, 1, callback_on_finished_fine_zero);
    PyTuple_SetItem(argsFineZero, 2, callback_on_error_fine_zero);
    PyTuple_SetItem(argsFineZero, 3, callback_on_completion_fine_zero);

    PyObject* resultFineZero = PyObject_CallObject(fineZeroAllSensorsCall, argsFineZero);
    if (resultFineZero == NULL)
    {
        printLog("call finezero call broken");
    } else {
        printLog("fine zero call call ok!");
    }

    Py_BEGIN_ALLOW_THREADS
    while (fineZeroFinished == false) {
        std::this_thread::sleep_for(std::chrono::milliseconds(600));
    }
    Py_END_ALLOW_THREADS

    Py_XDECREF(sensors);
    Py_XDECREF(fineZeroAllSensorsCall);
    Py_XDECREF(callback_on_finished_fine_zero);
    Py_XDECREF(callback_on_error_fine_zero);
    Py_XDECREF(callback_on_completion_fine_zero);
    Py_XDECREF(argsFineZero);
    Py_XDECREF(resultFineZero);
}

void FieldlineAcqSystem::setDataCallback() {

    GILHandler g;

    AcqOpener opener((PyObject*)m_fServiceInstance);

    PyObject* readDataCall = PyObject_GetAttrString((PyObject*)m_fServiceInstance, "read_data");
    if (readDataCall == NULL) {
        printLog("problem readDataCall");
    } else {
        printLog("readDataCall ok");
    }

    PyObject* parseCallbacksModule = (PyObject*)loadCModule("callbacks_parsing", *(void*(*)(void))&PyInit_callbacks_parsing);
    if (parseCallbacksModule == NULL)
    {
      printLog("callbacks module wrong!");
    } else{
      printLog("callbacks module ok!");
    }

    PyObject* parserCallback = PyObject_GetAttrString(parseCallbacksModule, "dict_parser");
    if (parserCallback == NULL) {
        printLog("problem parserCallback");
    } else {
        printLog("parserCallback ok");
    }
    PyObject* argsSetDataParser = PyTuple_New(1);
    PyTuple_SetItem(argsSetDataParser, 0, parserCallback);

    PyObject* readDataReturn = PyObject_CallObject(readDataCall, argsSetDataParser);
    if (readDataReturn == NULL)
    {
        printLog("readDataReturn bad");
    } else {
        printLog("readDataReturn ok");
    }
    Py_XDECREF(readDataCall);
    Py_XDECREF(parseCallbacksModule);
    Py_XDECREF(parserCallback);
    Py_XDECREF(argsSetDataParser);
    Py_XDECREF(readDataReturn);
}

void FieldlineAcqSystem::startADC() {

    GILHandler g;

    PyObject* openMethod = PyObject_GetAttrString((PyObject*)m_fServiceInstance, "open");
    if (openMethod == NULL)
    {
        printLog("openMethod wrong!");
    } else{
        printLog("openMethod ok!");
    }

    PyObject* openMethodCall = PyObject_CallNoArgs(openMethod);
    if (openMethodCall == NULL)
    {
      printLog("openMethodCall wrong!");
    } else{
      printLog("openMethodCall ok!");
    }
    Py_XDECREF(openMethod);
    Py_XDECREF(openMethodCall);

    PyObject* readDataCall = PyObject_GetAttrString((PyObject*)m_fServiceInstance, "read_data");
    if (readDataCall == NULL) {
        printLog("problem readDataCall");
    } else {
        printLog("readDataCall ok");
    }

    PyObject* parseCallbacksModule = (PyObject*)loadCModule("callbacks_parsing", *(void*(*)(void))&PyInit_callbacks_parsing);
    if (parseCallbacksModule == NULL)
    {
      printLog("callbacks module wrong!");
    } else{
      printLog("callbacks module ok!");
    }

    PyObject* parserCallback = PyObject_GetAttrString(parseCallbacksModule, "dict_parser");
    if (parserCallback == NULL) {
        printLog("problem parserCallback");
    } else {
        printLog("parserCallback ok");
    }
    PyObject* argsSetDataParser = PyTuple_New(1);
    PyTuple_SetItem(argsSetDataParser, 0, parserCallback);

    PyObject* readDataReturn = PyObject_CallObject(readDataCall, argsSetDataParser);
    if (readDataReturn == NULL)
    {
        printLog("readDataReturn bad");
    } else {
        printLog("readDataReturn ok");
    }

    Py_XDECREF(readDataCall);
    Py_XDECREF(parseCallbacksModule);
    //Py_XDECREF(parserCallback);
    Py_XDECREF(argsSetDataParser);
    Py_XDECREF(readDataReturn);


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

    Py_XDECREF(argsStartData);
    Py_XDECREF(zeroArg);
    Py_XDECREF(startResult);
}

void FieldlineAcqSystem::stopADC() {
    GILHandler g;

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

    PyObject* closeMethod = PyObject_GetAttrString((PyObject*)m_fServiceInstance, "close");
    if (closeMethod == NULL)
    {
        printLog("closeMethod wrong!");
    } else{
        printLog("closeMethod ok!");
    }

    PyObject* closeMethodCall = PyObject_CallNoArgs(closeMethod);
    if (closeMethodCall == NULL)
    {
      printLog("closeMethodCall wrong!");
    } else{
      printLog("closeMethodCall ok!");
    }

    Py_XDECREF(closeMethod);
    Py_XDECREF(closeMethodCall);

    Py_XDECREF(argsstopData);
    Py_XDECREF(stopArg);
    Py_XDECREF(stopResult);
}

void* FieldlineAcqSystem::loadModule(const char* moduleName)
{
    GILHandler g;

    PyObject* pModule = PyImport_ImportModule(moduleName);
    if (pModule == NULL) {
        printLog(std::string("Error loading module ").append(moduleName).append(".").c_str());
        PyErr_Print();
    }

    Py_XDECREF(pModule);

    return (void*)pModule;
}

void* FieldlineAcqSystem::loadCModule(const char* moduleName, void*(*moduleInitFunc)(void))
{
    GILHandler g;

    if (Py_IsInitialized())
    {
        PyImport_AddModule(moduleName);
        PyObject* PyModule = (PyObject*)moduleInitFunc();
        PyObject* sys_modules = PyImport_GetModuleDict();
        PyDict_SetItemString(sys_modules, moduleName, PyModule);
        Py_XDECREF(PyModule);
    } else {
        PyImport_AppendInittab(moduleName, (PyObject*(*)(void)) moduleInitFunc);
    }

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
    GILHandler g;

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
}

void FieldlineAcqSystem::preConfigurePython() const
{
    PyObject* sys = PyImport_ImportModule("sys");
    PyObject* versionInfo = PyObject_GetAttrString(sys, "version_info");
    PyObject* versionInfoMajor = PyObject_GetAttrString(versionInfo, "major");
    PyObject* versionInfoMinor = PyObject_GetAttrString(versionInfo, "minor");
    const std::string pythonVer(std::to_string(PyLong_AsLong(versionInfoMajor)) + \
                                                "." + std::to_string(PyLong_AsLong(versionInfoMinor)));
    Py_XDECREF(versionInfoMajor);
    Py_XDECREF(versionInfoMinor);
    Py_XDECREF(versionInfo);

    PyObject* path = PyObject_GetAttrString(sys, "path");
    PyObject* resourcesObj = PyUnicode_FromString(resourcesPath.c_str());
    PyList_Insert(path, 0, resourcesObj);
    Py_XDECREF(resourcesObj);

    const std::string pathVenvMods(resourcesPath + "venv/lib/python" + pythonVer + "/site-packages/");
    printLog(pathVenvMods);
    PyList_Insert(path, 1, PyUnicode_FromString(pathVenvMods.c_str()));
    Py_XDECREF(path);

    Py_XDECREF(sys);
}

void FieldlineAcqSystem::runPythonFile(const char* file, const char* comment) const
{
    FILE *py_file = fopen(file, "r");
    if (py_file)
    {
        PyObject* global_dict = PyDict_New();
        PyObject* local_dict = PyDict_New();
        PyObject* result = PyRun_File(py_file, comment, Py_file_input, global_dict, local_dict);
        Py_XDECREF(global_dict);
        Py_XDECREF(local_dict);
        Py_XDECREF(result);
        fclose(py_file);
    }
}

void FieldlineAcqSystem::initSampleArrays()
{
    m_samplesBlock = new double[m_numSensors * m_numSamplesPerBlock];
}

void FieldlineAcqSystem::addSampleToSamplesColumn(size_t sensorIdx, double value)
{
    static size_t sampleIdx = 0;
    std::cout << "Value: " << 2.27e-15 * value << "\n";
    std::cout.flush();
    m_samplesBlock[sensorIdx * m_numSamplesPerBlock + sampleIdx] = 2.27e-15 * value;
    if (sensorIdx == m_numSensors -1) {
        sampleIdx++;
        if (sampleIdx == m_numSamplesPerBlock) {
            sampleIdx = 0;
            m_pControllerParent->newData(m_samplesBlock, m_numSensors, m_numSamplesPerBlock);
        }
    }
}

}  // namespace FIELDLINEPLUGIN

