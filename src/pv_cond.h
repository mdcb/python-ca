#if !defined(PV_COND_H)
#define PV_COND_H

#ifdef __cplusplus
extern "C" {
#endif

PyObject * pv_cond_wait(pvobject *, PyObject *);
PyObject * pv_cond_signal(pvobject *, PyObject *);

#ifdef __cplusplus
}
#endif
#endif
