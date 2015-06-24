#include <cltypes.h>
#include <Python.h>
#include <string>

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
#include <clDbalApi.h>
#include <clDbalErrors.h>

using namespace SAFplus;

static ClDBHandleT dbHdl = 0x0;
static ClDBHandleT dbIterHdl = 0x0;

#define PYDBAL_DB_KEY_BITS (32ULL)
#define PYDBAL_DB_KEY_SIZE (1ULL << PYDBAL_DB_KEY_BITS)
#define PYDBAL_DB_KEY_MASK (PYDBAL_DB_KEY_SIZE - 1ULL)

static __inline__ ClUint32T getHashKeyFn(const ClCharT *keyStr)
{
    ClUint32T cksum = SAFplus::computeCrc32((ClUint8T*)keyStr, (ClUint32T)strlen(keyStr));
    return cksum & PYDBAL_DB_KEY_MASK;
}

static PyObject* Read(PyObject *self, PyObject *args)
{
    ClRcT rc = CL_OK;
    ClCharT *key;
    ClCharT *value;
    ClUint32T dataSize        = 0;

    if (!PyArg_ParseTuple(args, "s", &key))
    {
        return NULL;
    }

    ClUint32T hashKey = getHashKeyFn(key);

    rc = clDbalRecordGet(dbHdl, (ClDBKeyT)&hashKey, sizeof(hashKey), (ClDBRecordT*)&value, &dataSize);

    if (rc != CL_OK)
    {
        PyErr_SetObject(PyExc_SystemError, PyInt_FromLong(CL_GET_ERROR_CODE(rc)));
        return NULL;
    }

    PyObject* ret = PyString_FromStringAndSize(value, dataSize);
    return ret;
}

static PyObject* Write(PyObject *self, PyObject *args)
{
    ClRcT rc = CL_OK;
    ClCharT *key;
    ClCharT *value;
    if (!PyArg_ParseTuple(args, "ss", &key, &value))
    {
        return NULL;
    }

    ClUint32T hashKey = getHashKeyFn(key);

    /*
     * Insert into idx table
     */
    rc = clDbalRecordInsert(dbIterHdl, (ClDBKeyT) & hashKey, sizeof(hashKey), (ClDBRecordT) key, strlen(key));

    /*
     * Insert into data table
     */
    rc = clDbalRecordInsert(dbHdl, (ClDBKeyT) & hashKey, sizeof(hashKey), (ClDBRecordT) value, strlen(value));

    if (rc != CL_OK)
    {
        PyErr_SetObject(PyExc_SystemError, PyInt_FromLong(CL_GET_ERROR_CODE(rc)));
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject* Replace(PyObject *self, PyObject *args)
{
    ClRcT rc = CL_OK;
    ClCharT *key;
    ClCharT *value;
    if (!PyArg_ParseTuple(args, "ss", &key, &value))
    {
        return NULL;
    }

    ClUint32T hashKey = getHashKeyFn(key);

    /*
     * Replace a record with key
     */
    rc = clDbalRecordReplace(dbHdl, (ClDBKeyT)&hashKey, sizeof(hashKey), (ClDBRecordT) value, strlen(value));

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

    ClUint32T   keySize         = 0;
    ClUint32T   dataSize        = 0;
    ClUint32T   nextKeySize     = 0;
    ClUint32T   *recKey           = NULL;
    ClUint32T   *nextKey        = NULL;
    ClCharT     *recData          = NULL;

    /*
     * Iterators key value
     */
    PyObject* lstKeys = PyList_New(0);

    rc = clDbalFirstRecordGet(dbIterHdl, (ClDBKeyT*)&recKey, &keySize, (ClDBRecordT*)&recData, &dataSize);

    if (rc != CL_OK)
    {
        PyErr_SetObject(PyExc_SystemError, PyInt_FromLong(CL_GET_ERROR_CODE(rc)));
        return NULL;
    }

    PyObject* firstData = PyString_FromStringAndSize(recData, dataSize);
    PyList_Append(lstKeys, firstData);

    while (1)
    {
        if ((rc = clDbalNextRecordGet(dbIterHdl, (ClDBKeyT)recKey, keySize,
                        (ClDBKeyT*)&nextKey, &nextKeySize,
                        (ClDBRecordT*)&recData, &dataSize)) != CL_OK)
        {
            rc = CL_OK;
            break;
        }
        recKey = nextKey;
        keySize = nextKeySize;

        PyObject* nextData = PyString_FromStringAndSize(recData, dataSize);
        PyList_Append(lstKeys, nextData);
    }

    return lstKeys;
}

static PyObject* initializeDbal(PyObject *self, PyObject *args)
{
    ClRcT rc = CL_OK;
    ClUint32T maxKeySize = 0;
    ClUint32T maxRecordSize = 0;
    ClDBNameT dbName;

    string dbNameIdx = "";
    string dbNameData = "";

    /*
     * Initialize name index and data
     * dbName.idx
     * dbName.db
     */
    if (!PyArg_ParseTuple(args, "sii", &dbName, &maxKeySize, &maxRecordSize))
    {
        return NULL;
    }

    /*
     * Index DB
     */
    dbNameIdx.append(dbName).append(".idx");

    rc = clDbalOpen(dbNameIdx.c_str(), dbNameIdx.c_str(), CL_DB_APPEND, maxKeySize, maxRecordSize, &dbIterHdl);

    if (rc != CL_OK)
    {
        PyErr_SetObject(PyExc_SystemError, PyInt_FromLong(CL_GET_ERROR_CODE(rc)));
        return NULL;
    }

    /*
     * Data
     */
    dbNameData.append(dbName).append(".db");
    rc = clDbalOpen(dbNameData.c_str(), dbNameData.c_str(), CL_DB_APPEND, maxKeySize, maxRecordSize, &dbHdl);

    if (rc != CL_OK)
    {
        PyErr_SetObject(PyExc_SystemError, PyInt_FromLong(CL_GET_ERROR_CODE(rc)));
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject* finalizeDbal(PyObject *self, PyObject *args)
{
    clDbalLibFinalize();
    Py_RETURN_NONE;
}

static PyMethodDef DbalPyMethods[] = {
    { "initializeDbal", initializeDbal, METH_VARARGS, "Call this function to initialize DBAL library." },
    { "finalizeDbal", finalizeDbal, METH_VARARGS, "Call this function to finalize DBAL library." },
    { "read", Read, METH_VARARGS, "Call this function to read a record DB with key." },
    { "write", Write, METH_VARARGS, "Call this function to write a record with key and value to DB." },
    { "replace", Replace, METH_VARARGS, "Call this function to replace a record with key and value to DB." },
    { "iterators", Iterators, METH_VARARGS, "Call this function to iterator all key on DB." },
    { NULL, NULL, 0, NULL } /* Sentinel */
};

PyMODINIT_FUNC
initpyDbal(void)
{
    ClRcT rc = CL_OK;

    logEchoToFd = 1;  // echo logs to stdout for debugging
    logSeverity = LOG_SEV_MAX;

    safplusInitialize(SAFplus::LibDep::DBAL | SAFplus::LibDep::LOG | SAFplus::LibDep::OSAL | SAFplus::LibDep::HEAP | SAFplus::LibDep::TIMER | SAFplus::LibDep::BUFFER);

    (void) Py_InitModule("pyDbal", DbalPyMethods);
}
