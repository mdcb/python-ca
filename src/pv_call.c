#include "ca_module.h"
#include "pv_call.h"
#include "pv_put.h"
#include "pv_get.h"

/*******************************************************************
 * NAME
 *  pv_call - get/put value
 *
 * DESCRIPTION
 *   shortcut to get/put
 *
 * RETURNS:
 *
 * WARNING
 *
 * PYTHON API:
 *    Python >>> pv()\b
 *    Python >>> pv('hello')\b
 *
 * PERSONNEL:
 *    Matthieu Bec, Gemini Observatory -  mbec@gemini.edu\b
 *
 * HISTORY:
 *    29/03/06 - first documented version
 *
 *******************************************************************/
PyObject * pv_call(pvobject * self, /* self reference */
                   PyObject * args,  /* empty or not tuple */
                   PyObject * kw /* should be empty, ignored anyways */
                  )
{
  PyObject * argstuple;

  if (!self->chanId)
    { PYCA_ASSERTSTATE(cs_closed); }

  if (PyArg_ParseTuple(args, "O", &argstuple))
    {
      /* it's a put */
      return pv_put(self, args);
    }

  else if (PyArg_ParseTuple(args, ""))
    {
      PyErr_Clear();
      /* it's a get */
      return PyObject_CallMethod((PyObject *) self, "get", "");;
    }

  /* should never happen */
  return NULL;
}
