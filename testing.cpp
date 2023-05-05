#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <fstream>
#include <iostream>
// #include "defaults.hpp"

void dictParser() {
  std::cout << "Hello from dictParser\n";
}

static PyObject* dict_parser_py_fastcall(PyObject* self, PyObject*const* args, Py_ssize_t argc) {
  PyObject* dict = args[1];
  std::cout << "fastcall started.\n";
  std::cout << "num of args = " << argc << "\n";

  if (!PyLong_Check(args[0])) {
    std::cout << "Not int.\n";
    return NULL;
  } else {
    std::cout << "integer: " << PyLong_AsLong(args[0]) << "\n";
  }
  if (!PyDict_Check(dict)) {
    std::cout << "Not a dict.!\n";
    return NULL;
  } else {
    int typeDict = PyDict_Check(dict);
    if (typeDict)
      std::cout << "It is a boy!\n";

    int s = PyDict_Size(dict);
    std::cout << "Number of elements: " << s << "\n";

    PyObject *key = PyUnicode_FromString("keyA");
    if (PyDict_Contains(dict, key) == 1) {
      PyObject *value = PyDict_GetItem(dict, key);
      long my_value = PyLong_AsLong(value);
      std::cout
        << "The value :" << PyUnicode_AsUTF8(key)
        << " is " << my_value << "\n";
    }
  }
  return Py_None;
}

static PyObject* dict_parser_py_varargs(PyObject* self, PyObject* args) {
  // parse input args
  int numArgs = 0;
  int s = 0;
  int var;
  PyObject* dict;


  if (!PyTuple_Check(args)) {
    return NULL;
  }

  numArgs = PyTuple_Size(args);
  std::cout << "Num args: " << numArgs << "\n";
  // if (PyTuple_Size(args) != 1) 
  //   std::cout << "Num args not correct.\n";
  //
  if (!PyArg_ParseTuple(args, "iO", &var, &dict)) {
    std::cout << "Error parsing arguments.\n";
    return NULL;
  }
  std::cout << "var = " << var << "\n";
  int typeDict = PyDict_Check(dict);
  if (typeDict)
    std::cout << "It is a boy!\n";

  s = PyDict_Size(dict);
  std::cout << "Number of elements: " << s << "\n";

  PyObject *key = PyUnicode_FromString("keyA");
  if (PyDict_Contains(dict, key) == 1) {
    PyObject *value = PyDict_GetItem(dict, key);
    long my_value = PyLong_AsLong(value);
    std::cout
      << "The value :" << PyUnicode_AsUTF8(key)
      << " is " << my_value << "\n";
  }

  return Py_None;
}


void get_item_from_dict(PyObject* item, PyObject* dict, const char* key) {
  PyObject* key_obj = PyUnicode_FromString(key);
  item = PyDict_GetItem(dict, key_obj);
}


static PyObject* dict_parser(PyObject* self, PyObject*const* args, Py_ssize_t argc) {
  if (argc != 1) {
    return NULL;
  }

  if (!PyDict_Check(args[0])) {
    return NULL;
  }

  PyObject* dict = args[0];
  PyObject* data_frames;
  PyObject* dataKeyObj = PyUnicode_FromString("data");
  PyObject* sensorNameKeyObj = PyUnicode_FromString("sensor");

  PyObject* key_obj = PyUnicode_FromString("data_frames");

  data_frames = PyDict_GetItem(dict, key_obj);
  Py_DECREF(dict);
  Py_DECREF();

  int num_channels = PyDict_Size(data_frames);

  // std::cout << "Num channels: " << num_channels << "\n";
  // int* data = new int()
  PyObject* key, *value;
  Py_ssize_t pos = 0;
  struct dataValue {
    int value;
    char label[5+1];
  };
  dataValue* dataValues = new dataValue[num_channels];

  while (PyDict_Next(data_frames, &pos, &key, &value)) {
    // int num_values = PyDict_Size(value);
    // std::cout << "Num values: " << num_values << "\n";
    PyObject* data;
    PyObject* sensorName;
    sensorName = PyDict_GetItem(value, sensorNameKeyObj);
    data = PyDict_GetItem(value, dataKeyObj);
    // std::cout << "Sensor " << PyUnicode_AsUTF8(sensorName)
    //           << ": " << PyLong_AsLong(data)
    //           << " - pos: " << pos << "\n";
    memcpy((void*)dataValues[pos-1].label,(void*)PyUnicode_AsUTF8(sensorName), 5);

    dataValues[pos-1].label[5] = static_cast<char>(0);
    dataValues[pos-1].value = PyLong_AsLong(data);
  }

  // for (int i = 0; i < num_channels; i++) {
  //   std::cout << "(" << dataValues[i].label << ") - " << dataValues[i].value << "\n";
  // }

  delete[] dataValues;


  // important !!!!!
  // we need to Py_DECREF a few PyObjects!!!!

  return Py_None;
}

static struct PyMethodDef callbacksMethods[] = {
  {"dict_parser_fastcall", _PyCFunction_CAST(dict_parser_py_fastcall),
    METH_FASTCALL, "Parse dictionary fastcall"},
  {"dict_parser", _PyCFunction_CAST(dict_parser),
    METH_FASTCALL, "Parse dictionary"},
  {"dict_parser_varargs", dict_parser_py_varargs,
    METH_VARARGS, "Parse dictionary with varargs"},
  {NULL, NULL, 0, NULL}
};

static PyModuleDef CallModule = {
  PyModuleDef_HEAD_INIT, "parsing", NULL, -1, callbacksMethods,
  NULL, NULL, NULL, NULL
};


static PyObject* PyInit_parsing(void) {
  return PyModule_Create(&CallModule);
}



int main(int argc, char* argv[]) {
  // std::cout << "Starting execution\n";

  PyImport_AppendInittab("parsing", &PyInit_parsing);
  Py_Initialize();
  PyConfig config;
  PyConfig_InitPythonConfig(&config);
  config.module_search_paths_set = 1;
  PyWideStringList_Append(&config.module_search_paths, L".");
  Py_InitializeFromConfig(&config);

  FILE* py_file;
  if (argc != 2) {
    std::cout << "Usage: " << argv[0] << ": main.py\n";
    exit(1);
  }
  py_file = fopen(argv[1], "r");
  PyObject* global_dict = PyDict_New();
  PyObject* local_dict = PyDict_New();
  PyObject* result = PyRun_File(py_file,
                                argv[1],
                                Py_file_input,
                                global_dict,
                                local_dict);
  Py_DECREF(global_dict);
  Py_DECREF(local_dict);
  Py_DECREF(result);
  fclose(py_file);

  Py_Finalize();

  return 0;
}
