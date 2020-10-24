#include "ca_module.h"
#include "pv_gettersetter.h"

static PyObject * pv_getter_pvname(pvobject * self, void * closure);
static PyObject * pv_getter_pvstate(pvobject * self, void * closure);
static PyObject * pv_getter_pvdim(pvobject * self, void * closure);
static PyObject * pv_getter_pvtype(pvobject * self, void * closure);
static PyObject * pv_getter_pvaccess(pvobject * self, void * closure);
static PyObject * pv_getter_pvhostname(pvobject * self, void * closure);
static PyObject * pv_getter_pvtimestr(pvobject * self, void * closure);
static PyObject * pv_getter_pvtime(pvobject * self, void * closure);

/*******************************************************************
 * pv_getter_pvname
 *******************************************************************/

PyObject * pv_getter_pvname(pvobject * self, void * closure)
{
  if (self->chanId)
    { return PyUnicode_FromString(ca_name(self->chanId)); }

  else
    {
      PYCA_ERR("pvname: invalid chid");
    }
}

/*******************************************************************
 * pv_getter_pvstate
 *******************************************************************/

static char * channel_state_str[] =
{
  "cs_never_conn",
  "cs_prev_conn",
  "cs_conn",
  "cs_closed"
};

PyObject * pv_getter_pvstate(pvobject * self, void * closure)
{
  if (self->chanId)
    return
      PyUnicode_FromString(channel_state_str
                         [ca_state(self->chanId)]);

  else
    {
      PYCA_ERR("pvstate: invalid chid");
    }
}

/*******************************************************************
 * pv_getter_pvdim
 *******************************************************************/

PyObject * pv_getter_pvdim(pvobject * self, void * closure)
{
  if (self->chanId)
    { return PyLong_FromLong(ca_element_count(self->chanId)); }

  else
    {
      PYCA_ERR("pvdim: invalid chid");
    }
}

/*******************************************************************
 * pv_getter_pvtype
 *******************************************************************/

PyObject * pv_getter_pvtype(pvobject * self, void * closure)
{
  if (self->chanId && dbr_type_is_plain(xxx_ca_field_type(self->chanId)))
    return
      PyUnicode_FromString(dbr_text
                         [xxx_ca_field_type(self->chanId)]);

  else
    {
      PYCA_ERR("pvtype: invalid");
    }
}

/*******************************************************************
 * pv_getter_pvaccess
 *******************************************************************/

PyObject * pv_getter_pvaccess(pvobject * self, void * closure)
{
  if (self->chanId)
    return PyUnicode_FromFormat("%s", ca_read_access(self->chanId)
                              && ca_write_access(self->chanId) ?
                              "rw" : ca_read_access(self->chanId) ?
                              "ro" : ca_write_access(self->chanId)
                              ? "wo" : "None");

  else
    {
      PYCA_ERR("invalid chid");
    }
}

/*******************************************************************
 * pv_getter_pvhostname
 *******************************************************************/

PyObject * pv_getter_pvhostname(pvobject * self, void * closure)
{
  if (self->chanId)
    { return PyUnicode_FromString(ca_host_name(self->chanId)); }

  else
    {
      PYCA_ERR("pvhostname: invalid chid");
    }
}

/*******************************************************************
 * pv_getter_pvtimestr
 *******************************************************************/

PyObject * pv_getter_pvtimestr(pvobject * self, void * closure)
{
  char timeText[80];

  if (self->chanId && self->buff != NULL)
    {
      epicsTimeToStrftime(timeText, sizeof(timeText), "%Y-%m-%d %H:%M:%S", &self->buff->stamp);
      return PyUnicode_FromString(timeText);
    }

  else
    {
      PYCA_ERR("pvtimestr: indef");
    }
}

/*******************************************************************
 * pv_getter_pvtime
 *******************************************************************/

PyObject * pv_getter_pvtime(pvobject * self, void * closure)
{
  struct timespec posixtime;

  if (self->chanId && self->buff != NULL)
    {
      epicsTimeToTimespec(&posixtime, &self->buff->stamp);
      return PyFloat_FromDouble((double)posixtime.tv_sec +
                                (double)posixtime.tv_nsec * 1e-9);
    }

  else
    {
      PYCA_ERR("pvtime: indef");
    }
}

/*******************************************************************
 * pv_getter_pvval
 *******************************************************************/

PyObject * pv_getter_pvval(pvobject * self, void * closure)
{

  if (self->chanId == NULL || ca_state(self->chanId) != cs_conn
      || self->buff == NULL)
    {
      PYCA_ERR("pvval: indef");
    }

  int i;
  dbr_plaintype data;
  int dim = ca_element_count(self->chanId);
  chtype type = xxx_ca_field_type(self->chanId);
  PyObject * returnvalue = NULL;

  /* build the returned tuple */
  if (dim > 1)
    { returnvalue = PyTuple_New(dim); }

  switch (type)
    {
    case DBR_STRING:
      if (dim > 1)
        for (i = 0; i < dim; i++)
          {
            ca_module_utilsextract(self->buff,
                                   xxx_ca_field_type
                                   (self->chanId), i,
                                   &data);
            PyTuple_SetItem(returnvalue, i,
                            PyUnicode_FromString(data.s));
          }

      else
        returnvalue =
          PyUnicode_FromString(((struct dbr_time_string *)
                              self->buff)->value);

      break;

    case DBR_DOUBLE:
      if (dim > 1)
        for (i = 0; i < dim; i++)
          {
            ca_module_utilsextract(self->buff,
                                   xxx_ca_field_type
                                   (self->chanId), i,
                                   &data);
            PyTuple_SetItem(returnvalue, i,
                            PyFloat_FromDouble(data.d));
          }

      else
        returnvalue =
          PyFloat_FromDouble(((struct dbr_time_double *)
                              self->buff)->value);

      break;

    case DBR_LONG:
      if (dim > 1)
        for (i = 0; i < dim; i++)
          {
            ca_module_utilsextract(self->buff,
                                   xxx_ca_field_type
                                   (self->chanId), i,
                                   &data);
            PyTuple_SetItem(returnvalue, i,
                            PyLong_FromLong(data.l));
          }

      else
        returnvalue = PyLong_FromLong(((struct dbr_time_long *)
                                       self->buff)->value);

      break;

    case DBR_SHORT:
      if (dim > 1)
        for (i = 0; i < dim; i++)
          {
            ca_module_utilsextract(self->buff,
                                   xxx_ca_field_type
                                   (self->chanId), i,
                                   &data);
            PyTuple_SetItem(returnvalue, i,
                            PyLong_FromLong(data.i));
          }

      else
        returnvalue = PyLong_FromLong(((struct dbr_time_short *)
                                       self->buff)->value);

      break;

    case DBR_CHAR:
      if (dim > 1)
        for (i = 0; i < dim; i++)
          {
            ca_module_utilsextract(self->buff,
                                   xxx_ca_field_type
                                   (self->chanId), i,
                                   &data);
            PyTuple_SetItem(returnvalue, i,
                            PyLong_FromLong(data.c));
          }

      else
        returnvalue = PyLong_FromLong(((struct dbr_time_char *)
                                       self->buff)->value);

      break;

    case DBR_ENUM:
      if (dim > 1)
        for (i = 0; i < dim; i++)
          {
            ca_module_utilsextract(self->buff,
                                   xxx_ca_field_type
                                   (self->chanId), i,
                                   &data);
            PyTuple_SetItem(returnvalue, i,
                            PyLong_FromLong(data.e));
          }

      else
        returnvalue = PyLong_FromLong(((struct dbr_time_enum *)
                                       self->buff)->value);

      break;

    case DBR_FLOAT:
      if (dim > 1)
        for (i = 0; i < dim; i++)
          {
            ca_module_utilsextract(self->buff,
                                   xxx_ca_field_type
                                   (self->chanId), i,
                                   &data);
            PyTuple_SetItem(returnvalue, i,
                            PyFloat_FromDouble(data.f));
          }

      else
        returnvalue =
          PyFloat_FromDouble(((struct dbr_time_float *)
                              self->buff)->value);

      break;

    default:
      PYCA_ERR("pvval: invalid");
      break;
    }

  return returnvalue;
}

/*******************************************************************
 * pv_getter_pvtsval
 *******************************************************************/

PyObject * pv_getter_pvtsval(pvobject * self, void * closure)
{
  PyObject * timeobj = pv_getter_pvtime(self, closure);
  PyObject * valobj = pv_getter_pvval(self, closure);

  if (timeobj && valobj)
    {
      // return a time/val tuple
      PyObject * retval = PyTuple_New(2);
      PyTuple_SetItem(retval, 0, timeobj);
      PyTuple_SetItem(retval, 1, valobj);
      return retval;
    }

  else
    {
      Py_XDECREF(timeobj);
      Py_XDECREF(valobj);
      PYCA_ERR("pvtsval: invalid");
    }
}

/*******************************************************************
 * pv_getter_pvstats
 *******************************************************************/

PyObject * pv_getter_pvstats(pvobject * self, void * closure)
{

  PyObject * tmp;
  PyObject * stats;

  stats = PyDict_New();

  tmp = pv_getter_pvname(self, NULL);

  if (!tmp)
    {
      Py_INCREF(Py_None);
      tmp = Py_None;
    }

  PyDict_SetItemString(stats, "name", tmp);
  Py_DECREF(tmp);

  tmp = pv_getter_pvstate(self, NULL);

  if (!tmp)
    {
      Py_INCREF(Py_None);
      tmp = Py_None;
    }

  PyDict_SetItemString(stats, "state", tmp);
  Py_DECREF(tmp);

  tmp = pv_getter_pvdim(self, NULL);

  if (!tmp)
    {
      Py_INCREF(Py_None);
      tmp = Py_None;
    }

  PyDict_SetItemString(stats, "dim", tmp);
  Py_DECREF(tmp);

  tmp = pv_getter_pvtype(self, NULL);

  if (!tmp)
    {
      Py_INCREF(Py_None);
      tmp = Py_None;
    }

  PyDict_SetItemString(stats, "type", tmp);
  Py_DECREF(tmp);

  tmp = pv_getter_pvaccess(self, NULL);

  if (!tmp)
    {
      Py_INCREF(Py_None);
      tmp = Py_None;
    }

  PyDict_SetItemString(stats, "access", tmp);
  Py_DECREF(tmp);

  tmp = pv_getter_pvhostname(self, NULL);

  if (!tmp)
    {
      Py_INCREF(Py_None);
      tmp = Py_None;
    }

  PyDict_SetItemString(stats, "hostname", tmp);
  Py_DECREF(tmp);

  tmp = pv_getter_pvtimestr(self, NULL);

  if (!tmp)
    {
      Py_INCREF(Py_None);
      tmp = Py_None;
    }

  PyDict_SetItemString(stats, "ts", tmp);
  Py_DECREF(tmp);

  tmp = pv_getter_pvtime(self, NULL);

  if (!tmp)
    {
      Py_INCREF(Py_None);
      tmp = Py_None;
    }

  PyDict_SetItemString(stats, "time", tmp);
  Py_DECREF(tmp);

  tmp = pv_getter_pvval(self, NULL);

  /* for stats, ignore val errors */
  if (!tmp)
    {
      Py_INCREF(Py_None);
      tmp = Py_None;
      PYCA_ERRNONE()
    }

  PyDict_SetItemString(stats, "val", tmp);
  Py_DECREF(tmp);

//#ifdef PYTHON_CA_DEVCORE
  tmp = PyLong_FromLong(((PyObject *) self)->ob_refcnt);

  if (!tmp)
    {
      Py_INCREF(Py_None);
      tmp = Py_None;
    }

  PyDict_SetItemString(stats, "ob_refcnt", tmp);
  Py_DECREF(tmp);
//#endif

  // catch exceptions
  if (PyErr_Occurred())
    {
      PyErr_Clear();
    }

  return stats;
}

PyObject * pv_dict_subscript(pvobject * self, register PyObject * key)
{
  char * skey;
  PyObject * tmp = NULL;

  if (!PyUnicode_CheckExact(key))
    {
      PYCA_ERR("key invalid (not str)");
    }

  skey = PyUnicode_AsUTF8(key);

  if (!strcmp(skey, "name"))
    {
      tmp = pv_getter_pvname(self, NULL);
    }

  else if (!strcmp(skey, "state"))
    {
      tmp = pv_getter_pvstate(self, NULL);
    }

  else if (!strcmp(skey, "dim"))
    {
      tmp = pv_getter_pvdim(self, NULL);
    }

  else if (!strcmp(skey, "type"))
    {
      tmp = pv_getter_pvtype(self, NULL);
    }

  else if (!strcmp(skey, "access"))
    {
      tmp = pv_getter_pvaccess(self, NULL);
    }

  else if (!strcmp(skey, "hostname"))
    {
      tmp = pv_getter_pvhostname(self, NULL);
    }

  else if (!strcmp(skey, "ts"))
    {
      tmp = pv_getter_pvtimestr(self, NULL);
    }

  else if (!strcmp(skey, "time"))
    {
      tmp = pv_getter_pvtime(self, NULL);
    }

  else if (!strcmp(skey, "val"))
    {
      tmp = pv_getter_pvval(self, NULL);
    }

  else
    {
      PYCA_ERR("key invalid: %s", skey);
    }

  return tmp;

}
