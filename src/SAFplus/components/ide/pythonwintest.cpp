#include <Python.h>
#include <wx/wx.h>
#include <wx/splitter.h>
// Import Python and wxPython headers
#include <wx/wxPython/wxPython.h>


char* python_code2 = "\
import sys\n\
sys.path.append('.')\n\
import embedded_sample\n\
\n\
def makeWindow(parent):\n\
    win = embedded_sample.MyPanel(parent)\n\
    return win\n\
";

wxWindow* PythonWinTest(wxWindow* parent)
  {
    wxWindow* window = NULL;
    PyObject* result;
    

    // As always, first grab the GIL
    wxPyBlock_t blocked = wxPyBeginBlockThreads();

    // Now make a dictionary to serve as the global namespace when the code is
    // executed.  Put a reference to the builtins module in it.  (Yes, the
    // names are supposed to be different, I don't know why...)
    PyObject* globals = PyDict_New();
    PyObject* builtins = PyImport_ImportModule("__builtin__");
    PyDict_SetItemString(globals, "__builtins__", builtins);
    Py_DECREF(builtins);

        // Execute the code to make the makeWindow function
    result = PyRun_String(python_code2, Py_file_input, globals, globals);
    // Was there an exception?
    if (! result) {
        PyErr_Print();
        wxPyEndBlockThreads(blocked);
        return NULL;
    }
    Py_DECREF(result);

       // Now there should be an object named 'makeWindow' in the dictionary that
    // we can grab a pointer to:
    PyObject* func = PyDict_GetItemString(globals, "makeWindow");
    wxASSERT(PyCallable_Check(func));

    // Now build an argument tuple and call the Python function.  Notice the
    // use of another wxPython API to take a wxWindows object and build a
    // wxPython object that wraps it.
    PyObject* arg = wxPyMake_wxObject(parent, false);
    wxASSERT(arg != NULL);
    PyObject* tuple = PyTuple_New(1);
    PyTuple_SET_ITEM(tuple, 0, arg);
    result = PyEval_CallObject(func, tuple);

    // Was there an exception?
    if (! result)
        PyErr_Print();
    else {
        // Otherwise, get the returned window out of Python-land and
        // into C++-ville...
        bool success = wxPyConvertSwigPtr(result, (void**)&window, _T("wxWindow"));
        wxASSERT_MSG(success, _T("Returned object was not a wxWindow!"));
        Py_DECREF(result);
    }

    // Release the python objects we still have
    Py_DECREF(globals);
    Py_DECREF(tuple);

    // Finally, after all Python stuff is done, release the GIL
    wxPyEndBlockThreads(blocked);

    return window;
  }
