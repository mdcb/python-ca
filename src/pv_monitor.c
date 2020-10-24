#include "ca_module.h"
#include "pv_monitor.h"
#include "pv_addCb.h"

/*******************************************************************
 *  raise_one
 *******************************************************************/
void raise_one(pvobject * self, /* self reference */
               PyObject * cb)
{

  // don't go there is we're quiting
  if (python_ca_destroyed)
    { return; }

  Debug(3, "raise_one %s\n", self->name);

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
      Debug(0, "ERROR in raiseCb! %s\n", self->name);

      // FIXME: redundant?
      if (PyErr_Occurred())
        {
          PyErr_Print();
          PyErr_Clear();
        }
    }

  Py_XDECREF(retval);
}

/*******************************************************************
 *  raise_all
 *******************************************************************/
void raise_all(pvobject * self  /* self reference */
              )
{

  if (!self->chanId)
    {
      printf("invalid chid in raise_all!\n");
      return;
    }

  if (self->cbList)
    {
      int i;
      Debug(3, "%s acquire the lock\n", self->name);

      // don't go there is we're quiting
      if (python_ca_destroyed)
        { return; }

      PYTHON_CA_LOCK;
      Debug(3, "%s raiseCb\n", self->name);

      for (i = 0; i < PyList_Size(self->cbList); i++)
        {
          PyObject * cb = PyList_GetItem(self->cbList, i);
          raise_one(self, cb);
        }

      PYTHON_CA_UNLOCK;
      Debug(3, "%s released the lock\n", self->name);
    }

}

/*******************************************************************
 * pv_update
 *******************************************************************/
void pv_update(struct event_handler_args args)
{
  pvobject * self;
  PyObject * retval;

// is the main thread trying to exit?

//    if (!Py_IsInitialized()) {
//       printf ("interpreter died in pv_update!\n");
//       // dead end, the interpreter is gone
//       return;
//    }

  if (!args.chid)
    {
      Debug(0, "invalid args.chid\n");
      return;
    }

  self = (pvobject *) ca_puser(args.chid);

  Debug(10, "%s\n", self->name);

//      Debug(10,"monitor update %d bytes\n",dbr_size_n(dbf_type_to_DBR_TIME(xxx_ca_field_type (self->chanId)),ca_element_count(self->chanId)));
//      Debug(10,"  ca_element_count  %ld\n",ca_element_count(self->chanId));
//      Debug(10,"  self->buff 0x%p\n",self->buff);
//      Debug(10,"  args.dbr   0x%p\n",args.dbr);
//      Debug(10,"  dbr_size_n %d\n",dbr_size_n(dbf_type_to_DBR_TIME(xxx_ca_field_type (self->chanId)),ca_element_count(self->chanId)));

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

  /* update data */
  memcpy((char *)self->buff, args.dbr,
         dbr_size_n(dbf_type_to_DBR_TIME(xxx_ca_field_type(self->chanId)),
                    ca_element_count(self->chanId)));

  if (self->cbH)
    {
      // don't go there is we're quiting
      if (python_ca_destroyed)
        { return; }

      Debug(2, "instance method eventhandler %s\n", self->name);

      // invoke instance method if exists
      PYTHON_CA_LOCK;
      retval =
        PyObject_CallMethod((PyObject *) self, "eventhandler",
                            NULL);

      if (!retval)
        {
          Debug(0, "ERROR in HandlerCallback! %s\n", self->name);

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

  // invoke function callback function if exists
  raise_all(self);

  Debug(10, "%s done\n", self->name);
}

/*******************************************************************
 * NAME:
 *  pv_monitor
 *
 * DESCRIPTION:
 *  start/stop/freeze monitor
 *
 *
 * WARNING:
 *
 *
 * RETURNS:
 *  Py_None:
 *    success
 *  null:
 *    error
 *
 * PYTHON API:
 *  pv.monitor([1=on|0=0ff|-1=freeze])
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

PyObject * pv_monitor(pvobject * self, /* self reference */
                      PyObject * args  /* optional attribute */
                     )
{
  int status = ECA_NORMAL;
  int oof = mon_on; /* on, off, freeze, on by default */

  if (!PyArg_ParseTuple(args, "|i", &oof))
    { PYCA_USAGE("monitor([1=on|0=0ff|-1=freeze])"); }

  if (oof < mon_freeze || oof > mon_on)
    { PYCA_USAGE("monitor([1=on|0=0ff|-1=freeze])"); }

  Debug(5, "monitor pv: %s - oof %s\n", self->name,
        oof == 1 ? "on" : oof == 0 ? "off" : "freeze");

  self->mon = oof;

  if (oof == mon_on)    /* on */
    {
      // if the channel is valid and is connected and we never subscribed
      if (self->chanId && (ca_state(self->chanId) == cs_conn)
          && (self->eventId == NULL))
        {
          /* register callback - I don't pass any args as those are retreived from the chid -> puser (pointing to self) */
          Debug(2, "%s ca_create_subscription\n", self->name);
          status =
            ca_create_subscription(dbf_type_to_DBR_TIME
                                   (xxx_ca_field_type
                                    (self->chanId)),
                                   ca_element_count
                                   (self->chanId), self->chanId,
                                   DBE_VALUE | DBE_ALARM,
                                   pv_update, 0,
                                   &self->eventId);

          if (status != ECA_NORMAL)
            {
              PYCA_ERR("channel access: %s",
                       ca_message(status));
            }

          // send out the request
          if (self->syncT >= 0)
            {
              if ((status =
                     ca_pend_io(self->syncT)) != ECA_NORMAL)
                PYCA_ERR("channel access: %s",
                         ca_message(status));
            }

          else
            {
              if ((status = ca_flush_io()) != ECA_NORMAL)
                PYCA_ERR("channel access: %s",
                         ca_message(status));
            }
        }

      else
        {
          // if monitor() already monitored, noop
          // if monitor() invoked prior to the connection being established
          // our connection handler will take care to start monitoring looking at self->mon
          Debug(5,
                "%s already monitored or not connected yet -> NOOP\n",
                self->name);
        }
    }

  else
    {
      /* if no monitor exists, skip that entirely */
      if (self->eventId)
        {
          status = ca_clear_subscription(self->eventId);

          if (status != ECA_NORMAL)
            PYCA_ERR("channel access: %s",
                     ca_message(status));

          self->eventId = NULL;

          if (self->syncT >= 0)
            {
              if ((status =
                     ca_pend_io(self->syncT)) != ECA_NORMAL)
                PYCA_ERR("channel access: %s",
                         ca_message(status));
            }

          else
            {
              if ((status = ca_flush_io()) != ECA_NORMAL)
                PYCA_ERR("channel access: %s",
                         ca_message(status));
            }
        }

      if (oof == mon_off)   /* keep cbList for mon_freeze */
        {
          Py_XDECREF(self->cbList);
          self->cbList = NULL;
        }
    }

  Py_INCREF(Py_None);
  return Py_None;
}

/*******************************************************************
 * NAME:
 *  pv_eventhandler - default class method invoked on monitor callback
 *
 * DESCRIPTION:
 * vitual method
 *
 * WARNING:
 * do not implement
 *
 * RETURNS:
 *
 * PYTHON API:
 *
 * PERSONNEL:
 *    Matthieu Bec, Gemini Software Group - mbec@gemini.edu
 *
 * HISTORY:
 *    25/11/05 - first documented version
 *
 *******************************************************************/
PyObject * pv_eventhandler(pvobject * self, /* self reference */
                           PyObject * args /* optional attribute */
                          )
{

  PYCA_ERR("eventhandler: virtual in this context");

  // Py_INCREF(Py_None);
  // return Py_None;
}
