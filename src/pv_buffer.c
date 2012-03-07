#include "Python.h"
#include "structmember.h"
//#include "patchlevel.h"

#include "ca_module.h"

#if PY_VERSION_HEX < 0x02050000
typedef int Py_ssize_t;
#endif

/* ssize_t-based buffer interface */
static Py_ssize_t
pv_getreadbuffer(PyObject * self, Py_ssize_t segment, void **ptrptr)
{
	int numbytes;
	Debug(2, "pv_getreadbuffer self 0x%p segment %ld ptrptr 0x%p\n", self,
	      segment, ptrptr);
	pvobject *myself = (pvobject *) self;
	if (!myself->buff) {
		*ptrptr = NULL;
		numbytes = 0;
	} else {
		chtype type = xxx_ca_field_type(myself->chanId);
		numbytes =
		    dbr_value_size[type] * ca_element_count(myself->chanId);
		*ptrptr =
		    dbr_wavevalue_ptr(myself->buff, dbf_type_to_DBR_TIME(type),
				      0);
	}
	Debug(2, "   returned buff %p myselfbuf %p numbytes %d\n", *ptrptr,
	      myself->buff, numbytes);
	return numbytes;
}

static Py_ssize_t
pv_getwritebuffer(PyObject * self, Py_ssize_t segment, void **ptrptr)
{
	Debug(2, "pv_getwritebuffer self 0x%p segment %ld ptrptr 0x%p\n", self,
	      segment, ptrptr);
	*ptrptr = NULL;
	PyErr_Format(PyExc_TypeError, "writebuffer unsupported.");
	return -1;
}

static Py_ssize_t pv_getsegcount(PyObject * self, Py_ssize_t * lenp)
{
	Py_ssize_t segcount;
	Debug(2, "pv_getsegcount self 0x%p lenp 0x%p\n", self, lenp);
	if (lenp == NULL) {
		segcount = 1;
	} else {
		segcount = 1;
	}
	Debug(2, "   returned segcount %ld\n", segcount);
	return segcount;
}

static Py_ssize_t
pv_getcharbuffer(PyObject * self, Py_ssize_t segment, char **ptrptr)
{
	Debug(0,
	      "pv_getcharbuffer self 0x%p segment %ld ptrptr 0x%p, not implemented!\n",
	      self, segment, ptrptr);
	return -1;
}

#if PY_VERSION_HEX >= 0x02060000
static int pv_getbuffer(PyObject * self, Py_buffer * buf, int i)
{
	Debug(0, "pv_getbuffer self 0x%p buf %p i 0x%d, not implemented!\n",
	      self, buf, i);
	return -1;
}

static void pv_releasebuffer(PyObject * self, Py_buffer * buf)
{
	Debug(0, "pv_releasebuffer self 0x%p buf %p, not implemented!\n", self,
	      buf);
	return;
}
#endif

PyBufferProcs pv_buffer_procs = {
	pv_getreadbuffer,
	pv_getwritebuffer,
	pv_getsegcount,
	pv_getcharbuffer
#if PY_VERSION_HEX >= 0x02060000
	    ,
	pv_getbuffer,
	pv_releasebuffer
#endif
};
