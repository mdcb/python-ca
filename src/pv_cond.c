#include "ca_module.h"
#include "pv_cond.h"

/*******************************************************************
 * NAME:
 *  pv_cond_wait
 *
 * DESCRIPTION:
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

PyObject * pv_cond_wait(pvobject * self, /* self reference */
                        PyObject * args  /* optional timeout */
                       )
{

  PyObject * status;
  float timeout = -1.0;
  epicsEventWaitStatus st;

  if (!self->chanId)
    { PYCA_ASSERTSTATE(cs_closed); }

  if (!PyArg_ParseTuple(args, "|f", &timeout))
    { PYCA_USAGE("cond_wait([timeout])"); }

  // create condition if first time
  if (!self->conid)
    {
      self->conid = epicsEventMustCreate(epicsEventEmpty);
    }

  /* FIXME? sound but wasn't needed until run on darwin-ppc */
  /* why would the linux-x86 not need that ??!! */
  if (timeout < 0)
    {
      // forever
      Py_BEGIN_ALLOW_THREADS st = epicsEventWait(self->conid);
      Py_END_ALLOW_THREADS
    }

  else
    {
      Py_BEGIN_ALLOW_THREADS
      st = epicsEventWaitWithTimeout(self->conid, timeout);
      Py_END_ALLOW_THREADS
    }

  status = PyInt_FromLong(st);

  Py_INCREF(status);
  return status;
}

/*******************************************************************
 * NAME:
 *  pv_cond_signal
 *
 * DESCRIPTION:
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

PyObject * pv_cond_signal(pvobject * self, /* self reference */
                          PyObject * args  /* empty tuple */
                         )
{

  if (!self->chanId)
    { PYCA_ASSERTSTATE(cs_closed); }

  if (!self->conid)
    {
      /*
       * FIXME, one way or another
       *
       * PYCA_ERR( "cond_signal: condition does not exists.");
       *
       * alternatively, create it now. this is a design decision
       * for now, just print a reminder
       */
      Debug(1, "Cannot signal, no listener!\n");
      Py_INCREF(Py_None);
      return Py_None;
    }

  epicsEventSignal(self->conid);

  Py_INCREF(Py_None);
  return Py_None;
}
