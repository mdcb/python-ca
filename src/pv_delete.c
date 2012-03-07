#include "ca_module.h"
#include "pv_delete.h"

#include "ca_module_admin.h"

static char const rcsid[] =
    "$Id: pv_delete.c 34672 2011-05-13 19:53:11Z mdcb $";

/*******************************************************************
 * NAME
 *  pv_delete - object destructor
 *
 * DESCRIPTION
 *	release pv object and free allocated ressources
 *
 * PYTHON API:
 *		Python >>> del pv\b
 *
 * PERSONNEL:
 *  	Matthieu Bec, ING Software Group -  mdcb@ing.iac.es\b
 * 	
 * HISTORY:
 *  	29/06/99 - first documented version
 *
 *******************************************************************/
void pv_delete(pvobject * self	/* self-reference */
    )
{

	Debug(4, "%s %p\n", self->name, self->chanId);
	// Debug(4,"%s\n",ca_current_context()?ca_name(self->chanId):"dead ca context");

	// take care of internal weakref
	if (self->weakreflist != NULL)
		PyObject_ClearWeakRefs((PyObject *) self);

	// take care of book keeping
	if (!self->hidden)
		allyourbases_erase(self->name);

	pv_clear(self);

	if (self->name) {
		free(self->name);
		self->name = NULL;
	}
	// PyMem_DEL(self);
	self->ob_type->tp_free((PyObject *) self);
}

int pv_traverse(pvobject * self, visitproc visit, void *arg)
{
// cbH is an internal reference for us, does not participate in the cycle

	int vret;

	Debug(2, "%s\n", self->name);

	if (self->cbList && (vret = visit(self->cbList, arg) != 0))
		return vret;
	if (self->cbGet && (vret = visit(self->cbGet, arg) != 0))
		return vret;
	if (self->cbPut && (vret = visit(self->cbPut, arg) != 0))
		return vret;
	if (self->connH && (vret = visit(self->connH, arg) != 0))
		return vret;
	if (self->gpdict && (vret = visit(self->gpdict, arg) != 0))
		return vret;

	return 0;
}

int pv_clear(pvobject * self)
{
// from the doc:
// """use of a temporary variable 
// so that we can set each member to NULL before decrementing its reference 
// count. We do this because, as was discussed earlier, if the reference count
// drops to zero, we might cause code to run that calls back into the object.
// In addition, because we now support garbage collection, we also have to
// worry about code being run that triggers garbage collection. If garbage 
// collection is run, our tp_traverse handler could get called. We can't take 
// a chance of having Noddy_traverse() called when a member's reference count 
// has dropped to zero and its value hasn't been set to NULL."""

	int status;
	PyObject *tmp;

	Debug(2, "pv_clear %s %p\n", self->name, self->chanId);

	tmp = self->cbList;
	self->cbList = NULL;
	Py_XDECREF(tmp);

	tmp = self->cbGet;
	self->cbGet = NULL;
	Py_XDECREF(tmp);

	tmp = self->cbPut;
	self->cbPut = NULL;
	Py_XDECREF(tmp);

	tmp = self->connH;
	self->connH = NULL;
	Py_XDECREF(tmp);

	// void the reference if some callbacks are pending
	// anything else left to python
	self->cbH = NULL;

	// if ca context is gone, too late for calling the CA api
	if (!python_ca_destroyed) {
		// stop subscriptions (FIXME - do we need that before clear?)
		if (self->eventId) {
			if ((status =
			     ca_clear_subscription(self->eventId)) !=
			    ECA_NORMAL) {
				PYCA_ERRMSG(ca_current_context()?
					    ca_name(self->chanId) :
					    "dead ca context");
				return -1;
			}
			self->eventId = NULL;
		}

		if (self->chanId) {
			status = ca_clear_channel(self->chanId);
			if (status != ECA_NORMAL) {
				PYCA_ERRMSG(ca_current_context()?
					    ca_name(self->chanId) :
					    "dead ca context");
				return -1;
			}
			self->chanId = NULL;
		}
	}

	if (self->conid) {
		epicsEventDestroy(self->conid);
		self->conid = NULL;
	}

	if (self->buff) {
		free(self->buff);
		self->buff = NULL;
	}
	//   if (self->name) {
	//       free(self->name);
	//       self->name=NULL;
	//    }

	return 0;
}
