#if !defined(PV_GETTERSETTER_H)
#define PV_GETTERSETTER_H

#ifdef __cplusplus
extern "C" {
#endif

	PyObject *pv_getter_pvstats(pvobject * self, void *closure);
	PyObject *pv_getter_pvval(pvobject * self, void *closure);
	PyObject *pv_getter_pvtsval(pvobject * self, void *closure);

	PyObject *pv_dict_subscript(pvobject * mp, register PyObject * key);

#ifdef __cplusplus
}
#endif
#endif
