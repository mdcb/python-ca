#if !defined(PYTHON_CA_H)
#define PYTHON_CA_H

/*******************************************************************
 * include
 *******************************************************************/

#include "Python.h"

#include <stdarg.h>
#include <assert.h>

#include "cadef.h"
#include "epicsEvent.h"

#include "ca_module_admin.h"
#include "ca_module_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************
 * defined / static
 *******************************************************************/

#ifndef PYTHON_CA_VERSION
#define PYTHON_CA_VERSION "0.0.0"
#endif

/* a common holder for error message */
	extern PyObject *python_ca_error;
	extern int python_ca_destroyed;

static inline chtype xxx_ca_field_type(chid id) {
		chtype a;

		 a = ca_field_type(id);
		if (ca_module_admin_enum_as_string && (a == DBR_ENUM))
			return DBR_STRING;
		else
			 return a;
	}

	extern epicsEventId pyCAevent;

/*******************************************************************
 * typedef
 *******************************************************************/

	typedef enum {
		mon_freeze = -1,
		mon_off,
		mon_on
	} mon_state;

	typedef struct {
		PyObject_HEAD char *name;	/* ... */
		chid chanId;	/* channel id */
		evid eventId;	/* event id */
		dbr_time_buff *buff;	/* time+data buffer */
		float syncT;	/* async or sync timeout for pend_io */
		int mon;	/* monitor requested */
		int hidden;	/* hide from dictionary lookup */
		epicsEventId conid;	/* pthread condition id */
		PyObject *cbList;	/* callback list, for add/remCb */
		PyObject *cbH;	/* callback handler, for derived types */
		PyObject *cbGet;	/* callback handler for get */
		PyObject *cbPut;	/* callback handler for put */
		PyObject *connH;	/* connection handler */
		PyObject *gpdict;	/* general purpose dict */
		PyObject *weakreflist;	/* weak references support */
	} pvobject;

/*******************************************************************
 * debug
 *******************************************************************/

#ifdef PYTHON_CA_NODEBUG
#define Debug(level,fmt,...) ;
#else
	pthread_t main_thread_ctxt;
#define Debug(level, fmt, ...) { \
    if(level<=ca_module_admin_debug_level) { \
       struct timeval tv_now;\
       gettimeofday(&tv_now, NULL);\
       fprintf (stderr,(pthread_self()==main_thread_ctxt)?"\x1b[32m":"\x1b[33m"); \
       fprintf (stderr,"(%02d) %ld.%06ld [%s] %15s:%-4d - %15s: ",level,tv_now.tv_sec,tv_now.tv_usec,(pthread_self()==main_thread_ctxt)?"__main__":"__asyn__",__FILE__,__LINE__,__PRETTY_FUNCTION__); \
       fprintf (stderr, fmt, ## __VA_ARGS__); \
       fprintf (stderr,"\x1b[0m"); \
       fflush (stderr); \
    } \
 }
#endif

/*******************************************************************
 * macro
 *******************************************************************/

#define PYCA_ERRMSG(fmt, ...) \
   PyErr_Format(python_ca_error, fmt, ## __VA_ARGS__)

#define PYCA_ERR(fmt, ...) {\
   PYCA_ERRMSG(fmt, ## __VA_ARGS__);\
   return NULL;\
   }

#define PYCA_USAGE(fmt, ...) {\
   PyErr_Format(python_ca_error, "usage: " fmt, ## __VA_ARGS__);\
   return NULL;\
   }

#define PYCA_CAERR(status) {\
   PyErr_Format(python_ca_error, "%s", ca_message(status)));\
   return NULL;\
   }
#define PYCA_SEVCHK(CA_ERROR_CODE, MESSAGE_STRING) {\
    int ca_unique_status_name  = (CA_ERROR_CODE); \
    if(ca_unique_status_name != ECA_NORMAL) {\
        PyErr_Format(python_ca_error, "%s - %s (file:%s line:%d)",\
            (MESSAGE_STRING), \
            ca_message(ca_unique_status_name), \
            __FILE__, \
            __LINE__); \
      return NULL;\
      };\
   }

#define PYCA_ERRNONE() {\
    PyErr_SetNone(python_ca_error);PyErr_Clear();\
   }

#define PYCA_ASSERTSTATE(state) {\
	if (state == cs_never_conn) { PYCA_ERR("cs_never_conn: valid chid, IOC not found"); }\
	else if (state == cs_prev_conn) { PYCA_ERR("cs_prev_conn: valid chid, IOC was found, but unavailable");}\
	else if (state == cs_closed) { PYCA_ERR("cs_closed: invalid chid");}\
	}

/*
 * statically set when init starts.
 * ca_preemtive_callback_is_enabled() return 0 when there's a disconnect
 */
	extern int python_ca_preemtive_callback_is_enabled;

#if defined(PYTHON_CA_USE_GIL_API)
#define PYTHON_CA_LOCK                                         \
   PyGILState_STATE __gstate=PyGILState_UNLOCKED;              \
   if (python_ca_preemtive_callback_is_enabled) {                   \
   __gstate = PyGILState_Ensure();                             \
   Debug (3, "locked\n");                                     \
   }
#define PYTHON_CA_UNLOCK                                      \
   if (python_ca_preemtive_callback_is_enabled) {                   \
   Debug (3, "unlocking\n");                                  \
   PyGILState_Release(__gstate);                               \
   }
#else
	extern PyThreadState *pyCAThreadState;
// same as below but better. whatever that meant
#define PYTHON_CA_LOCK                                         \
   if (python_ca_preemtive_callback_is_enabled) {                   \
   PyEval_AcquireThread(pyCAThreadState);                      \
   Debug (3, "locked\n");                                     \
   }
#define PYTHON_CA_UNLOCK                                       \
   if (python_ca_preemtive_callback_is_enabled) {                   \
   Debug (3, "unlocking\n");                                  \
   PyEval_ReleaseThread(pyCAThreadState);                      \
   }
#endif
#ifdef __cplusplus
}
#endif

#endif
