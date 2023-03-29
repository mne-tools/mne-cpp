//=============================================================================================================
/**
 * @file     fieldline_system_controller.cpp
 * @author   Juan GarciaPrieto <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel B Motta <gbmotta@mgh.harvard.edu>;
 * @since    0.1.0
 * @date     February, 2023
 *
 * @section  LICENSE
 *
 * Copyright (C) 2023, Juan G Prieto, Gabriel B Motta. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors
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
 * @brief    Contains the definition of the Fieldline class.
 *
 */
#define PY_SSIZE_T_CLEAN
#include "Python.h"

#include "fieldline/fieldline.h"
#include "fieldline/fieldline_acqsystem.h"

#include <iomanip>
#include <iostream>

//extern "C" {
//
//PyObject *restartFinished(PyObject *self, PyObject *args) {
//  long chassis, sensor;
//  if (PyArg_ParseTuple(args, "ii", &chassis, &sensor)) {
//    std::cout << std::setfill('0') << std::setw(2) << chassis << ":" << sensor
//              << " - restart finished;\n";
//  } else {
//    std::cout << "A sensor has finished restarting!\n";
//  }
//
//  return NULL;
//}
//PyObject *coarseZeroFinished(PyObject *self, PyObject *args) {
//  long chassis, sensor;
//  if (PyArg_ParseTuple(args, "ii", &chassis, &sensor)) {
//    std::cout << std::setfill('0') << std::setw(2) << chassis << ":" << sensor
//              << " - coarse zero finished;\n";
//  } else {
//    std::cout << "A sensor has finished coarse zeroing!\n";
//  }
//
//  return NULL;
//}
//PyObject *fineZeroFinished(PyObject *self, PyObject *args) {
//  long chassis, sensor;
//  if (PyArg_ParseTuple(args, "ii", &chassis, &sensor)) {
//    std::cout << std::setfill('0') << std::setw(2) << chassis << ":" << sensor
//              << " - fine zero finished;\n";
//  } else {
//    std::cout << "A sensor has finished fine zeroing!\n";
//  }
//
//  return NULL;
//}
//}
//
//static PyMethodDef my_module_methods[] = {
//    {"restartFinished", restartFinished, METH_VARARGS, " "},
//    {"coarseZeroFinished", coarseZeroFinished, METH_VARARGS, " "},
//    {"fineZeroFinished", fineZeroFinished, METH_VARARGS, " "},
//    {NULL, NULL, 0, NULL}};
//
//static PyModuleDef my_module_def = {
//    PyModuleDef_HEAD_INIT,
//    "mne_cpp_callbacks",
//    "A module of callback functions for mne-cpp.",
//    -1,
//    my_module_methods,
//    NULL,
//    NULL,
//    NULL,
//    NULL};
//
//PyMODINIT_FUNC PyInit_my_module(void) {
//  return PyModule_Create(&my_module_def);
//}

namespace FIELDLINEPLUGIN {


FieldlineAcqSystemController::FieldlineAcqSystemController(Fieldline* parent) noexcept 
: m_pFieldlinePlugin(parent) {
  // pythonInterpreter initiate
  //
  // run script
  //
  // find ip
  //
  // start connection
  //
  // initialize channels
  //
  //
  Py_Initialize();
  PyConfig config;
  PyConfig_InitPythonConfig(&config);
  config.module_search_paths_set = 1;
  PyWideStringList_Append(&config.module_search_paths, L".");
  Py_InitializeFromConfig(&config);
  FILE *py_file = fopen("main.py", "r");
  PyObject *global_dict = PyDict_New();
  PyObject *local_dict = PyDict_New();
  PyObject *result =
    PyRun_File(py_file, "main.py", Py_file_input, global_dict, local_dict);
  Py_DECREF(global_dict);
  Py_DECREF(local_dict);
  Py_DECREF(result);
  fclose(py_file);
}


FieldlineAcqSystemController::~FieldlineAcqSystemController() noexcept {
  // Py_Finalize();
}

} // namespace FIELDLINEPLUGIN
