#include <cltypes.h>
#include <Python.h>
#include <string>
#include <vector>

using namespace std;

/*
 * POSIX Includes.
 */
#include <assert.h>
#include <stdlib.h>

/*
 * Basic ASP Includes.
 */
#include <clLogApi.hxx>
#include <clGlobals.hxx>
#include <clCommon.hxx>
#include <clMgtDatabase.hxx>

using namespace SAFplus;

static MgtDatabase* db=NULL;

/*static PyObject* Read(PyObject *self, PyObject *args)
{
    ClRcT rc = CL_OK;
    ClCharT *key;
    string value;

    if (!PyArg_ParseTuple(args, "s", &key))
    {
        Py_DECREF(args);
        return NULL;
    }

    Py_DECREF(args);

    rc = db->getRecord(key, value);
    if (rc != CL_OK)
    {
        PyErr_SetObject(PyExc_SystemError, PyInt_FromLong(CL_GET_ERROR_CODE(rc)));
        return NULL;
    }

    PyObject* ret = PyString_FromString(value.c_str());
    return ret;
}*/

static PyObject* Read(PyObject *self, PyObject *args)
{
    ClRcT rc = CL_OK;
    ClCharT *key;
    string value;
    vector<string> child;

    if (!PyArg_ParseTuple(args, "s", &key))
    {
        Py_DECREF(args);
        return NULL;
    }

    Py_DECREF(args);

    rc = db->getRecord(key, value, &child);
    if (rc != CL_OK)
    {
        //PyErr_SetObject(PyExc_SystemError, PyInt_FromLong(CL_GET_ERROR_CODE(rc)));
        return NULL;
    }

    PyObject* pVal = PyString_FromString(value.c_str());

    PyObject* pChild;
    pChild = PyList_New(child.size());
    int i=0;
    for (vector<string>::iterator it = child.begin();it!=child.end();it++)
    {
        PyObject* val = PyString_FromString((*it).c_str());
        PyList_SetItem(pChild,i++,val);
    }

    PyObject* ret;
    ret = PyTuple_New(2);
    PyTuple_SetItem(ret,0,pVal);
    PyTuple_SetItem(ret,1,pChild);
    return ret;
}

ClRcT ParseArgument(PyObject *args, string &strKey, string &strValue, vector<string> &child)
  {
    ClCharT *key;
    ClCharT *value;
    PyObject *argList;

    int i = 0;
    if (!PyArg_ParseTuple(args, "ssO!", &key, &value, &PyList_Type, &argList))
      {
        return 1;
      }

    if (argList)
      {
        PyObject *tseq = PySequence_List(argList);
        if (tseq)
          {
            int t_seqlen = PySequence_Fast_GET_SIZE(tseq);
            for (i = 0; i < t_seqlen; i++)
              {
                ClCharT *childArg = PyString_AsString(PySequence_Fast_GET_ITEM(tseq, i));
                child.push_back(childArg);
              }
            Py_DECREF(tseq);
          }
      }

    strKey.assign(key);
    strValue.assign(value);
    return CL_OK;
  }

static PyObject* Create(PyObject *self, PyObject *args)
{
    ClRcT rc = CL_OK;
    string key,value;
    vector<string> child;

    rc = ParseArgument(args, key, value, child);
    if (rc != CL_OK)
    {
        return NULL;
    }

    rc = db->insertRecord(key, value, &child);
    if (rc != CL_OK)
    {
        PyErr_SetObject(PyExc_SystemError, PyInt_FromLong(CL_GET_ERROR_CODE(rc)));
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject* Write(PyObject *self, PyObject *args)
{
    ClRcT rc = CL_OK;

    string key,value;
    vector<string> child;

    rc = ParseArgument(args, key, value, child);
    if (rc != CL_OK)
    {
        return NULL;
    }

    rc = db->setRecord(key, value, &child);
    if (rc != CL_OK)
    {
        PyErr_SetObject(PyExc_SystemError, PyInt_FromLong(CL_GET_ERROR_CODE(rc)));
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject* Iterators(PyObject *self, PyObject *args)
{
    ClRcT rc = CL_OK;
    ClCharT *xpath;

    if (!PyArg_ParseTuple(args, "s", &xpath))
    {
        PyErr_SetObject(PyExc_SystemError, PyString_FromString("Xpath filter is NULL"));
        return NULL;
    }

    /*
     * Iterators key value
     */
    PyObject* lstKeys = PyList_New(0);
    std::vector<std::string> xpathIterators;
    db->iterate(xpath, xpathIterators);

    if (xpathIterators.size())
    {
      for (std::vector<std::string>::iterator iter = xpathIterators.begin(); iter!=xpathIterators.end(); ++iter)
      {
        PyObject *item = PyString_FromString((*iter).c_str());
        PyList_Append(lstKeys, item);
      }
    }
    return lstKeys;
}

static PyObject* initializeDbal(PyObject *self, PyObject *args)
{
    ClRcT rc = CL_OK;
    ClUint32T maxKeySize = 0;
    ClUint32T maxRecordSize = 0;
    ClDBNameT dbName;
    const char* dbPlugin;

    /*
     * Initialize name index and data
     * dbName.idx
     * dbName.db
     */
    if (!PyArg_ParseTuple(args, "ssii", &dbName, &dbPlugin, &maxKeySize, &maxRecordSize))
    {
        return NULL;
    }
    db = new MgtDatabase();
    assert(db);
    rc = db->initialize(dbName, dbPlugin, maxKeySize, maxRecordSize);

    if (rc != CL_OK)
    {
        PyErr_SetObject(PyExc_SystemError, PyInt_FromLong(CL_GET_ERROR_CODE(rc)));
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject* finalizeDbal(PyObject *self, PyObject *args)
{
    db->finalize();
    delete db;
    Py_RETURN_NONE;
}

static PyMethodDef DbalPyMethods[] = {
    { "initialize", initializeDbal, METH_VARARGS, "Call this function to initialize this library.  Pass the database name, maximum key size and maximum record size." },
    { "finalize", finalizeDbal, METH_VARARGS, "Call this function to finalize this library.  All data is flushed and the database is closed." },
    { "read", Read, METH_VARARGS, "Call this function to read a database record given a key." },
    { "write", Write, METH_VARARGS, "Call this function to write a record with key and value into the database.  If the record does not exist, it will be created." },
    { "create", Create, METH_VARARGS, "Call this function to write a new record with key and value to the database.  If the record exists, it will not be overwritten" },
    { "iterators", Iterators, METH_VARARGS, "Call this function to iterate over all keys in the database." },
    { NULL, NULL, 0, NULL } /* Sentinel */
};

PyMODINIT_FUNC
initpyDbal(void)
{
    ClRcT rc = CL_OK;

    logEchoToFd = 1;  // echo logs to stdout for debugging
    //logSeverity = LOG_SEV_ERROR;

    safplusInitialize(SAFplus::LibDep::DBAL | SAFplus::LibDep::LOG | SAFplus::LibDep::OSAL | SAFplus::LibDep::HEAP | SAFplus::LibDep::TIMER | SAFplus::LibDep::BUFFER);

    (void) Py_InitModule("pyDbal", DbalPyMethods);
}
