/* -*- Mode: C; c-basic-offset: 4 -*- */
/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2016-2021 Antenore Gatta, Giovanni Panozzo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of portions of this program with the
 *  OpenSSL library under certain conditions as described in each
 *  individual source file, and distribute linked combinations
 *  including the two.
 *  You must obey the GNU General Public License in all respects
 *  for all of the code used other than OpenSSL. *  If you modify
 *  file(s) with this exception, you may extend this exception to your
 *  version of the file(s), but you are not obligated to do so. *  If you
 *  do not wish to do so, delete this exception statement from your
 *  version. *  If you delete this exception statement from all source
 *  files in the program, then also delete it here.
 *
 */

#ifndef _PYGOBJECT_H_
#define _PYGOBJECT_H_

#include <Python.h>

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

/* PyGClosure is a _private_ structure */
typedef void (* PyClosureExceptionHandler) (GValue *ret, guint n_param_values, const GValue *params);
typedef struct _PyGClosure PyGClosure;
typedef struct _PyGObjectData PyGObjectData;

struct _PyGClosure {
    GClosure closure;
    PyObject *callback;
    PyObject *extra_args; /* tuple of extra args to pass to callback */
    PyObject *swap_data; /* other object for gtk_signal_connect__object */
    PyClosureExceptionHandler exception_handler;
};

typedef enum {
    PYGOBJECT_USING_TOGGLE_REF = 1 << 0,
    PYGOBJECT_IS_FLOATING_REF = 1 << 1
} PyGObjectFlags;

  /* closures is just an alias for what is found in the
   * PyGObjectData */
typedef struct {
    PyObject_HEAD
    GObject *obj;
    PyObject *inst_dict; /* the instance dictionary -- must be last */
    PyObject *weakreflist; /* list of weak references */

      /*< private >*/
      /* using union to preserve ABI compatibility (structure size
       * must not change) */
    union {
        GSList *closures; /* stale field; no longer updated DO-NOT-USE! */
        PyGObjectFlags flags;
    } private_flags;

} PyGObject;

#define pygobject_get(v) (((PyGObject *)(v))->obj)
#define pygobject_check(v,base) (PyObject_TypeCheck(v,base))

typedef struct {
    PyObject_HEAD
    gpointer boxed;
    GType gtype;
    gboolean free_on_dealloc;
} PyGBoxed;

#define pyg_boxed_get(v,t)      ((t *)((PyGBoxed *)(v))->boxed)
#define pyg_boxed_check(v,typecode) (PyObject_TypeCheck(v, &PyGBoxed_Type) && ((PyGBoxed *)(v))->gtype == typecode)

typedef struct {
    PyObject_HEAD
    gpointer pointer;
    GType gtype;
} PyGPointer;

#define pyg_pointer_get(v,t)      ((t *)((PyGPointer *)(v))->pointer)
#define pyg_pointer_check(v,typecode) (PyObject_TypeCheck(v, &PyGPointer_Type) && ((PyGPointer *)(v))->gtype == typecode)

typedef void (*PyGFatalExceptionFunc) (void);
typedef void (*PyGThreadBlockFunc) (void);

typedef struct {
    PyObject_HEAD
    GParamSpec *pspec;
} PyGParamSpec;

#define PyGParamSpec_Get(v) (((PyGParamSpec *)v)->pspec)
#define PyGParamSpec_Check(v) (PyObject_TypeCheck(v, &PyGParamSpec_Type))

typedef int (*PyGClassInitFunc) (gpointer gclass, PyTypeObject *pyclass);
typedef PyTypeObject * (*PyGTypeRegistrationFunction) (const gchar *name,
						       gpointer data);

struct _PyGObject_Functions {
    /*
     * All field names in here are considered private,
     * use the macros below instead, which provides stability
     */
    void (* register_class)(PyObject *dict, const gchar *class_name,
			    GType gtype, PyTypeObject *type, PyObject *bases);
    void (* register_wrapper)(PyObject *self);
    PyTypeObject *(* lookup_class)(GType type);
    PyObject *(* newgobj)(GObject *obj);

    GClosure *(* closure_new)(PyObject *callback, PyObject *extra_args,
			      PyObject *swap_data);
    void      (* object_watch_closure)(PyObject *self, GClosure *closure);
    GDestroyNotify destroy_notify;

    GType (* type_from_object)(PyObject *obj);
    PyObject *(* type_wrapper_new)(GType type);

    gint (* enum_get_value)(GType enum_type, PyObject *obj, gint *val);
    gint (* flags_get_value)(GType flag_type, PyObject *obj, gint *val);
    void (* register_gtype_custom)(GType gtype,
			    PyObject *(* from_func)(const GValue *value),
			    int (* to_func)(GValue *value, PyObject *obj));
    int (* value_from_pyobject)(GValue *value, PyObject *obj);
    PyObject *(* value_as_pyobject)(const GValue *value, gboolean copy_boxed);

    void (* register_interface)(PyObject *dict, const gchar *class_name,
				GType gtype, PyTypeObject *type);

    PyTypeObject *boxed_type;
    void (* register_boxed)(PyObject *dict, const gchar *class_name,
			    GType boxed_type, PyTypeObject *type);
    PyObject *(* boxed_new)(GType boxed_type, gpointer boxed,
			    gboolean copy_boxed, gboolean own_ref);

    PyTypeObject *pointer_type;
    void (* register_pointer)(PyObject *dict, const gchar *class_name,
			      GType pointer_type, PyTypeObject *type);
    PyObject *(* pointer_new)(GType boxed_type, gpointer pointer);

    void (* enum_add_constants)(PyObject *module, GType enum_type,
				const gchar *strip_prefix);
    void (* flags_add_constants)(PyObject *module, GType flags_type,
				 const gchar *strip_prefix);

    const gchar *(* constant_strip_prefix)(const gchar *name,
				     const gchar *strip_prefix);

    gboolean (* error_check)(GError **error);

    /* hooks to register handlers for getting GDK threads to cooperate
     * with python threading */
    void (* set_thread_block_funcs) (PyGThreadBlockFunc block_threads_func,
				     PyGThreadBlockFunc unblock_threads_func);
    PyGThreadBlockFunc block_threads;
    PyGThreadBlockFunc unblock_threads;
    PyTypeObject *paramspec_type;
    PyObject *(* paramspec_new)(GParamSpec *spec);
    GParamSpec *(*paramspec_get)(PyObject *tuple);
    int (*pyobj_to_unichar_conv)(PyObject *pyobj, void* ptr);
    gboolean (*parse_constructor_args)(GType        obj_type,
                                       char       **arg_names,
                                       char       **prop_names,
                                       GParameter  *params,
                                       guint       *nparams,
                                       PyObject   **py_args);
    PyObject *(* param_gvalue_as_pyobject) (const GValue* gvalue,
                                            gboolean copy_boxed,
					    const GParamSpec* pspec);
    int (* gvalue_from_param_pyobject) (GValue* value,
                                        PyObject* py_obj,
					const GParamSpec* pspec);
    PyTypeObject *enum_type;
    PyObject *(*enum_add)(PyObject *module,
			  const char *type_name_,
			  const char *strip_prefix,
			  GType gtype);
    PyObject* (*enum_from_gtype)(GType gtype, int value);

    PyTypeObject *flags_type;
    PyObject *(*flags_add)(PyObject *module,
			   const char *type_name_,
			   const char *strip_prefix,
			   GType gtype);
    PyObject* (*flags_from_gtype)(GType gtype, int value);

    gboolean threads_enabled;
    int       (*enable_threads) (void);

    int       (*gil_state_ensure) (void);
    void      (*gil_state_release) (int flag);

    void      (*register_class_init) (GType gtype, PyGClassInitFunc class_init);
    void      (*register_interface_info) (GType gtype, const GInterfaceInfo *info);
    void      (*closure_set_exception_handler) (GClosure *closure, PyClosureExceptionHandler handler);

    void      (*add_warning_redirection) (const char *domain,
                                          PyObject   *warning);
    void      (*disable_warning_redirections) (void);
    void      (*type_register_custom)(const gchar *type_name,
				      PyGTypeRegistrationFunction callback,
				      gpointer data);
    gboolean  (*gerror_exception_check) (GError **error);
    PyObject* (*option_group_new) (GOptionGroup *group);
    GType (* type_from_object_strict) (PyObject *obj, gboolean strict);
};

#ifndef _INSIDE_PYGOBJECT_

#if defined(NO_IMPORT) || defined(NO_IMPORT_PYGOBJECT)
extern struct _PyGObject_Functions *_PyGObject_API;
#else
struct _PyGObject_Functions *_PyGObject_API;
#endif

#define pygobject_register_class    (_PyGObject_API->register_class)
#define pygobject_register_wrapper  (_PyGObject_API->register_wrapper)
#define pygobject_lookup_class      (_PyGObject_API->lookup_class)
#define pygobject_new               (_PyGObject_API->newgobj)
#define pyg_closure_new             (_PyGObject_API->closure_new)
#define pygobject_watch_closure     (_PyGObject_API->object_watch_closure)
#define pyg_closure_set_exception_handler (_PyGObject_API->closure_set_exception_handler)
#define pyg_destroy_notify          (_PyGObject_API->destroy_notify)
#define pyg_type_from_object_strict   (_PyGObject_API->type_from_object_strict)
#define pyg_type_from_object        (_PyGObject_API->type_from_object)
#define pyg_type_wrapper_new        (_PyGObject_API->type_wrapper_new)
#define pyg_enum_get_value          (_PyGObject_API->enum_get_value)
#define pyg_flags_get_value         (_PyGObject_API->flags_get_value)
#define pyg_register_gtype_custom   (_PyGObject_API->register_gtype_custom)
#define pyg_value_from_pyobject     (_PyGObject_API->value_from_pyobject)
#define pyg_value_as_pyobject       (_PyGObject_API->value_as_pyobject)
#define pyg_register_interface      (_PyGObject_API->register_interface)
#define PyGBoxed_Type               (*_PyGObject_API->boxed_type)
#define pyg_register_boxed          (_PyGObject_API->register_boxed)
#define pyg_boxed_new               (_PyGObject_API->boxed_new)
#define PyGPointer_Type             (*_PyGObject_API->pointer_type)
#define pyg_register_pointer        (_PyGObject_API->register_pointer)
#define pyg_pointer_new             (_PyGObject_API->pointer_new)
#define pyg_enum_add_constants      (_PyGObject_API->enum_add_constants)
#define pyg_flags_add_constants     (_PyGObject_API->flags_add_constants)
#define pyg_constant_strip_prefix   (_PyGObject_API->constant_strip_prefix)
#define pyg_error_check             (_PyGObject_API->error_check)
#define pyg_set_thread_block_funcs  (_PyGObject_API->set_thread_block_funcs)
#define PyGParamSpec_Type           (*_PyGObject_API->paramspec_type)
#define pyg_param_spec_new          (_PyGObject_API->paramspec_new)
#define pyg_param_spec_from_object  (_PyGObject_API->paramspec_get)
#define pyg_pyobj_to_unichar_conv   (_PyGObject_API->pyobj_to_unichar_conv)
#define pyg_parse_constructor_args  (_PyGObject_API->parse_constructor_args)
#define pyg_param_gvalue_as_pyobject   (_PyGObject_API->value_as_pyobject)
#define pyg_param_gvalue_from_pyobject (_PyGObject_API->gvalue_from_param_pyobject)
#define PyGEnum_Type                (*_PyGObject_API->enum_type)
#define pyg_enum_add                (_PyGObject_API->enum_add)
#define pyg_enum_from_gtype         (_PyGObject_API->enum_from_gtype)
#define PyGFlags_Type               (*_PyGObject_API->flags_type)
#define pyg_flags_add               (_PyGObject_API->flags_add)
#define pyg_flags_from_gtype        (_PyGObject_API->flags_from_gtype)
#define pyg_enable_threads          (_PyGObject_API->enable_threads)
#define pyg_gil_state_ensure        (_PyGObject_API->gil_state_ensure)
#define pyg_gil_state_release       (_PyGObject_API->gil_state_release)
#define pyg_register_class_init     (_PyGObject_API->register_class_init)
#define pyg_register_interface_info (_PyGObject_API->register_interface_info)
#define pyg_add_warning_redirection   (_PyGObject_API->add_warning_redirection)
#define pyg_disable_warning_redirections (_PyGObject_API->disable_warning_redirections)
#define pyg_type_register_custom_callback (_PyGObject_API->type_register_custom)
#define pyg_gerror_exception_check (_PyGObject_API->gerror_exception_check)
#define pyg_option_group_new       (_PyGObject_API->option_group_new)

#define pyg_block_threads()   G_STMT_START {   \
    if (_PyGObject_API->block_threads != NULL) \
      (* _PyGObject_API->block_threads)();     \
  } G_STMT_END
#define pyg_unblock_threads() G_STMT_START {     \
    if (_PyGObject_API->unblock_threads != NULL) \
      (* _PyGObject_API->unblock_threads)();     \
  } G_STMT_END

#define pyg_threads_enabled (_PyGObject_API->threads_enabled)

#define pyg_begin_allow_threads                 \
    G_STMT_START {                              \
        PyThreadState *_save = NULL;            \
        if (_PyGObject_API->threads_enabled)    \
            _save = PyEval_SaveThread();
#define pyg_end_allow_threads                   \
        if (_PyGObject_API->threads_enabled)    \
            PyEval_RestoreThread(_save);        \
    } G_STMT_END


/**
 * pygobject_init:
 * @req_major: minimum version major number, or -1
 * @req_minor: minimum version minor number, or -1
 * @req_micro: minimum version micro number, or -1
 *
 * Imports and initializes the 'gobject' python module.  Can
 * optionally check for a required minimum version if @req_major,
 * @req_minor, and @req_micro are all different from -1.
 *
 * Returns: a new reference to the gobject module on success, NULL in
 * case of failure (and raises ImportError).
 **/
static inline PyObject *
pygobject_init(int req_major, int req_minor, int req_micro)
{
    PyObject *gobject, *cobject;

    gobject = PyImport_ImportModule("gi._gobject");
    if (!gobject) {
        if (PyErr_Occurred())
        {
            PyObject *type, *value, *traceback;
            PyObject *py_orig_exc;
            PyErr_Fetch(&type, &value, &traceback);
            py_orig_exc = PyObject_Repr(value);
            Py_XDECREF(type);
            Py_XDECREF(value);
            Py_XDECREF(traceback);


#if PY_VERSION_HEX < 0x03000000
            PyErr_Format(PyExc_ImportError,
                         "could not import gobject (error was: %s)",
                         PyString_AsString(py_orig_exc));
#else
            {
                /* Can not use PyErr_Format because it doesn't have
                 * a format string for dealing with PyUnicode objects
                 * like PyUnicode_FromFormat has
                 */
                PyObject *errmsg = PyUnicode_FromFormat("could not import gobject (error was: %U)",
                                                        py_orig_exc);

                if (errmsg) {
                   PyErr_SetObject(PyExc_ImportError,
                                   errmsg);
                   Py_DECREF(errmsg);
                }
                /* if errmsg is NULL then we might have OOM
                 * PyErr should already be set and trying to
                 * return our own error would be futile
                 */
            }
#endif
            Py_DECREF(py_orig_exc);
        } else {
            PyErr_SetString(PyExc_ImportError,
                            "could not import gobject (no error given)");
        }
        return NULL;
    }

    cobject = PyObject_GetAttrString(gobject, "_PyGObject_API");
#if PY_VERSION_HEX >= 0x03000000
    if (cobject && PyCapsule_CheckExact(cobject))
        _PyGObject_API = (struct _PyGObject_Functions *) PyCapsule_GetPointer(cobject, "gobject._PyGObject_API");

#else
    if (cobject && PyCObject_Check(cobject))
        _PyGObject_API = (struct _PyGObject_Functions *) PyCObject_AsVoidPtr(cobject);
#endif
    else {
        PyErr_SetString(PyExc_ImportError,
                        "could not import gobject (could not find _PyGObject_API object)");
        Py_DECREF(gobject);
        return NULL;
    }

    if (req_major != -1)
    {
        int found_major, found_minor, found_micro;
        PyObject *version;

        version = PyObject_GetAttrString(gobject, "pygobject_version");
        if (!version) {
            PyErr_SetString(PyExc_ImportError,
                            "could not import gobject (version too old)");
            Py_DECREF(gobject);
            return NULL;
        }
        if (!PyArg_ParseTuple(version, "iii",
                              &found_major, &found_minor, &found_micro)) {
            PyErr_SetString(PyExc_ImportError,
                            "could not import gobject (version has invalid format)");
            Py_DECREF(version);
            Py_DECREF(gobject);
            return NULL;
        }
        Py_DECREF(version);
        if (req_major != found_major ||
            req_minor >  found_minor ||
            (req_minor == found_minor && req_micro > found_micro)) {
            PyErr_Format(PyExc_ImportError,
                         "could not import gobject (version mismatch, %d.%d.%d is required, "
                         "found %d.%d.%d)", req_major, req_minor, req_micro,
                         found_major, found_minor, found_micro);
            Py_DECREF(gobject);
            return NULL;
        }
    }
    return gobject;
}

/**
 * PYLIST_FROMGLIBLIST:
 * @type: the type of the GLib list e.g. #GList or #GSList
 * @prefix: the prefix of functions that manipulate a list of the type
 * given by type.
 *
 * A macro that creates a type specific code block which converts a GLib
 * list (#GSList or #GList) to a Python list. The first two args of the macro
 * are used to specify the type and list function prefix so that the type
 * specific macros can be generated.
 *
 * The rest of the args are for the standard args for the type specific
 * macro(s) created from this macro.
 */
 #define PYLIST_FROMGLIBLIST(type,prefix,py_list,list,item_convert_func,\
                            list_free,list_item_free)  \
G_STMT_START \
{ \
    gint i, len; \
    PyObject *item; \
    void (*glib_list_free)(type*) = list_free; \
    GFunc glib_list_item_free = (GFunc)list_item_free;  \
 \
    len = prefix##_length(list); \
    py_list = PyList_New(len); \
    for (i = 0; i < len; i++) { \
        gpointer list_item = prefix##_nth_data(list, i); \
 \
        item = item_convert_func; \
        PyList_SetItem(py_list, i, item); \
    } \
    if (glib_list_item_free != NULL) \
        prefix##_foreach(list, glib_list_item_free, NULL); \
    if (glib_list_free != NULL) \
        glib_list_free(list); \
} G_STMT_END

/**
 * PYLIST_FROMGLIST:
 * @py_list: the name of the Python list
 *
 * @list: the #GList to be converted to a Python list
 *
 * @item_convert_func: the function that converts a list item to a Python
 * object. The function must refer to the list item using "@list_item" and
 * must return a #PyObject* object. An example conversion function is:
 * [[
 * PyString_FromString(list_item)
 * ]]
 * A more elaborate function is:
 * [[
 * pyg_boxed_new(GTK_TYPE_RECENT_INFO, list_item, TRUE, TRUE)
 * ]]
 * @list_free: the name of a function that takes a single arg (the list) and
 * frees its memory. Can be NULL if the list should not be freed. An example
 * is:
 * [[
 * g_list_free
 * ]]
 * @list_item_free: the name of a #GFunc function that frees the memory used
 * by the items in the list or %NULL if the list items do not have to be
 * freed. A simple example is:
 * [[
 * g_free
 * ]]
 *
 * A macro that adds code that converts a #GList to a Python list.
 *
 */
#define PYLIST_FROMGLIST(py_list,list,item_convert_func,list_free,\
                         list_item_free) \
        PYLIST_FROMGLIBLIST(GList,g_list,py_list,list,item_convert_func,\
                            list_free,list_item_free)

/**
 * PYLIST_FROMGSLIST:
 * @py_list: the name of the Python list
 *
 * @list: the #GSList to be converted to a Python list
 *
 * @item_convert_func: the function that converts a list item to a Python
 * object. The function must refer to the list item using "@list_item" and
 * must return a #PyObject* object. An example conversion function is:
 * [[
 * PyString_FromString(list_item)
 * ]]
 * A more elaborate function is:
 * [[
 * pyg_boxed_new(GTK_TYPE_RECENT_INFO, list_item, TRUE, TRUE)
 * ]]
 * @list_free: the name of a function that takes a single arg (the list) and
 * frees its memory. Can be %NULL if the list should not be freed. An example
 * is:
 * [[
 * g_list_free
 * ]]
 * @list_item_free: the name of a #GFunc function that frees the memory used
 * by the items in the list or %NULL if the list items do not have to be
 * freed. A simple example is:
 * [[
 * g_free
 * ]]
 *
 * A macro that adds code that converts a #GSList to a Python list.
 *
 */
#define PYLIST_FROMGSLIST(py_list,list,item_convert_func,list_free,\
                          list_item_free) \
        PYLIST_FROMGLIBLIST(GSList,g_slist,py_list,list,item_convert_func,\
                            list_free,list_item_free)

/**
 * PYLIST_ASGLIBLIST
 * @type: the type of the GLib list e.g. GList or GSList
 * @prefix: the prefix of functions that manipulate a list of the type
 * given by type e.g. g_list or g_slist
 *
 * A macro that creates a type specific code block to be used to convert a
 * Python list to a GLib list (GList or GSList). The first two args of the
 * macro are used to specify the type and list function prefix so that the
 * type specific macros can be generated.
 *
 * The rest of the args are for the standard args for the type specific
 * macro(s) created from this macro.
 */
#define PYLIST_ASGLIBLIST(type,prefix,py_list,list,check_func,\
                           convert_func,child_free_func,errormsg,errorreturn) \
G_STMT_START \
{ \
    Py_ssize_t i, n_list; \
    GFunc glib_child_free_func = (GFunc)child_free_func;        \
 \
    if (!(py_list = PySequence_Fast(py_list, ""))) { \
        errormsg; \
        return errorreturn; \
    } \
    n_list = PySequence_Fast_GET_SIZE(py_list); \
    for (i = 0; i < n_list; i++) { \
        PyObject *py_item = PySequence_Fast_GET_ITEM(py_list, i); \
 \
        if (!check_func) { \
            if (glib_child_free_func) \
                    prefix##_foreach(list, glib_child_free_func, NULL); \
            prefix##_free(list); \
            Py_DECREF(py_list); \
            errormsg; \
            return errorreturn; \
        } \
        list = prefix##_prepend(list, convert_func); \
    }; \
        Py_DECREF(py_list); \
        list =  prefix##_reverse(list); \
} \
G_STMT_END
/**
 * PYLIST_ASGLIST
 * @py_list: the Python list to be converted
 * @list: the #GList list to be converted
 * @check_func: the expression that takes a #PyObject* arg (must be named
 * @py_item) and returns an int value indicating if the Python object matches
 * the required list item type (0 - %False or 1 - %True). An example is:
 * [[
 * (PyString_Check(py_item)||PyUnicode_Check(py_item))
 * ]]
 * @convert_func: the function that takes a #PyObject* arg (must be named
 * py_item) and returns a pointer to the converted list object. An example
 * is:
 * [[
 * pygobject_get(py_item)
 * ]]
 * @child_free_func: the name of a #GFunc function that frees a GLib list
 * item or %NULL if the list item does not have to be freed. This function is
 * used to help free the items in a partially created list if there is an
 * error. An example is:
 * [[
 * g_free
 * ]]
 * @errormsg: a function that sets up a Python error message. An example is:
 * [[
 * PyErr_SetString(PyExc_TypeError, "strings must be a sequence of" "strings
 * or unicode objects")
 * ]]
 * @errorreturn: the value to return if an error occurs, e.g.:
 * [[
 * %NULL
 * ]]
 *
 * A macro that creates code that converts a Python list to a #GList. The
 * returned list must be freed using the appropriate list free function when
 * it's no longer needed. If an error occurs the child_free_func is used to
 * release the memory used by the list items and then the list memory is
 * freed.
 */
#define PYLIST_ASGLIST(py_list,list,check_func,convert_func,child_free_func,\
                       errormsg,errorreturn) \
        PYLIST_ASGLIBLIST(GList,g_list,py_list,list,check_func,convert_func,\
                          child_free_func,errormsg,errorreturn)

/**
 * PYLIST_ASGSLIST
 * @py_list: the Python list to be converted
 * @list: the #GSList list to be converted
 * @check_func: the expression that takes a #PyObject* arg (must be named
 * @py_item) and returns an int value indicating if the Python object matches
 * the required list item type (0 - %False or 1 - %True). An example is:
 * [[
 * (PyString_Check(py_item)||PyUnicode_Check(py_item))
 * ]]
 * @convert_func: the function that takes a #PyObject* arg (must be named
 * py_item) and returns a pointer to the converted list object. An example
 * is:
 * [[
 * pygobject_get(py_item)
 * ]]
 * @child_free_func: the name of a #GFunc function that frees a GLib list
 * item or %NULL if the list item does not have to be freed. This function is
 * used to help free the items in a partially created list if there is an
 * error. An example is:
 * [[
 * g_free
 * ]]
 * @errormsg: a function that sets up a Python error message. An example is:
 * [[
 * PyErr_SetString(PyExc_TypeError, "strings must be a sequence of" "strings
 * or unicode objects")
 * ]]
 * @errorreturn: the value to return if an error occurs, e.g.:
 * [[
 * %NULL
 * ]]
 *
 * A macro that creates code that converts a Python list to a #GSList. The
 * returned list must be freed using the appropriate list free function when
 * it's no longer needed. If an error occurs the child_free_func is used to
 * release the memory used by the list items and then the list memory is
 * freed.
 */
#define PYLIST_ASGSLIST(py_list,list,check_func,convert_func,child_free_func,\
                        errormsg,errorreturn) \
        PYLIST_ASGLIBLIST(GSList,g_slist,py_list,list,check_func,convert_func,\
                          child_free_func,errormsg,errorreturn)

#endif /* !_INSIDE_PYGOBJECT_ */

G_END_DECLS

#endif /* !_PYGOBJECT_H_ */
