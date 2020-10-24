#include "ca_module.h"
#include "pv_enum.h"

/*******************************************************************
 * NAME
 *  pv_enum - get enum menuvalue a string
 *
 * DESCRIPTION
 *  retreive IOC enum menuvalue a string
 *
 * RETURNS:
 *  dictionary of menuvalue/string
 *
 * WARNING
 *
 * PYTHON API:
 *    Python >>> pv.enum()\b
 *
 * PERSONNEL:
 *    Matthieu Bec, Gemini -  mbec@gemini.edu\b
 *
 * HISTORY:
 *
 *******************************************************************/
PyObject * pv_enum(pvobject * self, /* self reference */
                   PyObject * args /* empty tuple */
                  )
{
  int status;
  PyObject * returnvalue;
  PyObject * keyval;

  struct dbr_gr_enum bufGrEnum;

  int i;

  returnvalue = Py_None;

  if (!self->chanId)
    { PYCA_ASSERTSTATE(cs_closed); }

  if (!PyArg_ParseTuple(args, ""))
    { PYCA_USAGE("get()"); }

  PYCA_ASSERTSTATE(ca_state(self->chanId));

//      if (xxx_ca_field_type (self->chanId) != DBR_ENUM)
//              PYCA_ERR( "pv not an enum");

  if (ca_element_count(self->chanId) != 1)
    { PYCA_ERR("pv dimension not 1"); }

  if (!ca_read_access(self->chanId))
    { PYCA_ERR(ca_name(self->chanId)); }

  status = ca_array_get(DBR_GR_ENUM, 1, self->chanId, &bufGrEnum);

  if (self->syncT >= 0)
    {
      if ((status = ca_pend_io(self->syncT)) != ECA_NORMAL)
        { PYCA_ERR("channel access: %s", ca_message(status)); }
    }

  else
    {
      if ((status = ca_pend_io(ca_module_admin_syncT)) != ECA_NORMAL)
        { PYCA_ERR("channel access: %s", ca_message(status)); }
    }

  returnvalue = PyDict_New();

  for (i = 0; i < bufGrEnum.no_str; i++)
    {
      keyval = PyUnicode_FromString(bufGrEnum.strs[i]);
      PyDict_SetItem(returnvalue, PyLong_FromLong(i), keyval);
      Py_DECREF(keyval);
    }

  return (returnvalue);

}
