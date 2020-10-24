#include "ca_module.h"
#include "pv_init.h"
#include "pv_get.h"
#include "pv_monitor.h"

#include "ca_module_admin.h"

/*******************************************************************
 * connectHandler
 *******************************************************************/
static void connectHandler(struct connection_handler_args args)
{
	pvobject *self;
	PyObject *retval;

	// FIXME - debug
	char *cs_string[] = {
		"cs_never_conn",
		"cs_prev_conn",
		"cs_conn",
		"cs_closed"
	};

	if (!args.chid) {
		Debug(0, "invalid args.chid\n");
		return;
	}

	self = (pvobject *) ca_puser(args.chid);

	if (!self) {
		Debug(0, "invalid self\n");
		return;
	}

	enum channel_state cs = ca_state(args.chid);

	Debug(5, "%s state: %s\n", self->name, cs_string[cs]);

	// don't go there if we're quiting
	if (python_ca_destroyed) {
		Debug(2, "%s python_ca_destroyed %d\n", self->name,
		      python_ca_destroyed);
		return;
	}

	if (self->connH) {
		// invoke connection handler
		Debug(2, "%s invoking connection handler\n", self->name);

		//Debug(2, "ca_current_context is %d\n", ca_current_context());
		//Debug(2, "ca_preemtive_callback_is_enabled is %d\n", ca_preemtive_callback_is_enabled());

		// atomicity ...

		PYTHON_CA_LOCK;
		if (!python_ca_destroyed) {
			retval =
			    PyObject_CallFunctionObjArgs(self->connH, self,
							 NULL);
			if (retval == NULL) {
				// FIXME: redundant?
				Debug(0,
				      "%s error occured while invoking connH\n",
				      self->name);
				if (PyErr_Occurred()) {
					PyErr_Print();
					PyErr_Clear();
				}
			} else {
				Py_DECREF(retval);
			}
		}

		PYTHON_CA_UNLOCK;
	}

	if (cs == cs_conn) {

		// if callback exist, subscribe now
		if ((self->cbList || self->cbH || self->mon == mon_on
		     || self->mon == mon_freeze) && self->eventId == NULL) {
			int status;

			Debug(2, "%s ca_create_subscription\n", self->name);

			status =
			    ca_create_subscription(dbf_type_to_DBR_TIME
						   (xxx_ca_field_type
						    (args.chid)),
						   ca_element_count(args.chid),
						   args.chid,
						   DBE_VALUE | DBE_ALARM,
						   pv_update, 0,
						   &self->eventId);
			if (status != ECA_NORMAL) {
				Debug(0, "%s ca_create_subscription gave %s\n",
				      self->name, ca_message(status));
				return;
			}

			if (self->syncT >= 0) {
				if ((status =
				     ca_pend_io(self->syncT)) != ECA_NORMAL) {
					Debug(0, "%s ca_pend_io gave %s\n",
					      self->name, ca_message(status));
					return;
				}
			} else {
				if ((status = ca_flush_io()) != ECA_NORMAL) {
					Debug(0, "%s ca_flush_io gave %s\n",
					      self->name, ca_message(status));
					return;
				}
			}
		} else {
			Debug(2, "%s connected\n", self->name);
		}
	} else if (cs == cs_never_conn) {
		Debug(4, "%s never connected\n", ca_name(self->chanId));
	} else if (cs == cs_prev_conn) {
		Debug(4, "%s lost connection\n", ca_name(self->chanId));
	} else if (cs == cs_closed) {
		Debug(1, "%s connection closed\n", ca_name(self->chanId));
	} else {
		Debug(1, "%s unexpected cs %d\n", ca_name(self->chanId), cs);
	}

}

/*******************************************************************
 * init
 *******************************************************************/

int pv_init(PyObject * self, PyObject * args, PyObject * kwds)
{
	int status;

	pvobject *myself = (pvobject *) self;

	if (myself->chanId) {
		Debug(5, "pv reference exist, skip init %s\n", myself->name);
		return 0;
	}
	// derived class? check if there's a callback handler
	myself->cbH = PyObject_GetAttrString((PyObject *) self, "eventhandler");
	if (!PyMethod_Check(myself->cbH)) {
		Py_XDECREF(myself->cbH);
		myself->cbH = NULL;
	} else {
		Debug(5, "instance implements eventhandler\n");
		// FIXME ??
		Py_XDECREF(myself->cbH);
	}

	Debug(5, "pv init %s\n", myself->name);

	/* set new link */
	if (myself->syncT >= 0) {
		Debug(10, "ca_create_channel %s\n", myself->name);
		status =
		    ca_create_channel(myself->name, NULL, NULL,
				      CA_PRIORITY_PYTHON_CA, &myself->chanId);
		if (status != ECA_NORMAL) {
			PYCA_ERRMSG("ca_create_channel: %s",
				    ca_message(status));
			return -1;
		}
		/* set puser to self */
		ca_set_puser(myself->chanId, (void *)myself);
		Debug(10, "ca_pend_io %f\n", myself->syncT);
		if ((status = ca_pend_io(myself->syncT)) != ECA_NORMAL) {
			PYCA_ERRMSG("ca_pend_io: %s", ca_message(status));
			return -1;
		}
	} else {
		Debug(10, "ca_create_channel %s\n", myself->name);
		status =
		    ca_create_channel(myself->name, connectHandler, NULL,
				      CA_PRIORITY_PYTHON_CA, &myself->chanId);
		if (status != ECA_NORMAL) {
			PYCA_ERRMSG("ca_create_channel: %s",
				    ca_message(status));
			return -1;
		}
		/* set puser to self */
		ca_set_puser(myself->chanId, (void *)myself);
		Debug(10, "ca_flush_io%s\n", "");
		if ((status = ca_flush_io()) != ECA_NORMAL) {
			PYCA_ERRMSG("ca_flush_io: %s", ca_message(status));
			return -1;
		}
	}

	/* all your bases are belong to us */
	if (!myself->hidden)
		allyourbases_insert(myself->name, self);

	return 0;
}

/*******************************************************************
 * new
 *******************************************************************/

PyObject *pv_new(PyTypeObject * type, PyObject * args, PyObject * kwds)
{

	static char *kwlist[] = { "name", "syncT", "connH", "hidden", NULL };
	char *pvname;
	PyObject *connH = NULL;
	float syncT = ca_module_admin_syncT;
	int hidden = 0;

	if (!PyArg_ParseTupleAndKeywords
	    (args, kwds, "s|fOi", kwlist, &pvname, &syncT, &connH, &hidden)) {
		PYCA_USAGE("pv('pvname'[,syncT=timeout|-1,connH=callable])");
	}

	if (!strcmp(pvname, "")) {
		PYCA_USAGE("pv('pvname'[,syncT=timeout|-1,connH=callable])");
	}
	// ignore None connH
	if (connH == Py_None) {
		connH = NULL;
	}

	if (connH && !PyCallable_Check(connH)) {
		PYCA_USAGE("pv('pvname'[,syncT=timeout|-1,connH=callable])");
	}
	// shorten pv name if .VAL was provided
	int n_length = strlen(pvname);
	if (!strcmp(pvname + n_length - 4, ".VAL")) {
		n_length -= 4;
	}
	// fix one-shot ca.get/ca.put for pv already created with syncT=-1

	char *pvshortname = (char *)calloc(n_length + 1, sizeof(char));
	strncpy(pvshortname, pvname, n_length);

	pvobject *pself;

	// look up, unless hidden

	PyObject *lookup = NULL;

	if (!hidden)
		lookup = allyourbases_find(pvshortname);

	if (lookup != NULL) {
		// we don't need another reference
		free(pvshortname);
		// grab a ref to the existing object
		pself = (pvobject *) lookup;
		// check syncT is same type
		if (syncT * pself->syncT < 0) {
			PYCA_ERRMSG("pv already exist but syncT mode differs");
			return NULL;
		} else {
			// debatable, take the more stringent val for syncT
			if (syncT < pself->syncT)
				pself->syncT = syncT;
		}
		// FIXME - would be nicer to have connH added to a list of callbacks
		// handle it strictly for now
		if (pself->connH) {
			PYCA_ERRMSG
			    ("pv already exist, support for additional connH not implemented");
			return NULL;
		}
		// we passed the tests, bump the reference count
		Py_INCREF((PyObject *) pself);
	} else {
		pself = (pvobject *) type->tp_alloc(type, 0);
		// fill in default structure
		pself->name = pvshortname;
		pself->chanId = NULL;
		pself->eventId = NULL;
		pself->cbList = NULL;
		pself->cbGet = NULL;
		pself->cbPut = NULL;
		pself->cbH = NULL;
		Py_XINCREF(connH);
		pself->connH = connH;
		pself->syncT = syncT;
		pself->mon = 0;
		pself->buff = NULL;
		pself->conid = NULL;
		pself->weakreflist = NULL;
		pself->hidden = hidden;
	}

	return (PyObject *) pself;
}
