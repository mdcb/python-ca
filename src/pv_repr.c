#include "ca_module.h"
#include "pv_repr.h"

static char const rcsid[] = "$Id: pv_repr.c 23769 2010-02-09 13:32:00Z mdcb $";

/*******************************************************************
 * NAME
 *  pv_repr - python internal representation
 *
 * DESCRIPTION
 *	 python internal representation
 *
 * RETURNS:
 *
 *
 * PERSONNEL:
 *  	Matthieu Bec, ING Software Group -  mdcb@ing.iac.es\b
 * 	
 * HISTORY:
 *  	30/06/99 - first documented version
 *  	12/05/04 - would seg. fault if not connected
 *               repr only gives basic info so just return that
 *
 *******************************************************************/
PyObject *pv_repr(pvobject * self)
{

	// default implementation

	Debug(5, "pv_repr\n");

	return PyString_FromFormat("<pythonCA.pv object at 0x%p>",
				   self->chanId);
}
