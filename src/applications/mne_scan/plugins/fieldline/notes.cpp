
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
