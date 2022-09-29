/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#include "hailopython_infra.hpp"

#include <pygobject-3.0/pygobject.h>

#define __PYFILTER_WRAPPER(_OBJECT) PyObjectWrapper(_OBJECT, #_OBJECT)
#define __PYFILTER_DECL_WRAPPER(_NAME, _OBJECT) PyObjectWrapper _NAME(_OBJECT, #_OBJECT)

PythonCallback *create_python_callback(const char *module_path, const char *function_name,
                                       const char *args_string, const char *keyword_args_string, char **error_msg)
{

    if (!module_path || !function_name)
    {
        *error_msg = strdup("module_path, function_name must not be NULL");
        return nullptr;
    }
    try
    {
        auto context_initializer = PythonContextInitializer();
        context_initializer.initialize();
        // add user-specified callback module into Python path
        const char *filename = strrchr(module_path, '/');
        if (filename)
        {
            std::string dir(module_path, filename);
            context_initializer.extendPath(dir);
        }

        return new PythonCallback(module_path, function_name, args_string,
                                  keyword_args_string);
    }
    catch (const std::exception &e)
    {
        PythonError python_err;
        std::string msg = std::string(e.what()) + std::string(": \n") + std::string(python_err.get());
        *error_msg = strdup(msg.c_str());
        return nullptr;
    }
}

GstFlowReturn invoke_python_callback(PythonCallback *python_callback, char **error_msg)
{
    if (!python_callback)
    {
        return GST_FLOW_ERROR;
    }

    auto context_initializer = PythonContextInitializer();
    try
    {
        return python_callback->CallPython();
    }
    catch (const std::exception &e)
    {
        PythonError python_err;
        std::string msg = std::string(e.what()) + std::string(": \n") + std::string(python_err.get());
        *error_msg = strdup(msg.c_str());
        return GST_FLOW_ERROR;
    }
}

GstFlowReturn invoke_python_callback(PythonCallback *python_callback, GstBuffer *buffer,
                                     py_descriptor_t desc, char **error_msg)
{
    if (!python_callback)
    {
        GST_ERROR("python_callback is not initialized");
        return GST_FLOW_ERROR;
    }

    auto context_initializer = PythonContextInitializer();
    try
    {
        return python_callback->CallPython(buffer, desc);
    }
    catch (const std::exception &e)
    {
        PythonError python_err;
        std::string msg = std::string(e.what()) + std::string(": \n") + std::string(python_err.get());
        *error_msg = strdup(msg.c_str());

        return GST_FLOW_ERROR;
    }
}

GstFlowReturn set_python_callback_caps(PythonCallback *python_callback, GstCaps *caps, char **error_msg)
{
    if (nullptr == python_callback)
    {
        GST_ERROR("python_callback is not initialized");
        return GST_FLOW_ERROR;
    }

    auto context_initializer = PythonContextInitializer();
    try
    {
        python_callback->SetCaps(caps);

        return GST_FLOW_OK;
    }
    catch (const std::exception &e)
    {
        PythonError python_err;
        std::string msg = std::string(e.what()) + std::string(": \n") + std::string(python_err.get());
        *error_msg = strdup(msg.c_str());

        return GST_FLOW_ERROR;
    }
}

GstFlowReturn PythonCallback::CallPython(GstBuffer *buffer, py_descriptor_t desc)
{
    // Convert py_descriptor_t to python Class of HailoROI. via python function.
    // The 'k' stands for the parameter type for this function (unsigned long).
    // https://docs.python.org/3/c-api/arg.html#c.Py_BuildValue
    __PYFILTER_DECL_WRAPPER(roi_as_unsigned_long, Py_BuildValue("(k)", desc));
    __PYFILTER_DECL_WRAPPER(hailo_roi, PyObject_CallObject(get_python_roi_function, roi_as_unsigned_long));
    if (!(PyObject *)hailo_roi)
    {
        throw std::runtime_error("Could not convert HailoROI to python");
    }
    // Create a Gst.Buffer object.
    __PYFILTER_DECL_WRAPPER(py_buffer, pyg_boxed_new(buffer->mini_object.type, buffer,
                                                     FALSE /*copy_boxed*/, FALSE /*own_ref*/));
    __PYFILTER_DECL_WRAPPER(py_caps, pyg_boxed_new(caps_ptr->mini_object.type, caps_ptr,
                                                   FALSE /*copy_boxed*/, FALSE /*own_ref*/));
    __PYFILTER_DECL_WRAPPER(frame, PyObject_CallFunctionObjArgs(python_frame_class, (PyObject *)py_buffer, (PyObject *)py_caps,
                                                                (PyObject *)hailo_roi, nullptr));

    // Create the arguments for the user function and call it.
    __PYFILTER_DECL_WRAPPER(args, Py_BuildValue("(O)", (PyObject *)frame));
    PyObjectWrapper result(PyObject_CallObject(user_python_function, args));

    if (((PyObject *)result) == nullptr)
    {
        throw std::runtime_error("Error in Python function");
    }

    return (GstFlowReturn)PyLong_AsLong(result);
}

GstFlowReturn PythonCallback::CallPython()
{
    PyObjectWrapper result(PyObject_CallObject(user_python_function, NULL));

    if (((PyObject *)result) == nullptr)
    {
        throw std::runtime_error("Error in Python function");
    }

    return (GstFlowReturn)PyLong_AsLong(result);
}

PythonCallback::PythonCallback(const char *module_path, const char *function_name,
                               const char *args_string, const char *kwargs_string)
{
    if (module_path == nullptr)
    {
        throw std::invalid_argument("module_path cannot be empty");
    }

    const char *filename = strrchr(module_path, '/');
    if (filename)
    {
        ++filename; // shifting next to '/'
    }
    else
    {
        filename = module_path;
    }

    const char *extension = strrchr(module_path, '.');
    if (!extension)
    {
        module_name = std::string(filename);
    }
    else
    {
        module_name = std::string(filename, extension);
    }

    PyObjectWrapper pluginModule(
        PyImport_Import(__PYFILTER_WRAPPER(PyUnicode_FromString(module_name.c_str()))));
    if (!(PyObject *)pluginModule)
    {
        // Can't be static because should be destroyed while Python context is initialized
        PythonError err;
        GST_ERROR("Python Error %s", err.get());
        throw std::runtime_error("Error loading Python module " + std::string(module_path));
    }

    user_python_function.reset(PyObject_GetAttrString(pluginModule, function_name), "user_python_function");

    if (!(PyObject *)user_python_function)
    {
        throw std::runtime_error("Error getting function '" + std::string(function_name) +
                                 "' from Python module " + std::string(module_path));
    }
    __PYFILTER_DECL_WRAPPER(hailo_module, PyImport_Import(__PYFILTER_WRAPPER(PyUnicode_FromString("hailo"))));
    if (!(PyObject *)hailo_module)
    {
        throw std::runtime_error("Error loading hailo module");
    }
    get_python_roi_function.reset(PyObject_GetAttrString(hailo_module, "access_HailoROI_from_desc"), "get_python_roi");

    __PYFILTER_DECL_WRAPPER(gsthailo_module, PyImport_Import(__PYFILTER_WRAPPER(PyUnicode_FromString("gsthailo"))));

    if (!(PyObject *)gsthailo_module)
    {
        throw std::runtime_error("Error getting gsthailo.VideoFrame");
    }

    python_frame_class.reset(PyObject_GetAttrString(gsthailo_module, "VideoFrame"), "videoframe_class");
}

PythonContextInitializer::PythonContextInitializer()
{
    state = PyGILState_UNLOCKED;
    if (Py_IsInitialized())
    {
        state = PyGILState_Ensure();
    }
    else
    {
        Py_Initialize();
    }

    sys_path = PySys_GetObject("path");
}

PythonContextInitializer::~PythonContextInitializer()
{
    if (Py_IsInitialized())
    {
        PyGILState_Release(state);
    }
    else
    {
        PyEval_SaveThread();
    }
}

void PythonContextInitializer::initialize()
{
    /* load libpython.so and initilize pygobject */
    Dl_info libpython_info = Dl_info();
    dladdr((void *)Py_IsInitialized, &libpython_info);
    GModule *libpython = g_module_open(libpython_info.dli_fname, G_MODULE_BIND_LAZY);
    if (!pygobject_init(3, 0, 0))
    {
        throw std::runtime_error("pygobject_init failed");
    }
    if (libpython)
    {
        g_module_close(libpython);
    }

    /* init arguments passed to a python script*/
    static wchar_t tmp[] = L"";
    static wchar_t *empty_argv[] = {tmp};
    PySys_SetArgv(1, empty_argv);
}

void PythonContextInitializer::extendPath(const std::string &module_path)
{
    if (!module_path.empty())
    {
        /* PyList_Append increases the reference counter */
        PyList_Append(sys_path, __PYFILTER_WRAPPER(PyUnicode_FromString(module_path.c_str())));
    }
}

void PythonCallback::SetCaps(GstCaps *caps)
{
    assert(caps && "Expected vaild caps in PythonCallback::SetCaps!");
    caps_ptr = caps;
}

PythonError::PythonError()
{
    PyObject *ptype, *pvalue, *ptraceback;
    PyErr_Fetch(&ptype, &pvalue, &ptraceback);
    __PYFILTER_DECL_WRAPPER(io_module, PyImport_ImportModule("io"));
    py_stringio_constructor.reset(PyObject_GetAttrString(io_module, "StringIO"));
    __PYFILTER_DECL_WRAPPER(traceback_module, PyImport_ImportModule("traceback"));
    py_traceback_print_exception.reset(PyObject_GetAttrString(traceback_module, "print_exception"));
    traceback = get_python_error(ptype, pvalue, ptraceback);
    PyErr_Restore(ptype, pvalue, ptraceback);
}

char *PythonError::get_python_error(PyObject *ptype, PyObject *pvalue, PyObject *ptraceback)
{
    __PYFILTER_DECL_WRAPPER(py_stringio_instance, PyObject_CallObject(py_stringio_constructor, NULL));
    PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);
    __PYFILTER_DECL_WRAPPER(py_args,
                            Py_BuildValue("OOOOO", ptype ? ptype : Py_None, pvalue ? pvalue : Py_None,
                                          ptraceback ? ptraceback : Py_None, Py_None,
                                          (PyObject *)py_stringio_instance));
    __PYFILTER_DECL_WRAPPER(py_traceback_result,
                            PyObject_CallObject(py_traceback_print_exception, py_args));
    __PYFILTER_DECL_WRAPPER(py_getvalue, PyObject_GetAttrString(py_stringio_instance, "getvalue"));
    __PYFILTER_DECL_WRAPPER(py_result, PyObject_CallObject(py_getvalue, nullptr));

    return (char *)PyUnicode_AsUTF8(py_result);
}

char *PythonError::get()
{
    return traceback;
}
