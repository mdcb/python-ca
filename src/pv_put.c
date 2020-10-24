#include "ca_module.h"
#include "pv_put.h"
#include "stddef.h"

/*******************************************************************
 * NAME
 *  pv_put - set PV value
 *
 * DESCRIPTION
 *   set PV value
 *
 * RETURNS:
 *
 * WARNING
 *    string will not work for enum type
 *
 * PYTHON API:
 *    Python >>> x.put(val)\b
 *    Python >>> x.put(str)\b
 *    Python >>> x.put( (val1, val2, val3) )\b
 *    Python >>> x.put( ('str1', 'str2', 'str3') )\b
 *
 * PERSONNEL:
 *    Matthieu Bec, ING Software Group -  mdcb@ing.iac.es\b
 *
 * HISTORY:
 *    30/06/99 - first documented version
 *    24/06/04 - check/conversion on put(val)
 *               when val does not match the dbf_type
 *
 *******************************************************************/
PyObject * pv_put(pvobject * self, /* self reference */
                  PyObject * args  /* value to put (as a tuple)  */
                 )
{
  int status;
  chtype type;
  int pvwriteaccess;

  PyObject * argstuple;
  PyObject * argstupleelt;
  PyObject * tmp;
  dbr_plaintype pval;
  int i;

  if (!self->chanId)
    { PYCA_ASSERTSTATE(cs_closed); }

  PYCA_ASSERTSTATE(ca_state(self->chanId));

  type = xxx_ca_field_type(self->chanId);
  pvwriteaccess = ca_write_access(self->chanId);

  if (!pvwriteaccess)
    { PYCA_ERR("put: no write access"); }

  /* if the size has changed - realloc */

  if (self->buff == NULL)
    {
      self->buff =
        (dbr_time_buff *) realloc(self->buff,
                                  dbr_size_n(dbf_type_to_DBR_TIME
                                             (type),
                                             ca_element_count
                                             (self->chanId)));
      Debug(10, "new self->buff 0x%p dim %ld dbr_size_n %d\n",
            self->buff, ca_element_count(self->chanId),
            dbr_size_n(dbf_type_to_DBR_TIME(type),
                       ca_element_count(self->chanId)));

      if (!self->buff)
        { PYCA_ERR("no memory"); }
    }

  if (ca_element_count(self->chanId) > 1)
    {
      // wave form
      if (!PyArg_ParseTuple(args, "O", &argstuple))
        { PYCA_USAGE("put(val,[...]) - illegal argument"); }

      if (!PyTuple_Check(argstuple))
        { PYCA_USAGE("put(val,[...]) - array needs tuple"); }
    }

  else
    {
      // scalar
      argstuple = args;
    }

  if (PyTuple_Size(argstuple) != (Py_ssize_t) ca_element_count(self->chanId))
    {
      Debug(0,
            "PyTuple_Size(argstuple) = %ld vs ca_element_count = %ld\n",
            PyTuple_Size(argstuple), ca_element_count(self->chanId));

      for (i = 0; i < PyTuple_Size(argstuple); i++)
        {
          argstupleelt = PyTuple_GetItem(argstuple, i);
          Debug(0, " elt %d = %s\n", i,
                PyUnicode_AsUTF8(tmp =
                                    PyObject_Str(argstupleelt)));
        }

      PYCA_USAGE("put(val[,...]) - size mismatch");
    }

  switch (type)
    {
    case DBR_STRING:
      for (i = 0; i < PyTuple_Size(argstuple); i++)
        {
          argstupleelt = PyTuple_GetItem(argstuple, i);
          strcpy(pval.s,
                 PyUnicode_AsUTF8(tmp =
                                     PyObject_Str(argstupleelt)));
          Py_DECREF(tmp);
          ca_module_utilsinject(self->buff, type, i, &pval);
        }

      break;

    case DBR_DOUBLE:
      for (i = 0; i < PyTuple_Size(argstuple); i++)
        {
          argstupleelt = PyTuple_GetItem(argstuple, i);

          if ((tmp = PyNumber_Float(argstupleelt)) != NULL)
            {
              pval.d = PyFloat_AsDouble(tmp);
              Py_DECREF(tmp);
              ca_module_utilsinject(self->buff, type, i,
                                    &pval);
            }

          else
            {
              PYCA_USAGE
              ("put(val[,...]) - type mismatch (double)");
            }
        }

      break;

    case DBR_FLOAT:
      for (i = 0; i < PyTuple_Size(argstuple); i++)
        {
          argstupleelt = PyTuple_GetItem(argstuple, i);

          if ((tmp = PyNumber_Float(argstupleelt)) != NULL)
            {
              pval.f = PyFloat_AsDouble(tmp);
              Py_DECREF(tmp);
              ca_module_utilsinject(self->buff, type, i,
                                    &pval);
            }

          else
            {
              PYCA_USAGE
              ("put(val[,...]) - type mismatch (float)");
            }
        }

      break;

    case DBR_LONG:
      for (i = 0; i < PyTuple_Size(argstuple); i++)
        {
          argstupleelt = PyTuple_GetItem(argstuple, i);

          if ((tmp = PyNumber_Long(argstupleelt)) != NULL)
            {
              pval.l = PyLong_AsLong(tmp);
              Py_DECREF(tmp);
              ca_module_utilsinject(self->buff, type, i,
                                    &pval);
            }

          else
            {
              PYCA_USAGE
              ("put(val[,...]) - type mismatch (long)");
            }
        }

      break;

    case DBR_SHORT:
      for (i = 0; i < PyTuple_Size(argstuple); i++)
        {
          argstupleelt = PyTuple_GetItem(argstuple, i);

          if ((tmp = PyNumber_Long(argstupleelt)) != NULL)
            {
              pval.i = PyLong_AsLong(tmp);
              Py_DECREF(tmp);
              ca_module_utilsinject(self->buff, type, i,
                                    &pval);
            }

          else
            {
              PYCA_USAGE
              ("put(val[,...]) - type mismatch (short)");
            }
        }

      break;

    case DBR_CHAR:
      for (i = 0; i < PyTuple_Size(argstuple); i++)
        {
          argstupleelt = PyTuple_GetItem(argstuple, i);

          if ((tmp = PyNumber_Long(argstupleelt)) != NULL)
            {
              pval.c = PyLong_AsLong(tmp);
              Py_DECREF(tmp);
              ca_module_utilsinject(self->buff, type, i,
                                    &pval);
            }

          else
            {
              PYCA_USAGE
              ("put(val[,...]) - type mismatch (char)");
            }
        }

      break;

    case DBR_ENUM:
      for (i = 0; i < PyTuple_Size(argstuple); i++)
        {
          argstupleelt = PyTuple_GetItem(argstuple, i);

          if ((tmp = PyNumber_Long(argstupleelt)) != NULL)
            {
              pval.e = PyLong_AsLong(tmp);
              Py_DECREF(tmp);
              ca_module_utilsinject(self->buff, type, i,
                                    &pval);
            }

          else
            {
              PYCA_USAGE
              ("put(val[,...]) - type mismatch (enum)");
            }
        }

      break;

    default:
      PYCA_ERR(ca_name(self->chanId));
    }

  status =
    ca_array_put(type, ca_element_count(self->chanId), self->chanId,
                 dbr_value_ptr(self->buff, dbf_type_to_DBR_TIME(type)));

  if (status != ECA_NORMAL)
    { PYCA_ERR("channel access: %s", ca_message(status)); }

  if (self->syncT >= 0)
    {
      Debug(5, "put -> ca_pend_io %f\n", self->syncT);

      if ((status = ca_pend_io(self->syncT)) != ECA_NORMAL)
        { PYCA_ERR("channel access: %s", ca_message(status)); }
    }

  else
    {
      if ((status = ca_flush_io()) != ECA_NORMAL)
        { PYCA_ERR("channel access: %s", ca_message(status)); }
    }

  Py_INCREF(Py_None);
  return (Py_None);
}
