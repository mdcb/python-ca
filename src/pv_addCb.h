#if !defined(PV_ADDCB_H)
#define PV_ADDCB_H

#ifdef __cplusplus
extern "C" {
#endif
PyObject * pv_addCb(pvobject * self, PyObject * args, PyObject * kw);
PyObject * pv_remCb(pvobject * self, PyObject * args, PyObject * kw);
#ifdef __cplusplus
}
#endif
#endif
