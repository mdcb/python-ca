#include "ca_module.h"
#include "pv_addCb.h"
#include "pv_monitor.h"

/*******************************************************************
 * NAME:
 *  pv_addCb
 *
 * DESCRIPTION:
 *
 * WARNING:
 *
 * RETURNS:
 *
 * PYTHON API:
 *  pv.addCb(fun,args,kwds)
 *
 * PERSONNEL:
 *    Matthieu Bec, Gemini Software Group -  mbec@gemini.edu
 *
 * HISTORY:
 *
 *******************************************************************/
PyObject * pv_addCb(pvobject * self, /* self reference */
                    PyObject * args, /* optional attribute */
                    PyObject * kw  /* optional keywords attribute */
                   )
{
  PyObject * func;

  Debug(10, "pv_addCb\n");

  if (!self->chanId)
    { PYCA_ASSERTSTATE(cs_closed); }

  int s = PyTuple_Size(args);

  if (s == 0)
    { PYCA_USAGE("addCb(func,*args,**kwds)"); }

  func = PyTuple_GetItem(args, 0);

  if (!PyCallable_Check(func))
    { PYCA_ERR("arg1 not callable"); }

  // create a list of callback if it doesn't exist yet
  if (!self->cbList)
    { self->cbList = PyList_New(0); }

  PyObject * cb = PyTuple_New(2);

  Py_XINCREF(args);
  PyTuple_SetItem(cb, 0, args);

  Py_XINCREF(kw);
  PyTuple_SetItem(cb, 1, kw);

  if (PyList_Append(self->cbList, cb))
    {
      Py_XDECREF(func);
      Py_XDECREF(args);
      Py_XDECREF(kw);
      Py_XDECREF(cb);
      PYCA_ERR("failed to append cb.");
    }

  if (ca_state(self->chanId) == cs_conn)
    {
      Debug(10, "pv_addCb, channel is connected\n");

      // if the pv is connected and monitoring has not started, start it now
      if (self->eventId == NULL)
        {
          int status;
          Debug(10, "pv_addCb, ca_create_subscription now..\n");
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

          if (self->syncT >= 0)
            {
              if ((status =
                     ca_pend_io(self->syncT)) != ECA_NORMAL)
                {
                  PYCA_ERRMSG("ca_pend_io: %s",
                              ca_message(status));
                  return NULL;
                }
            }

          else
            {
              if ((status = ca_flush_io()) != ECA_NORMAL)
                {
                  PYCA_ERRMSG("ca_flush_io: %s",
                              ca_message(status));
                  return NULL;
                }
            }
        }

      else
        {
          Debug(10, "pv_addCb, con and mon -> raise now..\n");
          // if the pv is connected and is already monitored, trigger the callback
          // this is so multiple/late addCb behave as if the channel had just connected
          raise_one(self, cb);
        }
    }

  else
    {
      Debug(10, "pv_addCb, delayed (not conn)..\n");
      // we're adding a callback on a pv that has not connected yet
      // monitoring will start in the connection handler when it detects
      // cbList is not NULL
    }

  Py_XDECREF(cb);
  Py_INCREF(Py_None);

  return Py_None;
}

/*******************************************************************
 * NAME:
 *  pv_cb_compare
 *
 * DESCRIPTION:
 * Compare callback arguments
 *
 *******************************************************************/

static int PyObj_XComp(PyObject * obj1, PyObject * obj2)
{
  // obj1 and obj2 are list of two elements
  // [0] is a tuple [func,*args] where args can be NULL
  // [1] is a dictionary { *kw } where kw can be NULL
  //int diff
  PyObject * cb1args = PyTuple_GetItem(obj1, 0);
  PyObject * cb1kw = PyTuple_GetItem(obj1, 1);
  PyObject * cb2args = PyTuple_GetItem(obj2, 0);
  PyObject * cb2kw = PyTuple_GetItem(obj2, 1);

  // no need to compare the same object or NULLs
  if (cb1kw != cb2kw)
    {
      // check if only one is NULL
      if (cb1kw == NULL || cb2kw == NULL)
        {
          return -1;
        }

      //XXX // none null, compare them
      //XXX PyObject_Cmp(cb1kw, cb2kw, &diff);
      //XXX if (diff)
      //XXX   {
      //XXX     return -1;
      //XXX   }
    }

  // move on the tuple now
  int len1 = PyTuple_Size(cb1args);
  int len2 = PyTuple_Size(cb2args);

  if (len1 != len2)
    {
      // length mismatch
      return -1;
    }

  for (int i = 0; i < len1; i++)
    {
      PyObject * elt1 = PyTuple_GetItem(cb1args, i);
      PyObject * elt2 = PyTuple_GetItem(cb2args, i);

      // no need to compare the same object or NULLs
      if (elt1 != elt2)
        {
          // check if only one is NULL
          if (elt1 == NULL || elt2 == NULL)
            {
              return -1;
            }

          //XXX // none null, compare them
          //XXX PyObject_Cmp(elt1, elt2, &diff);
          //XXX if (diff)
          //XXX   {
          //XXX     return -1;
          //XXX   }
        }
    }

  return 0;
}

/*******************************************************************
 * NAME:
 *  pv_remCb
 *
 * DESCRIPTION:
 *
 * WARNING:
 *
 * RETURNS:
 *
 * PYTHON API:
 *  pv.remCb(fun)
 *
 * PERSONNEL:
 *    Matthieu Bec, Gemini Software Group -  mbec@gemini.edu
 *
 * HISTORY:
 *
 *******************************************************************/
PyObject * pv_remCb(pvobject * self, /* self reference */
                    PyObject * args, /* optional attribute */
                    PyObject * kw  /* optional keywords attribute */
                   )
{

  PyObject * cblist_copy;
  int i, match_found = 0;

  Debug(10, "%s\n", self->name);

  if (!self->chanId)
    { PYCA_ASSERTSTATE(cs_closed); }

  // check there is list of callback
  if (!self->cbList || !PyList_Size(self->cbList))
    { PYCA_ERR("empty cbList"); }

  // check args list
  if (PyTuple_Size(args) == 0)
    { PYCA_USAGE("remCb(func,*args,**kwds)"); }

  // create a tuple with the callback args
  PyObject * cbargs_rem = PyTuple_New(2);
  Py_XINCREF(args);
  Py_XINCREF(kw);
  PyTuple_SetItem(cbargs_rem, 0, args);
  PyTuple_SetItem(cbargs_rem, 1, kw);

  cblist_copy = PyList_New(0);

  for (i = 0; i < PyList_Size(self->cbList); i++)
    {
      PyObject * cb = PyList_GetItem(self->cbList, i);
      Py_XINCREF(cb);

      if (!match_found)
        {
          int diff = PyObj_XComp(cbargs_rem, cb);

          if (diff)
            {
              // no match or already removed, append elt. to copy
              PyList_Append(cblist_copy, cb);
            }

          else
            {
              // match, skip from copy, mark it for rest of iteration
              Py_XDECREF(cb);
              match_found = 1;
            }
        }

      else
        {
          // match found, append all the rest
          PyList_Append(cblist_copy, cb);
        }
    }

  Py_XDECREF(cbargs_rem);

  if (!match_found)
    {
      Py_DECREF(cblist_copy);
      PYCA_ERR("remCb no match found");
    }

  else
    {
      Py_DECREF(self->cbList);
      self->cbList = cblist_copy;
    }

  Py_INCREF(Py_None);
  return Py_None;
}
