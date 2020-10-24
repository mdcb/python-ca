#include "ca_module.h"
#include "pv_str.h"

#include "pv_gettersetter.h"

/*******************************************************************
 * NAME
 *  pv_str - python internal
 *
 * DESCRIPTION
 *   python internal
 *
 * RETURNS:
 *
 * WARNING
 *
 * PERSONNEL:
 *    Matthieu Bec, ING Software Group -  mdcb@ing.iac.es\b
 *
 * HISTORY:
 *    30/06/99 - first documented version
 *
 *******************************************************************/
PyObject * pv_str(pvobject * self)
{
  PyObject * pvstats;
  PyObject * retval;

  pvstats = pv_getter_pvstats(self, NULL);

  retval = PyObject_Str(pvstats);

  Py_DECREF(pvstats);

  return retval;

}
