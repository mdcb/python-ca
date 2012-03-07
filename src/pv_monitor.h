#if !defined(PV_MONITOR_H)
#define PV_MONITOR_H

#ifdef __cplusplus
extern "C" {
#endif

	PyObject *pv_monitor(pvobject * self, PyObject * args);
	PyObject *pv_eventhandler(pvobject * self, PyObject * args);
	void pv_update(struct event_handler_args args);
	void raise_one(pvobject * self, PyObject * cb);

#ifdef __cplusplus
}
#endif
#endif
