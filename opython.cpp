#define PY_SSIZE_T_CLEAN
#define HAVE_ROUND
#ifdef _DEBUG
#define RESTORE_DEBUG
#undef _DEBUG
#endif
#include <Python.h>
#ifdef RESTORE_DEBUG
#define _DEBUG
#undef RESTORE_DEBUG
#endif

#include <iostream>
#include <conio.h>
#include "OpenHoldemFunctions.h"
#include <string>																							// 011

PyObject *pName = nullptr, *pModule = nullptr, *pDict = nullptr, *pClass = nullptr, *pInstance = nullptr;

static PyObject* getSymbol(PyObject *self, PyObject *args) {

	char *symbol;
	if (!PyArg_ParseTuple(args, "s", &symbol)) {
		WriteLog("Python Addon - could not parse getSymbol arg");
		return NULL;
	}

	double res = GetSymbol(symbol);
	//WriteLog("Python Addon - getSymbol %s: %f\n", symbol, res);
	return Py_BuildValue("d", res);
}

//011 code
static PyObject* getPlayerName(PyObject *self, PyObject *args) {
// This function should be recoded to accept an integer instead of a string
	char *thechairstr;
	if (!PyArg_ParseTuple(args, "s", &thechairstr)) {
		WriteLog("Python Addon - could not parse getPLayerName arg");
		return NULL;
	}
	try
	{
		int thechair = std::stoi(thechairstr);
		std::cout << thechair << '\n';
		char *res = GetPlayerName(thechair);
		//WriteLog("Python Addon - getSymbol %s: %f\n", symbol, res);
		return Py_BuildValue("s", res);																		// 011 return a string
	}
	catch (const std::invalid_argument& ia) 
	{
		std::cerr << "Bad input. Invalid argument: " << ia.what() << '\n';
	}
	return NULL;
}

static PyObject* getScrape(PyObject *self, PyObject *args) {
	char *region;
	if (!PyArg_ParseTuple(args, "s", &region)) {
		WriteLog("Python Addon - could not parse getScrape arg");
		return NULL;
	}

	int thelength = 0;
	char *res = ScrapeTableMapRegion(region, thelength);
	return Py_BuildValue("s", res);																			// 011 return a string
}
//011 code

PyObject* redirectStdout(PyObject* self, PyObject* pArgs) {

	char* LogStr = NULL;
	if (!PyArg_ParseTuple(pArgs, "s", &LogStr))
		return NULL;

	_cprintf(LogStr);
	WriteLog(LogStr);

	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* redirectStderr(PyObject* self, PyObject* pArgs) {

	char* LogStr = NULL;
	if (!PyArg_ParseTuple(pArgs, "s", &LogStr))
		return NULL;

	_cprintf(LogStr);
	WriteLog(LogStr);

	Py_INCREF(Py_None);
	return Py_None;
}

struct module_state {
	PyObject *error;
};

#define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))

static PyObject* error_out(PyObject *m) {
	struct module_state *st = GETSTATE(m);
	PyErr_SetString(st->error, "something bad happened");
	return NULL;
}

static PyMethodDef openholdem_methods[] = {
	{ "getSymbol", (PyCFunction)getSymbol, METH_VARARGS, "Get OpenHoldem symbol" },
	{ "getScrape", (PyCFunction)getScrape, METH_VARARGS, "Get OpenHoldem scrape" },							// 011
	{ "getPlayerName", (PyCFunction)getPlayerName, METH_VARARGS, "Get OpenHoldem PlayerName" },				// 011
	{ "_stdout", redirectStdout, METH_VARARGS, "Captures stdout" },
	{ "_stderr", redirectStderr, METH_VARARGS, "Captures stderr" },
	{ NULL, NULL, 0, NULL }
};

static int openholdem_traverse(PyObject *m, visitproc visit, void *arg) {
	Py_VISIT(GETSTATE(m)->error);
	return 0;
}

static int openholdem_clear(PyObject *m) {
	Py_CLEAR(GETSTATE(m)->error);
	return 0;
}

static struct PyModuleDef moduledef = {
	PyModuleDef_HEAD_INIT,
	"OpenHoldem",
	NULL,
	sizeof(struct module_state),
	openholdem_methods,
	NULL,
	openholdem_traverse,
	openholdem_clear,
	NULL
};

#define INITERROR return NULL

PyMODINIT_FUNC PyInit_openholdem(void) {

	PyObject *module = PyModule_Create(&moduledef);
	if (module == NULL)
		INITERROR;

	struct module_state *st = GETSTATE(module);
	st->error = PyErr_NewException("OpenHoldem.Error", NULL, NULL);
	if (st->error == NULL) {
		Py_DECREF(module);
		INITERROR;
	}

	return module;
}

int pyInit() {

	PyImport_AppendInittab("OpenHoldem", &PyInit_openholdem);
	Py_Initialize();

	PyRun_SimpleString(
		"import OpenHoldem\n"
		"import sys\n"
		"class StdoutCatcher:\n"
		"\tdef write(self, str):\n"
		"\t\tif (len(str.strip())):\n"
		"\t\t\tOpenHoldem._stdout('>> %s'%str)\n"
		"class StderrCatcher:\n"
		"\tdef write(self, str):\n"
		"\t\tif (len(str.strip())):\n"
		"\t\t\tOpenHoldem._stderr(str)\n"
		"sys.stdout = StdoutCatcher()\n"
		"sys.stderr = StderrCatcher()\n"
	);

	pName = PyUnicode_FromString("PyBot");																	// 011
	pModule = PyImport_Import(pName);
	Py_XDECREF(pName);

	if (pModule) {

		pDict = PyModule_GetDict(pModule);
		if (pDict == nullptr) {
			PyErr_Print();
			WriteLog("Python Addon - Failed to get the dictionary.\n");
			return 1;
		}

		pClass = PyDict_GetItemString(pDict, "Main");
		if (pClass == nullptr) {
			PyErr_Print();
			WriteLog("Python Addon - Failed to get the Python class.\n");
			return 1;
		}

		if (PyCallable_Check(pClass)) {
			pInstance = PyObject_CallObject(pClass, NULL);
		}
		else {
			PyErr_Print();
			WriteLog(
				"Python Addon - Main class not found in script file.\n"
				"The script will now stop"
			);
			return 1;
		}
	}
	else {
		PyErr_Print();
		WriteLog("Python Addon - ERROR: module bot.py not imported\n");
		return 1;
	}
	
	return 0;
}

int pyDestroy() {

	if(pInstance)
		Py_XDECREF(pInstance);

	if(pModule)
		Py_XDECREF(pModule);

	Py_Finalize();

	return 0;
}

double pyDecision() {

	PyObject *pValue;
	double ret = 0;

	pValue = PyObject_CallMethod(pInstance, "getDecision", nullptr);
	if (pValue == nullptr) {
		PyErr_Print();
	}
	else {
		ret = PyFloat_AsDouble(pValue);
		Py_XDECREF(pValue);
	}

	return ret;
}
