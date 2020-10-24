#include "ca_module.h"
#include "pv_get.h"

/*******************************************************************
 *  pv_get_callback
 *******************************************************************/

static void pv_get_callback(struct event_handler_args args)
{
  pvobject * self;

  // don't go there is we're quiting
  if (python_ca_destroyed)
    { return; }

  if (!args.chid)
    {
      Debug(0, "invalid args.chid\n");
      return;
    }

  self = (pvobject *) ca_puser(args.chid);

  if (!self->chanId)
    {
      printf("invalid chid in get cb!\n");
      return;
    }

  if (self->buff == NULL)
    {
      self->buff =
        (dbr_time_buff *) realloc(self->buff,
                                  dbr_size_n(dbf_type_to_DBR_TIME
                                             (xxx_ca_field_type
                                              (self->chanId)),
                                             ca_element_count
                                             (self->chanId)));
      Debug(10, "new self->buff 0x%p dbr_size_n %d dim=%ld\n",
            self->buff,
            dbr_size_n(dbf_type_to_DBR_TIME
                       (xxx_ca_field_type(self->chanId)),
                       ca_element_count(self->chanId)),
            ca_element_count(self->chanId));

      if (self->buff == NULL)
        {
          // FIXME: never return
          Debug(0, "failed to alloc buffer");
          return;
        }
    }

  PYTHON_CA_LOCK;
  /* update data */
  memcpy((char *)self->buff, args.dbr,
         dbr_size_n(dbf_type_to_DBR_TIME
                    (xxx_ca_field_type(self->chanId)),
                    ca_element_count(self->chanId)));

  if (self->cbGet)
    {
      Debug(3, "%s acquire the lock\n", self->name);

      Debug(3, "%s getCB\n", self->name);

      PyObject * cb = self->cbGet;

      /* our get callback might want to issue a new get, so clear ours now */
      self->cbGet = NULL;

      PyObject * funcargs = PyTuple_GetItem(cb, 0);
      PyObject * kwargs = PyTuple_GetItem(cb, 1);
      PyObject * func = PyTuple_GetItem(funcargs, 0);
      PyObject * retval = NULL;

      // FIXME -
      // one would think there is a better way

      PyObject * callargs = PyTuple_New(PyTuple_Size(funcargs));
      int j;
      Py_INCREF(self);
      PyTuple_SetItem(callargs, 0, (PyObject *) self);

      for (j = 1; j < PyTuple_Size(funcargs); j++)
        {
          PyObject * tmp = PyTuple_GetItem(funcargs, j);
          Py_INCREF(tmp);
          PyTuple_SetItem(callargs, j, tmp);
        }

      retval = PyObject_Call(func, callargs, kwargs);
      Py_XDECREF(callargs);

      if (retval == NULL)
        {
          Debug(0, "ERROR in getCb! %s\n", self->name);

          // FIXME: redundant?
          if (PyErr_Occurred())
            {
              PyErr_Print();
              PyErr_Clear();
            }
        }

      Py_XDECREF(retval);

      // free up the callback
      Py_XDECREF(funcargs);
      Py_XDECREF(kwargs);
      Py_XDECREF(cb);

      Debug(3, "%s released the lock\n", self->name);

    }

  PYTHON_CA_UNLOCK;
  Debug(10, "%s done\n", self->name);

}

/*******************************************************************
 * NAME
 *  pv_get - get value
 *
 * DESCRIPTION
 *  retreive IOC value
 *
 * RETURNS:
 *  sync mode: the actual value (tuple for waveform)
 *  syncT mode: PyNone
 *
 * WARNING
 *
 * PYTHON API:
 *    Python >>> pv_get()\b
 *
 * PERSONNEL:
 *    Matthieu Bec, ING Software Group -  mdcb@ing.iac.es\b
 *
 * HISTORY:
 *    30/06/99 - first documented version
 *
 *******************************************************************/
PyObject * pv_get(pvobject * self, /* self reference */
                  PyObject * args, /* optional attribute */
                  PyObject * kw  /* optional keywords attribute */
                 )
{
  int status;
  chtype type;
  PyObject * returnvalue;

  PyObject * func;
  PyObject * cb;

  dbr_plaintype data;
  int i;

  returnvalue = Py_None;

  if (!self->chanId)
    { PYCA_ASSERTSTATE(cs_closed); }

  int s = PyTuple_Size(args);

  if (s != 0)
    {

      if (self->cbGet)
        PYCA_ERR("get callback already present .. (FIXME)")
        // must be callable
        func = PyTuple_GetItem(args, 0);

      if (!PyCallable_Check(func))
        { PYCA_ERR("arg1 not callable") cb = PyTuple_New(2); }

      Py_XINCREF(args);
      PyTuple_SetItem(cb, 0, args);

      Py_XINCREF(kw);
      PyTuple_SetItem(cb, 1, kw);

      // FIXME - add a check if not NULL
      self->cbGet = cb;

    }

  int dim = ca_element_count(self->chanId);

  PYCA_ASSERTSTATE(ca_state(self->chanId));

  type = xxx_ca_field_type(self->chanId);

  if (!ca_read_access(self->chanId))
    { PYCA_ERR(ca_name(self->chanId)); }

  if (self->buff == NULL)
    {
      self->buff =
        (dbr_time_buff *) realloc(self->buff,
                                  dbr_size_n(dbf_type_to_DBR_TIME
                                             (xxx_ca_field_type
                                              (self->chanId)),
                                             ca_element_count
                                             (self->chanId)));
      Debug(10, "new self->buff 0x%p dbr_size_n %d\n", self->buff,
            dbr_size_n(dbf_type_to_DBR_TIME
                       (xxx_ca_field_type(self->chanId)),
                       ca_element_count(self->chanId)));

      if (!self->buff)
        { PYCA_ERR("no memory"); }
    }

  if (s > 0)
    {
      /* get callback */
      status =
        ca_array_get_callback(dbf_type_to_DBR_TIME
                              (xxx_ca_field_type(self->chanId)),
                              dim, self->chanId, pv_get_callback,
                              NULL);

      /* syncT flush and returns None */
      if ((status = ca_flush_io()) != ECA_NORMAL)
        { PYCA_ERR("channel access: %s", ca_message(status)); }

      Py_INCREF(Py_None);
      return (Py_None);

    }

  status =
    ca_array_get(dbf_type_to_DBR_TIME(xxx_ca_field_type(self->chanId)),
                 dim, self->chanId, self->buff);

  if (self->syncT >= 0)
    {
      if ((status = ca_pend_io(self->syncT)) != ECA_NORMAL)
        {
          PYCA_ERR("channel access: %s", ca_message(status));
        }

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
                                       (self->chanId),
                                       i, &data);
                PyTuple_SetItem(returnvalue, i,
                                PyString_FromString
                                (data.s));
              }

          else
            returnvalue =
              PyString_FromString(((struct dbr_time_string
                                    *)self->buff)->value);

          break;

        case DBR_DOUBLE:
          if (dim > 1)
            for (i = 0; i < dim; i++)
              {
                ca_module_utilsextract(self->buff,
                                       xxx_ca_field_type
                                       (self->chanId),
                                       i, &data);
                PyTuple_SetItem(returnvalue, i,
                                PyFloat_FromDouble
                                (data.d));
              }

          else
            returnvalue =
              PyFloat_FromDouble(((struct dbr_time_double
                                   *)self->buff)->value);

          break;

        case DBR_LONG:
          if (dim > 1)
            for (i = 0; i < dim; i++)
              {
                ca_module_utilsextract(self->buff,
                                       xxx_ca_field_type
                                       (self->chanId),
                                       i, &data);
                PyTuple_SetItem(returnvalue, i,
                                PyLong_FromLong
                                (data.l));
              }

          else
            returnvalue =
              PyLong_FromLong(((struct dbr_time_long *)
                               self->buff)->value);

          break;

        case DBR_SHORT:
          if (dim > 1)
            for (i = 0; i < dim; i++)
              {
                ca_module_utilsextract(self->buff,
                                       xxx_ca_field_type
                                       (self->chanId),
                                       i, &data);
                PyTuple_SetItem(returnvalue, i,
                                PyInt_FromLong(data.i));
              }

          else
            returnvalue =
              PyInt_FromLong(((struct dbr_time_short *)
                              self->buff)->value);

          break;

        case DBR_CHAR:
          if (dim > 1)
            for (i = 0; i < dim; i++)
              {
                ca_module_utilsextract(self->buff,
                                       xxx_ca_field_type
                                       (self->chanId),
                                       i, &data);
                PyTuple_SetItem(returnvalue, i,
                                PyInt_FromLong(data.c));
              }

          else
            returnvalue =
              PyInt_FromLong(((struct dbr_time_char *)
                              self->buff)->value);

          break;

        case DBR_ENUM:
          if (dim > 1)
            for (i = 0; i < dim; i++)
              {
                ca_module_utilsextract(self->buff,
                                       xxx_ca_field_type
                                       (self->chanId),
                                       i, &data);
                PyTuple_SetItem(returnvalue, i,
                                PyInt_FromLong(data.e));
              }

          else
            returnvalue =
              PyInt_FromLong(((struct dbr_time_enum *)
                              self->buff)->value);

          break;

        case DBR_FLOAT:
          if (dim > 1)
            for (i = 0; i < dim; i++)
              {
                ca_module_utilsextract(self->buff,
                                       xxx_ca_field_type
                                       (self->chanId),
                                       i, &data);
                PyTuple_SetItem(returnvalue, i,
                                PyFloat_FromDouble
                                (data.f));
              }

          else
            returnvalue =
              PyFloat_FromDouble(((struct dbr_time_float
                                   *)self->buff)->value);

          break;

        default:
          PYCA_ERR(ca_name(self->chanId));
        }

      // we already own a borrowed reference
      // this actually leaks memory
      //Py_INCREF(returnvalue);
      return returnvalue;
    }

  else
    {
      /* syncT flush and returns None */
      if ((status = ca_flush_io()) != ECA_NORMAL)
        { PYCA_ERR("channel access: %s", ca_message(status)); }

      Py_INCREF(Py_None);
      return (Py_None);
    }
}
