// gcc -c custom.c -o custom.o -I/usr/include/python2.5
// gcc -shared custom.o -o pyasp.so
/*
/root/tam/bin/asp_run python
import amfpy; amfpy.initializeAmf()
import asppycor
asppycor.GetObjectTree()
  
 */

#include <Python.h>

#define CL_AMS_MGMT_FUNC

#include <clCorSimpleApi.h>

#include "clIocApi.h"
#include "clOsalApi.h"
//#include "clTipcUserApi.h"
#include "clIocApiExt.h"
#include "clCpmExtApi.h"
#include "clLogApi.h"
#include "clDebugApi.h"
#include "clAmsMgmtClientApi.h"
#include "clAmsMgmtCommon.h"


//static PyObject* emptyDict = NULL;
/* static FILE* logfile=NULL; */

/*#define DbgLog(...)   clAppLog(CL_LOG_HANDLE_APP, CL_LOG_SEV_INFO, 10, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED, __VA_ARGS__) */
/*#define DbgLog(...) do { if(logfile) { fprintf(logfile,__VA_ARGS__); fprintf(logfile,"\n"); fflush(logfile); } } while(0)*/
#define DbgLog(...)

static PyObject* ClassNodeSibs2PyList(ClCorClassTreeNodeT* node)
{
    PyObject* lst = PyList_New(0);
    
    
    while(node)
    {
        PyObject* nodeLst = PyList_New(3);
        PyObject* nodeName = PyString_FromStringAndSize(node->name.value,node->name.length);
        
        //int PyList_SetItem(PyObject *list, Py_ssize_t index, PyObject *item)¶
        PyList_SetItem(nodeLst, 0, nodeName);
        PyObject* nodeType = PyString_FromString("class");
        PyList_SetItem(nodeLst, 1, nodeType);
        //printf("%s%s\n",ispaces,node->name.value);
        PyObject* children = ClassNodeSibs2PyList((ClCorClassTreeNodeT*)node->cmn.child);
        PyList_SetItem(nodeLst, 2, children);

        PyList_Append(lst, nodeLst);
        
        node=(ClCorClassTreeNodeT*) node->cmn.sib;
    }

    return lst;
}

static PyObject* CorData2Py(ClPtrT* value, ClWordT size, ClCorTypeT type, ClCorAttrTypeT ordinality)
{
    PyObject* ret = NULL;
    
    switch(ordinality)
    {
        case CL_COR_SIMPLE_ATTR:
            switch (type)
            {
                case CL_COR_INT8:
                {
                    assert(size == sizeof(ClInt8T));
                    ret = PyInt_FromLong(*((ClInt8T*)value));
                } break;
                
                case CL_COR_UINT8:
                {
                    assert(size == sizeof(ClUint8T));
                    ret = PyInt_FromLong(*((ClUint8T*)value));
                    
                } break;
                
                case CL_COR_INT16:
                {
                    assert(size == sizeof(ClInt16T));
                    ret = PyInt_FromLong(*((ClInt16T*)value));
                } break;                
                case CL_COR_UINT16:
                {
                    assert(size == sizeof(ClUint16T));
                    ret = PyInt_FromLong(*((ClUint16T*)value));
                    
                } break;

                
                case CL_COR_INT32:
                {
                    assert(size == sizeof(ClInt32T));
                    ret = PyInt_FromLong(*((ClInt32T*)value));
                } break;                
                case CL_COR_UINT32:
                case CL_COR_COUNTER32:
                case CL_COR_SEQUENCE32:
                {
                    assert(size == sizeof(ClUint32T));
                    ret = PyInt_FromLong(*((ClUint32T*)value));
                    
                } break;


                case CL_COR_INT64:
                {
                    assert(size == sizeof(ClInt64T));
                    ret = PyLong_FromLongLong(*((ClInt64T*)value));
                } break;                
                case CL_COR_UINT64:
                case CL_COR_COUNTER64:
                {
                    assert(size == sizeof(ClUint64T));
                    ret = PyLong_FromLongLong(*((ClUint64T*)value));                    
                } break;

                case CL_COR_FLOAT:
                case CL_COR_DOUBLE:
                default:
                    assert(0);
                    break;                 
            }
            
            
            break;
        case CL_COR_ARRAY_ATTR:
            switch (type)
            {
                case CL_COR_INT8:
                {
                    int len = strnlen((char*)value,size);                    
                    ret = PyString_FromStringAndSize((char*)value,len);
                } break;
                case CL_COR_UINT8:
                {
                    int len = strnlen((char*)value,size);                    
                    ret = PyString_FromStringAndSize((char*)value,len);
                } break;
                
                default:
                    clDbgCodeError(1,("Element type not handled"));
                    break;                 
                
            
                    break;
            }
            break;
        default:
            clDbgCodeError(1,("Element ordinality not handled"));
            break;
            
    
    }
    return ret;
    
}

    


static PyObject* ObjectNodeSibs2PyList(ClCorObjectTreeNodeT* node)
{
    PyObject* lst = PyList_New(0);
    
    
    while(node)
    {
        PyObject* nodeLst = PyList_New(3);
        PyObject* nodeName = PyString_FromStringAndSize(node->name.value,node->name.length);
        
        //int PyList_SetItem(PyObject *list, Py_ssize_t index, PyObject *item)¶
        PyList_SetItem(nodeLst, 0, nodeName);
        PyObject* nodeType;
        PyObject* children;
        
        
        if (node->size)
        {
            nodeType = PyString_FromString("attribute");
            children = CorData2Py(node->value,node->size,node->type,node->ordinality);            
        }
        else
        {
        nodeType = PyString_FromString("object");
        children = ObjectNodeSibs2PyList((ClCorObjectTreeNodeT*) node->cmn.child);
        }
        
        PyList_SetItem(nodeLst, 1, nodeType);
        //printf("%s%s\n",ispaces,node->name.value);
        PyList_SetItem(nodeLst, 2, children);

        PyList_Append(lst, nodeLst);
        
        node=(ClCorObjectTreeNodeT*) node->cmn.sib;
    }

    return lst;
}


static PyObject* GetClassTree(PyObject *self)
{
    ClCorClassTreeT ct;
    clCorClassTreeInit(&ct);    
    clCorClassTreeFill(&ct);
    clCorClassTreePrint(&ct);
    PyObject* ret = ClassNodeSibs2PyList((ClCorClassTreeNodeT*)ct.root->cmn.child);
    return ret;
}



static PyObject* GetObjectTree(PyObject *self)
{
    ClCorObjectTreeT ot;
    clCorObjectTreeInit(&ot);    
    clCorObjectTreeFill(&ot);
    clCorObjectTreePrint(&ot);
    PyObject* ret = ObjectNodeSibs2PyList((ClCorObjectTreeNodeT*)ot.root->cmn.child);
    return ret;
}


static PyMethodDef PyAspMethods[] = {
    /*    {"GetAmsEntityList", GetAmsEntityList, METH_VARARGS, "Pass an ams entity list from SWIG, and returns a Python list"}, */
    {"GetClassTree",  GetClassTree, METH_VARARGS, "Get the COR class tree"},
    {"GetObjectTree",  GetObjectTree, METH_VARARGS, "Get COR object tree"},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};


PyMODINIT_FUNC
initasppycor(void)
{
    (void) Py_InitModule("asppycor", PyAspMethods);
}

