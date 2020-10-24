#include "Python.h"
#include "structmember.h"

#include <stdio.h>
#include <math.h>

#include "ca_module.h"
#include "pv_addCb.h"
#include "pv_call.h"
#include "pv_cond.h"
#include "pv_delete.h"
#include "pv_enum.h"
#include "pv_get.h"
#include "pv_gettersetter.h"
#include "pv_init.h"
#include "pv_monitor.h"
#include "pv_put.h"
#include "pv_repr.h"
#include "pv_str.h"
#include "pv_buffer.h"

#include "ca_module_admin.h"

#include "epicsThread.h"
#include "errlog.h"

PyObject * python_camodule;
PyObject * python_ca_error;
int python_ca_destroyed;

epicsEventId pyCAevent;
int python_ca_preemtive_callback_is_enabled = 0;
#if !defined(PYTHON_CA_USE_GIL_API)
PyThreadState * pyCAThreadState;
#endif

/*******************************************************************
 * pvType
 *******************************************************************/

PyDoc_STRVAR(pvobject_doc, "Object type for Channel Access process variable.");

// methods

static struct PyMethodDef pv_methods[] =
{
  {"put", (PyCFunction) pv_put, METH_VARARGS, "put doc"},
  {"get", (PyCFunction) pv_get, METH_VARARGS | METH_KEYWORDS, "get doc"},
  {"enum", (PyCFunction) pv_enum, METH_VARARGS, "enum doc"},
  {
    "monitor", (PyCFunction) pv_monitor, METH_VARARGS,
    "monitor([1=on|0=0ff|-1=freeze])"
  },
  {
    "eventhandler", (PyCFunction) pv_eventhandler, METH_NOARGS,
    "eventhandler doc"
  },
  {
    "cond_wait", (PyCFunction) pv_cond_wait, METH_VARARGS,
    "cond_wait doc"
  },
  {
    "cond_signal", (PyCFunction) pv_cond_signal, METH_NOARGS,
    "cond_signal doc"
  },
  {
    "addCb", (PyCFunction) pv_addCb, METH_VARARGS | METH_KEYWORDS,
    "addCb doc"
  },
  {
    "remCb", (PyCFunction) pv_remCb, METH_VARARGS | METH_KEYWORDS,
    "remCb doc"
  },
  {NULL}      /* sentinel */
};

// attribute

static PyGetSetDef pv_getseters[] =
{
  {"pv_stats", (getter) pv_getter_pvstats, NULL, "pv stats doc", NULL}, /* read only */
  {"pv_val", (getter) pv_getter_pvval, NULL, "pv val doc", NULL}, /* read only */
  {"pv_tsval", (getter) pv_getter_pvtsval, NULL, "pv ts val doc", NULL},  /* read only */
  {NULL}      /* sentinel */
};

#ifndef PY_READONLY
#define PY_READONLY 1 // fixed in python3
#endif

static PyMemberDef pv_members[] =
{
  {"syncT", T_FLOAT, offsetof(pvobject, syncT), PY_READONLY, NULL},
  {NULL}      /* sentinel */
};

static PyMappingMethods pv_as_mapping =
{
  /* (lenfunc) */ NULL,
  /*mp_length - FIXME, type 2.7 */
  (binaryfunc) pv_dict_subscript, /*mp_subscript */
  (objobjargproc) NULL, /*mp_ass_subscript */
};

PyTypeObject pvType =
{
  PyObject_HEAD_INIT(&PyType_Type) 0, /* ob_size - deprecated */
  "ca.pv",    /* tp_name */
  sizeof(pvobject), /* tp_basicsize */
  0,      /* tp_itemsize */
  (destructor) pv_delete, /* tp_dealloc */
  0,      /* tp_print */
  0,      /* tp_getattr - deprecated */
  0,      /* tp_setattr - deprecated */
  0,      /* tp_compare */
  (reprfunc) pv_repr, /* tp_repr */
  0,      /* tp_as_number */
  0,      /* tp_as_sequence */
  &pv_as_mapping,   /* tp_as_mapping */
  0,      /* tp_hash */
  (ternaryfunc) pv_call,  /* tp_call */
  (reprfunc) pv_str,  /* tp_str */
  PyObject_GenericGetAttr,  /* tp_getattro */
  PyObject_GenericSetAttr,  /* tp_setattro */
  &pv_buffer_procs, /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
  pvobject_doc,   /* tp_doc */
  (traverseproc) pv_traverse, /* tp_traverse - for gc */
  (inquiry) pv_clear, /* tp_clear - for gc */
  0,      /* tp_richcompare */
  offsetof(pvobject, weakreflist),  /* tp_weaklistoffset */
  0,      /* tp_iter */
  0,      /* tp_iternext */
  pv_methods,   /* tp_methods */
  pv_members,   /* tp_members */
  pv_getseters,   /* tp_getset */
  0,      /* tp_base */
  0,      /* tp_dict */
  0,      /* tp_descr_get */
  0,      /* tp_descr_set */
  offsetof(pvobject, gpdict), /* tp_dictoffset */
  pv_init,    /* tp_init */
  PyType_GenericAlloc,  /* tp_alloc - PyType_GenericAlloc default allocates sizeof(pvType) */
  pv_new,     /* tp_new */
  0,      /* tp_free - PyObject_GC_Del default */
};

/*******************************************************************
 * NAME
 *  ca.pend
 *
 * DESCRIPTION
 *  ca_pend_io
 *
 * WARNING
 *
 *
 * RETURNS:
 *  Py_None:
 *    success
 *  Null:
 *    error.
 *
 * PERSONNEL:
 *    Matthieu Bec, Software Group, ING.  mdcb@ing.iac.es
 *
 * HISTORY:
 *    30/06/99 - first documented version
 *
 *******************************************************************/
PyDoc_STRVAR(python_ca_pend_doc, "pend([timeout])\n\
This function flushes the send buffer and then blocks until\n\
outstanding ca_get requests complete, and until channels created\n\
specifying nill connection handler function pointers connect for\n\
the first time.\n\
    * If ECA_NORMAL is returned then it can be safely assumed that\n\
    all outstanding ca_get requests have completed successfully\n\
    and channels created specifying nill connection handler function\n\
    pointers have connected for the first time.\n\
    * If ECA_TIMEOUT is returned then it must be assumed for\n\
    all previous ca_get requests and properly qualified first\n\
    time channel connects have failed.\n\
If ECA_TIMEOUT is returned then get requests may be reissued followed\n\
by a subsequent call to ca_pend_io(). Specifically, the function will\n\
block only for outstanding ca_get requests issued, and also any channels\n\
created specifying a nill connection handler function pointer, after the\n\
last call to ca_pend_io() or ca client context creation whichever is later.\n\
Note that ca_create_channel requests generally should not be reissued for \n\
the same process variable unless ca_clear_channel is called first.\n\
\n\
If no ca_get or connection state change events are outstanding then ca_pend_io()\n\
will flush the send buffer and return immediately without processing any\n\
outstanding channel access background activities.\n\
\n\
The delay specified to ca_pend_io() should take into account worst case network delays\n\
such as Ethernet collision exponential back off until retransmission delays which\n\
can be quite long on overloaded networks.\n\
\n\
Unlike ca_pend_event, this routine will not process CA's background activities\n\
if none of the selected IO requests are pending.");

PyObject * python_ca_pend(PyObject * self, PyObject * args)
{
  float timeout;

  timeout = ca_module_admin_syncT;

  if (!PyArg_ParseTuple(args, "|f", &timeout))
    { PYCA_USAGE("pend([timeout])"); }

  Debug(10, "ca_pend_io %f sec.\n", timeout);

  PYCA_SEVCHK(ca_pend_io(timeout), "ca.pend_io failed");

  Py_INCREF(Py_None);
  return Py_None;
}

/*******************************************************************
 * NAME
 *  ca.flush
 *
 * DESCRIPTION
 *  ca_flush_io
 *
 * WARNING
 *
 *
 * RETURNS:
 *  Py_None:
 *    success
 *  Null:
 *    error.
 *
 * PERSONNEL:
 *    Matthieu Bec, Software Group, ING.  mdcb@ing.iac.es
 *
 * HISTORY:
 *    30/06/99 - first documented version
 *
 *******************************************************************/
PyDoc_STRVAR(python_ca_flush_doc, "flush()\n\
Flush outstanding IO requests to the server.\n\
This routine might be useful to users who need to flush\n\
requests prior to performing client side labor in parallel\n\
with labor performed in the server.\n\
Outstanding requests are also sent whenever\n\
the buffer which holds them becomes full.");

PyObject * python_ca_flush(PyObject * self, PyObject * args)
{

  Debug(10, "ca_flush_io\n");

  PYCA_SEVCHK(ca_flush_io(), "ca.flush failed");

  Py_INCREF(Py_None);
  return Py_None;
}

/*******************************************************************
 * NAME
 *  ca.test
 *
 * DESCRIPTION
 *  ca_test_io
 *
 * WARNING
 *
 *
 * RETURNS:
 *  Py_None:
 *    success
 *  Null:
 *    error.
 *
 * PERSONNEL:
 *    Matthieu Bec, Gemini Observatory.  mbec@gemini.edu
 *
 * HISTORY:
 *    12/05/11 - first documented version
 *
 *******************************************************************/
PyDoc_STRVAR(python_ca_test_doc, "test()\n\
This function tests to see if all ca_get requests are complete and\n\
channels created specifying a nill connection callback function pointer\n\
are connected. It will report the status of outstanding ca_get requests issued,\n\
and channels created specifying a nill connection callback function pointer,\n\
after the last call to ca_pend_io() or CA context initialization whichever is\n\
later.\n\
Returns 0 for ECA_IODONE or 1 for ECA_IOINPROGRESS");

PyObject * python_ca_test(PyObject * self, PyObject * args)
{

  Debug(10, "ca_test_io\n");

  return PyLong_FromLong(ca_test_io() != ECA_IODONE);

}

/*******************************************************************
 * NAME
 *  ca.poll
 *
 * DESCRIPTION
 *  ca_poll
 *
 * WARNING
 *
 *
 * RETURNS:
 *  Py_None:
 *    success
 *  Null:
 *    error.
 *
 * PERSONNEL:
 *    Matthieu Bec, Software Group, ING.  mdcb@ing.iac.es
 *
 * HISTORY:
 *    30/06/99 - first documented version
 *
 *******************************************************************/
PyDoc_STRVAR(python_ca_poll_doc, "poll()\n\
Send buffer is flushed and any outstanding CA background activity is processed.");

PyObject * python_ca_poll(PyObject * self, PyObject * args)
{
  int status;

  Debug(10, "ca_poll\n");

  // Both ca_pend_event and ca_poll return ECA_TIMEOUT when successful.
  // This behavior probably isn't intuitive, but it is preserved to
  // insure backwards compatibility.

  if (!PyArg_ParseTuple(args, ""))
    {
      PYCA_USAGE
      ("ca.poll() takes no argument, use ca.pend([timeout]) instead");
    }

  if ((status = ca_poll()) != ECA_TIMEOUT)
    {
      PyErr_Format(python_ca_error, "%s - %s (file:%s line:%d)",
                   "ca.poll failed",
                   ca_message(status), __FILE__, __LINE__);
      return NULL;
    }

  Py_INCREF(Py_None);
  return Py_None;
}

/*******************************************************************
 * NAME
 *  python_ca_init
 *
 * DESCRIPTION
 *     init(async=True|False)
 *
 * WARNING
 *
 *
 * RETURNS:
 *     Py_None: success
 *     Null: error
 *
 * PERSONNEL:
 *    Matthieu Bec, Gemini Observatory mbec@gemini.edu
 *
 * HISTORY:
 *    27/08/07 - first documented version
 *
 *******************************************************************/
PyDoc_STRVAR(python_ca_init_doc, "init([async=True|False])");

PyObject * python_ca_init(PyObject * self, PyObject * args, /* optional (tuple) async argument */
                          PyObject * kwds)
{

  static char * kwlist[] = { "async", NULL };

  int async = 1;

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "|i", kwlist, &async))
    { PYCA_USAGE("init([async=True|False])"); }

  if (async)
    {
      // if we already have a context, check async is consistent
      if (ca_current_context() != NULL)
        {
          if (!ca_preemtive_callback_is_enabled())
            {
              PYCA_ERRMSG
              ("re-init channel access mismatched preemption");
              return 0;
            }

          else
            {
              Debug(2, "context already exists\n");
              Py_INCREF(Py_None);
              return Py_None;
            }
        }

      Debug(2, "ca_enable_preemptive_callback\n");
      PYCA_SEVCHK(ca_context_create(ca_enable_preemptive_callback),
                  "ca.init failed");
      python_ca_preemtive_callback_is_enabled = 1;
    }

  else
    {
      // if we already have a context, check async is consistent
      if (ca_current_context() != NULL)
        {
          if (ca_preemtive_callback_is_enabled())
            {
              PYCA_ERRMSG
              ("re-init channel access mismatched preemption");
              return 0;
            }

          else
            {
              Debug(2, "context already exists\n");
              Py_INCREF(Py_None);
              return Py_None;
            }
        }

      Debug(2, "ca_disable_preemptive_callback\n");
      PYCA_SEVCHK(ca_context_create(ca_disable_preemptive_callback),
                  "ca.init failed");
      python_ca_preemtive_callback_is_enabled = 0;
    }

  // initialization passed, add the context object to the module
  ca_module_admin_singleton_register(python_camodule);

  Py_INCREF(Py_None);
  return Py_None;
}

/*******************************************************************
 * NAME:
 *  python_ca_get - oneshot caget
 *
 * DESCRIPTION:
 *  interpreter interface to channel access get
 *
 * WARNING:
 *
 *
 * RETURNS:
 *  the pv value
 *
 * PERSONNEL:
 *    Matthieu Bec, Gemini Observatory - mbec@gemini.edu\b
 *
 * HISTORY:
 *    08/09/05 - first documented version
 *
 *******************************************************************/
PyDoc_STRVAR(python_ca_get_doc,
             "get('pvname',timeout=10.0, mode='terse|wide')");

PyObject * python_ca_get(PyObject * self, PyObject * args /* pv name as string */
                        )
{
  char * pvname;
  PyObject * pv;
  PyObject * retval;

  if (!PyArg_ParseTuple(args, "s", &pvname))
    { PYCA_USAGE("get('pvname')"); }

  if (!strcmp(pvname, ""))
    { PYCA_USAGE("get('pvname')"); }

  Debug(5, "get %s\n", pvname);

  // create - use fabsf ca_module_admin_syncT if admin is set to run asynchronous
  Py_INCREF(Py_None);
  pv = PyObject_CallMethod((PyObject *) python_camodule, "pv", "sfOi",
                           pvname, fabsf(ca_module_admin_syncT), Py_None,
                           1);
  Py_DECREF(Py_None);

  // error message already set so just return
  if (pv == NULL)
    { return NULL; }

  retval = PyObject_CallMethod((PyObject *) pv, "get", "");

  Py_DECREF(pv);

  return retval;

}

/*******************************************************************
 * NAME:
 *  python_ca_put - oneshot  caput
 *
 * DESCRIPTION:
 *  interpreter interface to channel access put
 *
 * WARNING:
 *
 * RETURNS:
 *  Ok, Error
 *
 * PERSONNEL:
 *    Matthieu Bec, Gemini Observatory - mbec@gemini.edu\b
 *
 * HISTORY:
 *    08/09/05 - first documented version
 *
 *******************************************************************/
PyDoc_STRVAR(python_ca_put_doc, "put('pvname',value)\n\
put('pvname',(val1, val2, ..))");

PyObject * python_ca_put(PyObject * self, PyObject * args)
{
  char * pvname;
  PyObject * argstuple;
  PyObject * pv;
  PyObject * retval;

  if (!PyArg_ParseTuple(args, "sO", &pvname, &argstuple))
    { PYCA_USAGE("put('pvname',val,[...])"); }

  if (!strcmp(pvname, ""))
    { PYCA_USAGE("put('pvname',val,[...])"); }

  Debug(5, "put %s\n", pvname);

  // create - use fabsf ca_module_admin_syncT if admin is set to run asynchronous
  Py_INCREF(Py_None);
  pv = PyObject_CallMethod((PyObject *) python_camodule, "pv", "sfOi",
                           pvname, fabsf(ca_module_admin_syncT), Py_None,
                           1);
  Py_DECREF(Py_None);

  // error message already set so just return
  if (pv == NULL)
    { return NULL; }

  retval = PyObject_CallMethod((PyObject *) pv, "put", "O", argstuple);

  Py_DECREF(pv);

  return retval;

}

/*******************************************************************
 * NAME:
 *  python_ca_client_status - client_status
 *
 * DESCRIPTION:
 *  interpreter interface to channel access client_status
 *
 * WARNING:
 *
 * RETURNS:
 *  Ok, Error
 *
 * PERSONNEL:
 *    Matthieu Bec, Gemini Observatory - mbec@gemini.edu\b
 *
 * HISTORY:
 *    08/11/05 - first documented version
 *
 *******************************************************************/
PyDoc_STRVAR(python_ca_client_status_doc, "client_status([level])");

PyObject * python_ca_client_status(PyObject * self, PyObject * args)
{
  int level = 0;

  if (!PyArg_ParseTuple(args, "|i", &level))
    { PYCA_USAGE("client_status([level])"); }

  ca_client_status(level);

  Py_INCREF(Py_None);
  return Py_None;
}

/*******************************************************************
 *  python_ca_printf_handler_func
 *******************************************************************/

static PyObject * python_ca_printf_handler_func = NULL;

int python_ca_printf_handler(const char * pformat, va_list args)
{
  PyObject * retval;
  PyObject * msg;

//     printf("~~~~~~~~~~~~~~ hanlder ~~~~~~~~~~~~~~~~~~~\n");
//     PyObject_Print(msg,stdout, Py_PRINT_RAW);
//
//    int status;
//    status = vfprintf( stderr, pformat, args);
//    return status;

  PYTHON_CA_LOCK;   // TODO - acquire the lock or not?
  msg = PyString_FromFormatV(pformat, args);
  retval =
    PyObject_CallFunctionObjArgs(python_ca_printf_handler_func, msg,
                                 NULL);

  if (retval == NULL)
    {
      printf("ERROR in callback! FIXME: PyErr_Clear?\n");

      // TODO: redundant?
      if (PyErr_Occurred())
        {
          PyErr_Print();
          PyErr_Clear();
        }
    }

  else
    {
      Py_DECREF(retval);
    }

  Py_DECREF(msg);

  PYTHON_CA_UNLOCK;
  return 0;
}

/*******************************************************************
 * NAME:
 *  python_ca_replace_printf_handler
 *
 * DESCRIPTION:
 *
 *
 * WARNING:
 *
 * RETURNS:
 *
 * PERSONNEL:
 *    Matthieu Bec, Gemini Observatory - mbec@gemini.edu\b
 *
 * HISTORY:
 *
 *
 *******************************************************************/
PyDoc_STRVAR(python_ca_replace_printf_handler_doc,
             "replace_printf_handler(handler)");

PyObject * python_ca_replace_printf_handler(PyObject * self, PyObject * args)
{
  int status;   // TODO: do something with it

  // previous handler ref count
  if (python_ca_printf_handler_func)
    {
      Py_DECREF(python_ca_printf_handler_func);
      python_ca_printf_handler_func = NULL;
    }

  if (!PyArg_ParseTuple(args, "O", &python_ca_printf_handler_func))
    {
      PYCA_USAGE("replace_printf_handler(handler)");
    }

  if (!PyCallable_Check(python_ca_printf_handler_func))
    {
      PYCA_USAGE("replace_printf_handler(handler=callable)");
    }

  if ((status =
         ca_replace_printf_handler(python_ca_printf_handler)) !=
      ECA_NORMAL)
    {
      python_ca_printf_handler_func = NULL;
      PYCA_ERR("channel access: %s", ca_message(status));
    }

  // new handler ref count
  Py_INCREF(python_ca_printf_handler_func);

  Py_INCREF(Py_None);
  return Py_None;
}

/*******************************************************************
 *  python_ca_exception_event_handler
 *******************************************************************/
static PyObject * python_ca_exception_event_func = NULL;

void python_ca_exception_event_handler(struct exception_handler_args exception)
{
  PyObject * tmp;
  PyObject * retval;

  PYTHON_CA_LOCK;

  if (exception.chid)
    {
      tmp = PyString_FromString(ca_name(exception.chid));
    }

  else
    {
      tmp = PyString_FromString(exception.ctx);
    }

  retval =
    PyObject_CallFunctionObjArgs(python_ca_exception_event_func, tmp,
                                 NULL);

  if (retval == NULL)
    {
      printf("ERROR in callback! FIXME: PyErr_Clear?\n");

      // FIXME: redundant?
      if (PyErr_Occurred())
        {
          PyErr_Print();
          PyErr_Clear();
        }
    }

  else
    {
      Py_DECREF(retval);
    }

  Py_DECREF(tmp);

  PYTHON_CA_UNLOCK;
}

/*******************************************************************
 * NAME:
 *  python_ca_add_exception_event
 *
 * DESCRIPTION:
 *
 *
 * WARNING:
 *
 * RETURNS:
 *
 * PERSONNEL:
 *    Matthieu Bec, Gemini Observatory - mbec@gemini.edu\b
 *
 * HISTORY:
 *
 *
 *******************************************************************/
PyDoc_STRVAR(python_ca_add_exception_event_doc, "add_exception_event(handler");

PyObject * python_ca_add_exception_event(PyObject * self, PyObject * args)
{
  int status;   // TODO: do something with it

  // previous handler ref count
  if (python_ca_exception_event_func)
    {
      Py_DECREF(python_ca_exception_event_func);
      python_ca_exception_event_func = NULL;
    }

  if (!PyArg_ParseTuple(args, "O", &python_ca_exception_event_func))
    {
      PYCA_USAGE("add_exception_event(handler)");
    }

  if (!PyCallable_Check(python_ca_exception_event_func))
    {
      PYCA_ERR("Not a valid add_exception_event");
    }

  if ((status = ca_add_exception_event(python_ca_exception_event_handler,
                                       NULL)) != ECA_NORMAL)
    {
      python_ca_exception_event_func = NULL;
      PYCA_ERR("channel access: %s", ca_message(status));
    }

  // new handler ref count
  Py_INCREF(python_ca_exception_event_func);

  Py_INCREF(Py_None);
  return Py_None;
}

/*******************************************************************
 * NAME:
 *  python_ca_eltc
 *
 * DESCRIPTION:
 *
 * WARNING:
 *
 * RETURNS:
 *
 * PERSONNEL:
 *    Matthieu Bec, Gemini Observatory - mbec@gemini.edu\b
 *
 * HISTORY:
 *
 *******************************************************************/
PyDoc_STRVAR(python_ca_eltc_doc, "eltc(yesno)");

PyObject * python_ca_eltc(PyObject * self, PyObject * args)
{
  int yesno;

  if (!PyArg_ParseTuple(args, "i", &yesno))
    { PYCA_USAGE("eltc(yesno)"); }

  eltc(yesno);

  Py_INCREF(Py_None);
  return Py_None;
}

/*******************************************************************
 * NAME:
 *  python_ca_errlogListener
 *
 * DESCRIPTION:
 *  interpreter interface to errlog errlogAddListener
 *
 * WARNING:
 *
 * RETURNS:
 *
 * PERSONNEL:
 *
 * HISTORY:
 *
 *******************************************************************/
PyDoc_STRVAR(python_ca_elal_doc, "errlogAddListener(listener)");

static PyObject * python_ca_errlogListenerFunc = NULL;

void python_ca_errlogListener(void * pPrivate, const char * message)
{
  PyObject * retval;
  PyObject * msg = PyString_FromString(message);

  // FIXME - acquire or not the log?
  PYTHON_CA_LOCK;
  retval =
    PyObject_CallFunctionObjArgs(python_ca_errlogListenerFunc, msg,
                                 NULL);

  if (retval == NULL)
    {
      printf("ERROR in callback! FIXME: PyErr_Clear?\n");

      // FIXME: redundant?
      if (PyErr_Occurred())
        {
          PyErr_Print();
          PyErr_Clear();
        }
    }

  else
    {
      Py_DECREF(retval);
    }

  PYTHON_CA_UNLOCK;
}

/*******************************************************************
 * NAME:
 *  python_ca_elal
 *
 * DESCRIPTION:
 *
 *
 * WARNING:
 *
 * RETURNS:
 *
 * PERSONNEL:
 *    Matthieu Bec, Gemini Observatory - mbec@gemini.edu\b
 *
 * HISTORY:
 *
 *
 *******************************************************************/

PyObject * python_ca_elal(PyObject * self, PyObject * args)
{

  // previous handler ref count
  if (python_ca_errlogListenerFunc)
    {
      Py_DECREF(python_ca_errlogListenerFunc);
      //python_ca_errlogListenerFunc=NULL;
    }

  if (!PyArg_ParseTuple(args, "O", &python_ca_errlogListenerFunc))
    {
      PYCA_USAGE("elal(listener)");
    }

  if (!PyCallable_Check(python_ca_errlogListenerFunc))
    {
      PYCA_ERR("listener not callable");
    }

  // new handler ref count
  Py_INCREF(python_ca_errlogListenerFunc);

  errlogAddListener(python_ca_errlogListener, NULL);  /* NULL private */

  Py_INCREF(Py_None);
  return Py_None;
}

/*******************************************************************
 * NAME:
 *  python_ca_quiet - shut off all console errors
 *
 * DESCRIPTION:
 *
 * WARNING:
 *
 * RETURNS:
 *
 * PERSONNEL:
 *    Matthieu Bec, Gemini Observatory - mbec@gemini.edu\b
 *
 * HISTORY:
 *
 *******************************************************************/

static int python_ca_quiet_printf_handler(const char * pformat, va_list args)
{
  return 0;
};

static void
python_ca_quiet_exception_event_handler(struct exception_handler_args exception)
{
};

PyDoc_STRVAR(python_ca_quiet_doc, "quiet()\n\
redirect ca anomalous messages to /dev/null.");

PyObject * python_ca_quiet(PyObject * self, PyObject * args)
{

  eltc(0);
  ca_replace_printf_handler(python_ca_quiet_printf_handler);
  ca_add_exception_event(python_ca_quiet_exception_event_handler, NULL);
  Py_INCREF(Py_None);
  return Py_None;
}

/*******************************************************************
 * NAME:
 *  python_ca_lock - locks the main python interpreter
 *
 * DESCRIPTION:
 *  Locks the main interpreter in view of being unlocked by a monitor or timeout
 *
 * WARNING:
 *
 * RETURNS:
 *
 * PERSONNEL:
 *    Matthieu Bec, Gemini Observatory - mbec@gemini.edu\b
 *
 * HISTORY:
 *    08/11/05 - first documented version
 *
 *******************************************************************/

PyDoc_STRVAR(python_ca_lock_doc,
             "lock([timeout]) -> 0: Ok, -1: error or timeout\n\
   args: timeout in seconds and fractions,\n\
         negative value are interpreted as forever\n\
   default: forever\n\
locks the global interpreter.");

PyObject * python_ca_lock(PyObject * self, PyObject * args)
{
  PyObject * status;
  float timeout = -1.0;
  epicsEventWaitStatus st;

  if (!PyArg_ParseTuple(args, "|f", &timeout))
    { PYCA_USAGE("lock([timeout])"); }

  if (timeout < 0)
    {
      // forever
      Py_BEGIN_ALLOW_THREADS st = epicsEventWait(pyCAevent);
      Py_END_ALLOW_THREADS
    }

  else
    {
      Py_BEGIN_ALLOW_THREADS
      st = epicsEventWaitWithTimeout(pyCAevent, timeout);
      Py_END_ALLOW_THREADS
    }

  status = PyLong_FromLong(st);

  Py_INCREF(status);
  return status;
}

/*******************************************************************
 * NAME:
 *  python_ca_unlock - unlocks the main python interpreter
 *
 * DESCRIPTION:
 *  unlocks the main interpreter
 *
 * WARNING:
 *
 * RETURNS:
 *
 * PERSONNEL:
 *    Matthieu Bec, Gemini Observatory - mbec@gemini.edu\b
 *
 * HISTORY:
 *    08/11/05 - first documented version
 *
 *******************************************************************/

PyDoc_STRVAR(python_ca_unlock_doc, "unlock()\n\
unlocks the global interpreter.");

PyObject * python_ca_unlock(PyObject * self, PyObject * args)
{

  epicsEventSignal(pyCAevent);

  Py_INCREF(Py_None);
  return Py_None;
}

#ifdef PYTHON_CA_DEVCORE
PyObject * objrefcnt(PyObject * self, PyObject * args)
{
  PyObject * obj;

  if (!PyArg_ParseTuple(args, "O", &obj))
    { PYCA_USAGE("refcnt(object)"); }

  return PyLong_FromLong(obj->ob_refcnt);
}

PyObject * GILlock(PyObject * self, PyObject * args)
{
  printf("acquiring lock..");
  PyEval_AcquireLock();
  Py_INCREF(Py_None);
  printf("done\n");
  return Py_None;
}

PyObject * GILunlock(PyObject * self, PyObject * args)
{
  Py_INCREF(Py_None);
  printf("releasing lock..");
  PyEval_ReleaseLock();
  printf("done\n");
  return Py_None;

}
#endif

/*******************************************************************
 * NAME:
 *  python_ca_exit
 *
 * DESCRIPTION:
 *
 * WARNING:
 *
 * RETURNS:
 *
 * PERSONNEL:
 *    Matthieu Bec, Gemini Observatory - mbec@gemini.edu\b
 *
 * HISTORY:
 *
 *******************************************************************/
PyDoc_STRVAR(python_ca_exit_doc, "context_destroy()");

PyObject * python_ca_exit(PyObject * self, PyObject * args)
{

//    Debug(0, "ca_current_context %d\n", ca_current_context());
//    Debug(0, "ca_preemtive_callback_is_enabled %d\n", ca_preemtive_callback_is_enabled());

  // todo - disable all the callbacks from admin facility

  python_ca_destroyed = 1;

  Debug(3, "ca_context_destroy..\n");

//    ca_poll();
//    ca_poll();
//    ca_poll();

  ca_context_destroy();

  Debug(3, "ca_context_destroyed\n");

  python_ca_destroyed = 2;

  Py_INCREF(Py_None);
  return Py_None;
}

/*******************************************************************
 * python_ca_dead
 *
 * DESCRIPTION:
 *
 * WARNING:
 *
 * RETURNS:
 *
 * PERSONNEL:
 *    Matthieu Bec, Gemini Observatory - mbec@gemini.edu\b
 *
 * HISTORY:
 *    // - first documented version
 *
 *******************************************************************/

static void python_ca_dead(void)
{
  // there is not much we can do here
  // the interpreter is dead

  python_ca_destroyed = 3;

}

/*******************************************************************
 * module static method
 *******************************************************************/

static struct PyMethodDef python_ca_methods[] =
{
  {
    "init", (PyCFunction) python_ca_init, METH_VARARGS | METH_KEYWORDS,
    python_ca_init_doc
  },
  {"exit", (PyCFunction) python_ca_exit, METH_NOARGS, python_ca_exit_doc},
  {
    "poll", (PyCFunction) python_ca_poll, METH_VARARGS,
    python_ca_poll_doc
  },
// these are really i/o
  {
    "pend", (PyCFunction) python_ca_pend, METH_VARARGS,
    python_ca_pend_doc
  },
  {
    "flush", (PyCFunction) python_ca_flush, METH_NOARGS,
    python_ca_flush_doc
  },
  {"test", (PyCFunction) python_ca_test, METH_NOARGS, python_ca_test_doc},
  {"get", (PyCFunction) python_ca_get, METH_VARARGS, python_ca_get_doc},
  {"put", (PyCFunction) python_ca_put, METH_VARARGS, python_ca_put_doc},
  {
    "client_status", (PyCFunction) python_ca_client_status, METH_VARARGS,
    python_ca_client_status_doc
  },
  {
    "lock", (PyCFunction) python_ca_lock, METH_VARARGS,
    python_ca_lock_doc
  },
  {
    "unlock", (PyCFunction) python_ca_unlock, METH_NOARGS,
    python_ca_unlock_doc
  },
  {
    "quiet", (PyCFunction) python_ca_quiet, METH_NOARGS,
    python_ca_quiet_doc
  },
  {
    "replace_printf_handler",
    (PyCFunction) python_ca_replace_printf_handler,
    METH_VARARGS, python_ca_replace_printf_handler_doc
  },
  {
    "errlog_to_console", (PyCFunction) python_ca_eltc, METH_VARARGS,
    python_ca_eltc_doc
  },
  {
    "errlog_add_listener", (PyCFunction) python_ca_elal, METH_VARARGS,
    python_ca_elal_doc
  },
  {
    "add_exception_event", (PyCFunction) python_ca_add_exception_event,
    METH_VARARGS, python_ca_add_exception_event_doc
  },
#ifdef PYTHON_CA_DEVCORE
  {"refcnt", (PyCFunction) objrefcnt, METH_VARARGS, "refcnt doc"},
  {"GILlock", (PyCFunction) GILlock, METH_VARARGS, "GILlock doc"},
  {"GILunlock", (PyCFunction) GILunlock, METH_VARARGS, "GILunlock doc"},
#endif
  {NULL}      /* sentinel */
};

/*******************************************************************
 * NAME
 *  initca - module initialization
 *
 * DESCRIPTION
 *  Initialize ca module: define error strings, global
 * variables, pv type, etc.
 *
 * WARNING
 *  this procedure must be called <modulename>init
 *
 * PYTHON API:
 *  import ca
 *
 * PERSONNEL:
 *    Matthieu Bec, ING Software Group - mdcb@ing.iac.es
 *    Matthieu Bec, Gemini Software Group -  mbec@gemini.edu
 *
 * HISTORY:
 *    30/06/99 - first documented version
 *    08/03/04 - multi-thread, epics R3.14, python2.2
 *
 *******************************************************************/
PyDoc_STRVAR(python_ca_doc, "Channel Access for Python.");

PyMODINIT_FUNC initca(void)
{

#ifdef PYTHON_CA_HELPERS
  PyObject * pca_DBR_TYPE;
  PyObject * pca_CS;
#endif

  // initialize Python with thread support
  Py_Initialize();

  int releaseLockWhenLeaving;

// we only need this at ca_init time
  if (PyEval_ThreadsInitialized())
    {
      releaseLockWhenLeaving = 0;
    }

  else
    {
      PyEval_InitThreads(); /* this acquires the lock */
      releaseLockWhenLeaving = 1;
    }

#if !defined(PYTHON_CA_NODEBUG)
  main_thread_ctxt = pthread_self();
#endif

#if !defined(PYTHON_CA_USE_GIL_API)
  PyThreadState * mainThreadState;
  PyInterpreterState * mainInterpreterState;
  mainThreadState = PyThreadState_Get();
  mainInterpreterState = mainThreadState->interp;
  pyCAThreadState = PyThreadState_New(mainInterpreterState);
#endif

  if (PyType_Ready(&pvType) < 0)
    { return; }

  if (ca_module_admin_singleton_create() < 0)
    { return; }

  /* Create the module and associated methods */
  if ((python_camodule =
         Py_InitModule3("ca", python_ca_methods, python_ca_doc)) == NULL)
    { return; }

  /* Register new pv type */

  /* gcc says dereferencing type-punned pointer will break strict-aliasing rules */
  union
  {
    PyTypeObject * ptop;
    PyObject * pop;
  } pun =
  {
    &pvType
  };
  Py_INCREF(pun.pop);

  PyModule_AddObject(python_camodule, "pv", (PyObject *) & pvType);

  /* Add symbolic constants to the module */
  PyModule_AddStringConstant(python_camodule, "version",
                             PYTHON_CA_VERSION);

#ifdef PYTHON_CA_HELPERS
  /* Add dictionary helpers */
  /* 2.7 has better macros, btw. */
  pca_CS = PyDict_New();
  PyDict_SetItemString(pca_CS, "cs_never_conn",
                       PyLong_FromLong(cs_never_conn));
  PyDict_SetItemString(pca_CS, "cs_prev_conn",
                       PyLong_FromLong(cs_prev_conn));
  PyDict_SetItemString(pca_CS, "cs_conn", PyLong_FromLong(cs_conn));
  PyDict_SetItemString(pca_CS, "cs_closed", PyLong_FromLong(cs_closed));
  Py_INCREF(pca_CS);
  PyModule_AddObject(python_camodule, "CS", pca_CS);

  pca_DBR_TYPE = PyDict_New();
  PyDict_SetItemString(pca_DBR_TYPE, "DBR_STRING",
                       PyLong_FromLong(DBR_STRING));
  PyDict_SetItemString(pca_DBR_TYPE, "DBR_INT", PyLong_FromLong(DBR_INT));
  PyDict_SetItemString(pca_DBR_TYPE, "DBR_SHORT",
                       PyLong_FromLong(DBR_SHORT));
  PyDict_SetItemString(pca_DBR_TYPE, "DBR_FLOAT",
                       PyLong_FromLong(DBR_FLOAT));
  PyDict_SetItemString(pca_DBR_TYPE, "DBR_ENUM",
                       PyLong_FromLong(DBR_ENUM));
  PyDict_SetItemString(pca_DBR_TYPE, "DBR_CHAR",
                       PyLong_FromLong(DBR_CHAR));
  PyDict_SetItemString(pca_DBR_TYPE, "DBR_LONG",
                       PyLong_FromLong(DBR_LONG));
  PyDict_SetItemString(pca_DBR_TYPE, "DBR_DOUBLE",
                       PyLong_FromLong(DBR_DOUBLE));
  Py_INCREF(pca_DBR_TYPE);
  PyModule_AddObject(python_camodule, "DBR_TYPE", pca_DBR_TYPE);

  PyModule_AddIntConstant(python_camodule, "LOCK_FOREVER", -1);
  PyModule_AddIntConstant(python_camodule, "LOCK_NOWAIT", 0);
#endif

  /* define module generic error */
  python_ca_error = PyErr_NewException("ca.error", NULL, NULL);
  PyModule_AddObject(python_camodule, "error", python_ca_error);

  // create synchronization event
  pyCAevent = epicsEventMustCreate(epicsEventEmpty);

  /* check for errors */
  if (PyErr_Occurred())
    { Py_FatalError("can't initialize module ca"); }

  // when we are gone
  python_ca_destroyed = 0;

  // invoked when the interpreter is dead
  Py_AtExit(python_ca_dead);

  // register ca_context_destroy with atexit
  // this allows cleaning up gracefully, but watch for long timeout
#ifdef PYTHON_CA_ATEXIT
  PyObject * atexit_module = PyImport_ImportModule("atexit");
  PyObject * atexit_register =
    PyObject_GetAttrString(atexit_module, "register");
  PyObject * obj_python_ca_exit =
    PyObject_GetAttrString(python_camodule, "exit");
  PyObject * retval =
    PyObject_CallFunctionObjArgs(atexit_register, obj_python_ca_exit,
                                 NULL);

  if (retval == NULL)
    {
      if (PyErr_Occurred())
        {
          PyErr_Print();
          PyErr_Clear();
        }
    }

  Py_XDECREF(retval);
  Py_XDECREF(obj_python_ca_exit);
  Py_XDECREF(atexit_register);
  Py_XDECREF(atexit_module);
#endif

  // release the lock

  if (releaseLockWhenLeaving)
    {
#if defined(PYTHON_CA_USE_GIL_API)
      PyEval_ReleaseLock();
#endif
    }
}
