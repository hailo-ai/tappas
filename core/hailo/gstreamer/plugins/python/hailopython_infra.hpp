/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <dlfcn.h>
#include <gmodule.h>
#include <gst/base/gstbasetransform.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <iostream>
#include <stdexcept>

using py_descriptor_t = unsigned long;

class PyObjectWrapper
{
    PyObject *object;
    std::string description;

public:
    PyObjectWrapper(PyObject *object = nullptr, const char *desc = nullptr) : object(object)
    {
        if (!object && desc)
        {
            throw std::runtime_error("Can't create PyObject " + std::string(desc));
        }
        description = desc != nullptr ? std::string(desc) : std::string();
    }

    operator PyObject *() { return object; }

    ~PyObjectWrapper()
    {
        if (object != nullptr)
        {
            GST_TRACE("~PyObjectWrapper() for %s", description.c_str());
        }
        Py_CLEAR(object);
    }

    PyObject *reset(PyObject *new_object = nullptr, const char *desc = nullptr)
    {
        description = desc != nullptr ? std::string(desc) : std::string();
        Py_CLEAR(object);
        object = new_object;
        return object;
    }

    PyObject *release()
    {
        PyObject *tmp = object;
        object = nullptr;
        return tmp;
    }

    PyObjectWrapper(const PyObjectWrapper &other) = delete;
    PyObjectWrapper &operator=(PyObjectWrapper other) = delete;
};

class PythonCallback
{
    PyObjectWrapper user_python_function;
    PyObjectWrapper get_python_roi_function;
    PyObjectWrapper python_frame_class;
    std::string module_name;
    GstCaps *caps_ptr;

public:
    PythonCallback(const char *module_path, const char *function_name,
                   const char *args_string, const char *kwargs_string);

    ~PythonCallback() = default;

    void SetCaps(GstCaps *caps);
    GstFlowReturn CallPython();
    GstFlowReturn CallPython(GstBuffer *buffer, py_descriptor_t desc);
};

class PythonContextInitializer
{
public:
    PythonContextInitializer();

    ~PythonContextInitializer();

    void initialize();

    void extendPath(const std::string &module_path);

private:
    PyGILState_STATE state;
    PyObject *sys_path;
};

GstFlowReturn set_python_callback_caps(PythonCallback *python_callback, GstCaps *caps, char **error_msg);
GstFlowReturn invoke_python_callback(PythonCallback *pycb, GstBuffer *buffer, py_descriptor_t desc, char **error_msg);
GstFlowReturn invoke_python_callback(PythonCallback *pycb, char **error_msg);
PythonCallback *create_python_callback(const char *module_path, const char *function_name,
                                       const char *args_string, const char *keyword_args_string, char **error_msg);

class PythonError
{
    PyObjectWrapper py_stringio_constructor;
    PyObjectWrapper py_traceback_print_exception;
    char *traceback;
    char *get_python_error(PyObject *ptype, PyObject *pvalue, PyObject *ptraceback);

public:
    PythonError();
    char *get();
};
