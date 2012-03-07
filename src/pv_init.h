#if !defined(PV_INIT_H)
#define PV_INIT_H

#define CA_PRIORITY_PYTHON_CA CA_PRIORITY_DEFAULT
//    feel lucky
//#define CA_PRIORITY_PYTHON_CA 50

#ifdef __cplusplus
extern "C" {
#endif

	int pv_init(PyObject * self, PyObject * args, PyObject * kwds);
	PyObject *pv_new(PyTypeObject * type, PyObject * args, PyObject * kwds);

#ifdef __cplusplus
}
#endif
#endif
