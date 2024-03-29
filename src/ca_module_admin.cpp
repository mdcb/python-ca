#include "Python.h"
#include "structmember.h"

#include <string>
#include <map>

#include "ca_module_admin.h"
#include "ca_module.h"
#include "pv_gettersetter.h"

std::map <std::string, PyObject *> allyourbases;

PyObject * ca_module_admin_singleton = NULL;

int ca_module_admin_debug_level = PYTHON_CA_DEBUGLEVEL;
int ca_module_admin_enum_as_string = PYTHON_CA_ENUM_AS_STRING;
float ca_module_admin_syncT = PYTHON_CA_SYNCT;

typedef struct
{
  PyObject_HEAD
} contextobject;

static PyObject * ca_module_admin_debug_getter(contextobject * self, void * closure)
{
  return PyLong_FromLong(ca_module_admin_debug_level);
}

static PyObject * ca_module_admin_enum_as_string_getter(contextobject * self, void * closure)
{
  return PyLong_FromLong(ca_module_admin_enum_as_string);
}

static PyObject * ca_module_admin_syncT_getter(contextobject * self, void * closure)
{
  return PyFloat_FromDouble(ca_module_admin_syncT);
}

static PyObject * ca_module_admin_channels_getter(contextobject * self, void * closure)
{
  // returns a dictionary of all the pv_stats
  // do not reference the pv here or they'll never destroy
  PyObject * retval = PyDict_New();
  std::map < std::string, PyObject * >::iterator it;

  for (it = allyourbases.begin(); it != allyourbases.end(); it++)
    {
      PyObject * tmp =
        pv_getter_pvstats((pvobject *) it->second, NULL);
      PyDict_SetItemString(retval, it->first.c_str(), tmp);
      Py_DECREF(tmp);
    }

  return retval;
}

static int ca_module_admin_debug_setter(contextobject * self, PyObject * value, void * closure)
{
  PyObject * tmp;

  if (value == NULL)
    {
      PYCA_ERRMSG("Cannot delete debug attribute");
      return -1;
    }

  if ((tmp = PyNumber_Long(value)) != NULL)
    {
      ca_module_admin_debug_level = PyLong_AsLong(tmp);
      Py_DECREF(tmp);
    }

  else
    {
      PYCA_ERRMSG("debug must be integer");
      return -1;
    }

  return 0;
}

static int ca_module_admin_enum_as_string_setter(contextobject * self, PyObject * value, void * closure)
{
  PyObject * tmp;

  if (value == NULL)
    {
      PYCA_ERRMSG("Cannot delete enum_as_string attribute");
      return -1;
    }

  if ((tmp = PyNumber_Long(value)) != NULL)
    {
      ca_module_admin_enum_as_string = PyLong_AsLong(tmp);
      Py_DECREF(tmp);
    }

  else
    {
      PYCA_ERRMSG("enum_as_string must be integer");
      return -1;
    }

  return 0;
}

static int ca_module_admin_syncT_setter(contextobject * self, PyObject * value, void * closure)
{
  PyObject * tmp;

  if (value == NULL)
    {
      PYCA_ERRMSG("Cannot delete syncT attribute");
      return -1;
    }

  if ((tmp = PyNumber_Float(value)) != NULL)
    {
      ca_module_admin_syncT = PyFloat_AsDouble(tmp);
      Py_DECREF(tmp);
    }

  else
    {
      PYCA_ERRMSG("syncT must be float");
      return -1;
    }

  return 0;
}

static PyGetSetDef ca_module_admin_getseters[] =
{
  {
    (char *)"debug", (getter) ca_module_admin_debug_getter,
    (setter) ca_module_admin_debug_setter, (char *)"get/set debug level",
    NULL
  },
  {
    (char *)"enum_as_string",
    (getter) ca_module_admin_enum_as_string_getter,
    (setter) ca_module_admin_enum_as_string_setter,
    (char *)"get/set enum_as_string", NULL
  },
  {
    (char *)"syncT", (getter) ca_module_admin_syncT_getter,
    (setter) ca_module_admin_syncT_setter, (char *)"get/set sync time",
    NULL
  },
  {(char *)"channels", (getter) ca_module_admin_channels_getter, 0, (char *)"get list of pv", NULL},  /* read only */
  {NULL}      /* sentinel */
};

PyObject * ca_module_admin_repr(contextobject * self)
{
  return PyUnicode_FromFormat("<ca.admin object at 0x%p>", self);
}

PyObject * ca_module_admin_str(contextobject * self)
{
  return PyUnicode_FromFormat("<ca.admin xxx>");
}

PyDoc_STRVAR(ca_module_admin_doc, "blah, blah.\n\
blah.");

static struct PyMethodDef ca_module_admin_methods[] =
{
  {NULL}      /* sentinel */
};

static PyMemberDef ca_module_admin_members[] =
{
  {NULL}      /* sentinel */
};

PyObject * ca_module_admin_new(PyTypeObject * type, PyObject * args, PyObject * kwds)
{
  if (ca_module_admin_singleton != NULL)
    {
      PYCA_ERRMSG("singleton cannot be instantiated.");
      return NULL;
    }

  ca_module_admin_singleton = type->tp_alloc(type, 0);
  return ca_module_admin_singleton;
}

PyTypeObject ca_module_adminType =
{
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "ca.admin",
  .tp_basicsize = sizeof(contextobject),
  .tp_repr = (reprfunc) ca_module_admin_repr,
  .tp_str = (reprfunc) ca_module_admin_str,
  .tp_getattro = PyObject_GenericGetAttr,
  .tp_setattro = PyObject_GenericSetAttr,
  .tp_doc = ca_module_admin_doc,
  .tp_methods = ca_module_admin_methods,
  .tp_members = ca_module_admin_members,
  .tp_getset = ca_module_admin_getseters,
  .tp_alloc = PyType_GenericAlloc,
  .tp_new = ca_module_admin_new,
};

int ca_module_admin_singleton_create(void)
{
  if (PyType_Ready(&ca_module_adminType) < 0)
    { return -1; }

  ca_module_admin_singleton =
    ca_module_admin_new(&ca_module_adminType, NULL, NULL);

  if (ca_module_admin_singleton == NULL)
    { return -1; }

  return 0;
}

void ca_module_admin_singleton_register(PyObject * m)
{
  PyModule_AddObject(m, "admin", ca_module_admin_singleton);
}

void allyourbases_erase(char * name)
{
  std::map < std::string, PyObject * >::iterator deadbeef;
  deadbeef = allyourbases.find(name);

  if (deadbeef != allyourbases.end())
    {
      allyourbases.erase(deadbeef);
    }

  else
    {
      // born dead may happen synchronously
    }
}

void allyourbases_insert(char * name, PyObject * obj)
{
  allyourbases.insert(std::pair < std::string, PyObject * >(name, obj));
}

PyObject * allyourbases_find(char * name)
{
  std::map < std::string, PyObject * >::iterator lookup;
  lookup = allyourbases.find(name);

  if (lookup != allyourbases.end())
    {
      return (PyObject *) lookup->second;
    }

  else
    {
      return NULL;
    }
}
